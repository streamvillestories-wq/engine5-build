#pragma once

#include "e5/core/log.hpp"

namespace e5::bridge {

// Logger that forwards to Godot's output panel / console. This is the one
// deliberate process-wide access point in the bridge: Godot instantiates nodes
// itself, so they cannot receive a logger through their constructors.
[[nodiscard]] const Logger& logger();

} // namespace e5::bridge
