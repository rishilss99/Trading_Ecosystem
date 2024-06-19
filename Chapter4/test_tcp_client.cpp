#include "test_tcp_client.h"

using namespace Common;
using namespace std;

auto TestTCPClient::connect(const string &ip, const string &iface, int port) -> void
{

    const SocketCfg socket_cfg{ip, iface, port, false, false, true};

    // In case of client, this will create socket and connect with given ip to server
    logger_.log("%", socket_cfg.toString());
    socket_fd_ = createSocket(logger_, socket_cfg);
}

auto TestTCPClient::send(const std::string &msg) noexcept -> void
{
    memcpy(outbound_data_.data() + next_send_index_, msg.c_str(), msg.length());
    next_send_index_ += msg.length();
}

auto TestTCPClient::sendAndRecv() noexcept -> bool
{
    char ctrl[CMSG_SPACE(sizeof(timeval))];
    cmsghdr *cmsg = (cmsghdr *)&ctrl;
    iovec iov;
    iov.iov_base = inbound_data_.data() + next_recv_index_;
    iov.iov_len = TCPBufferSize - next_recv_index_;
    msghdr msg;
    msg.msg_control = ctrl;
    msg.msg_controllen = sizeof(ctrl);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    const auto n_rcv = recvmsg(socket_fd_, &msg, MSG_DONTWAIT);
    if (n_rcv > 0)
    {
        next_recv_index_ += n_rcv;
        Nanos kernel_time = 0;
        timeval time_kernel;
        if (cmsg->cmsg_level == SOL_SOCKET &&
            cmsg->cmsg_type == SCM_TIMESTAMP &&
            cmsg->cmsg_len == CMSG_LEN(sizeof(time_kernel)))
        {
            memcpy(&time_kernel, CMSG_DATA(cmsg), sizeof(time_kernel));
            kernel_time = time_kernel.tv_sec * Common::NANOS_TO_SECS + time_kernel.tv_usec * Common::NANOS_TO_MICROS;
        }
        const auto user_time = Common::getCurrentNanos();
        logger_.log("%:% %() % read socket:% len:% utime:% ktime : % diff : %\n ", __FILE__, __LINE__, __FUNCTION__,
                    Common::getCurrentTimeStr(&time_str_), socket_fd_, next_recv_index_, user_time, kernel_time, (user_time - kernel_time));
        recv_callback_(this, kernel_time);
    }
    if (next_send_index_ > 0)
    {
        // Non-blocking call to send data.
        const auto n = ::send(socket_fd_, outbound_data_.data(), next_send_index_, MSG_DONTWAIT | MSG_NOSIGNAL);
        logger_.log("%:% %() % send socket:% len:%\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_), socket_fd_, n);
    }
    return (n_rcv > 0);
}