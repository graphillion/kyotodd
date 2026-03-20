# Undocumented Classes and Functions

## C++ Headers — Doc comments (/** */ or ///) が存在しないファイル

### `include/seqbdd.h` — SeqBDD class (全体)

| 種別 | 名前 | 説明 |
|------|------|------|
| 定数 | `BDD_OP_SEQBDD_CONCAT` | SeqBDD cache 用オペレーションコード |
| 定数 | `BDD_OP_SEQBDD_LQUOT` | SeqBDD cache 用オペレーションコード |
| コンストラクタ | `SeqBDD()` | デフォルトコンストラクタ |
| コンストラクタ | `SeqBDD(int)` | 値からの構築 |
| コンストラクタ | `SeqBDD(const SeqBDD&)` | コピーコンストラクタ |
| コンストラクタ | `SeqBDD(const ZDD&)` | ZDD からの構築 |
| 演算子 | `&, +, -, *, /, %, ==, !=` | 集合演算・比較 |
| 演算子 | `&=, +=, -=, *=, /=, %=` | 複合代入 |
| メソッド | `top()` | トップ変数 |
| メソッド | `get_zdd()` | 内部 ZDD の取得 |
| メソッド | `size()` | ノード数 |
| メソッド | `card()` | カーディナリティ |
| メソッド | `lit()` | リテラル数 |
| メソッド | `len()` | 最大集合サイズ |
| メソッド | `off_set(int)` | 変数 v を含まない集合の選択 |
| メソッド | `on_set0(int)` | 変数 v を含まない集合の選択 |
| メソッド | `on_set(int)` | 変数 v を含む集合の選択（v を除去） |
| メソッド | `push(int)` | 変数のプッシュ |
| メソッド | `print()` | 出力 |
| メソッド | `export_to(FILE*)` | FILE への出力 |
| メソッド | `export_to(std::ostream&)` | ストリームへの出力 |
| メソッド | `print_seq()` | シーケンスとして出力 |
| メソッド | `seq_str()` | シーケンス文字列の取得 |
| 関数 | `SeqBDD_ID(bddp)` | 生ノード ID から SeqBDD を構築 |

### `include/pidd.h` — PiDD class (全体)

| 種別 | 名前 | 説明 |
|------|------|------|
| 定数 | `PiDD_MaxVar` | 最大変数数 |
| 定数 | `BDD_OP_PIDD_SWAP` 他 5 個 | PiDD cache 用オペレーションコード |
| グローバル変数 | `PiDD_TopVar`, `PiDD_VarTableSize`, `PiDD_LevOfX[]`, `PiDD_XOfLev` | PiDD 変数テーブル |
| 関数 | `PiDD_NewVar()` | 新規 PiDD 変数の作成 |
| 関数 | `PiDD_VarUsed()` | 使用中の変数数 |
| マクロ | `PiDD_X_Lev`, `PiDD_Y_Lev`, `PiDD_Lev_XY` | レベル変換 |
| マクロ | `PiDD_Y_YUV`, `PiDD_U_XYU` | 転置代数マクロ |
| コンストラクタ | `PiDD()`, `PiDD(int)`, `PiDD(const PiDD&)`, `PiDD(const ZDD&)` | 各種コンストラクタ |
| 演算子 | `&, +, -, *, /, %, ==, !=` | 集合演算・比較 |
| 演算子 | `&=, +=, -=, *=, /=, %=` | 複合代入 |
| メソッド | `TopX()`, `TopY()`, `TopLev()` | トップ変数情報 |
| メソッド | `Size()`, `Card()` | サイズ・カーディナリティ |
| メソッド | `GetZDD()` | 内部 ZDD の取得 |
| メソッド | `Swap(int, int)` | 転置 Swap(u, v) の適用 |
| メソッド | `Cofact(int, int)` | 位置 u の値が v である順列の抽出 |
| メソッド | `Odd()` | 奇順列の抽出 |
| メソッド | `Even()` | 偶順列の抽出 |
| メソッド | `SwapBound(int)` | 対称制約の適用 |
| メソッド | `Print()`, `Enum()`, `Enum2()` | 出力・列挙 |

### `include/rotpidd.h` — RotPiDD class (全体)

| 種別 | 名前 | 説明 |
|------|------|------|
| 定数 | `RotPiDD_MaxVar` | 最大変数数 |
| 定数 | `BDD_OP_ROTPIDD_LEFTROT` 他 12 個 | RotPiDD cache 用オペレーションコード |
| グローバル変数 | `RotPiDD_TopVar`, `RotPiDD_VarTableSize`, `RotPiDD_LevOfX[]`, `RotPiDD_XOfLev` | RotPiDD 変数テーブル |
| 関数 | `RotPiDD_NewVar()` | 新規 RotPiDD 変数の作成 |
| 関数 | `RotPiDD_VarUsed()` | 使用中の変数数 |
| マクロ | `RotPiDD_X_Lev`, `RotPiDD_Y_Lev`, `RotPiDD_Lev_XY` | レベル変換 |
| マクロ | `RotPiDD_Y_YUV`, `RotPiDD_U_XYU` | 回転代数マクロ |
| コンストラクタ | `RotPiDD()`, `RotPiDD(int)`, `RotPiDD(const RotPiDD&)`, `RotPiDD(const ZDD&)` | 各種コンストラクタ |
| 演算子 | `&, +, -, *, ==, !=` | 集合演算・比較 |
| 演算子 | `&=, +=, -=, *=` | 複合代入 |
| メソッド | `TopX()`, `TopY()`, `TopLev()` | トップ変数情報 |
| メソッド | `Size()`, `Card()` | サイズ・カーディナリティ |
| メソッド | `GetZDD()` | 内部 ZDD の取得 |
| メソッド | `LeftRot(int, int)` | 左回転 LeftRot(u, v) の適用 |
| メソッド | `Swap(int, int)` | 位置の交換 |
| メソッド | `Reverse(int, int)` | 位置の反転 |
| メソッド | `Cofact(int, int)` | 位置 u の値が v である順列の抽出 |
| メソッド | `Odd()`, `Even()` | 奇/偶順列の抽出 |
| メソッド | `RotBound(int)` | 対称制約の適用 |
| メソッド | `Order(int, int)` | pi(a) < pi(b) を満たす順列の抽出 |
| メソッド | `Inverse()` | 各順列の逆順列の計算 |
| メソッド | `Insert(int, int)` | 値の挿入 |
| メソッド | `RemoveMax(int)` | 大きい変数の除去 |
| メソッド | `normalizeRotPiDD(int)` | 正規化 |
| 静的メソッド | `normalizePerm(std::vector<int>&)` | 順列の正規化 |
| 静的メソッド | `VECtoRotPiDD(std::vector<int>)` | ベクトルから RotPiDD への変換 |
| メソッド | `RotPiDDToVectorOfPerms()` | 順列ベクトルのリストへの変換 |
| メソッド | `Extract_One()` | 1 つの順列の抽出 |
| メソッド | `Print()`, `Enum()`, `Enum2()` | 出力・列挙 |
| 構造体 | `hash_func` | カスタムハッシュ関数 |
| メソッド | `contradictionMaximization(...)` | 矛盾最大化の最適化 |

### `include/bdd_internal.h` — Internal utilities (全体)

| 種別 | 名前 | 説明 |
|------|------|------|
| マクロ | `BDD_DEBUG_ASSERT(cond)` | デバッグアサーション |
| 構造体 | `BDD_RecurGuard` | 再帰深度ガード (RAII) |
| 構造体 | `BDD_GCDepthGuard` | GC 深度ガード (RAII) |
| 関数 | `bdd_should_gc()` | GC 実行判定 |
| テンプレート | `bdd_gc_guard(F func)` | GC ガード付き実行 |
| inline 関数 | `node_index(bddp)` | ノード ID → 配列インデックス変換 |
| inline 関数 | `node_write(bddp, bddvar, bddp, bddp)` | ノードデータの書き込み |
| inline 関数 | `node_set_reduced(bddp)` | reduced フラグの設定 |
| inline 関数 | `node_is_reduced(bddp)` | reduced フラグの確認 |
| inline 関数 | `bddp_is_reduced(bddp)` | bddp の reduced 判定 |
| inline 関数 | `node_var(bddp)` | ノードの変数番号の取得 |
| inline 関数 | `node_lo(bddp)` | ノードの lo 子の取得 |
| inline 関数 | `node_hi(bddp)` | ノードの hi 子の取得 |
| テンプレート | `bdd_lshift_core(bddp, bddvar, uint8_t, MakeNode)` | 左シフト再帰コア |
| テンプレート | `bdd_rshift_core(bddp, bddvar, uint8_t, MakeNode)` | 右シフト再帰コア |

---

## Python `.pyi` — クラス定義がまるごと欠けている

以下の 3 クラスは `_binding.cpp` でバインドされ、`__init__.py` で export されているが、`_core.pyi` にスタブ定義がない。

### `QDD` class

| 種別 | 名前 |
|------|------|
| コンストラクタ | `__init__(val: int = 0)` |
| 演算子 | `__eq__`, `__ne__`, `__hash__`, `__repr__`, `__bool__`, `__invert__` |
| メソッド | `child0()`, `child1()`, `child(c: int)` |
| メソッド | `raw_child0()`, `raw_child1()`, `raw_child(c: int)` |
| メソッド | `to_bdd()`, `to_zdd()` |
| プロパティ | `top_var`, `node_id`, `raw_size`, `is_terminal`, `is_one`, `is_zero` |
| I/O | `export_binary_str()`, `export_binary_file(path)` |
| 静的メソッド | `zero()`, `one()`, `import_binary_str(data)`, `import_binary_file(path)` |
| 静的メソッド | `import_binary_multi_str(data)`, `import_binary_multi_file(path)` |

### `UnreducedDD` class

| 種別 | 名前 |
|------|------|
| コンストラクタ | `__init__(val: int = 0)` |
| 演算子 | `__eq__`, `__ne__`, `__lt__`, `__hash__`, `__repr__`, `__bool__`, `__invert__` |
| メソッド | `raw_child0()`, `raw_child1()`, `raw_child(c: int)` |
| メソッド | `set_child0(child)`, `set_child1(child)` |
| メソッド | `reduce_as_bdd()`, `reduce_as_zdd()`, `reduce_as_qdd()` |
| プロパティ | `top_var`, `node_id`, `raw_size`, `is_terminal`, `is_one`, `is_zero`, `is_reduced` |
| 静的メソッド | `zero()`, `one()`, `getnode(var, lo, hi)` |
| 静的メソッド | `from_bdd(bdd)`, `from_zdd(zdd)`, `from_qdd(qdd)` |
| 静的メソッド | `wrap_raw_bdd(bdd)`, `wrap_raw_zdd(zdd)`, `wrap_raw_qdd(qdd)` |
| I/O | `export_binary_str()`, `export_binary_file(path)` |
| 静的メソッド | `import_binary_str(data)`, `import_binary_file(path)` |

### `SeqBDD` class

| 種別 | 名前 |
|------|------|
| コンストラクタ | `__init__(val: int = 0)` |
| 演算子 | `__eq__`, `__ne__`, `__hash__`, `__repr__`, `__str__`, `__bool__` |
| 演算子 | `__and__`, `__add__`, `__sub__`, `__mul__`, `__truediv__`, `__mod__` |
| 演算子 | `__iand__`, `__iadd__`, `__isub__`, `__imul__`, `__itruediv__`, `__imod__` |
| メソッド | `off_set(v)`, `on_set0(v)`, `on_set(v)`, `push(v)` |
| プロパティ | `top`, `zdd`, `size`, `card`, `lit`, `len` |
| I/O | `export_str()`, `export_file(path)`, `print_seq()` |

---

## Python `.pyi` — モジュールレベル関数が欠けている

| 関数名 | 説明 |
|--------|------|
| `new_vars(n: int, reverse: bool = False)` | 複数変数の一括作成 |
| `gc_rootcount()` | GC ルートの登録数 |
| `top_level()` | 最大レベル番号 |

---

## Python `.pyi` — BDD class のメソッドが欠けている

| メソッド名 | 説明 |
|-----------|------|
| `to_qdd()` | BDD → QDD 変換 |
| `count(n: int)` | モデル数のカウント（uint64_t） |
| `exact_count(n: int)` | モデル数の正確なカウント（Python 任意精度整数） |
| `uniform_sample(n: int, seed: int)` | 一様サンプリング |
| `child0()`, `child1()`, `child(c: int)` | 補辺を解決した子ノードの取得 |
| `raw_child0()`, `raw_child1()`, `raw_child(c: int)` | 生の子ノードの取得 |
| `export_binary_str()` | バイナリ形式での文字列エクスポート |
| `export_binary_file(path: str)` | バイナリ形式でのファイルエクスポート |
| `export_sapporo_str()` | Sapporo 形式での文字列エクスポート |
| `export_sapporo_file(path: str)` | Sapporo 形式でのファイルエクスポート |
| `save_graphviz_str(raw: bool = False)` | Graphviz DOT 文字列の生成 |
| `save_graphviz_file(path: str, raw: bool = False)` | Graphviz DOT ファイルの保存 |
| `import_binary_str(data: bytes)` (static) | バイナリ形式での文字列インポート |
| `import_binary_file(path: str)` (static) | バイナリ形式でのファイルインポート |
| `import_binary_multi_str(data: bytes)` (static) | 複数ルートのバイナリインポート（文字列） |
| `import_binary_multi_file(path: str)` (static) | 複数ルートのバイナリインポート（ファイル） |
| `import_sapporo_str(s: str)` (static) | Sapporo 形式での文字列インポート |
| `import_sapporo_file(path: str)` (static) | Sapporo 形式でのファイルインポート |

## Python `.pyi` — ZDD class のメソッドが欠けている

| メソッド名 | 説明 |
|-----------|------|
| `to_qdd()` | ZDD → QDD 変換 |
| `count()` | カーディナリティ（uint64_t） |
| `uniform_sample(seed: int)` | 一様サンプリング |
| `enumerate()` | 全集合の列挙（文字列） |
| `has_empty()` | 空集合を含むか判定 |
| `child0()`, `child1()`, `child(c: int)` | 補辺を解決した子ノードの取得 |
| `raw_child0()`, `raw_child1()`, `raw_child(c: int)` | 生の子ノードの取得 |
| `export_binary_str()` | バイナリ形式での文字列エクスポート |
| `export_binary_file(path: str)` | バイナリ形式でのファイルエクスポート |
| `export_sapporo_str()` | Sapporo 形式での文字列エクスポート |
| `export_sapporo_file(path: str)` | Sapporo 形式でのファイルエクスポート |
| `export_graphillion_str(offset: int = 0)` | Graphillion 形式での文字列エクスポート |
| `export_graphillion_file(path: str, offset: int = 0)` | Graphillion 形式でのファイルエクスポート |
| `save_graphviz_str(raw: bool = False)` | Graphviz DOT 文字列の生成 |
| `save_graphviz_file(path: str, raw: bool = False)` | Graphviz DOT ファイルの保存 |
| `import_binary_str(data: bytes)` (static) | バイナリ形式での文字列インポート |
| `import_binary_file(path: str)` (static) | バイナリ形式でのファイルインポート |
| `import_binary_multi_str(data: bytes)` (static) | 複数ルートのバイナリインポート（文字列） |
| `import_binary_multi_file(path: str)` (static) | 複数ルートのバイナリインポート（ファイル） |
| `import_sapporo_str(s: str)` (static) | Sapporo 形式での文字列インポート |
| `import_sapporo_file(path: str)` (static) | Sapporo 形式でのファイルインポート |
| `import_graphillion_str(s: str, offset: int = 0)` (static) | Graphillion 形式での文字列インポート |
| `import_graphillion_file(path: str, offset: int = 0)` (static) | Graphillion 形式でのファイルインポート |

## Python `.pyi` — PiDD class のメソッドが欠けている

| メソッド名 | 説明 |
|-----------|------|
| `__truediv__`, `__mod__` | 除算・剰余 |
| `__itruediv__`, `__imod__` | 複合代入の除算・剰余 |
| `print()` | 出力（文字列を返す） |
| `enum()` | 列挙（文字列を返す） |
| `enum2()` | 列挙2（文字列を返す） |

## Python `.pyi` — RotPiDD class のメソッドが欠けている

| メソッド名 | 説明 |
|-----------|------|
| `print()` | 出力（文字列を返す） |
| `enum()` | 列挙（文字列を返す） |
| `enum2()` | 列挙2（文字列を返す） |
| `contradiction_maximization(...)` | 矛盾最大化の最適化 |
