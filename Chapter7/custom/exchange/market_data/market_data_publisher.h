#pragma once

#include <functional>
#include "snapshot_synthesizer.h"

namespace CustomExchange
{
    class MarketDataPublisher
    {
    public:
        MarketDataPublisher(MEMarketUpdateLFQueue *market_updates, const std::string &iface, const std::string &snapshot_ip,
                            int snapshot_port, const std::string &incremental_ip, int incremental_port);

        ~MarketDataPublisher()
        {
            stop();
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(5s);
            delete snapshot_synthesier_;
            snapshot_synthesier_ = nullptr;
        }

        auto start() -> void
        {
            run_.store(true);
            ASSERT((Common::createAndStartThread(-1, "Exchange/MarketDataPublisher", [this]()
                                                 { run(); })) != nullptr,
                   "Failed to start MarketData thread.");
            snapshot_synthesier_->start();
        }

        auto stop() -> void
        {
            run_.store(false);
            snapshot_synthesier_->stop();
        }

        auto run() noexcept -> void;

    private:
        size_t next_inc_seq_num_ = 1;
        MEMarketUpdateLFQueue *outgoing_md_updates_ = nullptr;
        MDPMarketUpdateLFQueue snapshot_md_updates_;
        std::string time_str_;
        std::atomic<bool> run_;
        Logger logger_;
        Common::McastSocket incremental_socket_;
        SnapshotSynthesizer *snapshot_synthesier_ = nullptr;
    };
}