#include "test_tcp_server.h"

using namespace std;
using namespace Common;

auto TestTCPServer::listen(const string &iface, int port) -> void
{
    efd_ = epoll_create(1);

    ASSERT(efd_ >= 0, "epoll_create() failed error:" + std::string(std::strerror(errno)));

    const SocketCfg socket_cfg{"", iface, port, false, true, true};

    // In case of server, this will create socket and bind it to address and port
    listener_socket_.socket_fd_ = createSocket(logger_, socket_cfg);

    ASSERT(listener_socket_.socket_fd_ >= 0, "Listener socket failed to connect. iface:" + iface + " port:" + std::to_string(port) + " error:" + std::string(std::strerror(errno)));
    ASSERT(epoll_add(&listener_socket_), "epoll_ctl() failed. error:" + std::string(std::strerror(errno)));
}

auto TestTCPServer::poll() -> void
{
    const int max_events = 1 + recv_sockets_.size() + send_sockets_.size();
    const int n_events = epoll_wait(efd_, events_, max_events, 0);

    bool have_new_connection = false;

    for (int i = 0; i < n_events; i++)
    {
        epoll_event &event = events_[i];
        TestTCPClient *sock = reinterpret_cast<TestTCPClient *>(event.data.ptr);

        if (event.events & EPOLLIN)
        {
            if (sock == &listener_socket_)
            {
                logger_.log("%:% %() % EPOLLIN listener_socket : %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), listener_socket_.socket_fd_);
                have_new_connection = true;
                continue;
            }
            logger_.log("%:% %() % EPOLLIN socket : %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), sock->socket_fd_);
            if (std::find(recv_sockets_.begin(), recv_sockets_.end(), sock) == recv_sockets_.end())
            {
                recv_sockets_.push_back(sock);
            }
        }

        if (event.events & EPOLLOUT)
        {
            logger_.log("%:% %() % EPOLLOUT socket : %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), sock->socket_fd_);
            if (std::find(send_sockets_.begin(), send_sockets_.end(), sock) == send_sockets_.end())
            {
                send_sockets_.push_back(sock);
            }
        }
    }

    while (have_new_connection)
    {
        logger_.log("%:% %() % have_new_connection\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_));
        sockaddr_storage addr;
        socklen_t addr_len = sizeof(addr);
        int fd = accept(listener_socket_.socket_fd_, reinterpret_cast<sockaddr *>(&addr), &addr_len);
        if (fd == -1)
        {
            break;
        }
        ASSERT(setNonBlocking(fd) && disableNagle(fd), "Failed to set non-blocking or no-delay on socket:" + std::to_string(fd));
        logger_.log("%:% %() % accepted socket:%\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_), fd);
        TestTCPClient *sock = new TestTCPClient(logger_);
        sock->socket_fd_ = fd;
        sock->recv_callback_ = recv_callback_;
        ASSERT(epoll_add(sock), "Unable to add socket error:" + std::string(std::strerror(errno)));
        if (std::find(recv_sockets_.begin(), recv_sockets_.end(), sock) == recv_sockets_.end())
        {
            recv_sockets_.push_back(sock);
        }
    }
}

auto TestTCPServer::sendAndRecv() -> void
{
    auto recv = false;

    std::for_each(recv_sockets_.begin(), recv_sockets_.end(), [&recv](auto socket)
                  { recv |= socket->sendAndRecv(); });

    
    std::for_each(send_sockets_.begin(), send_sockets_.end(), [](auto socket) {
      socket->sendAndRecv();
    });
}

auto TestTCPServer::epoll_add(TestTCPClient *sock) -> bool
{
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    ev.data.ptr = sock;
    return (epoll_ctl(efd_, EPOLL_CTL_ADD, sock->socket_fd_, &ev) != -1);
}

auto TestTCPServer::epoll_del(TestTCPClient *sock) -> bool
{
    return (epoll_ctl(efd_, EPOLL_CTL_DEL, sock->socket_fd_, nullptr) != -1);
}
