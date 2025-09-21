// logger.cpp
#include "logger.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

// bool g_enable_latency_logging = false;
// void dump_latency_trace(const std::vector<LatencySample>& samples) {
    
//     std::filesystem::create_directories("test_results");  
//     // std::ofstream out("test_results/latency_trace.csv");
//     std::ofstream out("test_results/latency_trace.csv");
//     if (!out) {
//         std::cerr << "[ERROR] Failed to open latency_trace.csv for writing\n";
//         return;
//     }

//     out << "subject_id,t_recv,t_parsed,t_calc_start,t_calc_end,t_sent\n";
//     for (const auto& s : samples) {
//         out << s.subject_id << ","
//             << s.t_recv << ","
//             << s.t_parsed << ","
//             << s.t_calc_start << ","
//             << s.t_calc_end << ","
//             << s.t_sent << "\n";
//     }

//     // std::cout << "[INFO] Latency trace saved to latency_results/latency_trace.csv\n";
// }

void append_latency_sample(const LatencySample& s) {
    static std::ofstream out("test_results/latency_trace.csv", std::ios::app);
    
    static bool initialized = false;

    

    if (!initialized) {
        std::filesystem::create_directories("test_results");
        // out << "subject_id,t_recv,t_parsed,t_calc_start,t_calc_end,t_sent\n";
        out << "subject_id,t_recv,t_parsed,t_calc_start,t_calc_end,t_sent,num_updates\n";

        initialized = true;
    }

    

    out << s.subject_id << ","
        << s.t_recv << ","
        << s.t_parsed << ","
        << s.t_calc_start << ","
        << s.t_calc_end << ","
        << s.t_sent << ","
        << s.num_updates << "\n";
    out.flush(); 
    // std::cout << "[STREAM] Writing latency for SID=" << s.subject_id << "\n";

}

