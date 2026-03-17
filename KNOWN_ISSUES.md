# Known Issues and Deferred Items

Items identified in the code review of 2026-03-17 that were intentionally
left unfixed. This document exists to prevent re-flagging in future reviews.

## Architecture: GC Root Management Performance

- **`gc_roots()` uses `std::unordered_set` — hash set insert/erase on every BDD/ZDD construct/destruct** (`include/bdd_types.h`)
- **Move constructors cannot be `noexcept`** because `bddgc_protect` calls `unordered_set::insert` which may throw (`include/bdd_types.h`)

These two issues share the same root cause. Fixing requires replacing the
GC root data structure (e.g. with an intrusive linked list), which changes
the BDD/ZDD class layout and is a significant architectural change.

## Legacy API Compatibility

- **`ZDD_LCM_A/C/M` take `char*` instead of `const char*`** (`include/bdd_ops.h`).
  These are unimplemented stubs retained for API compatibility. Changing the
  signature would break source compatibility for existing callers.

- **`CardMP16` returns `malloc`-allocated `char*`** (`include/bdd_types.h`).
  Legacy C-style API. The safe alternative `bddexactcount()` returning
  `BigInt` already exists.

- **`ZDD::PrintPla()`, `ZDD::ZLev()`, `ZDD::SetZSkip()` are declared but unimplemented** (`include/bdd_types.h`).
  Retained intentionally for future implementation. They throw
  `std::logic_error` if called.

## Performance Optimizations (Not Yet Implemented)

- **`bddremainder` does not cache results** (`src/zdd_ops.cpp`).
  Repeated calls with the same arguments recompute from scratch.

- **`bddlshift` / `bddrshift` traverse the BDD twice** (`src/bdd_ops.cpp`).
  Once for `bddsupport_collect` validation, then again for the actual shift.

- **GC mark array uses `uint8_t` per node** (`src/bdd_base.cpp`).
  A bit vector would reduce memory usage to 1/8.

## Design Decisions (Accepted)

- **`ZDD_Random` uses `std::rand()`** (`src/zdd_ops.cpp`).
  Not thread-safe, but the entire library is documented as single-threaded.

- **`bddnull` value (0x7FFFFFFFFFFF) overlaps the theoretical node ID space** (`include/bdd_types.h`).
  Would require ~70 trillion allocated nodes to collide. Not practically
  reachable.

- **`BDD(42)` / `ZDD(42)` accept arbitrary integer values without validation** (`python/src/kyotodd/_binding.cpp`).
  The C++ constructors map any non-negative value to true/single. This is
  consistent with C-style boolean semantics.
