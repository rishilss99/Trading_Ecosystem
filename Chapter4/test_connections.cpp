#include "logging.h"
#include "test_tcp_client.h"
#include "test_tcp_server.h"

int main()
{
    using namespace Common;

    std::string time_str;
    Logger logger_("test_connections.log");
    auto tcpServerRecvCallback = [&logger_](TestTCPClient *socket, Nanos rx_time) noexcept
    {
        logger_.log("TCPServer::defaultRecvCallback() socket:% len:% rx:% \n", socket->socket_fd_,
                    socket->next_recv_index_, rx_time);
        const std::string reply = "TCPServer received msg:" +
                                  std::string(socket->inbound_data_.data(), socket->next_recv_index_);
        socket->next_recv_index_ = 0;
        socket->send(reply);
    };

    // auto tcpServerRecvFinishedCallback = [&logger_]() noexcept
    // {
    //     logger_.log("TCPServer::defaultRecvFinishedCallback()\n");
    // };

    auto tcpClientRecvCallback = [&logger_](TestTCPClient *socket, Nanos rx_time) noexcept
    {
        const std::string recv_msg = std::string(socket->inbound_data_.data(), socket->next_recv_index_);
        socket->next_recv_index_ = 0;
        logger_.log("TCPSocket::defaultRecvCallback() socket:% len:% rx:% msg:% \n", socket->socket_fd_,
                    socket->next_recv_index_, rx_time, recv_msg);
    };

    const std::string iface = "lo";
    const std::string ip = "127.0.0.1";
    const int port = 12345;

    // Create and initialize Server and Clients
    logger_.log("Creating TCPServer on iface:% port:%\n", iface, port);
    TestTCPServer server(logger_);
    server.recv_callback_ = tcpServerRecvCallback;
    // server.recv_finished_callback_ = tcpServerRecvFinishedCallback;
    server.listen(iface, port);
    std::vector<std::unique_ptr<TestTCPClient>> clients;
    const int n_clients = 2;
    for (size_t i = 0; i < n_clients; i++)
    {
        clients.push_back(std::make_unique<TestTCPClient>(logger_));
        clients[i]->recv_callback_ = tcpClientRecvCallback;
        logger_.log("Connecting TCPClient-[%] on ip:% iface:% port:%\n", i, ip, iface, port);
        clients[i]->connect(ip, iface, port);
        server.poll();
    }

    // Clients send data and Client/Server use polling mechanism
    using namespace std::chrono_literals;
    const int n_iters = 1;
    for (auto itr = 0; itr < n_iters; ++itr)
    {
        for (size_t i = 0; i < n_clients; ++i)
        {
            const std::string client_msg = "CLIENT-[" + std::to_string(i) + "] : Sending " + std::to_string(itr * 100 + i);
            logger_.log("Sending TCPClient-[%] %\n", i, client_msg);
            clients[i]->send(client_msg);
            clients[i]->sendAndRecv();

            std::this_thread::sleep_for(500ms);
            server.poll();
            server.sendAndRecv();
        }
    }

    for (size_t i = 0; i < n_clients; ++i)
    {
        clients[i]->sendAndRecv();
    }

    return 0;
}