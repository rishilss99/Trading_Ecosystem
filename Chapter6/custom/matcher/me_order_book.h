#pragma once

#include "../../common/types.h"
#include "../../common/mem_pool.h"
// #include "../../common/logging.h"
#include "../../exchange/matcher/me_order.h"
#include "../../exchange/market_data/market_update.h"
#include "../../exchange/order_server/client_response.h"
#include "../../exchange/matcher/matching_engine.h"

using namespace Common;

namespace Exchange
{
    class MatchingEngine;

    class MEOrderBook final
    {
    public:
        explicit MEOrderBook(TickerId ticker_id, Logger *logger, MatchingEngine *matching_engine);
        ~MEOrderBook();

        auto add(ClientId client_id, OrderId client_order_id, TickerId ticker_id, Side side, Price price, Qty qty) noexcept -> void;

        auto cancel(ClientId client_id, OrderId order_id, TickerId ticker_id) noexcept -> void;

        // Boilerplate code to delete default, copy & move constructors and
        // assignment-operators.
        MEOrderBook() = delete;
        MEOrderBook(const MEOrderBook &) = delete;
        MEOrderBook(MEOrderBook &&) = delete;
        MEOrderBook &operator=(const MEOrderBook &) = delete;
        MEOrderBook &operator=(MEOrderBook &&) = delete;

    private:
        auto generateNewMarketOrderId() noexcept -> OrderId
        {
            return next_market_order_id_++;
        }

        auto priceToIndex(Price price) const noexcept
        {
            return (price % ME_MAX_PRICE_LEVELS);
        }

        auto getOrdersAtPrice(Price price) const noexcept -> MEOrdersAtPrice *
        {
            return price_orders_at_price_.at(priceToIndex(price));
        }

        auto getNextPriority(Price price) noexcept
        {
            const auto orders_at_price = getOrdersAtPrice(price);
            if (!orders_at_price)
            {
                return 1UL;
            }
            return orders_at_price->first_me_order_->prev_order_->priority_ + 1;
        }

        auto match(TickerId ticker_id, ClientId client_id, Side side, OrderId client_order_id, OrderId new_market_order_id, MEOrder* itr, Qty *leaves_qty) noexcept;

        auto checkForMatch(ClientId client_id, OrderId client_order_id, TickerId ticker_id, Side side, Price price, Qty qty, OrderId new_market_order_id) noexcept;

        auto addOrdersAtPrice(MEOrdersAtPrice *new_orders_at_price) noexcept
        {
            price_orders_at_price_.at(new_orders_at_price->price_) = new_orders_at_price;
            const auto best_orders_by_price = (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_);
            if (UNLIKELY(!best_orders_by_price))
            {
                (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_) = new_orders_at_price;
                new_orders_at_price->prev_entry_ = new_orders_at_price->next_entry_ = new_orders_at_price;
            }
            else
            {
                auto target = best_orders_by_price;
                bool add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_) ||
                                  (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
                if (add_after)
                {
                    target = target->next_entry_;
                    add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_) ||
                                 (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
                }
                while (add_after && target != best_orders_by_price)
                {
                    add_after = ((new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ > target->price_) ||
                                 (new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ < target->price_));
                    if (add_after)
                    {
                        target = target->next_entry_;
                    }
                }

                // I THINK CODE BELOW IS REDUNDANT
                // new_orders_at_price->prev_entry_ = target->prev_entry_;
                // new_orders_at_price->next_entry_ = target;
                // target->prev_entry_->next_entry_ = new_orders_at_price;
                // target->prev_entry_ = new_orders_at_price;

                // if ((new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ > best_orders_by_price->price_) ||
                //     (new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ < best_orders_by_price->price_))
                // {
                //     target->next_entry_ = (target->next_entry_ == best_orders_by_price ? new_orders_at_price : target->next_entry_);
                //     (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_) = new_orders_at_price;
                // }

                if (add_after)
                { // add new_orders_at_price after target.
                    if (target == best_orders_by_price)
                    {
                        target = best_orders_by_price->prev_entry_;
                    }
                    new_orders_at_price->prev_entry_ = target;
                    target->next_entry_->prev_entry_ = new_orders_at_price;
                    new_orders_at_price->next_entry_ = target->next_entry_;
                    target->next_entry_ = new_orders_at_price;
                }
                else
                { // add new_orders_at_price before target.
                    new_orders_at_price->prev_entry_ = target->prev_entry_;
                    new_orders_at_price->next_entry_ = target;
                    target->prev_entry_->next_entry_ = new_orders_at_price;
                    target->prev_entry_ = new_orders_at_price;

                    if ((new_orders_at_price->side_ == Side::BUY && new_orders_at_price->price_ > best_orders_by_price->price_) ||
                        (new_orders_at_price->side_ == Side::SELL && new_orders_at_price->price_ < best_orders_by_price->price_))
                    {
                        target->next_entry_ = (target->next_entry_ == best_orders_by_price ? new_orders_at_price : target->next_entry_);
                        (new_orders_at_price->side_ == Side::BUY ? bids_by_price_ : asks_by_price_) = new_orders_at_price;
                    }
                }
            }
        }

        auto addOrder(MEOrder *order) noexcept
        {
            const auto orders_at_price = getOrdersAtPrice(order->price_);
            if (!orders_at_price)
            {
                order->next_order_ = order->prev_order_ = order;
                auto new_orders_at_price = orders_at_price_pool_.allocate(order->side_, order->price_, order, nullptr, nullptr);
                addOrdersAtPrice(new_orders_at_price);
            }
            else
            {
                // Insertion at the tail of DLL
                auto first_order = orders_at_price->first_me_order_;
                first_order->prev_order_->next_order_ = order;
                order->prev_order_ = first_order->prev_order_;
                first_order->prev_order_ = order;
                order->next_order_ = first_order;
            }
            cid_oid_to_order_.at(order->client_id_).at(order->client_order_id_) = order;
        }

        auto removeOrdersAtPrice(Side side, Price price) noexcept
        {
            const auto best_orders_by_price = (side == Side::BUY ? bids_by_price_ : asks_by_price_);
            auto orders_at_price = getOrdersAtPrice(price);
            if(UNLIKELY(orders_at_price->next_entry_ == orders_at_price))
            {
                (side == Side::BUY ? bids_by_price_ : asks_by_price_) = nullptr;
            }
            else
            {
                orders_at_price->next_entry_->prev_entry_ = orders_at_price->prev_entry_;
                orders_at_price->prev_entry_->next_entry_ = orders_at_price->next_entry_;
                if(best_orders_by_price == orders_at_price)
                {
                    (side == Side::BUY ? bids_by_price_ : asks_by_price_) = orders_at_price->next_entry_;
                }
                orders_at_price->next_entry_ = orders_at_price->prev_entry_ = nullptr;
            }
            price_orders_at_price_.at(priceToIndex(price)) = nullptr;
            orders_at_price_pool_.deallocate(orders_at_price);
        }

        auto removeOrder(MEOrder *order) noexcept
        {
            auto orders_at_price = getOrdersAtPrice(order->price_);
            if (order->next_order_ == order)
            {
                removeOrdersAtPrice(order->side_, order->price_);
            }
            else // Remove the link
            {
                const auto order_after = order->next_order_;
                const auto order_before = order->prev_order_;
                order_before->next_order_ = order_after;
                order_after->prev_order_ = order_before;
                if(orders_at_price->first_me_order_ == order)
                {
                    orders_at_price->first_me_order_ = order;
                }
                order->prev_order_ = order->next_order_ = nullptr;
            }
            cid_oid_to_order_.at(order->client_id_).at(order->client_order_id_) = nullptr;
            order_pool_.deallocate(order);
        }

        TickerId ticker_id_ = TickerId_INVALID;
        MatchingEngine *matching_engine_ = nullptr;
        ClientOrderHashMap cid_oid_to_order_;
        MemPool<MEOrdersAtPrice> orders_at_price_pool_;
        MEOrdersAtPrice *bids_by_price_ = nullptr;
        MEOrdersAtPrice *asks_by_price_ = nullptr;
        OrdersAtPriceHashMap price_orders_at_price_;
        MemPool<MEOrder> order_pool_;
        MEClientResponse client_response_; // Why not a pointer? Ans: Coz it's a placeholder member to send client response
        MEMarketUpdate market_update_;     // Why not a pointer? Ans: Coz it's a placeholder member to send market update
        OrderId next_market_order_id_ = 1;
        std::string time_str_;
        Logger *logger_ = nullptr;
    };

    typedef std::array<MEOrderBook *, ME_MAX_TICKERS> OrderBookHashMap;
} // namespace CustomExchange