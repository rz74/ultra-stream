// tcp_receiver.cpp
#include <iostream>
#include <fstream>
#include <csignal>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

bool running = true;

void handle_sigint(int) {
    running = false;
}

int main(int argc, char* argv[]) {

    std::ofstream marker("/tmp/tcp_receiver_marker.log", std::ios::app);
    marker << "[MARKER] tcp_receiver main() entered, PID=" << getpid() << std::endl;
    marker.close();


    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    std::cerr << "[DEBUG] tcp_receiver started with PID=" << getpid() << std::endl;


    int port = std::stoi(argv[1]);
    const std::string output_file = "test_results/tcp_sent.csv";

    signal(SIGINT, handle_sigint);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "[INFO] TCP Receiver listening on port " << port << "..." << std::endl;
    int addrlen = sizeof(address);

    std::cerr << "[DEBUG] Waiting for TCP connection on socket..." << std::endl;

    int new_socket = accept(server_fd, (sockaddr*)&address, (socklen_t*)&addrlen);

    std::cerr << "[DEBUG] Connection accepted!" << std::endl;


    if (new_socket < 0) {
        perror("accept");
        std::cerr << "[DEBUG] accept() failed. errno=" << errno << "\n";
        return 1;
    }
    std::cerr << "[DEBUG] Connection accepted\n";
    std::ofstream out(output_file);
    out << "subject_id,scaled_score\n";

    uint8_t buffer[12];
    while (running) {
        ssize_t received = recv(new_socket, buffer, sizeof(buffer), MSG_WAITALL);
        if (received <= 0) break;

        uint32_t sid;
        int64_t scaled;
        std::memcpy(&sid, buffer, 4);
        std::memcpy(&scaled, buffer + 4, 8);

        out << sid << "," << scaled << "\n";
    }

    std::cout << "[INFO] TCP Receiver shutting down." << std::endl;
    close(new_socket);
    close(server_fd);
    return 0;
}
