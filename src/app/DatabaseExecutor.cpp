#include "DatabaseExecutor.hpp"

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>

namespace app {

DatabaseExecutor::DatabaseExecutor(std::string_view db_path,
                                   size_t queue_capacity)
    : db_manager_(db_path),
      repository_(db_manager_),
      service_(repository_),
      queue_capacity_(queue_capacity) {
    if (queue_capacity == 0) {
        throw std::invalid_argument("Database queue capacity must be positive");
    }
    worker_ = std::jthread([this] { Run(); });
}

DatabaseExecutor::~DatabaseExecutor() { Stop(); }

void DatabaseExecutor::Run() {
    while (true) {
        std::function<void()> operation;
        {
            std::unique_lock lock(mutex_);
            condition_.wait(lock,
                            [this] { return stopping_ || !queue_.empty(); });
            if (stopping_ && queue_.empty()) {
                return;
            }
            operation = std::move(queue_.front());
            queue_.pop();
        }
        operation();
    }
}

void DatabaseExecutor::Stop() noexcept {
    {
        const std::scoped_lock lock(mutex_);
        stopping_ = true;
    }
    condition_.notify_one();
}

}  // namespace app