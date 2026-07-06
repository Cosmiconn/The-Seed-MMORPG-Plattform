# TheSeed – Technische Spezifikation
## C++23 MMORPG Engine & Cloud-Plattform

**Version:** 1.0  
**Datum:** 2026-07-04  
**Autor:** Cosmiconn  
**Status:** Entwurf

---

## 1. Übersicht

TheSeed ist eine **modulare, data-driven MMORPG-Engine** mit integrierter Cloud-Plattform. Sie ermöglicht Solo-Entwicklern und kleinen Teams, persistente Online-Welten zu erstellen, zu hosten und zu monetarisieren – ohne tiefgehende Programmierkenntnisse.

**Kernprinzipien:**
1. **Data-Oriented Design:** Cache-freundliche ECS-Architektur
2. **Determinismus:** Server-Simulation für Replay und Anti-Cheat
3. **Modularität:** Gameplay als dynamische Plugins, Core als statische Module
4. **Zero-Overhead:** Abstraktion kostet keine Performance

---

## 2. Systemarchitektur

### 2.1 High-Level Diagramm

```
┌─────────────────────────────────────────────────────────────┐
│                      TheSeed Ecosystem                       │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │   Client    │  │   Editor    │  │   Web Dashboard     │ │
│  │  (C++23)    │  │  (C++23)    │  │   (React/Next.js)   │ │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘ │
│         │                │                    │            │
│         └────────────────┴────────────────────┘            │
│                          │                                 │
│              ┌───────────┴───────────┐                    │
│              │    TheSeed Cloud       │                    │
│              │  (Kubernetes, Serverless)│                    │
│              └───────────┬───────────┘                    │
│                          │                                 │
│         ┌────────────────┼────────────────┐               │
│         ▼                ▼                ▼               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │
│  │   Server    │  │   Server    │  │   Server    │      │
│  │  (Zone A)   │  │  (Zone B)   │  │  (Zone C)   │      │
│  │  C++23 Core │  │  C++23 Core │  │  C++23 Core │      │
│  └─────────────┘  └─────────────┘  └─────────────┘      │
│         │                │                │               │
│         └────────────────┴────────────────┘               │
│                          │                                 │
│              ┌───────────┴───────────┐                    │
│              │   Shared Storage     │                    │
│              │  PostgreSQL + Redis  │                    │
│              └─────────────────────┘                    │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Komponenten

| Komponente | Technologie | Zweck |
|------------|-------------|-------|
| **Engine Core** | C++23 (Modules, Coroutines, Ranges) | Performance, Determinismus |
| **Rendering** | Vulkan 1.3, DX12 Ultimate, Metal 3 | Cross-Platform, GPU-Driven |
| **Physics** | Jolt Physics | Deterministisch, schnell |
| **Netzwerk** | Custom UDP + QUIC Fallback | Kontrolle, geringe Latenz |
| **ECS** | Custom (EnTT-inspiriert) | Cache-freundlich, kein Overhead |
| **Scripting** | Lua 5.4 / C# .NET | Hot-Reload, embeddable |
| **UI Editor** | ImGui + Custom Widgets | Schnelle Iteration |
| **DB** | PostgreSQL 15+ | ACID, JSON, skalierbar |
| **Cache** | Redis 7+ | Sessions, Leaderboards |
| **Storage** | MinIO / AWS S3 | S3-API, CDN |
| **Cloud** | Kubernetes + Terraform | IaC, Auto-Scaling |
| **Monitoring** | Prometheus + Grafana + Loki | Open Source |
| **CI/CD** | GitHub Actions / GitLab CI | Integriert |
| **KI-APIs** | REST/gRPC-Hub | Entkopplung |

---

## 3. ECS-Architektur (Entity-Component-System)

### 3.1 Design-Entscheidungen

- **Archetype-based Storage:** Entities mit gleicher Komponenten-Signatur werden in Chunks gespeichert (SoA)
- **Chunk Capacity:** 1024 Entities pro Chunk (passt in L1-Cache)
- **Query Performance:** O(1) Lookup via Archetype-Hash, O(n) Iteration innerhalb Chunks
- **Determinismus:** Keine Pointer, keine Virtual Functions im Hot Path

### 3.2 Entity Lifecycle

```
[create_entity()] → [emplace<Components>()] → [Systems tick()] → [destroy_entity()]
       │                    │                        │                  │
       ▼                    ▼                        ▼                  ▼
   Entity ID          Archetype-Lookup         Query-Filter       Chunk Cleanup
   Generation         Chunk-Allocation         System-Update      Memory-Pool
```

### 3.3 Komponenten-Beispiele

```cpp
struct Transform {
    float x, y, z;
    float qx, qy, qz, qw;
    float sx, sy, sz;
}; // 40 bytes, cache-aligned

struct NetworkSync {
    uint32_t net_id;
    uint8_t owner;
    bool authoritative;
}; // 8 bytes

struct Health {
    float current, max;
    uint32_t last_damage_tick;
}; // 12 bytes
```

### 3.4 System-Prioritäten

| Priority | System | Zweck |
|----------|--------|-------|
| -100 | InputSystem | Spieler-Eingaben sammeln |
| -50 | PhysicsSystem | Jolt Physics tick |
| 0 | MovementSystem | Velocity → Transform |
| 10 | CombatSystem | Schadensberechnung |
| 20 | QuestSystem | Quest-Fortschritt |
| 50 | ReplicationSystem | Netzwerk-Sync |
| 100 | CleanupSystem | Destroyed Entities entfernen |

---

## 4. Netzwerk-Protokoll

### 4.1 Protokoll-Stack

```
┌─────────────────────────────────────┐
│         Application Layer           │
│   (Entity Spawn, Input, Chat)       │
├─────────────────────────────────────┤
│         Replication Layer             │
│   (Delta-Compression, Interest)       │
├─────────────────────────────────────┤
│         Reliable Layer              │
│   (ACKs, NACKs, Resend)             │
├─────────────────────────────────────┤
│         Transport Layer             │
│   (Custom UDP / QUIC Fallback)      │
├─────────────────────────────────────┤
│         Network Layer               │
│   (IPv4/IPv6, Routing)            │
└─────────────────────────────────────┘
```

### 4.2 Packet Format

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Sequence Number                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           ACK Number                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           ACK Bits                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          Payload Size         |  Packet Type  |     Flags     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Payload                            |
|                              ...                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### 4.3 Interest Management

- **Spatial Hashing:** Welt in Gitterzellen (100m × 100m) unterteilt
- **Zone-Query:** Spieler sendet Position → Server gibt relevante Zonen zurück
- **Entity-Filter:** Nur Entities innerhalb Sichtradius werden repliziert
- **Bandwidth-Ziel:** <50KB/s pro Spieler bei 1000 Entities in Reichweite

### 4.4 Lag Compensation

1. Client sendet Input mit Client-Tick
2. Server empfängt Input bei Server-Tick T (RTT/2 verzögert)
3. Server rewinds Entity-Positionen zu Tick T – RTT/2
4. Server führt Hit-Validation durch
5. Server spielt Simulation vorwärts

---

## 5. Modul-System

### 5.1 Modul-Lifecycle

```
[Load DLL] → [on_load()] → [on_init(config, api)] → [on_tick(dt)] → [on_shutdown()] → [Unload DLL]
                │                  │                      │
                ▼                  ▼                      ▼
           Host-Registrierung   Config laden         System-Tick
```

### 5.2 Inter-Modul-Kommunikation

- **API-Calls:** Synchrone Funktionsaufrufe zwischen Modulen
- **Events:** Asynchrone Pub/Sub-Nachrichten
- **Shared State:** Kein Shared State! Nur via API/Events

### 5.3 Hot-Reload

1. Modul wird entladen (State serialisieren)
2. DLL wird neu geladen
3. State wird deserialisiert
4. `on_hot_reload()` wird aufgerufen
5. Modul läuft weiter ohne Neustart

---

## 6. Rendering-System

### 6.1 Pipeline

```
[Scene Graph] → [Culling (Frustum + Occlusion)] → [GPU-Driven Rendering]
                    │                                    │
                    ▼                                    ▼
            [Mesh Shaders]                      [Virtual Texturing]
                    │                                    │
                    ▼                                    ▼
            [Deferred G-Buffer]                 [PBR Shading]
                    │                                    │
                    ▼                                    ▼
            [Post-Processing]                     [DLSS/FSR Upscale]
```

### 6.2 Virtual Texturing

- **Page Cache:** 4K-Texturen werden in 128×128-Tiles geladen
- **Feedback Pass:** GPU schreibt benötigte Tiles in Buffer
- **Async Upload:** Tiles werden im Hintergrund von S3/MinIO geladen
- **Memory Budget:** Konfigurierbar (z.B. 512MB VRAM für Textures)

### 6.3 LOD-System

- **Automatisch:** Mesh-Optimierung via MeshOptimizer
- **Manuell:** Artist-definierte LOD-Stufen
- **Transition:** Dithered oder Morph-Target (kein Pop)

---

## 7. Cloud-Architektur

### 7.1 Deploy-Fluss

```
[Developer clicks "Publish"]
           │
           ▼
[Editor builds Client + Server]
           │
           ▼
[Terraform apply → AWS/GCP/Azure]
           │
           ▼
[Kubernetes deploys Server Pods]
           │
           ▼
[PostgreSQL schema migration]
           │
           ▼
[Assets uploaded to CDN]
           │
           ▼
[SSL cert issued, DNS updated]
           │
           ▼
[Monitoring dashboards live]
           │
           ▼
[Game live at dein-spiel.theseed.world]
```

### 7.2 Auto-Scaling

- **Metric:** CPU-Usage + Active Players
- **Scale-Up:** >70% CPU oder >80% Capacity → +1 Pod
- **Scale-Down:** <30% CPU und <20% Capacity → -1 Pod
- **Min Pods:** 1 (kostet nur wenn Spieler online)
- **Max Pods:** 100 (pro Zone)
- **Startup Time:** <3 Sekunden (Container-Image optimiert)

### 7.3 Datenbank-Schema (Ausschnitt)

```sql
-- Players
CREATE TABLE players (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    username VARCHAR(32) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    last_login TIMESTAMPTZ,
    subscription_tier VARCHAR(16) DEFAULT 'free'
);

-- Worlds
CREATE TABLE worlds (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    owner_id UUID REFERENCES players(id),
    name VARCHAR(64) NOT NULL,
    template VARCHAR(32) NOT NULL,
    region VARCHAR(16) DEFAULT 'eu-central',
    status VARCHAR(16) DEFAULT 'draft',
    ccu_limit INT DEFAULT 100,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Entities (persisted world state)
CREATE TABLE entities (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    world_id UUID REFERENCES worlds(id),
    archetype_id VARCHAR(64) NOT NULL,
    components JSONB NOT NULL,
    transform JSONB NOT NULL,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Items (inventory, economy)
CREATE TABLE items (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    world_id UUID REFERENCES worlds(id),
    owner_id UUID REFERENCES players(id),
    template_id VARCHAR(64) NOT NULL,
    quantity INT DEFAULT 1,
    durability FLOAT DEFAULT 1.0,
    metadata JSONB DEFAULT '{}',
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Transactions (economy, audit)
CREATE TABLE transactions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    world_id UUID REFERENCES worlds(id),
    buyer_id UUID REFERENCES players(id),
    seller_id UUID REFERENCES players(id),
    item_id UUID REFERENCES items(id),
    amount DECIMAL(18, 2) NOT NULL,
    currency VARCHAR(8) NOT NULL,
    tax DECIMAL(18, 2) DEFAULT 0,
    created_at TIMESTAMPTZ DEFAULT NOW()
);
```

---

## 8. Sicherheit

### 8.1 Anti-Cheat

- **Server-Authoritative:** Alle Gameplay-Entscheidungen auf Server
- **Determinismus:** Replay-Validierung möglich
- **Input-Validation:** Rate-Limiting, Sanity-Checks
- **Obfuscation:** Client-Code-Obfuscation (optional)
- **Encryption:** TLS 1.3 für reliable, ChaCha20-Poly1305 für unreliable

### 8.2 Datenschutz

- **GDPR-konform:** Datenlöschung auf Anfrage
- **DGSVO:** EU-Server standardmäßig
- **Pseudonymisierung:** Spieler-IDs statt E-Mail in Logs

---

## 9. Performance-Ziele

| Metrik | Ziel | Testbedingung |
|--------|------|---------------|
| Server-Tick | 60 Hz | 10.000 Entities/Zone |
| Client FPS | 60+ | 100 Spieler, GTX 1060 |
| Compile-Zeit | <5 Min | Full Build, C++20 Modules |
| Netzwerk | <50KB/s | Pro Spieler, 1000 Entities |
| Latenz | <100ms | EU→EU, 95th Percentile |
| Ladezeit | <30s | Spielstart, SSD |
| Zone-Wechsel | <3s | Asset-Streaming |

---

## 10. API-Referenz (Ausschnitt)

### 10.1 ECS API

```cpp
// World erstellen
ts::ecs::World world;

// Entity erstellen
auto player = world.create_entity();
world.emplace<ts::ecs::Transform>(player, 0.0f, 0.0f, 0.0f);
world.emplace<ts::ecs::Health>(player, 100.0f, 100.0f, 0);
world.emplace<ts::ecs::NetworkSync>(player, 42, 1, true);

// Systeme registrieren
world.register_system(std::make_unique<MovementSystem>());
world.register_system(std::make_unique<CombatSystem>());

// Query
world.query<ts::ecs::Transform, ts::ecs::Health>()
    .each([](ts::ecs::Entity e, ts::ecs::Transform& t, ts::ecs::Health& h) {
        if (h.current <= 0) {
            world.destroy_entity(e);
        }
    });

// Tick
world.update(1.0f / 60.0f);
```

### 10.2 Netzwerk API

```cpp
// Server
ts::net::Server::Config cfg{.port = 7777, .max_clients = 1000};
ts::net::Server server(cfg);
server.on_connect([](auto id) { spdlog::info("Player {} connected", id); });
server.on_packet([](auto id, auto type, auto payload) {
    if (type == ts::net::PacketType::PlayerInput) {
        // Process input
    }
});
server.run();

// Client
ts::net::Client::Config ccfg{.server_address = "theseed.world"};
ts::net::Client client(ccfg);
client.connect();
client.send_input(input_data);
```

### 10.3 Modul API

```cpp
class MyModule : public ts::modules::IModule {
public:
    ModuleInfo info() const override { return {.id = "my_module", ...}; }
    void on_init(ts::modules::IModuleConfig& cfg, ts::modules::IModuleAPI& api) override {
        auto value = cfg.get("my_param").as<int>(42);
        api.register_function<int>("my_module.add", [](int a, int b) { return a + b; });
    }
    void on_tick(float dt) override { /* ... */ }
};
TS_REGISTER_MODULE(MyModule)
```

---

## 11. Anhang

### A. Build-Kommandos

```bash
# Setup
vcpkg install

# Configure
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --parallel

# Test
ctest --test-dir build --output-on-failure

# Package
cpack -G ZIP
```

### B. Port-Liste

| Dienst | Port | Protokoll |
|--------|------|-----------|
| Game Server | 7777 | UDP + Reliable |
| Game Server (QUIC) | 7778 | QUIC |
| Web Dashboard | 443 | HTTPS |
| Metrics | 9090 | HTTP |
| Logs | 3100 | HTTP |

### C. Abkürzungen

| Abkürzung | Bedeutung |
|-----------|-----------|
| ECS | Entity-Component-System |
| SoA | Structure of Arrays |
| AoS | Array of Structures |
| RTT | Round-Trip Time |
| CCU | Concurrent Concurrent Users |
| ARR | Annual Recurring Revenue |
| SLA | Service Level Agreement |
| IaC | Infrastructure as Code |

---

**Ende der Spezifikation.**
