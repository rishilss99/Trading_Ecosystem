#pragma once

#include <sstream>
#include <../../common/lf_queue.h>
#include <../../common/types.h>

using namespace Common;

namespace CustomExchange
{
#pragma pack(push, 1)

    enum class ClientRequestType : uint8_t
    {
        INVALID = 0,
        NEW = 1,
        CANCEL = 2
    };

    inline std::string clientRequestTypeToString(ClientRequestType type)
    {
        switch (type)
        {
        case ClientRequestType::INVALID:
            return "INVALID";
        case ClientRequestType::NEW:
            return "NEW";
        case ClientRequestType::CANCEL:
            return "CANCEL";
        default:
            return "UNKNOWN";
        }
    }

    struct MEClientRequest
    {
        ClientRequestType type_ = ClientRequestType::INVALID;
        ClientId client_id_ = ClientId_INVALID;
        TickerId ticker_id_ = TickerId_INVALID;
        OrderId order_id_ = OrderId_INVALID;
        Price price_ = Price_INVALID;
        Qty qty_ = Qty_INVALID;
        Side side_ = Side::INVALID;

        auto toString() const
        {
            std::stringstream ss;
            ss << "MEClientRequest"
               << " ["
               << "type:" << clientRequestTypeToString(type_)
               << " client:" << clientIdToString(client_id_)
               << " ticker:" << tickerIdToString(ticker_id_)
               << " oid:" << orderIdToString(order_id_)
               << " side:" << sideToString(side_)
               << " qty:" << qtyToString(qty_)
               << " price:" << priceToString(price_)
               << "]";
            return ss.str();
        }
    };

#pragma pack(pop)

    using ClientRequestLFQueue = LFQueue<MEClientRequest>;
} // namespace CustomExchange
