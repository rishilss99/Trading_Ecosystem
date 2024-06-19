#pragma once

#include "examples_tcp_socket.h"

namespace ExampleSocketUtils
{
    struct TCPServer
    {
        explicit TCPServer(Common::Logger &logger) : listener_socket_(logger), logger_(logger) {}

        ~TCPServer()
        {
            destroy();
        }

        TCPServer() = delete;
        TCPServer(const TCPServer &) = delete;
        TCPServer(TCPServer &&) = delete;
        TCPServer &operator=(const TCPServer &) = delete;
        TCPServer &operator=(TCPServer &&) = delete;

        auto destroy() noexcept -> void;

        auto listen(const std::string &iface, int port) -> void;

        auto epoll_add(TCPSocket *socket) -> bool;

        auto epoll_del(TCPSocket *socket) -> bool;

        auto del(TCPSocket *socket) -> void;

        auto poll() noexcept -> void;

        auto sendAndRecv() noexcept -> void;

        int efd_ = -1;
        TCPSocket listener_socket_;
        epoll_event events_[1024];
        std::vector<TCPSocket *> sockets_, receive_sockets_, send_sockets_, disconnected_sockets_;
        std::function<void(TCPSocket *s, Common::Nanos rx_time)> recv_callback_;
        std::function<void()> recv_finished_callback_;
        std::string time_str_;
        Common::Logger &logger_;
    };

}