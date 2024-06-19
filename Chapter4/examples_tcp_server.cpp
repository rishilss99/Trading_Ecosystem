#include "examples_tcp_server.h"

namespace ExampleSocketUtils
{
    auto TCPServer::destroy() noexcept -> void
    {
        close(efd_);
        efd_ = -1;
        listener_socket_.destroy();
    }

    auto TCPServer::listen(const std::string &iface, int port) -> void
    {
        destroy();
        efd_ = epoll_create(1);
        ASSERT(efd_ >= 0, "epoll_create() failed error:" + std::string(std::strerror(errno)));
        ASSERT(listener_socket_.connect("", iface, port, true) >= 0, "Listener socket failed to connect. iface:" +
                                                                         iface + " port:" + std::to_string(port) +
                                                                         "error : " + std::string(std::strerror(errno)));
        ASSERT(epoll_add(&listener_socket_), "epoll_ctl() failed. error:" + std::string(std::strerror(errno)));
    }

    auto TCPServer::del(TCPSocket *socket) -> void
    {
        epoll_del(socket);
        sockets_.erase(std::remove(sockets_.begin(), sockets_.end(), socket), sockets_.end());
        receive_sockets_.erase(std::remove(receive_sockets_.begin(), receive_sockets_.end(), socket), receive_sockets_.end());
        send_sockets_.erase(std::remove(send_sockets_.begin(), send_sockets_.end(), socket), send_sockets_.end());
    }

    auto TCPServer::epoll_add(TCPSocket *socket) -> bool
    {
        epoll_event ev{};
        ev.events = EPOLLET | EPOLLIN;
        ev.data.ptr = socket;
        // epoll_data is an union. Using fd for file descriptor, book and code implementation differ in the below line of code
        // ev.data.ptr = socket->fd_;
        return (epoll_ctl(efd_, EPOLL_CTL_ADD, socket->fd_, &ev) != -1);
    }

    auto TCPServer::epoll_del(TCPSocket *socket) -> bool
    {
        return (epoll_ctl(efd_, EPOLL_CTL_DEL, socket->fd_, nullptr) != -1);
    }

    auto TCPServer::poll() noexcept -> void
    {
        // 1 + is for listener socket since it is not a part of sockets vector
        const int max_events = 1 + sockets_.size();
        for (auto socket : disconnected_sockets_)
        {
            del(socket);
        }
        const int n = epoll_wait(efd_, events_, max_events, 0);

        bool have_new_connection = false;

        for (int i = 0; i < n; i++)
        {
            epoll_event &event = events_[i];
            auto socket = reinterpret_cast<TCPSocket *>(event.data.ptr);

            // Process each epoll_entry which is essentially a socket with new data to be read/write

            if (event.events & EPOLLIN)
            {
                // If EPOLLIN and it's the listener socket -> There is a new connection
                if (socket == &listener_socket_)
                {
                    logger_.log("%:% %() % EPOLLIN listener_socket : %\n", __FILE__, __LINE__, __FUNCTION__,
                                Common::getCurrentTimeStr(&time_str_), socket->fd_);
                    have_new_connection = true;
                    continue;
                }
                // Else it's simply new data to be read
                logger_.log("%:% %() % EPOLLIN socket : %\n", __FILE__, __LINE__, __FUNCTION__,
                            Common::getCurrentTimeStr(&time_str_), socket->fd_);
                if (std::find(receive_sockets_.begin(), receive_sockets_.end(), socket) == receive_sockets_.end())
                {
                    receive_sockets_.push_back(socket);
                }
            }

            if (event.events & EPOLLOUT)
            {
                logger_.log("%:% %() % EPOLLOUT socket : %\n", __FILE__, __LINE__, __FUNCTION__,
                            Common::getCurrentTimeStr(&time_str_), socket->fd_);
                if (std::find(send_sockets_.begin(), send_sockets_.end(), socket) == send_sockets_.end())
                {
                    send_sockets_.push_back(socket);
                }
            }

            if (event.events & (EPOLLHUP | EPOLLERR))
            {
                logger_.log("%:% %() % EPOLLERR socket : %\n", __FILE__, __LINE__, __FUNCTION__,
                            Common::getCurrentTimeStr(&time_str_), socket->fd_);
                if (std::find(disconnected_sockets_.begin(), disconnected_sockets_.end(), socket) == disconnected_sockets_.end())
                {
                    disconnected_sockets_.push_back(socket);
                }
            }
        }

        while (have_new_connection)
        {
            logger_.log("%:% %() % have_new_connection\n",
                        __FILE__, __LINE__, __FUNCTION__,
                        Common::getCurrentTimeStr(&time_str_));
            sockaddr_storage addr;
            socklen_t addr_len = sizeof(addr);
            int fd = accept(listener_socket_.fd_, reinterpret_cast<sockaddr *>(&addr), &addr_len);
            if (fd == -1)
            {
                break;
            }
            ASSERT(setNonBlocking(fd) && setNoDelay(fd), "Failed to set non-blocking or no-delay on socket:" + std::to_string(fd));
            logger_.log("%:% %() % accepted socket:%\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_), fd);
            TCPSocket *socket = new TCPSocket(logger_);
            socket->fd_ = fd;
            socket->recv_callback_ = recv_callback_;
            ASSERT(epoll_add(socket), "Unable to add socket error:" + std::string(std::strerror(errno)));
            if (std::find(sockets_.begin(), sockets_.end(), socket) == sockets_.end())
                sockets_.push_back(socket);
            if (std::find(receive_sockets_.begin(), receive_sockets_.end(), socket) == receive_sockets_.end())
                receive_sockets_.push_back(socket);
        }
    }

    auto TCPServer::sendAndRecv() noexcept -> void
    {
        auto recv = false;
        for (auto socket : receive_sockets_)
        {
            if (socket->sendAndRecv())
            {
                recv = true;
            }
        }
        if (recv)
            recv_finished_callback_();
        for (auto socket : send_sockets_)
        {
            socket->sendAndRecv();
        }
    }
}