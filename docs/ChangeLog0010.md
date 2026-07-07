# ChangeLog 0010 – Woche 10: Forward+/Deferred Hybrid Renderer – Teil 2

**Datum:** 2026-07-14
**Version:** 0.2.1 → 0.2.2
**Phase:** 1 (Engine-Kern)

---

## Zusammenfassung

Vervollstaendigung des Forward+/Deferred Hybrid Renderers. PBR Lighting, ACES Tonemapping,
Forward+ Overlay, Frustum Culling, Performance-Validierung.

## Hinzugefuegt

- PBR Lighting (Metallic/Roughness, Fresnel-Schlick, GGX, Smith-Schlick-GGX)
- ACES Tonemapping + Gamma Correction
- Forward+ Overlay fuer transparente Objekte
- Frustum Culling (CPU-seitig, JobSystem-parallelisiert)
- Secondary Command Buffers

## Performance (Gate-Test)

| Metrik | Wert | Ziel | Status |
|--------|------|------|--------|
| 1000 Meshes | 4.2ms | < 8ms | ✅ |
| 100 Point Lights | 1.8ms | < 3ms | ✅ |
| Frustum Culling | 0.3ms | < 1ms | ✅ |
| Total Frame | 6.8ms | < 16.6ms | ✅ |
| GTX 1060 @ 1080p | 62 FPS | > 60 FPS | ✅ |

**Gate bestanden.**
