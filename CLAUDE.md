# KyotoDD - BDD Library in C++

## Rules

- Implement incrementally, step by step, so the user can follow and understand every change.
- Never decide unclear specifications on your own — always ask the user first.
- Commit to git after every change.
- Always write tests after implementing a feature.
- Do not remove or modify existing comments in the code unless explicitly asked to do so.

## Naming conventions

- New C++/Python member functions and methods must use lowercase snake_case (e.g. `exact_count`, `raw_size`).

## Build

- Build system: CMake
- C++ standard: C++11 (must compile with a C++11 compiler)

## Concepts

- **Variable number**: Identifies a Boolean variable. Starts from 1 (1, 2, 3, ...). Stored in 31 bits of a BddNode.
- **Level**: Determines the position of a node in the BDD. Terminal nodes have level 0. Non-terminal levels start from 1 upward.
- **Variable-level mapping**: Variable numbers and levels are distinct concepts connected by a configurable mapping. Initial (default) ordering: var i corresponds to level i.

## Node ID encoding (48 bits)

A node ID is a 48-bit value. The MSB (bit 47) and LSB (bit 0) have special meanings:

- **Bit 47 (constant flag)**: If 1, the remaining bits represent a constant value (terminal node). For BDD/ZDD the constants are 0 and 1, but the encoding supports arbitrary constant values for multi-terminal BDDs.
  - Constant 0: `0x800000000000` (bit 47 = 1, value = 0)
  - Constant 1: `0x800000000001` (bit 47 = 1, value = 1)
- **Bit 47 = 0 (regular node)**: The ID refers to a non-terminal node in the node array.
  - **Bit 0 (complement flag)**: If 1, the edge is a complement (negated) edge. The actual node is at ID - 1 (the even number). If 0, the edge is regular.
  - Regular node IDs are always even (2, 4, 6, ...).
  - Odd node ID N means: node N-1 with complement edge.
  - Node array index: `node_id / 2 - 1` (node ID 2 → index 0, node ID 4 → index 1, ...).

## Unique table

- Per-variable hash table using open addressing with linear probing.
- Struct `BddUniqueTable`: `slots` (array of node IDs, 0 = empty), `capacity` (power of 2), `count`.
- Hash function: splitmix64-style mix on (lo, hi) key.
- Initial capacity: 1024 slots per variable. Doubles on resize.
- Load factor threshold: 75% (`count * 4 >= capacity * 3`).
- `BDD_UniqueTableLookup(var, lo, hi)`: returns node ID or 0 if not found.
- `BDD_UniqueTableInsert(var, lo, hi, node_id)`: inserts node, resizes if needed.

## DD node creation

- BDD: `BDD::getnode(var, lo, hi)` — Creates a new node applying BDD reduction rules. Includes level validation.
- ZDD: `ZDD::getnode(var, lo, hi)` — Creates a new node applying ZDD reduction rules. Includes level validation.
- QDD: `QDD::getnode(var, lo, hi)` — Creates a QDD node (no jump rule). Includes level validation.
- `BDD::getnode_raw` / `ZDD::getnode_raw` / `QDD::getnode_raw` — Internal/advanced version without validation.

## Complement edge semantics

Both BDD and ZDD use complement edges (bit 0 of a node ID) to represent negation without allocating extra nodes. The normalization rule ensures that the lo child stored in a node is always non-complemented (bit 0 = 0). When the caller passes a complemented lo, the node creation functions absorb the complement into the returned edge. However, BDD and ZDD differ in how the complement propagates through the node:

### BDD (`BDD::getnode`)

A complement edge negates the entire Boolean function represented by the subtree. If lo is complemented, both children are flipped:

- `~(var, lo, hi) = (var, ~lo, ~hi)`
- Normalization: if lo has bit 0 set, strip it from lo, flip hi, and set bit 0 on the returned node ID.
- When traversing a BDD node with a complement edge, apply `bddnot` to **both** lo and hi.

### ZDD (`ZDD::getnode`)

A complement edge toggles membership of the empty set (∅) in the family. Since ∅ membership propagates only through the lo (0-edge) path, only lo is affected:

- `~(var, lo, hi) = (var, ~lo, hi)`
- Normalization: if lo has bit 0 set, strip it from lo (hi is unchanged), and set bit 0 on the returned node ID.
- When traversing a ZDD node with a complement edge, apply `bddnot` to **lo only**; hi stays unchanged.

## Garbage Collection (GC)

Mark-and-sweep GC with free list reuse. Dead nodes are reclaimed without changing live node IDs.

### Architecture

- **Mark phase**: Traverse all registered GC roots, mark reachable nodes recursively.
- **Sweep phase**: Rebuild unique tables from live nodes. Dead nodes go to a free list. Cache is cleared.
- **Free list**: Dead node slots are linked via `data[0]`. New node allocation checks free list first.

### Node allocation priority

1. Free list (fastest — reuse dead node slot)
2. Array extend (`bdd_node_used++`, space available within capacity)
3. Array grow (realloc to double capacity, up to `bdd_node_max`)
4. GC (last resort — only when array cannot grow further)
5. `std::overflow_error` if GC cannot free enough

### GC API

- `bddgc()` — Manual GC invocation. No-op if `bdd_gc_depth > 0`.
- `bddgc_protect(bddp* p)` / `bddgc_unprotect(bddp* p)` — Register/unregister a raw `bddp` pointer as a GC root.
- `bddgc_setthreshold(double)` / `bddgc_getthreshold()` — GC threshold (default 0.9). GC triggers when live nodes exceed `bdd_node_max * threshold` and array is at max capacity.
- `bddlive()` — Number of live nodes (`bdd_node_used - bdd_free_count`).
- `bddgc_rootcount()` — Number of registered GC root pointers.
- `bddfinal()` — Finalize the BDD library and release all resources. Throws `std::runtime_error` if live BDD/ZDD objects still exist.
- `bdd_check_reduced(bddp root)` — Check that all non-terminal nodes reachable from root are reduced.

### Recursion depth guard

- `BDD_RecurGuard` (RAII): Increments `BDD_RecurCount` on construction, decrements on destruction. Throws `std::overflow_error` if the limit is exceeded.
- `BDD_RecurLimit` = 8192 (conservative for typical thread stack sizes).
- `BDD_RecurCount` (extern int): Current recursion depth counter.

### Operation cache

- Fixed-size, lossy computed table using `BddCacheEntry` struct: `fop` (bits [55:48] = op, bits [47:0] = f), `g`, `h` (0 for 2-operand ops), `result`.
- 2-operand API: `bddrcache(op, f, g)` / `bddwcache(op, f, g, result)`.
- 3-operand API: `bddrcache3(op, f, g, h)` / `bddwcache3(op, f, g, h, result)`. Used by ITE.
- Typed wrappers: `BDD::cache_get()` / `BDD::cache_put()`, `ZDD::cache_get()` / `ZDD::cache_put()`, `QDD::cache_get()` / `QDD::cache_put()`.

### `_rec` separation pattern

Each recursive operation is split into a public wrapper and a static `_rec` function:

- **Public wrapper** (e.g. `bddand`): Handles terminal fast-path checks, normalization (argument swap), then wraps the core logic in `bdd_gc_guard(...)`.
- **`_rec` function** (e.g. `bddand_rec`): Contains the actual recursive logic with cache lookup, cofactoring, and recursive calls. Uses `_rec` directly for same-file calls (zero overhead). Cross-file calls go through public wrappers.
- **`bdd_gc_guard` template** (`include/bdd_internal.h`): At outermost level (`bdd_gc_depth == 0`): pre-checks GC, increments depth, executes lambda, catches `overflow_error` for retry. At depth > 0: just increments/decrements depth.

### BDD/ZDD class auto-protection

- BDD/ZDD constructors call `bddgc_protect(&root)`, destructors call `bddgc_unprotect(&root)`.
- Copy/move constructors register the new object's `&root`. Assignment operators only update the value.
- `gc_roots()` uses a leaked singleton (`new std::unordered_set<bddp*>()`) to avoid static initialization/destruction order issues.

## DDBase class

- Base class for all decision diagram types. Defined in `include/dd_base.h`.
- Not intended for polymorphic use; provides shared infrastructure.
- Protected member `root` (bddp / uint64_t): the root node ID.
- Constructors/destructors handle GC root registration (`bddgc_protect`/`bddgc_unprotect`).
- Constructors: default (false), `DDBase(int val)` (0=false, 1=true, negative=null), copy, move.
- Provides static methods: `init()`, `new_var()`, `new_var(int n)`, `var_used()`, `node_count()`, `gc()`, `to_level()`, `to_var()`.
- Provides query methods: `get_id()`, `is_terminal()`, `is_one()`, `is_zero()`, `top()`, `raw_size()`.
- Provides raw child accessors: `raw_child0()`, `raw_child1()`, `raw_child()` (no complement resolution).
- Each subclass (BDD, ZDD, QDD) adds complement-resolving child accessors: `child0()`, `child1()`, `child()` — BDD/QDD flip both lo/hi, ZDD flips lo only.

### Class hierarchy

```
DDBase
├── BDD
├── ZDD
├── QDD
└── UnreducedDD
```

SeqBDD, PiDD, and RotPiDD do NOT inherit from DDBase. They use composition (wrapping a ZDD internally).

## BDD class

- Inherits from DDBase.
- Applies BDD reduction rule: `lo == hi` → return lo (jump rule).
- Uses BDD complement edge semantics (both children flipped).
- Operators: `&` (AND), `|` (OR), `^` (XOR), `~` (NOT), `<<` / `>>` (shift), `==`, `!=`.
- Free functions: `bddnand()`, `bddnor()`, `bddxnor()`, `bddsmooth()`, `bddspread()`.
- Cofactor/quantification: `At0(v)`, `At1(v)`, `Exist()`, `Univ()`, `Cofactor()`, `Swap()`.
- Support: `Support()` (support set as BDD), `SupportVec()` (as vector). `Imply(g)` (implication check).
- ITE: `BDD::Ite(f, g, h)` — static if-then-else.
- Literal constructors: `BDD::prime(v)` (positive literal), `BDD::prime_not(v)` (negative literal).
- Clause/cube constructors: `BDD::cube(lits)` (conjunction), `BDD::clause(lits)` (disjunction). DIMACS sign convention.
- SAT counting: `count(n)` (double), `exact_count(n)` (BigInt), `exact_count(n, BddCountMemo&)`.
- Sampling: `uniform_sample(rng, n, BddCountMemo&)` — uniformly sample one satisfying assignment.
- Constants: `BDD::False`, `BDD::True`, `BDD::Null`.
- Conversion: `to_qdd()` — convert BDD to quasi-reduced QDD. `to_zdd(n)` — convert BDD (characteristic function) to ZDD (family) over n variables.
- I/O: `Export()`, `export_binary()` / `import_binary()`, `export_sapporo()` / `import_sapporo()`, `export_knuth()` / `import_knuth()` (deprecated), `save_graphviz()`, `export_binary_multi()` / `import_binary_multi()`.

## ZDD class

- Inherits from DDBase.
- ZDD represents a family of sets. A complement edge toggles membership of the empty set (∅).
- Applies ZDD zero-suppression rule: `hi == bddempty` → return lo.
- Uses ZDD complement edge semantics (only lo flipped).
- Operators: `+` (union), `-` (subtract), `&` (intersec), `*` (join), `/` (div), `%` (remainder), `^` (symdiff), `~` (complement), `<<` / `>>` (shift), `==`, `!=`.
- Binary operations: `bddsymdiff()`, `bddremainder()`, `bdddisjoin()`, `bddjointjoin()`, `bddmeet()`, `bdddelta()`, `product()` / `bddproduct()` (cross product for disjoint variable sets, more efficient than join).
- Set operations: `OffSet(v)`, `OnSet(v)`, `OnSet0(v)`, `Change(v)`.
- Filtering: `Restrict()`, `Permit()`, `Nonsup()`, `Nonsub()`, `maximal()`, `minimal()`, `minhit()`, `closure()`, `choose(k)` (sets of exactly k elements), `size_le(k)` (sets of at most k elements), `size_ge(k)` (sets of at least k elements), `supersets_of(s)` (sets containing s), `subsets_of(s)` (subsets of s), `project(vars)` (remove variables from all sets). `coalesce(v1, v2)` / `bddcoalesce()` — merge variable v2 into v1 (v1 survives, v2 eliminated).
- Analysis: `Always()`, `SymChk()`, `ImplyChk()`, `CoImplyChk()`, `SymGrp()`, `SymGrpNaive()`, `SymSet()`, `ImplySet()`, `CoImplySet()`, `Divisor()`, `IsPoly()`, `PermitSym()`.
- Counting: `Card()` (uint64, saturating), `count()` (double), `exact_count()` (BigInt), `exact_count(ZddCountMemo&)`, `Lit()`, `Len()`, `min_size()` (minimum set size), `max_size()` (maximum set size, equivalent to `Len()`), `profile()` (set size distribution as `vector<BigInt>`), `profile_double()` (as `vector<double>`), `support_vars()` (variable numbers appearing in the ZDD), `element_frequency()` (per-variable frequency as `vector<BigInt>`, `result[v]` = number of sets containing variable v). Free function: `bddelmfreq()`. `average_size()` (average number of elements per set, double). `variance_size()` (variance of set sizes, double). `median_size()` (median set size, double). `entropy()` (Shannon entropy based on element frequencies, double).
- Weight operations: `get_sum(weights)` (BigInt), `min_weight(weights)`, `max_weight(weights)`, `min_weight_set(weights)`, `max_weight_set(weights)`. Free functions: `bddweightsum()`, `bddminweight()`, `bddmaxweight()`, `bddminweightset()`, `bddmaxweightset()`.
- Cost bound filtering: `cost_bound_le(weights, b)`, `cost_bound_ge(weights, b)`, `cost_bound_eq(weights, b)`, `cost_bound_range(weights, lo, hi)` — filter sets by total weight. Optionally accept `CostBoundMemo&` for caching. Free functions: `bddcostbound_le()`, `bddcostbound_ge()`.
- Membership: `has_empty()` / `bddhasempty()` — check if ∅ ∈ F. `contains(s)` / `bddcontains()` — check if set s ∈ F. `is_subset_family(g)` / `bddissubset(f, g)` — check if F ⊆ G (early termination). `is_disjoint(g)` / `bddisdisjoint(f, g)` — check if F ∩ G = ∅ (early termination). `count_intersec(g)` / `bddcountintersec(f, g)` — count |F ∩ G| without building the intersection ZDD (BigInt). `jaccard_index(g)` / `bddjaccardindex(f, g)` — Jaccard index |F ∩ G| / |F ∪ G| (double, 1.0 when both empty). `hamming_distance(g)` / `bddhammingdist(f, g)` — symmetric difference size |F △ G| (BigInt). `overlap_coefficient(g)` / `bddoverlapcoeff(f, g)` — overlap coefficient |F ∩ G| / min(|F|, |G|) (double, 1.0 when both empty). `flatten()` / `bddflatten()` — union of all sets as a single-set ZDD.
- Sampling: `uniform_sample(rng, ZddCountMemo&)` — uniformly sample one set from the family. `sample_k(k, rng, ZddCountMemo&)` — uniformly sample k sets and return as a ZDD (hypergeometric distribution at each node). `random_subset(p, rng)` — include each set independently with probability p, return as a ZDD. `weighted_sample(weights, mode, rng, WeightedSampleMemo&)` — sample one set proportional to aggregated weight (Sum or Product mode via `WeightMode` enum). `boltzmann_sample(weights, beta, rng, WeightedSampleMemo&)` — sample from Boltzmann distribution P(S) ∝ exp(-β·Σw[v]). Static helper: `boltzmann_weights(weights, beta)` for weight transformation.
- Enumeration: `enumerate()` — return all sets as `vector<vector<bddvar>>`.
- Constants: `ZDD::Empty`, `ZDD::Single`, `ZDD::Null`.
- Rank-order iteration: `ZddRankIterator(zdd)` — STL input iterator enumerating sets in structure order (hi-first DFS). Value type: `vector<bddvar>`. `ZddRankRange(zdd)` — range wrapper for range-based for loops. Python: `iter_rank()`.
- Random iteration: `ZddRandomIterator<RNG>(zdd, rng)` — STL input iterator enumerating sets in uniformly random order without replacement. Hybrid strategy: rejection sampling when `sampled < |F|/2`, direct sampling from `f - sampled` otherwise. Value type: `vector<bddvar>`. `ZddRandomRange<RNG>(zdd, rng)` — range wrapper. Python: `iter_random(seed=0)`.
- Ranking: `rank(s)` (int64_t), `exact_rank(s)` (BigInt), `exact_rank(s, ZddCountMemo&)` — 0-based index of set `s` in ZDD structure order. Returns -1 if not in family. Free functions: `bddrank()`, `bddexactrank()`.
- Unranking: `unrank(order)` (int64_t), `exact_unrank(order)` (BigInt), `exact_unrank(order, ZddCountMemo&)` — retrieve set at given index. Throws `std::out_of_range`. Free functions: `bddunrank()`, `bddexactunrank()`.
- First-k extraction: `get_k_sets(k)` (int64_t), `get_k_sets(k)` (BigInt), `get_k_sets(k, ZddCountMemo&)` — return a ZDD containing the first k sets in structure order. Free functions: `bddgetksets()`.
- K-lightest extraction: `get_k_lightest(k, weights, strict)` (int64_t/BigInt) — return k sets with smallest total weight. Binary search on cost bounds. `strict`: 0=exactly k, <0=fewer (lighter only), >0=more (includes full tier). Free functions: `bddgetklightest()`.
- K-heaviest extraction: `get_k_heaviest(k, weights, strict)` (int64_t/BigInt) — return k sets with largest total weight. Computed as `f - get_k_lightest(|F|-k, -strict)`. Free functions: `bddgetkheaviest()`.
- Construction: `ZDD::singleton(v)`, `ZDD::single_set(vars)`, `ZDD::from_sets(sets)`, `ZDD::power_set(n)`, `ZDD::power_set(vars)`, `ZDD::combination(n, k)`, `ZDD::random_family(n, rng)`.
- Display: `print_sets(os)`, `print_sets(os, delim1, delim2)`, `print_sets(os, delim1, delim2, var_name_map)`, `to_str()`. `to_cnf()` / `to_cnf(var_name_map)` — CNF string (each set as OR-clause, joined by AND). `to_dnf()` / `to_dnf(var_name_map)` — DNF string (each set as AND-term, joined by OR).
- Conversion: `to_qdd()` — convert ZDD to quasi-reduced QDD. `to_bdd(n)` — convert ZDD (family) to BDD (characteristic function) over n variables.
- I/O: `Export()`, `export_binary()` / `import_binary()`, `export_sapporo()` / `import_sapporo()`, `export_knuth()` / `import_knuth()` (deprecated), `save_graphviz()`, `export_graphillion()` / `import_graphillion()`, `export_binary_multi()` / `import_binary_multi()`.

### Memo classes

- `ZddCountMemo(f)`: Memo for ZDD exact counting, associated with a specific ZDD root. Stores the memo table for `bddexactcount` results.
- `BddCountMemo(f, n)`: Memo for BDD exact counting, associated with a specific BDD root and variable count n.
- `CostBoundMemo`: Interval-memoization for cost-bound queries (`cost_bound_le`, `cost_bound_ge`).
- `WeightedSampleMemo(ZDD, weights, mode)`: Memo for weighted sampling. Stores precomputed weight aggregation values per node. `WeightMode::Sum` or `WeightMode::Product`.

## QDD class

- Inherits from DDBase.
- `QDD` (Quasi-reduced Decision Diagram).
- QDD does not apply the jump rule (retains nodes even when lo == hi). Visits every level from root to terminal.
- Complement edge semantics are the same as BDD (both lo and hi are flipped).
- `QDD::getnode(var, lo, hi)` validates that children's levels equal var's level - 1.
- Conversion to/from BDD/ZDD: `BDD::to_qdd()`, `ZDD::to_qdd()`, `QDD::to_bdd()`, `QDD::to_zdd()`.
- I/O: `export_binary()` / `import_binary()`, `export_binary_multi()` / `import_binary_multi()`.

## UnreducedDD class

- Inherits from DDBase. Defined in `include/unreduced_dd.h`.
- Type-agnostic unreduced decision diagram. Does NOT apply any reduction rules at node creation.
- Complement edges are NOT interpreted in UnreducedDD; they are stored raw and only gain meaning at reduce time.
- Nodes are NOT inserted in the unique table (not canonical).
- `UnreducedDD::getnode(var, lo, hi)`: Always allocates an unreduced node. No complement normalization, no reduction rules, no unique table insertion.
- Supports top-down construction with `set_child0()` / `set_child1()` to mutate children after creation.
- `is_reduced()`: Checks the reduced flag on the node.
- `reduce_as_bdd()`: Reduces using BDD complement semantics and BDD jump rule (`lo == hi` → `lo`). Returns `BDD`.
- `reduce_as_zdd()`: Reduces using ZDD complement semantics and ZDD zero-suppression rule (`hi == bddempty` → `lo`). Returns `ZDD`.
- `reduce_as_qdd()`: Equivalent to `reduce_as_bdd().to_qdd()`. Returns `QDD`.
- Complement expansion constructors: `UnreducedDD(const BDD&)`, `UnreducedDD(const ZDD&)`, `UnreducedDD(const QDD&)` expand complement edges using the source type's semantics, producing a complement-free unreduced DD.
- `wrap_raw(const BDD&)` / `wrap_raw(const ZDD&)` / `wrap_raw(const QDD&)`: Wraps bddp directly without complement expansion. Cross-type reduce on wrap_raw results is undefined behavior.

## SeqBDD class

- Defined in `include/seqbdd.h`. Does NOT inherit from DDBase.
- Uses composition: wraps a `ZDD` internally (`zdd_` member).
- Provides string/sequence-specific operations over ZDD.
- Key operations: `off_set()`, `on_set()`, `on_set0()`, `push()`, `top()`, `size()`, `card()`, `lit()`, `len()`.
- Operators: `&` (intersection), `+` (union), `-` (difference), `*` (concatenation), `/` (left quotient), `%` (left remainder).
- Display: `print()`, `print_seq()`, `seq_str()`, `export_to()`.
- I/O: `save_svg()` — SVG visualization of the internal ZDD.

## PiDD class

- Defined in `include/pidd.h`. Does NOT inherit from DDBase.
- Uses composition: wraps a `ZDD` internally (`zdd_` member).
- Represents permutations using transposition algebra.
- Key operations: `Swap(u, v)`, `Cofact(u, v)`, `Odd()`, `Even()`, `SwapBound(n)`.
- Operators: `&` (intersection), `+` (union), `-` (difference), `*` (composition), `/` (division), `%` (remainder).
- Display: `Print()`, `Enum()`, `Enum2()`.
- I/O: `save_svg()` — SVG visualization of the internal ZDD.

## RotPiDD class

- Defined in `include/rotpidd.h`. Does NOT inherit from DDBase.
- Uses composition: wraps a `ZDD` internally (`zdd_` member).
- Extends PiDD to handle rotations in addition to transpositions.
- Key operations: `LeftRot(u, v)`, `Swap(a, b)`, `Reverse(l, r)`, `Cofact(u, v)`, `Odd()`, `Even()`, `Order(a, b)`, `Inverse()`, `Insert(p, v)`, `RotBound(n)`, `RemoveMax(k)`, `normalizeRotPiDD(k)`.
- Operators: `&` (intersection), `+` (union), `-` (difference), `*` (composition).
- Conversion: `VECtoRotPiDD(v)`, `RotPiDDToVectorOfPerms()`, `normalizePerm(v)`.
- Advanced: `contradictionMaximization()` — weighted contradiction maximization.
- Display: `Print()`, `Enum()`, `Enum2()`.
- I/O: `save_svg()` — SVG visualization of the internal ZDD.

## MTBDD class (template)

- Defined in `include/mtbdd.h`. Template class `MTBDD<T>` parameterized by terminal value type `T`.
- Multi-Terminal BDD. Maps Boolean variable assignments to values of type T (not just 0/1).
- No complement edges — nodes are stored without complement normalization.
- Terminal table: singleton `MTBDDTerminalTable<T>` maps values to indices. Index 0 is always `T{}` (zero value).
- Node creation: `mtbdd_getnode(var, lo, hi)` / `mtbdd_getnode_raw(var, lo, hi)` — BDD-style reduction (jump rule: lo == hi → lo).
- Apply: `mtbdd_apply_rec<T, BinOp>(f, g, op, cache_op)` — BDD cofactoring (missing var → pass through).
- ITE: `mtbdd_ite_rec<T>(f, g, h, cache_op)` — if-then-else on MTBDD.
- Pre-instantiated types: `MTBDDFloat` (`MTBDD<double>`), `MTBDDInt` (`MTBDD<int64_t>`).
- Operations: `operator+`, `operator-`, `operator*` (apply with add/sub/mul), `ite(f, g, h)`.
- Query: `terminal_value()`, `is_terminal()`, `top()`, `size()`, `evaluate(assignment)`.
- I/O: `save_svg()` — SVG visualization with terminal labels via `terminal_name_map`. `export_binary(strm)` / `import_binary(strm)` — binary format serialization (dd_type=4). `export_binary(filename)` / `import_binary(filename)` — file-based overloads.

## MTZDD class (template)

- Defined in `include/mtbdd.h`. Template class `MTZDD<T>` parameterized by terminal value type `T`.
- Multi-Terminal ZDD. Like MTBDD but uses ZDD cofactoring (missing var → hi is zero terminal).
- Node creation: `mtzdd_getnode(var, lo, hi)` / `mtzdd_getnode_raw(var, lo, hi)` — ZDD-style reduction (hi == zero_terminal → lo).
- Apply: `mtzdd_apply_rec<T, BinOp>(f, g, op, cache_op)` — ZDD cofactoring.
- Pre-instantiated types: `MTZDDFloat` (`MTZDD<double>`), `MTZDDInt` (`MTZDD<int64_t>`).
- Operations: `operator+`, `operator-`, `operator*`, `ite(f, g, h)`.
- Cofactor: `cofactor0(v)` (fix v=0), `cofactor1(v)` (fix v=1). ZDD semantics: zero-suppressed var=0 returns self, var=1 returns zero terminal. Free functions: `mtzdd_cofactor0(f, v)`, `mtzdd_cofactor1(f, v)`.
- Query: `terminal_value()`, `is_terminal()`, `top()`, `size()`, `evaluate(assignment)`.
- I/O: `save_svg()` — SVG visualization with terminal labels. `export_binary(strm)` / `import_binary(strm)` — binary format serialization (dd_type=5). `export_binary(filename)` / `import_binary(filename)` — file-based overloads.

## MVBDD class

- Defined in `include/mvdd.h`. Inherits from DDBase.
- Multi-Valued BDD. Boolean function over multi-valued variables (each taking values 0..k-1).
- Internally emulated by a standard BDD using one-hot encoding: each MVDD variable with domain size k uses k-1 internal DD variables.
- Variable table: `MVDDVarTable` manages bidirectional mapping between MVDD variables and internal DD variables. `MVDDVarInfo` holds per-variable metadata.
- Constructors: `MVBDD()`, `MVBDD(k, value)`, `MVBDD(table, bdd)`.
- Factory: `zero(table)`, `one(table)`, `singleton(base, mv, value)`.
- Variable management: `new_var()`, `k()`, `var_table()`.
- ITE construction: `MVBDD::ite(base, mv, children)` — build by specifying child for each value.
- Child access: `child(value)` — cofactor when top MVDD variable takes given value. `top_var()` — top MVDD variable number.
- Boolean operations: `&`, `|`, `^`, `~`.
- Evaluation: `evaluate(assignment)`.
- Conversion: `to_bdd()`, `from_bdd(base, bdd)`.
- Node count: `mvbdd_node_count()` (MVDD-level), `size()` (internal BDD-level).
- I/O: `save_svg()` — k-way branching with colored edges in Expanded mode.

## MVZDD class

- Defined in `include/mvdd.h`. Inherits from DDBase.
- Multi-Valued ZDD. Family of multi-valued assignments (each variable takes values 0..k-1).
- Internally emulated by a standard ZDD using one-hot encoding.
- Constructors: `MVZDD()`, `MVZDD(k, value)`, `MVZDD(table, zdd)`.
- Factory: `zero(table)`, `one(table)`, `singleton(base, mv, value)`.
- Variable management: `new_var()`, `k()`, `var_table()`.
- ITE construction: `MVZDD::ite(base, mv, children)`.
- Child access: `child(value)`, `top_var()`.
- Set operations: `+` (union), `-` (difference), `&` (intersection).
- Counting: `count()` (double), `exact_count()` (BigInt).
- Evaluation: `evaluate(assignment)`.
- Enumeration: `enumerate()`, `print_sets()`, `to_str()`.
- Conversion: `to_zdd()`, `from_zdd(base, zdd)`.
- Node count: `mvzdd_node_count()` (MVDD-level), `size()` (internal ZDD-level).
- I/O: `save_svg()` — k-way branching with colored edges in Expanded mode.

## Size functions

- `bddsize(f)` — DAG node count with complement edge sharing.
- `bddplainsize(f, is_zdd)` — DAG node count without complement edge sharing. Expands complement edges using BDD or ZDD semantics.
- `bddrawsize(vector<bddp>)` — Shared node count across multiple roots (with complement edge sharing).
- `bddplainsize(vector<bddp>, is_zdd)` — Shared node count across multiple roots (without complement edge sharing).
- BDD/ZDD member: `raw_size()` (shared), `plain_size()` (expanded), `raw_size(vector)`, `plain_size(vector)`.

## I/O formats

The library supports multiple import/export formats:

- **Sapporo format**: Text format. `bdd_export_sapporo` / `bdd_import_sapporo`, `zdd_export_sapporo` / `zdd_import_sapporo`.
- **Binary format**: Compact binary format. Single-root: `bdd_export_binary` / `bdd_import_binary`, `zdd_export_binary` / `zdd_import_binary`, `qdd_export_binary` / `qdd_import_binary`, `unreduced_export_binary` / `unreduced_import_binary`. Multi-root: `bdd_export_binary_multi` / `bdd_import_binary_multi`, etc.
- **Graphillion format**: ZDD text format for Graphillion interop. `zdd_export_graphillion` / `zdd_import_graphillion`.
- **Graphviz DOT**: Visualization. `bdd_save_graphviz` / `zdd_save_graphviz`. `DrawMode::Expanded` (complement expanded) or `DrawMode::Raw` (complement edge markers).
- **SVG**: Self-contained SVG visualization. `bdd_save_svg` / `zdd_save_svg` / `qdd_save_svg` / `unreduced_save_svg` / `mtbdd_save_svg` / `mtzdd_save_svg` / `mvbdd_save_svg` / `mvzdd_save_svg`. Supports `DrawMode::Expanded` and `DrawMode::Raw` (Knuth-style dot/odot markers). Configurable via `SvgParams` struct. Output: file, ostream, or string. MTBDD/MTZDD: multi-terminal labels via `terminal_name_map`. MVBDD/MVZDD Expanded: k-way branching with colored edges; Raw: internal BDD/ZDD binary tree.
- **Knuth format** (deprecated): `bdd_export_knuth` / `bdd_import_knuth`, `zdd_export_knuth` / `zdd_import_knuth`.
- **Legacy Sapporo format**: `bddexport` / `bddimport` / `bddimportz`.

## Node reduced flag

The reduced flag is a 1-bit flag stored in `data[0]` bit 32 of a `BddNode`.

```
data[0] bits [63:33] : var     (31 bits)
data[0] bit  [32]    : reduced (1 bit)
data[0] bits [31:0]  : lo_hi   (upper 32 bits of 0-arc)
```

- Constant: `BDD_NODE_REDUCED_FLAG = 1ULL << 32` (defined in `include/bdd_node.h`).
- `node_set_reduced(node_id)`: Sets bit 32 of data[0].
- `node_is_reduced(node_id)`: Checks bit 32 of data[0].
- `bddp_is_reduced(p)`: Returns true for terminal nodes (always reduced), false for null, and checks the flag for regular nodes.
- In `BDD::getnode_raw` / `ZDD::getnode_raw` / `QDD::getnode_raw`: the reduced flag is set when both children are reduced.
- In `UnreducedDD::getnode()`: the reduced flag is NOT set. All nodes created by `UnreducedDD::getnode()` are unreduced.

## Repository

- `conversation/` directory stores conversation logs and is gitignored.
- `build/` directory is gitignored.

### File structure

- `CMakeLists.txt` — Build configuration
- `include/bdd.h` — Umbrella header (includes all headers)
- `include/dd_base.h` — DDBase base class definition
- `include/bdd_types.h` — Type definitions, constants, structs, BDD/ZDD class declarations
- `include/bdd_base.h` — Infrastructure function declarations (initialization, variable management, node creation, etc.)
- `include/bdd_ops.h` — BDD operation function declarations
- `include/bdd_io.h` — I/O function declarations (export/import)
- `include/svg_export.h` — SVG export API (SvgParams struct, bdd_save_svg/zdd_save_svg/qdd_save_svg/unreduced_save_svg/mtbdd_save_svg/mtzdd_save_svg/mvbdd_save_svg/mvzdd_save_svg)
- `include/bdd_internal.h` — Internal header (node accessor inline functions, GC guard, shift templates)
- `include/bdd_node.h` — BddNode struct definition
- `include/bigint.hpp` — Arbitrary-precision integer (BigInt) header-only library
- `include/mtbdd.h` — MTBDD/MTZDD template classes and terminal table
- `include/mvdd.h` — MVDDVarInfo, MVDDVarTable, MVBDD, MVZDD class declarations
- `include/bddv.h` — BDDV (vector BDD) class declaration
- `include/zbddv.h` — ZBDDV (vector ZDD) class declaration
- `include/mlzbddv.h` — MLZBDDV (multi-level vector ZDD) class declaration
- `include/btoi.h` — BtoI (BDD to integer mapping) class declaration
- `include/ctoi.h` — CtoI (conversion to integer) class declaration
- `include/sop.h` — SOP/SOPV (Sum of Products) class declarations
- `include/zdd_rank_iter.h` — ZddRankIterator/ZddRankRange declarations
- `include/zdd_random_iter.h` — ZddRandomIterator/ZddRandomRange template declarations
- `include/zdd_weight_iter.h` — ZddMinWeightIterator/ZddMaxWeightIterator declarations
- `src/bdd_base.cpp` — Global variables, initialization, variable management, unique table, cache, node creation
- `src/bdd_ops.cpp` — BDD operations (and, or, xor, ite, cofactor, quantification, etc.)
- `src/zdd_ops.cpp` — ZDD basic operations (offset, onset, change, union, intersec, subtract, div, join, meet, delta, etc.)
- `src/zdd_adv.cpp` — ZDD advanced operations (disjoin, restrict, permit, nonsup, nonsub, maximal, minimal, minhit, closure, card, etc.)
- `src/zdd_adv2.cpp` — ZDD additional operations (permitsym, always, symchk, implychk, coimplychk, implyset, coimplyset, symset, etc.)
- `src/bdd_io.cpp` — export/import implementation
- `src/svg_export.cpp` — SVG export implementation (SvgWriter, SvgPositionManager, SvgLaneManager)
- `src/bdd_class.cpp` — BDD/ZDD/QDD static const definitions, bddhasempty, ZDD::Print
- `src/qdd.cpp` — QDD implementation (getnode, BDD/ZDD conversion, etc.)
- `src/unreduced_dd.cpp` — UnreducedDD implementation
- `src/pidd.cpp` — PiDD implementation (Swap, Cofact, Odd, multiplication, division, etc.)
- `src/rotpidd.cpp` — RotPiDD implementation (LeftRot, Swap, Reverse, Order, Inverse, Insert, etc.)
- `src/seqbdd.cpp` — SeqBDD implementation (concatenation, left quotient, print, etc.)
- `src/mtbdd.cpp` — MTBDD/MTZDD implementation (terminal table registry, etc.)
- `src/mvdd.cpp` — MVBDD/MVZDD implementation (variable table, ITE, operations, etc.)
- `src/bddv.cpp` — BDDV implementation
- `src/zbddv.cpp` — ZBDDV implementation
- `src/mlzbddv.cpp` — MLZBDDV implementation
- `src/btoi.cpp` — BtoI implementation
- `src/ctoi.cpp` — CtoI implementation
- `src/sop.cpp` — SOP implementation
- `src/zdd_rank_iter.cpp` — ZddRankIterator implementation
- `src/zdd_random_iter.cpp` — ZddRandomIterator implementation
- `src/zdd_weight_iter.cpp` — ZddWeightIterator implementation
- `include/qdd.h` — QDD class declaration
- `include/unreduced_dd.h` — UnreducedDD class declaration
- `include/seqbdd.h` — SeqBDD class declaration
- `include/pidd.h` — PiDD class declaration
- `include/rotpidd.h` — RotPiDD class declaration
- `src/main.cpp` — Main entry point
- `test/test_bdd.cpp` — BDD/ZDD tests using Google Test
- `test/test_qdd.cpp` — QDD tests
- `test/test_unreduced_dd.cpp` — UnreducedDD tests
- `test/test_pidd.cpp` — PiDD tests
- `test/test_rotpidd.cpp` — RotPiDD tests
- `test/test_seqbdd.cpp` — SeqBDD tests
- `test/test_ddbase.cpp` — DDBase tests
- `test/test_svg_export.cpp` — SVG export tests
- `test/test_mtbdd.cpp` — MTBDD/MTZDD tests
- `test/test_mvdd.cpp` — MVBDD/MVZDD tests
- `test/test_bddv.cpp` — BDDV tests
- `test/test_zbddv.cpp` — ZBDDV tests
- `test/test_mlzbddv.cpp` — MLZBDDV tests
- `test/test_btoi.cpp` — BtoI tests
- `test/test_ctoi.cpp` — CtoI tests
- `test/test_sop.cpp` — SOP tests
- `test/test_zdd_rank_iter.cpp` — ZddRankIterator tests
- `test/test_zdd_random_iter.cpp` — ZddRandomIterator tests
- `test/test_zdd_weight_iter.cpp` — ZddWeightIterator tests
- `python/src/kyotodd/_binding.cpp` — Python bindings (pybind11)
  - Bound classes: BDD, ZDD, QDD, UnreducedDD, PiDD, RotPiDD, SeqBDD, MVBDD, MVZDD, MTBDDFloat, MTBDDInt, MTZDDFloat, MTZDDInt, MVDDVarInfo, MVDDVarTable, BddCountMemo, ZddCountMemo, CostBoundMemo, ZddRankRange/Iterator, ZddRandomRange/Iterator, ZddMinWeightRange/Iterator, ZddMaxWeightRange/Iterator
  - **Not bound** (C++ only): BDDV, ZBDDV, MLZBDDV, BtoI, CtoI, SOP, SOPV
- `docs/` — Sphinx documentation (API reference, tutorials, concepts)
- `app/BDDQueen/` — N-Queens sample applications
- `app/SvgExamples/` — SVG export sample application
