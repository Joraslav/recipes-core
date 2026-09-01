#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace app {

/**
 * @brief Categories of errors returned by application use cases.
 */
enum class ErrorCode : uint8_t {
    Validation,
    NotFound,
    Conflict,
    Storage,
    Overloaded
};

/**
 * @brief Transport-independent application error.
 */
class Error final {
 public:
    /**
     * @brief Creates an error by copying its message.
     * @param c Error category.
     * @param msg Human-readable error message.
     */
    Error(ErrorCode c, const std::string& msg) : code_(c), message_(msg) {}
    /**
     * @brief Creates an error by taking ownership of its message.
     * @param c Error category.
     * @param msg Human-readable error message.
     */
    Error(ErrorCode c, std::string&& msg)
        : code_(c), message_(std::move(msg)) {}

    /** @brief Returns the machine-readable error category. */
    [[nodiscard]] ErrorCode GetCode() const noexcept { return code_; }
    /** @brief Returns the human-readable error message. */
    [[nodiscard]] const std::string& GetMessage() const noexcept {
        return message_;
    }

 private:
    ErrorCode code_;
    std::string message_;
};

}  // namespace app