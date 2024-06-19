#pragma once

#include "tcp_socket.h"
#include "test_tcp_client.h"

class TestTCPServer final
{
public:
    TestTCPServer() = delete;
    TestTCPServer(const TestTCPServer &) = delete;
    TestTCPServer(TestTCPServer &&) = delete;
    TestTCPServer &operator=(const TestTCPServer &) = delete;
    TestTCPServer &operator=(TestTCPServer &&) = delete;

    explicit TestTCPServer(Common::Logger &logger) : logger_(logger), listener_socket_(logger)
    {
    }

    ~TestTCPServer()
    {
        close(efd_);
    }

    auto listen(const std::string &iface, int port) -> void;

    auto poll() -> void;

    auto sendAndRecv() -> void;

    std::function<void(TestTCPClient *socket, Common::Nanos rx_time)> recv_callback_;

private:
    auto epoll_add(TestTCPClient *sock) -> bool;

    auto epoll_del(TestTCPClient *sock) -> bool;

private:
    Common::Logger &logger_;
    int efd_ = -1;
    TestTCPClient listener_socket_;
    epoll_event events_[1024];
    std::vector<TestTCPClient*> send_sockets_, recv_sockets_;
    
    std::string time_str_;
};