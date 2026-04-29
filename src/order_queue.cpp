#include "order_queue.h"

void OrderQueue::push(OrderPtr order) {
    // using inner braces forces the mutex to release before .notify_one() runs
    {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(order);
    }
    cv_.notify_one();
}

std::optional<OrderPtr> OrderQueue::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !queue_.empty() || stopped_; });  // lambda for wake conditions
    if (queue_.empty()) return std::nullopt;
    OrderPtr order = queue_.front();
    queue_.pop();
    return order;
}

void OrderQueue::stop() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stopped_ = true;
    }
    cv_.notify_all();
} 