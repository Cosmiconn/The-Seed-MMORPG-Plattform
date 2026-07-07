# ChangeLog 0009 – Woche 9: Forward+/Deferred Hybrid Renderer – Teil 1

**Datum:** 2026-07-07
**Version:** 0.2.0 → 0.2.1
**Phase:** 1 (Engine-Kern)

---

## Zusammenfassung

Aufbau der Rendering-Pipeline-Architektur fuer den Forward+/Deferred Hybrid Renderer.

## Hinzugefuegt

- G-Buffer Attachment System (4 Targets)
- Geometry Pass (Deferred)
- Lighting Pass (Deferred) mit Directional + 64 Point Lights
- Descriptor Set Layout fuer Lighting
- MaterialLibrary Integration fuer Deferred Rendering
- Inline SPIR-V Shader

## Performance

| Metrik | Wert |
|--------|------|
| G-Buffer Memory | ~44 MB @ 1080p |
| Geometry Pass | < 2ms (1000 Cubes) |
| Lighting Pass | < 1ms (1 Dir + 8 Point) |
| Total Frame | < 4ms |
