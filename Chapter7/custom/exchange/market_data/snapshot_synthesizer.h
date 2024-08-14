#pragma once

#include "../../../common/types.h"
#include "../../../common/thread_utils.h"
#include "../../../common/lf_queue.h"
#include "../../../common/macros.h"
#include "../../../common/mcast_socket.h"
#include "../../../common/mem_pool.h"
#include "../../../common/logging.h"

#include "market_update.h"
#include "../matcher/me_order.h"

namespace CustomExchange
{
    class SnapshotSynthesizer
    {
    public:
        SnapshotSynthesizer(MDPMarketUpdateLFQueue *market_updates, const std::string &iface, const std::string &snapshot_ip, int snapshot_port);
        ~SnapshotSynthesizer();
        
        auto start() -> void;
        
        auto stop() -> void;
        
        void run();

        auto addToSnapshot(const MDPMarketUpdate *market_update);

        auto publishSnapshot();

        void run();

    private:
        MDPMarketUpdateLFQueue *snapshot_md_updates_ = nullptr;
        std::atomic<bool> run_ = false;
        Logger logger_;
        std::string time_str_;
        Common::McastSocket snapshot_socket_;
        size_t last_inc_seq_num_ = 0;
        Nanos last_snapshot_time_ = 0;
        std::array<std::array<MEMarketUpdate *, ME_MAX_ORDER_IDS>, ME_MAX_TICKERS> ticker_orders_;
        MemPool<MEMarketUpdate> order_pool_;
    };
}