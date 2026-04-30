# Known Issues and Deferred Items

Items identified in the code review of 2026-03-17 that were intentionally
left unfixed. This document exists to prevent re-flagging in future reviews.

## Architecture: GC Root Management Performance

- **`gc_roots()` uses `std::unordered_set` — hash set insert/erase on every BDD/ZDD construct/destruct** (`src/bdd_base.cpp`)
- **Move constructors cannot be `noexcept`** because `bddgc_protect` calls `unordered_set::insert` which may throw (`include/dd_base.h`)

These two issues share the same root cause. Fixing requires replacing the
GC root data structure (e.g. with an intrusive linked list), which changes
the BDD/ZDD class layout and is a significant architectural change.

## Legacy API Compatibility

- **`ZDD_LCM_A/C/M` take `char*` instead of `const char*`** (`include/zdd_ops.h`).
  These are unimplemented stubs retained for API compatibility. Changing the
  signature would break source compatibility for existing callers.

- **`CardMP16` returns `malloc`-allocated `char*`** (`include/zdd_class.h`).
  Legacy C-style API. The safe alternative `bddexactcount()` returning
  `BigInt` already exists.

## Design Decisions (Accepted)

- **`ZDD_Random` uses `std::rand()`** (`src/zdd_ops.cpp`).
  Not thread-safe, but the entire library is documented as single-threaded.

- **`bddnull` value (0x7FFFFFFFFFFF) overlaps the theoretical node ID space** (`include/bdd_types.h`).
  Would require ~70 trillion allocated nodes to collide. Not practically
  reachable.

- **`BDD(42)` / `ZDD(42)` accept arbitrary integer values without validation** (`include/dd_base.h`, `python/src/kyotodd/_binding.cpp`).
  The C++ `DDBase(int val)` constructor maps any non-negative value to
  true/single. This is consistent with C-style boolean semantics.

- **`bddprime` family of APIs is limited to 65536 (2^16) variables**
  (`src/bdd_base.cpp`, `src/bdd_class.cpp`, `src/bdd_ops.cpp`).
  Although the 31-bit variable number field in node IDs can represent a
  much wider range, the internal prime table used by `bddprime()` is
  fixed at 2^16 entries and intentionally does not auto-expand beyond
  that. `BDD::prime` / `prime_not` / `cube` / `clause`, single-variable
  `Exist` / `Univ`, and `bddswap` all depend on `bddprime()`, so the
  same upper bound applies. To work with more than 65536 variables,
  construct nodes directly via `BDD::getnode()`.

- **`bddchange` is limited to 65536 (2^16) variables**
  (`src/zdd_ops.cpp`).
  `bddchange()` auto-expands variables up to the requested variable
  number but rejects any variable exceeding 65536. This is the same
  2^16 ceiling as `bddprime`. To toggle membership of higher-numbered
  variables, construct nodes directly via `ZDD::getnode()`.

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

- **`RotPiDD::normalizePerm()`** — Rank-normalizes a permutation vector in place. In Python, `rotpidd_from_perm()` calls this automatically.

- **`SeqBDD::print()`** — Prints the internal ZDD structure to stdout. In Python, use `print_seq()`, `seq_str()`, or access the internal ZDD via the `.zdd` property.

- **`ZDD::CardMP16()`** — Legacy C-style cardinality function returning `malloc`-allocated `char*`. In Python, use `count()` or `exact_count` property instead.

- **`BDD::XPrint0()` / `BDD::XPrint()` / `ZDD::XPrint()`** — Deprecated stubs that always throw `std::logic_error`. Use `save_graphviz_str()` / `save_graphviz_file()` instead.

- **PiDD / RotPiDD internal level tables are not updated by `bddnewvaroflev()`** (`include/pidd.h`, `include/rotpidd.h`, `src/bdd_base.cpp`).
  `PiDD_XOfLev` / `PiDD_LevOfX` and `RotPiDD_XOfLev` / `RotPiDD_LevOfX` are
  only built during `PiDD_NewVar()` / `RotPiDD_NewVar()` calls.
  `bddnewvaroflev()` updates the global `var2level` / `level2var` arrays but
  does not propagate to PiDD / RotPiDD tables, causing `TopX()`, `TopY()`,
  `Swap()`, `Cofact()`, and other operations on existing objects to return
  incorrect results. Do not call `bddnewvaroflev()` while PiDD / RotPiDD
  objects are in use.

## MTBDD / MTZDD Limitations

- **`MTBDD::apply` / `MTZDD::apply` cache key does not include functor state**
  (`include/mtbdd_core.h`).
  The operation cache is keyed on `(cache_op, f, g)` where `cache_op` is a
  `static uint8_t` per `BinOp` type. Stateful functors (e.g. structs with
  different member values but the same type) share the same op code, so the
  cache cannot distinguish between different instances. Calling `apply` with
  `PlusBias{1}` followed by `PlusBias{2}` on the same operands will return
  the cached result of the first call. Workaround: use distinct types (e.g.
  template parameters or separate lambdas) for functors with different state.

- **MTZDD terminal condition loses zero-suppressed variable information in ITE**
  (`include/mtbdd_core.h`).
  When an MTZDD condition is reduced to a terminal by zero-suppression (e.g.
  `MTZDD::ite(v, zero, x)` reduces to `terminal(x)`), the `cond.ite(then,
  else)` wrapper's terminal fast path cannot recover the lost variable. The
  condition is treated as unconditionally non-zero for all assignments. This
  is a fundamental consequence of ZDD zero-suppression: once a variable is
  eliminated from the structure, the ITE cannot consult it. The recursive
  `mtzdd_ite_rec` correctly handles zero-suppressed variables when the
  condition is non-terminal.

- **`bddfinal()` cannot detect MTBDD/MTZDD terminals at index 1**
  (`src/bdd_base.cpp`).
  MTBDD terminal index 1 (`BDD_CONST_FLAG | 1`) is the same bddp value as
  `bddtrue`, which is used by BDD/ZDD static constants. `bddfinal()` rejects
  non-standard terminals with index >= 2 but cannot distinguish index 1 from
  `bddtrue`. If the first non-zero value registered in an MTBDD terminal
  table is the only non-standard terminal alive, `bddfinal()` will not reject
  it and the terminal will be silently invalidated.
