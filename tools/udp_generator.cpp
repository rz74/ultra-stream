// tools/udp_generator.cpp
#include <iostream>
#include <vector>
#        double demand = value_dist(rng);
        double supply = demand + 0.5;
        uint32_t qty = qty_dist(rng);
        auto packet = createPacket(sid, demand, supply, qty);ude <cstring>
#include <random>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <unistd.h>

std::vector<uint8_t> createPacket(uint32_t sid, double demand, double supply, uint32_t qty) {
    std::vector<uint8_t> data;

    uint32_t msg_len = 4 + 2 + 2 * (1 + 1 + 8 + 4);
    uint16_t num_updates = 2;

    int64_t demand_value = static_cast<int64_t>(demand * 1e9);
    int64_t supply_value = static_cast<int64_t>(supply * 1e9);

    auto append = [&](const void* p, size_t len) {
        const uint8_t* b = reinterpret_cast<const uint8_t*>(p);
        data.insert(data.end(), b, b + len);
    };

    uint32_t msg_len_net = htonl(msg_len);
    uint32_t sid_net = htonl(sid);
    uint16_t num_net = htons(num_updates);
    append(&msg_len_net, 4);
    append(&sid_net, 4);
    append(&num_net, 2);

    uint8_t level = 0, side_demand = 0, side_supply = 1;
    uint64_t demand_be, supply_be;
    std::memcpy(&demand_be, &demand_value, 8);
    std::memcpy(&supply_be, &supply_value, 8);
    demand_be = htobe64(demand_be);
    supply_be = htobe64(supply_be);

    uint32_t qty_net = htonl(qty);

    append(&level, 1); append(&side_demand, 1); append(&demand_be, 8); append(&qty_net, 4);
    append(&level, 1); append(&side_supply, 1); append(&supply_be, 8); append(&qty_net, 4);

    return data;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <mcast_ip> <port> <num_packets>\n";
        return 1;
    }

    std::string ip = argv[1];
    uint16_t port = std::stoi(argv[2]);
    int count = std::stoi(argv[3]);

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> sid_dist(10000, 20000);
    std::uniform_real_distribution<double> value_dist(95.0, 105.0);
    std::uniform_int_distribution<int> qty_dist(100, 1000);

    for (int i = 0; i < count; ++i) {
        uint32_t sid = sid_dist(rng);
        double demand = value_dist(rng);
        double supply = demand + 0.5;
        int qty = qty_dist(rng);
        auto packet = createPacket(sid, demand, supply, qty);
        sendto(sock, packet.data(), packet.size(), 0, (sockaddr*)&addr, sizeof(addr));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    close(sock);
    return 0;
}
