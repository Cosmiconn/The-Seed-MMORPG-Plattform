# TheSeed – Solo-Entwickler Roadmap
## Von Null zum eigenen MMORPG in 18 Monaten

**Autor:** Cosmiconn  
**Version:** 1.0  
**Datum:** 2026-07-04  
**Status:** MVP-fokussiert für 1-Person-Team

---

## Grundprinzip: "Eat Your Own Dog Food"

> Bevor TheSeed ein Produkt wird, **baut der Solo-Entwickler zuerst sein eigenes MMORPG** mit der Engine. Die Engine entsteht iterativ aus den echten Bedürfnissen des Spiels, nicht aus Spekulation.

**Arbeitszeit:** 30–40 Std/Woche (Solo)  
**Gesamtdauer:** 18 Monate (6 Monate Engine-MVP + 12 Monate Spiel + Plattform-Polish)  
**Philosophie:** Jede Engine-Feature muss im eigenen Spiel bewiesen werden, bevor es "fertig" ist.

---

## Phase 0: Fundament & Proof-of-Concept (Woche 1–8)

| Woche | Task | Deliverable | Stunden/Woche |
|-------|------|-------------|---------------|
| 1 | Git-Setup, CMake, vcpkg, CI/CD | Build läuft auf Win+Linux, <5 min | 30 |
| 2 | ECS-Prototyp (EnTT-inspiriert) | 100k Entities, 60 FPS | 35 |
| 3 | Job-System (Work-Stealing) | 1M Tasks, keine Deadlocks | 35 |
| 4 | Netzwerk-Prototyp (UDP + Reliable) | <20ms localhost, 0% Loss | 40 |
| 5 | Vulkan-Renderer: Triangle → Mesh | Validation clean, erste PBR-Szene | 40 |
| 6 | Serialisierung + Delta-Kompression | 10MB → <100KB Delta | 30 |
| 7 | **Integration:** 2 Clients, Cubes, Server | 100 Cubes, <50ms, 60 FPS | 40 |
| 8 | Profiler, Logger, Memory-Tracker | Frame-Time-Graphen, Leak-Detection | 30 |

**Gate:** Zwei Clients verbinden sich übers Internet, bewegen Cubes, Server authorisiert. Wenn nicht → Netzwerk-Stack neu.

---

## Phase 1: Engine-Kern (Woche 9–20)

| Woche | Task | Deliverable | Spiel-Relevanz |
|-------|------|-------------|---------------|
| 9–10 | Vulkan Forward+/Deferred Hybrid | 1000 Meshes, 60 FPS, GTX 1060 | Basis-Rendering für Welt |
| 11 | Material-System (PBR, Hot-Reload) | Custom-Shaders ohne Neustart | Visuelle Qualität |
| 12–13 | Virtual Texturing / Streaming | 4K-Texturen ohne Stutter | Große offene Welt |
| 14 | Interest Management (Spatial Hash) | Keine Pakete aus fernen Zonen | Performance bei 100+ Spielern |
| 15–16 | Entity Replication + Delta | <50KB/s pro Spieler | Skalierbarkeit |
| 17 | Lag Compensation | Hit-Validation bei 200ms | Kampf-System |
| 18 | Deterministische Simulation | Replay-fähig, Anti-Cheat | Server-Authority |
| 19 | Jolt Physics (deterministisch) | 1000 Körper, Server-Client sync | Kollision, Kampf |
| 20 | Animation (Skeletal, Blending) | 100 Charaktere, 60 FPS | NPCs, Spieler |

**Gate:** Ein einfaches Multiplayer-Spiel (Tag) läuft stabil mit 10+ Spielern über das Internet.

---

## Phase 2: Editor MVP (Woche 21–28)

| Woche | Task | Deliverable | Spiel-Relevanz |
|-------|------|-------------|---------------|
| 21 | ImGui Editor-Framework | Docking, Layout speicherbar | Produktivität |
| 22 | Echtzeit-Viewport | Editor = Spiel | WYSIWYG |
| 23 | Play-in-Editor | Server+Client im Editor | Iterationsgeschwindigkeit |
| 24 | Terrain-Editor (Heightmap, Pinsel) | 4k×4k Terrain | Spiel-Welt |
| 25 | Entity-Placement (Drag & Drop) | Gizmos, Skalieren, Rotieren | Level-Design |
| 26 | NavMesh-Editor | NPC findet Weg A→B | AI-Patrol |
| 27 | Asset-Import (FBX/GLTF) | Auto-LOD, Vorschau | Asset-Pipeline |
| 28 | Hot-Reload für alle Assets | Änderung sofort sichtbar | Workflow |

**Gate:** Der Entwickler baut in 1 Tag eine Zone mit Terrain, 10 NPCs, 3 Quests (noch rudimentär).

---

## Phase 3: Das EIGENE Spiel – MMORPG-Kern (Woche 29–44)

> **WICHTIG:** Ab hier wird nicht mehr "Engine für andere" gebaut, sondern **das eigene Spiel**. Jede Feature-Entscheidung ist durch das Spiel motiviert.

| Woche | Task | Deliverable | Modul |
|-------|------|-------------|-------|
| 29–30 | Inventar-Modul | 40 Slots, Drag-Drop, DB-Persistenz | Inventory |
| 31–32 | Kampf-Modul (Tab-Target + Action) | Formeln, CC, Server-Validierung | Combat |
| 33–34 | Quest-Modul (Kill, Fetch, Talk) | Verzweigungen, Shared Credit | Quest |
| 35–36 | Crafting-Modul | Rezepte, Skill-Up, Qualität | Crafting |
| 37 | Economy (Auction House, Shops) | Währungen, Steuern | Economy |
| 38 | Housing-Modul | Instanziert, Dekoration, Besucher | Housing |
| 39 | Gilden-Modul | Ränge, Bank, Halle | Guild |
| 40 | AI Behavior (Patrol, Combat, Social) | Behavior Trees, Konfiguration | AI |
| 41–42 | Node-Editor für Quests/Dialoge | Visuelles Scripting ohne Code | Editor |
| 43–44 | **Integrationstest** | Alle Module zusammen, 2 Stunden Content | Spiel |

**Gate:** Ein Alpha-Tester (Freund) spielt 2 Stunden ohne Crash, versteht das Spiel, kommt zurück.

---

## Phase 4: Welt & Content (Woche 45–56)

| Woche | Task | Deliverable |
|-------|------|-------------|
| 45–46 | Welt-Template "Medieval Kingdom" | Start-Stadt, 10 NPCs, 5 Quests |
| 47–48 | Dungeon-System | 1 Dungeon mit Boss, Loot, Mechaniken |
| 49–50 | KI-Asset-Generierung (Meshy, SD) | 20+ Assets, Text-to-3D/Texture |
| 51–52 | Audio-System (3D, Stems, Musik) | FMOD-ähnlich, 100 Quellen |
| 53–54 | Polishing (UI, FX, Partikel) | Professioneller Look |
| 55–56 | **Closed Alpha** | 10 Freunde spielen, Feedback |

**Gate:** 10 Spieler gleichzeitig online, 1 Stunde Session, keine kritischen Bugs.

---

## Phase 5: Cloud & Publish (Woche 57–64)

| Woche | Task | Deliverable |
|-------|------|-------------|
| 57–58 | Docker-Container, Kubernetes | Server läuft in Container |
| 59 | PostgreSQL + Redis | Spielerdaten, Sessions, Cache |
| 60 | Auto-Scaling (0→100 Spieler) | Pay-as-you-grow |
| 61 | One-Click Publish | Editor → Live-Server in <10 Min |
| 62 | Patch-System (Delta) | 2GB → 15MB Delta |
| 63 | GM-Tools (Web-Interface) | Teleport, Ban, Spawn |
| 64 | **Open Beta** | Öffentlicher Test, 100+ Spieler |

**Gate:** Ein Spieler meldet sich an, lädt Client, spielt 30 Minuten, kein Crash.

---

## Phase 6: Monetarisierung & Plattform (Woche 65–72)

| Woche | Task | Deliverable |
|-------|------|-------------|
| 65 | In-Game-Shop | Items, Währungen, Server-Validierung |
| 66 | Subscription-Management | Premium-Ränge, Stripe |
| 67 | TheSeed Cloud Free Tier | 100 CCU, 1 Region, 10GB |
| 68 | TheSeed Cloud Indie/Pro Tier | Skalierbare Preise |
| 69 | Asset-Exchange (Grundgerüst) | Upload, Download, Karma |
| 70 | Dokumentation & Tutorials | Self-Service für Entwickler |
| 71 | Marketing-Website | theseed.world |
| 72 | **Launch** | TheSeed als Plattform live |

**Gate:** 1.000 Downloads, 100 aktive Entwickler, 10 veröffentlichte Welten.

---

## Solo-Entwickler Stunden-Plan (Woche 1–72)

| Phase | Dauer | Std/Woche | Gesamt-Stunden |
|-------|-------|-----------|----------------|
| P0: Fundament | 8 Wochen | 35 | 280 |
| P1: Engine-Kern | 12 Wochen | 35 | 420 |
| P2: Editor MVP | 8 Wochen | 30 | 240 |
| P3: Eigene Spiel | 16 Wochen | 40 | 640 |
| P4: Welt & Content | 12 Wochen | 35 | 420 |
| P5: Cloud & Publish | 8 Wochen | 30 | 240 |
| P6: Plattform | 8 Wochen | 30 | 240 |
| **Gesamt** | **72 Wochen (~18 Monate)** | **~34** | **~2.480 Stunden** |

---

## Risiken & Mitigation (Solo)

| Risiko | Wahrscheinlichkeit | Mitigation |
|--------|-------------------|------------|
| Burnout | Hoch | 1 Woche Pause alle 8 Wochen, kein Crunch |
| Scope Creep | Hoch | Jede Feature-Entscheidung: "Braucht DAS mein Spiel?" |
| Netzwerk zu komplex | Mittel | Frühen Prototyp (Woche 7) nicht verschieben! |
| Keine Spieler | Mittel | Von Anfang an Community aufbauen (Devlog, Twitter) |
| Kosten zu hoch | Mittel | Free Tier nutzen, erst bei Revenue skalieren |

---

## Erfolgskriterien (Solo)

- [ ] Monat 3: Erster Cube bewegt sich über Netzwerk
- [ ] Monat 6: Erste spielbare Zone mit Kampf & Quest
- [ ] Monat 12: Eigenes MMORPG in Closed Alpha
- [ ] Monat 15: Open Beta mit 100+ Spielern
- [ ] Monat 18: TheSeed Plattform live, 1.000+ Downloads

---

> **"Der beste Weg, eine Engine zu bauen, ist, ein Spiel zu bauen. Der beste Weg, eine Plattform zu bauen, ist, sie selbst zu brauchen."**
