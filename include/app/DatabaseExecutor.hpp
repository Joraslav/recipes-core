#pragma once

#include "app/Error.hpp"
#include "app/RecipeService.hpp"
#include "app/SqliteRecipeRepository.hpp"
#include "db/DBManager.hpp"

#include <boost/asio/associated_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/post.hpp>

#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <expected>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>

namespace app {

/**
 * @brief Serializes blocking SQLite work on one dedicated worker thread.
 * @details The executor owns its `DBManager`, repository and service. The
 * queue capacity applies to pending work; an operation already running on the
 * worker does not occupy a queue slot. `Submit` returns `Overloaded` when no
 * slot is available and `Storage` after shutdown.
 */
class DatabaseExecutor final {
 public:
    /**
     * @brief Creates and starts a worker for one SQLite database.
     * @param db_path Path passed to `db::DBManager`.
     * @param queue_capacity Maximum number of pending operations.
     * @pre `queue_capacity > 0`.
     * @throw SQLite exception if the database cannot be opened.
     */
    explicit DatabaseExecutor(std::string_view db_path, size_t queue_capacity);
    ~DatabaseExecutor();

    DatabaseExecutor(const DatabaseExecutor&) = delete;
    DatabaseExecutor& operator=(const DatabaseExecutor&) = delete;
    DatabaseExecutor(DatabaseExecutor&&) = delete;
    DatabaseExecutor& operator=(DatabaseExecutor&&) = delete;

    /**
     * @brief Queues an operation for serialized execution against the service.
     * @tparam Function Callable returning `std::expected<T, app::Error>`.
     * @param operation Work to run on the database worker.
     * @return Future containing the operation result, `Overloaded` when the
     * queue is full, or `Storage` after shutdown.
     * @pre The callable must not retain a reference to `RecipeService` after it
     * returns.
     */
    template <typename Function>
        requires std::invocable<Function, RecipeService&>
    [[nodiscard]] auto Submit(Function&& operation)
        -> std::future<std::invoke_result_t<Function, RecipeService&>>;

    /**
     * @brief Queues an operation and completes it on the handler's executor.
     * @tparam Function Callable returning `std::expected<T, app::Error>`.
     * @tparam CompletionToken Asio completion token.
     * @param operation Work to run on the database worker.
     * @param token Completion token receiving the operation result.
     * @return Asio asynchronous operation result for `void(Result)`.
     * @details Cobalt callers can pass `boost::cobalt::use_op` and co_await
     * the returned operation without blocking a network worker.
     */
    template <typename Function, typename CompletionToken>
        requires std::invocable<Function, RecipeService&>
    auto AsyncSubmit(Function&& operation, CompletionToken&& token);

 private:
    void Run();
    void Stop() noexcept;

    db::DBManager db_manager_;
    SqliteRecipeRepository repository_;
    RecipeService service_;
    size_t queue_capacity_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<std::function<void()>> queue_;
    bool stopping_{false};
    std::jthread worker_;
};

template <typename Function>
    requires std::invocable<Function, RecipeService&>
auto DatabaseExecutor::Submit(Function&& operation)
    -> std::future<std::invoke_result_t<Function, RecipeService&>> {
    using Result = std::invoke_result_t<Function, RecipeService&>;
    static_assert(
        requires { Result{std::unexpected(Error{ErrorCode::Overloaded, ""})}; },
        "DatabaseExecutor operations must return std::expected<T, app::Error>");

    auto promise = std::make_shared<std::promise<Result>>();
    std::future<Result> future = promise->get_future();
    auto operation_ptr = std::make_shared<std::decay_t<Function>>(
        std::forward<Function>(operation));

    std::lock_guard lock(mutex_);
    if (stopping_) {
        promise->set_value(std::unexpected(
            Error{ErrorCode::Storage, "Database executor is stopping"}));
        return future;
    }
    if (queue_.size() >= queue_capacity_) {
        promise->set_value(std::unexpected(
            Error{ErrorCode::Overloaded, "Database executor queue is full"}));
        return future;
    }

    queue_.emplace([this, promise = std::move(promise),
                    operation = std::move(operation_ptr)]() mutable {
        try {
            promise->set_value(std::invoke(*operation, service_));
        } catch (const std::exception& exception) {
            promise->set_value(
                std::unexpected(Error{ErrorCode::Storage, exception.what()}));
        } catch (...) {
            promise->set_value(std::unexpected(
                Error{ErrorCode::Storage, "Unknown database worker error"}));
        }
    });
    condition_.notify_one();
    return future;
}

template <typename Function, typename CompletionToken>
    requires std::invocable<Function, RecipeService&>
auto DatabaseExecutor::AsyncSubmit(Function&& operation,
                                   CompletionToken&& token) {
    using Result = std::invoke_result_t<Function, RecipeService&>;
    static_assert(
        requires { Result{std::unexpected(Error{ErrorCode::Overloaded, ""})}; },
        "DatabaseExecutor operations must return std::expected<T, app::Error>");

    return boost::asio::async_initiate<void(Result)>(
        [this, operation = std::forward<Function>(operation)](
            auto&& completion_handler) mutable {
            using Handler = std::decay_t<decltype(completion_handler)>;
            auto handler = std::make_shared<Handler>(
                std::forward<decltype(completion_handler)>(completion_handler));
            const auto handler_executor =
                boost::asio::get_associated_executor(*handler);
            auto operation_ptr =
                std::make_shared<std::decay_t<Function>>(std::move(operation));
            const auto complete = [handler, handler_executor](Result result) {
                boost::asio::post(
                    handler_executor,
                    [handler, result = std::move(result)]() mutable {
                        std::move (*handler)(std::move(result));
                    });
            };

            std::lock_guard lock(mutex_);
            if (stopping_) {
                complete(std::unexpected(Error{
                    ErrorCode::Storage, "Database executor is stopping"}));
                return;
            }
            if (queue_.size() >= queue_capacity_) {
                complete(std::unexpected(Error{
                    ErrorCode::Overloaded, "Database executor queue is full"}));
                return;
            }

            queue_.emplace([this, operation = std::move(operation_ptr),
                            complete = std::move(complete)]() mutable {
                try {
                    complete(std::invoke(*operation, service_));
                } catch (const std::exception& exception) {
                    complete(std::unexpected(
                        Error{ErrorCode::Storage, exception.what()}));
                } catch (...) {
                    complete(std::unexpected(Error{
                        ErrorCode::Storage, "Unknown database worker error"}));
                }
            });
            condition_.notify_one();
        },
        std::forward<CompletionToken>(token));
}

}  // namespace app