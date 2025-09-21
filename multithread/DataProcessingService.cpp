#include "DataProcessingService.h"
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <unordered_map>
#include <array>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <cstring>
#include <fstream>
#include <filesystem>


using boost::asio::ip::udp;

constexpr int DATA_BOOK_LEVELS = 10;

struct DataLevel {
    double value = 0.0;
    int volume = 0;
};

struct DataBook {
    std::array<DataLevel, DATA_BOOK_LEVELS> demand;
    std::array<DataLevel, DATA_BOOK_LEVELS> supply;
};

std::unordered_map<int, DataBook> data_books;
std::mutex data_mutex;

struct CompositeScoreRecord {
    int subject_id;
    int64_t composite_score;
};
std::vector<CompositeScoreRecord> results;
std::mutex result_mutex;

int64_t calculateCompositeScore(const DataBook& db) {
    const auto& best_demand = db.demand[0];
    const auto& best_supply = db.supply[0];
    if (best_demand.volume + best_supply.volume == 0) return 0;
    double cs = (best_demand.value * best_supply.volume + best_supply.value * best_demand.volume) / (best_demand.volume + best_supply.volume);
    return static_cast<int64_t>(cs * 1e9);
}

std::queue<std::vector<uint8_t>> task_queue;
std::mutex queue_mutex;
std::condition_variable queue_cv;
bool running = true;

void parse_and_update(const std::vector<uint8_t>& data) {
    if (data.size() < 10) return;

    size_t offset = 0;
    uint32_t msg_length;
    std::memcpy(&msg_length, &data[offset], 4); offset += 4;
    uint32_t subject_id;
    std::memcpy(&subject_id, &data[offset], 4); offset += 4;
    uint16_t num_updates;
    std::memcpy(&num_updates, &data[offset], 2); offset += 2;

    {
        std::lock_guard<std::mutex> lock(data_mutex);
        DataBook& db = data_books[subject_id];

        for (int i = 0; i < num_updates; ++i) {
            if (offset + 14 > data.size()) break;

            uint8_t level = data[offset]; offset += 1;
            uint8_t side = data[offset]; offset += 1;
            int64_t scaled_value;
            std::memcpy(&scaled_value, &data[offset], 8); offset += 8;
            uint32_t volume;
            std::memcpy(&volume, &data[offset], 4); offset += 4;

            double value = static_cast<double>(scaled_value) / 1e9;
            if (level >= DATA_BOOK_LEVELS) continue;

            if (side == 0) {
                db.demand[level] = { value, static_cast<int>(volume) };
            }
            else if (side == 1) {
                db.supply[level] = { value, static_cast<int>(volume) };
            }
        }
    }

    int64_t cs = 0;
    {
        std::lock_guard<std::mutex> lock(data_mutex);
        cs = calculateCompositeScore(data_books[subject_id]);
    }

    std::cout << "[SUBJECT=" << subject_id << "] CompositeScore: " << cs << std::endl;
    {
        std::lock_guard<std::mutex> lock(result_mutex);
        results.push_back({ static_cast<int>(subject_id), cs });
    }
}

void worker_thread() {
    while (running) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait(lock, [] { return !task_queue.empty() || !running; });
        if (!running && task_queue.empty()) break;

        auto data = std::move(task_queue.front());
        task_queue.pop();
        lock.unlock();
        std::cout << "[Thread " << std::this_thread::get_id() << "] processing..." << std::endl;

        parse_and_update(data);
    }
}

void async_receive_loop(std::shared_ptr<udp::socket> socket) {
    if (!socket || !socket->is_open()) {
        std::cerr << "[ERROR] Invalid or closed socket in async_receive_loop" << std::endl;
        return;
    }

    auto recv_buf = std::make_shared<boost::array<uint8_t, 2048>>();
    auto sender_endpoint = std::make_shared<udp::endpoint>();

    socket->async_receive_from(
        boost::asio::buffer(*recv_buf), *sender_endpoint,
        [socket, recv_buf, sender_endpoint](
            const boost::system::error_code& error, 
            std::size_t len) {
            if (!running) {
                return; 
            }

            if (!error && len > 0) {
                try {
                    std::vector<uint8_t> data(recv_buf->begin(), recv_buf->begin() + len);
                    {
                        std::lock_guard<std::mutex> lock(queue_mutex);
                        task_queue.push(std::move(data));
                    }
                    queue_cv.notify_one();
                }
                catch (const std::exception& e) {
                    std::cerr << "[ERROR] Failed to process received data: " 
                             << e.what() << std::endl;
                }
            }
            else if (error && running) {
                std::cerr << "[ERROR] Receive error: " << error.message() << std::endl;
            }

            if (running && socket && socket->is_open()) {
                async_receive_loop(socket);  
            }
        }
    );
}

void start_receiver(boost::asio::io_context& io_context, const std::string& multicast_address, short port) {
    auto socket = std::make_shared<udp::socket>(io_context);

    udp::endpoint listen_endpoint(udp::v4(), port);
    socket->open(listen_endpoint.protocol());
    socket->set_option(boost::asio::ip::udp::socket::reuse_address(true));
    socket->bind(listen_endpoint);

    boost::asio::ip::address multicast_addr = boost::asio::ip::make_address(multicast_address);
    socket->set_option(boost::asio::ip::multicast::join_group(multicast_addr));

    std::cout << "[INFO] Listening on " << multicast_address << ":" << port << std::endl;

    async_receive_loop(socket);
}

int main(int argc, char* argv[]) {
    std::cout << "[DEBUG] Output path: " << std::filesystem::current_path() << std::endl;

    try {
        std::string multicast_address;
        short port;

        
        if (argc < 3) {
            multicast_address = "239.255.0.1"; 
            port = 30001;                      
            std::cout << "[INFO] Using default values - address: " 
                     << multicast_address << ", port: " << port << std::endl;
        }
        else {
            multicast_address = argv[1];
            port = static_cast<short>(std::stoi(argv[2]));
            std::cout << "[INFO] Using provided values - address: " 
                     << multicast_address << ", port: " << port << std::endl;
        }

        boost::asio::io_context io_context;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> 
            work_guard(io_context.get_executor());

        
        try {
            start_receiver(io_context, multicast_address, port);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to start receiver: " << e.what() << std::endl;
            return 1;
        }

      
        std::thread io_thread([&io_context]() {
            try {
                io_context.run();
            }
            catch (const std::exception& e) {
                std::cerr << "[ERROR] IO context error: " << e.what() << std::endl;
            }
        });

        const int NUM_WORKERS = 4;
        std::vector<std::thread> workers;
        workers.reserve(NUM_WORKERS); 

        for (int i = 0; i < NUM_WORKERS; ++i) {
            workers.emplace_back(worker_thread);
        }

        std::cout << "[INFO] Data processing service started. Press ENTER to quit.\n";
        std::cin.get();

       
        std::cout << "[INFO] Initiating shutdown..." << std::endl;
        running = false;
        queue_cv.notify_all();
        io_context.stop();

     
        for (auto& t : workers) {
            if (t.joinable()) {
                t.join();
            }
        }

        if (io_thread.joinable()) {
            io_thread.join();
        }
      
        {
            std::ofstream ofs("output.csv");
            ofs << "subject_id,composite_score\n";
            std::lock_guard<std::mutex> lock(result_mutex);
            for (const auto& r : results) {
                ofs << r.subject_id << "," << r.composite_score << "\n";
            }
            std::cout << "[INFO] Exported composite score results to output.csv\n";
        }

        std::cout << "[INFO] Shutdown complete." << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << std::endl;
        return 1;
    }
}
