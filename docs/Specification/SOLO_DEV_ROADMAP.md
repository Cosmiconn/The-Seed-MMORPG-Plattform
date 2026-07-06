# TheSeed – Solo-Dev Roadmap
**Strategie:** Spiel zuerst, Plattform danach.  
**Zeithorizont:** 18 Monate bis spielbares MMORPG, 24 Monate bis Plattform-Launch.  
**Plattform:** Windows 11 (Primary), Linux-Server (Cloud).  

---

## Prinzip: "Dogfood First"

> Du baust kein Werkzeug, das du nicht selbst jeden Tag benutzt.  
> Jede Zeile Engine-Code entsteht, weil dein Spiel sie braucht.

---

## Phase 0: Fundament (Monat 1–2)

| Woche | Ziel | Deliverable | Warum das reicht |
|-------|------|-------------|------------------|
| 1 | Projekt-Setup | CMake + vcpkg baut auf Win11 | Keine Zeit mit Build-Problemen verschwenden |
| 2 | ECS-Core | 100k Entities, 60 FPS | EnTT-Style, Archetype-basiert |
| 3 | Netzwerk-POC | 2 Clients bewegen Cubes, Server authorisiert | Custom UDP + Reliable Layer |
| 4 | Job-System | 64 Threads, Work-Stealing | Für spätere 100-Spieler-Schlachten |

**Gate:** 100 Cubes über Netzwerk, <50ms Latenz, 60 FPS.  
*Wenn nicht: Architektur vereinfachen, kein Over-Engineering.*

---

## Phase 1: Das Spiel entsteht (Monat 3–6)

| Monat | Ziel | Deliverable |
|-------|------|-------------|
| 3 | **Modul-System** + Inventar | HelloWorld-Modul lädt/unlädt. Inventar: Grid, Drag-Drop, DB-Persistenz |
| 4 | **Kampf-System** + NPCs | Tab-Target, Schadensformel, 3 NPCs mit Patrol |
| 5 | **Quest-System** + Zone | 3 Quests (Kill/Fetch/Talk), 1 Zone mit Terrain |
| 6 | **Closed Alpha** | 5 Freunde spielen 2 Stunden, Server läuft auf deinem PC |

**Wichtig:** Kein Editor. Alles über YAML/JSON konfiguriert.  
**Rendering:** Vulkan Forward+, PBR-Material, 1 Lichtquelle. Kein GI, kein Virtual Texturing.

---

## Phase 2: Editor & Tools (Monat 7–9)

> *Erst jetzt baust du Tools, weil YAML-Editieren zu langsam wird.*

| Monat | Ziel | Deliverable |
|-------|------|-------------|
| 7 | **Editor-Grundgerüst** (ImGui) | Viewport, Entity-Placement, Gizmos |
| 8 | **Terrain + Vegetation** | Heightmap-Pinsel, 10k Bäume scatter |
| 9 | **Node-Editor** | Quest-Logik visuell, Hot-Reload |

**Gate:** Du erstellst eine neue Quest in 30 Minuten ohne Code.  
**Philosophie:** Der Editor ist ein "Modul" wie jedes andere – er linkt gegen die Engine.

---

## Phase 3: Cloud MVP (Monat 10–11)

> *Kein Kubernetes. Ein Docker-Container auf einem VPS.*

| Woche | Ziel | Deliverable |
|-------|------|-------------|
| 1 | Docker-Container | `docker run theseed-server` startet Server |
| 2 | Deploy-Script | `deploy.ps1` → baut, pusht zu VPS, startet |
| 3 | PostgreSQL + Redis | Spieler, Items, Sessions persistieren |
| 4 | Delta-Patch | Client updated sich automatisch |

**Kosten:** ~€10/Monat VPS.  
**Auto-Scaling:** Nicht vor Monat 18. Du skalierst manuell hoch.

---

## Phase 4: Content & Asset-Pipeline (Monat 12–14)

| Monat | Ziel | Deliverable |
|-------|------|-------------|
| 12 | **Asset-Import** | FBX/GLTF → Engine, LOD-Gen, Collision |
| 13 | **1 Template** | Medieval Kingdom: Stadt, 10 NPCs, Dungeon |
| 14 | **KI-Integration** | Text-to-3D via Meshy API im Editor |

**Asset-Ökosystem:** Du nutzt Sketchfab/PolyHaven manuell.  
**Stil-Harmonisierung:** Nicht vor Monat 20. Einheitlicher Stil durch Auswahl.

---

## Phase 5: Module & Monetarisierung (Monat 15–17)

| Monat | Ziel | Deliverable |
|-------|------|-------------|
| 15 | **Economy + Shop** | Auktionshaus, In-Game-Währung, Stripe |
| 16 | **Gilden + Housing** | Ränge, Gildenbank, instanziertes Haus |
| 17 | **Events + Seasonal** | Doppel-XP-Wochenende, World-Boss |

**Monetarisierung:** Cosmetic-Only. Kein P2W.  
**Ziel:** Erste €100 Umsatz durch Freunde und Early-Adopter.

---

## Phase 6: Beta & Polishing (Monat 18–21)

| Monat | Ziel | Deliverable |
|-------|------|-------------|
| 18 | **Closed Beta** | 50 Spieler, Discord, Bug-Tracker |
| 19 | **Performance** | 60 FPS GTX 1060, 60 Hz Server-Tick |
| 20 | **Stil-Harmonisierung** | ML-Transfer für importierte Assets |
| 21 | **Dokumentation** | API-Docs, 10 Tutorials |

---

## Phase 7: Plattform-Shift (Monat 22–24)

> *Jetzt extrahierst du dein Spiel in eine Plattform.*

| Monat | Ziel | Deliverable |
|-------|------|-------------|
| 22 | **Modul-Marktplatz** | Andere Devs können Module hochladen |
| 23 | **One-Click Publish** | Editor → Live-Server in 10 Min |
| 24 | **Open Beta / Launch** | 1000 Downloads, 100 aktive Welten |

---

## Solo-Dev Regeln

1. **Nicht vor Monat 12 an Cloud-Scaling denken.**  
2. **Nicht vor Monat 18 an KI-Hub denken.**  
3. **Jede Feature-Idee durch den Filter: "Braucht MEIN Spiel das JETZT?"**  
4. **Freitags sind Bugfix-Tage. Keine neuen Features.**  
5. **Jeden Sonntag: 1 Stunde Dokumentation schreiben.**  
