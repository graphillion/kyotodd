# KyotoDD - BDD Library in C++

## Rules

- Implement incrementally, step by step, so the user can follow and understand every change.
- Never decide unclear specifications on your own — always ask the user first.
- Commit to git after every change.
- Always write tests after implementing a feature.
- Do not remove or modify existing comments in the code unless explicitly asked to do so.
- Keep each source file within 2000 lines. If a file exceeds 2000 lines, split it into multiple files along a natural boundary (e.g., by feature or operation category).
- Recursive DD operations MUST be implemented in two versions: a `_rec` function (recursive) and an `_iter` function (iterative with explicit stack). The public wrapper MUST dispatch between them using the same condition as `bddand` (via `use_iter_1op` / `use_iter_2op` / `use_iter_3op` against `BDD_RecurLimit`), and MUST expose the same `BddExecMode` argument (`Auto` / `Recursive` / `Iterative`) so callers can force a specific mode. See the `_rec` / `_iter` separation pattern section for details.

## Naming conventions

- New C++/Python member functions and methods must use lowercase snake_case (e.g. `exact_count`, `raw_size`).

## Build

- Build system: CMake. Library target: `kyotodd_lib`. Each app under `app/` and each test under `test/` builds as its own executable (see `CMakeLists.txt`). Tests use Google Test.
- C++ standard: C++11 (must compile with a C++11 compiler).

## API reference

Do NOT duplicate API listings in this file. For class/function details, consult:

- Header files under `include/` — authoritative declarations with comments.
- Sphinx documentation under `docs/` (published at https://graphillion.github.io/kyotodd/).
- `KNOWN_ISSUES.md` — intentionally unfixed items; do not re-flag these in reviews.

## Concepts

- **Variable number**: identifies a Boolean variable; starts from 1; stored in 31 bits of a `BddNode`.
- **Level**: node position in the DD; terminals are level 0, non-terminal levels start from 1. Variables and levels are distinct concepts connected by a configurable mapping (default: var i ↔ level i).

## Node ID encoding (48 bits)

- Bit 47 = 1 → terminal (constant). Constant 0 = `0x800000000000`, constant 1 = `0x800000000001` (the encoding supports arbitrary constant values for multi-terminal DDs).
- Bit 47 = 0 → regular node; bit 0 is the complement-edge flag. Regular node IDs are even; odd ID N means node N-1 with a complement edge. Node array index = `node_id / 2 - 1`.
- Reduced flag: bit 32 of `data[0]` in `BddNode` (`BDD_NODE_REDUCED_FLAG`, `include/bdd_node.h`). Set by `getnode_raw` when both children are reduced; never set by `UnreducedDD::getnode`.

## Complement edge semantics

The stored lo child is always non-complemented; node creation absorbs a complemented lo into the returned edge. BDD and ZDD differ in how the complement propagates through the node:

- **BDD**: a complement edge negates the whole function: `~(var, lo, hi) = (var, ~lo, ~hi)`. Traversing a complemented BDD/QDD edge flips **both** children.
- **ZDD**: a complement edge toggles membership of the empty set (∅), which propagates only along the 0-edge: `~(var, lo, hi) = (var, ~lo, hi)`. Traversing a complemented ZDD edge flips **lo only**.

## Node creation

- `BDD::getnode` / `ZDD::getnode` / `QDD::getnode` apply the type's reduction rule (BDD: jump rule `lo == hi` → lo; ZDD: zero-suppression `hi == bddempty` → lo; QDD: no jump rule) plus level validation. The `getnode_raw` variants skip validation.
- `UnreducedDD::getnode` applies no reduction, no complement normalization, and no unique-table insertion (not canonical).

## Garbage collection

- Mark-and-sweep with free-list reuse; live node IDs never change. The operation cache is cleared on sweep.
- BDD/ZDD/QDD objects auto-register their `root` as a GC root (`bddgc_protect` in constructors, `bddgc_unprotect` in destructors).
- Node allocation priority: free list → array extend → array grow → GC → `std::overflow_error`.
- Unique table: per-variable open-addressing hash table (linear probing, doubling at 75% load factor).
- Operation cache: fixed-size lossy computed table; 2-operand (`bddrcache` / `bddwcache`) and 3-operand (`bddrcache3` / `bddwcache3`) APIs plus per-class typed wrappers (`cache_get` / `cache_put`).

## `_rec` / `_iter` separation pattern

Each recursive operation is split into three parts:

- **Public wrapper** (e.g. `bddand`): terminal fast paths, normalization, dispatch to `_rec` or `_iter`, wrapped in `bdd_gc_guard(...)`. Defaults to `BddExecMode::Auto`, which picks `_iter` when the relevant levels exceed `BDD_RecurLimit` (8192) via `use_iter_1op` / `use_iter_2op` / `use_iter_3op` (`include/bdd_internal.h`). Callers can force `Recursive` / `Iterative` (shorthand constants: `BDD_EXEC_AUTO` / `BDD_EXEC_RECURSIVE` / `BDD_EXEC_ITERATIVE`).
- **`_rec` function**: recursive implementation with cache lookup and cofactoring. Same-file calls use `_rec` directly (zero overhead); cross-file calls go through public wrappers. Internal `_rec` functions take no mode argument.
- **`_iter` function**: explicit-stack (`std::vector<IterFrame>`) implementation sharing the same op code / cache slots as `_rec`, so results are interchangeable. PRECONDITION: the caller has entered `bdd_gc_guard`. Non-template `_iter` functions live in dedicated `src/<module>_iter.cpp` files; template `_iter` functions live in headers alongside their `_rec` counterparts.
- `BDD_RecurGuard` (RAII) enforces the recursion depth limit; the `bdd_gc_guard` template (`include/bdd_internal.h`) pre-checks GC and retries on `overflow_error` at the outermost level.

## DD class overview

```
DDBase (include/dd_base.h)
├── BDD  (include/bdd_class.h)   — reduced BDD with complement edges
├── ZDD  (include/zdd_class.h)   — family of sets; complement edge toggles ∅
├── QDD  (include/qdd.h)         — quasi-reduced (no jump rule); BDD complement semantics
├── MVBDD / MVZDD (include/mvdd.h) — multi-valued variables via one-hot encoding over BDD/ZDD
└── UnreducedDD (include/unreduced_dd.h) — no reduction rules; reduce_as_bdd() / reduce_as_zdd() / reduce_as_qdd()
```

- SeqBDD (`include/seqbdd.h`), PiDD (`include/pidd.h`), RotPiDD (`include/rotpidd.h`) do NOT inherit from DDBase; each wraps a ZDD by composition.
- `MTBDD<T>` / `MTZDD<T>` (`include/mtbdd.h`, split into `mtbdd_core.h` / `mtbdd_class.h` / `mtbdd_io.h`): multi-terminal templates without complement edges. Pre-instantiated: `MTBDDFloat` / `MTBDDInt` / `MTZDDFloat` / `MTZDDInt`.
- Memo classes for repeated queries: `BddCountMemo`, `ZddCountMemo`, `CostBoundMemo`, `WeightedSampleMemo`.
- Python bindings: `python/src/kyotodd/_binding.cpp` (pybind11). C++-only (not bound): BDDV, ZBDDV, MLZBDDV, BtoI, CtoI, SOP/SOPV.

## I/O formats

Binary (single- and multi-root), Sapporo text, Graphillion (ZDD interop), Graphviz DOT, self-contained SVG (`DrawMode::Expanded` expands complement edges; `DrawMode::Raw` shows edge markers), Knuth (deprecated), and legacy Sapporo (`bddexport` / `bddimport`). Declarations in `include/bdd_io.h` and `include/svg_export.h`.

## Repository layout

- `include/` — public and internal headers. `include/bdd.h` is the umbrella header.
- `src/` — implementation. Conventions: `<module>_iter.cpp` holds the iterative `_iter` counterparts of `<module>.cpp`; large modules are split by category (e.g. `zdd_adv_count` / `zdd_adv_filter` / `zdd_adv_rank` / `zdd_adv_weight`, `bdd_io_*`).
- `test/` — Google Test suites, split per feature area.
- `app/` — sample applications, one directory per app.
- `python/` — pybind11 bindings and Python tests.
- `docs/` — Sphinx documentation (API reference, tutorials, concepts).
- `review*.md`, `log/`, `conversation/`, `build/` — gitignored working files (review notes, conversation logs, build artifacts).
