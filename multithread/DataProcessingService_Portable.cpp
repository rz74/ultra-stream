// DataProcessingService_Portable.cpp - Portable version using standard C++ instead of Boost
#include "DataProcessingService.h"
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <queue>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

constexpr int DATA_BOOK_LEVELS = 10;

struct DataLevel {
    double value = 0.0;
    int volume = 0;
};

struct DataBook {
    std::array<DataLevel, DATA_BOOK_LEVELS> demand_levels;  // bid levels
    std::array<DataLevel, DATA_BOOK_LEVELS> supply_levels;  // ask levels
    mutable std::mutex mtx;
    
    void updateLevel(char side, int level, double value, int volume) {
        std::lock_guard<std::mutex> lock(mtx);
        if (level >= 0 && level < DATA_BOOK_LEVELS) {
            if (side == 'B') {  // demand (bid)
                demand_levels[level] = {value, volume};
            } else if (side == 'S') {  // supply (ask)
                supply_levels[level] = {value, volume};
            }
        }
    }
    
    double calculateCompositeScore() const {
        std::lock_guard<std::mutex> lock(mtx);
        double total_demand_value = 0.0;
        double total_supply_value = 0.0;
        int total_demand_volume = 0;
        int total_supply_volume = 0;
        
        for (int i = 0; i < DATA_BOOK_LEVELS; ++i) {
            if (demand_levels[i].volume > 0) {
                total_demand_value += demand_levels[i].value * demand_levels[i].volume;
                total_demand_volume += demand_levels[i].volume;
            }
            if (supply_levels[i].volume > 0) {
                total_supply_value += supply_levels[i].value * supply_levels[i].volume;
                total_supply_volume += supply_levels[i].volume;
            }
        }
        
        if (total_demand_volume == 0 && total_supply_volume == 0) return 0.0;
        if (total_demand_volume == 0) return total_supply_value / total_supply_volume;
        if (total_supply_volume == 0) return total_demand_value / total_demand_volume;
        
        double avg_demand = total_demand_value / total_demand_volume;
        double avg_supply = total_supply_value / total_supply_volume;
        return (avg_demand + avg_supply) / 2.0;
    }
};

class DataProcessingService {
private:
    std::unordered_map<int, DataBook> data_books;
    std::mutex data_books_mutex;
    std::queue<std::string> packet_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::atomic<bool> running{true};
    std::vector<std::thread> worker_threads;
    std::thread udp_receiver_thread;
    std::thread tcp_sender_thread;
    
    SOCKET udp_socket = INVALID_SOCKET;
    SOCKET tcp_socket = INVALID_SOCKET;
    
    std::string multicast_address;
    int port;
    
    std::atomic<int> packets_processed{0};
    std::atomic<int> packets_received{0};
    
public:
    DataProcessingService(const std::string& addr, int p) 
        : multicast_address(addr), port(p) {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("Failed to initialize Winsock");
        }
#endif
    }
    
    ~DataProcessingService() {
        stop();
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    void start() {
        std::cout << "Starting multithreaded data processing service..." << std::endl;
        std::cout << "Multicast: " << multicast_address << ":" << port << std::endl;
        
        // Start worker threads
        int num_workers = std::max(2, (int)std::thread::hardware_concurrency());
        std::cout << "Starting " << num_workers << " worker threads" << std::endl;
        
        for (int i = 0; i < num_workers; ++i) {
            worker_threads.emplace_back(&DataProcessingService::workerThread, this, i);
        }
        
        // Start UDP receiver thread
        udp_receiver_thread = std::thread(&DataProcessingService::udpReceiverThread, this);
        
        // Start TCP sender thread  
        tcp_sender_thread = std::thread(&DataProcessingService::tcpSenderThread, this);
        
        // Start statistics thread
        std::thread stats_thread(&DataProcessingService::statisticsThread, this);
        
        // Wait for threads
        if (udp_receiver_thread.joinable()) udp_receiver_thread.join();
        if (tcp_sender_thread.joinable()) tcp_sender_thread.join();
        
        for (auto& worker : worker_threads) {
            if (worker.joinable()) worker.join();
        }
        
        if (stats_thread.joinable()) stats_thread.join();
    }
    
    void stop() {
        running = false;
        queue_cv.notify_all();
        
        if (udp_socket != INVALID_SOCKET) {
            closesocket(udp_socket);
            udp_socket = INVALID_SOCKET;
        }
        if (tcp_socket != INVALID_SOCKET) {
            closesocket(tcp_socket);
            tcp_socket = INVALID_SOCKET;
        }
    }

private:
    void udpReceiverThread() {
        std::cout << "UDP receiver thread started" << std::endl;
        
        // Create UDP socket
        udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_socket == INVALID_SOCKET) {
            std::cerr << "Failed to create UDP socket" << std::endl;
            return;
        }
        
        // Set socket options
        int reuse = 1;
        setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
        
        // Bind socket
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(udp_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Failed to bind UDP socket" << std::endl;
            return;
        }
        
        // Join multicast group
        ip_mreq mreq{};
        inet_pton(AF_INET, multicast_address.c_str(), &mreq.imr_multiaddr);
        mreq.imr_interface.s_addr = INADDR_ANY;
        
        if (setsockopt(udp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) == SOCKET_ERROR) {
            std::cerr << "Failed to join multicast group" << std::endl;
            return;
        }
        
        std::cout << "Joined multicast group " << multicast_address << ":" << port << std::endl;
        
        char buffer[1024];
        while (running) {
            int bytes_received = recv(udp_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                
                // Add packet to queue
                {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    packet_queue.push(std::string(buffer, bytes_received));
                }
                queue_cv.notify_one();
                packets_received++;
            }
        }
        
        std::cout << "UDP receiver thread stopped" << std::endl;
    }
    
    void workerThread(int worker_id) {
        std::cout << "Worker thread " << worker_id << " started" << std::endl;
        
        while (running) {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [this] { return !packet_queue.empty() || !running; });
            
            if (!running) break;
            
            if (packet_queue.empty()) continue;
            
            std::string packet = packet_queue.front();
            packet_queue.pop();
            lock.unlock();
            
            // Process packet
            processPacket(packet, worker_id);
            packets_processed++;
        }
        
        std::cout << "Worker thread " << worker_id << " stopped" << std::endl;
    }
    
    void processPacket(const std::string& packet, int worker_id) {
        // Parse packet: subject_id,side,level,value,volume
        std::istringstream iss(packet);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 5) {
            try {
                int subject_id = std::stoi(tokens[0]);
                char side = tokens[1][0];
                int level = std::stoi(tokens[2]);
                double value = std::stod(tokens[3]);
                int volume = std::stoi(tokens[4]);
                
                // Update data book
                {
                    std::lock_guard<std::mutex> lock(data_books_mutex);
                    data_books[subject_id].updateLevel(side, level, value, volume);
                }
                
                // Calculate and output composite score
                double composite_score = data_books[subject_id].calculateCompositeScore();
                
                auto now = std::chrono::high_resolution_clock::now();
                auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                    now.time_since_epoch()).count();
                
                std::cout << "[WORKER=" << worker_id << "] [SUBJECT=" << subject_id 
                          << "] CompositeScore=" << std::fixed << std::setprecision(6) 
                          << composite_score << " [TS=" << timestamp << "]" << std::endl;
                          
            } catch (const std::exception& e) {
                std::cerr << "Error processing packet: " << e.what() << std::endl;
            }
        }
    }
    
    void tcpSenderThread() {
        std::cout << "TCP sender thread started" << std::endl;
        
        // For this demo, we'll just log that TCP would send data
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (running) {
                std::cout << "[TCP] Would send aggregated data to downstream systems" << std::endl;
            }
        }
        
        std::cout << "TCP sender thread stopped" << std::endl;
    }
    
    void statisticsThread() {
        std::cout << "Statistics thread started" << std::endl;
        
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            if (running) {
                std::cout << "\n=== STATISTICS ===" << std::endl;
                std::cout << "Packets received: " << packets_received.load() << std::endl;
                std::cout << "Packets processed: " << packets_processed.load() << std::endl;
                std::cout << "Active data books: ";
                {
                    std::lock_guard<std::mutex> lock(data_books_mutex);
                    std::cout << data_books.size() << std::endl;
                }
                std::cout << "==================\n" << std::endl;
            }
        }
        
        std::cout << "Statistics thread stopped" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    std::string multicast_address = "239.255.0.1";
    int port = 12345;
    
    if (argc >= 3) {
        multicast_address = argv[1];
        port = std::atoi(argv[2]);
    }
    
    try {
        DataProcessingService service(multicast_address, port);
        
        std::cout << "=== Multithreaded Data Processing Service ===" << std::endl;
        std::cout << "Configuration:" << std::endl;
        std::cout << "  Multicast Address: " << multicast_address << std::endl;
        std::cout << "  Port: " << port << std::endl;
        std::cout << "  CPU Cores: " << std::thread::hardware_concurrency() << std::endl;
        std::cout << "=============================================" << std::endl;
        
        service.start();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}