#pragma once

#include "tcp_socket.h"

constexpr size_t TCPClientBufferSize = 64 * 1024 * 1024;

class TestTCPClient final
{
public:
    TestTCPClient() = delete;
    TestTCPClient(const TestTCPClient &) = delete;
    TestTCPClient(TestTCPClient &&) = delete;
    TestTCPClient &operator=(const TestTCPClient &) = delete;
    TestTCPClient &operator=(TestTCPClient &&) = delete;

    explicit TestTCPClient(Common::Logger &logger) : logger_(logger), outbound_data_(TCPClientBufferSize), inbound_data_(TCPClientBufferSize)
    {
    }

    ~TestTCPClient()
    {
        close(socket_fd_);
    }

    auto connect(const std::string &ip, const std::string &iface, int port) -> void;

    auto send(const std::string &msg) noexcept -> void;

    // Why this?
    auto sendAndRecv() noexcept -> bool;

    Common::Logger &logger_;
    // One socket per client model, can have a map if want 1 client -> Multiple sockets
    int socket_fd_ = -1;
    std::vector<char> outbound_data_;
    std::vector<char> inbound_data_;
    size_t next_recv_index_ = 0;
    size_t next_send_index_ = 0;
    std::string time_str_;
    std::function<void(TestTCPClient *socket, Common::Nanos rx_time)> recv_callback_;
};