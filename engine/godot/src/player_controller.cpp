#include "player_controller.hpp"

#include "aim_offset.hpp"
#include "arrow.hpp"
#include "arrow_rain.hpp"
#include "bird.hpp"
#include "black_hole.hpp"
#include "bow_string.hpp"
#include "combat.hpp"
#include "e5/core/profiling.hpp"
#include "effect.hpp"
#include "enemy.hpp"
#include "godot_log.hpp"
#include "health_hud.hpp"
#include "input_actions.hpp"
#include "inventory.hpp"
#include "lightning_arc.hpp"
#include "lingering.hpp"
#include "skill_bar_hud.hpp"
#include "spell_bolt.hpp"
#include "target.hpp"

#include <godot_cpp/classes/bone_attachment3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/omni_light3d.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/spring_arm3d.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>
#include <string_view>
#include <utility>

namespace e5::bridge {

// How a spell or blow is timed against its clip (declared in the header for the controller's methods).
struct SpellTiming {
    float playback_scale; // how much faster than authored the clip is played
    float strike_at;      // seconds into the clip: the hands are furthest out
    float end_at;         // seconds into the clip: he is free to move again
};

namespace {

// Ground speeds the walk and run clips were authored for. Playback is scaled
// by actual speed / authored speed so the feet slide less.
constexpr float walk_clip_speed = 1.6F;
constexpr float run_clip_speed = 4.5F;

// Below this horizontal speed the character keeps its current facing instead
// of snapping to a direction derived from numerical noise.
constexpr float min_turn_speed = 0.3F;

// Over-the-shoulder camera while aiming: with the camera straight behind her,
// the character would cover whatever the crosshair points at.
constexpr float aim_shoulder_offset = 0.45F;  // metres to the right
constexpr float aim_camera_distance = 1.7F;   // metres behind
constexpr float aim_camera_blend_rate = 8.0F; // 1/s
constexpr float mount_camera_back = 2.2F;     // metres further back on the horse
constexpr float mount_camera_rate = 3.0F;     // 1/s
constexpr float aim_ray_length = 200.0F;      // metres
constexpr float dodge_hand_over_seconds = 0.18F;
constexpr float usual_fade_seconds = 0.2F;   // from one whole-body clip to the next
constexpr float landing_fade_seconds = 0.3F; // out of the jump clip, coming down on the spot
// ... and coming down on the move. Short: while the flight pose is still mixed in her legs do not
// step, and for as long as that lasted (0.38 s at first) she glided over the ground.
constexpr float landing_moving_fade_seconds = 0.12F;
constexpr float dodge_cooldown_seconds = 0.8F; // from the end of one dodge to the start of the next
// Whoever stands this close ahead of her chest is what she shoots at, whatever the crosshair covers.
constexpr float point_blank_reach = 2.6F;  // metres
constexpr float point_blank_height = 1.2F; // metres above her feet
// The arrow appears on the string once the draw clip has had time to reach the quiver.
constexpr float arrow_appears_at_draw = 0.35F;
// From this point of the draw the string starts following the drawing hand;
// before it, the hand is still fetching the arrow.
constexpr float hand_takes_string_at = 0.55F;
// Distance from the wrist bone to where the fingers hold the string.
constexpr float fingers_from_wrist = 0.07F;
constexpr const char* crosshair_texture_path = "res://ui/icons/crosshair.svg";
// Half the crosshair's width on a 1600 x 900 screen (the interface is scaled from that).
constexpr float crosshair_half_size = 18.0F;

// The spine only bends so far; beyond this the arrow still follows the crosshair.
constexpr float max_body_pitch = 0.8F; // radians, about 46 degrees
constexpr float power_shot_speed_bonus = 0.7F;
constexpr float prewarm_seconds = 0.5F;
constexpr float rain_body_pitch = 0.75F; // radians she leans back to shoot skyward
constexpr float rain_max_range = 45.0F;  // metres from the archer
constexpr float rain_signal_arrow_speed = 60.0F;
constexpr double rain_signal_arrow_seconds = 0.6;
constexpr float tip_glow_particle_share = 0.2F;
constexpr int fan_arrow_count = 5;
constexpr float fan_spread = 0.42F;       // radians between the outermost arrows, about 24 degrees
constexpr float fire_blast_radius = 3.0F; // metres

// The archer's newer arrows (to choose from): what each carries, shows and leaves behind. The
// scenes are named here and not set on the hero: there may be fewer of these skills soon.
struct SpecialArrow {
    gameplay::SkillId skill;
    E5Arrow::Special special;
    const char* trail;
    const char* impact;
    const char* first;  // see E5Arrow::set_special
    const char* second; //
};
constexpr std::array special_arrows{
    SpecialArrow{.skill = gameplay::SkillId::VenomArrow,
                 .special = E5Arrow::Special::Venom,
                 .trail = "res://effects/venom_trail.tscn",
                 .impact = "res://effects/venom_impact.tscn",
                 .first = "res://effects/venom_cloud.tscn",
                 .second = ""},
    SpecialArrow{.skill = gameplay::SkillId::GaleArrow,
                 .special = E5Arrow::Special::Gale,
                 .trail = "res://effects/gale_trail.tscn",
                 .impact = "res://effects/gale_impact.tscn",
                 .first = "",
                 .second = ""},
    SpecialArrow{.skill = gameplay::SkillId::StormArrow,
                 .special = E5Arrow::Special::Storm,
                 .trail = "res://effects/storm_trail.tscn",
                 .impact = "res://effects/lightning_strike.tscn",
                 .first = "res://effects/lightning_strike.tscn",
                 .second = ""},
    SpecialArrow{.skill = gameplay::SkillId::BrambleArrow,
                 .special = E5Arrow::Special::Bramble,
                 .trail = "res://effects/bramble_trail.tscn",
                 .impact = "res://effects/bramble_impact.tscn",
                 .first = "res://effects/bramble_burst.tscn",
                 .second = "res://effects/bramble_cage.tscn"},
};

godot::Ref<godot::PackedScene> effect_scene(const char* path) {
    godot::ResourceLoader* const loader = godot::ResourceLoader::get_singleton();
    if (path == nullptr || *path == '\0' || !loader->exists(path)) {
        return {};
    }
    return loader->load(path);
}
constexpr float kick_strike_fraction = 0.42F; // of the clip: the moment the leg is stretched out
constexpr float kick_reach = 1.2F;            // metres in front of her where the shockwave is centred
constexpr float kick_radius = 2.2F;           // metres
constexpr float kick_height = 0.9F;
constexpr int kick_bolt_count = 5;
constexpr float kick_bolt_spread = 1.9F; // radians across the fan
constexpr float kick_bolt_reach = 3.4F;  // metres from her
// The wizard's spells. Each plays a clip of the magic pack, faster than it was made and
// without its long wind-down; the spell leaves his hands at the moment they are stretched
// furthest forward (measured in the clips, in seconds of the clip).
constexpr SpellTiming bolt_timing{.playback_scale = 1.7F, .strike_at = 1.13F, .end_at = 1.75F};
constexpr SpellTiming fireball_timing{.playback_scale = 1.5F, .strike_at = 1.33F, .end_at = 2.1F};
constexpr SpellTiming nova_timing{.playback_scale = 1.5F, .strike_at = 1.23F, .end_at = 2.2F};
constexpr SpellTiming lightning_timing{.playback_scale = 1.5F, .strike_at = 0.67F, .end_at = 1.45F};
constexpr SpellTiming meteor_timing{.playback_scale = 1.4F, .strike_at = 1.4F, .end_at = 2.05F};
// The warrior's blows, timed like the spells: the blow lands when the sword arm (or the foot) is
// furthest out, measured in the clips.
// The combo's three blows, then the four special ones.
// The clips are played a little slower than they were made: the testers liked the movements
// and wanted them to take a little longer. The step forward slows with them.
constexpr float combo_playback = 0.87F;
constexpr std::array<SpellTiming, 3> combo_timings{
    // Made by tools/godot/warrior_combo.gd, at the speed they are played: these are its
    // "strike" and "end". Each blow ends in the pose the next one starts in.
    SpellTiming{.playback_scale = combo_playback, .strike_at = 0.3F, .end_at = 0.56F}, // combo_1: down from the right
    SpellTiming{.playback_scale = combo_playback, .strike_at = 0.34F, .end_at = 0.6F}, // combo_2: a turn in the air
    SpellTiming{
        .playback_scale = combo_playback, .strike_at = 0.52F, .end_at = 1.05F}}; // combo_3: the leap, the finisher
// The dwarf's: the same three-blow combo with his axe, then his special blows.
// The dwarf's axe combo: blows of his own from the same tool (--hero=dwarf, DWARF_BLOWS), these
// its "strike" and "end". He is carried forward by the share of the tool's distances by
// which his legs are shorter than the warrior's (the tool prints it).
constexpr float axe_playback = 0.92F;
constexpr float axe_advance_share = 0.69F;
constexpr std::array<SpellTiming, 3> axe_combo_timings{
    SpellTiming{.playback_scale = axe_playback, .strike_at = 0.4F, .end_at = 0.72F},   // axe_1: round from the right
    SpellTiming{.playback_scale = axe_playback, .strike_at = 0.3F, .end_at = 0.62F},   // axe_2: ripped up from below
    SpellTiming{.playback_scale = axe_playback, .strike_at = 0.36F, .end_at = 0.95F}}; // axe_3: down into the ground
// The archer's dagger: three quick blows made by the same tool (--hero=ranger, DAGGER_BLOWS).
constexpr std::array<SpellTiming, 3> dagger_combo_timings{
    SpellTiming{.playback_scale = 1.0F, .strike_at = 0.16F, .end_at = 0.34F}, // dagger_1: across from her right
    SpellTiming{.playback_scale = 1.0F, .strike_at = 0.14F, .end_at = 0.32F}, // dagger_2: back from her left
    SpellTiming{.playback_scale = 1.0F, .strike_at = 0.22F, .end_at = 0.5F}}; // dagger_3: the lunge
// Every blow carries her a step in, the lunge the furthest (as "step" of each blow in the tool:
// distance, from, to).
struct DaggerStep {
    float metres;
    float from;
    float to;
};
constexpr std::array<DaggerStep, 3> dagger_steps{DaggerStep{.metres = 0.35F, .from = 0.05F, .to = 0.2F},
                                                 DaggerStep{.metres = 0.3F, .from = 0.03F, .to = 0.17F},
                                                 DaggerStep{.metres = 0.7F, .from = 0.08F, .to = 0.24F}};
constexpr SpellTiming whirlwind_timing{.playback_scale = 1.5F, .strike_at = 1.03F, .end_at = 1.9F};
constexpr SpellTiming earthbreaker_timing{.playback_scale = 1.3F, .strike_at = 0.83F, .end_at = 1.6F};
constexpr SpellTiming leap_timing{.playback_scale = 1.5F, .strike_at = 1.7F, .end_at = 2.6F};
constexpr SpellTiming battlecry_timing{.playback_scale = 1.3F, .strike_at = 0.75F, .end_at = 2.0F};
constexpr float combo_window = 0.7F; // seconds after a blow in which the next press continues the combo
// Her four special blows are made by tools/godot/warrior_combo.gd, like the combo: these are
// its "strike" and "end", and they are played a little slower than made, like the combo.
constexpr float special_playback = 0.9F;
constexpr SpellTiming flame_timing{.playback_scale = special_playback, .strike_at = 0.5F, .end_at = 0.94F};
constexpr SpellTiming frost_timing{.playback_scale = special_playback, .strike_at = 0.46F, .end_at = 0.88F};
constexpr SpellTiming thunder_timing{.playback_scale = special_playback, .strike_at = 0.72F, .end_at = 1.25F};
constexpr SpellTiming star_timing{.playback_scale = special_playback, .strike_at = 0.62F, .end_at = 1.2F};
struct MeleeBlow {
    float reach = 0.0F;  // metres in front of her where the blow is centred
    float radius = 0.0F; // metres around that
    float shake = 0.0F;  // metres the camera shakes when it lands
};
constexpr float shake_fade = 7.0F; // 1/s
constexpr int starting_potions = 3;
constexpr const char* interface_scene = "res://ui/game_interface.tscn";
constexpr float hurt_shake = 0.035F;     // metres: the camera's jolt when she is hit
constexpr float fall_over_rate = 3.2F;   // rad/s
constexpr float fall_over_angle = 1.45F; // rad: flat on her back
constexpr MeleeBlow plain_blow{.reach = 1.3F, .radius = 1.7F};
// The sword combo: the arc each blow leaves in the air, and how hard it jolts the camera.
constexpr const char* slash_arc_path = "res://effects/slash_arc.tscn";
constexpr const char* landing_dust_path = "res://effects/axe_wind_impact.tscn";
// The shield block: the clip that makes a hero able to block, the sparks of a stopped hit,
// and the jolt it gives the camera.
constexpr const char* block_clip = "shield_block";
constexpr const char* block_spark_path = "res://effects/shield_block.tscn";
constexpr float block_shake = 0.03F;
// The Counter Attack (the numbers are in e5/gameplay/skills.hpp): what is seen and heard of it.
constexpr const char* counter_stance_path = "res://effects/counter_stance.tscn";
constexpr const char* counter_strike_path = "res://effects/counter_strike.tscn";
constexpr const char* counter_bleed_path = "res://effects/counter_bleed.tscn";
// The new warrior's Whirlwind (numbers in e5/gameplay/skills.hpp): the wind round her.
constexpr const char* whirl_storm_path = "res://effects/whirlwind_storm.tscn";
constexpr float whirl_shake = 0.008F;       // metres, with every tick
constexpr float whirl_pull_keep_off = 1.1F; // metres: nearer than this nothing is drawn further in
// The new warrior's Jump Attack (numbers in e5/gameplay/skills.hpp).
constexpr const char* leap_impact_path = "res://effects/jump_attack_impact.tscn";
constexpr const char* leap_marker_path = "res://effects/leap_marker.tscn"; // where she would land, while she aims
constexpr float leap_shake = 0.09F;           // metres, as she lands
constexpr float leap_strike_lead = 0.5F;      // seconds before she lands that the blow's clip begins
constexpr float leap_min_air_seconds = 0.12F; // not "landed" while she is still leaving the ground
constexpr float leap_longest_fall = 2.5F;     // seconds: a leap off a cliff ends at the latest then
constexpr float leap_remote_seconds = 0.85F;  // another player's leap is taken to land after this
// The new warrior's Seismic Slash (numbers in e5/gameplay/skills.hpp): the ground breaks open in
// rows, each further out, wider and larger than the one before, one after another.
constexpr const char* seismic_burst_path = "res://effects/seismic_burst.tscn";
constexpr const char* seismic_crack_path = "res://effects/seismic_crack.tscn"; // its sound, once
constexpr const char* seismic_stun_path = "res://effects/seismic_stun.tscn";   // over a stunned enemy
constexpr int seismic_rows = 7; // (five at first, of one to five bursts: too thin to fill the wedge, bug report 33)
constexpr float seismic_row_seconds = 0.07F; // from one row to the next
constexpr float seismic_first_row = 2.0F;    // metres ahead of her
constexpr float seismic_first_size = 0.7F;   // of the burst as made; the last row is twice that
constexpr float seismic_shake = 0.1F;
// The new warrior's Enrage (numbers in e5/gameplay/skills.hpp): the cry and the red round her.
constexpr const char* enrage_path = "res://effects/enrage.tscn";
// The archer's Vine Tower (numbers in e5/gameplay/skills.hpp): what shows round her while she
// charges, and the tower itself (effects/vine_tower.gd grows it, carries her and takes it away).
constexpr const char* tower_charge_path = "res://effects/vine_tower_charge.tscn";
constexpr const char* tower_path = "res://effects/vine_tower.tscn";
// The new warrior's Sprintsz: its pounding, heard for as long as it lasts; the stone thrown up round
// her with every step of it is the Seismic Slash's burst, small.
constexpr const char* stampede_path = "res://effects/stampede.tscn";
constexpr int stampede_bursts = 3;           // with every pounding
constexpr float stampede_burst_size = 0.4F;  // of the burst as made
constexpr float stampede_shake = 0.012F;     // metres
// The new warrior's Cut in Pieces (numbers in e5/gameplay/skills.hpp).
constexpr float pieces_stand_off = 1.0F;   // metres from an enemy's body to where she appears beside it
constexpr float pieces_blow_pace = 2.2F;   // the combo's clips, so much faster than made
constexpr float pieces_shake = 0.02F;      // metres, with every blow
// The new warrior's Never Give Up: the cry and the blue round her.
constexpr const char* resolve_path = "res://effects/never_give_up.tscn";
constexpr float enrage_strike_gap = 0.5F;  // metres beyond her blow's middle at which she starts to strike
constexpr float enrage_blow_fade = 0.06F;  // seconds from one blow's clip to the next: they are short
constexpr float enrage_shake_share = 0.5F; // of the combo's jolts: there are three times as many
constexpr float counter_shake = 0.02F;         // metres: the jolt of an answered blow
constexpr float counter_swing_seconds = 0.45F; // her arm strikes the first blow of the combo
// A blow comes "from" where the one who struck stands: the enemy this near to that place is it.
constexpr float counter_attacker_slack = 1.5F; // metres
struct ComboLook {
    // The plane the arc is drawn in, as two directions in her own terms (right, up, forward):
    // the arc runs from a little behind `y` round through `x` and on. None of the planes is
    // the blade's true one: seen from the camera behind her that would be a line. Each is
    // turned far enough towards the camera to be seen as an arc.
    godot::Vector3 x;
    godot::Vector3 y;
    float before;      // seconds before the blow lands that the arc starts
    float shake;       // metres
    float arc_seconds; // how long the arc takes
};
const std::array<ComboLook, 3> combo_looks{
    // From above her right shoulder, across the front, down to her left.
    ComboLook{
        .x = {-0.45F, -0.1F, 0.75F}, .y = {0.62F, 0.78F, 0.0F}, .before = 0.07F, .shake = 0.012F, .arc_seconds = 0.24F},
    // Level, from her left round the front to her right; the far side dips towards the camera.
    ComboLook{
        .x = {0.0F, -0.34F, 0.94F}, .y = {-1.0F, 0.0F, 0.0F}, .before = 0.2F, .shake = 0.02F, .arc_seconds = 0.34F},
    // Straight down in front of her, turned a little aside so that it is not seen edge on.
    ComboLook{
        .x = {0.5F, 0.0F, 0.85F}, .y = {0.0F, 1.0F, 0.0F}, .before = 0.08F, .shake = 0.085F, .arc_seconds = 0.26F}};
// The same for the dwarf's axe.
const std::array<ComboLook, 3> axe_looks{
    // Level, from his right round the front to his left.
    ComboLook{.x = {0.0F, -0.34F, 0.94F}, .y = {1.0F, 0.0F, 0.0F}, .before = 0.1F, .shake = 0.03F, .arc_seconds = 0.3F},
    // From low at his left up through the front and over his head.
    ComboLook{.x = {0.3F, 0.15F, 0.9F}, .y = {-0.6F, -0.8F, 0.0F}, .before = 0.1F, .shake = 0.02F, .arc_seconds = 0.3F},
    // Straight down in front of him.
    ComboLook{.x = {0.5F, 0.0F, 0.85F}, .y = {0.0F, 1.0F, 0.0F}, .before = 0.08F, .shake = 0.1F, .arc_seconds = 0.26F}};
// The dagger draws no ready-made arc: its streak follows the blade itself
// (game/characters/blade_trail.gd). Only the shake of each blow is its own.
const std::array<ComboLook, 3> dagger_looks{
    ComboLook{.x = {0.0F, 0.0F, 1.0F}, .y = {1.0F, 0.0F, 0.0F}, .before = 0.0F, .shake = 0.006F, .arc_seconds = 0.0F},
    ComboLook{.x = {0.0F, 0.0F, 1.0F}, .y = {1.0F, 0.0F, 0.0F}, .before = 0.0F, .shake = 0.008F, .arc_seconds = 0.0F},
    ComboLook{.x = {0.0F, 0.0F, 1.0F}, .y = {1.0F, 0.0F, 0.0F}, .before = 0.0F, .shake = 0.03F, .arc_seconds = 0.0F}};
const ComboLook& look_of(gameplay::SkillId skill, int step) {
    const auto index = static_cast<std::size_t>(std::clamp(step, 0, 2));
    if (skill == gameplay::SkillId::DaggerCombo) {
        return dagger_looks.at(index);
    }
    return skill == gameplay::SkillId::AxeCombo ? axe_looks.at(index) : combo_looks.at(index);
}
constexpr MeleeBlow dagger_blow{.reach = 1.2F, .radius = 1.35F}; // short: what is on top of her
constexpr MeleeBlow flame_blow{.reach = 1.4F, .radius = 1.9F};
constexpr MeleeBlow frost_blow{.reach = 1.3F, .radius = 2.3F};
constexpr MeleeBlow thunder_blow{.reach = 1.5F, .radius = 2.6F};
constexpr MeleeBlow star_blow{.reach = 0.6F, .radius = 2.6F};                      // a whirl: around her
constexpr MeleeBlow whirlwind_blow{.reach = 0.0F, .radius = 2.8F, .shake = 0.02F}; // around him
constexpr MeleeBlow earthbreaker_blow{.reach = 1.4F, .radius = 2.5F, .shake = 0.1F};
constexpr MeleeBlow leap_blow{.reach = 1.6F, .radius = 2.0F, .shake = 0.08F};
constexpr MeleeBlow battlecry_blow{.reach = 0.0F, .radius = 5.0F, .shake = 0.05F};
constexpr float cry_push_speed = 8.0F; // m/s away from him
constexpr float cry_push_lift = 4.5F;  // m/s upwards
constexpr float melee_height = 0.9F;
constexpr float blade_length = 0.85F; // metres from the grip to the tip, for effects on the sword
constexpr std::array<const char*, 5> cast_glow_names{"fire", "frost", "lightning", "star", "void"};

constexpr SpellTiming black_hole_timing{.playback_scale = 1.5F, .strike_at = 1.5F, .end_at = 2.3F};
// Black hole: opens where the crosshair points, a little above the ground.
constexpr float black_hole_range = 30.0F; // metres; further away it opens at this distance
constexpr float black_hole_height = 1.6F; // metres above the place aimed at
constexpr float black_hole_tick = 3.0F;   // damage per second to everything inside
constexpr SpellTiming barrage_timing{.playback_scale = 1.6F, .strike_at = 1.53F, .end_at = 2.3F};
// Chain lightning: strikes what the crosshair covers, then jumps on to enemies nearby.
constexpr float lightning_strike_radius = 1.2F; // metres around the first strike
constexpr float lightning_jump_reach = 9.0F;    // metres from one victim to the next
constexpr int lightning_jumps = 4;
constexpr float lightning_jump_share = 0.7F; // of the damage, for every victim after the first
// Meteor: falls from high behind him onto the place the crosshair covers.
constexpr float meteor_range = 45.0F; // metres; no meteor beyond
constexpr float meteor_height = 24.0F;
constexpr float meteor_setback = 9.0F; // metres towards him from the target, where it starts
constexpr float meteor_speed = 34.0F;
constexpr float meteor_blast_radius = 5.5F;
constexpr float meteor_trail_scale = 3.4F;
// Star barrage: stars rise from above his head in a fan and curve to the target.
constexpr int star_count = 6;
constexpr float star_rise_speed = 11.0F;
constexpr float star_speed = 30.0F;
constexpr float star_turn = 60.0F;
constexpr float star_interval = 0.07F;        // seconds between two stars
constexpr float bolt_speed = 45.0F;           // m/s
constexpr float fireball_speed = 24.0F;       // m/s
constexpr float fireball_blast_radius = 3.5F; // metres
constexpr float fireball_trail_scale = 1.8F;
constexpr float nova_radius = 5.5F; // metres around him
constexpr float nova_height = 0.9F;
constexpr float summon_release_seconds = 1.6F; // into the clip: the bird on her hand takes off
// The summon's clip is played this much slower, so that the bird sits on her hand long enough to
// be looked at: 2.3 s from the start to the release instead of 1.6.
constexpr float summon_playback_scale = 0.7F;
constexpr float perch_appears_at = 0.3F;   // seconds into the clip: her hand is up
constexpr float perch_spreads_from = 0.7F; // the bird opens its wings between these two moments
constexpr float perch_spreads_until = 1.35F;
constexpr float perch_height = 0.26F;           // metres from her palm to the middle of the bird
constexpr float perch_lean_back = 0.5236F;      // radians; undoes the forward tilt of the flying model
constexpr float flock_interval_seconds = 0.16F; // between the birds that follow the first
constexpr int summoned_bird_count = 4;
constexpr float summoned_bird_launch_speed = 9.0F; // m/s; the bird keeps its own speed afterwards
constexpr float palm_from_wrist = 0.1F;            // metres           // metres above her feet

// The timing of a spell or blow; nullptr for the skills that are timed otherwise.
const SpellTiming* timing_of(gameplay::SkillId skill) {
    switch (skill) {
    case gameplay::SkillId::ArcaneBolt:
        return &bolt_timing;
    case gameplay::SkillId::Fireball:
        return &fireball_timing;
    case gameplay::SkillId::FrostNova:
        return &nova_timing;
    case gameplay::SkillId::ChainLightning:
        return &lightning_timing;
    case gameplay::SkillId::Meteor:
        return &meteor_timing;
    case gameplay::SkillId::StarBarrage:
        return &barrage_timing;
    case gameplay::SkillId::BlackHole:
        return &black_hole_timing;
    case gameplay::SkillId::FlameBlade:
        return &flame_timing;
    case gameplay::SkillId::FrostEdge:
        return &frost_timing;
    case gameplay::SkillId::ThunderCleave:
        return &thunder_timing;
    case gameplay::SkillId::StarWhirl:
        return &star_timing;
    case gameplay::SkillId::SeismicSlash:
        return &thunder_timing; // the same clip: the blow that comes down into the ground
    case gameplay::SkillId::Whirlwind:
        return &whirlwind_timing;
    case gameplay::SkillId::Earthbreaker:
        return &earthbreaker_timing;
    case gameplay::SkillId::LeapStrike:
        return &leap_timing;
    case gameplay::SkillId::Battlecry:
        return &battlecry_timing;
    default:
        return nullptr;
    }
}

const MeleeBlow& blow_of(gameplay::SkillId skill) {
    switch (skill) {
    case gameplay::SkillId::FlameBlade:
        return flame_blow;
    case gameplay::SkillId::FrostEdge:
        return frost_blow;
    case gameplay::SkillId::ThunderCleave:
        return thunder_blow;
    case gameplay::SkillId::StarWhirl:
        return star_blow;
    case gameplay::SkillId::Whirlwind:
        return whirlwind_blow;
    case gameplay::SkillId::Earthbreaker:
        return earthbreaker_blow;
    case gameplay::SkillId::LeapStrike:
        return leap_blow;
    case gameplay::SkillId::Battlecry:
        return battlecry_blow;
    case gameplay::SkillId::DaggerCombo:
        return dagger_blow;
    default:
        return plain_blow;
    }
}

bool is_combo(gameplay::SkillId skill) {
    return skill == gameplay::SkillId::Slash || skill == gameplay::SkillId::AxeCombo ||
           skill == gameplay::SkillId::DaggerCombo;
}

bool is_melee(gameplay::SkillId skill) {
    switch (skill) {
    case gameplay::SkillId::Slash:
    case gameplay::SkillId::FlameBlade:
    case gameplay::SkillId::FrostEdge:
    case gameplay::SkillId::ThunderCleave:
    case gameplay::SkillId::StarWhirl:
    case gameplay::SkillId::AxeCombo:
    case gameplay::SkillId::DaggerCombo:
    case gameplay::SkillId::Whirlwind:
    case gameplay::SkillId::Earthbreaker:
    case gameplay::SkillId::LeapStrike:
    case gameplay::SkillId::Battlecry:
    case gameplay::SkillId::SeismicSlash:
        return true;
    default:
        return false;
    }
}

gameplay::SkillSet skill_set_from(int value) {
    if (value == 5) {
        return gameplay::SkillSet::ArcherFull;
    }
    if (value == 4) {
        return gameplay::SkillSet::Blade;
    }
    if (value == 3) {
        return gameplay::SkillSet::Dwarf;
    }
    if (value == 2) {
        return gameplay::SkillSet::Warrior;
    }
    return value == 1 ? gameplay::SkillSet::Wizard : gameplay::SkillSet::Archer;
}

} // namespace

void E5PlayerController::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;

    ClassDB::bind_method(D_METHOD("set_walk_speed", "speed"), &E5PlayerController::set_walk_speed);
    ClassDB::bind_method(D_METHOD("get_walk_speed"), &E5PlayerController::get_walk_speed);
    ClassDB::bind_method(D_METHOD("set_sprint_speed", "speed"), &E5PlayerController::set_sprint_speed);
    ClassDB::bind_method(D_METHOD("get_sprint_speed"), &E5PlayerController::get_sprint_speed);
    ClassDB::bind_method(D_METHOD("set_jump_velocity", "velocity"), &E5PlayerController::set_jump_velocity);
    ClassDB::bind_method(D_METHOD("get_jump_velocity"), &E5PlayerController::get_jump_velocity);
    ClassDB::bind_method(D_METHOD("set_fall_acceleration", "acceleration"), &E5PlayerController::set_fall_acceleration);
    ClassDB::bind_method(D_METHOD("get_fall_acceleration"), &E5PlayerController::get_fall_acceleration);
    ClassDB::bind_method(D_METHOD("set_turn_speed", "radians_per_second"), &E5PlayerController::set_turn_speed);
    ClassDB::bind_method(D_METHOD("get_turn_speed"), &E5PlayerController::get_turn_speed);
    ClassDB::bind_method(D_METHOD("set_mouse_sensitivity", "radians_per_pixel"),
                         &E5PlayerController::set_mouse_sensitivity);
    ClassDB::bind_method(D_METHOD("get_mouse_sensitivity"), &E5PlayerController::get_mouse_sensitivity);
    ClassDB::bind_method(D_METHOD("set_aim_move_speed", "speed"), &E5PlayerController::set_aim_move_speed);
    ClassDB::bind_method(D_METHOD("get_aim_move_speed"), &E5PlayerController::get_aim_move_speed);
    ClassDB::bind_method(D_METHOD("set_arrow_speed", "speed"), &E5PlayerController::set_arrow_speed);
    ClassDB::bind_method(D_METHOD("get_arrow_speed"), &E5PlayerController::get_arrow_speed);
    ClassDB::bind_method(D_METHOD("set_capture_mouse_on_ready", "capture"),
                         &E5PlayerController::set_capture_mouse_on_ready);
    ClassDB::bind_method(D_METHOD("get_capture_mouse_on_ready"), &E5PlayerController::get_capture_mouse_on_ready);
    ClassDB::bind_method(D_METHOD("set_animation_library", "library"), &E5PlayerController::set_animation_library);
    ClassDB::bind_method(D_METHOD("get_animation_library"), &E5PlayerController::get_animation_library);
    ClassDB::bind_method(D_METHOD("get_current_animation"), &E5PlayerController::get_current_animation);
    ClassDB::bind_method(D_METHOD("prewarm_effects"), &E5PlayerController::prewarm_effects);
    ClassDB::bind_method(D_METHOD("set_charge_effect", "scene"), &E5PlayerController::set_charge_effect);
    ClassDB::bind_method(D_METHOD("get_charge_effect"), &E5PlayerController::get_charge_effect);
    ClassDB::bind_method(D_METHOD("set_charge_full_effect", "scene"), &E5PlayerController::set_charge_full_effect);
    ClassDB::bind_method(D_METHOD("get_charge_full_effect"), &E5PlayerController::get_charge_full_effect);
    ClassDB::bind_method(D_METHOD("set_rain_marker_effect", "scene"), &E5PlayerController::set_rain_marker_effect);
    ClassDB::bind_method(D_METHOD("get_rain_marker_effect"), &E5PlayerController::get_rain_marker_effect);
    ClassDB::bind_method(D_METHOD("set_rain_impact_effect", "scene"), &E5PlayerController::set_rain_impact_effect);
    ClassDB::bind_method(D_METHOD("get_rain_impact_effect"), &E5PlayerController::get_rain_impact_effect);
    ClassDB::bind_method(D_METHOD("set_frost_trail_effect", "scene"), &E5PlayerController::set_frost_trail_effect);
    ClassDB::bind_method(D_METHOD("get_frost_trail_effect"), &E5PlayerController::get_frost_trail_effect);
    ClassDB::bind_method(D_METHOD("set_frost_impact_effect", "scene"), &E5PlayerController::set_frost_impact_effect);
    ClassDB::bind_method(D_METHOD("get_frost_impact_effect"), &E5PlayerController::get_frost_impact_effect);
    ClassDB::bind_method(D_METHOD("set_fire_trail_effect", "scene"), &E5PlayerController::set_fire_trail_effect);
    ClassDB::bind_method(D_METHOD("get_fire_trail_effect"), &E5PlayerController::get_fire_trail_effect);
    ClassDB::bind_method(D_METHOD("set_fire_impact_effect", "scene"), &E5PlayerController::set_fire_impact_effect);
    ClassDB::bind_method(D_METHOD("get_fire_impact_effect"), &E5PlayerController::get_fire_impact_effect);
    ClassDB::bind_method(D_METHOD("set_kick_effect", "scene"), &E5PlayerController::set_kick_effect);
    ClassDB::bind_method(D_METHOD("get_kick_effect"), &E5PlayerController::get_kick_effect);
    ClassDB::bind_method(D_METHOD("set_bird_scene", "scene"), &E5PlayerController::set_bird_scene);
    ClassDB::bind_method(D_METHOD("get_bird_scene"), &E5PlayerController::get_bird_scene);
    ClassDB::bind_method(D_METHOD("set_summon_cast_effect", "scene"), &E5PlayerController::set_summon_cast_effect);
    ClassDB::bind_method(D_METHOD("get_summon_cast_effect"), &E5PlayerController::get_summon_cast_effect);
    ClassDB::bind_method(D_METHOD("set_summon_burst_effect", "scene"), &E5PlayerController::set_summon_burst_effect);
    ClassDB::bind_method(D_METHOD("get_summon_burst_effect"), &E5PlayerController::get_summon_burst_effect);
    ClassDB::bind_method(D_METHOD("set_skill_set", "set"), &E5PlayerController::set_skill_set);
    ClassDB::bind_method(D_METHOD("get_skill_set"), &E5PlayerController::get_skill_set);
    ClassDB::bind_method(D_METHOD("set_cast_effect", "scene"), &E5PlayerController::set_cast_effect);
    ClassDB::bind_method(D_METHOD("get_cast_effect"), &E5PlayerController::get_cast_effect);
    ClassDB::bind_method(D_METHOD("set_bolt_trail_effect", "scene"), &E5PlayerController::set_bolt_trail_effect);
    ClassDB::bind_method(D_METHOD("get_bolt_trail_effect"), &E5PlayerController::get_bolt_trail_effect);
    ClassDB::bind_method(D_METHOD("set_bolt_impact_effect", "scene"), &E5PlayerController::set_bolt_impact_effect);
    ClassDB::bind_method(D_METHOD("get_bolt_impact_effect"), &E5PlayerController::get_bolt_impact_effect);
    ClassDB::bind_method(D_METHOD("set_nova_effect", "scene"), &E5PlayerController::set_nova_effect);
    ClassDB::bind_method(D_METHOD("get_nova_effect"), &E5PlayerController::get_nova_effect);
    ClassDB::bind_method(D_METHOD("set_lightning_effect", "scene"), &E5PlayerController::set_lightning_effect);
    ClassDB::bind_method(D_METHOD("get_lightning_effect"), &E5PlayerController::get_lightning_effect);
    ClassDB::bind_method(D_METHOD("set_meteor_marker_effect", "scene"), &E5PlayerController::set_meteor_marker_effect);
    ClassDB::bind_method(D_METHOD("get_meteor_marker_effect"), &E5PlayerController::get_meteor_marker_effect);
    ClassDB::bind_method(D_METHOD("set_meteor_impact_effect", "scene"), &E5PlayerController::set_meteor_impact_effect);
    ClassDB::bind_method(D_METHOD("get_meteor_impact_effect"), &E5PlayerController::get_meteor_impact_effect);
    ClassDB::bind_method(D_METHOD("set_star_trail_effect", "scene"), &E5PlayerController::set_star_trail_effect);
    ClassDB::bind_method(D_METHOD("get_star_trail_effect"), &E5PlayerController::get_star_trail_effect);
    ClassDB::bind_method(D_METHOD("set_star_impact_effect", "scene"), &E5PlayerController::set_star_impact_effect);
    ClassDB::bind_method(D_METHOD("get_star_impact_effect"), &E5PlayerController::get_star_impact_effect);
    ClassDB::bind_method(D_METHOD("set_black_hole_effect", "scene"), &E5PlayerController::set_black_hole_effect);
    ClassDB::bind_method(D_METHOD("get_black_hole_effect"), &E5PlayerController::get_black_hole_effect);
    ClassDB::bind_method(D_METHOD("set_black_hole_burst_effect", "scene"),
                         &E5PlayerController::set_black_hole_burst_effect);
    ClassDB::bind_method(D_METHOD("get_black_hole_burst_effect"), &E5PlayerController::get_black_hole_burst_effect);
    ClassDB::bind_method(D_METHOD("set_weapon_effect_1", "scene"), &E5PlayerController::set_weapon_effect_1);
    ClassDB::bind_method(D_METHOD("get_weapon_effect_1"), &E5PlayerController::get_weapon_effect_1);
    ClassDB::bind_method(D_METHOD("set_weapon_effect_2", "scene"), &E5PlayerController::set_weapon_effect_2);
    ClassDB::bind_method(D_METHOD("get_weapon_effect_2"), &E5PlayerController::get_weapon_effect_2);
    ClassDB::bind_method(D_METHOD("set_weapon_effect_3", "scene"), &E5PlayerController::set_weapon_effect_3);
    ClassDB::bind_method(D_METHOD("get_weapon_effect_3"), &E5PlayerController::get_weapon_effect_3);
    ClassDB::bind_method(D_METHOD("set_weapon_effect_4", "scene"), &E5PlayerController::set_weapon_effect_4);
    ClassDB::bind_method(D_METHOD("get_weapon_effect_4"), &E5PlayerController::get_weapon_effect_4);
    ClassDB::bind_method(D_METHOD("heal", "amount"), &E5PlayerController::heal);
    ClassDB::bind_method(D_METHOD("get_effective_max_health"), &E5PlayerController::get_effective_max_health);
    ClassDB::bind_method(D_METHOD("get_inventory"), &E5PlayerController::get_inventory);
    ClassDB::bind_method(D_METHOD("set_input_blocked", "blocked"), &E5PlayerController::set_input_blocked);
    ClassDB::bind_method(D_METHOD("is_input_blocked"), &E5PlayerController::is_input_blocked);
    ClassDB::bind_method(D_METHOD("take_damage", "amount"), &E5PlayerController::take_damage);
    ClassDB::bind_method(D_METHOD("take_damage_from", "amount", "from"), &E5PlayerController::take_damage_from);
    ClassDB::bind_method(D_METHOD("can_block"), &E5PlayerController::can_block);
    ClassDB::bind_method(D_METHOD("is_blocking"), &E5PlayerController::is_blocking);
    ClassDB::bind_method(D_METHOD("get_block_cooldown"), &E5PlayerController::get_block_cooldown);
    ClassDB::bind_method(D_METHOD("get_block_cooldown_seconds"), &E5PlayerController::get_block_cooldown_seconds);
    ClassDB::bind_method(D_METHOD("get_block_time_left"), &E5PlayerController::get_block_time_left);
    ClassDB::bind_method(D_METHOD("get_hits_blocked"), &E5PlayerController::get_hits_blocked);
    ClassDB::bind_method(D_METHOD("get_stance_seconds"), &E5PlayerController::get_stance_seconds);
    ClassDB::bind_method(D_METHOD("get_counters_struck"), &E5PlayerController::get_counters_struck);
    ClassDB::bind_method(D_METHOD("get_enrage_seconds"), &E5PlayerController::get_enrage_seconds);
    ClassDB::bind_method(D_METHOD("get_enrage_blows"), &E5PlayerController::get_enrage_blows);
    ClassDB::bind_method(D_METHOD("get_resolve_seconds"), &E5PlayerController::get_resolve_seconds);
    ClassDB::bind_method(D_METHOD("get_towers_grown"), &E5PlayerController::get_towers_grown);
    ClassDB::bind_method(D_METHOD("get_pieces_blows"), &E5PlayerController::get_pieces_blows);
    ClassDB::bind_method(D_METHOD("get_stampede_seconds"), &E5PlayerController::get_stampede_seconds);
    ClassDB::bind_method(D_METHOD("get_stampede_ticks"), &E5PlayerController::get_stampede_ticks);
    ClassDB::bind_method(D_METHOD("get_resolve_spared"), &E5PlayerController::get_resolve_spared);
    ClassDB::bind_method(D_METHOD("get_resolve_healed"), &E5PlayerController::get_resolve_healed);
    ClassDB::bind_method(D_METHOD("get_health"), &E5PlayerController::get_health);
    ClassDB::bind_method(D_METHOD("is_dead"), &E5PlayerController::is_dead);
    ClassDB::bind_method(D_METHOD("set_max_health", "health"), &E5PlayerController::set_max_health);
    ClassDB::bind_method(D_METHOD("get_max_health"), &E5PlayerController::get_max_health);
    ClassDB::bind_method(D_METHOD("select_skill", "slot"), &E5PlayerController::select_skill);
    ClassDB::bind_method(D_METHOD("get_selected_skill"), &E5PlayerController::get_selected_skill);
    ClassDB::bind_method(D_METHOD("get_charge"), &E5PlayerController::get_charge);
    ClassDB::bind_method(D_METHOD("get_bow_phase"), &E5PlayerController::get_bow_phase);
    ClassDB::bind_method(D_METHOD("get_cast_progress"), &E5PlayerController::get_cast_progress);
    ClassDB::bind_method(D_METHOD("get_cast_seconds"), &E5PlayerController::get_cast_seconds);
    ClassDB::bind_method(D_METHOD("get_cast_power"), &E5PlayerController::get_cast_power);
    ClassDB::bind_method(D_METHOD("get_skill_cooldown", "slot"), &E5PlayerController::get_skill_cooldown);
    ClassDB::bind_method(D_METHOD("get_skill_cooldown_seconds", "slot"),
                         &E5PlayerController::get_skill_cooldown_seconds);
    ClassDB::bind_method(D_METHOD("skill_takes_charge", "slot"), &E5PlayerController::skill_takes_charge);
    ClassDB::bind_method(D_METHOD("is_skill_ready", "slot"), &E5PlayerController::is_skill_ready);
    ClassDB::bind_method(D_METHOD("set_remote", "remote"), &E5PlayerController::set_remote);
    ClassDB::bind_method(D_METHOD("is_remote"), &E5PlayerController::is_remote);
    ClassDB::bind_method(D_METHOD("get_net_state"), &E5PlayerController::get_net_state);
    ClassDB::bind_method(D_METHOD("apply_net_state", "position", "facing", "clip", "speed", "look_yaw", "look_pitch"),
                         &E5PlayerController::apply_net_state);
    ClassDB::bind_method(D_METHOD("take_net_events"), &E5PlayerController::take_net_events);
    ClassDB::bind_method(D_METHOD("apply_net_event", "kind", "slot", "power", "combo_step", "look_yaw", "look_pitch"),
                         &E5PlayerController::apply_net_event);
    ClassDB::bind_method(D_METHOD("take_outgoing_damage"), &E5PlayerController::take_outgoing_damage);
    ClassDB::bind_method(D_METHOD("get_skill_name", "slot"), &E5PlayerController::get_skill_name);
    ClassDB::bind_method(D_METHOD("get_open_slot_count"), &E5PlayerController::get_open_slot_count);
    ClassDB::bind_method(D_METHOD("get_last_skill"), &E5PlayerController::get_last_skill);
    ClassDB::bind_method(D_METHOD("get_skills_used"), &E5PlayerController::get_skills_used);
    ClassDB::bind_method(D_METHOD("is_standard_attack_in_use"), &E5PlayerController::is_standard_attack_in_use);
    ClassDB::bind_method(D_METHOD("set_skill_bar_visible", "visible"), &E5PlayerController::set_skill_bar_visible);
    ClassDB::bind_method(D_METHOD("set_trail_effect", "scene"), &E5PlayerController::set_trail_effect);
    ClassDB::bind_method(D_METHOD("get_trail_effect"), &E5PlayerController::get_trail_effect);
    ClassDB::bind_method(D_METHOD("set_impact_effect", "scene"), &E5PlayerController::set_impact_effect);
    ClassDB::bind_method(D_METHOD("get_impact_effect"), &E5PlayerController::get_impact_effect);

    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "walk_speed", godot::PROPERTY_HINT_RANGE, "0,20,0.1,suffix:m/s"),
                 "set_walk_speed", "get_walk_speed");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "sprint_speed", godot::PROPERTY_HINT_RANGE, "0,30,0.1,suffix:m/s"),
                 "set_sprint_speed", "get_sprint_speed");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "jump_velocity", godot::PROPERTY_HINT_RANGE, "0,20,0.1,suffix:m/s"),
        "set_jump_velocity", "get_jump_velocity");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "fall_acceleration", godot::PROPERTY_HINT_RANGE, "0,50,0.1,suffix:m/s²"),
        "set_fall_acceleration", "get_fall_acceleration");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "turn_speed", godot::PROPERTY_HINT_RANGE, "0.5,40,0.1,suffix:rad/s"),
        "set_turn_speed", "get_turn_speed");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "mouse_sensitivity", godot::PROPERTY_HINT_RANGE, "0.0001,0.02,0.0001"),
        "set_mouse_sensitivity", "get_mouse_sensitivity");
    ClassDB::bind_method(D_METHOD("set_dodge_distance", "metres"), &E5PlayerController::set_dodge_distance);
    ClassDB::bind_method(D_METHOD("get_dodge_distance"), &E5PlayerController::get_dodge_distance);
    ClassDB::bind_method(D_METHOD("is_dodging"), &E5PlayerController::is_dodging);
    ClassDB::bind_method(D_METHOD("is_emoting"), &E5PlayerController::is_emoting);
    ClassDB::bind_method(D_METHOD("is_mounted"), &E5PlayerController::is_mounted);
    ClassDB::bind_method(D_METHOD("set_mounted", "mounted"), &E5PlayerController::set_mounted);
    ClassDB::bind_method(D_METHOD("try_set_mounted", "mounted"), &E5PlayerController::try_set_mounted);
    ClassDB::bind_method(D_METHOD("set_mount_speed", "speed"), &E5PlayerController::set_mount_speed);
    ClassDB::bind_method(D_METHOD("get_mount_speed"), &E5PlayerController::get_mount_speed);
    ClassDB::bind_method(D_METHOD("set_mount_sprint_speed", "speed"), &E5PlayerController::set_mount_sprint_speed);
    ClassDB::bind_method(D_METHOD("get_mount_sprint_speed"), &E5PlayerController::get_mount_sprint_speed);
    ClassDB::bind_method(D_METHOD("set_mount_walk_speed", "speed"), &E5PlayerController::set_mount_walk_speed);
    ClassDB::bind_method(D_METHOD("get_mount_walk_speed"), &E5PlayerController::get_mount_walk_speed);
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "mount_speed", godot::PROPERTY_HINT_RANGE, "0,30,0.1,suffix:m/s"),
                 "set_mount_speed", "get_mount_speed");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "mount_sprint_speed", godot::PROPERTY_HINT_RANGE, "0,30,0.1,suffix:m/s"),
        "set_mount_sprint_speed", "get_mount_sprint_speed");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "mount_walk_speed", godot::PROPERTY_HINT_RANGE, "0,30,0.1,suffix:m/s"),
        "set_mount_walk_speed", "get_mount_walk_speed");
    ClassDB::bind_method(D_METHOD("get_action_skill_name"), &E5PlayerController::get_action_skill_name);
    ClassDB::bind_method(D_METHOD("set_charge_always_full", "full"), &E5PlayerController::set_charge_always_full);
    ClassDB::bind_method(D_METHOD("get_charge_always_full"), &E5PlayerController::get_charge_always_full);
    ADD_PROPERTY(PropertyInfo(godot::Variant::BOOL, "charge_always_full"), "set_charge_always_full",
                 "get_charge_always_full");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "dodge_distance", godot::PROPERTY_HINT_RANGE, "0,10,0.1,suffix:m"),
                 "set_dodge_distance", "get_dodge_distance");
    ClassDB::bind_method(D_METHOD("set_quick_cast", "enabled"), &E5PlayerController::set_quick_cast);
    ClassDB::bind_method(D_METHOD("get_quick_cast"), &E5PlayerController::get_quick_cast);
    ADD_PROPERTY(PropertyInfo(godot::Variant::BOOL, "quick_cast"), "set_quick_cast", "get_quick_cast");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "aim_move_speed", godot::PROPERTY_HINT_RANGE, "0,10,0.1,suffix:m/s"),
        "set_aim_move_speed", "get_aim_move_speed");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "arrow_speed", godot::PROPERTY_HINT_RANGE, "1,200,1,suffix:m/s"),
                 "set_arrow_speed", "get_arrow_speed");
    ADD_PROPERTY(PropertyInfo(godot::Variant::BOOL, "capture_mouse_on_ready"), "set_capture_mouse_on_ready",
                 "get_capture_mouse_on_ready");
    ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "animation_library", godot::PROPERTY_HINT_RESOURCE_TYPE,
                              "AnimationLibrary"),
                 "set_animation_library", "get_animation_library");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "max_health", godot::PROPERTY_HINT_RANGE, "1,1000,1"),
                 "set_max_health", "get_max_health");
    ADD_PROPERTY(PropertyInfo(godot::Variant::INT, "skill_set", godot::PROPERTY_HINT_ENUM,
                              "Archer,Wizard,Warrior,Dwarf,Blade,ArcherFull"),
                 "set_skill_set", "get_skill_set");
    for (const char* const effect :
         {"charge_effect",           "charge_full_effect",  "trail_effect",       "impact_effect",
          "rain_marker_effect",      "rain_impact_effect",  "frost_trail_effect", "frost_impact_effect",
          "fire_trail_effect",       "fire_impact_effect",  "kick_effect",        "bird_scene",
          "summon_cast_effect",      "summon_burst_effect", "cast_effect",        "bolt_trail_effect",
          "bolt_impact_effect",      "nova_effect",         "lightning_effect",   "meteor_marker_effect",
          "meteor_impact_effect",    "star_trail_effect",   "star_impact_effect", "black_hole_effect",
          "black_hole_burst_effect", "weapon_effect_1",     "weapon_effect_2",    "weapon_effect_3",
          "weapon_effect_4"}) {
        ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, effect, godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"),
                     godot::String("set_") + effect, godot::String("get_") + effect);
    }
}

void E5PlayerController::_ready() {
    ensure_default_input_actions();
    action_forward_ = godot::StringName(actions::move_forward);
    action_back_ = godot::StringName(actions::move_back);
    action_left_ = godot::StringName(actions::move_left);
    action_right_ = godot::StringName(actions::move_right);
    action_jump_ = godot::StringName(actions::jump);
    action_sprint_ = godot::StringName(actions::sprint);
    clip_idle_ = godot::StringName("idle");
    clip_walk_ = godot::StringName("walk");
    clip_run_ = godot::StringName("run");
    clip_jump_ = godot::StringName("jump");
    action_aim_ = godot::StringName(actions::aim);
    action_attack_ = godot::StringName(actions::attack);
    action_use_potion_ = godot::StringName(actions::use_potion);
    for (int slot = 0; slot < actions::skill_slot_count; ++slot) {
        skill_actions_.at(static_cast<std::size_t>(slot)) =
            godot::StringName(godot::String(actions::skill_prefix) + godot::String::num_int64(slot + 1));
    }
    clip_bow_draw_ = godot::StringName("bow_draw");
    clip_bow_aim_ = godot::StringName("bow_aim");
    clip_bow_recoil_ = godot::StringName("bow_recoil");
    clip_bow_walk_forward_ = godot::StringName("bow_walk_forward");
    clip_bow_walk_back_ = godot::StringName("bow_walk_back");
    clip_bow_walk_left_ = godot::StringName("bow_walk_left");
    clip_bow_walk_right_ = godot::StringName("bow_walk_right");
    clip_kick_ = godot::StringName("thunder_kick");
    clip_summon_ = godot::StringName("summon");
    clip_bolt_ = godot::StringName("attack_1h_1");
    clip_fireball_ = godot::StringName("attack_2h_1");
    clip_nova_ = godot::StringName("area_1");
    clip_lightning_ = godot::StringName("attack_1h_2");
    clip_meteor_ = godot::StringName("cast_2h");
    clip_barrage_ = godot::StringName("attack_2h_2");
    clip_black_hole_ = godot::StringName("area_2");
    clip_combo_ = {godot::StringName("combo_1"), godot::StringName("combo_2"), godot::StringName("combo_3")};
    clip_flame_ = godot::StringName("flame_blade");
    clip_frost_ = godot::StringName("frost_edge");
    clip_thunder_ = godot::StringName("thunder_cleave");
    clip_star_ = godot::StringName("star_whirl");
    clip_axe_combo_ = {godot::StringName("axe_1"), godot::StringName("axe_2"), godot::StringName("axe_3")};
    clip_dagger_combo_ = {godot::StringName("dagger_1"), godot::StringName("dagger_2"), godot::StringName("dagger_3")};
    clip_whirlwind_ = godot::StringName("spin_high");
    clip_earthbreaker_ = godot::StringName("downward");
    clip_leap_ = godot::StringName("leap");
    clip_battlecry_ = godot::StringName("battlecry");
    clip_whirl_ = godot::StringName("whirl");
    clip_kneel_ = godot::StringName("kneel");
    skills_ = gameplay::SkillBar(skill_bar_set());
    // The first slot is on the left mouse button anyway: the right one starts on the second.
    skills_.select(1);

    add_to_group(remote_ ? remote_group_name : group_name);
    if (remote_) {
        // Placed by what arrives over the network: she is pushed by nothing. She keeps her
        // body, so that blows and missiles find her.
        set_collision_mask(0);
    }
    clip_death_ = godot::StringName("death");
    clip_block_ = godot::StringName(block_clip);
    clip_dodge_ = godot::StringName("dodge");
    clip_dodge_alt_ = godot::StringName("dodge_alt");
    clip_emote_ = godot::StringName("emote");
    clip_ride_ = godot::StringName("ride");
    dodge_clip_ = clip_dodge_;
    spawn_transform_ = get_global_transform();
    vitals_ = gameplay::full_vitals(vitals_params_);
    inventory_ = memnew(E5Inventory);
    inventory_->set_name(E5Inventory::node_name);
    add_child(inventory_);
    // Nobody sets out with empty pockets.
    inventory_->give(gameplay::ItemId::HealthPotion, starting_potions);
    if (skill_set_ == static_cast<int>(gameplay::SkillSet::Archer) ||
        skill_set_ == static_cast<int>(gameplay::SkillSet::ArcherFull)) {
        // Her oath-bow in hand; and, while the bows are being tried out, every other in her bag.
        inventory_->arm(gameplay::ItemId::BowWarden);
        for (const gameplay::ItemId bow :
             {gameplay::ItemId::BowHunter, gameplay::ItemId::BowIronbound, gameplay::ItemId::BowLeafwood,
              gameplay::ItemId::BowMoonglass, gameplay::ItemId::BowBriarbloom, gameplay::ItemId::BowStormfeather,
              gameplay::ItemId::BowNightthorn, gameplay::ItemId::BowDragonfire}) {
            inventory_->give(bow, 1);
        }
    } else if (fights_with_sword()) {
        // A soldier's sword in hand; and, while the swords are being tried out, every other in her bag.
        inventory_->arm(gameplay::ItemId::SwordSoldier);
        for (const gameplay::ItemId sword :
             {gameplay::ItemId::SwordKnight, gameplay::ItemId::SwordSapphire, gameplay::ItemId::SwordGilded,
              gameplay::ItemId::SwordDuskfang, gameplay::ItemId::SwordEmberbrand, gameplay::ItemId::SwordDawnbreaker,
              gameplay::ItemId::SwordStarweaver}) {
            inventory_->give(sword, 1);
        }
    }

    const std::string name = godot::String(get_name()).utf8().get_data();
    camera_pivot_ = get_node<godot::Node3D>(godot::NodePath("CameraPivot"));
    if (camera_pivot_ == nullptr && remote_) {
        // A remote hero has no camera of its own.
    } else if (camera_pivot_ == nullptr) {
        logger().error("E5PlayerController '{}' needs a Node3D child named 'CameraPivot'; camera control is disabled",
                       name);
    } else {
        // The camera boom must not treat the player's own capsule as an obstacle.
        const godot::TypedArray<godot::Node> arms = camera_pivot_->find_children("*", "SpringArm3D", true, false);
        for (const godot::Variant& node : arms) {
            if (auto* const arm = godot::Object::cast_to<godot::SpringArm3D>(node)) {
                arm->add_excluded_object(get_rid());
                if (camera_arm_ == nullptr) {
                    camera_arm_ = arm;
                    camera_rest_distance_ = arm->get_length();
                }
            }
        }
    }
    model_ = get_node<godot::Node3D>(godot::NodePath("Model"));
    if (model_ == nullptr) {
        logger().warn("E5PlayerController '{}' has no Node3D child named 'Model'; nothing will be shown or animated",
                      name);
    }

    // Start with the camera behind the character: both look along -Z.
    look_.yaw = static_cast<float>(get_rotation().y);
    model_yaw_ = gameplay::facing_yaw(-std::sin(look_.yaw), -std::cos(look_.yaw));
    apply_look_to_nodes();
    if (model_ != nullptr) {
        model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
    }

    setup_animation();

    if (capture_mouse_on_ready_ && !remote_) {
        godot::Input::get_singleton()->set_mouse_mode(godot::Input::MOUSE_MODE_CAPTURED);
    }
}

void E5PlayerController::setup_animation() {
    if (animation_library_.is_null() || model_ == nullptr) {
        return;
    }
    for (const godot::StringName& clip : {clip_idle_, clip_walk_, clip_run_, clip_jump_}) {
        if (!animation_library_->has_animation(clip)) {
            logger().error("animation library is missing the '{}' clip; animation is disabled",
                           godot::String(clip).utf8().get_data());
            return;
        }
    }
    if (!animator_.setup(this, model_, animation_library_)) {
        return;
    }
    animator_.set_base(clip_idle_, 1.0F);
    // Whoever has the clip `dodge` and no shield clip can dodge, on the shield's key.
    dodge_enabled_ = animator_.has_clip(clip_dodge_) && !animator_.has_clip(clip_block_);
    if (dodge_enabled_) {
        // It ends a little before its clip does: she is handed back to running while the clip
        // still plays out under the cross-fade. Ended with the clip, she stood for a moment.
        dodge_seconds_ = std::max(animator_.clip_length(clip_dodge_) - dodge_hand_over_seconds, 0.2F);
    }
    // Prefer a held full-draw pose over the pack's aim clip: that clip keeps pulling for a
    // few seconds and then loops, which snaps the bow back to a half draw.
    if (animator_.has_clip("bow_hold")) {
        clip_bow_aim_ = godot::StringName("bow_hold");
    }

    // Archery is optional: it needs its clips and a bow with a string.
    bool has_bow_clips = true;
    for (const godot::StringName& clip : {clip_bow_draw_, clip_bow_aim_, clip_bow_recoil_, clip_bow_walk_forward_,
                                          clip_bow_walk_back_, clip_bow_walk_left_, clip_bow_walk_right_}) {
        has_bow_clips = has_bow_clips && animation_library_->has_animation(clip);
    }
    const godot::TypedArray<godot::Node> strings = find_children("*", "E5BowString", true, false);
    for (const godot::Variant& node : strings) {
        bow_string_ = godot::Object::cast_to<E5BowString>(node);
        if (bow_string_ != nullptr) {
            break;
        }
    }
    archery_enabled_ = has_bow_clips && bow_string_ != nullptr;
    if (archery_enabled_) {
        setup_archery();
        // The shot cycle follows the clips, so the string and the hands stay in step.
        bow_timings_.draw_seconds = animator_.clip_length(clip_bow_draw_);
        bow_timings_.release_seconds = animator_.clip_length(clip_bow_recoil_);
    } else if (instant_clip(skills_.slot(0)) != nullptr) {
        // (Her standard attack's clip: the slot that is selected may hold a stance, which has none.)
        setup_spells();
    }
}

godot::Ref<godot::PackedScene> E5PlayerController::cast_glow(gameplay::SkillId spell) const {
    std::size_t index = cast_glows_.size(); // none: the arcane bolt uses the cast effect as it is
    switch (spell) {
    case gameplay::SkillId::Fireball:
    case gameplay::SkillId::Meteor:
        index = 0;
        break;
    case gameplay::SkillId::FrostNova:
        index = 1;
        break;
    case gameplay::SkillId::ChainLightning:
        index = 2;
        break;
    case gameplay::SkillId::StarBarrage:
        index = 3;
        break;
    case gameplay::SkillId::BlackHole:
    case gameplay::SkillId::Slash:
    case gameplay::SkillId::FlameBlade:
    case gameplay::SkillId::FrostEdge:
    case gameplay::SkillId::ThunderCleave:
    case gameplay::SkillId::StarWhirl:
    case gameplay::SkillId::AxeCombo:
    case gameplay::SkillId::Whirlwind:
    case gameplay::SkillId::Earthbreaker:
    case gameplay::SkillId::LeapStrike:
    case gameplay::SkillId::Battlecry:
        index = 4;
        break;
    default:
        break;
    }
    return index < cast_glows_.size() && cast_glows_.at(index).is_valid() ? cast_glows_.at(index) : cast_effect_;
}

void E5PlayerController::setup_spells() {
    spells_enabled_ = true;
    const std::array weapon_effects{weapon_effect_1_, weapon_effect_2_, weapon_effect_3_, weapon_effect_4_};
    for (std::size_t index = 0; index < weapon_effects.size(); ++index) {
        if (weapon_effects.at(index).is_null()) {
            continue;
        }
        const godot::String path = weapon_effects.at(index)->get_path().get_basename() + godot::String("_impact.tscn");
        if (godot::ResourceLoader::get_singleton()->exists(path)) {
            weapon_impacts_.at(index) = godot::ResourceLoader::get_singleton()->load(path);
        }
    }
    weapon_holder_ = godot::Object::cast_to<godot::Node3D>(find_child("WeaponHolder", true, false));
    // Whoever has the clip has a shield to raise.
    block_enabled_ = animator_.is_ready() && animator_.has_clip(clip_block_);
    for (const auto& [path, scene] :
         {std::pair{slash_arc_path, &slash_arc_}, std::pair{landing_dust_path, &landing_dust_},
          std::pair{block_spark_path, &block_spark_}}) {
        if (godot::ResourceLoader::get_singleton()->exists(path)) {
            *scene = godot::ResourceLoader::get_singleton()->load(path);
        }
    }
    if (cast_effect_.is_valid()) {
        // The variants lie beside the cast effect and are found by name.
        const godot::String base = cast_effect_->get_path().get_basename();
        for (std::size_t index = 0; index < cast_glows_.size(); ++index) {
            const godot::String path =
                base + godot::String("_") + godot::String(cast_glow_names.at(index)) + godot::String(".tscn");
            if (godot::ResourceLoader::get_singleton()->exists(path)) {
                cast_glows_.at(index) = godot::ResourceLoader::get_singleton()->load(path);
            }
        }
    }
    setup_skill_ui();
    // He has no aiming stance: the crosshair is always there to cast at.
    if (crosshair_ != nullptr) {
        crosshair_->set_visible(true);
    }
    // Deferred: the parent is still building its children while this node becomes ready.
    call_deferred("prewarm_effects");
}

void E5PlayerController::finish_prewarm(float delta) {
    if (prewarm_seconds_left_ <= 0.0F) {
        return;
    }
    prewarm_seconds_left_ -= delta;
    if (prewarm_seconds_left_ > 0.0F) {
        return;
    }
    for (const std::uint64_t id : prewarm_ids_) {
        // Only free what is still there.
        if (auto* const node = godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(id))) {
            node->queue_free();
        }
    }
    prewarm_ids_.clear();
}

void E5PlayerController::_physics_process(double delta) {
    E5_PROFILE_SCOPE("E5PlayerController::_physics_process");
    if (remote_) {
        update_remote(static_cast<float>(delta));
        return;
    }

    const godot::Input* const input = godot::Input::get_singleton();

    finish_prewarm(static_cast<float>(delta));
    update_shake(static_cast<float>(delta));
    if (update_vitals(static_cast<float>(delta))) {
        return;
    }

    // Which button is in use. A button keeps the skill it started until that is over and
    // the button is let go; with nothing going on, the left one comes first.
    const bool standard_pressed = attack_held();
    const bool selected_pressed = aim_held();
    if (quick_key_slot_ >= 0 && !selected_pressed) {
        if (skills_.slot(static_cast<std::size_t>(quick_key_slot_)) == gameplay::SkillId::DaggerCombo) {
            dagger_return_pending_ = true;
        }
        quick_key_slot_ = -1; // the key was let go: it is the right button no longer
    }
    if (dagger_return_pending_ && !is_busy() && !selected_pressed) {
        // The dagger is done: the bar goes back to the skill that was chosen before it.
        dagger_return_pending_ = false;
        if (dagger_return_slot_ >= 0 && skills_.selected() == gameplay::SkillId::DaggerCombo) {
            select_skill(dagger_return_slot_);
        }
    }
    const bool held =
        use_button_ == UseButton::Standard ? standard_pressed : use_button_ == UseButton::Selected && selected_pressed;
    if (!held && !is_busy()) {
        set_use_button(standard_pressed   ? UseButton::Standard
                       : selected_pressed ? UseButton::Selected
                                          : UseButton::None);
    }
    const bool aim_pressed =
        use_button_ == UseButton::Standard ? standard_pressed : use_button_ == UseButton::Selected && selected_pressed;
    // The other button, pressed while aiming, cancels.
    const bool other_pressed =
        use_button_ == UseButton::Standard ? selected_pressed : use_button_ == UseButton::Selected && standard_pressed;
    const bool other_just_pressed = other_pressed && !other_button_was_pressed_;
    other_button_was_pressed_ = other_pressed;
    const bool aim_just_pressed = aim_pressed && !aim_was_pressed_;
    aim_was_pressed_ = aim_pressed;
    for (float& left : cooldown_left_) {
        left = std::max(left - static_cast<float>(delta), 0.0F);
    }
    update_counter(static_cast<float>(delta));
    update_whirl(static_cast<float>(delta));
    update_eruptions(static_cast<float>(delta));
    if (is_leaping()) {
        // In the air, and for a moment after she lands, the leap is all that moves her.
        update_leap(static_cast<float>(delta));
        animator_.update(static_cast<float>(delta));
        return;
    }
    if (update_enrage(static_cast<float>(delta))) {
        return; // beside herself: she goes for the nearest enemy, whatever the keys say
    }
    if (update_pieces(static_cast<float>(delta))) {
        return; // from one enemy to the next: nobody steers her
    }
    update_tower_charge(aim_pressed, static_cast<float>(delta));
    // The shield: up while the key is held, as long as it lasts.
    if (block_enabled_) {
        const bool block_key = !input_blocked_ && input->is_action_pressed(actions::block);
        const bool able = !action_.active && !is_aiming() && is_on_floor() && !vitals_.dead;
        const gameplay::BlockStep block = gameplay::step_block(block_, block_key && !block_key_was_down_, block_key,
                                                               able, block_params_, static_cast<float>(delta));
        block_key_was_down_ = block_key;
        block_ = block.state;
        if (block.raised_now) {
            // Towards where the player looks: that is where the danger is.
            model_yaw_ = gameplay::facing_yaw(-std::sin(look_.yaw), -std::cos(look_.yaw));
            if (model_ != nullptr) {
                model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
            }
        }
    }

    if (dodge_enabled_) {
        dodge_cooldown_left_ = std::max(dodge_cooldown_left_ - static_cast<float>(delta), 0.0F);
        const bool alt_key =
            !input_blocked_ && animator_.has_clip(clip_dodge_alt_) && input->is_action_pressed(actions::dodge_alt);
        const bool dodge_key = alt_key || (!input_blocked_ && input->is_action_pressed(actions::dodge));
        if (dodge_key && !dodge_key_was_down_ && dodge_cooldown_left_ <= 0.0F && !is_busy() && is_on_floor() &&
            !vitals_.dead && !mounted_) {
            // The way she is steered (seen from the camera), or else the way she faces.
            const float right = input->get_axis(action_left_, action_right_);
            const float forward = input->get_axis(action_back_, action_forward_);
            godot::Vector3 way(-std::sin(look_.yaw) * forward + std::cos(look_.yaw) * right, 0.0F,
                               -std::cos(look_.yaw) * forward - std::sin(look_.yaw) * right);
            if (way.length() < 0.2F) {
                way = godot::Vector3(std::sin(model_yaw_), 0.0F, std::cos(model_yaw_));
            }
            dodge_direction_ = way.normalized();
            dodge_clip_ = alt_key ? clip_dodge_alt_ : clip_dodge_;
            dodge_seconds_ = std::max(animator_.clip_length(dodge_clip_) - dodge_hand_over_seconds, 0.2F);
            dodge_left_ = dodge_seconds_;
            model_yaw_ =
                gameplay::facing_yaw(static_cast<float>(dodge_direction_.x), static_cast<float>(dodge_direction_.z));
            if (model_ != nullptr) {
                model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
            }
        }
        dodge_key_was_down_ = dodge_key;
    }
    if (animator_.is_ready() && animator_.has_clip(clip_emote_)) {
        const bool emote_key = !input_blocked_ && input->is_action_pressed(actions::emote);
        const bool pressed = emote_key && !emote_key_was_down_;
        emote_key_was_down_ = emote_key;
        const bool steered = !input_blocked_ && (std::abs(input->get_axis(action_left_, action_right_)) > 0.2F ||
                                                 std::abs(input->get_axis(action_back_, action_forward_)) > 0.2F ||
                                                 input->is_action_pressed(action_jump_));
        if (emote_left_ > 0.0F) {
            emote_left_ -= static_cast<float>(delta);
            // Anything else she is asked to do ends it; so does the key again.
            if (pressed || steered || is_busy() || !is_on_floor() || vitals_.dead) {
                emote_left_ = 0.0F;
            }
        } else if (pressed && !steered && !is_busy() && is_on_floor() && !vitals_.dead && !mounted_) {
            emote_left_ = animator_.clip_length(clip_emote_);
        }
    }

    // Instant skills start on the press and play through; the bow stays down meanwhile.
    const bool instant = gameplay::skill_info(skills_.selected()).kind == gameplay::SkillKind::Instant;
    if (archery_enabled_ || spells_enabled_) {
        // A stance is on at once and holds her to nothing: no action begins.
        if (instant && aim_just_pressed && gameplay::skill_is_stance(skills_.selected()) && !mounted_ &&
            skill_ready(skills_.selected())) {
            start_stance(skills_.selected());
        }
        // A channel too is on at once; it is then what she does until it is over.
        if (instant && aim_just_pressed && gameplay::skill_is_channel(skills_.selected()) && !mounted_ && !is_busy() &&
            animator_.has_clip(clip_whirl_) && skill_ready(skills_.selected())) {
            start_channel(skills_.selected());
        }
        // The Vine Tower is charged for as long as the button is held, from the next step on.
        if (aim_just_pressed && skills_.selected() == gameplay::SkillId::VineTower && !mounted_ && !is_busy() &&
            is_on_floor() && animator_.has_clip(clip_kneel_) && skill_ready(skills_.selected())) {
            start_tower_charge(skills_.selected());
        }
        // Cut in Pieces takes her charge and begins, if there is anyone to cut.
        if (aim_just_pressed && skills_.selected() == gameplay::SkillId::CutInPieces && !mounted_ && !is_busy() &&
            is_on_floor() && skill_ready(skills_.selected()) && pieces_victim() != nullptr) {
            start_pieces(skills_.selected());
        }
        // Enrage is on at once too; what it does begins with the next step (update_enrage).
        if (aim_just_pressed && skills_.selected() == gameplay::SkillId::Enrage && !mounted_ && !is_busy() &&
            !is_enraged() && skill_ready(skills_.selected())) {
            start_enrage(skills_.selected());
        }
        // The jump attack is aimed for as long as its button is held, a mark showing where she would
        // land, and leaves the ground when it is let go, for the place the crosshair covers.
        if (aim_just_pressed && skills_.selected() == gameplay::SkillId::JumpAttack && !mounted_ && !is_busy() &&
            is_on_floor() && skill_ready(skills_.selected())) {
            leap_aiming_ = true;
        }
        update_leap_aim(aim_pressed);
        const bool start = instant && aim_just_pressed && !block_.raised && can_start_instant_skill(skills_.selected());
        // A combo is one movement, not three clicks timed to the frame: a press during a blow
        // counts for the next one, and so does a button that is still held when the blow ends.
        const bool combo = instant && is_combo(skills_.selected());
        const bool was_active = action_.active;
        if (combo && was_active && aim_just_pressed) {
            combo_queued_ = true;
        }
        gameplay::ActionStep action = gameplay::step_action(action_, start, action_timings_, static_cast<float>(delta));
        action_ = action.state;
        const bool blow_over = was_active && !action_.active;
        if (combo && blow_over && (combo_queued_ || aim_pressed) && combo_step_ >= 0 &&
            combo_step_ < gameplay::combo_length - 1 && can_start_instant_skill(skills_.selected())) {
            // In the same frame, so that the next blow takes over from this one's last pose
            // and not from a first step back towards standing.
            action = gameplay::step_action(action_, true, action_timings_, 0.0F);
            action_ = action.state;
        }
        if (action.started) {
            combo_arc_shown_ = false;
            combo_queued_ = false;
            start_instant_skill(skills_.selected());
        } else if (!action_.active) {
            combo_queued_ = false;
        }
        update_summon(static_cast<float>(delta));
        if (action.strike && action_skill_ == gameplay::SkillId::ThunderKick) {
            strike_kick();
        } else if (action.strike && action_skill_ == gameplay::SkillId::Kingfishers) {
            release_birds();
        } else if (action.strike) {
            cast_spell(action_skill_);
        }
        tick_combo(static_cast<float>(delta));
        if (action_.active && is_combo(action_skill_) && combo_step_ >= 0 && !combo_arc_shown_ &&
            action_.elapsed >= action_timings_.strike_at_seconds - look_of(action_skill_, combo_step_).before) {
            combo_arc_shown_ = true;
            show_slash_arc();
        }
    }

    if (archery_enabled_) {
        const bool cancel = is_aiming() && other_just_pressed;
        if (cancel) {
            aim_blocked_ = true;
        } else if (!aim_pressed) {
            aim_blocked_ = false;
        }
        const gameplay::BowInput bow_input{
            // The bow can only be raised on the ground; leaving it lowers the bow.
            // A draw that has begun may be held; a new one needs the skill to be ready. (The
            // follow-through of a shot counted as "still aiming", so with the button held the
            // next draw began at once and the cooldown was never asked: bug report 13.)
            .aim_held = aim_pressed && !aim_blocked_ && !instant && !action_.active && is_on_floor() &&
                        (bow_.phase == gameplay::BowPhase::Drawing || bow_.phase == gameplay::BowPhase::Aiming ||
                         skill_ready(skills_.selected())),
            .cancel_pressed = cancel,
            .build_charge = gameplay::skill_info(skills_.selected()).charges,
        };
        const gameplay::BowStep bow_step =
            gameplay::step_bow(bow_, bow_input, bow_timings(), static_cast<float>(delta));
        bow_ = bow_step.state;
        update_bow_string(bow_step.string_draw);
        update_nocked_arrow(bow_step.string_draw);
        // Use the skill before refreshing the rain marker: the volley goes where the
        // marker was while aiming, and the refresh hides it once the bow is released.
        if (bow_step.arrow_released) {
            use_skill(bow_step.shot_power);
        }
        update_rain_marker();
        update_charge_effect(bow_.charge, static_cast<float>(delta));
        update_aim_camera(static_cast<float>(delta));
    }

    const gameplay::MotorParams params = motor_params();
    // She stands still for the length of a kick.
    const bool dodging = dodge_left_ > 0.0F;
    const bool rooted = action_.active || input_blocked_ || block_.raised || dodging || tower_charging_;
    const gameplay::MotorInput motor_input{
        .move_right = rooted ? 0.0F : input->get_axis(action_left_, action_right_),
        .move_forward = rooted ? 0.0F : input->get_axis(action_back_, action_forward_),
        .sprint = input->is_action_pressed(action_sprint_),
        .jump = !input_blocked_ && input->is_action_pressed(action_jump_) && !is_busy() && !mounted_,
    };

    // Without the combo's push of the last frame: that is added on top of what the motor
    // decides, and must not be fed back into it (it would pile up into a slide).
    const godot::Vector3 current = get_velocity() - combo_push_;
    combo_push_ = godot::Vector3();
    const gameplay::MotorState state{
        .velocity = {.x = static_cast<float>(current.x),
                     .y = static_cast<float>(current.y),
                     .z = static_cast<float>(current.z)},
        .on_floor = is_on_floor(),
    };

    // Input is interpreted relative to the camera: "forward" is where it looks.
    gameplay::Vec3 next = gameplay::step_velocity(state, motor_input, look_.yaw, params, static_cast<float>(delta));
    // The sword combo is not fought on the spot: every blow carries her a step towards
    // where she faces (the clips are made for exactly these steps).
    if (action_.active && is_combo(action_skill_) && combo_step_ >= 0 && is_on_floor()) {
        // In the clip's time: it may be played slower or faster than it was made.
        const bool axe = action_skill_ == gameplay::SkillId::AxeCombo;
        float forward = gameplay::combo_advance_speed(combo_step_, action_.elapsed * action_playback_scale_, axe) *
                        action_playback_scale_ * (axe ? axe_advance_share : 1.0F);
        if (action_skill_ == gameplay::SkillId::DaggerCombo) {
            // Her own steps: each distance as a smooth step, like the others'.
            const DaggerStep& step = dagger_steps.at(static_cast<std::size_t>(std::clamp(combo_step_, 0, 2)));
            const float u = (action_.elapsed - step.from) / (step.to - step.from);
            forward = u > 0.0F && u < 1.0F ? step.metres * 6.0F * u * (1.0F - u) / (step.to - step.from) : 0.0F;
        }
        combo_push_ = godot::Vector3(std::sin(model_yaw_) * forward, 0.0F, std::cos(model_yaw_) * forward);
        next.x += static_cast<float>(combo_push_.x);
        next.z += static_cast<float>(combo_push_.z);
    }

    if (dodging) {
        // The roll carries her: its distance over its time, whatever the keys say.
        const float speed = dodge_distance_ / dodge_seconds_;
        next.x = static_cast<float>(dodge_direction_.x) * speed;
        next.z = static_cast<float>(dodge_direction_.z) * speed;
        dodge_left_ -= static_cast<float>(delta);
        if (dodge_left_ <= 0.0F) {
            dodge_left_ = 0.0F;
            dodge_cooldown_left_ = dodge_cooldown_seconds;
        }
    }
    set_velocity(godot::Vector3(next.x, next.y, next.z));
    {
        E5_PROFILE_SCOPE("move_and_slide");
        move_and_slide();
    }

    // Use the velocity that survived collision, so running into a wall idles.
    const godot::Vector3 resolved = get_velocity();
    const gameplay::Vec3 actual{
        .x = static_cast<float>(resolved.x), .y = static_cast<float>(resolved.y), .z = static_cast<float>(resolved.z)};
    update_facing(actual, static_cast<float>(delta));
    update_animation(actual, static_cast<float>(delta));
}

godot::Array E5PlayerController::get_net_state() const {
    godot::Array state;
    state.push_back(get_global_position());
    state.push_back(model_yaw_);
    state.push_back(godot::String(animator_.base_clip()));
    state.push_back(animator_.base_scale());
    state.push_back(look_.yaw);
    state.push_back(look_.pitch);
    state.push_back(vitals_.dead ? 0.0F : vitals_.health / std::max(get_effective_max_health(), 1.0F));
    return state;
}

void E5PlayerController::apply_net_state(const godot::Vector3& position, float facing, const godot::String& clip,
                                         float speed, float look_yaw, float look_pitch) {
    if (!has_net_state_) {
        // The first word of where she is: there at once, not gliding in from somewhere.
        set_global_position(position);
        model_yaw_ = facing;
        has_net_state_ = true;
    }
    net_position_ = position;
    net_facing_ = facing;
    net_clip_ = godot::StringName(clip);
    net_speed_ = speed;
    look_.yaw = look_yaw;
    look_.pitch = look_pitch;
    apply_look_to_nodes();
}

void E5PlayerController::note_net_event(int kind, float power) {
    if (remote_) {
        return;
    }
    godot::Array event;
    event.push_back(kind);
    event.push_back(static_cast<int>(skills_.selected_index()));
    event.push_back(power);
    event.push_back(combo_step_);
    event.push_back(look_.yaw);
    event.push_back(look_.pitch);
    net_events_.push_back(event);
}

godot::Array E5PlayerController::take_net_events() {
    const godot::Array events = net_events_;
    net_events_ = godot::Array();
    return events;
}

void E5PlayerController::apply_net_event(int kind, int slot, float power, int combo_step, float look_yaw,
                                         float look_pitch) {
    if (!remote_ || slot < 0) {
        return;
    }
    look_.yaw = look_yaw;
    look_.pitch = look_pitch;
    apply_look_to_nodes();
    skills_.select(static_cast<std::size_t>(slot));
    if (kind == 0) {
        // Started in the next frame of hers, as on the machine that plays her.
        pending_start_ = true;
        pending_combo_step_ = combo_step;
    } else if (archery_enabled_) {
        use_skill(power);
    }
}

void E5PlayerController::take_remote_damage(float amount, const godot::Vector3& /*position*/) {
    if (remote_ && amount > 0.0F) {
        outgoing_damage_ += amount;
    }
}

float E5PlayerController::take_outgoing_damage() {
    const float damage = outgoing_damage_;
    outgoing_damage_ = 0.0F;
    return damage;
}

// A remote hero follows what arrived last: she closes most of the distance within a tenth of
// a second, so she moves smoothly between messages that come fifteen times a second. Skills
// she was told of run as they do for the hero played here, only for show.
void E5PlayerController::update_remote(float delta) {
    if (has_net_state_) {
        const float share = 1.0F - std::exp(-14.0F * delta);
        set_global_position(get_global_position().lerp(net_position_, share));
        const float turn = std::remainder(net_facing_ - model_yaw_, 2.0F * std::numbers::pi_v<float>);
        // (Whirling, she turns here by herself: her facing arrives too seldom to show it.)
        model_yaw_ +=
            is_whirling() ? 2.0F * std::numbers::pi_v<float> * gameplay::whirl_turns_per_second * delta : turn * share;
        if (model_ != nullptr) {
            model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
        }
    }
    update_counter(delta);
    update_whirl(delta);
    update_eruptions(delta);
    update_enrage(delta);
    if ((archery_enabled_ || spells_enabled_) && animator_.is_ready()) {
        if (pending_start_ && skills_.selected() == gameplay::SkillId::Enrage) {
            // Where she runs and what she strikes arrive with her place and her clips.
            pending_start_ = false;
            start_enrage(skills_.selected());
        }
        if (pending_start_ && gameplay::skill_is_stance(skills_.selected())) {
            pending_start_ = false;
            start_stance(skills_.selected());
        }
        if (pending_start_ && gameplay::skill_is_channel(skills_.selected())) {
            pending_start_ = false;
            start_channel(skills_.selected());
        }
        if (pending_start_ && skills_.selected() == gameplay::SkillId::JumpAttack) {
            // Where she flies arrives with her place; what is left to show is where she lands.
            pending_start_ = false;
            remote_leap_left_ = leap_remote_seconds;
        }
        if (remote_leap_left_ > 0.0F) {
            remote_leap_left_ -= delta;
            if (remote_leap_left_ <= 0.0F) {
                E5Effect::spawn(effect_scene(leap_impact_path), get_parent(),
                                get_global_position() + godot::Vector3(0.0F, 0.06F, 0.0F));
            }
        }
        const bool start = pending_start_ && instant_clip(skills_.selected()) != nullptr;
        pending_start_ = false;
        if (start) {
            // Whatever she was doing is over: the next one has begun where she is played.
            action_ = gameplay::ActionState{};
        }
        const gameplay::ActionStep action = gameplay::step_action(action_, start, action_timings_, delta);
        action_ = action.state;
        if (action.started) {
            // The blow of the combo she is at, not the one this copy would count to.
            combo_step_ = pending_combo_step_ - 1;
            combo_idle_seconds_ = 0.0F;
            start_instant_skill(skills_.selected());
        }
        update_summon(delta);
        if (action.strike && action_skill_ == gameplay::SkillId::ThunderKick) {
            strike_kick();
        } else if (action.strike && action_skill_ == gameplay::SkillId::Kingfishers) {
            release_birds();
        } else if (action.strike) {
            cast_spell(action_skill_);
        }
        tick_combo(delta);
    }
    if (has_net_state_ && animator_.is_ready() && !net_clip_.is_empty() && animator_.has_clip(net_clip_)) {
        animator_.set_upper(godot::StringName());
        animator_.set_base(net_clip_, net_speed_);
    }
    animator_.update(delta);
}

bool E5PlayerController::skill_ready(gameplay::SkillId skill) const {
    if (!cooldowns_enabled_ || remote_) {
        return true; // a remote hero replays what her own machine has decided
    }
    if (gameplay::skill_needs_charge(skill)) {
        return charge_always_full_ || charge_ >= 1.0F;
    }
    return cooldown_left_.at(static_cast<std::size_t>(skill)) <= 0.0F;
}

void E5PlayerController::spend(gameplay::SkillId skill) {
    if (!cooldowns_enabled_ || remote_) {
        return;
    }
    if (gameplay::skill_needs_charge(skill)) {
        charge_ = charge_always_full_ ? 1.0F : 0.0F;
    } else {
        cooldown_left_.at(static_cast<std::size_t>(skill)) = gameplay::skill_cooldown_seconds(skill);
    }
}

void E5PlayerController::credit_damage(float damage, bool killed) {
    if (!remote_) {
        charge_ = gameplay::charge_after(charge_, damage, killed);
    }
}

float E5PlayerController::get_cast_progress() const {
    if (tower_charging_) {
        return std::clamp(tower_charge_ / gameplay::tower_full_charge_seconds, 0.0F, 1.0F);
    }
    const float seconds = get_cast_seconds();
    if (seconds <= 0.0F) {
        return -1.0F;
    }
    if (bow_.phase == gameplay::BowPhase::Aiming) {
        return 1.0F;
    }
    const float waited = bow_.phase == gameplay::BowPhase::Drawing ? bow_.phase_seconds : action_.elapsed;
    return std::clamp(waited / seconds, 0.0F, 1.0F);
}

float E5PlayerController::get_cast_seconds() const {
    if (tower_charging_) {
        return gameplay::tower_full_charge_seconds;
    }
    if (bow_.phase == gameplay::BowPhase::Drawing || bow_.phase == gameplay::BowPhase::Aiming) {
        return bow_timings().draw_seconds;
    }
    if (action_.active && !is_combo(action_skill_) && action_.elapsed < action_timings_.strike_at_seconds) {
        return action_timings_.strike_at_seconds;
    }
    return 0.0F;
}

float E5PlayerController::get_cast_power() const {
    return bow_.phase == gameplay::BowPhase::Aiming ? bow_.charge : 0.0F;
}

float E5PlayerController::get_skill_cooldown(int slot) const {
    if (slot < 0 || static_cast<std::size_t>(slot) >= gameplay::SkillBar::slot_count) {
        return 0.0F;
    }
    return cooldown_left_.at(static_cast<std::size_t>(skills_.slot(static_cast<std::size_t>(slot))));
}

float E5PlayerController::get_skill_cooldown_seconds(int slot) const {
    if (slot < 0 || static_cast<std::size_t>(slot) >= gameplay::SkillBar::slot_count) {
        return 0.0F;
    }
    return gameplay::skill_cooldown_seconds(skills_.slot(static_cast<std::size_t>(slot)));
}

bool E5PlayerController::skill_takes_charge(int slot) const {
    return slot >= 0 && static_cast<std::size_t>(slot) < gameplay::SkillBar::slot_count &&
           gameplay::skill_needs_charge(skills_.slot(static_cast<std::size_t>(slot)));
}

bool E5PlayerController::is_skill_ready(int slot) const {
    return slot >= 0 && static_cast<std::size_t>(slot) < gameplay::SkillBar::slot_count &&
           skill_ready(skills_.slot(static_cast<std::size_t>(slot)));
}

void E5PlayerController::take_damage_from(float amount, const godot::Vector3& from) {
    if (remote_ || amount <= 0.0F || vitals_.dead || dodge_left_ > 0.0F) {
        return; // (a dodge is not hit)
    }
    if (counter_left_ > 0.0F) {
        answer_blow(from); // she is hit all the same, unless her shield is in the way
    }
    const godot::Vector3 away = from - get_global_position();
    if (block_.raised && gameplay::shield_covers(model_yaw_, static_cast<float>(away.x), static_cast<float>(away.z))) {
        // On the shield: nothing gets through. Sparks where it struck, and she feels it.
        ++hits_blocked_;
        shake_ = std::max(shake_, block_shake);
        const godot::Vector3 forward(std::sin(model_yaw_), 0.0F, std::cos(model_yaw_));
        E5Effect::spawn(block_spark_, get_parent(),
                        get_global_position() + forward * 0.5F + godot::Vector3(0.0F, 1.1F, 0.0F));
        return;
    }
    take_damage(amount);
}

void E5PlayerController::take_damage(float amount) {
    if (remote_) {
        return; // decided on its own machine
    }
    if (amount > 0.0F && !vitals_.dead && dodge_left_ <= 0.0F) {
        if (resolve_left_ > 0.0F) {
            // Never Give Up: only a share of it gets through.
            resolve_spared_ += amount * (1.0F - gameplay::resolve_damage_share);
            amount *= gameplay::resolve_damage_share;
        }
        pending_damage_ += amount;
    }
}

bool E5PlayerController::aim_held() const {
    if (input_blocked_) {
        return false;
    }
    godot::Input* const input = godot::Input::get_singleton();
    return input->is_action_pressed(action_aim_) ||
           (quick_key_slot_ >= 0 &&
            input->is_action_pressed(skill_actions_.at(static_cast<std::size_t>(quick_key_slot_))));
}

godot::String E5PlayerController::get_last_skill() const {
    const std::string_view name = gameplay::skill_info(last_skill_).name;
    return last_skill_ == gameplay::SkillId::None
               ? godot::String()
               : godot::String::utf8(name.data(), static_cast<std::int64_t>(name.size()));
}

bool E5PlayerController::fights_with_sword() const {
    return skill_set_ == static_cast<int>(gameplay::SkillSet::Warrior) ||
           skill_set_ == static_cast<int>(gameplay::SkillSet::Blade);
}

int E5PlayerController::get_open_slot_count() const {
    return gameplay::open_slot_count(skill_bar_set());
}

// Which bar she has. The Archer's own is her standard shot alone; the skills she had are used
// where they must go on being tested and looked at: in test runs (`--benchmark`), unless
// `--menu-wished-skills` asks for the bar the players see, and with `--menu-archer-full`.
gameplay::SkillSet E5PlayerController::skill_bar_set() const {
    const gameplay::SkillSet set = skill_set_from(skill_set_);
    if (set != gameplay::SkillSet::Archer) {
        return set;
    }
    const godot::PackedStringArray args = godot::OS::get_singleton()->get_cmdline_user_args();
    const bool full = args.has("--menu-archer-full") || (args.has("--benchmark") && !args.has("--menu-wished-skills"));
    return full ? gameplay::SkillSet::ArcherFull : set;
}

godot::String E5PlayerController::get_skill_name(int slot) const {
    if (slot < 0 || static_cast<std::size_t>(slot) >= gameplay::SkillBar::slot_count) {
        return {};
    }
    const gameplay::SkillId skill = skills_.slot(static_cast<std::size_t>(slot));
    const std::string_view name = gameplay::skill_info(skill).name;
    return skill == gameplay::SkillId::None ? godot::String()
                                            : godot::String::utf8(name.data(), static_cast<std::int64_t>(name.size()));
}

void E5PlayerController::set_skill_bar_visible(bool visible) {
    if (skill_hud_ != nullptr) {
        skill_hud_->set_visible(visible);
    }
}

bool E5PlayerController::attack_held() const {
    return !input_blocked_ && godot::Input::get_singleton()->is_action_pressed(action_attack_);
}

void E5PlayerController::set_use_button(UseButton next) {
    if (next == use_button_) {
        return;
    }
    if (use_button_ == UseButton::Standard) {
        skills_.select(selected_slot_);
    }
    if (next == UseButton::Standard) {
        selected_slot_ = skills_.selected_index();
        skills_.select(0);
    }
    use_button_ = next;
}

bool E5PlayerController::try_set_mounted(bool mounted) {
    if (remote_ || input_blocked_ || is_busy() || !is_on_floor() || vitals_.dead) {
        return false;
    }
    set_mounted(mounted);
    return mounted_ == mounted;
}

void E5PlayerController::set_mounted(bool mounted) {
    mounted_ = mounted && !vitals_.dead && animator_.is_ready() && animator_.has_clip(clip_ride_);
    if (mounted_) {
        emote_left_ = 0.0F;
    }
}

gameplay::MotorParams E5PlayerController::motor_params() const {
    gameplay::MotorParams params = params_;
    if (mounted_) {
        params.walk_speed = mount_speed_;
        params.sprint_speed = mount_sprint_speed_;
    }
    if (inventory_ != nullptr) {
        const float faster = 1.0F + inventory_->bonuses().speed;
        params.walk_speed *= faster;
        params.sprint_speed *= faster;
    }
    if (is_whirling()) {
        params.walk_speed = gameplay::whirl_move_speed;
        params.sprint_speed = gameplay::whirl_move_speed;
    }
    if (stampede_.seconds_left > 0.0F) {
        params.walk_speed *= gameplay::stampede_move_share;
        params.sprint_speed *= gameplay::stampede_move_share;
    }
    if (is_enraged()) {
        params.walk_speed *= gameplay::enrage_move_share;
        params.sprint_speed *= gameplay::enrage_move_share;
    }
    // Held to a walk: the pace her walking clip is made for, whatever else is pressed.
    if (!input_blocked_ && !remote_ && !enrage_driving_ &&
        godot::Input::get_singleton()->is_action_pressed(actions::walk)) {
        params.walk_speed = mounted_ ? mount_walk_speed_ : walk_clip_speed;
        params.sprint_speed = params.walk_speed;
    }
    // While aiming she moves slowly and cannot sprint or jump.
    if (is_aiming()) {
        params.walk_speed = aim_move_speed_;
        params.sprint_speed = aim_move_speed_;
    }
    return params;
}

gameplay::VitalsParams E5PlayerController::effective_vitals() const {
    gameplay::VitalsParams params = vitals_params_;
    if (inventory_ != nullptr) {
        const gameplay::Bonuses worn = inventory_->bonuses();
        params.max_health += worn.health;
        params.regen_per_second += worn.regen;
    }
    return params;
}

float E5PlayerController::dealt(gameplay::SkillId skill, float power) const {
    if (remote_) {
        return 0.0F; // shown here for another player: her own machine counts what she does
    }
    const float damage = gameplay::skill_damage(skill, power);
    if (inventory_ == nullptr) {
        return damage;
    }
    // A weapon counts for what is done with it: a bow for what is shot, a sword for every blow of
    // a hero who fights with one (the warrior's skills are all her sword's).
    const gameplay::WeaponClass held = inventory_->weapon_class();
    const bool shot =
        held == gameplay::WeaponClass::Bow && gameplay::skill_info(skill).kind == gameplay::SkillKind::Bow;
    const bool struck = held == gameplay::WeaponClass::Sword && fights_with_sword();
    if (!shot && !struck) {
        return damage;
    }
    return gameplay::weapon_hit(damage, inventory_->bonuses(), static_cast<float>(godot::UtilityFunctions::randf()));
}

gameplay::BowTimings E5PlayerController::bow_timings() const {
    gameplay::BowTimings timings = bow_timings_;
    if (inventory_ != nullptr) {
        timings.draw_seconds /= 1.0F + inventory_->bonuses().draw_speed;
    }
    return timings;
}

float E5PlayerController::get_effective_max_health() const {
    return effective_vitals().max_health;
}

bool E5PlayerController::heal(float amount) {
    const float max_health = effective_vitals().max_health;
    if (amount <= 0.0F || vitals_.dead || vitals_.health >= max_health) {
        return false;
    }
    vitals_.health = std::min(vitals_.health + amount, max_health);
    return true;
}

bool E5PlayerController::update_vitals(float delta) {
    const float damage = pending_damage_;
    pending_damage_ = 0.0F;
    const gameplay::VitalsParams vitals_params = effective_vitals();
    // A charm taken off takes its extra health with it.
    vitals_.health = std::min(vitals_.health, vitals_params.max_health);
    const gameplay::VitalsStep step = gameplay::step_vitals(vitals_, damage, vitals_params, delta);
    vitals_ = step.state;
    const bool has_death_clip = animator_.is_ready() && animator_.has_clip(clip_death_);

    if (step.hurt) {
        damage_taken_ += damage;
        shake_ = std::max(shake_, hurt_shake);
        if (health_hud_ != nullptr) {
            health_hud_->flash(damage / std::max(vitals_params.max_health, 1.0F));
        }
    }
    if (step.died) {
        ++death_count_;
        // Whatever she was doing ends here.
        counter_left_ = 0.0F;
        counter_swing_left_ = 0.0F;
        whirl_ = {};
        leap_ = {};
        leap_aiming_ = false;
        if (leap_marker_ != nullptr) {
            leap_marker_->set_visible(false);
        }
        resolve_left_ = 0.0F;
        pieces_ = {};
        stampede_ = {};
        tower_charge_ = 0.0F; // (whatever she charged is lost: nothing grows)
        update_tower_charge(false, 0.0F);
        tower_charging_ = false;
        enrage_left_ = 0.0F;
        enrage_driving_ = false;
        enrage_blow_ = -1;
        action_ = {};
        bow_ = {};
        if (nocked_arrow_ != nullptr) {
            nocked_arrow_->set_visible(false);
        }
        if (animator_.is_ready()) {
            animator_.set_upper(godot::StringName());
            animator_.set_base(has_death_clip ? clip_death_ : clip_idle_, 1.0F);
        }
        logger().info("the player died");
    }
    if (step.respawned) {
        set_global_transform(spawn_transform_);
        set_velocity(godot::Vector3());
        if (model_ != nullptr) {
            model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
        }
        if (animator_.is_ready()) {
            animator_.set_base(clip_idle_, 1.0F);
        }
    }
    if (health_hud_ != nullptr) {
        health_hud_->show_health(vitals_.health, vitals_params.max_health);
        health_hud_->show_respawn(vitals_.dead ? vitals_params.respawn_seconds - vitals_.seconds_dead : -1.0F);
    }
    if (!vitals_.dead) {
        return false;
    }

    // Dead: no input. She drops to the ground and lies there.
    godot::Vector3 velocity = get_velocity();
    velocity.x = 0.0F;
    velocity.z = 0.0F;
    velocity.y = is_on_floor() ? 0.0F : velocity.y - params_.gravity * delta;
    set_velocity(velocity);
    move_and_slide();
    if (!has_death_clip && model_ != nullptr) {
        // No clip for it: she falls over backwards.
        const float tilt = std::min(vitals_.seconds_dead * fall_over_rate, fall_over_angle);
        model_->set_rotation(godot::Vector3(-tilt, model_yaw_, 0.0F));
    }
    animator_.update(delta);
    return true;
}

void E5PlayerController::update_facing(const gameplay::Vec3& velocity, float delta) {
    if (model_ == nullptr) {
        return;
    }
    if (is_whirling()) {
        // Round and round, whichever way she walks.
        model_yaw_ =
            std::remainder(model_yaw_ + 2.0F * std::numbers::pi_v<float> * gameplay::whirl_turns_per_second * delta,
                           2.0F * std::numbers::pi_v<float>);
        model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
        return;
    }
    if (tower_charging_) {
        return; // she kneels as she stood
    }
    float target = 0.0F;
    if (is_busy()) {
        // An archer faces where the camera looks and strafes, instead of turning into the movement.
        target = gameplay::facing_yaw(-std::sin(look_.yaw), -std::cos(look_.yaw));
    } else if (std::hypot(velocity.x, velocity.z) >= min_turn_speed) {
        target = gameplay::facing_yaw(velocity.x, velocity.z);
    } else {
        return;
    }
    model_yaw_ = gameplay::turn_toward(model_yaw_, target, turn_speed_ * delta);
    model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
}

void E5PlayerController::update_animation(const gameplay::Vec3& velocity, float delta) {
    if (!animator_.is_ready()) {
        return;
    }
    const float speed = std::hypot(velocity.x, velocity.z);

    if (dodge_left_ > 0.0F) {
        animator_.set_upper(godot::StringName());
        animator_.set_base(dodge_clip_, 1.0F);
        return;
    }
    if (emote_left_ > 0.0F) {
        animator_.set_upper(godot::StringName());
        animator_.set_base(clip_emote_, 1.0F);
        return;
    }
    if (tower_charging_) {
        animator_.set_upper(godot::StringName());
        animator_.set_base(clip_kneel_, 1.0F);
        animator_.update(delta);
        return;
    }
    if (is_whirling()) {
        // The pose she holds while the whole of her is turned (update_facing).
        animator_.set_upper(godot::StringName());
        animator_.set_base(clip_whirl_, 1.0F);
        animator_.update(delta);
        return;
    }
    if (mounted_) {
        // She sits; the bow is drawn and shot from the saddle with the upper body alone.
        animator_.set_base(clip_ride_, 1.0F);
        if (!is_aiming()) {
            animator_.set_upper(godot::StringName());
        } else if (bow_.phase == gameplay::BowPhase::Drawing) {
            animator_.set_upper(clip_bow_draw_);
        } else if (bow_.phase == gameplay::BowPhase::Releasing) {
            animator_.set_upper(clip_bow_recoil_);
        } else {
            animator_.set_upper(clip_bow_aim_);
        }
        animator_.update(delta);
        return;
    }
    if (block_.raised) {
        // The clip brings the shield up and then holds it there.
        animator_.set_upper(godot::StringName());
        animator_.set_base(clip_block_, 1.0F);
        animator_.update(delta);
        return;
    }
    if (action_.active) {
        animator_.set_upper(godot::StringName());
        if (const godot::StringName* const clip = instant_clip(action_skill_)) {
            animator_.set_base(*clip, action_playback_scale_);
        }
        animator_.update(delta);
        return;
    }

    if (is_aiming()) {
        // Whole body: the aim stance, or the aim-walk that matches the movement.
        const godot::StringName* base = &clip_bow_aim_;
        if (speed >= gameplay::LocomotionThresholds{}.idle_below) {
            // Velocity in the character's own frame: its front is +Z, its left is +X.
            const float sin_yaw = std::sin(model_yaw_);
            const float cos_yaw = std::cos(model_yaw_);
            const float forward_speed = velocity.x * sin_yaw + velocity.z * cos_yaw;
            const float left_speed = velocity.x * cos_yaw - velocity.z * sin_yaw;
            switch (gameplay::select_strafe_direction(forward_speed, left_speed)) {
            case gameplay::StrafeDirection::Forward:
                base = &clip_bow_walk_forward_;
                break;
            case gameplay::StrafeDirection::Back:
                base = &clip_bow_walk_back_;
                break;
            case gameplay::StrafeDirection::Left:
                base = &clip_bow_walk_left_;
                break;
            case gameplay::StrafeDirection::Right:
                base = &clip_bow_walk_right_;
                break;
            }
        }
        animator_.set_base(*base, 1.0F);

        // Upper body only: drawing and releasing, so the legs keep doing the above.
        if (bow_.phase == gameplay::BowPhase::Drawing) {
            animator_.set_upper(clip_bow_draw_);
        } else if (bow_.phase == gameplay::BowPhase::Releasing) {
            animator_.set_upper(clip_bow_recoil_);
        } else {
            // While strafing, the walk clips bring their own, weaker draw; keep the arms at full draw.
            animator_.set_upper(base != &clip_bow_aim_ ? clip_bow_aim_ : godot::StringName());
        }
    } else {
        // An answered blow: her arm strikes while her legs go on with what they do.
        animator_.set_upper(counter_swing_left_ > 0.0F && animator_.has_clip(clip_combo_.front())
                                ? clip_combo_.front()
                                : godot::StringName());
        // Coming down from a jump she settles into standing or running over a longer stretch.
        const bool landing = animator_.base_clip() == clip_jump_;
        const float fade = landing ? landing_fade_seconds : usual_fade_seconds;
        const float moving_fade = landing ? landing_moving_fade_seconds : usual_fade_seconds;
        switch (gameplay::select_locomotion_state(speed, is_on_floor())) {
        case gameplay::LocomotionState::Idle:
            animator_.set_base(clip_idle_, 1.0F, fade);
            break;
        case gameplay::LocomotionState::Walk:
            animator_.set_base(clip_walk_, speed / walk_clip_speed, moving_fade);
            break;
        case gameplay::LocomotionState::Run:
            animator_.set_base(clip_run_, speed / run_clip_speed, moving_fade);
            break;
        case gameplay::LocomotionState::Airborne:
            animator_.set_base(clip_jump_, 1.0F);
            break;
        }
    }
    animator_.update(delta);
}

void E5PlayerController::update_bow_string(float string_draw) {
    if (right_hand_ == nullptr) {
        // No hand to follow: pull the string straight back by the draw amount.
        bow_string_->set_draw(string_draw);
        return;
    }
    float follow = 0.0F;
    if (bow_.phase == gameplay::BowPhase::Aiming) {
        follow = 1.0F;
    } else if (bow_.phase == gameplay::BowPhase::Drawing) {
        const float t = std::clamp((string_draw - hand_takes_string_at) / (1.0F - hand_takes_string_at), 0.0F, 1.0F);
        follow = t * t * (3.0F - 2.0F * t);
    }
    const godot::Vector3 fingers =
        right_hand_->get_global_transform().xform(godot::Vector3(0.0F, fingers_from_wrist, 0.0F));
    bow_string_->set_draw(0.0F);
    bow_string_->set_nock_target(bow_string_->to_local(fingers), follow);
}
void E5PlayerController::setup_archery() {
    // The arrow that rests on the string; it never flies, it is only shown and hidden.
    nocked_arrow_ = memnew(E5Arrow);
    bow_string_->add_child(nocked_arrow_);
    nocked_arrow_->set_visible(false);

    setup_tip_glows();
    setup_charge_effect();
    // Deferred: the parent is still building its children while this node becomes ready.
    call_deferred("prewarm_effects");
    setup_aim_rig();
}

void E5PlayerController::setup_skill_ui() {
    if (godot::Skeleton3D* const skeleton = animator_.skeleton()) {
        // The hands, tracked after all modifiers: the string follows the right one, spells leave from them.
        const auto track = [skeleton](const char* bone) -> godot::BoneAttachment3D* {
            if (skeleton->find_bone(bone) < 0) {
                return nullptr;
            }
            auto* const attachment = memnew(godot::BoneAttachment3D);
            skeleton->add_child(attachment);
            attachment->set_bone_name(bone);
            return attachment;
        };
        right_hand_ = track("RightHand");
        left_hand_ = track("LeftHand");
    }

    if (camera_pivot_ != nullptr) {
        const godot::TypedArray<godot::Node> cameras = camera_pivot_->find_children("*", "Camera3D", true, false);
        for (const godot::Variant& node : cameras) {
            camera_ = godot::Object::cast_to<godot::Camera3D>(node);
            if (camera_ != nullptr) {
                break;
            }
        }
    }

    if (remote_) {
        // Her camera says where she aims; nobody looks through it (whoever makes a remote hero
        // switches it off before she enters the tree: see game/net/net.gd). No bars, no crosshair.
        return;
    }

    // In the middle of the screen: where skills go. Four arrowheads around a dot.
    crosshair_ = memnew(godot::CanvasLayer);
    add_child(crosshair_);
    auto* const dot = memnew(godot::TextureRect);
    dot->set_texture(godot::ResourceLoader::get_singleton()->load(crosshair_texture_path));
    // The picture is drawn larger than it is shown, so it stays sharp when the interface is
    // scaled up for a large screen.
    dot->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
    // It sits exactly where the captured mouse is. Left at the default it swallows mouse
    // events, and the camera stops turning whenever it is shown.
    dot->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
    for (const godot::Side side : {godot::SIDE_LEFT, godot::SIDE_TOP, godot::SIDE_RIGHT, godot::SIDE_BOTTOM}) {
        dot->set_anchor(side, 0.5F);
        dot->set_offset(side, side == godot::SIDE_LEFT || side == godot::SIDE_TOP ? -crosshair_half_size
                                                                                  : crosshair_half_size);
    }
    crosshair_->add_child(dot);
    crosshair_->set_visible(false);

    health_hud_ = memnew(E5HealthHud);
    add_child(health_hud_);
    // The bag, the pause menu and the loot notices are a script's (docs/DECISIONS.md, D-013).
    if (interface_ == nullptr && godot::ResourceLoader::get_singleton()->exists(interface_scene)) {
        const godot::Ref<godot::PackedScene> scene = godot::ResourceLoader::get_singleton()->load(interface_scene);
        if (scene.is_valid()) {
            interface_ = scene->instantiate();
            add_child(interface_);
        }
    }
    skill_hud_ = memnew(E5SkillBarHud);
    add_child(skill_hud_);
    for (std::size_t slot = 0; slot < gameplay::SkillBar::slot_count; ++slot) {
        const std::string_view name = gameplay::skill_info(skills_.slot(slot)).name;
        skill_hud_->set_slot_name(slot, godot::String::utf8(name.data(), static_cast<std::int64_t>(name.size())));
    }
    skill_hud_->set_selected(skills_.selected_index());
}

void E5PlayerController::setup_tip_glows() {
    // The arrowhead glows in the colour of the selected skill while it is on the string.
    std::vector<std::pair<gameplay::SkillId, godot::Ref<godot::PackedScene>>> glows{
        {gameplay::SkillId::FrostFan, frost_trail_effect_}, {gameplay::SkillId::FireArrow, fire_trail_effect_}};
    for (const SpecialArrow& special : special_arrows) {
        glows.emplace_back(special.skill, effect_scene(special.trail));
    }
    for (const auto& [skill, scene] : glows) {
        if (scene.is_null()) {
            continue;
        }
        if (auto* const glow = godot::Object::cast_to<godot::Node3D>(scene->instantiate())) {
            nocked_arrow_->add_child(glow);
            glow->set_position(godot::Vector3(0.0F, 0.0F, -E5Arrow::length));
            // The streak behind a flying arrow has no place on one that rests on the string.
            const godot::TypedArray<godot::Node> tails = glow->find_children("Tail*", "", true, false);
            for (const godot::Variant& node : tails) {
                if (auto* const tail = godot::Object::cast_to<godot::Node>(node)) {
                    glow->remove_child(tail);
                    tail->queue_free();
                }
            }
            // A trail is made for an arrow in flight; on a resting arrow it would pile up into a cloud.
            const godot::TypedArray<godot::Node> systems = glow->find_children("*", "GPUParticles3D", true, false);
            for (const godot::Variant& node : systems) {
                if (auto* const particles = godot::Object::cast_to<godot::GPUParticles3D>(node)) {
                    particles->set_amount_ratio(tip_glow_particle_share);
                }
            }
            E5Effect::set_active(glow, false);
            tip_glows_.push_back({.skill = skill, .node = glow});
        }
    }
}

void E5PlayerController::setup_aim_rig() {
    if (godot::Skeleton3D* const skeleton = animator_.skeleton()) {
        // Bends the spine with the vertical aim. First among the skeleton's modifiers, so
        // cloth and hair simulation react to the bent pose.
        aim_offset_ = memnew(E5AimOffset);
        skeleton->add_child(aim_offset_);
        skeleton->move_child(aim_offset_, 0);
    }
    setup_skill_ui();

    // Shows where an arrow rain would fall while it is being aimed.
    if (rain_marker_effect_.is_valid()) {
        rain_marker_ = godot::Object::cast_to<godot::Node3D>(rain_marker_effect_->instantiate());
        if (rain_marker_ != nullptr) {
            add_child(rain_marker_);
            rain_marker_->set_as_top_level(true); // placed in the world, not carried by the character
            rain_marker_->set_visible(false);
        }
    }
}

void E5PlayerController::update_nocked_arrow(float string_draw) {
    const bool on_string = bow_.phase == gameplay::BowPhase::Aiming ||
                           (bow_.phase == gameplay::BowPhase::Drawing && string_draw >= arrow_appears_at_draw);
    nocked_arrow_->set_visible(on_string);
    for (const TipGlow& glow : tip_glows_) {
        E5Effect::set_active(glow.node, on_string && skills_.selected() == glow.skill);
    }
    if (!on_string) {
        return;
    }
    // Tail on the string, shaft across the arrow rest at the grip.
    const godot::Vector3 nock = bow_string_->get_nock_position();
    godot::Vector3 forward = bow_string_->get_arrow_rest_position() - nock;
    forward = forward.length() > 0.01F ? forward.normalized() : -bow_string_->get_pull_direction().normalized();
    nocked_arrow_->set_transform(godot::Transform3D(godot::Basis::looking_at(forward, godot::Vector3(0.0F, 1.0F, 0.0F)),
                                                    bow_string_->get_nock_position()));
}

void E5PlayerController::update_aim_camera(float delta) {
    crosshair_->set_visible(is_aiming());
    if (aim_offset_ != nullptr) {
        // For the arrow rain she shoots into the sky while the player looks at the ground.
        const float pitch = skills_.selected() == gameplay::SkillId::ArrowRain
                                ? rain_body_pitch
                                : std::clamp(look_.pitch, -max_body_pitch, max_body_pitch);
        aim_offset_->set_pitch(pitch * aim_camera_blend_);
    }
    if (camera_arm_ == nullptr) {
        return;
    }
    // Also during the summon: the bird on her hand deserves a closer look.
    const bool summoning = action_.active && action_skill_ == gameplay::SkillId::Kingfishers;
    const float target = is_aiming() || summoning ? 1.0F : 0.0F;
    aim_camera_blend_ +=
        std::clamp(target - aim_camera_blend_, -aim_camera_blend_rate * delta, aim_camera_blend_rate * delta);
    camera_arm_->set_position(godot::Vector3(aim_shoulder_offset * aim_camera_blend_, 0.0F, 0.0F));
    // On the horse the camera stands further back: a horse is long, and its croup filled the picture.
    const float mount_step = mount_camera_rate * delta;
    mount_camera_blend_ += std::clamp((mounted_ ? 1.0F : 0.0F) - mount_camera_blend_, -mount_step, mount_step);
    const float rest_distance = camera_rest_distance_ + mount_camera_back * mount_camera_blend_;
    camera_arm_->set_length(std::lerp(rest_distance, aim_camera_distance, aim_camera_blend_));
}

void E5PlayerController::setup_charge_effect() {
    if (charge_effect_.is_null()) {
        return;
    }
    auto* const effect = godot::Object::cast_to<godot::Node3D>(charge_effect_->instantiate());
    if (effect == nullptr) {
        return;
    }
    // The orb sits on the arrow's tip and is driven by the charge level.
    nocked_arrow_->add_child(effect);
    effect->set_position(godot::Vector3(0.0F, 0.0F, -E5Arrow::length));

    const godot::TypedArray<godot::Node> systems = effect->find_children("*", "GPUParticles3D", true, false);
    for (const godot::Variant& node : systems) {
        if (auto* const particles = godot::Object::cast_to<godot::GPUParticles3D>(node)) {
            charge_particles_.push_back(particles);
        }
    }
    for (const char* const type : {"MeshInstance3D", "OmniLight3D"}) {
        const godot::TypedArray<godot::Node> visuals = effect->find_children("*", type, true, false);
        for (const godot::Variant& node : visuals) {
            if (auto* const visual = godot::Object::cast_to<godot::Node3D>(node)) {
                charge_visuals_.push_back(visual);
            }
        }
    }
    charge_core_ = godot::Object::cast_to<godot::Node3D>(effect->find_child("Core", true, false));
    charge_ground_ring_ = godot::Object::cast_to<godot::Node3D>(effect->find_child("GroundRing", true, false));
    charge_light_ = godot::Object::cast_to<godot::OmniLight3D>(effect->find_child("Light", true, false));
    if (charge_light_ != nullptr) {
        charge_light_energy_ = charge_light_->get_param(godot::Light3D::PARAM_ENERGY);
    }

    // The part of the effect that belongs at her feet must not tilt with the arrow.
    if (auto* const ground = godot::Object::cast_to<godot::Node3D>(effect->find_child("Ground", true, false))) {
        ground->reparent(this, false);
        ground->set_transform(godot::Transform3D());
    }
    update_charge_effect(0.0F, 0.0F);
}
void E5PlayerController::prewarm_effects() {
    if (remote_) {
        return; // the hero played here has warmed them already
    }
    // The first time an effect is drawn the graphics driver compiles its
    // pipelines, which froze the game for about 350 ms on the first power shot.
    // Showing each effect once at start, out of sight, moves that cost into loading.
    const godot::Vector3 out_of_sight = get_global_position() + godot::Vector3(0.0F, -40.0F, 0.0F);
    for (const godot::Ref<godot::PackedScene>& scene :
         {trail_effect_,       impact_effect_,        charge_full_effect_,      rain_impact_effect_,
          frost_trail_effect_, frost_impact_effect_,  fire_trail_effect_,       fire_impact_effect_,
          kick_effect_,        summon_cast_effect_,   summon_burst_effect_,     bird_scene_,
          cast_effect_,        bolt_trail_effect_,    bolt_impact_effect_,      nova_effect_,
          lightning_effect_,   meteor_marker_effect_, meteor_impact_effect_,    star_trail_effect_,
          star_impact_effect_, black_hole_effect_,    black_hole_burst_effect_, weapon_effect_1_,
          weapon_effect_2_,    weapon_effect_3_,      weapon_effect_4_}) {
        if (const godot::Node3D* const instance = E5Effect::spawn(scene, get_parent(), out_of_sight)) {
            prewarm_ids_.push_back(instance->get_instance_id());
        }
    }
    if (bow_string_ != nullptr) {
        for (const SpecialArrow& special : special_arrows) {
            for (const char* const path : {special.trail, special.impact, special.first, special.second}) {
                if (const godot::Node3D* const instance =
                        E5Effect::spawn(effect_scene(path), get_parent(), out_of_sight)) {
                    prewarm_ids_.push_back(instance->get_instance_id());
                }
            }
        }
    }
    for (std::size_t slot = 0; slot < gameplay::SkillBar::slot_count; ++slot) {
        if (skills_.slot(slot) != gameplay::SkillId::CounterAttack) {
            continue;
        }
        for (const char* const path : {counter_stance_path, counter_strike_path, counter_bleed_path, whirl_storm_path,
                                       leap_impact_path, seismic_burst_path, seismic_stun_path, enrage_path,
                                       resolve_path, stampede_path}) {
            if (const godot::Node3D* const instance = E5Effect::spawn(effect_scene(path), get_parent(), out_of_sight)) {
                prewarm_ids_.push_back(instance->get_instance_id());
            }
        }
    }
    for (const godot::Ref<godot::PackedScene>& scene : weapon_impacts_) {
        if (const godot::Node3D* const instance = E5Effect::spawn(scene, get_parent(), out_of_sight)) {
            prewarm_ids_.push_back(instance->get_instance_id());
        }
    }
    for (const godot::Ref<godot::PackedScene>& scene : cast_glows_) {
        if (const godot::Node3D* const instance = E5Effect::spawn(scene, get_parent(), out_of_sight)) {
            prewarm_ids_.push_back(instance->get_instance_id());
        }
    }
    prewarm_seconds_left_ = prewarm_seconds;
}

void E5PlayerController::update_charge_effect(float charge, float delta) {
    const bool charging = charge > 0.0F;
    charge_time_ = charging ? charge_time_ + delta : 0.0F;

    for (godot::GPUParticles3D* const particles : charge_particles_) {
        particles->set_emitting(charging);
        // Never fewer than a sixth of the particles, so the start of a charge is visible.
        particles->set_amount_ratio(std::max(charge, 0.16F));
        // The energy rushes in faster as the charge builds.
        particles->set_speed_scale(static_cast<double>(0.7F + 0.9F * charge));
    }
    for (godot::Node3D* const visual : charge_visuals_) {
        visual->set_visible(charging);
    }

    // A pulse that quickens and deepens with the charge keeps the orb alive.
    const float pulse = std::sin(charge_time_ * (10.0F + 14.0F * charge));
    if (charge_core_ != nullptr) {
        const float size = (0.25F + 0.75F * charge) * (1.0F + 0.12F * charge * pulse);
        charge_core_->set_scale(godot::Vector3(size, size, size));
    }
    if (charge_ground_ring_ != nullptr) {
        const float size = 0.35F + 0.65F * charge;
        charge_ground_ring_->set_scale(godot::Vector3(size, size, size));
    }
    if (charge_light_ != nullptr) {
        charge_light_->set_param(godot::Light3D::PARAM_ENERGY, charge_light_energy_ * charge * (1.0F + 0.2F * pulse));
    }

    // One flash at the moment the charge is complete: the cue to let go.
    const bool full = charge >= 1.0F;
    if (full && !charge_was_full_ && nocked_arrow_ != nullptr) {
        E5Effect::spawn(charge_full_effect_, get_parent(),
                        nocked_arrow_->to_global(godot::Vector3(0.0F, 0.0F, -E5Arrow::length)));
    }
    charge_was_full_ = full;
}
E5PlayerController::AimPoint E5PlayerController::find_aim_point(const godot::Vector3& fallback_origin) const {
    // Skills go to whatever the crosshair covers, not straight out of the bow:
    // the camera sits to the side of the bow, so the two lines differ.
    if (camera_ == nullptr) {
        // Without a camera: straight ahead of the character (its front is +Z).
        return {.position = fallback_origin +
                            model_->get_global_basis().xform(godot::Vector3(0.0F, 0.0F, 1.0F)) * aim_ray_length,
                .hit = false};
    }
    const godot::Vector3 origin = camera_->get_global_position();
    const godot::Vector3 direction = camera_->get_global_basis().xform(godot::Vector3(0.0F, 0.0F, -1.0F));
    const godot::Vector3 far_point = origin + direction * aim_ray_length;
    godot::TypedArray<godot::RID> excluded;
    excluded.push_back(get_rid());
    // Point blank the crosshair lies: the camera looks past her shoulder, so its line runs
    // beside whoever stands right in front of her, and the shot went by on the right (bug
    // report 10). A creature or hero within arm's reach ahead of her chest takes the shot.
    {
        const godot::Vector3 chest = get_global_position() + godot::Vector3(0.0F, point_blank_height, 0.0F);
        godot::Vector3 ahead(direction.x, 0.0F, direction.z);
        if (ahead.length_squared() > 0.0001F) {
            ahead = ahead.normalized();
            const godot::Ref<godot::PhysicsRayQueryParameters3D> near_query =
                godot::PhysicsRayQueryParameters3D::create(chest, chest + ahead * point_blank_reach, 0xFFFFFFFF,
                                                           excluded);
            const godot::Dictionary near_hit = get_world_3d()->get_direct_space_state()->intersect_ray(near_query);
            if (!near_hit.is_empty() && godot::Object::cast_to<godot::CharacterBody3D>(
                                            static_cast<godot::Object*>(near_hit["collider"])) != nullptr) {
                return {.position = near_hit["position"], .hit = true};
            }
        }
    }
    const godot::Ref<godot::PhysicsRayQueryParameters3D> query =
        godot::PhysicsRayQueryParameters3D::create(origin, far_point, 0xFFFFFFFF, excluded);
    const godot::Dictionary hit = get_world_3d()->get_direct_space_state()->intersect_ray(query);
    if (hit.is_empty()) {
        return {.position = far_point, .hit = false};
    }
    return {.position = hit["position"], .hit = true};
}

void E5PlayerController::use_skill(float power) {
    spend(skills_.selected());
    last_skill_ = skills_.selected();
    ++skills_used_;
    note_net_event(1, power);
    switch (skills_.selected()) {
    case gameplay::SkillId::VineTower:
        grow_tower(power);
        break;
    case gameplay::SkillId::ArrowRain:
        fire_rain();
        break;
    case gameplay::SkillId::FrostFan:
        fire_fan();
        break;
    case gameplay::SkillId::FireArrow:
        fire_blast_arrow();
        break;
    case gameplay::SkillId::VenomArrow:
    case gameplay::SkillId::GaleArrow:
    case gameplay::SkillId::StormArrow:
    case gameplay::SkillId::BrambleArrow:
        fire_special_arrow(skills_.selected());
        break;
    case gameplay::SkillId::ThunderKick: // not bow skills; handled by the action timeline
    case gameplay::SkillId::Kingfishers:
    case gameplay::SkillId::ArcaneBolt:
    case gameplay::SkillId::Fireball:
    case gameplay::SkillId::FrostNova:
    case gameplay::SkillId::ChainLightning:
    case gameplay::SkillId::Meteor:
    case gameplay::SkillId::StarBarrage:
    case gameplay::SkillId::BlackHole:
    case gameplay::SkillId::PowerShot:
    case gameplay::SkillId::Shot:
    case gameplay::SkillId::None:
    default: // the blows of the sword and the axe never come here either
        fire_arrow(power);
        break;
    }
}

E5Arrow* E5PlayerController::spawn_arrow(const godot::Vector3& position, const godot::Vector3& direction) {
    // Arrows belong to the world, not to the archer: they must stay where they land.
    auto* const arrow = memnew(E5Arrow);
    get_parent()->add_child(arrow);
    const godot::Vector3 up =
        std::abs(direction.y) > 0.99F ? godot::Vector3(1.0F, 0.0F, 0.0F) : godot::Vector3(0.0F, 1.0F, 0.0F);
    arrow->set_global_transform(godot::Transform3D(godot::Basis::looking_at(direction, up), position));
    return arrow;
}

void E5PlayerController::fire_fan() {
    const godot::Vector3 spawn = bow_string_->to_global(bow_string_->get_nock_position());
    const godot::Vector3 centre = (find_aim_point(spawn).position - spawn).normalized();
    for (int index = 0; index < fan_arrow_count; ++index) {
        // Fanned out sideways: the middle arrow goes to the crosshair.
        const godot::Vector3 direction = centre.rotated(godot::Vector3(0.0F, 1.0F, 0.0F),
                                                        gameplay::fan_yaw_offset(index, fan_arrow_count, fan_spread));
        E5Arrow* const arrow = spawn_arrow(spawn, direction);
        arrow->set_damage(dealt(gameplay::SkillId::FrostFan));
        arrow->set_trail_effect(frost_trail_effect_);
        arrow->set_impact_effect(frost_impact_effect_);
        arrow->launch(direction * arrow_speed_, get_rid());
    }
}

void E5PlayerController::fire_blast_arrow() {
    const godot::Vector3 spawn = bow_string_->to_global(bow_string_->get_nock_position());
    const godot::Vector3 direction = (find_aim_point(spawn).position - spawn).normalized();
    E5Arrow* const arrow = spawn_arrow(spawn, direction);
    arrow->set_damage(dealt(gameplay::SkillId::FireArrow));
    arrow->set_trail_effect(fire_trail_effect_);
    arrow->set_impact_effect(fire_impact_effect_);
    arrow->set_blast_radius(fire_blast_radius);
    // What it catches burns on. (Not for a hero shown here for another player: hers does no damage.)
    arrow->set_special(E5Arrow::Special::Burn,
                       dealt(gameplay::SkillId::FireArrow) > 0.0F ? gameplay::burn_tick_damage : 0.0F,
                       effect_scene("res://effects/burning.tscn"));
    arrow->launch(direction * arrow_speed_, get_rid());
}

void E5PlayerController::fire_special_arrow(gameplay::SkillId skill) {
    const auto special = std::ranges::find(special_arrows, skill, &SpecialArrow::skill);
    if (special == special_arrows.end()) {
        return;
    }
    const godot::Vector3 spawn = bow_string_->to_global(bow_string_->get_nock_position());
    const godot::Vector3 direction = (find_aim_point(spawn).position - spawn).normalized();
    E5Arrow* const arrow = spawn_arrow(spawn, direction);
    const float damage = dealt(skill);
    arrow->set_damage(damage);
    arrow->set_trail_effect(effect_scene(special->trail));
    arrow->set_impact_effect(effect_scene(special->impact));
    // A hero shown here for another player does no damage; neither does what her arrows leave.
    const float share = damage > 0.0F ? 1.0F : 0.0F;
    float second_damage = 0.0F;
    if (skill == gameplay::SkillId::VenomArrow) {
        second_damage = gameplay::venom_tick_damage * share;
    } else if (skill == gameplay::SkillId::StormArrow) {
        second_damage = damage * gameplay::storm_jump_share;
    } else if (skill == gameplay::SkillId::BrambleArrow) {
        arrow->set_blast_radius(gameplay::bramble_radius);
    }
    arrow->set_special(special->special, second_damage, effect_scene(special->first), effect_scene(special->second));
    arrow->launch(direction * arrow_speed_, get_rid());
}

const godot::StringName* E5PlayerController::instant_clip(gameplay::SkillId skill) const {
    const godot::StringName* clip = nullptr;
    if (skill == gameplay::SkillId::ThunderKick) {
        clip = &clip_kick_;
    } else if (skill == gameplay::SkillId::Kingfishers) {
        clip = &clip_summon_;
    } else if (skill == gameplay::SkillId::ArcaneBolt) {
        clip = &clip_bolt_;
    } else if (skill == gameplay::SkillId::Fireball) {
        clip = &clip_fireball_;
    } else if (skill == gameplay::SkillId::FrostNova) {
        clip = &clip_nova_;
    } else if (skill == gameplay::SkillId::ChainLightning) {
        clip = &clip_lightning_;
    } else if (skill == gameplay::SkillId::Meteor) {
        clip = &clip_meteor_;
    } else if (skill == gameplay::SkillId::StarBarrage) {
        clip = &clip_barrage_;
    } else if (skill == gameplay::SkillId::BlackHole) {
        clip = &clip_black_hole_;
    } else if (skill == gameplay::SkillId::Slash) {
        // The blow the combo is at; before the first one, its first.
        clip = &clip_combo_.at(static_cast<std::size_t>(std::clamp(combo_step_, 0, gameplay::combo_length - 1)));
    } else if (skill == gameplay::SkillId::FlameBlade) {
        clip = &clip_flame_;
    } else if (skill == gameplay::SkillId::FrostEdge) {
        clip = &clip_frost_;
    } else if (skill == gameplay::SkillId::ThunderCleave || skill == gameplay::SkillId::SeismicSlash) {
        clip = &clip_thunder_;
    } else if (skill == gameplay::SkillId::StarWhirl) {
        clip = &clip_star_;
    } else if (skill == gameplay::SkillId::AxeCombo) {
        clip = &clip_axe_combo_.at(static_cast<std::size_t>(std::clamp(combo_step_, 0, gameplay::combo_length - 1)));
    } else if (skill == gameplay::SkillId::DaggerCombo) {
        clip = &clip_dagger_combo_.at(static_cast<std::size_t>(std::clamp(combo_step_, 0, gameplay::combo_length - 1)));
    } else if (skill == gameplay::SkillId::Whirlwind) {
        clip = &clip_whirlwind_;
    } else if (skill == gameplay::SkillId::Earthbreaker) {
        clip = &clip_earthbreaker_;
    } else if (skill == gameplay::SkillId::LeapStrike) {
        clip = &clip_leap_;
    } else if (skill == gameplay::SkillId::Battlecry) {
        clip = &clip_battlecry_;
    }
    return clip != nullptr && animator_.has_clip(*clip) ? clip : nullptr;
}

bool E5PlayerController::can_start_instant_skill(gameplay::SkillId skill) const {
    // Nothing that takes the whole body from the saddle (a kick, the dagger, a cast).
    if (mounted_ || !is_on_floor() || instant_clip(skill) == nullptr || !skill_ready(skill)) {
        return false;
    }
    // One flock at a time: no new birds while the last ones are still flying. One black hole at a time.
    if (skill == gameplay::SkillId::BlackHole) {
        return get_tree()->get_nodes_in_group(E5BlackHole::group_name).is_empty();
    }
    return skill != gameplay::SkillId::Kingfishers || get_tree()->get_nodes_in_group(E5Bird::group_name).is_empty();
}

void E5PlayerController::tick_combo(float delta) {
    if (!action_.active) {
        combo_idle_seconds_ += delta;
    }
}

const SpellTiming* E5PlayerController::advance_combo(gameplay::SkillId skill) {
    const float idle = combo_idle_seconds_;
    combo_idle_seconds_ = 0.0F;
    if (!is_combo(skill)) {
        combo_step_ = -1;
        return nullptr;
    }
    // Pressed again soon after the last blow: the next blow of the combo.
    combo_step_ = gameplay::next_combo_step(combo_step_, idle, combo_window);
    // The clip was chosen before the step was known.
    action_timings_.duration_seconds = animator_.clip_length(*instant_clip(skill));
    const auto step = static_cast<std::size_t>(combo_step_);
    if (skill == gameplay::SkillId::DaggerCombo) {
        return &dagger_combo_timings.at(step);
    }
    return skill == gameplay::SkillId::AxeCombo ? &axe_combo_timings.at(step) : &combo_timings.at(step);
}

godot::String E5PlayerController::get_action_skill_name() const {
    if (!action_.active && !is_whirling() && !is_leaping() && !is_enraged() && !tower_charging_ && !is_cutting()) {
        return {};
    }
    const std::string_view name = gameplay::skill_info(action_skill_).name;
    return godot::String::utf8(name.data(), static_cast<std::int64_t>(name.size()));
}

void E5PlayerController::start_blade_effect(gameplay::SkillId skill) {
    if (weapon_holder_ == nullptr) {
        return;
    }
    // The special blows play their effect along the blade for as long as the blow lasts.
    godot::Node3D* const effect =
        E5Effect::spawn(blade_effect(skill), weapon_holder_, weapon_holder_->get_global_position());
    if (effect == nullptr) {
        return;
    }
    effect->set_transform(godot::Transform3D());
    if (auto* const timed = godot::Object::cast_to<E5Effect>(effect)) {
        timed->set_lifetime(action_timings_.duration_seconds);
    }
}

void E5PlayerController::start_instant_skill(gameplay::SkillId skill) {
    spend(skill);
    last_skill_ = skill;
    ++skills_used_;
    action_skill_ = skill;
    action_playback_scale_ = 1.0F;
    // An instant skill lasts as long as its clip.
    action_timings_.duration_seconds = animator_.clip_length(*instant_clip(skill));
    const SpellTiming* spell = timing_of(skill);
    if (const SpellTiming* const blow = advance_combo(skill)) {
        spell = blow;
    }
    const bool melee = is_melee(skill);
    if (spell != nullptr) {
        action_playback_scale_ = spell->playback_scale;
        action_timings_.duration_seconds =
            std::min(spell->end_at, action_timings_.duration_seconds) / spell->playback_scale;
        action_timings_.strike_at_seconds = spell->strike_at / spell->playback_scale;
        start_blade_effect(skill);
        // Power gathers in his casting hand until the spell leaves it; the glow rides on the hand.
        if (right_hand_ != nullptr && !melee) {
            E5Effect::spawn(cast_glow(skill), right_hand_, casting_point(false));
        }
        const bool one_handed = skill == gameplay::SkillId::ArcaneBolt || skill == gameplay::SkillId::ChainLightning;
        if (!one_handed && !melee && left_hand_ != nullptr) {
            E5Effect::spawn(cast_glow(skill), left_hand_,
                            left_hand_->get_global_transform().xform(godot::Vector3(0.0F, palm_from_wrist, 0.0F)));
        }
    } else if (skill == gameplay::SkillId::ThunderKick) {
        action_timings_.strike_at_seconds = action_timings_.duration_seconds * kick_strike_fraction;
    } else {
        action_playback_scale_ = summon_playback_scale;
        action_timings_.duration_seconds /= summon_playback_scale;
        action_timings_.strike_at_seconds = summon_release_seconds / summon_playback_scale;
        // Energy gathers in the raised hand until the birds are released; the effect rides on the hand.
        if (right_hand_ != nullptr) {
            E5Effect::spawn(summon_cast_effect_, right_hand_, raised_hand_position());
        }
        // The first kingfisher appears on her hand and opens its wings there.
        if (bird_scene_.is_valid()) {
            if (auto* const bird = godot::Object::cast_to<E5Bird>(bird_scene_->instantiate())) {
                bird->set_harmless(remote_);
                bird->perch();
                get_parent()->add_child(bird);
                bird->set_scale(godot::Vector3(0.001F, 0.001F, 0.001F));
                perched_bird_id_ = bird->get_instance_id();
            }
        }
    }
    note_net_event(0, 0.0F);
}

godot::Vector3 E5PlayerController::casting_point(bool both_hands) const {
    if (right_hand_ == nullptr) {
        // No hand to follow: chest height, a little in front of him.
        return get_global_position() + godot::Vector3(-std::sin(look_.yaw) * 0.6F, 1.3F, -std::cos(look_.yaw) * 0.6F);
    }
    const godot::Vector3 palm(0.0F, palm_from_wrist, 0.0F);
    const godot::Vector3 right = right_hand_->get_global_transform().xform(palm);
    if (!both_hands || left_hand_ == nullptr) {
        return right;
    }
    return (right + left_hand_->get_global_transform().xform(palm)) * 0.5F;
}

void E5PlayerController::cast_spell(gameplay::SkillId spell) {
    if (spell == gameplay::SkillId::FrostNova) {
        // A ring of frost around him, on the ground he stands on.
        const godot::Vector3 centre = get_global_position();
        E5Effect::spawn(nova_effect_, get_parent(), centre + godot::Vector3(0.0F, 0.08F, 0.0F));
        combat::blast(this, centre + godot::Vector3(0.0F, nova_height, 0.0F), nova_radius, dealt(spell));
        return;
    }
    if (spell == gameplay::SkillId::ChainLightning) {
        cast_lightning();
        return;
    }
    if (spell == gameplay::SkillId::Meteor) {
        cast_meteor();
        return;
    }
    if (spell == gameplay::SkillId::StarBarrage) {
        cast_star_barrage();
        return;
    }
    if (spell == gameplay::SkillId::BlackHole) {
        cast_black_hole();
        return;
    }
    if (spell == gameplay::SkillId::SeismicSlash) {
        strike_seismic();
        return;
    }
    if (is_melee(spell)) {
        strike_melee(spell);
        return;
    }
    const bool fireball = spell == gameplay::SkillId::Fireball;
    const godot::Vector3 origin = casting_point(fireball);
    // To whatever the crosshair covers, like the archer's arrows.
    const godot::Vector3 direction = (find_aim_point(origin).position - origin).normalized();
    // Spells belong to the world, not to the caster: they fly on when he moves.
    auto* const bolt = memnew(E5SpellBolt);
    get_parent()->add_child(bolt);
    bolt->set_global_position(origin);
    bolt->set_damage(dealt(spell));
    if (fireball) {
        bolt->set_trail_effect(fire_trail_effect_, fireball_trail_scale);
        bolt->set_impact_effect(fire_impact_effect_);
        bolt->set_blast_radius(fireball_blast_radius);
    } else {
        bolt->set_trail_effect(bolt_trail_effect_);
        bolt->set_impact_effect(bolt_impact_effect_);
    }
    bolt->launch(direction * (fireball ? fireball_speed : bolt_speed), get_rid());
}

void E5PlayerController::cast_lightning() {
    const godot::Color colour(0.5F, 0.9F, 3.0F);
    const float damage = dealt(gameplay::SkillId::ChainLightning);
    const godot::Vector3 origin = casting_point(false);
    godot::Vector3 current = find_aim_point(origin).position;
    E5LightningArc::spawn(get_parent(), origin, current, colour);
    E5Effect::spawn(lightning_effect_, get_parent(), current);
    combat::blast(this, current, lightning_strike_radius, damage);

    // From there it jumps to the nearest enemy it has not touched yet, and on from that one.
    std::vector<E5Enemy*> untouched;
    const godot::TypedArray<godot::Node> enemies = get_tree()->get_nodes_in_group(E5Enemy::group_name);
    for (const godot::Variant& node : enemies) {
        auto* const enemy = godot::Object::cast_to<E5Enemy>(node);
        // Those within the first strike have had theirs.
        if (enemy != nullptr && enemy->is_alive() &&
            enemy->get_aim_point().distance_to(current) > lightning_strike_radius + enemy->get_body_radius()) {
            untouched.push_back(enemy);
        }
    }
    for (int jump = 0; jump < lightning_jumps && !untouched.empty(); ++jump) {
        const auto nearest = std::ranges::min_element(untouched, {}, [&current](const E5Enemy* enemy) {
            return enemy->get_aim_point().distance_squared_to(current);
        });
        const godot::Vector3 body = (*nearest)->get_aim_point();
        if (body.distance_to(current) > lightning_jump_reach) {
            break;
        }
        E5LightningArc::spawn(get_parent(), current, body, colour);
        E5Effect::spawn(lightning_effect_, get_parent(), body);
        combat::hit(*nearest, body, damage * lightning_jump_share);
        current = body;
        untouched.erase(nearest);
    }
}

void E5PlayerController::cast_meteor() {
    // Only onto something: a place on the ground, a wall, an enemy, within range.
    const AimPoint aim = find_aim_point(get_global_position());
    if (!aim.hit || aim.position.distance_to(get_global_position()) > meteor_range) {
        return;
    }
    E5Effect::spawn(meteor_marker_effect_, get_parent(), aim.position + godot::Vector3(0.0F, 0.06F, 0.0F));
    godot::Vector3 towards_him = get_global_position() - aim.position;
    towards_him.y = 0.0F;
    towards_him = towards_him.length() > 0.01F ? towards_him.normalized() : godot::Vector3(0.0F, 0.0F, 1.0F);
    const godot::Vector3 start =
        aim.position + towards_him * meteor_setback + godot::Vector3(0.0F, meteor_height, 0.0F);
    auto* const meteor = memnew(E5SpellBolt);
    get_parent()->add_child(meteor);
    meteor->set_global_position(start);
    meteor->set_damage(dealt(gameplay::SkillId::Meteor));
    meteor->set_trail_effect(fire_trail_effect_, meteor_trail_scale);
    meteor->set_impact_effect(meteor_impact_effect_);
    meteor->set_blast_radius(meteor_blast_radius);
    meteor->launch((aim.position - start).normalized() * meteor_speed, get_rid());
}

godot::Ref<godot::PackedScene> E5PlayerController::blade_effect(gameplay::SkillId skill) const {
    switch (skill) {
    case gameplay::SkillId::FlameBlade:
        return weapon_effect_1_;
    case gameplay::SkillId::FrostEdge:
        return weapon_effect_2_;
    case gameplay::SkillId::ThunderCleave:
        return weapon_effect_3_;
    case gameplay::SkillId::StarWhirl:
        return weapon_effect_4_;
    case gameplay::SkillId::Whirlwind:
        return weapon_effect_1_;
    case gameplay::SkillId::Earthbreaker:
        return weapon_effect_2_;
    case gameplay::SkillId::LeapStrike:
        return weapon_effect_3_;
    default:
        return {}; // the plain combo has none
    }
}

godot::Ref<godot::PackedScene> E5PlayerController::impact_effect(gameplay::SkillId skill) const {
    const godot::Ref<godot::PackedScene> on_weapon = blade_effect(skill);
    const std::array effects{weapon_effect_1_, weapon_effect_2_, weapon_effect_3_, weapon_effect_4_};
    for (std::size_t index = 0; index < effects.size(); ++index) {
        if (on_weapon.is_valid() && on_weapon == effects.at(index)) {
            return weapon_impacts_.at(index);
        }
    }
    return {};
}

void E5PlayerController::update_shake(float delta) {
    if (camera_ == nullptr || shake_ <= 0.0F) {
        return;
    }
    shake_time_ += delta;
    shake_ *= std::exp(-shake_fade * delta);
    if (shake_ < 0.002F) {
        shake_ = 0.0F;
    }
    // Two sines of unrelated speeds: jittery, and the same on every machine.
    camera_->set_h_offset(std::sin(shake_time_ * 83.0F) * shake_);
    camera_->set_v_offset(std::sin(shake_time_ * 127.0F + 1.3F) * shake_);
}

void E5PlayerController::show_slash_arc() {
    if (slash_arc_.is_null() || model_ == nullptr) {
        return;
    }
    const ComboLook& look = look_of(action_skill_, combo_step_);
    if (look.arc_seconds <= 0.0F) {
        return; // a thrust: nothing sweeps
    }
    // Hung on her, so that it goes along with her step.
    godot::Node3D* const arc =
        E5Effect::spawn(slash_arc_, this, get_global_position() + godot::Vector3(0.0F, 1.2F, 0.0F));
    if (arc == nullptr) {
        return;
    }
    const godot::Vector3 forward(std::sin(model_yaw_), 0.0F, std::cos(model_yaw_));
    const godot::Vector3 right(-forward.z, 0.0F, forward.x);
    const godot::Vector3 up(0.0F, 1.0F, 0.0F);
    const auto in_world = [&](const godot::Vector3& own) { return right * own.x + up * own.y + forward * own.z; };
    const godot::Vector3 x = in_world(look.x).normalized();
    godot::Vector3 y = in_world(look.y);
    y = (y - x * y.dot(x)).normalized();
    arc->set_global_basis(godot::Basis(x, y, x.cross(y)));
    if (auto* const particles = godot::Object::cast_to<godot::GPUParticles3D>(arc->find_child("Arc", true, false))) {
        particles->set_lifetime(static_cast<double>(look.arc_seconds));
    }
}

void E5PlayerController::strike_melee(gameplay::SkillId skill) {
    const MeleeBlow& blow = blow_of(skill);
    // Where the player looks, like every other skill.
    const godot::Vector3 forward(-std::sin(look_.yaw), 0.0F, -std::cos(look_.yaw));
    const godot::Vector3 centre =
        get_global_position() + forward * blow.reach + godot::Vector3(0.0F, melee_height, 0.0F);
    float damage = dealt(skill);
    if (is_combo(skill)) {
        damage *= gameplay::combo_damage_factor(combo_step_);
    }
    combat::blast(this, centre, blow.radius, damage);
    shake_ = std::max(shake_, blow.shake);
    if (is_combo(skill) && combo_step_ >= 0) {
        shake_ = std::max(shake_, look_of(skill, combo_step_).shake);
        // The finisher comes down out of the air: the ground answers.
        if (combo_step_ == gameplay::combo_length - 1 && landing_dust_.is_valid() &&
            skill != gameplay::SkillId::DaggerCombo) {
            E5Effect::spawn(landing_dust_, get_parent(),
                            get_global_position() + forward * 0.9F + godot::Vector3(0.0F, 0.06F, 0.0F));
        }
    }
    // On the ground where the blow lands, turned the way she strikes.
    if (godot::Node3D* const impact =
            E5Effect::spawn(impact_effect(skill), get_parent(),
                            get_global_position() + forward * blow.reach + godot::Vector3(0.0F, 0.06F, 0.0F))) {
        impact->set_rotation(godot::Vector3(0.0F, gameplay::facing_yaw(forward.x, forward.z), 0.0F));
    }
    if (skill == gameplay::SkillId::Battlecry) {
        // The cry rolls out from his feet and throws back everything it reaches.
        E5Effect::spawn(weapon_effect_4_, get_parent(), get_global_position() + godot::Vector3(0.0F, 0.08F, 0.0F));
        const godot::TypedArray<godot::Node> enemies = get_tree()->get_nodes_in_group(E5Enemy::group_name);
        for (const godot::Variant& node : enemies) {
            auto* const enemy = godot::Object::cast_to<E5Enemy>(node);
            if (enemy == nullptr || !enemy->is_alive() || enemy->is_held()) {
                continue;
            }
            godot::Vector3 away = enemy->get_global_position() - get_global_position();
            away.y = 0.0F;
            if (away.length() <= blow.radius + enemy->get_body_radius()) {
                enemy->fling(away.normalized() * cry_push_speed + godot::Vector3(0.0F, cry_push_lift, 0.0F));
            }
        }
    }
    // The cleave's lightning leaps from the tip of the blade into the ground where it lands.
    if (skill == gameplay::SkillId::ThunderCleave && weapon_holder_ != nullptr) {
        const godot::Vector3 tip =
            weapon_holder_->get_global_transform().xform(godot::Vector3(0.0F, blade_length, 0.0F));
        const godot::Vector3 right(-forward.z, 0.0F, forward.x);
        for (const float side : {-1.0F, 0.0F, 1.0F}) {
            E5LightningArc::spawn(get_parent(), tip,
                                  get_global_position() + forward * (blow.reach + 0.6F) + right * (side * 0.9F) +
                                      godot::Vector3(0.0F, 0.05F, 0.0F),
                                  godot::Color(0.6F, 0.9F, 3.0F));
        }
    }
}

void E5PlayerController::start_stance(gameplay::SkillId skill) {
    spend(skill);
    last_skill_ = skill;
    ++skills_used_;
    // Round her and with her for as long as it lasts: the effect's own lifetime is the stance's.
    if (skill == gameplay::SkillId::NeverGiveUp) {
        resolve_left_ = gameplay::resolve_seconds;
        E5Effect::spawn(effect_scene(resolve_path), this, get_global_position());
    } else if (skill == gameplay::SkillId::Stampede) {
        stampede_ = gameplay::open_wound({}, gameplay::stampede_seconds, gameplay::stampede_tick_seconds);
        E5Effect::spawn(effect_scene(stampede_path), this, get_global_position());
    } else {
        counter_left_ = gameplay::counter_seconds;
        E5Effect::spawn(effect_scene(counter_stance_path), this, get_global_position());
    }
    note_net_event(0, 0.0F);
}

void E5PlayerController::answer_blow(const godot::Vector3& from) {
    // Who struck: the living enemy nearest to where the blow came from, if it is that near to
    // the place and within her reach. (An arrow or a bolt comes from no enemy's place.)
    E5Enemy* attacker = nullptr;
    float nearest = counter_attacker_slack;
    const godot::TypedArray<godot::Node> enemies = get_tree()->get_nodes_in_group(E5Enemy::group_name);
    for (const godot::Variant& node : enemies) {
        auto* const enemy = godot::Object::cast_to<E5Enemy>(node);
        if (enemy == nullptr || !enemy->is_alive()) {
            continue;
        }
        const float distance = static_cast<float>(enemy->get_global_position().distance_to(from));
        if (distance <= nearest) {
            nearest = distance;
            attacker = enemy;
        }
    }
    if (attacker == nullptr) {
        return;
    }
    godot::Vector3 towards = attacker->get_global_position() - get_global_position();
    towards.y = 0.0F;
    if (static_cast<float>(towards.length()) > gameplay::counter_reach + attacker->get_body_radius()) {
        return;
    }
    const godot::Vector3 body = attacker->get_aim_point();
    combat::hit(attacker, body, dealt(gameplay::SkillId::CounterAttack));
    ++counters_struck_;
    shake_ = std::max(shake_, counter_shake);
    counter_swing_left_ = counter_swing_seconds;
    E5Effect::spawn(effect_scene(counter_strike_path), get_parent(), body);

    // The wound: one for each enemy, begun anew by every answer.
    const std::uint64_t id = attacker->get_instance_id();
    auto wound = std::ranges::find(bleeding_, id, &Bleeding::enemy);
    if (wound == bleeding_.end()) {
        bleeding_.push_back({.enemy = id, .wound = {}});
        wound = bleeding_.end() - 1;
    }
    wound->wound =
        gameplay::open_wound(wound->wound, gameplay::counter_bleed_seconds, gameplay::counter_bleed_tick_seconds);

    // The arc of her blade, towards the one she answers: level, dipping on the far side.
    if (slash_arc_.is_null() || towards.length() < 0.05F) {
        return;
    }
    if (godot::Node3D* const arc =
            E5Effect::spawn(slash_arc_, this, get_global_position() + godot::Vector3(0.0F, 1.2F, 0.0F))) {
        const godot::Vector3 forward = towards.normalized();
        const godot::Vector3 right(-forward.z, 0.0F, forward.x);
        const godot::Vector3 x = (forward * 0.94F + godot::Vector3(0.0F, -0.34F, 0.0F)).normalized();
        const godot::Vector3 y = (right - x * right.dot(x)).normalized();
        arc->set_global_basis(godot::Basis(x, y, x.cross(y)));
    }
}

void E5PlayerController::update_counter(float delta) {
    counter_left_ = std::max(counter_left_ - delta, 0.0F);
    if (stampede_.seconds_left > 0.0F) {
        // Sprintsz: the ground she pounds hurts what is near, and stone flies up round her.
        const gameplay::BleedStep pounding = gameplay::step_bleed(stampede_, gameplay::stampede_tick_seconds, delta);
        stampede_ = pounding.state;
        for (int tick = 0; tick < pounding.ticks; ++tick) {
            ++stampede_ticks_;
            if (!remote_) {
                combat::blast(this, get_global_position() + godot::Vector3(0.0F, melee_height, 0.0F),
                              gameplay::stampede_radius, dealt(gameplay::SkillId::Stampede));
                shake_ = std::max(shake_, stampede_shake);
            }
            for (int burst = 0; burst < stampede_bursts; ++burst) {
                // Spread round her, never twice in the same place (the golden angle), one after another.
                const float angle = 2.39996F * static_cast<float>(stampede_ticks_ * stampede_bursts + burst);
                const float away = gameplay::stampede_radius * (0.35F + 0.2F * static_cast<float>(burst));
                eruptions_.push_back({.in_seconds = 0.08F * static_cast<float>(burst),
                                      .place = get_global_position() + godot::Vector3(std::cos(angle) * away, 0.05F,
                                                                                      std::sin(angle) * away),
                                      .size = stampede_burst_size});
            }
        }
    }
    if (resolve_left_ > 0.0F) {
        // Never Give Up: her wounds close, hurt a moment ago or not. (Not for a hero shown here
        // for another player: her health is counted where she is played.)
        const float lasts = std::min(delta, resolve_left_);
        resolve_left_ -= lasts;
        if (!remote_ && !vitals_.dead) {
            const float full = effective_vitals().max_health;
            const float gained = std::min(full * gameplay::resolve_heal_share * lasts, std::max(full - vitals_.health, 0.0F));
            vitals_.health += gained;
            resolve_healed_ += gained;
        }
    }
    counter_swing_left_ = std::max(counter_swing_left_ - delta, 0.0F);
    for (Bleeding& bleeding : bleeding_) {
        auto* const enemy = godot::Object::cast_to<E5Enemy>(godot::ObjectDB::get_instance(bleeding.enemy));
        if (enemy == nullptr || !enemy->is_alive()) {
            bleeding.wound = {};
            continue;
        }
        const gameplay::BleedStep step =
            gameplay::step_bleed(bleeding.wound, gameplay::counter_bleed_tick_seconds, delta);
        bleeding.wound = step.state;
        for (int tick = 0; tick < step.ticks; ++tick) {
            const godot::Vector3 body = enemy->get_aim_point();
            combat::hit(enemy, body, gameplay::counter_bleed_tick_damage);
            E5Effect::spawn(effect_scene(counter_bleed_path), get_parent(), body);
        }
    }
    std::erase_if(bleeding_, [](const Bleeding& bleeding) { return bleeding.wound.seconds_left <= 0.0F; });
}

void E5PlayerController::start_channel(gameplay::SkillId skill) {
    spend(skill);
    last_skill_ = skill;
    ++skills_used_;
    action_skill_ = skill;
    whirl_ = gameplay::open_wound({}, gameplay::whirl_seconds, gameplay::whirl_tick_seconds);
    // The wind round her, with her for as long as it lasts.
    E5Effect::spawn(effect_scene(whirl_storm_path), this, get_global_position());
    note_net_event(0, 0.0F);
}

void E5PlayerController::update_whirl(float delta) {
    if (!is_whirling()) {
        return;
    }
    const gameplay::BleedStep step = gameplay::step_bleed(whirl_, gameplay::whirl_tick_seconds, delta);
    whirl_ = step.state;
    if (remote_) {
        return; // shown here; what it does is decided where she is played
    }
    for (int tick = 0; tick < step.ticks; ++tick) {
        combat::blast(this, get_global_position() + godot::Vector3(0.0F, melee_height, 0.0F), gameplay::whirl_radius,
                      dealt(gameplay::SkillId::BladeWhirl));
        shake_ = std::max(shake_, whirl_shake);
    }
    // What stands near is drawn in, step by step.
    const godot::TypedArray<godot::Node> enemies = get_tree()->get_nodes_in_group(E5Enemy::group_name);
    for (const godot::Variant& node : enemies) {
        auto* const enemy = godot::Object::cast_to<E5Enemy>(node);
        if (enemy == nullptr || !enemy->is_alive() || enemy->is_held()) {
            continue;
        }
        godot::Vector3 towards = get_global_position() - enemy->get_global_position();
        towards.y = 0.0F;
        const auto distance = static_cast<float>(towards.length());
        if (distance > whirl_pull_keep_off && distance <= gameplay::whirl_pull_radius + enemy->get_body_radius()) {
            enemy->drag(towards / distance * gameplay::whirl_pull_speed);
        }
    }
}

void E5PlayerController::start_tower_charge(gameplay::SkillId skill) {
    last_skill_ = skill;
    action_skill_ = skill;
    emote_left_ = 0.0F;
    tower_charging_ = true;
    tower_charge_ = 0.0F;
    set_velocity(godot::Vector3());
    // The green that runs from her hand into the ground and out round her, while she charges.
    const godot::Node3D* const effect = E5Effect::spawn(effect_scene(tower_charge_path), this, get_global_position());
    tower_charge_effect_ = effect != nullptr ? effect->get_instance_id() : 0;
}

void E5PlayerController::update_tower_charge(bool held, float delta) {
    if (!tower_charging_) {
        return;
    }
    if (held && is_on_floor()) {
        tower_charge_ = std::min(tower_charge_ + delta, gameplay::tower_full_charge_seconds);
        return;
    }
    tower_charging_ = false;
    if (auto* const effect = godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(tower_charge_effect_))) {
        effect->queue_free();
    }
    tower_charge_effect_ = 0;
    const float power = tower_charge_ / gameplay::tower_full_charge_seconds;
    if (gameplay::tower_height(tower_charge_) <= 0.0F || !is_on_floor()) {
        // Let go too soon: nothing grows, and she can try again shortly.
        if (cooldowns_enabled_) {
            cooldown_left_.at(static_cast<std::size_t>(gameplay::SkillId::VineTower)) =
                gameplay::tower_cancel_cooldown_seconds;
        }
        return;
    }
    use_skill(power); // (the cooldown, the count, the word to the other players; then grow_tower)
}

void E5PlayerController::grow_tower(float power) {
    const float height = gameplay::tower_height(power * gameplay::tower_full_charge_seconds);
    const godot::Ref<godot::PackedScene> scene = effect_scene(tower_path);
    if (height <= 0.0F || scene.is_null()) {
        return;
    }
    auto* const tower = godot::Object::cast_to<godot::Node3D>(scene->instantiate());
    if (tower == nullptr) {
        return;
    }
    tower->set("height", height);
    tower->set("stand_seconds", gameplay::tower_stand_seconds);
    get_parent()->add_child(tower);
    tower->set_global_position(get_global_position());
    ++towers_grown_;
}

E5Enemy* E5PlayerController::pieces_victim() const {
    // One of those within reach, by chance. (On the ground: she cannot stand beside one that flies.)
    std::vector<E5Enemy*> near;
    const godot::TypedArray<godot::Node> enemies = get_tree()->get_nodes_in_group(E5Enemy::group_name);
    for (const godot::Variant& node : enemies) {
        auto* const enemy = godot::Object::cast_to<E5Enemy>(node);
        if (enemy == nullptr || !enemy->is_alive() || enemy->is_held() || !enemy->is_on_floor()) {
            continue;
        }
        godot::Vector3 to = enemy->get_global_position() - get_global_position();
        to.y = 0.0F;
        if (static_cast<float>(to.length()) - enemy->get_body_radius() <= gameplay::pieces_reach) {
            near.push_back(enemy);
        }
    }
    if (near.empty()) {
        return nullptr;
    }
    return near.at(static_cast<std::size_t>(godot::UtilityFunctions::randi() % near.size()));
}

void E5PlayerController::start_pieces(gameplay::SkillId skill) {
    spend(skill);
    last_skill_ = skill;
    ++skills_used_;
    action_skill_ = skill;
    emote_left_ = 0.0F;
    // The first blow at once, the others every half second after it.
    pieces_ = {.seconds_left = gameplay::pieces_seconds, .until_tick = 0.0F};
    note_net_event(0, 0.0F);
}

bool E5PlayerController::update_pieces(float delta) {
    if (!is_cutting()) {
        return false;
    }
    const gameplay::BleedStep step = gameplay::step_bleed(pieces_, gameplay::pieces_tick_seconds, std::max(delta, 1e-4F));
    pieces_ = step.state;
    if (remote_) {
        return false; // her place and her clips arrive from where she is played
    }
    for (int tick = 0; tick < step.ticks; ++tick) {
        E5Enemy* const victim = pieces_victim();
        if (victim == nullptr) {
            break;
        }
        // Beside it, on the side she comes from, facing it.
        godot::Vector3 from = get_global_position() - victim->get_global_position();
        from.y = 0.0F;
        const godot::Vector3 side = from.length() > 0.05F
                                        ? from.normalized()
                                        : godot::Vector3(-std::sin(model_yaw_), 0.0F, -std::cos(model_yaw_));
        const godot::Vector3 place =
            victim->get_global_position() + side * (victim->get_body_radius() + pieces_stand_off);
        // The streak she leaves from where she was to where she is.
        E5LightningArc::spawn(get_parent(), get_global_position() + godot::Vector3(0.0F, 1.1F, 0.0F),
                              place + godot::Vector3(0.0F, 1.1F, 0.0F), godot::Color(2.4F, 2.4F, 2.8F));
        set_global_position(place);
        model_yaw_ = gameplay::facing_yaw(-static_cast<float>(side.x), -static_cast<float>(side.z));
        if (model_ != nullptr) {
            model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
        }
        const godot::Vector3 body = victim->get_aim_point();
        combat::hit(victim, body, dealt(gameplay::SkillId::CutInPieces));
        E5Effect::spawn(effect_scene(counter_strike_path), get_parent(), body);
        shake_ = std::max(shake_, pieces_shake);
        // The blow: the combo's first two, turn about, begun anew each time.
        combo_step_ = pieces_blows_ % 2;
        ++pieces_blows_;
        show_slash_arc();
        if (animator_.is_ready() && animator_.has_clip(clip_combo_.front())) {
            animator_.set_upper(godot::StringName());
            animator_.set_base(clip_idle_, 1.0F, 0.0F);
            animator_.set_base(clip_combo_.at(static_cast<std::size_t>(combo_step_)), pieces_blow_pace, 0.04F);
        }
    }
    // She stands where the last blow put her.
    godot::Vector3 velocity = get_velocity();
    velocity.x = 0.0F;
    velocity.z = 0.0F;
    velocity.y = is_on_floor() ? 0.0F : velocity.y - params_.gravity * delta;
    set_velocity(velocity);
    move_and_slide();
    animator_.update(delta);
    if (!is_cutting()) {
        combo_step_ = -1;
    }
    return true;
}

void E5PlayerController::start_enrage(gameplay::SkillId skill) {
    spend(skill);
    last_skill_ = skill;
    ++skills_used_;
    action_skill_ = skill;
    emote_left_ = 0.0F;
    enrage_left_ = gameplay::enrage_seconds;
    enrage_blow_ = -1;
    enrage_next_blow_ = 0;
    // Her cry, and the red round her for as long as it lasts.
    E5Effect::spawn(effect_scene(enrage_path), this, get_global_position());
    note_net_event(0, 0.0F);
}

bool E5PlayerController::update_enrage(float delta) {
    const bool was_driving = enrage_driving_;
    enrage_driving_ = false;
    if (!is_enraged()) {
        enrage_blow_ = -1;
        return false;
    }
    enrage_left_ = std::max(enrage_left_ - delta, 0.0F);
    // Shown here for another player, or over; or something begun before still has her (a blow
    // of her own, a roll), or she is in the air or in the saddle: then it does not move her.
    const bool taken = action_.active || is_aiming() || block_.raised || dodge_left_ > 0.0F || is_whirling() ||
                       mounted_ || !is_on_floor() || !animator_.is_ready() || !animator_.has_clip(clip_combo_.front());
    E5Enemy* target = nullptr;
    if (!remote_ && is_enraged() && !taken) {
        float nearest = gameplay::enrage_reach;
        const godot::TypedArray<godot::Node> enemies = get_tree()->get_nodes_in_group(E5Enemy::group_name);
        for (const godot::Variant& node : enemies) {
            auto* const enemy = godot::Object::cast_to<E5Enemy>(node);
            if (enemy == nullptr || !enemy->is_alive() || enemy->is_held()) {
                continue;
            }
            godot::Vector3 to = enemy->get_global_position() - get_global_position();
            to.y = 0.0F;
            const float distance = static_cast<float>(to.length()) - enemy->get_body_radius();
            if (distance <= nearest) {
                nearest = distance;
                target = enemy;
            }
        }
    }
    if (target == nullptr) {
        // Nobody near enough: she is the player's to steer (and still faster on her feet).
        enrage_blow_ = -1;
        if (was_driving) {
            combo_step_ = -1;
        }
        return false;
    }
    enrage_driving_ = true;
    godot::Vector3 towards = target->get_global_position() - get_global_position();
    towards.y = 0.0F;
    const auto distance = static_cast<float>(towards.length());
    const godot::Vector3 forward =
        distance > 0.05F ? towards / distance : godot::Vector3(std::sin(model_yaw_), 0.0F, std::cos(model_yaw_));

    // The blows of her combo, one after another for as long as the enemy is within her sword's
    // reach: the same clips and moments, only faster.
    if (enrage_blow_ >= 0) {
        const SpellTiming& timing = combo_timings.at(static_cast<std::size_t>(enrage_blow_));
        const float pace = timing.playback_scale * gameplay::enrage_attack_speed;
        const godot::StringName& clip = clip_combo_.at(static_cast<std::size_t>(enrage_blow_));
        enrage_blow_elapsed_ += delta;
        if (!enrage_arc_shown_ &&
            enrage_blow_elapsed_ >= (timing.strike_at - look_of(gameplay::SkillId::Slash, enrage_blow_).before) / pace) {
            enrage_arc_shown_ = true;
            show_slash_arc();
        }
        if (!enrage_struck_ && enrage_blow_elapsed_ >= timing.strike_at / pace) {
            enrage_struck_ = true;
            strike_enrage(forward);
        }
        if (enrage_blow_elapsed_ >= std::min(timing.end_at, animator_.clip_length(clip)) / pace) {
            enrage_blow_ = -1;
        }
    }
    if (enrage_blow_ < 0 && distance <= plain_blow.reach + target->get_body_radius() + enrage_strike_gap) {
        enrage_blow_ = enrage_next_blow_;
        enrage_next_blow_ = (enrage_next_blow_ + 1) % gameplay::combo_length;
        enrage_blow_elapsed_ = 0.0F;
        enrage_struck_ = false;
        enrage_arc_shown_ = false;
        // (What draws the arc of a blow and her sword asks these.)
        action_skill_ = gameplay::SkillId::Enrage;
        combo_step_ = enrage_blow_;
        combo_idle_seconds_ = 0.0F;
    }
    const bool running = enrage_blow_ < 0;

    // Towards the enemy, as if the player steered her there; she stands for a blow.
    const gameplay::MotorParams params = motor_params();
    const gameplay::MotorInput motor_input{
        .move_right = 0.0F, .move_forward = running ? 1.0F : 0.0F, .sprint = false, .jump = false};
    const godot::Vector3 current = get_velocity() - combo_push_;
    combo_push_ = godot::Vector3();
    const gameplay::MotorState state{
        .velocity = {.x = static_cast<float>(current.x),
                     .y = static_cast<float>(current.y),
                     .z = static_cast<float>(current.z)},
        .on_floor = is_on_floor(),
    };
    // (The motor's "forward" is where a camera of this yaw would look.)
    const float steer_yaw = std::atan2(-static_cast<float>(forward.x), -static_cast<float>(forward.z));
    const gameplay::Vec3 next = gameplay::step_velocity(state, motor_input, steer_yaw, params, delta);
    set_velocity(godot::Vector3(next.x, next.y, next.z));
    move_and_slide();

    model_yaw_ = gameplay::turn_toward(
        model_yaw_, gameplay::facing_yaw(static_cast<float>(forward.x), static_cast<float>(forward.z)),
        turn_speed_ * delta);
    if (model_ != nullptr) {
        model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
    }
    animator_.set_upper(godot::StringName());
    if (running) {
        const godot::Vector3 resolved = get_velocity();
        const float speed = std::hypot(static_cast<float>(resolved.x), static_cast<float>(resolved.z));
        if (speed < gameplay::LocomotionThresholds{}.idle_below) {
            animator_.set_base(clip_idle_, 1.0F);
        } else {
            animator_.set_base(clip_run_, speed / run_clip_speed);
        }
    } else {
        animator_.set_base(clip_combo_.at(static_cast<std::size_t>(enrage_blow_)),
                           combo_timings.at(static_cast<std::size_t>(enrage_blow_)).playback_scale *
                               gameplay::enrage_attack_speed,
                           enrage_blow_fade);
    }
    animator_.update(delta);
    return true;
}

void E5PlayerController::strike_enrage(const godot::Vector3& forward) {
    ++enrage_blows_;
    const godot::Vector3 centre =
        get_global_position() + forward * plain_blow.reach + godot::Vector3(0.0F, melee_height, 0.0F);
    combat::blast(this, centre, plain_blow.radius,
                  dealt(gameplay::SkillId::Enrage) * gameplay::combo_damage_factor(enrage_blow_));
    shake_ = std::max(shake_, look_of(gameplay::SkillId::Slash, enrage_blow_).shake * enrage_shake_share);
    if (godot::Node3D* const impact =
            E5Effect::spawn(impact_effect(gameplay::SkillId::Slash), get_parent(),
                            get_global_position() + forward * plain_blow.reach + godot::Vector3(0.0F, 0.06F, 0.0F))) {
        impact->set_rotation(godot::Vector3(
            0.0F, gameplay::facing_yaw(static_cast<float>(forward.x), static_cast<float>(forward.z)), 0.0F));
    }
}

void E5PlayerController::update_leap_aim(bool held) {
    if (!leap_aiming_) {
        return;
    }
    // Something else has begun, or she can no longer leap: the aiming is over and nothing happens.
    const bool able = skills_.selected() == gameplay::SkillId::JumpAttack && !mounted_ && !is_busy() &&
                      is_on_floor() && skill_ready(skills_.selected());
    if (!held || !able) {
        leap_aiming_ = false;
        if (leap_marker_ != nullptr) {
            leap_marker_->set_visible(false);
        }
        if (able) {
            start_leap(skills_.selected());
        }
        return;
    }
    if (leap_marker_ == nullptr) {
        const godot::Ref<godot::PackedScene> scene = effect_scene(leap_marker_path);
        leap_marker_ = scene.is_valid() ? godot::Object::cast_to<godot::Node3D>(scene->instantiate()) : nullptr;
        if (leap_marker_ == nullptr) {
            return;
        }
        add_child(leap_marker_);
        leap_marker_->set_as_top_level(true); // placed in the world, not carried by her
    }
    // Where start_leap would take her now: what the crosshair covers, or as far as she can leap that way.
    const godot::Vector3 feet = get_global_position();
    const AimPoint aim = find_aim_point(feet + godot::Vector3(0.0F, point_blank_height, 0.0F));
    godot::Vector3 way = aim.position - feet;
    way.y = 0.0F;
    const auto length = static_cast<float>(way.length());
    godot::Vector3 place = aim.position;
    if (!aim.hit || length > gameplay::leap_max_distance) {
        // Beyond her leap: the mark at its end, at the height she stands on (the ground there is not known).
        place = feet + (length > 0.01F ? way / length : godot::Vector3()) * gameplay::leap_max_distance;
    }
    leap_marker_->set_global_position(place);
    leap_marker_->set_visible(true);
}

void E5PlayerController::start_leap(gameplay::SkillId skill) {
    spend(skill);
    last_skill_ = skill;
    ++skills_used_;
    action_skill_ = skill;
    // To what the crosshair covers; with nothing under it, as far as she can leap that way.
    const godot::Vector3 feet = get_global_position();
    godot::Vector3 way = find_aim_point(feet + godot::Vector3(0.0F, point_blank_height, 0.0F)).position - feet;
    way.y = 0.0F;
    const float distance = std::min(static_cast<float>(way.length()), gameplay::leap_max_distance);
    leap_ = {};
    leap_.flying = true;
    leap_.arc = gameplay::leap_arc(distance);
    leap_.rise = leap_.arc.rise_speed;
    leap_.direction =
        distance > 0.3F ? way.normalized() : godot::Vector3(std::sin(model_yaw_), 0.0F, std::cos(model_yaw_));
    model_yaw_ = gameplay::facing_yaw(static_cast<float>(leap_.direction.x), static_cast<float>(leap_.direction.z));
    if (model_ != nullptr) {
        model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
    }
    animator_.set_upper(godot::StringName());
    animator_.set_base(clip_jump_, 1.0F);
    note_net_event(0, 0.0F);
}

void E5PlayerController::update_leap(float delta) {
    if (!leap_.flying) {
        // On the ground after the blow: she gathers herself where she landed.
        leap_.recover_left = std::max(leap_.recover_left - delta, 0.0F);
        godot::Vector3 velocity = get_velocity();
        velocity.x = 0.0F;
        velocity.z = 0.0F;
        velocity.y = is_on_floor() ? 0.0F : velocity.y - params_.gravity * delta;
        set_velocity(velocity);
        move_and_slide();
        return;
    }
    leap_.elapsed += delta;
    leap_.rise -= leap_.arc.gravity * delta;
    set_velocity(leap_.direction * leap_.arc.forward_speed + godot::Vector3(0.0F, leap_.rise, 0.0F));
    move_and_slide();
    // The blow's clip (the combo's finisher, which comes down out of the air) begins so that
    // the sword strikes as she lands.
    if (!leap_.striking && leap_.elapsed >= leap_.arc.seconds - leap_strike_lead &&
        animator_.has_clip(clip_combo_.back())) {
        leap_.striking = true;
        animator_.set_base(clip_combo_.back(), 1.0F);
    }
    const bool landed = leap_.elapsed > leap_min_air_seconds && leap_.rise <= 0.0F && is_on_floor();
    if (landed || leap_.elapsed > leap_.arc.seconds + leap_longest_fall) {
        land_leap();
    }
}

void E5PlayerController::land_leap() {
    leap_.flying = false;
    leap_.recover_left = gameplay::leap_recover_seconds;
    set_velocity(godot::Vector3());
    const godot::Vector3 feet = get_global_position();
    combat::blast(this, feet + godot::Vector3(0.0F, melee_height, 0.0F), gameplay::leap_radius,
                  dealt(gameplay::SkillId::JumpAttack));
    shake_ = std::max(shake_, leap_shake);
    E5Effect::spawn(effect_scene(leap_impact_path), get_parent(), feet + godot::Vector3(0.0F, 0.06F, 0.0F));
    // Whatever stood there staggers on more slowly for a while.
    const godot::TypedArray<godot::Node> enemies = get_tree()->get_nodes_in_group(E5Enemy::group_name);
    for (const godot::Variant& node : enemies) {
        auto* const enemy = godot::Object::cast_to<E5Enemy>(node);
        if (enemy == nullptr || !enemy->is_alive()) {
            continue;
        }
        if (enemy->get_aim_point().distance_to(feet + godot::Vector3(0.0F, melee_height, 0.0F)) <=
            gameplay::leap_radius + enemy->get_body_radius()) {
            enemy->slow(gameplay::leap_slow_share, gameplay::leap_slow_seconds);
        }
    }
}

void E5PlayerController::strike_seismic() {
    // Where the player looks, like every other skill.
    const godot::Vector3 ahead(-std::sin(look_.yaw), 0.0F, -std::cos(look_.yaw));
    const godot::Vector3 right(-ahead.z, 0.0F, ahead.x);
    const godot::Vector3 feet = get_global_position();
    shake_ = std::max(shake_, seismic_shake);
    E5Effect::spawn(effect_scene(seismic_crack_path), get_parent(), feet + ahead * seismic_first_row);

    // The rows of bursts: further out, wider and larger, one after another.
    const float step = (gameplay::seismic_length - seismic_first_row) / static_cast<float>(seismic_rows - 1);
    for (int row = 0; row < seismic_rows; ++row) {
        const float out = seismic_first_row + step * static_cast<float>(row);
        const float half_width = out * std::tan(gameplay::seismic_half_angle) * 0.8F;
        const int count = row + 2;
        for (int index = 0; index < count; ++index) {
            const float across =
                count == 1 ? 0.0F : (static_cast<float>(index) / static_cast<float>(count - 1) - 0.5F) * 2.0F;
            eruptions_.push_back(
                {.in_seconds = seismic_row_seconds * static_cast<float>(row),
                 .place = feet + ahead * out + right * (across * half_width) + godot::Vector3(0.0F, 0.06F, 0.0F),
                 .size = seismic_first_size * (1.0F + static_cast<float>(row) / static_cast<float>(seismic_rows - 1))});
        }
    }

    // What stands in the wedge is hit and stunned. (A hero shown here for another player hurts
    // and stuns nobody: that is decided where she is played.)
    const float damage = dealt(gameplay::SkillId::SeismicSlash);
    if (remote_) {
        return;
    }
    const godot::TypedArray<godot::Node> enemies = get_tree()->get_nodes_in_group(E5Enemy::group_name);
    for (const godot::Variant& node : enemies) {
        auto* const enemy = godot::Object::cast_to<E5Enemy>(node);
        if (enemy == nullptr || !enemy->is_alive() || enemy->is_held()) {
            continue;
        }
        const godot::Vector3 to = enemy->get_global_position() - feet;
        if (std::abs(static_cast<float>(to.y)) > 6.0F ||
            !gameplay::in_wedge(static_cast<float>(to.x), static_cast<float>(to.z), static_cast<float>(ahead.x),
                                static_cast<float>(ahead.z), gameplay::seismic_length, gameplay::seismic_half_angle,
                                gameplay::seismic_near, enemy->get_body_radius())) {
            continue;
        }
        combat::hit(enemy, enemy->get_aim_point(), damage);
        enemy->stun(gameplay::seismic_stun_seconds);
        // Over its head for as long as it stands stunned; it goes with the enemy.
        E5Effect::spawn(effect_scene(seismic_stun_path), enemy,
                        enemy->get_aim_point() + godot::Vector3(0.0F, enemy->get_body_radius() + 0.7F, 0.0F));
    }
}

void E5PlayerController::update_eruptions(float delta) {
    if (eruptions_.empty()) {
        return;
    }
    for (Eruption& eruption : eruptions_) {
        eruption.in_seconds -= delta;
        if (eruption.in_seconds <= 0.0F) {
            if (godot::Node3D* const burst =
                    E5Effect::spawn(effect_scene(seismic_burst_path), get_parent(), eruption.place)) {
                burst->set_scale(godot::Vector3(eruption.size, eruption.size, eruption.size));
            }
        }
    }
    std::erase_if(eruptions_, [](const Eruption& eruption) { return eruption.in_seconds <= 0.0F; });
}

void E5PlayerController::cast_black_hole() {
    const godot::Vector3 feet = get_global_position();
    const AimPoint aim = find_aim_point(feet);
    godot::Vector3 place = aim.position;
    if (!aim.hit || place.distance_to(feet) > black_hole_range) {
        place = feet + (place - feet).normalized() * black_hole_range;
    }
    auto* const hole = memnew(E5BlackHole);
    hole->configure(black_hole_effect_, black_hole_burst_effect_, remote_ ? 0.0F : black_hole_tick,
                    dealt(gameplay::SkillId::BlackHole));
    get_parent()->add_child(hole);
    hole->set_global_position(place + godot::Vector3(0.0F, black_hole_height, 0.0F));
}

void E5PlayerController::cast_star_barrage() {
    const godot::Vector3 forward(-std::sin(look_.yaw), 0.0F, -std::cos(look_.yaw));
    const godot::Vector3 right(-forward.z, 0.0F, forward.x);
    const godot::Vector3 above = get_global_position() + godot::Vector3(0.0F, 2.5F, 0.0F);
    const godot::Vector3 target = find_aim_point(above).position;
    for (int index = 0; index < star_count; ++index) {
        // A fan above his head, from one side to the other; the stars go one after another.
        const float side = gameplay::fan_yaw_offset(index, star_count, 2.0F);
        auto* const star = memnew(E5SpellBolt);
        get_parent()->add_child(star);
        star->set_global_position(above + right * (side * 0.9F) +
                                  godot::Vector3(0.0F, 0.3F * (1.0F - std::abs(side)), 0.0F));
        star->set_damage(dealt(gameplay::SkillId::StarBarrage));
        star->set_trail_effect(star_trail_effect_);
        star->set_impact_effect(star_impact_effect_);
        star->set_homing(target, star_speed, star_turn);
        star->set_launch_delay(star_interval * static_cast<float>(index));
        star->launch((godot::Vector3(0.0F, 1.0F, 0.0F) + right * side - forward * 0.35F).normalized() * star_rise_speed,
                     get_rid());
    }
}

godot::Vector3 E5PlayerController::raised_hand_position() const {
    if (right_hand_ == nullptr) {
        return get_global_position() + godot::Vector3(0.0F, 2.0F, 0.0F);
    }
    // The palm, a little beyond the wrist bone.
    return right_hand_->get_global_transform().xform(godot::Vector3(0.0F, palm_from_wrist, 0.0F));
}

void E5PlayerController::update_summon(float delta) {
    // The bird on her hand: it appears, sits upright facing where she looks, and opens its wings.
    if (auto* const bird = godot::Object::cast_to<E5Bird>(godot::ObjectDB::get_instance(perched_bird_id_))) {
        const float t = action_.elapsed * summon_playback_scale; // seconds into the clip
        const auto ramp = [t](float from, float to) {
            const float x = std::clamp((t - from) / (to - from), 0.0F, 1.0F);
            return x * x * (3.0F - 2.0F * x);
        };
        const float size = std::max(ramp(perch_appears_at, perch_appears_at + 0.3F), 0.001F);
        bird->set_wing_spread(ramp(perch_spreads_from, perch_spreads_until));
        // The bird scene tilts the model forward for flight; sitting, it leans back by the same angle.
        const godot::Basis facing(godot::Quaternion(godot::Vector3(0.0F, 1.0F, 0.0F), model_yaw_) *
                                  godot::Quaternion(godot::Vector3(1.0F, 0.0F, 0.0F), -perch_lean_back));
        bird->set_global_transform(
            godot::Transform3D(facing, raised_hand_position() + godot::Vector3(0.0F, perch_height * size, 0.0F)));
        bird->set_scale(godot::Vector3(size, size, size));
    }

    // The rest of the flock follows out of her hand, one after another.
    if (birds_to_release_ > 0) {
        next_bird_seconds_ -= delta;
        if (next_bird_seconds_ <= 0.0F) {
            next_bird_seconds_ = flock_interval_seconds;
            const int index = summoned_bird_count - birds_to_release_;
            --birds_to_release_;
            if (auto* const bird = godot::Object::cast_to<E5Bird>(bird_scene_->instantiate())) {
                bird->set_harmless(remote_);
                get_parent()->add_child(bird);
                launch_bird(bird, index);
                E5Effect::spawn(summon_burst_effect_, get_parent(), raised_hand_position());
            }
        }
    }
}

void E5PlayerController::launch_bird(E5Bird* bird, int index) {
    const godot::Vector3 forward(-std::sin(look_.yaw), 0.0F, -std::cos(look_.yaw));
    const godot::Vector3 right(-forward.z, 0.0F, forward.x);
    // Thrown up and outward in a fan, alternating sides.
    const float spread = gameplay::fan_yaw_offset(index, summoned_bird_count, 2.0F);
    const godot::Vector3 direction = (godot::Vector3(0.0F, 1.0F, 0.0F) + forward * 0.5F + right * spread).normalized();
    bird->set_scale(godot::Vector3(1.0F, 1.0F, 1.0F));
    bird->launch(raised_hand_position() + godot::Vector3(0.0F, perch_height, 0.0F),
                 direction * summoned_bird_launch_speed, index % 2 == 0 ? 1.0F : -1.0F, index, this);
}

void E5PlayerController::release_birds() {
    E5Effect::spawn(summon_burst_effect_, get_parent(), raised_hand_position());
    // The bird on her hand goes first; the others follow from update_summon.
    int released = 0;
    if (auto* const bird = godot::Object::cast_to<E5Bird>(godot::ObjectDB::get_instance(perched_bird_id_))) {
        launch_bird(bird, 0);
        released = 1;
    }
    perched_bird_id_ = 0;
    if (bird_scene_.is_valid()) {
        birds_to_release_ = summoned_bird_count - released;
        next_bird_seconds_ = flock_interval_seconds;
    }
}

void E5PlayerController::strike_kick() {
    // The kick goes where the player looks, like every other skill.
    const godot::Vector3 forward(-std::sin(look_.yaw), 0.0F, -std::cos(look_.yaw));
    const godot::Vector3 ground = get_global_position() + forward * kick_reach;
    if (godot::Node3D* const effect = E5Effect::spawn(kick_effect_, get_parent(), ground)) {
        effect->set_rotation(godot::Vector3(0.0F, gameplay::facing_yaw(forward.x, forward.z), 0.0F));
    }
    combat::blast(this, ground + godot::Vector3(0.0F, kick_height, 0.0F), kick_radius,
                  dealt(gameplay::SkillId::ThunderKick));
    // Lightning leaps from her foot across the ground in a fan.
    const godot::Vector3 foot = get_global_position() + forward * 0.5F + godot::Vector3(0.0F, kick_height, 0.0F);
    for (int index = 0; index < kick_bolt_count; ++index) {
        const float angle = gameplay::fan_yaw_offset(index, kick_bolt_count, kick_bolt_spread);
        const godot::Vector3 direction = forward.rotated(godot::Vector3(0.0F, 1.0F, 0.0F), angle);
        E5LightningArc::spawn(get_parent(), foot,
                              get_global_position() + direction * kick_bolt_reach + godot::Vector3(0.0F, 0.1F, 0.0F),
                              godot::Color(1.6F, 0.7F, 3.0F));
    }
}

void E5PlayerController::fire_arrow(float power) {
    const godot::Vector3 spawn = bow_string_->to_global(bow_string_->get_nock_position());
    const godot::Vector3 direction = (find_aim_point(spawn).position - spawn).normalized();
    E5Arrow* const arrow = spawn_arrow(spawn, direction);
    arrow->set_damage(dealt(skills_.selected(), power));
    if (power > 0.0F) {
        arrow->set_power(power, trail_effect_, impact_effect_);
    }
    // A fully charged arrow leaves the bow considerably faster and so flies flatter.
    arrow->launch(direction * arrow_speed_ * (1.0F + power_shot_speed_bonus * power), get_rid());
}

void E5PlayerController::update_rain_marker() {
    const bool wanted = is_aiming() && bow_.phase != gameplay::BowPhase::Releasing &&
                        skills_.selected() == gameplay::SkillId::ArrowRain;
    rain_target_valid_ = false;
    if (wanted) {
        const AimPoint aim = find_aim_point(get_global_position());
        if (aim.hit && aim.position.distance_to(get_global_position()) <= rain_max_range) {
            rain_target_ = aim.position;
            rain_target_valid_ = true;
        }
    }
    if (rain_marker_ != nullptr) {
        rain_marker_->set_visible(rain_target_valid_);
        if (rain_target_valid_) {
            rain_marker_->set_global_position(rain_target_);
        }
    }
}

void E5PlayerController::fire_rain() {
    // The marker was placed during the last step; nothing under the crosshair means no volley.
    if (!rain_target_valid_) {
        return;
    }
    // One glowing arrow into the sky, as the visible start of the volley.
    const godot::Vector3 spawn = bow_string_->to_global(bow_string_->get_nock_position());
    godot::Vector3 skyward = rain_target_ - spawn;
    skyward.y = 0.0F;
    skyward = (skyward.normalized() * 0.35F + godot::Vector3(0.0F, 1.0F, 0.0F)).normalized();
    auto* const signal_arrow = memnew(E5Arrow);
    get_parent()->add_child(signal_arrow);
    signal_arrow->set_global_transform(
        godot::Transform3D(godot::Basis::looking_at(skyward, godot::Vector3(1.0F, 0.0F, 0.0F)), spawn));
    signal_arrow->set_power(1.0F, trail_effect_, godot::Ref<godot::PackedScene>());
    signal_arrow->set_flight_lifetime(rain_signal_arrow_seconds);
    signal_arrow->launch(skyward * rain_signal_arrow_speed, get_rid());

    auto* const rain = memnew(E5ArrowRain);
    rain->configure(get_rid(), rain_marker_effect_, rain_impact_effect_);
    rain->set_harmless(remote_);
    rain->set_damage(dealt(gameplay::SkillId::ArrowRain));
    get_parent()->add_child(rain);
    rain->set_global_position(rain_target_);
}

void E5PlayerController::select_skill(int slot) {
    // Not while the bow is raised: the skill in use must not change under the player's hands.
    // Nor while the left button has the bar on its first slot.
    if (slot < 0 || is_busy() || use_button_ == UseButton::Standard ||
        !skills_.select(static_cast<std::size_t>(slot))) {
        return;
    }
    if (skill_hud_ != nullptr) {
        skill_hud_->set_selected(skills_.selected_index());
    }
}
void E5PlayerController::set_camera_yaw(float radians) {
    look_.yaw = radians;
    apply_look_to_nodes();
}

void E5PlayerController::set_camera_pitch(float radians) {
    look_.pitch = radians;
    apply_look_to_nodes();
}

godot::String E5PlayerController::get_current_animation() const {
    const godot::String clip = animator_.base_clip();
    return clip;
}

void E5PlayerController::_unhandled_input(const godot::Ref<godot::InputEvent>& event) {
    if (remote_) {
        return;
    }
    godot::Input* const input = godot::Input::get_singleton();
    const bool captured = input->get_mouse_mode() == godot::Input::MOUSE_MODE_CAPTURED;

    // A menu is open: everything belongs to it.
    if (input_blocked_) {
        return;
    }
    if (inventory_ != nullptr && event->is_action_pressed(action_use_potion_)) {
        inventory_->use_potion();
        return;
    }

    const godot::Ref<godot::InputEventMouseMotion> motion = event;
    if (motion.is_valid()) {
        if (captured) {
            const godot::Vector2 relative = motion->get_relative();
            look_ = gameplay::apply_look(look_, static_cast<float>(relative.x), static_cast<float>(relative.y),
                                         mouse_sensitivity_);
            apply_look_to_nodes();
        }
        return;
    }

    if ((archery_enabled_ || spells_enabled_) && event->is_pressed() && !event->is_echo()) {
        for (int slot = 0; slot < actions::skill_slot_count; ++slot) {
            if (event->is_action(skill_actions_.at(static_cast<std::size_t>(slot)))) {
                // The dagger is used from its key, whatever the setting, and the bar remembers
                // what was chosen before it.
                const bool dagger = skills_.slot(static_cast<std::size_t>(slot)) == gameplay::SkillId::DaggerCombo;
                if (dagger && skills_.selected() != gameplay::SkillId::DaggerCombo) {
                    dagger_return_slot_ = static_cast<int>(skills_.selected_index());
                }
                select_skill(slot);
                // Quick cast: if the skill was taken, the key now counts as the right button.
                if ((quick_cast_ || dagger) && skills_.selected_index() == static_cast<std::size_t>(slot) &&
                    !is_busy()) {
                    quick_key_slot_ = slot;
                }
                return;
            }
        }
    }

    // Esc belongs to the interface (the pause menu). Without one it releases the mouse;
    // clicking the window takes it back.
    if (interface_ == nullptr && captured && event->is_action_pressed("ui_cancel")) {
        input->set_mouse_mode(godot::Input::MOUSE_MODE_VISIBLE);
        return;
    }
    const godot::Ref<godot::InputEventMouseButton> button = event;
    if (!captured && button.is_valid() && button->is_pressed()) {
        input->set_mouse_mode(godot::Input::MOUSE_MODE_CAPTURED);
    }
}

void E5PlayerController::apply_look_to_nodes() {
    // The camera orbits the character; the body itself never rotates, so its
    // collision shape and the movement basis stay independent of the view.
    if (camera_pivot_ != nullptr) {
        camera_pivot_->set_rotation(godot::Vector3(look_.pitch, look_.yaw, 0.0F));
    }
}

} // namespace e5::bridge
