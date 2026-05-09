#pragma once

#include "order_book.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>


class OrderQueue {
 public:
    void push(OrderPtr order);
    std::optional<OrderPtr> pop();
    void stop();

 private:
    std::queue<OrderPtr> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stopped_ = false;

};