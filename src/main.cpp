#include "udp_receiver.hpp"
// #include "packet_parser.hpp"
#include "data_book.hpp"
#include "composite_score_calculator.hpp"
#include "tcp_sender.hpp"
#include "logger.hpp"
// #include "logger.cpp"
#include "process_packet_core.hpp"
#include "types.hpp"
#include "parser_utils.hpp" 

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <vector>

std::atomic<bool> keep_running(true);

void signalHandler(int) {
    keep_running = false;
    std::cerr << "\n[INFO] Caught SIGINT, stopping receiver...\n";
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <mcast_ip> <mcast_port> <interface> <endpointA_host> <endpointA_port>\n";
        return 1;
    }

    std::string mcast_ip = argv[1];
    uint16_t mcast_port = static_cast<uint16_t>(std::stoi(argv[2]));
    std::string interface_name = argv[3];
    std::string endpointA_host = argv[4];
    uint16_t endpointA_port = static_cast<uint16_t>(std::stoi(argv[5]));

    DataBookManager book_manager;
    CompositeScoreCalculator calculator;
    TcpSender sender(endpointA_host, endpointA_port);
    std::vector<LatencySample> latency_samples;


    if (!sender.connect()) {
        std::cerr << "Failed to connect to Destination Endpoint at " << endpointA_host << ":" << endpointA_port << "\n";
        return 1;
    }

    UdpReceiver receiver(mcast_ip, mcast_port, interface_name);
    std::signal(SIGINT, signalHandler);

    std::thread recv_thread([&]() {
        receiver.start([&](const uint8_t* data, size_t len) {
            

            ProcessedMessage processed_msg;
            if (!parse_data_packet(data, len, processed_msg)) {
                std::cerr << "[ERROR] Failed to parse incoming UDP packet\n";
            } else {
                // process_decoded_packet(processed_msg, book_manager, calculator, nullptr, nullptr, -1, &sender);
                // process_decoded_packet(processed_msg, book_manager, calculator, &latency_samples, nullptr, -1, &sender);
                process_decoded_packet(processed_msg, book_manager, calculator, &latency_samples, nullptr, -1, &sender);

            }

        });
    });

    for (int i = 0; i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    keep_running = false;





    receiver.stop();
    recv_thread.join();

    // std::cout << "[DEBUG] Total latency samples: " << latency_samples.size() << "\n";

    // if (!latency_samples.empty()) {
    //     dump_latency_trace(latency_samples);
    // }

    return 0;
}
