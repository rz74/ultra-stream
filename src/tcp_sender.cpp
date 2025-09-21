#include "tcp_sender.hpp"
#include "logger.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fstream>

std::vector<TcpSendRecord> TcpSender::send_log;
std::mutex TcpSender::log_mtx;

TcpSender::TcpSender(const std::string& host, uint16_t port)
    : host_(host), port_(port) {}

TcpSender::~TcpSender() {
    close();
}

bool TcpSender::connect() {
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) return false;

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);

    hostent* server = gethostbyname(host_.c_str());
    if (!server) return false;
    std::memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (::connect(sockfd_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ::close(sockfd_);
        return false;
    }

    return true;
}

bool TcpSender::hasScoreChanged(uint32_t subject_id, int64_t new_score) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = last_sent_.find(subject_id);
    return (it == last_sent_.end() || it->second != new_score);
}

void TcpSender::send(const CompositeScoreMessage& msg, uint64_t* send_timestamp_ns) {
    std::lock_guard<std::mutex> lock(mtx_);

    uint8_t buffer[12];
    std::memcpy(buffer, &msg.subject_id, 4);
    std::memcpy(buffer + 4, &msg.scaled_composite_score, 8);

    size_t total_sent = 0;
    while (total_sent < sizeof(buffer)) {
        ssize_t sent = ::send(sockfd_, buffer + total_sent, sizeof(buffer) - total_sent, 0);
        if (sent <= 0) {
            std::cerr << "Error: partial or failed send\n";
            return;
        }
        total_sent += static_cast<size_t>(sent);
    }

    last_sent_[msg.subject_id] = msg.scaled_composite_score;

    if (send_timestamp_ns) {
        *send_timestamp_ns = now_ns();
    }

    {
        std::lock_guard<std::mutex> lg(log_mtx);
        send_log.push_back({msg.subject_id, msg.scaled_composite_score, *send_timestamp_ns});
    }
}

void TcpSender::sendIfChanged(const CompositeScoreMessage& msg, uint64_t* send_timestamp_ns) {
    if (hasScoreChanged(msg.subject_id, msg.scaled_composite_score)) {
        send(msg, send_timestamp_ns);
    }
}

void TcpSender::close() {
    if (sockfd_ >= 0) ::close(sockfd_);
    sockfd_ = -1;
}

void TcpSender::dumpSendLog(const std::string& filename) {
    std::ofstream out(filename);
    out << "subject_id,scaled_composite_score,timestamp_ns\n";
    std::lock_guard<std::mutex> lock(log_mtx);
    for (const auto& r : send_log) {
        out << r.subject_id << "," << r.scaled_composite_score << "," << r.timestamp_ns << "\n";
    }
}
