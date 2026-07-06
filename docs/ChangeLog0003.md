# ChangeLog 0003 – Woche 3: Job-System

**Datum:** 2026-07-01  
**Version:** 0.1.2 → 0.1.3

## Hinzugefuegt
- Work-Stealing Thread-Pool
- Lock-free(ish) Task-Queues (eine pro Worker)
- LIFO fuer eigene Queue (Cache-freundlich)
- FIFO fuer Stealing (Fairness)
- Atomischer Pending-Task-Counter
- Condition-Variable fuer WaitForAll

## API
```cpp
JobSystem js;
js.Initialize(4);
js.Submit([]{ /* Arbeit */ });
js.SubmitAndWait([]{ /* ... */ });
js.WaitForAll();
```

## Tests
- Initialize/Shutdown
- 1M Tasks ohne Deadlock
- Parallel Sum Korrektheit
- Nested Tasks
- SubmitAndWait
- Fallback wenn nicht initialisiert

## Dateien
- `src/core/jobs.hpp`
- `src/core/jobs.cpp`
- `tests/test_jobs.cpp`
