#include <catch2/catch_test_macros.hpp>
#include "core/material.hpp"

using namespace TheSeed::Render;

TEST_CASE("MaterialLibrary: RedPlastic") {
    auto m = MaterialLibrary::RedPlastic();
    CHECK_THAT(m.pbr.albedo[0], WithinAbs(1.0f, 0.001f));
    CHECK_THAT(m.pbr.albedo[1], WithinAbs(0.0f, 0.001f));
    CHECK_THAT(m.pbr.albedo[2], WithinAbs(0.0f, 0.001f));
    CHECK(m.pbr.metallic == 0.0f);
    CHECK_THAT(m.pbr.roughness, WithinAbs(0.3f, 0.001f));
}

TEST_CASE("MaterialLibrary: GreenMetal") {
    auto m = MaterialLibrary::GreenMetal();
    CHECK_THAT(m.pbr.albedo[1], WithinAbs(0.7f, 0.001f));
    CHECK_THAT(m.pbr.metallic, WithinAbs(0.9f, 0.001f));
}

TEST_CASE("MaterialLibrary: Gold") {
    auto m = MaterialLibrary::Gold();
    CHECK_THAT(m.pbr.albedo[0], WithinAbs(1.0f, 0.001f));
    CHECK(m.pbr.metallic == 1.0f);
    CHECK_THAT(m.pbr.roughness, WithinAbs(0.15f, 0.001f));
}

TEST_CASE("MaterialLibrary: Silver") {
    auto m = MaterialLibrary::Silver();
    CHECK_THAT(m.pbr.albedo[0], WithinAbs(0.97f, 0.001f));
    CHECK(m.pbr.metallic == 1.0f);
}

TEST_CASE("MaterialLibrary: RustyMetal") {
    auto m = MaterialLibrary::RustyMetal();
    CHECK_THAT(m.pbr.albedo[0], WithinAbs(0.72f, 0.001f));
    CHECK_THAT(m.pbr.roughness, WithinAbs(0.65f, 0.001f));
}

TEST_CASE("MaterialLibrary: EmissiveRed") {
    auto m = MaterialLibrary::EmissiveRed();
    CHECK_THAT(m.pbr.emissive[0], WithinAbs(1.0f, 0.001f));
    CHECK_THAT(m.pbr.emissiveStrength, WithinAbs(2.0f, 0.001f));
}

TEST_CASE("MaterialLibrary: Glass") {
    auto m = MaterialLibrary::Glass();
    CHECK_THAT(m.pbr.roughness, WithinAbs(0.05f, 0.001f));
    CHECK(m.transparent == true);
}

TEST_CASE("MaterialLibrary: BlueRubber") {
    auto m = MaterialLibrary::BlueRubber();
    CHECK_THAT(m.pbr.albedo[2], WithinAbs(0.85f, 0.001f));
    CHECK(m.pbr.metallic == 0.0f);
    CHECK_THAT(m.pbr.roughness, WithinAbs(0.9f, 0.001f));
}
