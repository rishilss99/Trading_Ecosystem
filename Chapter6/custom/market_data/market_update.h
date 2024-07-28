#pragma once

#include <sstream>
#include "../../common/lf_queue.h"
#include "../../common/types.h"

using namespace Common;

namespace CustomExchange
{
#pragma pack(push, 1)

    enum class MarketUpdateType : uint8_t
    {
        INVALID = 0,
        ADD = 1,
        MODIFY = 2,
        CANCEL = 3,
        TRADE = 4
    };

    auto marketUpdateTypeToString(MarketUpdateType type)
    {
        switch (type)
        {
        case MarketUpdateType::ADD:
            return "ADD";
        case MarketUpdateType::MODIFY:
            return "MODIFY";
        case MarketUpdateType::CANCEL:
            return "CANCEL";
        case MarketUpdateType::TRADE:
            return "TRADE";
        case MarketUpdateType::INVALID:
            return "INVALID";
        default:
            return "UNKNOWN";
        }
    }

    struct MEMarketUpdate
    {
        MarketUpdateType type_ = MarketUpdateType::INVALID;
        OrderId order_id_ = OrderId_INVALID;
        TickerId ticker_id_ = TickerId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty qty_ = Qty_INVALID;
        Priority priority_ = Priority_INVALID;
        auto toString() const
        {
            std::stringstream ss;
            ss << "MEMarketUpdate"
               << " ["
               << " type:" << marketUpdateTypeToString(type_)
               << " ticker:" << tickerIdToString(ticker_id_)
               << " oid:" << orderIdToString(order_id_)
               << " side:" << sideToString(side_)
               << " qty:" << qtyToString(qty_)
               << " price:" << priceToString(price_)
               << " priority:" << priorityToString(priority_)
               << "]";
            return ss.str();
        }
    };
#pragma pack(pop)

    using MEMarketUpdateLFQueue = LFQueue<MEMarketUpdate>;

} // namespace CustomExchange