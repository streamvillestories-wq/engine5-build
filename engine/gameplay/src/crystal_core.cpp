#include "e5/gameplay/crystal_core.hpp"

#include <algorithm>
#include <cmath>

namespace e5::gameplay {

CoreSite judge_core_site(const CoreSiteProbe& probe, const CoreRules& rules) noexcept {
    if (probe.underground) {
        return CoreSite::Underground;
    }
    if (probe.height < rules.min_height || probe.lowest < rules.min_height ||
        probe.river_distance < rules.water_clearance || probe.lake_distance < rules.water_clearance) {
        return CoreSite::Water;
    }
    if (probe.road_distance < rules.road_clearance) {
        return CoreSite::Road;
    }
    if (probe.keep_out) {
        return CoreSite::KeepOut;
    }
    if (probe.slope > rules.max_slope) {
        return CoreSite::Steep;
    }
    if (probe.highest - probe.lowest > rules.max_unevenness) {
        return CoreSite::Uneven;
    }
    if (probe.nearest_core < rules.spacing) {
        return CoreSite::TooClose;
    }
    return CoreSite::Ok;
}

const char* core_site_reason(CoreSite site) noexcept {
    switch (site) {
    case CoreSite::Ok:
        return "Place your core here";
    case CoreSite::Underground:
        return "Not under the island";
    case CoreSite::Water:
        return "Too near the water";
    case CoreSite::Road:
        return "Not on a road or trail";
    case CoreSite::Uneven:
        return "The ground is too uneven";
    case CoreSite::Steep:
        return "The ground is too steep";
    case CoreSite::KeepOut:
        return "Not in a camp or at a landmark";
    case CoreSite::TooClose:
        return "Too close to another core";
    }
    return "";
}

float nearest_core_distance(CoreSpot candidate, std::span<const CoreSpot> others) noexcept {
    float nearest = 1e9F;
    for (const CoreSpot& other : others) {
        nearest = std::min(nearest, std::hypot(other.x - candidate.x, other.z - candidate.z));
    }
    return nearest;
}

} // namespace e5::gameplay
