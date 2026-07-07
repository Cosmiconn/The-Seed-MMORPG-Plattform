#include <catch2/catch_test_macros.hpp>
#include "core/material.hpp"

using namespace TheSeed::Render;

TEST_CASE("Material: Library Register/Get", "[material]") {
    MaterialLibrary lib;
    auto mat = MaterialLibrary::RedPlastic();
    uint32_t id = lib.RegisterMaterial("red_plastic", mat);

    REQUIRE(id != 0);
    REQUIRE(lib.Count() == 1);

    auto* retrieved = lib.GetMaterial(id);
    REQUIRE(retrieved != nullptr);
    REQUIRE(retrieved->pbr.albedo[0] == 1.0f);
    REQUIRE(retrieved->pbr.metallic == 0.0f);
    REQUIRE(retrieved->pbr.roughness == 0.3f);
}

TEST_CASE("Material: Library GetByName", "[material]") {
    MaterialLibrary lib;
    lib.RegisterMaterial("gold", MaterialLibrary::Gold());

    auto* gold = lib.GetMaterial("gold");
    REQUIRE(gold != nullptr);
    REQUIRE(gold->pbr.metallic == 1.0f);
    REQUIRE(gold->pbr.roughness == 0.15f);
}

TEST_CASE("Material: Predefined Materials", "[material]") {
    auto red = MaterialLibrary::RedPlastic();
    REQUIRE(red.pbr.albedo[0] == 1.0f);
    REQUIRE(red.pbr.metallic == 0.0f);

    auto gold = MaterialLibrary::Gold();
    REQUIRE(gold.pbr.albedo[0] == 1.0f);
    REQUIRE(gold.pbr.albedo[1] == 0.78f);
    REQUIRE(gold.pbr.metallic == 1.0f);

    auto silver = MaterialLibrary::Silver();
    REQUIRE(silver.pbr.metallic == 1.0f);
    REQUIRE(silver.pbr.roughness == 0.1f);
}

TEST_CASE("Material: Update", "[material]") {
    MaterialLibrary lib;
    uint32_t id = lib.RegisterMaterial("test", MaterialLibrary::RedPlastic());

    Material updated = MaterialLibrary::Gold();
    lib.SetMetallic(id, updated.pbr.metallic);
    lib.SetRoughness(id, updated.pbr.roughness);

    auto* mat = lib.GetMaterial(id);
    REQUIRE(mat->pbr.metallic == 1.0f);
}

TEST_CASE("Material: Remove", "[material]") {
    MaterialLibrary lib;
    uint32_t id = lib.RegisterMaterial("remove_me", MaterialLibrary::BlueRubber());
    REQUIRE(lib.Count() == 1);

    lib.RemoveMaterial(id);
    REQUIRE(lib.Count() == 0);
    REQUIRE(lib.GetMaterial(id) == nullptr);
}
