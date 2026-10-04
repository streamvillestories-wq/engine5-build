#include "e5/core/config.hpp"

#include <cmath>
#include <format>
#include <optional>

namespace e5 {
namespace {

using OptionValue = std::optional<std::string_view>;

Result<std::string_view> require_value(std::string_view key, OptionValue value) {
    if (!value || value->empty()) {
        return fail(ErrorCode::InvalidArgument, std::format("option '{}' requires a value", key));
    }
    return *value;
}

// Parses "12", "3.5" or ".25": digits with an optional fraction, nothing else.
// Hand-written because the standard alternatives are not portable here:
// floating-point std::from_chars is missing from some Apple libc++ versions,
// and strtof depends on the process locale (a German locale expects "3,5").
std::optional<float> parse_plain_decimal(std::string_view text) {
    double result = 0.0;
    double fraction_scale = 0.1;
    bool seen_digit = false;
    bool in_fraction = false;
    for (const char c : text) {
        if (c == '.' && !in_fraction) {
            in_fraction = true;
        } else if (c >= '0' && c <= '9') {
            seen_digit = true;
            const auto digit = static_cast<double>(c - '0');
            if (in_fraction) {
                result += digit * fraction_scale;
                fraction_scale *= 0.1;
            } else {
                result = result * 10.0 + digit;
            }
        } else {
            return std::nullopt;
        }
    }
    if (!seen_digit) {
        return std::nullopt;
    }
    return static_cast<float>(result);
}

Result<float> number_option(std::string_view key, OptionValue value) {
    return require_value(key, value).and_then([key](std::string_view text) -> Result<float> {
        const auto seconds = parse_plain_decimal(text);
        if (!seconds) {
            return fail(ErrorCode::InvalidArgument,
                        std::format("option '{}' expects a non-negative number, got '{}'", key, text));
        }
        return *seconds;
    });
}

// Like number_option, but accepts a leading minus sign.
Result<float> signed_number_option(std::string_view key, OptionValue value) {
    return require_value(key, value).and_then([key](std::string_view text) -> Result<float> {
        const bool negative = text.starts_with('-');
        const auto magnitude = parse_plain_decimal(negative ? text.substr(1) : text);
        if (!magnitude) {
            return fail(ErrorCode::InvalidArgument, std::format("option '{}' expects a number, got '{}'", key, text));
        }
        return negative ? -*magnitude : *magnitude;
    });
}

// An hour of the day, 0 to 24.
[[nodiscard]] Result<float> hour_option(std::string_view key, std::optional<std::string_view> value) {
    return number_option(key, value).and_then([key](float hour) -> Result<float> {
        if (hour > 24.0F) {
            return fail(ErrorCode::InvalidArgument, std::format("option '{}' expects an hour from 0 to 24", key));
        }
        return hour;
    });
}

Result<void> apply_option(RuntimeConfig& config, std::string_view key, OptionValue value) {
    if (key == "--benchmark") {
        config.benchmark = true;
        return {};
    }
    if (key == "--no-overlay") {
        config.show_overlay = false;
        return {};
    }
    if (key == "--overlay-detail") {
        config.overlay_detail = true;
        return {};
    }
    if (key == "--auto-turn") {
        config.auto_turn = true;
        return {};
    }
    if (key == "--auto-charge") {
        config.auto_charge = true;
        return {};
    }
    if (key == "--auto-fire") {
        config.auto_fire = true;
        return {};
    }
    if (key == "--auto-attack") {
        config.auto_attack = true;
        return {};
    }
    if (key == "--auto-aim") {
        config.auto_aim = true;
        return {};
    }
    if (key == "--auto-move") {
        const std::string_view direction = value.value_or("forward");
        if (direction != "forward" && direction != "back" && direction != "left" && direction != "right") {
            return fail(ErrorCode::InvalidArgument,
                        std::format("option '{}' expects forward, back, left or right, got '{}'", key, direction));
        }
        config.auto_move = direction;
        return {};
    }
    if (key == "--benchmark-seconds") {
        return number_option(key, value).transform([&config](float s) { config.benchmark_seconds = s; });
    }
    if (key == "--camera-pitch") {
        return signed_number_option(key, value).transform([&config](float degrees) {
            config.camera_pitch_degrees = degrees;
        });
    }
    if (key == "--auto-skill") {
        return number_option(key, value).and_then([&config, key](float slot) -> Result<void> {
            if (slot < 1.0F || slot > 10.0F || slot != std::floor(slot)) {
                return fail(ErrorCode::InvalidArgument, std::format("option '{}' expects a slot from 1 to 10", key));
            }
            config.auto_skill = static_cast<int>(slot);
            return {};
        });
    }
    if (key == "--time-of-day") {
        return hour_option(key, value).transform([&config](float hour) { config.time_of_day = hour; });
    }
    if (key == "--start-x") {
        return signed_number_option(key, value).transform([&config](float metres) { config.start_x = metres; });
    }
    if (key == "--start-z") {
        return signed_number_option(key, value).transform([&config](float metres) { config.start_z = metres; });
    }
    if (key == "--camera-yaw") {
        return number_option(key, value).transform([&config](float degrees) { config.camera_yaw_degrees = degrees; });
    }
    if (key == "--benchmark-warmup") {
        return number_option(key, value).transform([&config](float s) { config.benchmark_warmup_seconds = s; });
    }
    if (key == "--benchmark-output") {
        return require_value(key, value).transform([&config](std::string_view path) {
            config.benchmark_output = path;
        });
    }
    if (key == "--camera") {
        return require_value(key, value).transform([&config](std::string_view name) { config.camera_name = name; });
    }
    if (key == "--screenshot") {
        return require_value(key, value).transform([&config](std::string_view path) {
            config.screenshot_output = path;
        });
    }
    if (key.starts_with("--menu")) {
        // For the character select screen, which reads them itself (game/ui/character_select.gd).
        return {};
    }
    if (key == "--server" || key.starts_with("--net-")) {
        // For playing together (game/net/net.gd). The launcher passes --server to every start:
        // a game that did not know it stopped at once.
        return {};
    }
    return fail(ErrorCode::InvalidArgument, std::format("unknown option '{}'", key));
}

} // namespace

Result<RuntimeConfig> parse_runtime_args(std::span<const std::string_view> args) {
    RuntimeConfig config;
    for (const std::string_view arg : args) {
        const auto separator = arg.find('=');
        const std::string_view key = arg.substr(0, separator);
        const OptionValue value =
            separator == std::string_view::npos ? std::nullopt : OptionValue(arg.substr(separator + 1));

        if (auto applied = apply_option(config, key, value); !applied) {
            return std::unexpected(std::move(applied).error());
        }
    }
    return config;
}

} // namespace e5
