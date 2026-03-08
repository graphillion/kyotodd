[English](README.md) | [日本語](README_ja.md)

# KyotoDD (SAPPOROBDD 2.0)

> **Note:** All code in this project is written by AI (Claude). This is currently an **alpha version** — APIs may change without notice.

KyotoDD is a high-performance BDD (Binary Decision Diagram) and ZDD (Zero-suppressed Decision Diagram) library written in C++ with Python bindings.

**Documentation:** [https://graphillion.github.io/kyotodd/](https://graphillion.github.io/kyotodd/)

## Features

- **BDD operations**: AND, OR, XOR, NOT, ITE, cofactoring, existential/universal quantification, variable shifting, implication checking
- **ZDD operations**: union, intersection, subtraction, join, meet, delta, division, remainder, offset/onset, maximal/minimal sets, minimum hitting sets, downward closure, and more
- **Complement edges**: Both BDD and ZDD use complement edges for compact representation
- **Garbage collection**: Automatic mark-and-sweep GC with configurable threshold
- **Import/Export**: Serialize and deserialize BDD/ZDD to strings or files
- **Python bindings**: Full-featured Python API via pybind11
- **C++11 compatible**: Works with any C++11-compliant compiler

## Quick Start

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

x1 = kyotodd.newvar()
x2 = kyotodd.newvar()

a = kyotodd.BDD.var(x1)
b = kyotodd.BDD.var(x2)

f = a & b   # AND
g = a | b   # OR
h = ~a      # NOT
```

## Installation

### C++ (CMake)

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

Requirements: Python >= 3.7, CMake >= 3.14, C++11 compiler.

## Building Documentation

```bash
cd docs
pip install -r requirements.txt
doxygen Doxyfile
sphinx-build -b html . _build/html
```

## Authors

- **Jun Kawahara**
- **Shin-ichi Minato**
- **Claude** (Anthropic)

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
