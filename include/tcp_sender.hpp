#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <cstdint>
#include "types.hpp"
struct TcpSendRecord {
    uint32_t subject_id;
    int64_t scaled_composite_score;
    uint64_t timestamp_ns;
};


class TcpSender {
public:
    TcpSender(const std::string& host, uint16_t port);
    ~TcpSender();

    bool connect();
    void close();
    void dumpSendLog(const std::string& filename);

    void sendIfChanged(const CompositeScoreMessage& msg, uint64_t* send_timestamp_ns = nullptr); // optional legacy
    void send(const CompositeScoreMessage& msg, uint64_t* send_timestamp_ns = nullptr);           // new raw sender
    bool hasScoreChanged(uint32_t subject_id, int64_t new_score) const;                          // new checker

private:
    std::string host_;
    uint16_t port_;
    int sockfd_ = -1;

    mutable std::mutex mtx_;
    std::unordered_map<uint32_t, int64_t> last_sent_;

    static std::vector<TcpSendRecord> send_log;
    static std::mutex log_mtx;
};
