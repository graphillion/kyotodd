[English](README.md) | [日本語](README_ja.md)

# KyotoDD (SAPPOROBDD 2.0)

> **注意:** このプロジェクトのすべてのコードは AI（Claude）によって書かれています。現在は**アルファバージョン**であり、API は予告なく変更される可能性があります。

KyotoDD は、C++ で実装された高性能な BDD（二分決定グラフ）および ZDD（ゼロサプレス型二分決定グラフ）ライブラリです。Python バインディングも提供しています。

**ドキュメント:** [https://graphillion.github.io/kyotodd/](https://graphillion.github.io/kyotodd/)

## 特徴

- **BDD 演算**: AND, OR, XOR, NOT, ITE, コファクタリング, 存在/全称量化, 変数シフト, 含意チェック
- **ZDD 演算**: 和集合, 共通集合, 差集合, join, meet, delta, 除算, 剰余, offset/onset, 極大/極小集合, 最小ヒッティング集合, 下方閉包など
- **補数辺**: BDD・ZDD ともに補数辺によるコンパクトな表現
- **ガベージコレクション**: 閾値設定可能なマーク・アンド・スイープ GC
- **入出力**: BDD/ZDD の文字列・ファイルへのシリアライズ/デシリアライズ
- **Python バインディング**: pybind11 による完全な Python API
- **C++11 対応**: C++11 準拠のコンパイラで動作

## クイックスタート

### C++

```cpp
#include "bdd.h"

int main() {
    bddinit(256, UINT64_MAX);

    bddvar x1 = bddnewvar();
    bddvar x2 = bddnewvar();

    BDD a = BDDvar(x1);
    BDD b = BDDvar(x2);

    BDD f = a & b;   // AND
    BDD g = a | b;   // OR
    BDD h = ~a;      // NOT

    return 0;
}
```

### Python

```python
import kyotodd

x1 = kyotodd.new_var()
x2 = kyotodd.new_var()

a = kyotodd.BDD.var(x1)
b = kyotodd.BDD.var(x2)

f = a & b   # AND
g = a | b   # OR
h = ~a      # NOT
```

## インストール

### C++（CMake）

```bash
git clone https://github.com/graphillion/kyotodd.git
cd kyotodd
mkdir build && cd build
cmake ..
cmake --build .
```

### Python

```bash
pip install git+https://github.com/graphillion/kyotodd.git#subdirectory=python
```

要件: Python >= 3.7, CMake >= 3.14, C++11 コンパイラ

## ドキュメントのビルド

```bash
cd docs
pip install -r requirements.txt
doxygen Doxyfile
sphinx-build -b html . _build/html
```

## 著者

- **川原 純** (Jun Kawahara)
- **湊 真一** (Shin-ichi Minato)
- **Claude** (Anthropic)

## ライセンス

このプロジェクトは MIT ライセンスの下で公開されています。詳細は [LICENSE](LICENSE) を参照してください。
