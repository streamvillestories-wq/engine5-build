#include "godot_log.hpp"

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstdint>

namespace e5::bridge {
namespace {

void godot_sink(LogLevel level, std::string_view message) {
    const godot::String text =
        godot::String("[e5] ") + godot::String::utf8(message.data(), static_cast<std::int64_t>(message.size()));
    switch (level) {
    case LogLevel::Error:
        godot::UtilityFunctions::push_error(text);
        break;
    case LogLevel::Warning:
        godot::UtilityFunctions::push_warning(text);
        break;
    case LogLevel::Trace:
    case LogLevel::Debug:
    case LogLevel::Info:
        godot::UtilityFunctions::print(text);
        break;
    }
}

} // namespace

const Logger& logger() {
    static const Logger instance(godot_sink, LogLevel::Info);
    return instance;
}

} // namespace e5::bridge
