#pragma once

#include <cstdint>
#include <vector>
#include <chrono>
#include <fstream>
#include <string>
#include "types.hpp"

// Timestamp resolution: nanoseconds
inline uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// struct LatencySample {
//     uint32_t subject_id;
//     uint64_t t_recv;
//     uint64_t t_parsed;
//     uint64_t t_calc_start;
//     uint64_t t_calc_end;
//     uint64_t t_sent;
// };

// void dump_latency_trace(const std::vector<LatencySample>& samples);
void append_latency_sample(const LatencySample& s);
