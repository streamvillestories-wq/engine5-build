#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace godot {
class Sprite3D;
} // namespace godot

namespace e5::bridge {

// Loot lying on the ground: an item stack or gold. It hops out of whatever
// dropped it, hovers with its icon and a beam of light in its rarity's colour,
// and flies to the player who comes near, if there is room in the bag.
// Built in code; made with `spawn`, or placed in a scene with `item`/`count`/`gold` set.
class E5Pickup : public godot::Node3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5Pickup, godot::Node3D)

public:
    static constexpr const char* group_name = "e5_pickup";

    // `from`: where it appears; `to`: the spot on the ground it hops to.
    static E5Pickup* spawn(godot::Node* parent, const godot::Vector3& from, const godot::Vector3& to, int item,
                           int count, int gold);

    void _ready() override;
    void _process(double delta) override;

    void set_item(int item) { item_ = item; }
    [[nodiscard]] int get_item() const { return item_; }
    void set_count(int count) { count_ = count; }
    [[nodiscard]] int get_count() const { return count_; }
    void set_gold(int gold) { gold_ = gold; }
    [[nodiscard]] int get_gold() const { return gold_; }

    // For what the player threw away: it stays until the player has walked off once.
    void wait_until_player_left() { wait_for_leave_ = true; }

protected:
    static void _bind_methods();

private:
    enum class Phase : std::uint8_t { Hop, Rest, Home };

    void build();
    void collect(godot::Node3D& player);

    int item_ = 0;
    int count_ = 1;
    int gold_ = 0;
    Phase phase_ = Phase::Rest;
    godot::Vector3 hop_from_;
    godot::Vector3 hop_to_;
    float phase_seconds_ = 0.0F;
    float age_seconds_ = 0.0F;
    float retry_seconds_ = 0.0F; // after finding the bag full
    float home_speed_ = 0.0F;
    bool wait_for_leave_ = false;
    godot::Sprite3D* icon_ = nullptr; // non-owning child
};

} // namespace e5::bridge
