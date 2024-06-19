#pragma once
#include <iostream>
#include <string>
#include <unordered_set>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <boost/asio.hpp>
#include "macros.h"
#include "logging.h"

namespace Sockets
{
    constexpr int MaxTCPServerBacklog = 1024;

    auto getIfaceIP(const std::string &iface) -> std::string // DONE (Not using Asio)
    {
        char buf[NI_MAXHOST] = {'\0'};
        ifaddrs *ifaddr = nullptr;
        if (getifaddrs(&ifaddr) != -1)
        {
            for (ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr->sa_family == AF_INET && ifa->ifa_name == iface)
                {
                    getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), buf, sizeof(buf), nullptr, 0, NI_NUMERICHOST);
                    break;
                }
            }
            freeifaddrs(ifaddr);
        }
        return buf;
    }

    // auto setNonBlocking(int fd) -> bool;            // LATER (Not required in Asio -> Unified I/O model)
    // {
    //     const auto flags = fcntl(fd, F_GETFL, 0);
    //     if (flags == -1)
    //     {
    //         return false;
    //     }
    //     if (flags & O_NONBLOCK)
    //     {
    //         return true;
    //     }
    //     return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
    // }

    // auto wouldBlock() -> bool                      // LATER
    // {
    //     return (errno == EWOULDBLOCK || errno == EINPROGRESS);
    // }

    auto setNoDelay(boost::asio::ip::tcp::socket &tcp_sock) // DONE (Using Asio -> Only required for TCP socket
    {
        try
        {
            boost::asio::ip::tcp::no_delay option(true);
            tcp_sock.set_option(option);
        }
        catch (const boost::system::system_error &e)
        {
            std::cerr << "Failed to set TCP_NODELAY option: " << e.what() << std::endl;
        }
    }

    template <typename Protocol>
    auto setSOTimestamp(boost::asio::basic_socket<Protocol> &sock) // DONE (Using Asio)
    {
        int enable = 1;
        int fd = sock.native_handle();
        try
        {
            if (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, &enable, sizeof(enable)) < 0)
            {
                throw boost::system::system_error(boost::system::error_code(errno, boost::system::system_category()), "Failed to set SO_TIMESTAMP");
            }
        }
        catch (const boost::system::system_error &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    template <typename Protocol>
    auto setMcastTTL(boost::asio::basic_socket<Protocol> &sock, int ttl) // DONE (Using Asio)
    {
        try
        {
            boost::asio::ip::multicast::hops option(ttl);
            sock.set_option(option);
        }
        catch (const boost::system::system_error &e)
        {
            std::cerr << "Failed to set Unicast Hops option: " << e.what() << std::endl;
        }
    }

    template <typename Protocol>
    auto setTTL(boost::asio::basic_socket<Protocol> &sock, int ttl) // DONE (Using Asio)
    {
        try
        {
            boost::asio::ip::unicast::hops option(ttl);
            sock.set_option(option);
        }
        catch (const boost::system::system_error &e)
        {
            std::cerr << "Failed to set Unicast Hops option: " << e.what() << std::endl;
        }
    }

    auto join(int fd, const std::string &ip,
              const std::string &iface, int port) -> bool;

    auto createSocket(Common::Logger &logger, const std::string &t_ip, const std::string &iface, int port, bool is_udp,
                      bool is_blocking, bool is_listening, int ttl, bool needs_so_timestamp) -> int;
    // {
    //     std::string time_str;
    //     const auto ip = t_ip.empty() ? getIfaceIP(iface) : t_ip;
    //     logger.log("%:% %() % ip:% iface:% port:% is_udp:%is_blocking:% is_listening:% ttl:% SO_time:%\n",
    //                __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str),
    //                ip, iface, port, is_udp, is_blocking, is_listening, ttl, needs_so_timestamp);
    //     addrinfo hints{.ai_flags = is_listening ? AI_PASSIVE : 0, .ai_family = AF_INET, .ai_socktype = is_udp ? SOCK_DGRAM : SOCK_STREAM, .ai_protocol = is_udp ? IPPROTO_UDP : IPPROTO_TCP};
    //     if (std::isdigit(ip.c_str()[0]))
    //     {
    //         hints.ai_flags |= AI_NUMERICHOST;
    //     }
    //     hints.ai_flags |= AI_NUMERICSERV;
    //     addrinfo *result = nullptr;
    //     const auto rc = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result);
    //     if (rc)
    //     {
    //         logger.log("getaddrinfo() failed. error:% errno:%\n",
    //                    gai_strerror(rc), strerror(errno));
    //         return -1;
    //     }

    //     int fd = -1;
    //     int one = 1;
    //     for (addrinfo *rp = result; rp; rp = rp->ai_next)
    //     {
    //         fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    //         if (fd == -1)
    //         {
    //             logger.log("socket() failed. errno:%\n", strerror(errno));
    //             return -1;
    //         }
    //         if (!is_blocking)
    //         {
    //             if (!setNonBlocking(fd))
    //             {
    //                 logger.log("setNonBlocking() failed. errno:%\n", strerror(errno));
    //                 return -1;
    //             }
    //             if (!is_udp && !setNoDelay(fd))
    //             {
    //                 logger.log("setNoDelay() failed. errno:%\n", strerror(errno));
    //                 return -1;
    //             }
    //         }
    //     }
    // }
}