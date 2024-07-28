#pragma once

#include <cstdint>
#include <limits>
#include "../common/macros.h"

namespace Custom
{
    constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;
    constexpr size_t ME_MAX_TICKERS = 8;
    constexpr size_t ME_MAX_CLIENT_UPDATES = 256 * 1024;
    constexpr size_t ME_MAX_MARKET_UPDATES = 256 * 1024;
    constexpr size_t ME_MAX_NUM_CLIENTS = 256;
    constexpr size_t ME_MAX_ORDER_IDS = 1024 * 1024;
    constexpr size_t ME_MAX_PRICE_LEVELS = 256;

    using OrderId = uint64_t;
    constexpr auto OrderId_INVALID = std::numeric_limits<OrderId>::max();
    inline auto orderIdToString(OrderId order_id) -> std::string
    {
        if (UNLIKELY(order_id == OrderId_INVALID))
        {
            return "INVALID";
        }
        else
        {
            return std::to_string(order_id);
        }
    }

    using TickerId = uint32_t;
    constexpr auto TickerId_INVALID = std::numeric_limits<TickerId>::max();
    inline auto tickerIdToString(TickerId ticker_id) -> std::string
    {
        if (UNLIKELY(ticker_id == TickerId_INVALID))
        {
            return "INVALID";
        }
        else
        {
            return std::to_string(ticker_id);
        }
    }

    using ClientId = uint32_t;
    constexpr auto ClientId_INVALID = std::numeric_limits<ClientId>::max();
    inline auto clientIdToString(ClientId client_id) -> std::string
    {
        if (UNLIKELY(client_id == ClientId_INVALID))
        {
            return "INVALID";
        }
        else
        {
            return std::to_string(client_id);
        }
    }

    using Price = int64_t;
    constexpr auto Price_INVALID = std::numeric_limits<Price>::max();
    inline auto priceToString(Price price) -> std::string
    {
        if (UNLIKELY(price == Price_INVALID))
        {
            return "INVALID";
        }
        return std::to_string(price);
    }

    using Qty = uint32_t;
    constexpr auto Qty_INVALID = std::numeric_limits<Qty>::max();
    inline auto qtyToString(Qty qty) -> std::string
    {
        if (UNLIKELY(qty == Qty_INVALID))
        {
            return "INVALID";
        }
        else
        {
            return std::to_string(qty);
        }
    }

    using Priority = uint64_t;
    constexpr auto Priority_INVALID = std::numeric_limits<Priority>::max();
    inline auto priorityToString(Priority priority) -> std::string
    {
        if (UNLIKELY(priority == Priority_INVALID))
        {
            return "INVALID";
        }
        return std::to_string(priority);
    }

    enum class Side : int8_t
    {
        INVALID = 0,
        BUY = 1,
        SELL = -1
    };

    inline auto sideToString(Side side) -> std::string
    {
        switch (side)
        {
        case Side::INVALID:
            return "INVALID";
        case Side::BUY:
            return "BUY";
        case Side::SELL:
            return "SELL";
        default:
            return "UNKNOWN";
        }
    }
} // namespace Custom
