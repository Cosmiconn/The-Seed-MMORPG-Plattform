#pragma once
#include <cstdint>
#include <vector>
#include <typeindex>
#include <unordered_map>
#include <memory>
#include <span>
#include <type_traits>
#include <algorithm>
#include <cstring>
#include <string>
#include <optional>
#include <functional>  // fuer std::less

namespace TheSeed::ECS {

using Entity = uint64_t;

inline constexpr uint32_t EntityId(Entity e)     { return static_cast<uint32_t>(e); }
inline constexpr uint32_t EntityVersion(Entity e)  { return static_cast<uint32_t>(e >> 32); }
inline constexpr Entity MakeEntity(uint32_t id, uint32_t ver) { return (static_cast<uint64_t>(ver) << 32) | id; }

template<typename T>
concept Component = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

// Hilfsfunktion fuer stabilen type_index-Vergleich
inline bool TypeIndexLess(const std::type_index& a, const std::type_index& b) {
    // Vergleiche name() Zeiger (stabil innerhalb einer Session)
    return std::less<const char*>{}(a.name(), b.name());
}

class Archetype;
class World;

class Archetype {
public:
    explicit Archetype(std::vector<std::type_index> types);

    size_t EntityCount() const { return m_entities.size(); }
    size_t ComponentCount() const { return m_types.size(); }
    const std::vector<std::type_index>& Types() const { return m_types; }

    void RegisterComponentSize(std::type_index type, size_t size);

    size_t AddEntity(Entity e, const std::vector<std::pair<std::type_index, const void*>>& components);
    Entity RemoveEntity(Entity e);  // Returns swapped entity (or 0)
    bool HasEntity(Entity e) const;
    size_t GetEntityIndex(Entity e) const;

    void* GetComponentRaw(size_t entityIndex, std::type_index type, size_t componentSize);
    const void* GetComponentRaw(size_t entityIndex, std::type_index type, size_t componentSize) const;

    const std::vector<Entity>& GetEntities() const { return m_entities; }

private:
    std::vector<std::type_index> m_types;
    std::vector<size_t> m_componentSizes;
    std::vector<std::vector<uint8_t>> m_componentStorage;  // SoA: one buffer per type
    std::unordered_map<uint32_t, size_t> m_entityToIndex;
    std::vector<Entity> m_entities;
    size_t m_entityCapacity = 0;

    void Grow(size_t newCapacity);
};

struct QueryFilter {
    std::vector<std::type_index> all;
    std::vector<std::type_index> any;
    std::vector<std::type_index> none;
};

struct EntityRecord {
    Archetype* archetype = nullptr;
    size_t index = 0;
    uint32_t version = 0;
    bool alive = false;
};

class World {
public:
    World();
    ~World();

    Entity CreateEntity();
    void DestroyEntity(Entity e);
    bool IsAlive(Entity e) const;

    template<Component T, typename... Args>
    T& AddComponent(Entity e, Args&&... args);

    template<Component T>
    T* GetComponent(Entity e);

    template<Component T>
    const T* GetComponent(Entity e) const;

    template<Component T>
    void RemoveComponent(Entity e);

    template<Component T>
    bool HasComponent(Entity e) const;

    template<Component... Cs>
    std::vector<Entity> Query();

    std::vector<Entity> QueryRaw(const QueryFilter& filter);

    using SystemFunc = void(*)(World&, float);
    void RegisterSystem(SystemFunc sys, const char* name, int priority = 0);
    void Tick(float deltaTime);

    size_t EntityCount() const;

    Archetype* FindOrCreateArchetype(const std::vector<std::type_index>& types);

private:
    std::vector<std::unique_ptr<Archetype>> m_archetypes;
    std::vector<std::optional<EntityRecord>> m_entities;
    std::vector<uint32_t> m_freeEntities;
    std::vector<std::tuple<SystemFunc, std::string, int>> m_systems;
    uint32_t m_nextId = 1;

    std::unordered_map<std::type_index, size_t> m_componentSizes;

    void MoveEntity(Entity e, Archetype* oldArchetype, size_t oldIndex,
                    Archetype* newArchetype,
                    const std::vector<std::pair<std::type_index, const void*>>& addedComponents);
};

// Template-Implementierungen

template<Component T, typename... Args>
T& World::AddComponent(Entity e, Args&&... args) {
    if (!IsAlive(e)) {
        static T dummy{};
        return dummy;
    }

    uint32_t id = EntityId(e);
    auto& record = m_entities[id].value();

    if (record.archetype) {
        for (auto& t : record.archetype->Types()) {
            if (t == typeid(T)) {
                return *static_cast<T*>(record.archetype->GetComponentRaw(record.index, typeid(T), sizeof(T)));
            }
        }
    }

    T component(std::forward<Args>(args)...);

    std::vector<std::type_index> newTypes;
    if (record.archetype) {
        newTypes = record.archetype->Types();
    }
    newTypes.push_back(typeid(T));
    std::sort(newTypes.begin(), newTypes.end(), TypeIndexLess);

    Archetype* newArchetype = FindOrCreateArchetype(newTypes);
    for (auto& t : newTypes) {
        auto it = m_componentSizes.find(t);
        if (it != m_componentSizes.end()) {
            newArchetype->RegisterComponentSize(t, it->second);
        }
    }
    newArchetype->RegisterComponentSize(typeid(T), sizeof(T));
    m_componentSizes[typeid(T)] = sizeof(T);

    std::vector<std::pair<std::type_index, const void*>> addedComponents;
    if (record.archetype) {
        for (auto& t : record.archetype->Types()) {
            auto* ptr = record.archetype->GetComponentRaw(record.index, t, m_componentSizes[t]);
            addedComponents.emplace_back(t, ptr);
        }
    }
    addedComponents.emplace_back(typeid(T), &component);
    std::sort(addedComponents.begin(), addedComponents.end(),
              [](const auto& a, const auto& b) { return TypeIndexLess(a.first, b.first); });

    MoveEntity(e, record.archetype, record.index, newArchetype, addedComponents);

    return *static_cast<T*>(record.archetype->GetComponentRaw(record.index, typeid(T), sizeof(T)));
}

template<Component T>
T* World::GetComponent(Entity e) {
    if (!IsAlive(e)) return nullptr;
    uint32_t id = EntityId(e);
    auto& record = m_entities[id].value();
    if (!record.archetype) return nullptr;

    for (auto& t : record.archetype->Types()) {
        if (t == typeid(T)) {
            return static_cast<T*>(record.archetype->GetComponentRaw(record.index, typeid(T), sizeof(T)));
        }
    }
    return nullptr;
}

template<Component T>
const T* World::GetComponent(Entity e) const {
    if (!IsAlive(e)) return nullptr;
    uint32_t id = EntityId(e);
    const auto& record = m_entities[id].value();
    if (!record.archetype) return nullptr;

    for (auto& t : record.archetype->Types()) {
        if (t == typeid(T)) {
            return static_cast<const T*>(record.archetype->GetComponentRaw(record.index, typeid(T), sizeof(T)));
        }
    }
    return nullptr;
}

template<Component T>
void World::RemoveComponent(Entity e) {
    if (!IsAlive(e)) return;
    uint32_t id = EntityId(e);
    auto& record = m_entities[id].value();
    if (!record.archetype) return;

    std::vector<std::type_index> newTypes;
    std::vector<std::pair<std::type_index, const void*>> keptComponents;

    for (auto& t : record.archetype->Types()) {
        if (t != typeid(T)) {
            newTypes.push_back(t);
            auto* ptr = record.archetype->GetComponentRaw(record.index, t, m_componentSizes[t]);
            keptComponents.emplace_back(t, ptr);
        }
    }

    if (newTypes.empty()) {
        Entity swapped = record.archetype->RemoveEntity(e);
        if (swapped != 0) {
            uint32_t swappedId = EntityId(swapped);
            if (swappedId < m_entities.size() && m_entities[swappedId].has_value()) {
                m_entities[swappedId]->index = record.index;
            }
        }
        record.archetype = nullptr;
        record.index = 0;
        return;
    }

    Archetype* newArchetype = FindOrCreateArchetype(newTypes);
    for (auto& t : newTypes) {
        newArchetype->RegisterComponentSize(t, m_componentSizes[t]);
    }
    MoveEntity(e, record.archetype, record.index, newArchetype, keptComponents);
}

template<Component T>
bool World::HasComponent(Entity e) const {
    return GetComponent<T>(e) != nullptr;
}

template<Component... Cs>
std::vector<Entity> World::Query() {
    QueryFilter filter;
    (filter.all.push_back(typeid(Cs)), ...);
    return QueryRaw(filter);
}

} // namespace TheSeed::ECS
