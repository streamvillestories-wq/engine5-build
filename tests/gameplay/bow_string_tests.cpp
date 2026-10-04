#include "e5/gameplay/bow_string.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using namespace e5::gameplay;

namespace {
constexpr Vec3 top{.x = 0.1F, .y = 1.0F, .z = 0.0F};
constexpr Vec3 bottom{.x = 0.1F, .y = 0.0F, .z = 0.0F};
constexpr Vec3 toward_archer{.x = 1.0F, .y = 0.0F, .z = 0.0F};
} // namespace

TEST_CASE("an undrawn string is straight between its anchors", "[bow]") {
    const BowStringShape shape = bow_string_shape(top, bottom, toward_archer, 0.5F, 0.0F);
    CHECK(shape.nock.x == Approx(0.1F));
    CHECK(shape.nock.y == Approx(0.5F));
    CHECK(shape.nock.z == Approx(0.0F));
    CHECK(shape.top.y == Approx(1.0F));
    CHECK(shape.bottom.y == Approx(0.0F));
}

TEST_CASE("drawing moves the nock point along the pull direction", "[bow]") {
    CHECK(bow_string_shape(top, bottom, toward_archer, 0.5F, 1.0F).nock.x == Approx(0.6F));
    CHECK(bow_string_shape(top, bottom, toward_archer, 0.5F, 0.5F).nock.x == Approx(0.35F));
}

TEST_CASE("the pull direction is normalised", "[bow]") {
    const Vec3 long_direction{.x = 4.0F, .y = 0.0F, .z = 0.0F};
    CHECK(bow_string_shape(top, bottom, long_direction, 0.5F, 1.0F).nock.x == Approx(0.6F));
}

TEST_CASE("draw is clamped to 0..1", "[bow]") {
    CHECK(bow_string_shape(top, bottom, toward_archer, 0.5F, 3.0F).nock.x == Approx(0.6F));
    CHECK(bow_string_shape(top, bottom, toward_archer, 0.5F, -1.0F).nock.x == Approx(0.1F));
}

TEST_CASE("a zero pull direction leaves the string at rest", "[bow]") {
    const BowStringShape shape = bow_string_shape(top, bottom, Vec3{}, 0.5F, 1.0F);
    CHECK(shape.nock.x == Approx(0.1F));
    CHECK(shape.nock.y == Approx(0.5F));
}
