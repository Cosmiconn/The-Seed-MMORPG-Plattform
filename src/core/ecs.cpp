#include "core/ecs.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace TheSeed::ECS {

Archetype::Archetype(std::vector<std::type_index> types)
    : m_types(std::move(types)) {
    std::sort(m_types.begin(), m_types.end(), TypeIndexLess);
    m_componentStorage.resize(m_types.size());
    m_componentSizes.resize(m_types.size(), 0);
}

void Archetype::RegisterComponentSize(std::type_index type, size_t size) {
    for (size_t i = 0; i < m_types.size(); ++i) {
        if (m_types[i] == type) {
            m_componentSizes[i] = size;
            return;
        }
    }
}

size_t Archetype::AddEntity(Entity e, const std::vector<std::pair<std::type_index, const void*>>& components) {
    if (m_entities.size() >= m_entityCapacity) {
        Grow(m_entityCapacity == 0 ? 64 : m_entityCapacity * 2);
    }

    size_t index = m_entities.size();
    m_entities.push_back(e);
    m_entityToIndex[EntityId(e)] = index;

    for (const auto& [type, ptr] : components) {
        for (size_t i = 0; i < m_types.size(); ++i) {
            if (m_types[i] == type && m_componentSizes[i] > 0 && ptr != nullptr) {
                std::memcpy(m_componentStorage[i].data() + index * m_componentSizes[i], ptr, m_componentSizes[i]);
                break;
            }
        }
    }

    return index;
}

Entity Archetype::RemoveEntity(Entity e) {
    auto it = m_entityToIndex.find(EntityId(e));
    if (it == m_entityToIndex.end()) return 0;

    size_t index = it->second;
    size_t lastIndex = m_entities.size() - 1;
    Entity swappedEntity = 0;

    if (index != lastIndex) {
        Entity lastEntity = m_entities[lastIndex];
        m_entities[index] = lastEntity;
        m_entityToIndex[EntityId(lastEntity)] = index;
        swappedEntity = lastEntity;

        for (size_t i = 0; i < m_types.size(); ++i) {
            if (m_componentSizes[i] > 0) {
                std::memcpy(
                    m_componentStorage[i].data() + index * m_componentSizes[i],
                    m_componentStorage[i].data() + lastIndex * m_componentSizes[i],
                    m_componentSizes[i]
                );
            }
        }
    }

    m_entities.pop_back();
    m_entityToIndex.erase(it);
    return swappedEntity;
}

bool Archetype::HasEntity(Entity e) const {
    return m_entityToIndex.find(EntityId(e)) != m_entityToIndex.end();
}

size_t Archetype::GetEntityIndex(Entity e) const {
    auto it = m_entityToIndex.find(EntityId(e));
    if (it != m_entityToIndex.end()) return it->second;
    return 0;
}

void* Archetype::GetComponentRaw(size_t entityIndex, std::type_index type, size_t componentSize) {
    for (size_t i = 0; i < m_types.size(); ++i) {
        if (m_types[i] == type) {
            if (m_componentSizes[i] == 0 && componentSize > 0) {
                m_componentSizes[i] = componentSize;
                m_componentStorage[i].resize(m_entityCapacity * componentSize);
            }
            size_t size = m_componentSizes[i];
            if (size == 0 || entityIndex >= m_entities.size()) return nullptr;
            if (m_componentStorage[i].size() < (entityIndex + 1) * size) {
                m_componentStorage[i].resize((entityIndex + 1) * size);
            }
            return m_componentStorage[i].data() + entityIndex * size;
        }
    }
    return nullptr;
}

const void* Archetype::GetComponentRaw(size_t entityIndex, std::type_index type, size_t componentSize) const {
    return const_cast<Archetype*>(this)->GetComponentRaw(entityIndex, type, componentSize);
}

void Archetype::Grow(size_t newCapacity) {
    m_entityCapacity = newCapacity;
    for (size_t i = 0; i < m_types.size(); ++i) {
        if (m_componentSizes[i] > 0) {
            m_componentStorage[i].resize(m_entityCapacity * m_componentSizes[i]);
        }
    }
}

World::World() = default;
World::~World() = default;

Entity World::CreateEntity() {
    uint32_t id;
    uint32_t version = 1;

    if (!m_freeEntities.empty()) {
        id = m_freeEntities.back();
        m_freeEntities.pop_back();
        if (m_entities[id].has_value()) {
            version = m_entities[id]->version + 1;
        }
    } else {
        id = m_nextId++;
        if (id >= m_entities.size()) {
            m_entities.resize(id + 1);
        }
    }

    m_entities[id] = EntityRecord{nullptr, 0, version, true};
    return MakeEntity(id, version);
}

void World::DestroyEntity(Entity e) {
    if (!IsAlive(e)) return;
    uint32_t id = EntityId(e);
    auto& record = m_entities[id].value();

    if (record.archetype) {
        Entity swapped = record.archetype->RemoveEntity(e);
        if (swapped != 0) {
            uint32_t swappedId = EntityId(swapped);
            if (swappedId < m_entities.size() && m_entities[swappedId].has_value()) {
                m_entities[swappedId]->index = record.index;
            }
        }
    }

    record.archetype = nullptr;
    record.index = 0;
    record.alive = false;
    m_freeEntities.push_back(id);
}

bool World::IsAlive(Entity e) const {
    uint32_t id = EntityId(e);
    uint32_t ver = EntityVersion(e);
    if (id >= m_entities.size() || !m_entities[id].has_value()) return false;
    return m_entities[id]->alive && m_entities[id]->version == ver;
}

Archetype* World::FindOrCreateArchetype(const std::vector<std::type_index>& types) {
    for (auto& arch : m_archetypes) {
        if (arch->Types() == types) {
            return arch.get();
        }
    }
    auto newArch = std::make_unique<Archetype>(types);
    auto* ptr = newArch.get();
    m_archetypes.push_back(std::move(newArch));
    return ptr;
}

void World::MoveEntity(Entity e, Archetype* oldArchetype, size_t oldIndex,
                       Archetype* newArchetype,
                       const std::vector<std::pair<std::type_index, const void*>>& components) {
    if (oldArchetype) {
        Entity swapped = oldArchetype->RemoveEntity(e);
        if (swapped != 0) {
            uint32_t swappedId = EntityId(swapped);
            if (swappedId < m_entities.size() && m_entities[swappedId].has_value()) {
                m_entities[swappedId]->index = oldIndex;
            }
        }
    }

    size_t newIndex = newArchetype->AddEntity(e, components);

    uint32_t id = EntityId(e);
    m_entities[id]->archetype = newArchetype;
    m_entities[id]->index = newIndex;
}

std::vector<Entity> World::QueryRaw(const QueryFilter& filter) {
    std::vector<Entity> result;

    for (auto& arch : m_archetypes) {
        const auto& types = arch->Types();

        bool hasAll = true;
        for (auto& t : filter.all) {
            bool found = false;
            for (auto& at : types) {
                if (at == t) { found = true; break; }
            }
            if (!found) { hasAll = false; break; }
        }
        if (!hasAll) continue;

        bool hasNone = false;
        for (auto& t : filter.none) {
            for (auto& at : types) {
                if (at == t) { hasNone = true; break; }
            }
            if (hasNone) break;
        }
        if (hasNone) continue;

        if (!filter.any.empty()) {
            bool hasAny = false;
            for (auto& t : filter.any) {
                for (auto& at : types) {
                    if (at == t) { hasAny = true; break; }
                }
                if (hasAny) break;
            }
            if (!hasAny) continue;
        }

        for (Entity e : arch->GetEntities()) {
            if (IsAlive(e)) {
                result.push_back(e);
            }
        }
    }

    return result;
}

void World::RegisterSystem(SystemFunc sys, const char* name, int priority) {
    m_systems.push_back({sys, name, priority});
    std::sort(m_systems.begin(), m_systems.end(), [](const auto& a, const auto& b) {
        return std::get<2>(a) < std::get<2>(b);
    });
}

void World::Tick(float deltaTime) {
    for (auto& [sys, name, prio] : m_systems) {
        sys(*this, deltaTime);
    }
}

size_t World::EntityCount() const {
    size_t count = 0;
    for (const auto& opt : m_entities) {
        if (opt.has_value() && opt->alive) count++;
    }
    return count;
}

} // namespace TheSeed::ECS
