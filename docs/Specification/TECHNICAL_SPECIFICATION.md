# TheSeed – Technische Spezifikation v1.0
**Datum:** 2026-07-04  
**Autor:** Cosmiconn  
**Status:** Draft  

---

## 1. Zusammenfassung

TheSeed ist eine C++23-basierte MMORPG-Engine mit Zero-Code-Editor und integrierter Cloud-Infrastruktur. Dieses Dokument definiert die technischen Anforderungen, Architektur-Entscheidungen und Schnittstellen.

---

## 2. System-Architektur

```
┌─────────────────────────────────────────┐
│           TheSeed Editor                │
│  (ImGui + Custom Viewport + Node-Ed)   │
├─────────────────────────────────────────┤
│           Scripting Layer               │
│      (Lua 5.4 / C# Hot-Reload)        │
├─────────────────────────────────────────┤
│           TheSeed Core                  │
│  ┌─────────┐ ┌─────────┐ ┌──────────┐  │
│  │  ECS    │ │  Net    │ │ Modules │  │
│  │(Archety)│ │(UDP+Rel)│ │(DLL/SO) │  │
│  └─────────┘ └─────────┘ └──────────┘  │
│  ┌─────────┐ ┌─────────┐ ┌──────────┐  │
│  │ Render  │ │ Physics │ │  Audio  │  │
│  │(Vulkan) │ │(Jolt)   │ │(FMOD)   │  │
│  └─────────┘ └─────────┘ └──────────┘  │
└─────────────────────────────────────────┘
                    │
┌─────────────────────────────────────────┐
│         TheSeed Cloud (Docker)          │
│  ┌─────────┐ ┌─────────┐ ┌──────────┐  │
│  │ Server  │ │  DB     │ │  Cache  │  │
│  │ (C++23) │ │(Postgre)│ │ (Redis) │  │
│  └─────────┘ └─────────┘ └──────────┘  │
│  ┌─────────┐ ┌─────────┐ ┌──────────┐  │
│  │ Gateway │ │Metrics  │ │  Patch  │  │
│  │(Envoy)  │ │(Prom)   │ │ System  │  │
│  └─────────┘ └─────────┘ └──────────┘  │
└─────────────────────────────────────────┘
```

---

## 3. Komponenten-Spezifikation

### 3.1 ECS-Core

| Anforderung | Spezifikation |
|-------------|---------------|
| Entity-Limit | 1.000.000 pro Zone |
| Component-Limit | 256 Typen (uint8_t ID) |
| Speicher-Layout | Archetype-basiert, SoA |
| Query-Performance | <1ms für 100k Entities |
| Thread-Safety | Read-Parallel, Write-Serial |

**Schnittstelle:** Siehe `TECH_ARCHITECTURE.md` → ECS-Design.

### 3.2 Netzwerk

| Anforderung | Spezifikation |
|-------------|---------------|
| Protokoll | Custom UDP + Reliable Layer |
| Fallback | QUIC (für Firewall/NAT) |
| Tick-Rate | 60 Hz Server, 60 Hz Client |
| Latenz-Ziel | <50ms (EU), <100ms (US/ASIA) |
| Bandbreite | <50KB/s pro Spieler |
| Max Connections | 10.000 pro Zone (Sharding) |

**Replication:** State-Sync mit Delta-Kompression.  
**Interest Management:** Spatial Hashing (2D) / Octree (3D).

### 3.3 Rendering

| Anforderung | Spezifikation |
|-------------|---------------|
| API | Vulkan 1.3 (Primary), DX12 (Fallback) |
| Pipeline | Forward+ / Deferred Hybrid |
| Features | PBR, Virtual Texturing, LOD, TAA |
| Upscale | DLSS 3.5 / FSR 3 / XeSS |
| Ziel-HW | GTX 1060 @ 60 FPS (100 Spieler) |

### 3.4 Modul-System

| Anforderung | Spezifikation |
|-------------|---------------|
| Interface | C++ pure virtual (IModule) |
| Loading | DLL/SO dynamisch zur Laufzeit |
| Hot-Reload | Serialisieren → Entladen → Laden → Deserialisieren |
| Config | YAML/JSON Schema, validiert |
| Sandbox | Keine (vertrauensbasierter Modul-Store) |

---

## 4. Datenbank-Schema (Minimal)

```sql
-- Spieler
CREATE TABLE players (
    id UUID PRIMARY KEY,
    username VARCHAR(32) UNIQUE NOT NULL,
    email VARCHAR(255),
    created_at TIMESTAMP DEFAULT NOW(),
    last_login TIMESTAMP,
    world_id UUID REFERENCES worlds(id)
);

-- Welten
CREATE TABLE worlds (
    id UUID PRIMARY KEY,
    owner_id UUID REFERENCES players(id),
    name VARCHAR(64) NOT NULL,
    template VARCHAR(32),
    region VARCHAR(16),
    status VARCHAR(16) DEFAULT 'active',
    ccu_limit INT DEFAULT 100,
    created_at TIMESTAMP DEFAULT NOW()
);

-- Items (generisch, JSONB für Flexibilität)
CREATE TABLE items (
    id UUID PRIMARY KEY,
    player_id UUID REFERENCES players(id),
    template_id VARCHAR(64),
    quantity INT DEFAULT 1,
    properties JSONB,
    created_at TIMESTAMP DEFAULT NOW()
);

-- Gilden
CREATE TABLE guilds (
    id UUID PRIMARY KEY,
    name VARCHAR(64) UNIQUE NOT NULL,
    world_id UUID REFERENCES worlds(id),
    leader_id UUID REFERENCES players(id),
    bank_gold INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT NOW()
);
```

---

## 5. API-Spezifikation (Modul-Config)

```yaml
# Beispiel: Combat-Modul Konfiguration
combat:
  type: "action_target"           # action_target | tab_target | hybrid
  global_cooldown: 1.0            # Sekunden
  damage_formula: "((atk * skill) / (def + 100)) * crit"
  damage_types: [physical, fire, ice, lightning]

  elemental_reactions:
    enabled: true
    reactions:
      - trigger: [fire, ice]
        result: steam
        multiplier: 1.5

  crowd_control:
    stun_duration_max: 5.0
    root_duration_max: 8.0
    diminishing_returns: true
```

---

## 6. Performance-Budgets

| System | Budget | Messmethode |
|--------|--------|-------------|
| Frame-Time (Client) | 16.6ms | GPU/CPU Profiler |
| Server-Tick | 16.6ms | Custom Benchmark |
| DB-Query | <10ms | PostgreSQL EXPLAIN |
| Asset-Import | <30s | FBX → Native |
| KI-Generierung | <5min | Meshy API Timer |

---

## 7. Sicherheit

- **Anti-Cheat:** Server-authoritative Validierung (kein Client-Trust)
- **SQL-Injection:** Prepared Statements, ORM-Layer
- **XSS:** Content-Security-Policy, Input-Sanitization
- **Cheat-Engine:** Memory-Encryption, Heartbeat-Checks

---

## 8. Test-Matrix

| Test-Typ | Tool | Frequenz |
|----------|------|----------|
| Unit-Tests | Catch2 | Jeder Commit |
| Integration | Custom Runner | Täglich |
| Performance | Custom Benchmarks | Wöchentlich |
| Stress | k6 / Locust | Monatlich |
| Security | OWASP ZAP | Quartalsweise |

---

## 9. Abkürzungen & Glossar

| Begriff | Bedeutung |
|---------|-----------|
| ECS | Entity Component System |
| SoA | Structure of Arrays |
| CCU | Concurrent Users |
| GI | Global Illumination |
| VT | Virtual Texturing |
| LOD | Level of Detail |
| RTT | Round Trip Time |
