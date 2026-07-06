# ChangeLog 0004 – Woche 4: Netzwerk-Prototyp

**Datum:** 2026-07-01  
**Version:** 0.1.3 → 0.1.4

## Hinzugefuegt
- Custom UDP Protokoll mit 16-Byte Header
- Sequence-Nummern, ACK, ACK-Bits
- Unreliable (0) + Reliable (1) + Control (2) Channels
- Non-blocking Server + Client
- WSA-Refcounting (Thread-safe)

## Gefixt
- WSAStartup/WSACleanup Double-Init Crash
- Fehlende `<unordered_map>` Include

## API
```cpp
auto server = Net::CreateServer();
server->Start(7777, 64);
auto client = Net::CreateClient();
client->Connect("127.0.0.1", 7777);
```

## Tests
- Server Start/Stop
- Client Connect
- Send/Receive Roundtrip

## Dateien
- `src/core/net.hpp`
- `src/core/net.cpp`
- `tests/test_net.cpp`
