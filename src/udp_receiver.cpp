// udp_receiver.cpp
#include "udp_receiver.hpp"
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

UdpReceiver::UdpReceiver(const std::string& mcast_ip,
                         uint16_t mcast_port,
                         const std::string& interface_name)
    : mcast_ip_(mcast_ip), mcast_port_(mcast_port), interface_name_(interface_name) {}

bool UdpReceiver::start(PacketCallback callback) {
    if (running_) return false;

    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) return false;

    // Allow reuse
    int reuse = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mcast_port_);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd_);
        return false;
    }

    ip_mreqn mreq = {};
    mreq.imr_multiaddr.s_addr = inet_addr(mcast_ip_.c_str());
    mreq.imr_address.s_addr = INADDR_ANY;
    mreq.imr_ifindex = if_nametoindex(interface_name_.c_str());

    if (setsockopt(sockfd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        close(sockfd_);
        return false;
    }

    running_ = true;
    uint8_t buffer[2048];
    while (running_) {
        ssize_t len = recv(sockfd_, buffer, sizeof(buffer), 0);
        if (len > 0) {
            callback(buffer, static_cast<size_t>(len));
        }
    }

    return true;
}

void UdpReceiver::stop() {
    if (sockfd_ >= 0) close(sockfd_);
    running_ = false;
}
