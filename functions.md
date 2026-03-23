# C++ グローバル関数と Python ラッパーの対応表

## 初期化・変数管理 (bdd_base.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddinit(node_count, node_max)` | `kyotodd.init(node_count, node_max)` | |
| `bddfinal()` | `kyotodd.finalize()` | |
| `bddnewvar()` | `kyotodd.new_var()` | |
| `bddnewvar(int n)` | `kyotodd.new_vars(n, reverse)` | Python版はreverse引数追加 |
| `bddnewvaroflev(lev)` | `kyotodd.new_var_of_level(lev)` | |
| `bddlevofvar(var)` | `kyotodd.to_level(var)` | |
| `bddvaroflev(lev)` | `kyotodd.to_var(level)` | |
| `bddvarused()` | `kyotodd.var_count()` | |
| `bddtoplev()` | `kyotodd.top_level()` | |
| `bddtop(f)` | `BDD.top_var` / `ZDD.top_var` / `QDD.top_var` | property |
| `bddcopy(f)` | — | inline, Python不要(参照カウント) |
| `bddfree(f)` | — | Python不要(デストラクタで自動管理) |
| `bddused()` | `kyotodd.node_count()` | |
| `bddconst(val)` | — | |
| `bddprime(v)` | — | `BDD.prime(v)` 内部で使用 |
| `BDDvar(v)` | `BDD.var(v)` | static method |

## ノードサイズ (bdd_base.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddsize(f)` | `BDD.raw_size` / `ZDD.raw_size` / `QDD.raw_size` | property |
| `bddplainsize(f, is_zdd)` | `BDD.plain_size` / `ZDD.plain_size` | property |
| `bddrawsize(vector<bddp>)` | `BDD.shared_size(bdds)` / `ZDD.shared_size(zdds)` | static method |
| `bddplainsize(vector<bddp>, is_zdd)` | `BDD.shared_plain_size(bdds)` / `ZDD.shared_plain_size(zdds)` | static method |
| `bddvsize(bddp*, size_t)` | — | deprecated |
| `bddvsize(vector<bddp>)` | — | deprecated |

## ユニークテーブル・ノード割り当て (bdd_base.h, 内部)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `BDD_UniqueTableLookup(var, lo, hi)` | — | 内部API |
| `BDD_UniqueTableInsert(var, lo, hi, node_id)` | — | 内部API |
| `allocate_node()` | — | 内部API |

## ID変換ヘルパー (bdd_base.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `BDD_ID(bddp)` | — | 内部ヘルパー |
| `ZDD_ID(bddp)` | — | 内部ヘルパー |
| `QDD_ID(bddp)` | — | 内部ヘルパー (qdd.h) |
| `SeqBDD_ID(bddp)` | — | 内部ヘルパー (seqbdd.h) |
| `ZDD_Meet(f, g)` | — | `ZDD.meet()` 内部で使用 |

## キャッシュAPI (bdd_base.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddrcache(op, f, g)` | — | 低レベルAPI |
| `bddwcache(op, f, g, result)` | — | 低レベルAPI |
| `bddrcache3(op, f, g, h)` | — | 低レベルAPI (3オペランド) |
| `bddwcache3(op, f, g, h, result)` | — | 低レベルAPI (3オペランド) |
| `BDD_CacheBDD(op, f, g)` | — | 内部ヘルパー |
| `BDD_CacheZDD(op, f, g)` | — | 内部ヘルパー |

※ 型付きラッパー `BDD::cache_get()` / `BDD::cache_put()` 等はクラスの static method として公開済み。

## GC API (bdd_base.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddgc()` | `kyotodd.gc()` | |
| `bddgc_protect(bddp*)` | — | Python不要(自動管理) |
| `bddgc_unprotect(bddp*)` | — | Python不要(自動管理) |
| `bddgc_setthreshold(threshold)` | `kyotodd.gc_set_threshold(threshold)` | |
| `bddgc_getthreshold()` | `kyotodd.gc_get_threshold()` | |
| `bddlive()` | `kyotodd.live_nodes()` | |
| `bddgc_rootcount()` | `kyotodd.gc_rootcount()` | |
| `bdd_check_reduced(root)` | — | |

## 非推奨 (bdd_base.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddisbdd(f)` | — | deprecated |
| `bddiszbdd(f)` | — | deprecated |

## BDD 論理演算 (bdd_ops.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddnot(p)` | `~BDD` | 演算子 `__invert__` |
| `bddand(f, g)` | `BDD & BDD` | 演算子 `__and__` |
| `bddor(f, g)` | `BDD \| BDD` | 演算子 `__or__` |
| `bddxor(f, g)` | `BDD ^ BDD` | 演算子 `__xor__` |
| `bddnand(f, g)` | `BDD.nand(other)` | |
| `bddnor(f, g)` | `BDD.nor(other)` | |
| `bddxnor(f, g)` | `BDD.xnor(other)` | |
| `bddite(f, g, h)` | `BDD.ite(f, g, h)` | static method |
| `bddimply(f, g)` | `BDD.imply(other)` | method |

## BDD Cofactor・量化 (bdd_ops.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddat0(f, v)` | `BDD.at0(v)` | |
| `bddat1(f, v)` | `BDD.at1(v)` | |
| `bddcofactor(f, g)` | `BDD.cofactor(g)` | |
| `bddsupport(f)` | `BDD.support` | property |
| `bddsupport_vec(f)` | `BDD.support_vec()` | method |
| `bddexist(f, g)` | `BDD.exist(cube)` | |
| `bddexist(f, vector<bddvar>)` | `BDD.exist(vars)` | |
| `bddexistvar(f, v)` | `BDD.exist(var)` | |
| `bdduniv(f, g)` | `BDD.univ(cube)` | |
| `bdduniv(f, vector<bddvar>)` | `BDD.univ(vars)` | |
| `bddunivvar(f, v)` | `BDD.univ(var)` | |
| `bddswap(f, v1, v2)` | `BDD.swap(v1, v2)` | |
| `bddsmooth(f, v)` | `BDD.smooth(v)` | |
| `bddspread(f, k)` | `BDD.spread(k)` | |

## BDD シフト (bdd_ops.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddlshiftb(f, shift)` | `BDD << shift` | 演算子 `__lshift__` |
| `bddrshiftb(f, shift)` | `BDD >> shift` | 演算子 `__rshift__` |

## BDD カウント (bdd_ops.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddcount(f, n)` | `BDD.count(n)` | BDD用 (double) |
| `bddexactcount(f, n)` | `BDD.exact_count(n)` | BDD用 (BigInt → Python int) |
| `bddexactcount(f, n, memo)` | `BDD.exact_count_with_memo(n, memo)` | BDD用 (memo付き) |

## ZDD 集合演算 (bdd_ops.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddoffset(f, var)` | `ZDD.offset(v)` | |
| `bddonset(f, var)` | `ZDD.onset(v)` | |
| `bddonset0(f, var)` | `ZDD.onset0(v)` | |
| `bddchange(f, var)` | `ZDD.change(v)` | |
| `bddunion(f, g)` | `ZDD + ZDD` | 演算子 `__add__` |
| `bddintersec(f, g)` | `ZDD & ZDD` | 演算子 `__and__` |
| `bddsubtract(f, g)` | `ZDD - ZDD` | 演算子 `__sub__` |
| `bdddiv(f, g)` | `ZDD / ZDD` | 演算子 `__truediv__` |
| `bddsymdiff(f, g)` | `ZDD ^ ZDD` | 演算子 `__xor__` |
| `bddremainder(f, g)` | `ZDD % ZDD` | 演算子 `__mod__` |
| `bddjoin(f, g)` | `ZDD * ZDD` | 演算子 `__mul__` |
| `bddmeet(f, g)` | `ZDD.meet(other)` | |
| `bdddelta(f, g)` | `ZDD.delta(g)` | |
| `bdddisjoin(f, g)` | `ZDD.disjoin(g)` | |
| `bddjointjoin(f, g)` | `ZDD.jointjoin(g)` | |

## ZDD フィルタリング (bdd_ops.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddrestrict(f, g)` | `ZDD.restrict(g)` | |
| `bddpermit(f, g)` | `ZDD.permit(g)` | |
| `bddnonsup(f, g)` | `ZDD.nonsup(g)` | |
| `bddnonsub(f, g)` | `ZDD.nonsub(g)` | |

## ZDD 単項演算 (bdd_ops.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddmaximal(f)` | `ZDD.maximal()` | |
| `bddminimal(f)` | `ZDD.minimal()` | |
| `bddminhit(f)` | `ZDD.minhit()` | |
| `bddclosure(f)` | `ZDD.closure()` | |

## ZDD プッシュ・シフト (bdd_ops.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddpush(f, v)` | `SeqBDD.push(v)` | SeqBDD method |
| `bddlshiftz(f, shift)` | `ZDD << shift` | 演算子 `__lshift__` |
| `bddrshiftz(f, shift)` | `ZDD >> shift` | 演算子 `__rshift__` |
| `bddlshift(f, shift)` | — | deprecated |
| `bddrshift(f, shift)` | — | deprecated |

## ZDD カウント (bdd_ops.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddcard(f)` | — | `ZDD.exact_count` 経由で代替可 |
| `bddcount(f)` | `ZDD.count()` | ZDD用 (double) |
| `bddexactcount(f)` | `ZDD.exact_count` | property (BigInt → Python int) |
| `bddexactcount(f, memo)` | `ZDD.exact_count_with_memo(memo)` | memo付き |
| `bddlit(f)` | `ZDD.lit` | property |
| `bddlen(f)` | `ZDD.max_set_size` | property |
| `bddhasempty(f)` | `ZDD.has_empty()` | |
| `bddcardmp16(f, s)` | — | deprecated, C++のみ |

## ZDD 分析・対称性 (bdd_ops.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddispoly(f)` | `ZDD.is_poly` | property |
| `bddswapz(f, v1, v2)` | `ZDD.swap(v1, v2)` | |
| `bddimplychk(f, v1, v2)` | `ZDD.imply_chk(v1, v2)` | |
| `bddcoimplychk(f, v1, v2)` | `ZDD.coimply_chk(v1, v2)` | |
| `bddsymchk(f, v1, v2)` | `ZDD.sym_chk(v1, v2)` | |
| `bddpermitsym(f, n)` | `ZDD.permit_sym(n)` | |
| `bddalways(f)` | `ZDD.always()` | |
| `bddimplyset(f, v)` | `ZDD.imply_set(v)` | |
| `bddsymgrp(f)` | `ZDD.sym_grp()` | |
| `bddsymgrpnaive(f)` | `ZDD.sym_grp_naive()` | |
| `bddsymset(f, v)` | `ZDD.sym_set(v)` | |
| `bddcoimplyset(f, v)` | `ZDD.coimply_set(v)` | |
| `bdddivisor(f)` | `ZDD.divisor()` | |

## ZDD その他 (bdd_ops.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `ZDD_Random(lev, density)` | `kyotodd.zdd_random(lev, density)` | |
| `ZDD_LCM_A(filename, threshold)` | — | C++のみ |
| `ZDD_LCM_C(filename, threshold)` | — | C++のみ |
| `ZDD_LCM_M(filename, threshold)` | — | C++のみ |

## レガシーExport/Import (bdd_io.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bddexport(FILE*, bddp*, int)` | `BDD.export_str()` / `BDD.export_file()` | |
| `bddexport(ostream, bddp*, int)` | 同上 | |
| `bddexport(FILE*, vector<bddp>)` | — | 複数root版 |
| `bddexport(ostream, vector<bddp>)` | — | 複数root版 |
| `bddimport(FILE*, bddp*, int)` | `BDD.import_str()` / `BDD.import_file()` | |
| `bddimport(istream, bddp*, int)` | 同上 | |
| `bddimport(FILE*, vector<bddp>)` | — | 複数root版 |
| `bddimport(istream, vector<bddp>)` | — | 複数root版 |
| `bddimportz(FILE*, bddp*, int)` | `ZDD.import_str()` / `ZDD.import_file()` | |
| `bddimportz(istream, bddp*, int)` | 同上 | |
| `bddimportz(FILE*, vector<bddp>)` | — | 複数root版 |
| `bddimportz(istream, vector<bddp>)` | — | 複数root版 |
| `ZDD_Import(FILE*)` | — | |
| `ZDD_Import(FILE*, vector<ZDD>)` | — | |
| `ZDD_Import(istream)` | — | |
| `ZDD_Import(istream, vector<ZDD>)` | — | |

## Knuth形式 I/O (bdd_io.h, deprecated)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bdd_export_knuth(FILE*, f, is_hex, offset)` | `BDD.export_knuth_file(path, is_hex, offset)` | deprecated |
| `bdd_export_knuth(ostream, f, is_hex, offset)` | `BDD.export_knuth_str(is_hex, offset)` | deprecated |
| `bdd_import_knuth(FILE*, is_hex, offset)` | `BDD.import_knuth_file(path, is_hex, offset)` | deprecated |
| `bdd_import_knuth(istream, is_hex, offset)` | `BDD.import_knuth_str(s, is_hex, offset)` | deprecated |
| `zdd_export_knuth(FILE*, f, is_hex, offset)` | `ZDD.export_knuth_file(path, is_hex, offset)` | deprecated |
| `zdd_export_knuth(ostream, f, is_hex, offset)` | `ZDD.export_knuth_str(is_hex, offset)` | deprecated |
| `zdd_import_knuth(FILE*, is_hex, offset)` | `ZDD.import_knuth_file(path, is_hex, offset)` | deprecated |
| `zdd_import_knuth(istream, is_hex, offset)` | `ZDD.import_knuth_str(s, is_hex, offset)` | deprecated |

## バイナリ形式 I/O — 単一root (bdd_io.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bdd_export_binary(FILE*, f)` | `BDD.export_binary_file(path)` | |
| `bdd_export_binary(ostream, f)` | `BDD.export_binary_str()` | |
| `bdd_import_binary(FILE*, ignore_type)` | `BDD.import_binary_file(path, ignore_type)` | |
| `bdd_import_binary(istream, ignore_type)` | `BDD.import_binary_str(data, ignore_type)` | |
| `zdd_export_binary(FILE*, f)` | `ZDD.export_binary_file(path)` | |
| `zdd_export_binary(ostream, f)` | `ZDD.export_binary_str()` | |
| `zdd_import_binary(FILE*, ignore_type)` | `ZDD.import_binary_file(path, ignore_type)` | |
| `zdd_import_binary(istream, ignore_type)` | `ZDD.import_binary_str(data, ignore_type)` | |
| `qdd_export_binary(FILE*, f)` | `QDD.export_binary_file(path)` | |
| `qdd_export_binary(ostream, f)` | `QDD.export_binary_str()` | |
| `qdd_import_binary(FILE*, ignore_type)` | `QDD.import_binary_file(path, ignore_type)` | |
| `qdd_import_binary(istream, ignore_type)` | `QDD.import_binary_str(data, ignore_type)` | |
| `unreduced_export_binary(FILE*, f)` | `UnreducedDD.export_binary_file(path)` | |
| `unreduced_export_binary(ostream, f)` | `UnreducedDD.export_binary_str()` | |
| `unreduced_import_binary(FILE*)` | `UnreducedDD.import_binary_file(path)` | |
| `unreduced_import_binary(istream)` | `UnreducedDD.import_binary_str(data)` | |

## バイナリ形式 I/O — 複数root (bdd_io.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bdd_export_binary_multi(FILE*, roots, n)` | `BDD.export_binary_multi_file(bdds, path)` | |
| `bdd_export_binary_multi(ostream, roots, n)` | `BDD.export_binary_multi_str(bdds)` | |
| `bdd_import_binary_multi(FILE*, ignore_type)` | `BDD.import_binary_multi_file(path, ignore_type)` | |
| `bdd_import_binary_multi(istream, ignore_type)` | `BDD.import_binary_multi_str(data, ignore_type)` | |
| `zdd_export_binary_multi(FILE*, roots, n)` | `ZDD.export_binary_multi_file(zdds, path)` | |
| `zdd_export_binary_multi(ostream, roots, n)` | `ZDD.export_binary_multi_str(zdds)` | |
| `zdd_import_binary_multi(FILE*, ignore_type)` | `ZDD.import_binary_multi_file(path, ignore_type)` | |
| `zdd_import_binary_multi(istream, ignore_type)` | `ZDD.import_binary_multi_str(data, ignore_type)` | |
| `qdd_export_binary_multi(FILE*, roots, n)` | `QDD.export_binary_multi_file(qdds, path)` | |
| `qdd_export_binary_multi(ostream, roots, n)` | `QDD.export_binary_multi_str(qdds)` | |
| `qdd_import_binary_multi(FILE*, ignore_type)` | `QDD.import_binary_multi_file(path, ignore_type)` | |
| `qdd_import_binary_multi(istream, ignore_type)` | `QDD.import_binary_multi_str(data, ignore_type)` | |

## Sapporo形式 I/O (bdd_io.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bdd_export_sapporo(FILE*, f)` | `BDD.export_sapporo_file(path)` | |
| `bdd_export_sapporo(ostream, f)` | `BDD.export_sapporo_str()` | |
| `bdd_import_sapporo(FILE*)` | `BDD.import_sapporo_file(path)` | |
| `bdd_import_sapporo(istream)` | `BDD.import_sapporo_str(s)` | |
| `zdd_export_sapporo(FILE*, f)` | `ZDD.export_sapporo_file(path)` | |
| `zdd_export_sapporo(ostream, f)` | `ZDD.export_sapporo_str()` | |
| `zdd_import_sapporo(FILE*)` | `ZDD.import_sapporo_file(path)` | |
| `zdd_import_sapporo(istream)` | `ZDD.import_sapporo_str(s)` | |

## Graphillion形式 I/O (bdd_io.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `zdd_export_graphillion(FILE*, f, offset)` | `ZDD.export_graphillion_file(path, offset)` | |
| `zdd_export_graphillion(ostream, f, offset)` | `ZDD.export_graphillion_str(offset)` | |
| `zdd_import_graphillion(FILE*, offset)` | `ZDD.import_graphillion_file(path, offset)` | |
| `zdd_import_graphillion(istream, offset)` | `ZDD.import_graphillion_str(s, offset)` | |

## Graphviz DOT I/O (bdd_io.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bdd_save_graphviz(FILE*, f, mode)` | `BDD.save_graphviz_file(path, raw)` | |
| `bdd_save_graphviz(ostream, f, mode)` | `BDD.save_graphviz_str(raw)` | |
| `zdd_save_graphviz(FILE*, f, mode)` | `ZDD.save_graphviz_file(path, raw)` | |
| `zdd_save_graphviz(ostream, f, mode)` | `ZDD.save_graphviz_str(raw)` | |

## Dump・Graph (bdd_io.h, deprecated)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bdddump(f)` | — | deprecated |
| `bddvdump(p, n)` | — | deprecated |
| `bddgraph0(f)` | — | deprecated |
| `bddgraph(f)` | — | deprecated |
| `bddvgraph0(ptr, lim)` | — | deprecated |
| `bddvgraph(ptr, lim)` | — | deprecated |

## 内部テンプレート (bdd_internal.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `bdd_lshift_core(f, shift, op, make_node)` | — | 内部テンプレート |
| `bdd_rshift_core(f, shift, op, make_node)` | — | 内部テンプレート |
| `bdd_should_gc()` | — | 内部API |

## PiDD (pidd.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `PiDD_NewVar()` | `kyotodd.pidd_newvar()` | |
| `PiDD_VarUsed()` | `kyotodd.pidd_var_used()` | |

## RotPiDD (rotpidd.h)

| C++ グローバル関数 | Python ラッパー | 備考 |
|---|---|---|
| `RotPiDD_NewVar()` | `kyotodd.rotpidd_newvar()` | |
| `RotPiDD_VarUsed()` | `kyotodd.rotpidd_var_used()` | |
