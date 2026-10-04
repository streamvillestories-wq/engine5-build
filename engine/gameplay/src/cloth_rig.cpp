#include "e5/gameplay/cloth_rig.hpp"

#include <algorithm>
#include <charconv>
#include <map>
#include <system_error>
#include <utility>

namespace e5::gameplay {
namespace {

constexpr std::string_view prefix = "cloth_";

std::optional<int> parse_index(std::string_view text) {
    int value = 0;
    const char* const begin = text.data();
    const char* const end = begin + text.size();
    const auto [parsed_end, error] = std::from_chars(begin, end, value);
    if (text.empty() || error != std::errc{} || parsed_end != end || value < 0) {
        return std::nullopt;
    }
    return value;
}

} // namespace

std::optional<ClothBoneName> parse_cloth_bone_name(std::string_view name) {
    if (!name.starts_with(prefix)) {
        return std::nullopt;
    }
    // Split from the right so the part name itself may contain underscores.
    const auto segment_separator = name.rfind('_');
    if (segment_separator == std::string_view::npos || segment_separator <= prefix.size()) {
        return std::nullopt;
    }
    const auto chain_separator = name.rfind('_', segment_separator - 1);
    if (chain_separator == std::string_view::npos || chain_separator < prefix.size()) {
        return std::nullopt;
    }

    const std::string_view part = name.substr(prefix.size(), chain_separator - prefix.size());
    const auto chain = parse_index(name.substr(chain_separator + 1, segment_separator - chain_separator - 1));
    const auto segment = parse_index(name.substr(segment_separator + 1));
    if (part.empty() || !chain || !segment) {
        return std::nullopt;
    }
    return ClothBoneName{.part = part, .chain = *chain, .segment = *segment};
}

std::vector<ClothChain> find_cloth_chains(std::span<const std::string_view> bone_names) {
    // (part, chain) -> list of (segment, bone index). std::map keeps the
    // result ordered, which makes the simulator setup deterministic.
    std::map<std::pair<std::string, int>, std::vector<std::pair<int, int>>> grouped;
    for (std::size_t index = 0; index < bone_names.size(); ++index) {
        if (const auto parsed = parse_cloth_bone_name(bone_names[index])) {
            grouped[{std::string(parsed->part), parsed->chain}].emplace_back(parsed->segment, static_cast<int>(index));
        }
    }

    std::vector<ClothChain> chains;
    chains.reserve(grouped.size());
    for (auto& [key, segments] : grouped) {
        std::ranges::sort(segments);
        bool contiguous = true;
        for (std::size_t expected = 0; expected < segments.size(); ++expected) {
            contiguous = contiguous && std::cmp_equal(segments[expected].first, expected);
        }
        if (!contiguous) {
            continue;
        }
        ClothChain chain{.part = key.first, .chain = key.second, .bones = {}};
        chain.bones.reserve(segments.size());
        for (const auto& [segment, bone] : segments) {
            chain.bones.push_back(bone);
        }
        chains.push_back(std::move(chain));
    }
    return chains;
}

} // namespace e5::gameplay
