# TheSeed – Pitch Deck
## "Der WordPress für MMORPGs"

---

## Folie 1: Das Problem

**MMORPGs bauen ist elitär.**

- 3–7 Jahre Entwicklungszeit
- Teams von 50–200 Personen
- Budgets von $10M–$500M
- Solo-Entwickler? Unmöglich.

> **Die Welt verliert tausende großartige Welten, weil die Technologie die Visionäre aussperrt.**

---

## Folie 2: Die Lösung

**TheSeed – Die Plattform, die jedem erlaubt, seine eigene persistente Online-Welt zu erschaffen.**

- **Zero-Code Editor:** Drag & Drop, Node-Editor, Schieberegler
- **One-Click Publish:** Live-Server in <10 Minuten
- **KI-Assistent:** Text-Prompt → komplette Asset-Sets
- **Cloud-First:** 0 Spieler = 0 Kosten

> **"Von der Idee zum Live-Server in 48 Stunden."**

---

## Folie 3: Das Produkt

### Vier Säulen:

1. **TheSeed Engine** (C++23)
   - Vulkan/DX12, virtuelle Geometrie, 60 FPS in 100-Spieler-Schlachten
   - Authoritative Server, deterministische ECS, Lag-Kompensation

2. **TheSeed Editor** (Zero-Code IDE)
   - Visueller Welt-Editor, Node-Editor für Quests
   - KI-Panel für 3D, Texturen, Musik, Voice-Over

3. **TheSeed Cloud** (Serverless)
   - Auto-Scaling, Global Deploy, Anti-Cheat, Backups
   - Pay-as-you-grow: 0 Spieler = 0 €

4. **TheSeed Exchange** (Asset-Ökosystem)
   - KI-Generierung + Marktplatz-Import + Community-Share
   - Stil-Harmonisierung: Alles sieht wie aus einem Set aus

---

## Folie 4: Der Markt

| Segment | Marktgröße | Wachstum |
|---------|-----------|----------|
| Game Engines | $4.5B (2024) | 8.5% CAGR |
| Cloud Gaming | $6.2B (2024) | 45% CAGR |
| UGC/Metaverse | $12B (2024) | 35% CAGR |
| **MMORPG-Specific** | **~$2B Nische** | **Explosiv** |

**Zielgruppen:**
- Solo-Träumer (keine Programmierung)
- Hobby-Entwickler (Freunde spielen)
- Indie-Teams (2–5 Personen, kommerziell)
- Lehrer (interaktive Bildungswelten)
- AAA-Veteranen (schnelle Prototypen)
- Content-Creator (Welt als Plattform)

---

## Folie 5: Das Geschäftsmodell

### Entwickler-Seite (B2B2C)

| Tier | Preis | Enthält |
|------|-------|---------|
| **Free** | €0 | 100 CCU, 1 Region, 10GB |
| **Indie** | €12/Monat | 1.000 CCU, 3 Regionen, 100GB |
| **Pro** | €49/Monat | 10.000 CCU, Global, 1TB |
| **Enterprise** | Custom | Dedicated, SLA, White-Label |

### Revenue-Share
- **In-Game:** 10% TheSeed, 90% Entwickler
- **Asset-Store:** 15% TheSeed, 85% Creator
- **KI-API:** Zu Selbstkosten (kein Aufschlag)

### Monetarisierung im Spiel
- Free-to-Play, Buy-to-Play, Subscription, Cosmetic-Only, Trading-Tax

---

## Folie 6: Die Technologie

**Warum C++23?**
- Modules → <5 Min Compile-Zeit
- Coroutines → Async Netzwerk ohne Callback-Hölle
- Ranges → ECS-Queries wie `world.query<Transform, Health>().each(...)`

**Warum Vulkan/DX12?**
- GPU-Driven Rendering, virtuelle Geometrie, dynamisches GI
- 1000+ Meshes bei 60 FPS auf GTX 1060

**Warum Custom Netzwerk?**
- Authoritative Server, deterministisch, <50KB/s pro Spieler
- Lag-Kompensation, Interest Management, Auto-Sharding

**Warum KI-First?**
- Text-to-3D in <5 Minuten (Meshy, Tripo3D)
- Stil-Harmonisierung via ML
- Der Entwickler denkt in Welten, nicht in Polygonen

---

## Folie 7: Der Wettbewerb

| Produkt | Stärke | Schwäche | TheSeed-Vorteil |
|---------|--------|----------|---------------|
| **Unreal Engine 5** | AAA-Rendering | Kein MMORPG-Netcode | Zero-Code + Cloud |
| **Unity** | Flexibel | Chaos, Lizenz-Drama | Deterministisch + Open |
| **Godot** | Open Source | Kein Netzwerk, kein MMORPG | Produktionsreif |
| **Roblox** | UGC, Community | Kindlich, limitiert | Erwachsene, offene Welten |
| **Core (Manticore)** | Einfach | Epic-Shutdown-Risiko | Unabhängig, C++23 |
| **HeroEngine** | MMORPG-Fokus | Veraltet, tot | Modern, KI, Cloud |

> **TheSeed ist die einzige Plattform, die MMORPG-spezifisch, Zero-Code, KI-integriert und Cloud-native ist.**

---

## Folie 8: Die Roadmap

| Phase | Zeit | Meilenstein |
|-------|------|-------------|
| **P0** | Monat 1–3 | Prototyp: 100 Cubes, Netzwerk, ECS |
| **P1** | Monat 4–9 | Engine-Kern: Rendering, Netzwerk, Physics |
| **P2** | Monat 10–14 | Editor: Terrain, Nodes, Hot-Reload |
| **P3** | Monat 15–18 | Eigenes MMORPG: Module, Content, Alpha |
| **P4** | Monat 19–22 | Cloud: One-Click Publish, Auto-Scaling |
| **P5** | Monat 23–27 | Plattform: Templates, Exchange, Monetarisierung |
| **P6** | Monat 28–32 | Beta: 100 Entwickler, Performance, QA |
| **P7** | Monat 33–36 | Launch: 1.000 Welten, €1M ARR |

**Solo-Entwickler:** 18 Monate bis zum eigenen MMORPG + Live-Plattform

---

## Folie 9: Das Team

**Phase 1 (Solo):**
- Cosmiconn – Lead Engine, Graphics, Network, Gameplay

**Phase 2 (Erweitern auf 3–5):**
- + Tools Programmer / UX Designer
- + DevOps / Backend Engineer
- + Content Designer

**Phase 3 (Erweitern auf 8–12):**
- + AI/ML Engineer
- + QA / Community Manager
- + Marketing / Sales

**Minimal-Viable-Team:** 3 Personen (Engine, Tools, Backend)

---

## Folie 10: Die Ask

**Gesucht:** Seed-Runde / Angel-Investor

| Posten | Betrag | Verwendung |
|--------|--------|------------|
| Entwicklung | €150.000 | 12 Monate Solo + erste Freelancer |
| Cloud-Infrastruktur | €30.000 | AWS/GCP, KI-APIs, CDN |
| Marketing | €20.000 | Content-Creator, Ads, Events |
| **Gesamt** | **€200.000** | **18 Monate Runway** |

**Gegenleistung:** 10–15% Equity

**Ziele nach 18 Monaten:**
- 1.000 veröffentlichte Welten
- 10.000 aktive Entwickler
- €1M ARR (Jahr 3)
- Exit-Möglichkeit: Acquisition durch Epic, Unity, Microsoft, oder IPO

---

## Folie 11: Warum Jetzt?

1. **KI-Revolution:** Text-to-3D, Text-to-Texture, Text-to-Music – erst jetzt produktionsreif
2. **Cloud-Native:** Kubernetes, Serverless, Auto-Scaling – erst jetzt erschwinglich
3. **MMORPG-Renaissance:** Palworld, Enshrouded, Valheim – Beweis, dass die Nachfrage da ist
4. **Solo-Entwickler-Boom:** Vampire Survivors, Stardew Valley, Terraria – Ein Mensch kann alles

> **Die Technologie ist reif. Die Nachfrage ist da. Es fehlt nur die Brücke.**

---

## Folie 12: Kontakt

**Cosmiconn**  
Founder & Lead Developer, TheSeed

🌐 theseed.world  
📧 contact@theseed.world  
🐦 @TheSeedEngine  
💬 Discord: discord.gg/theseed

> **"TheSeed ist der Garten, in dem jeder seine Welt pflanzen kann."**
