#include "examples_sockets_utils.h"

namespace ExampleSocketUtils
{
    auto getIfaceIP(const std::string &iface) -> std::string
    {
        char buf[NI_MAXHOST] = {'\0'};
        ifaddrs *ifaddr = nullptr;
        if (getifaddrs(&ifaddr) != -1)
        {
            for (ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && ifa->ifa_name == iface)
                {
                    getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), buf, sizeof(buf), nullptr, 0, NI_NUMERICHOST);
                    break;
                }
            }
            freeifaddrs(ifaddr);
        }
        return buf;
    }

    auto setNonBlocking(int fd) -> bool
    {
        const auto flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1)
        {
            return false;
        }
        if (flags & O_NONBLOCK)
        {
            return true;
        }
        return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
    }

    auto setNoDelay(int fd) -> bool
    {
        int enable = 1;
        return (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable)) != -1);
    }

    auto wouldBlock() -> bool
    {
        return (errno == EWOULDBLOCK || errno == EINPROGRESS);
    }

    auto setSOTimestamp(int fd) -> bool
    {
        int enable = 1;
        return (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, &enable, sizeof(enable)) != -1);
    }

    // auto setMcastTTL(int fd, int ttl) -> bool
    // {
    //     return (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) != -1);
    // }

    // auto setTTL(int fd, int ttl) -> bool
    // {
    //     return (setsockopt(fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) != -1);
    // }

    auto createSocket(Common::Logger &logger, const std::string &t_ip, const std::string &iface, int port, bool is_udp,
                      bool is_blocking, bool is_listening, /*int ttl,*/ bool needs_so_timestamp) -> int
    {
        constexpr int MaxTCPServerBacklog = 1024;
        std::string time_str;
        // Common function -> Separate behavior for Server and Client
        const auto ip = t_ip.empty() ? getIfaceIP(iface) : t_ip;
        logger.log("%:% %() % ip:% iface:% port:% is_udp:% is_blocking : % is_listening : % SO_time : %\n ",
                   __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str), ip, iface, port, is_udp, is_blocking,
                   is_listening, needs_so_timestamp);
        addrinfo hints{.ai_flags = is_listening ? AI_PASSIVE : 0, .ai_family = AF_INET, .ai_socktype = is_udp ? SOCK_DGRAM : SOCK_STREAM, .ai_protocol = is_udp ? IPPROTO_UDP : IPPROTO_TCP};
        if (isdigit(ip.c_str()[0]))
        {
            hints.ai_flags |= AI_NUMERICHOST;
        }
        hints.ai_flags |= AI_NUMERICSERV;
        addrinfo *result = nullptr;
        const auto rc = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result);
        if (rc)
        {
            logger.log("getaddrinfo() failed. error:% errno:%\n",
                       gai_strerror(rc), strerror(errno));
            return -1;
        }
        int fd = -1;
        int enable = 1;
        // This loop will only run once since we gave a specific IP address and port number
        // Returned data-structure is a linked list but only with a single node
        for (addrinfo *rp = result; rp; rp = rp->ai_next)
        {
            fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

            if (fd == -1)
            {
                logger.log("socket() failed. errno:%\n", strerror(errno));
                return -1;
            }

            if (!is_blocking && !setNonBlocking(fd))
            {
                logger.log("setNonBlocking() failed. errno:%\n", strerror(errno));
                return -1;
            }

            if (!is_udp && !setNoDelay(fd))
            {
                logger.log("setNoDelay() failed. errno:%\n", strerror(errno));
                return -1;
            }

            if (!is_listening && connect(fd, rp->ai_addr, rp->ai_addrlen) == 1)
            {
                logger.log("connect() failed. errno:%\n", strerror(errno));
                return -1;
            }

            if (is_listening && setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
            {
                logger.log("setsockopt() SO_REUSEADDR failed. errno:%\n", strerror(errno));
                return -1;
            }

            if (is_listening && bind(fd, rp->ai_addr, rp->ai_addrlen) == -1)
            {
                logger.log("bind() failed. errno:%\n", strerror(errno));
                return -1;
            }

            if (!is_udp && is_listening && listen(fd, MaxTCPServerBacklog) == -1)
            {
                logger.log("listen() failed. errno:%\n", strerror(errno));
                return -1;
            }

            if (needs_so_timestamp && !setSOTimestamp(fd))
            {
                logger.log("setSOTimestamp() failed. errno:%\n", strerror(errno));
                return -1;
            }
        }

        if (result)
        {
            freeaddrinfo(result);
        }
        return fd;
    }
}