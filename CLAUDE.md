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
- **Node ID**: Even natural numbers (2, 4, 6, ...).

## Repository

- `conversation/` directory stores conversation logs and is gitignored.
- `build/` directory is gitignored.
