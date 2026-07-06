# ChangeLog 0007 – Woche 7: Integration (2 Clients, Cubes)

**Datum:** 2026-07-06  
**Version:** 0.1.6 → 0.2.0

## Hinzugefuegt
- GameServer: Authoritative ECS-Simulation
- GameClient: State-Empfang + Interpolation + Rendering
- Orbital Cube-Bewegung (Server-seitig)
- Delta-Komprimierte State-Replication
- Client-Side Interpolation (smooth movement)

## Architektur
```
Server (ECS + FixedTick 60Hz) → UDP Delta → Client (Interp + Vulkan)
```

## Tests
- Server Start/Stop
- Client Connect + State Sync
- 2 Clients gleichzeitig
- Delta Compression Bandwidth

## Gefixt
- `std::span` CTAD MSVC-Kompatibilitaet (explizite Template-Args)

## Dateien
- `src/core/game_server.hpp/.cpp`
- `src/core/game_client.hpp/.cpp`
- `tests/test_integration.cpp`
