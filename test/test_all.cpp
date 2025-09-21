// test_all.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <csignal>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../include/types.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <csignal>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../include/types.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <arpa/inet.h>

void send_udp_packets(const std::string& path, const std::string& ip, uint16_t port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "[ERROR] Cannot open " << path << "\n";
        exit(1);
    }

    int packet_count = 0;
    while (true) {
        uint32_t net_len;
        if (!in.read(reinterpret_cast<char*>(&net_len), sizeof(net_len))) {
            if (packet_count == 0) {
                std::cerr << "[WARN] input file empty or missing length field\n";
            }
            break;
        }
        uint32_t len = ntohl(net_len); // Length of the body

        if (len == 0 || len > 4096) {
            std::cerr << "[ERROR] Invalid packet length: " << len << "\n";
            break;
        }

        std::vector<char> buf(sizeof(net_len) + len);
        std::memcpy(buf.data(), &net_len, sizeof(net_len));
        if (!in.read(buf.data() + sizeof(net_len), len)) {
            std::cerr << "[ERROR] Failed to read full packet payload of size " << len << "\n";
            break;
        }

        ssize_t sent = sendto(sock, buf.data(), buf.size(), 0, (sockaddr*)&addr, sizeof(addr));
        if (sent < 0) {
            perror("[ERROR] sendto failed");
        } 
        // else {
        //     std::cerr << "[DEBUG] Sent UDP packet #" << packet_count
        //               << " of size " << sent << " to " << ip << ":" << port << "\n";
        // }

        ++packet_count;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cerr << "[INFO] Total UDP packets sent: " << packet_count << "\n";
    close(sock);
}


std::vector<CompositeScoreMessage> load_expected(const std::string& path) {
    std::ifstream in(path);
    std::vector<CompositeScoreMessage> result;
    std::string line;
    std::getline(in, line); // skip header
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        std::string tok;
        CompositeScoreMessage r;
        std::getline(ss, tok, ','); // packet_id
        std::getline(ss, tok, ','); r.subject_id = std::stoul(tok);
        std::getline(ss, tok, ','); // composite_score (not used)
        std::getline(ss, tok, ','); r.scaled_composite_score = std::stoll(tok);
        result.push_back(r);
    }
    return result;
}

std::vector<CompositeScoreMessage> load_actual(const std::string& path) {
    std::ifstream in(path);
    std::vector<CompositeScoreMessage> result;
    std::string line;
    std::getline(in, line); // skip header
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        std::string tok;
        CompositeScoreMessage r;
        std::getline(ss, tok, ','); r.subject_id = std::stoul(tok);
        std::getline(ss, tok, ','); r.scaled_composite_score = std::stoll(tok);
        result.push_back(r);
    }
    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <mcast_ip> <mcast_port> <interface> <tcp_host> <tcp_port>\n";
        return 1;
    }

    const std::string service_path = "./build/bin/data_processing_service";
    const std::string input_bin = "test_data/input_packets.bin";
    const std::string expected_csv = "test_data/expected_composite_scores.csv";
    const std::string actual_csv = "test_results/tcp_sent.csv";

    const std::string mcast_ip = argv[1];
    const uint16_t mcast_port = static_cast<uint16_t>(std::stoi(argv[2]));
    const std::string iface = argv[3];
    const std::string tcp_host = argv[4];
    const uint16_t tcp_port = static_cast<uint16_t>(std::stoi(argv[5]));




    pid_t pid = fork();
    if (pid == 0) {
        execl(service_path.c_str(), service_path.c_str(),
              mcast_ip.c_str(), std::to_string(mcast_port).c_str(), iface.c_str(),
              tcp_host.c_str(), std::to_string(tcp_port).c_str(), nullptr);
        perror("execl");
        exit(1);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    send_udp_packets(input_bin, mcast_ip, mcast_port);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    kill(pid, SIGTERM);  
    waitpid(pid, nullptr, 0);

    // auto expected = load_expected(expected_csv);
    // auto actual = load_actual(actual_csv);

    // size_t min_len = std::min(expected.size(), actual.size());
    // int mismatches = 0;
    // for (size_t i = 1; i < min_len; ++i) {
    //     if (expected[i].subject_id != actual[i].subject_id || expected[i].scaled_score != actual[i].scaled_score) {
    //         std::cout << "[MISMATCH] SID=" << expected[i].subject_id << " Expected=" << expected[i].scaled_score
    //                   << " Got=" << actual[i].scaled_value << "\n";
    //         ++mismatches;
    //     }
    // }

    // std::cout << "[INFO] Total: " << min_len << ", Mismatches: " << mismatches << "\n";
    return 0;
}