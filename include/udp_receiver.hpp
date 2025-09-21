// udp_receiver.hpp
#pragma once

#include <string>
#include <cstdint>
#include <functional>

class UdpReceiver {
public:
    using PacketCallback = std::function<void(const uint8_t* data, size_t length)>;

    UdpReceiver(const std::string& mcast_ip,
                uint16_t mcast_port,
                const std::string& interface_name);

    bool start(PacketCallback callback);
    void stop();

private:
    int sockfd_ = -1;
    bool running_ = false;

    std::string mcast_ip_;
    uint16_t mcast_port_;
    std::string interface_name_;
};
