#include "examples_tcp_socket.h"

namespace ExampleSocketUtils
{
    auto TCPSocket::destroy() noexcept -> void
    {
        close(fd_);
        fd_ = -1;
    }

    auto TCPSocket::connect(const std::string &ip, const std::string &iface, int port, bool is_listening) -> int
    {
        destroy();
        fd_ = createSocket(logger_, ip, iface, port, false, false, is_listening, true);
        // What is inInAddr and why are we populating it as such?
        // Common function to both client and server, it populates inInAddr
        // such that it receives from any internet address
        inInAddr.sin_addr.s_addr = INADDR_ANY;
        inInAddr.sin_port = htons(port);
        inInAddr.sin_family = AF_INET;
        return fd_;
    }

    auto TCPSocket::send(const void *data, size_t len) noexcept -> void
    {
        memcpy(send_buffer_ + next_send_valid_index_, data, len);
        next_send_valid_index_ += len;
    }

    auto TCPSocket::sendAndRecv() noexcept -> bool
    {
        char ctrl[CMSG_SPACE(sizeof(struct timeval))];
        struct cmsghdr *cmsg = (struct cmsghdr *)&ctrl;
        struct iovec iov;
        iov.iov_base = rcv_buffer_ + next_rcv_valid_index_;
        iov.iov_len = TCPBufferSize - next_rcv_valid_index_;
        msghdr msg;
        msg.msg_control = ctrl;
        msg.msg_controllen = sizeof(ctrl);
        msg.msg_name = &inInAddr;
        msg.msg_namelen = sizeof(inInAddr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        // Here msg.msg_name has been populated to receive message on the socket
        // from any address using INADDR_ANY.
        const auto n_rcv = recvmsg(fd_, &msg, MSG_DONTWAIT);
        if (n_rcv > 0)
        {
            next_rcv_valid_index_ += n_rcv;
            Common::Nanos kernel_time = 0;
            struct timeval time_kernel;
            if (cmsg->cmsg_level == SOL_SOCKET &&
                cmsg->cmsg_type == SCM_TIMESTAMP &&
                cmsg->cmsg_len == CMSG_LEN(sizeof(time_kernel)))
            {
                memcpy(&time_kernel, CMSG_DATA(cmsg), sizeof(time_kernel));
                kernel_time = time_kernel.tv_sec * Common::NANOS_TO_SECS + time_kernel.tv_usec * Common::NANOS_TO_MICROS;
            }
            const auto user_time = Common::getCurrentNanos();
            logger_.log("%:% %() % read socket:% len:% utime:% ktime : % diff : %\n ", __FILE__, __LINE__, __FUNCTION__,
                        Common::getCurrentTimeStr(&time_str_), fd_,
                        next_rcv_valid_index_, user_time, kernel_time, (user_time - kernel_time));
            recv_callback_(this, kernel_time);
        }
        ssize_t n_send = std::min(TCPBufferSize,
                                  next_send_valid_index_);
        while (n_send > 0)
        {
            auto n_send_this_msg = std::min(static_cast<ssize_t>(next_send_valid_index_), n_send);
            const int flags = MSG_DONTWAIT | MSG_NOSIGNAL |
                              (n_send_this_msg < n_send ? MSG_MORE : 0);
            auto n = ::send(fd_, send_buffer_, n_send_this_msg,
                            flags);
            if (UNLIKELY(n < 0))
            {
                if (!wouldBlock())
                    send_disconnected_ = true;
                break;
            }
            logger_.log("%:% %() % send socket:% len:%\n",
                        __FILE__, __LINE__, __FUNCTION__,
                        Common::getCurrentTimeStr(&time_str_), fd_, n);
            n_send -= n;
            ASSERT(n == n_send_this_msg, "Don't support partial send lengths yet.");
        }
        next_send_valid_index_ = 0;
        return (n_rcv > 0);
    }
}