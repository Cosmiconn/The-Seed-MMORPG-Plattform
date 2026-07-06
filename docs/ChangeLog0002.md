# ChangeLog 0002 – Woche 2: ECS-Prototyp

**Datum:** 2026-07-01  
**Version:** 0.1.1 → 0.1.2

## Hinzugefuegt
- Archetype-basiertes ECS (EnTT-inspiriert)
- Entity = 64-Bit (32-Bit ID + 32-Bit Version)
- Component-Constraint: trivially_copyable + standard_layout
- SoA-Storage (Structure of Arrays) pro Archetype
- Query-System mit all/any/none Filter
- Entity Version Recycling

## API
```cpp
World world;
auto e = world.CreateEntity();
world.AddComponent<Transform>(e, 1.0f, 2.0f, 3.0f);
auto results = world.Query<Transform, Health>();
```

## Tests
- Entity Create/Destroy
- Component Add/Get/Remove
- Query Filter
- Entity Version Recycling

## Dateien
- `src/core/ecs.hpp`
- `src/core/ecs.cpp`
- `tests/test_ecs.cpp`
