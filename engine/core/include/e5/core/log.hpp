#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <string_view>
#include <utility>

namespace e5 {

enum class LogLevel : std::uint8_t { Trace, Debug, Info, Warning, Error };

[[nodiscard]] std::string_view to_string(LogLevel level) noexcept;

// Receives fully formatted messages. The host (Godot bridge, test harness,
// tool) decides where they go; the core never writes to a console itself.
using LogSink = std::function<void(LogLevel, std::string_view)>;

class Logger {
public:
    explicit Logger(LogSink sink, LogLevel min_level = LogLevel::Info);

    void set_min_level(LogLevel level) noexcept { min_level_ = level; }
    [[nodiscard]] LogLevel min_level() const noexcept { return min_level_; }
    [[nodiscard]] bool enabled(LogLevel level) const noexcept { return level >= min_level_; }

    void write(LogLevel level, std::string_view message) const;

    template <typename... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) const {
        // Level check first so disabled messages cost no formatting or allocation.
        if (enabled(level)) {
            write(level, std::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) const {
        log(LogLevel::Info, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args) const {
        log(LogLevel::Warning, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) const {
        log(LogLevel::Error, fmt, std::forward<Args>(args)...);
    }

private:
    LogSink sink_;
    LogLevel min_level_;
};

} // namespace e5
