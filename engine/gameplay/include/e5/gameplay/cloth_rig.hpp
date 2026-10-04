#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace e5::gameplay {

// Naming convention shared by the asset tool (tools/blender/cloth_rig.py) and
// the runtime: cloth bones are called cloth_<part>_<chain>_<segment>, e.g.
// "cloth_skirt_03_1". Keeping the convention in engine-agnostic code means the
// rules for what counts as a valid chain are tested without a running engine.

struct ClothBoneName {
    std::string_view part; // view into the name that was parsed
    int chain = 0;
    int segment = 0;
};

// Returns nothing if the name does not follow the convention.
[[nodiscard]] std::optional<ClothBoneName> parse_cloth_bone_name(std::string_view name);

struct ClothChain {
    std::string part;
    int chain = 0;
    // Indices into the bone list passed to find_cloth_chains, ordered from
    // the root (segment 0) to the tip.
    std::vector<int> bones;
};

// Groups cloth bones into chains, sorted by part then chain number. A chain
// whose segments are not exactly 0..n-1 (a gap or a duplicate) is dropped:
// simulating a broken chain would visibly detach the cloth.
[[nodiscard]] std::vector<ClothChain> find_cloth_chains(std::span<const std::string_view> bone_names);

} // namespace e5::gameplay
