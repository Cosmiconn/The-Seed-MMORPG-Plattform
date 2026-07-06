# ChangeLog 0006 – Woche 6: Serialisierung + Delta

**Datum:** 2026-07-01  
**Version:** 0.1.5 → 0.1.6

## Hinzugefuegt
- BinarySerializer (WriteU32, WriteF32, WriteString, Write bytes)
- BinaryDeserializer (ReadU32, ReadF32, ReadString, Read bytes)
- Delta-Kompression via XOR
- ApplyDelta fuer State-Rekonstruktion

## API
```cpp
auto delta = Serialize::ComputeDelta(oldState, newState);
auto reconstructed = Serialize::ApplyDelta(oldState, delta);
```

## Dateien
- `src/core/serialize.hpp`
- `src/core/serialize.cpp`
