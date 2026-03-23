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

## Design Decisions (Accepted)

- **`ZDD_Random` uses `std::rand()`** (`src/zdd_ops.cpp`).
  Not thread-safe, but the entire library is documented as single-threaded.

- **`bddnull` value (0x7FFFFFFFFFFF) overlaps the theoretical node ID space** (`include/bdd_types.h`).
  Would require ~70 trillion allocated nodes to collide. Not practically
  reachable.

- **`BDD(42)` / `ZDD(42)` accept arbitrary integer values without validation** (`python/src/kyotodd/_binding.cpp`).
  The C++ constructors map any non-negative value to true/single. This is
  consistent with C-style boolean semantics.

- **`bddprime` 系 API は 65536 (2^16) 変数までに制限されている** (`src/bdd_base.cpp`, `src/bdd_class.cpp`, `src/bdd_ops.cpp`).
  ノード ID の 31-bit variable number フィールドはより広い範囲を格納できるが、
  `bddprime()` の内部 prime テーブルは 2^16 エントリで固定サイズであり、
  これを超える自動拡張は意図的にサポートしていない。`BDD::prime` /
  `prime_not` / `cube` / `clause`、単変数版 `Exist` / `Univ`、`bddswap` は
  すべて `bddprime()` に依存するため、同じ上限が適用される。65536 を超える
  変数を扱う場合は `BDD::getnode()` で直接ノードを構築すること。

- **`bddnewvaroflev()` invalidates existing DDs and external memo objects**
  (`src/bdd_base.cpp`).
  `bddnewvaroflev()` inserts a new variable at an arbitrary level, shifting
  the levels of all existing variables at or above the target level. This
  changes the variable-level mapping globally, which means the internal
  structure of every previously constructed BDD/ZDD/QDD may no longer be
  consistent with the new mapping. The library clears the operation cache
  automatically, but it does **not** rebuild existing DDs or invalidate
  user-held `BddCountMemo` / `ZddCountMemo` objects. Callers must treat
  `bddnewvaroflev()` as a global state mutation: discard any external memo
  objects and be aware that operations on previously constructed DDs may
  produce results under the new level ordering, not the one that was in
  effect when those DDs were built. This is by design — the cost of
  automatically detecting or correcting all affected objects would be
  prohibitive.

## C++ Only APIs (Not Exposed to Python)

The following public C++ APIs are intentionally not provided in the Python
binding.

- **`BDD::getnode_raw` / `ZDD::getnode_raw` / `QDD::getnode_raw` / `UnreducedDD::getnode_raw`** — Internal low-level node constructors without validation. The safe `getnode()` with level validation is available in Python.

- **`BDD::cache_get/cache_put` / `ZDD::cache_get/cache_put` / `QDD::cache_get/cache_put`** — Direct operation cache manipulation. This is an internal optimization detail; misuse can cause correctness issues.

- **`RotPiDD::normalizePerm()`** — Rank-normalizes a permutation vector in place. In Python, `rotpidd_from_perm()` calls this automatically.

- **`SeqBDD::print()`** — Prints the internal ZDD structure to stdout. In Python, use `print_seq()`, `seq_str()`, or access the internal ZDD via the `.zdd` property.

- **PiDD / RotPiDD internal level tables are not updated by `bddnewvaroflev()`** (`include/pidd.h`, `include/rotpidd.h`, `src/bdd_base.cpp`).
  `PiDD_XOfLev` / `PiDD_LevOfX` and `RotPiDD_XOfLev` / `RotPiDD_LevOfX` are
  only built during `PiDD_NewVar()` / `RotPiDD_NewVar()` calls.
  `bddnewvaroflev()` updates the global `var2level` / `level2var` arrays but
  does not propagate to PiDD / RotPiDD tables, causing `TopX()`, `TopY()`,
  `Swap()`, `Cofact()`, and other operations on existing objects to return
  incorrect results. Do not call `bddnewvaroflev()` while PiDD / RotPiDD
  objects are in use.
