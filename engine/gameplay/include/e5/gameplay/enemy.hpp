#pragma once

namespace e5::gameplay {

// Behaviour of a simple enemy, as a small state machine over plain data. It fights
// hand to hand; with a `cast_range` it also throws magic from a distance.
//
//   Idle    stands until the player comes within `aggro_range` or it is hurt
//   Chase   walks at the player; within `attack_range` it attacks when ready
//   Attack  plays its attack for `attack_seconds`, then chases again
//   Cast    stands and throws a spell: it leaves the hand part-way through, and a
//           heavy blow before that moment stops it
//   Hit     staggered by a heavy blow; cannot act for `hit_seconds`
//   Dead    health used up; stays dead until whoever owns it resets the state
//   Special a boss's great attack: with a `special_range` it stands and does one every
//           `special_cooldown_seconds` while a player is that near, before anything else. What
//           the attack is, is its owner's business; they are counted so the owner can take turns.
// New phases go at the end: the number is what is sent between machines.
enum class EnemyPhase : unsigned char { Idle, Chase, Attack, Cast, Hit, Dead, Special };

struct EnemyState {
    EnemyPhase phase = EnemyPhase::Idle;
    float phase_seconds = 0.0F; // time spent in the current phase
    float health = 100.0F;
    bool aggro = false;                    // has noticed the player
    float attack_cooldown_seconds = 0.0F;  // until it may attack again
    float cast_cooldown_seconds = 0.0F;    // until it may cast again
    bool cast_released = false;            // the spell of the cast in progress has left the hand
    float special_cooldown_seconds = 0.0F; // until its next great attack; runs only once it has noticed someone
    int specials_started = 0;              // how many great attacks it has begun
};

struct EnemyParams {
    float max_health = 100.0F;
    float aggro_range = 5.0F;    // metres
    float attack_range = 1.3F;   // metres
    float give_up_range = 25.0F; // metres; beyond this it loses interest
    float attack_seconds = 1.0F;
    float attack_cooldown_seconds = 0.6F; // rest between two attacks
    float hit_seconds = 0.45F;
    float stagger_damage = 10.0F; // a single blow of at least this much staggers it
    float cast_range = 0.0F;      // metres; it casts at a player nearer than this. 0 = it has no magic
    float cast_min_range = 3.5F;  // metres; nearer than this it comes to blows instead
    float cast_seconds = 1.5F;
    float cast_release_share = 0.55F;   // of cast_seconds: when the spell leaves the hand
    float cast_cooldown_seconds = 5.0F; // between two casts; it keeps chasing meanwhile
    // Metres it likes to stay away from the player: it comes no nearer, and backs off when the
    // player comes much nearer. 0 = it walks right up (a brawler).
    float keep_distance = 0.0F;
    float special_range = 0.0F;             // metres; 0 = it has no great attacks
    float special_seconds = 2.0F;           // how long the one in progress takes
    float special_cooldown_seconds = 10.0F; // between two of them
    float special_first_share = 0.4F;       // of the cooldown: the wait before the first, once it fights
};

struct EnemyInput {
    bool has_player = false;
    float distance_to_player = 0.0F; // metres; meaningful only with has_player
    float damage = 0.0F;             // total damage received since the last step
    float heaviest_blow = 0.0F;      // largest single part of that damage
};

struct EnemyStep {
    EnemyState state;
    bool moving = false;          // wants to walk toward the player during this step
    bool retreating = false;      // wants to back away from the player during this step
    bool attack_started = false;  // true on the step an attack begins
    bool cast_started = false;    // true on the step it starts to cast
    bool cast_released = false;   // true on the step the spell leaves its hand
    bool died = false;            // true on the step its health ran out
    bool special_started = false; // true on the step a great attack begins
};

[[nodiscard]] EnemyState spawn_enemy(const EnemyParams& params) noexcept;

[[nodiscard]] EnemyStep step_enemy(const EnemyState& state, const EnemyInput& input, const EnemyParams& params,
                                   float delta_seconds) noexcept;

} // namespace e5::gameplay