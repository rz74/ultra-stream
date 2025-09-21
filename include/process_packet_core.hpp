// process_packet_core.hpp
#pragma once

#include <iostream>
#include <unordered_set>
#include "types.hpp"
#include "data_book.hpp"
#include "composite_score_calculator.hpp"
#include "tcp_sender.hpp"
#include "logger.hpp"

// Apply processed message to data book and TCP, and log timing
inline void process_decoded_packet(
    const ProcessedMessage& msg,
    DataBookManager& book_manager,
    CompositeScoreCalculator& calculator,
    std::vector<LatencySample>* latency_log,
    std::ostream* out,
    int packet_id,
    TcpSender* sender)
{
    (void)packet_id; // Suppress unused parameter warning - available for debugging/logging
    static std::unordered_set<uint32_t> seen_sids;

    uint64_t t_recv = now_ns();

    DataBook& book = book_manager.getOrCreateBook(msg.subject_id);
    for (const auto& u : msg.updates) {
        book.applyUpdate(u);
    }

    uint64_t t_parsed = now_ns();
    int64_t scaled_score = calculator.calculateCompositeScore(book);
    uint64_t t_calc_end = now_ns();

    if (sender) {
        bool is_first_time = seen_sids.insert(msg.subject_id).second;
        if (is_first_time || sender->hasScoreChanged(msg.subject_id, scaled_score)) {
            CompositeScoreMessage tcp_msg{msg.subject_id, scaled_score};
            sender->send(tcp_msg, &t_calc_end);

            uint64_t t_sent = now_ns();

            if (latency_log) {
                LatencySample sample;
                
                sample.subject_id = msg.subject_id;
                sample.t_recv = t_recv;
                sample.t_parsed = t_parsed;
                sample.t_calc_start = t_parsed;   
                sample.t_calc_end = t_calc_end;
                sample.t_sent = t_sent;
                sample.num_updates = static_cast<int>(msg.updates.size());

                append_latency_sample(sample);
            }

            if (out) {
                *out << "[main] SID=" << msg.subject_id
                     << " Composite-score=" << (scaled_score / 1e9)
                     << " scaled=" << scaled_score << "\n";
            }
        }
    }
}
