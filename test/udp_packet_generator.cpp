// udp_packet_generator.cpp
#include <fstream>
#include <random>
#include <vector>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <arpa/inet.h>
#include <endian.h> 

// Format constants
constexpr int NUM_PACKETS = 20;
constexpr int MAX_UPDATES_PER_PACKET = 5;
constexpr int NUM_SUBJECTS = 5;

struct Update {
    uint8_t level;      // 0-9
    uint8_t side;       // 0 = demand, 1 = supply
    int64_t value;      // scaled by 1e9
    uint32_t volume;    // units
};

void write_packet(std::ofstream& out, uint32_t subject_id, const std::vector<Update>& updates) {
    uint32_t message_length = 4 + 2 + updates.size() * 14;
    uint16_t num_updates = updates.size();

    uint32_t msg_len_net = htonl(message_length);
    uint32_t sid_net = htonl(subject_id);
    uint16_t updates_net = htons(num_updates);

    out.write(reinterpret_cast<const char*>(&msg_len_net), sizeof(msg_len_net));
    out.write(reinterpret_cast<const char*>(&sid_net), sizeof(sid_net));
    out.write(reinterpret_cast<const char*>(&updates_net), sizeof(updates_net));

    for (const auto& u : updates) {
        uint64_t value_net = htobe64(u.value);
        uint32_t vol_net = htonl(u.volume);

        out.write(reinterpret_cast<const char*>(&u.level), sizeof(u.level));
        out.write(reinterpret_cast<const char*>(&u.side), sizeof(u.side));
        out.write(reinterpret_cast<const char*>(&value_net), sizeof(value_net));
        out.write(reinterpret_cast<const char*>(&vol_net), sizeof(vol_net));
    }
}

int main() {
    std::ofstream bin("test_vectors/input_packets.bin", std::ios::binary);
    std::ofstream txt("test_vectors/input_packets.csv");

    if (!bin || !txt) {
        std::cerr << "[ERROR] Failed to open output files." << std::endl;
        return 1;
    }

    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> subject_dist(10000, 10000 + 20);
    std::uniform_int_distribution<int> updates_dist(1, MAX_UPDATES_PER_PACKET);
    std::uniform_int_distribution<int> level_dist(0, 9);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int64_t> value_dist(100000000, 200000000); // 100.0 - 200.0
    std::uniform_int_distribution<uint32_t> vol_dist(10, 1000);

    // Pick 5 SIDs and reuse them
    std::vector<uint32_t> subject_pool;
    for (int i = 0; i < 5; ++i)
        subject_pool.push_back(subject_dist(rng));

    txt << "packet_id,subject_id,update_id,level,side,value,volume\n";

    for (int i = 0; i < NUM_PACKETS; ++i) {
        uint32_t sid = subject_pool[rng() % subject_pool.size()];
        int num_updates = updates_dist(rng);
        std::vector<Update> updates;

        for (int j = 0; j < num_updates; ++j) {
            Update u;
            u.level = level_dist(rng);
            u.side = side_dist(rng);
            u.value = value_dist(rng);
            u.volume = vol_dist(rng);
            updates.push_back(u);

            txt << i << "," << sid << "," << j << ","
                << static_cast<int>(u.level) << ","
                << static_cast<int>(u.side) << ","
                << u.value << "," << u.volume << "\n";
        }

        write_packet(bin, sid, updates);
    }

    // Force one packet with both demand and supply at level 0 for subject_pool[0]
    std::vector<Update> guaranteed = {
        {0, 0, 150000000, 100},  // demand
        {0, 1, 151000000, 200}   // supply
    };
    write_packet(bin, subject_pool[0], guaranteed);
    txt << NUM_PACKETS << "," << subject_pool[0] << ",0,0,0,150000000,100\n";
    txt << NUM_PACKETS << "," << subject_pool[0] << ",1,0,1,151000000,200\n";

    std::cout << "[INFO] Generated " << NUM_PACKETS + 1
              << " packets to test_vectors/input_packets.bin" << std::endl;
    return 0;
}
