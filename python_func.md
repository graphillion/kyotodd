# Python未実装関数一覧

C++に実装されていてPythonバインディングに未実装の関数。

## BDD クラス

| C++ メソッド | 説明 |
|---|---|
| `BDD::to_qdd()` | QDD への変換 |
| `BDD::count(bddvar)` | SAT count (浮動小数点) |
| `BDD::exact_count(bddvar)` | SAT count (任意精度整数) |
| `BDD::uniform_sample(RNG&, bddvar, BddCountMemo&)` | 一様ランダムサンプリング |
| `BDD::prime(bddvar)` | 正リテラル BDD を生成 (static) |
| `BDD::prime_not(bddvar)` | 負リテラル BDD を生成 (static) |
| `BDD::cube(const vector<int>&)` | リテラル列から積項 (cube) を生成 (static) |
| `BDD::clause(const vector<int>&)` | リテラル列から節 (clause) を生成 (static) |
| `BDD::getnode(bddvar, const BDD&, const BDD&)` | ノード作成 (static) |
| `BDD::child0()` / `BDD::child1()` / `BDD::child(int)` | complement 解決付き子アクセサ |
| `BDD::raw_child0()` / `BDD::raw_child1()` / `BDD::raw_child(int)` | raw 子アクセサ |
| `BDD::raw_size(const vector<BDD>&)` | 複数 BDD の共有ノード数 (static) |
| `BDD::plain_size(const vector<BDD>&)` | 複数 BDD の非共有ノード数 (static) |
| `BDD::export_binary()` | バイナリ形式エクスポート |
| `BDD::import_binary()` | バイナリ形式インポート (static) |
| `BDD::export_binary_multi()` | 複数 BDD バイナリエクスポート (static) |
| `BDD::import_binary_multi()` | 複数 BDD バイナリインポート (static) |
| `BDD::export_sapporo()` | Sapporo 形式エクスポート |
| `BDD::import_sapporo()` | Sapporo 形式インポート (static) |
| `BDD::save_graphviz()` | Graphviz DOT 出力 |

## ZDD クラス

| C++ メソッド | 説明 |
|---|---|
| `ZDD::to_qdd()` | QDD への変換 |
| `ZDD::count()` | 集合族のカウント (浮動小数点) |
| `ZDD::uniform_sample(RNG&, ZddCountMemo&)` | 一様ランダムサンプリング |
| `ZDD::enumerate()` | 全集合を列挙 |
| `ZDD::has_empty()` | 空集合を含むか |
| `ZDD::singleton(bddvar)` | 単一変数の集合 {{v}} を生成 (static) |
| `ZDD::single_set(const vector<bddvar>&)` | 単一集合 {v1,v2,...} を生成 (static) |
| `ZDD::power_set(bddvar)` | べき集合を生成 (変数数指定) (static) |
| `ZDD::power_set(const vector<bddvar>&)` | べき集合を生成 (変数リスト指定) (static) |
| `ZDD::from_sets(const vector<vector<bddvar>>&)` | 集合のリストから ZDD を構築 (static) |
| `ZDD::combination(bddvar, bddvar)` | 組み合わせ ZDD を生成 (static) |
| `ZDD::getnode(bddvar, const ZDD&, const ZDD&)` | ノード作成 (static) |
| `ZDD::child0()` / `ZDD::child1()` / `ZDD::child(int)` | complement 解決付き子アクセサ |
| `ZDD::raw_child0()` / `ZDD::raw_child1()` / `ZDD::raw_child(int)` | raw 子アクセサ |
| `ZDD::raw_size(const vector<ZDD>&)` | 複数 ZDD の共有ノード数 (static) |
| `ZDD::plain_size(const vector<ZDD>&)` | 複数 ZDD の非共有ノード数 (static) |
| `ZDD::export_binary()` | バイナリ形式エクスポート |
| `ZDD::import_binary()` | バイナリ形式インポート (static) |
| `ZDD::export_binary_multi()` | 複数 ZDD バイナリエクスポート (static) |
| `ZDD::import_binary_multi()` | 複数 ZDD バイナリインポート (static) |
| `ZDD::export_sapporo()` | Sapporo 形式エクスポート |
| `ZDD::import_sapporo()` | Sapporo 形式インポート (static) |
| `ZDD::export_graphillion()` | Graphillion 形式エクスポート |
| `ZDD::import_graphillion()` | Graphillion 形式インポート (static) |
| `ZDD::save_graphviz()` | Graphviz DOT 出力 |
| `ZDD::ZLev(int, int)` | ZLev 操作 |
| `ZDD_LCM_A(char*, int)` | LCM アルゴリズム (A) |
| `ZDD_LCM_C(char*, int)` | LCM アルゴリズム (C) |
| `ZDD_LCM_M(char*, int)` | LCM アルゴリズム (M) |

## QDD クラス

| C++ メソッド | 説明 |
|---|---|
| `QDD::export_binary()` | バイナリ形式エクスポート |
| `QDD::import_binary()` | バイナリ形式インポート (static) |
| `QDD::export_binary_multi()` | 複数 QDD バイナリエクスポート (static) |
| `QDD::import_binary_multi()` | 複数 QDD バイナリインポート (static) |

## UnreducedDD クラス

| C++ メソッド | 説明 |
|---|---|
| `UnreducedDD::export_binary()` | バイナリ形式エクスポート |
| `UnreducedDD::import_binary()` | バイナリ形式インポート (static) |

## PiDD クラス

| C++ メソッド | 説明 |
|---|---|
| `PiDD::operator/()` | 除算 |
| `PiDD::operator%()` | 剰余 |
| `PiDD::operator/=()` | in-place 除算 |
| `PiDD::operator%=()` | in-place 剰余 |
| `PiDD::Print()` | コンソール出力 |
| `PiDD::Enum()` | 全順列列挙 |
| `PiDD::Enum2()` | 全順列列挙 (形式2) |

## RotPiDD クラス

| C++ メソッド | 説明 |
|---|---|
| `RotPiDD::contradictionMaximization(...)` | 矛盾最大化アルゴリズム |
| `RotPiDD::Print()` | コンソール出力 |
| `RotPiDD::Enum()` | 全順列列挙 |
| `RotPiDD::Enum2()` | 全順列列挙 (形式2) |

## SeqBDD クラス

| C++ メソッド | 説明 |
|---|---|
| `SeqBDD::export_to(ostream&)` | テキストエクスポート |
| `SeqBDD::print_seq()` | シーケンス出力 |

## モジュールレベル関数

| C++ 関数 | 説明 |
|---|---|
| `DDBase::new_var(int n, bool reverse)` | n 個の変数を一括作成 |
| `bddgc_rootcount()` | GC ルート数 |
| `bddtoplev()` | 最上位レベル |
