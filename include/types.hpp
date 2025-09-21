// types.hpp
#pragma once

#include <array>
#include <cstdint>
#include <vector>


extern bool g_enable_latency_logging;
constexpr int MAX_BOOK_LEVELS = 10;

// one level of data book update
struct DataLevel {
    uint8_t level;     // 0–9
    uint8_t side;      // 0 = demand, 1 = supply
    int64_t value;     // scaled value (value * 10^9)
    uint32_t volume;   // number of units at this level
};

// result of one data stream message
struct [[nodiscard]] ProcessedMessage {
    uint32_t subject_id;
    std::vector<DataLevel> updates;
};

// Op to TCP
struct [[nodiscard]] CompositeScoreMessage {
    uint32_t subject_id;
    int64_t scaled_composite_score; // composite score * 10^9
};
struct LatencySample {
    uint32_t subject_id;
    int64_t t_recv;
    int64_t t_parsed;
    int64_t t_calc_start;
    int64_t t_calc_end;
    int64_t t_sent;
    int num_updates; 
};
