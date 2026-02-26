# KyotoDD - BDD Library in C++

## Rules

- Implement incrementally, step by step, so the user can follow and understand every change.
- Never decide unclear specifications on your own — always ask the user first.
- Commit to git after every change.

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

- BDD: `getnode(var, lo, hi)` — BDD の削減規則を適用して新しいノードを作成する。
- ZDD: `getznode(var, lo, hi)` — ZDD の削減規則を適用して新しいノードを作成する。

## BDD class

- `BDD` class has a single member `root` (uint64_t): the root node ID.

## Repository

- `conversation/` directory stores conversation logs and is gitignored.
- `build/` directory is gitignored.

### File structure

- `CMakeLists.txt` — ビルド設定
- `include/bdd.h` — アンブレラヘッダ（全ヘッダを include）
- `include/bdd_types.h` — 型定義、定数、構造体、BDD/ZDD クラス宣言
- `include/bdd_base.h` — インフラ関数宣言（初期化、変数管理、ノード作成等）
- `include/bdd_ops.h` — BDD 演算関数宣言
- `include/bdd_io.h` — I/O 関数宣言（export/import）
- `include/bdd_internal.h` — 内部ヘッダ（ノードアクセサ inline 関数）
- `include/bdd_node.h` — BddNode 構造体の定義
- `src/bdd_base.cpp` — グローバル変数、初期化、変数管理、ユニークテーブル、キャッシュ、ノード作成
- `src/bdd_ops.cpp` — BDD 演算（and, or, xor, ite, cofactor, 量化等）
- `src/bdd_io.cpp` — export/import の実装
- `src/bdd_class.cpp` — BDD/ZDD static const 定義
- `src/main.cpp` — メインエントリポイント
- `test/test_bdd.cpp` — Google Test によるテスト
