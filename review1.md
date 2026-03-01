# KyotoDD コードレビュー

## 確認範囲

- `include/`, `src/`, `test/`, `CMakeLists.txt` を全件確認
- 既存テスト `build/kyotodd_test` は 419 件すべて成功
- ただし、下記 4 件は既存テスト外の境界条件で再現を確認

## 指摘事項

### 1. `node_max` 到達時に `getnode` / `getznode` が領域外書き込みする

- 該当箇所: `src/bdd_base.cpp:317-324`, `src/bdd_base.cpp:341-350`, `src/bdd_base.cpp:368-377`
- `node_array_grow()` は `bdd_node_max` に達していると `bdd_node_count` を増やさないまま返ります。
- その直後に `getnode()` / `getznode()` は `bdd_node_used++` して `node_write()` を呼ぶため、確保済み配列の外へ書き込みます。
- `bddinit(1, 1)` のような小さい上限で 2 個目のノードを作るだけで発火します。ASan 付きで `bddprime(v1); bddprime(v2);` を実行すると `heap-buffer-overflow` を確認しました。
- これはメモリ破壊なので最優先で修正すべきです。

### 2. `BDD::Null` / `ZDD::Null` が公開されているのに、ほとんどの演算が `bddnull` を安全に扱えない

- 該当箇所: `include/bdd_types.h:109-111`, `include/bdd_types.h:141-143`, `include/bdd.h:9-68`, `src/bdd_ops.cpp:7-69`, `include/bdd_internal.h:32-35`
- `BDD::Null` と `ZDD::Null` は公開定数ですが、演算子はそのまま `bddand()` などの生 API に流し、内部では `node_var()` を即座に呼びます。
- `bddnull` は terminal でも regular node でもない特別値なので、`node_var(bddnull)` は巨大な添字で `bdd_nodes` を読み、簡単に SEGV します。
- 実際に `bddand(bddnull, bddprime(v))` を ASan 付きで実行すると `node_var()` でクラッシュしました。
- `bddtop()` だけは `bddnull` を明示的に拒否している (`src/bdd_base.cpp:247-255`) ので、他 API も同様の防御が必要です。

### 3. export/import が variable number を保存しないため、非デフォルト順序でラウンドトリップすると別の変数に化ける

- 該当箇所: `src/bdd_io.cpp:61-78`, `src/bdd_io.cpp:198-212`, `src/bdd_io.cpp:235-248`
- 仕様では variable number と level は別概念ですが、エクスポートでは level しか書かれていません。
- インポート側は `bddvaroflev(level)` で現在の level->var 対応を引き直すため、ファイルに含まれていた元の variable number は復元できません。
- 例: `bddnewvaroflev(1)` で順序を変更した後に `bddprime(v1)` を export し、初期状態に戻して import すると、復元後のトップ変数は `v1` ではなく level 2 側の別変数になります。
- 既存テストはデフォルト順序の round-trip しか見ていないため、この壊れ方を検出できていません。

### 4. 多くの公開 API が `bddvar` の範囲チェックをしておらず、未定義動作やクラッシュを起こす

- 該当箇所の例: `src/bdd_ops.cpp:238-271`, `src/bdd_ops.cpp:274-307`, `src/bdd_ops.cpp:657-798`
- `bddlevofvar()` / `bddvaroflev()` は範囲チェックしていますが、`bddat0()`, `bddat1()`, `bddoffset()`, `bddonset()`, `bddonset0()`, `bddchange()` などは `var2level[var]` を直接参照しています。
- そのため `var == 0` や `var > bdd_varcount` が入ると、静かに誤動作するか、範囲外アクセスで落ちます。
- 実際に `bddat0(bddprime(v), 999999)` を ASan 付きで実行すると SEGV しました。`bddat0(..., 0)` もこの環境では「たまたま」恒等写像のように振る舞いましたが、これは未定義動作です。
- ライブラリ API として公開している以上、境界チェックなしは危険です。

## 補足

- 既存テストはかなり手厚いですが、今回の 4 件はいずれも未カバーでした。
- 追加するなら、少なくとも以下の回帰テストが必要です。
  - `node_max` ぴったり到達時の失敗動作
  - `BDD::Null` / `ZDD::Null` を含む API 呼び出し
  - 非デフォルト variable-level mapping での export/import round-trip
  - 不正 `bddvar` 入力の例外 or エラー処理
