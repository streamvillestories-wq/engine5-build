#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <utility>

namespace e5 {

enum class ErrorCode : std::uint8_t {
    InvalidArgument,
    NotFound,
    IoFailure,
    InvalidState,
};

struct Error {
    ErrorCode code;
    std::string message;
};

// Recoverable failures are returned, not thrown: the code runs inside Godot,
// which is built without exception support across its ABI boundary.
template <typename T>
using Result = std::expected<T, Error>;

[[nodiscard]] inline std::unexpected<Error> fail(ErrorCode code, std::string message) {
    return std::unexpected(Error{.code = code, .message = std::move(message)});
}

} // namespace e5
