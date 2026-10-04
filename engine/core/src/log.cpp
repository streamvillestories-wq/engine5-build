#include "e5/core/log.hpp"

namespace e5 {

std::string_view to_string(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace:
        return "trace";
    case LogLevel::Debug:
        return "debug";
    case LogLevel::Info:
        return "info";
    case LogLevel::Warning:
        return "warning";
    case LogLevel::Error:
        return "error";
    }
    return "unknown";
}

Logger::Logger(LogSink sink, LogLevel min_level) : sink_(std::move(sink)), min_level_(min_level) {}

void Logger::write(LogLevel level, std::string_view message) const {
    if (sink_ && enabled(level)) {
        sink_(level, message);
    }
}

} // namespace e5
