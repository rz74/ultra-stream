#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <arpa/inet.h>
#include <endian.h>

#include "../include/types.hpp"
#include "../include/data_book.hpp"
#include "../include/composite_score_calculator.hpp"
#include "../include/parser_utils.hpp"

static_assert(__BYTE_ORDER == __LITTLE_ENDIAN, "Expected little-endian platform");

void write_packet(std::ostream& out, uint32_t subject_id, const std::vector<DataLevel>& levels) {
    uint16_t count = levels.size();
    uint32_t message_length = 4 + 2 + count * (1 + 1 + 8 + 4);

    uint32_t message_length_be = htonl(message_length);
    uint32_t subj_id_be = htonl(subject_id);
    uint16_t count_be = htons(count);

    out.write(reinterpret_cast<const char*>(&message_length_be), sizeof(message_length_be));
    out.write(reinterpret_cast<const char*>(&subj_id_be), sizeof(subj_id_be));
    out.write(reinterpret_cast<const char*>(&count_be), sizeof(count_be));

    for (const auto& lvl : levels) {
        uint8_t level_be = lvl.level;
        uint8_t side_be = lvl.side;
        out.write(reinterpret_cast<const char*>(&level_be), sizeof(level_be));
        out.write(reinterpret_cast<const char*>(&side_be), sizeof(side_be));
        uint64_t value_be = htobe64(static_cast<uint64_t>(lvl.value));
        out.write(reinterpret_cast<const char*>(&value_be), sizeof(value_be));
        uint32_t vol_be = htonl(lvl.volume);
        out.write(reinterpret_cast<const char*>(&vol_be), sizeof(vol_be));
    }
}

int main(int argc, char* argv[]) {
    // std::cout << "[MARKER] generate_test_vectors.cpp - 5.2 running\n";

    // if (argc != 2) {
    //     std::cerr << "Usage: " << argv[0] << " <output_dir>\n";
    //     return 1;
    // }
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <output_dir> <num_packets>\n";
        return 1;
    }

    std::string output_dir = argv[1];
    if (output_dir.back() != '/') output_dir += "/";

    std::ofstream bin_out(output_dir + "input_packets.bin", std::ios::binary);
    std::ofstream input_csv(output_dir + "input_packets.csv");
    std::ofstream summary_csv(output_dir + "input_summary.csv");
    summary_csv << "index,msg_len,subject_id,num_updates,sub_msg_level,sub_msg_side,sub_msg_scaled_value,sub_msg_vol\n";

    // std::default_random_engine rng(42);

    std::random_device rd;
    std::default_random_engine rng(rd());

    std::uniform_int_distribution<uint32_t> sid_dist(10000, 20000);
    std::uniform_int_distribution<uint32_t> vol_dist(10, 1000);
    std::uniform_real_distribution<double> value_dist(100.0, 200.0);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> num_updates_dist(1, 3);

    std::unordered_set<uint32_t> unique_ids;
    while (unique_ids.size() < 5) unique_ids.insert(sid_dist(rng));
    std::vector<uint32_t> subject_ids(unique_ids.begin(), unique_ids.end());

    int num_packets = std::stoi(argv[2]);
    for (int i = 0; i < num_packets; ++i) {
        uint32_t sid = subject_ids[i % subject_ids.size()];
        int num_updates = num_updates_dist(rng);
        std::vector<DataLevel> levels;

        input_csv << sid << ",";
        for (int j = 0; j < num_updates; ++j) {
            DataLevel lvl;
            lvl.level = j % 10;
            lvl.side = side_dist(rng);
            lvl.volume = vol_dist(rng);
            lvl.value = static_cast<int64_t>(value_dist(rng) * 1e9);
            levels.push_back(lvl);
            input_csv << (int)lvl.level << ":" << (int)lvl.side << ":" << lvl.value << ":" << lvl.volume;
            if (j + 1 < num_updates) input_csv << "|";
        }
        input_csv << "\n";

        std::ostringstream oss(std::ios::binary);
        write_packet(oss, sid, levels);
        std::string raw = oss.str();
        bin_out.write(raw.data(), raw.size());

        uint32_t msg_len = raw.size();
        for (size_t j = 0; j < levels.size(); ++j) {
            const auto& lvl = levels[j];
            if (j == 0) {
                summary_csv << i << "," << msg_len << "," << sid << "," << levels.size() << ","
                            << (int)lvl.level << "," << (int)lvl.side << "," << lvl.value << "," << lvl.volume << "\n";
            } else {
                summary_csv << ",,,," << (int)lvl.level << "," << (int)lvl.side << "," << lvl.value << "," << lvl.volume << "\n";
            }
        }
    }

    std::cout << "Generated 100 input test packets.\n";
    return 0;
}
