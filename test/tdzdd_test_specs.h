// Specs and helpers shared by the TdZdd-related tests
// (test_unreduced_dd_tdzdd.cpp, test_tdzdd_subset.cpp).
#ifndef KYOTODD_TEST_TDZDD_TEST_SPECS_H
#define KYOTODD_TEST_TDZDD_TEST_SPECS_H

#include "bdd.h"
#include "unreduced_dd_tdzdd.h"
#include <tdzdd/DdStructure.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace kyotodd;

// =============================================================
// Specs used by the tests
// =============================================================

// Choose exactly k out of n items (TdZdd user guide example).
class Combination : public tdzdd::DdSpec<Combination, int, 2> {
    int const n;
    int const k;
public:
    Combination(int n, int k) : n(n), k(k) {}
    int getRoot(int& state) const {
        state = 0;
        return n;
    }
    int getChild(int& state, int level, int value) const {
        state += value;
        if (--level == 0) return (state == k) ? -1 : 0;
        if (state > k) return 0;
        if (state + level < k) return 0;
        return level;
    }
};

// All subsets of n items (every path leads to the 1-terminal).
class AllSubsets : public tdzdd::StatelessDdSpec<AllSubsets, 2> {
    int const n;
public:
    explicit AllSubsets(int n) : n(n) {}
    int getRoot() const { return n; }
    int getChild(int level, int /*value*/) const {
        return (level == 1) ? -1 : level - 1;
    }
};

// Level-skipping spec on 3 levels (stateless):
//   level 3: lo -> level 1 (skips level 2), hi -> level 2
//   level 2: lo, hi -> level 1
//   level 1: lo -> 0, hi -> 1
class Skip3 : public tdzdd::StatelessDdSpec<Skip3, 2> {
public:
    int getRoot() const { return 3; }
    int getChild(int level, int value) const {
        if (level == 3) return value ? 2 : 1;
        if (level == 2) return 1;
        return value ? -1 : 0;
    }
};

// Level-skipping spec on 4 levels where two parents skip to the same child:
//   level 4: lo -> level 1, hi -> level 3
//   level 3: lo -> level 1, hi -> level 2
//   level 2: lo, hi -> level 1
//   level 1: lo -> 0, hi -> 1
class Skip4 : public tdzdd::StatelessDdSpec<Skip4, 2> {
public:
    int getRoot() const { return 4; }
    int getChild(int level, int value) const {
        if (level == 4) return value ? 3 : 1;
        if (level == 3) return value ? 2 : 1;
        if (level == 2) return 1;
        return value ? -1 : 0;
    }
};

// Edge to the 1-terminal that skips level 1:
//   level 2: lo -> 0, hi -> 1-terminal
class SkipToOne : public tdzdd::StatelessDdSpec<SkipToOne, 2> {
public:
    int getRoot() const { return 2; }
    int getChild(int /*level*/, int value) const { return value ? -1 : 0; }
};

// Keep only the path with the minimum (KeepMax: maximum) number of 1s.
// All states at a level compare equal; mergeStates decides which to keep.
// Every path of length n is accepted, so without merging the result
// would be the constant-1 function.
template<bool KeepMax>
class KeepExtreme : public tdzdd::DdSpec<KeepExtreme<KeepMax>, int, 2> {
    int const n;
public:
    explicit KeepExtreme(int n) : n(n) {}
    int getRoot(int& state) const { state = 0; return n; }
    int getChild(int& state, int level, int value) const {
        state += value;
        return (level == 1) ? -1 : level - 1;
    }
    size_t hashCode(int const&) const { return 0; }
    bool equalTo(int const&, int const&) const { return true; }
    int mergeStates(int& s1, int& s2) const {
        if (s1 == s2) return 0;
        bool keep_first = KeepMax ? (s1 > s2) : (s1 < s2);
        return keep_first ? 2 : 1;
    }
};

// Combination written as a PodArrayDdSpec with a one-element array.
class PodCombination : public tdzdd::PodArrayDdSpec<PodCombination, int, 2> {
    int const n;
    int const k;
public:
    PodCombination(int n, int k) : n(n), k(k) { setArraySize(1); }
    int getRoot(int* a) const { a[0] = 0; return n; }
    int getChild(int* a, int level, int value) const {
        a[0] += value;
        if (--level == 0) return (a[0] == k) ? -1 : 0;
        if (a[0] > k || a[0] + level < k) return 0;
        return level;
    }
};

// State that counts live instances, to check construct/destruct balance.
struct Counted {
    static int& live() { static int n = 0; return n; }
    int v;
    Counted() : v(0) { ++live(); }
    Counted(const Counted& o) : v(o.v) { ++live(); }
    ~Counted() { --live(); }
    bool operator==(const Counted& o) const { return v == o.v; }
};

class CountedCombination : public tdzdd::DdSpec<CountedCombination, Counted, 2> {
    int const n;
    int const k;
    int const throw_at;   // level at which getChild throws (0 = never)
public:
    CountedCombination(int n, int k, int throw_at = 0)
        : n(n), k(k), throw_at(throw_at) {}
    int getRoot(Counted& s) const { s.v = 0; return n; }
    int getChild(Counted& s, int level, int value) const {
        if (level == throw_at) throw std::runtime_error("boom");
        s.v += value;
        if (--level == 0) return (s.v == k) ? -1 : 0;
        if (s.v > k || s.v + level < k) return 0;
        return level;
    }
    size_t hashCode(Counted const& s) const { return static_cast<size_t>(s.v); }
};

// State is a std::string of the chosen bits: every path has a distinct
// state (no sharing), and the state must survive arena growth without
// being relocated.
class StringPaths : public tdzdd::DdSpec<StringPaths, std::string, 2> {
    int const n;
    int const k;
public:
    StringPaths(int n, int k) : n(n), k(k) {}
    int getRoot(std::string& s) const { s.clear(); return n; }
    int getChild(std::string& s, int level, int value) const {
        s.push_back(value ? '1' : '0');
        if (--level == 0) {
            int ones = 0;
            for (size_t i = 0; i < s.size(); ++i) ones += (s[i] == '1');
            return (ones == k) ? -1 : 0;
        }
        return level;
    }
    size_t hashCode(std::string const& s) const { return std::hash<std::string>()(s); }
};

// getChild returns the current level (not lower): must be rejected.
class BadLevel : public tdzdd::StatelessDdSpec<BadLevel, 2> {
public:
    int getRoot() const { return 3; }
    int getChild(int level, int) const { return level; }
};

// Order-dependent merging: states compare equal by `key` only, and
// mergeStates always keeps the state that was presented FIRST (return 2).
// Which state survives therefore depends on the order in which equal
// states are visited, so the result must match tdzdd::DdBuilder's order
// exactly. Transitions are a fixed pseudo-random table so that paths with
// the same key carry different costs.
struct KeyCost { int key; int cost; };
class KeepFirst : public tdzdd::DdSpec<KeepFirst, KeyCost, 2> {
    int const n;
    int const nkeys;
    unsigned seed;
    // (level, key, value) -> (next level, next key, cost added)
    int next_level(int level, int key, int value) const {
        unsigned h = mix(level, key, value);
        if (level == 1) return (h % 2) ? -1 : 0;
        if (h % 10 == 0) return (h / 10 % 2) ? -1 : 0;      // terminal
        if (level >= 3 && h % 10 < 3) return 1 + (int)(h / 10 % (level - 1)); // skip
        return level - 1;
    }
    int next_key(int level, int key, int value) const {
        return (int)(mix(level, key, value) / 100 % nkeys);
    }
    int add_cost(int level, int key, int value) const {
        return (int)(mix(level, key, value) / 1000 % 3);
    }
    unsigned mix(int level, int key, int value) const {
        unsigned h = seed ^ (unsigned)level * 2654435761u;
        h ^= (unsigned)key * 40503u + (unsigned)value * 97u;
        h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
        return h;
    }
public:
    KeepFirst(int n, int nkeys, unsigned seed) : n(n), nkeys(nkeys), seed(seed) {}
    int getRoot(KeyCost& s) const { s.key = 0; s.cost = 0; return n; }
    int getChild(KeyCost& s, int level, int value) const {
        int lv = next_level(level, s.key, value);
        s.cost += add_cost(level, s.key, value);
        s.key = next_key(level, s.key, value);
        return lv;
    }
    size_t hashCode(KeyCost const& s) const { return (size_t)s.key; }
    bool equalTo(KeyCost const& a, KeyCost const& b) const { return a.key == b.key; }
    int mergeStates(KeyCost& a, KeyCost& b) const {
        return (a.cost == b.cost) ? 0 : 2;   // keep the first one presented
    }
};

// Convert a tdzdd::DdStructure into an UnreducedDD of the same shape
// (level i -> bddvaroflev(i)), for comparing reductions. The result and
// every memoized node are UnreducedDD objects, hence GC-protected.
inline UnreducedDD from_tdzdd(const tdzdd::DdStructure<2>& dd, tdzdd::NodeId f,
                              std::unordered_map<uint64_t, UnreducedDD>& memo) {
    if (f.row() == 0) return UnreducedDD(f.col() ? 1 : 0);
    auto it = memo.find(f.code());
    if (it != memo.end()) return it->second;
    UnreducedDD lo = from_tdzdd(dd, dd.child(f, 0), memo);
    UnreducedDD hi = from_tdzdd(dd, dd.child(f, 1), memo);
    UnreducedDD r = UnreducedDD::getnode(bddvaroflev(f.row()), lo, hi);
    memo.insert(std::make_pair(f.code(), r));
    return r;
}

inline uint64_t binom(int n, int k) {
    if (k < 0 || k > n) return 0;
    uint64_t r = 1;
    for (int i = 1; i <= k; ++i) r = r * (n - k + i) / i;
    return r;
}

// Count reachable non-terminal nodes without any reduction interpretation.
inline size_t walk_count(const UnreducedDD& dd) {
    std::unordered_set<bddp> seen;
    std::vector<bddp> stack;
    stack.push_back(dd.id() & ~BDD_COMP_FLAG);
    while (!stack.empty()) {
        bddp f = stack.back(); stack.pop_back();
        if (f & BDD_CONST_FLAG) continue;
        if (!seen.insert(f).second) continue;
        stack.push_back(DDBase::raw_child0(f) & ~BDD_COMP_FLAG);
        stack.push_back(DDBase::raw_child1(f) & ~BDD_COMP_FLAG);
    }
    return seen.size();
}

// True iff every edge goes to a terminal or to a node exactly one
// KyotoDD level below (quasi-reduced shape).
inline bool is_quasi_reduced(const UnreducedDD& dd) {
    std::unordered_set<bddp> seen;
    std::vector<bddp> stack;
    stack.push_back(dd.id() & ~BDD_COMP_FLAG);
    while (!stack.empty()) {
        bddp f = stack.back(); stack.pop_back();
        if (f & BDD_CONST_FLAG) continue;
        if (!seen.insert(f).second) continue;
        bddvar flev = bddlevofvar(node_var(f));
        for (int b = 0; b < 2; ++b) {
            bddp c = DDBase::raw_child(f, b) & ~BDD_COMP_FLAG;
            if (c & BDD_CONST_FLAG) continue;
            bddvar clev = bddlevofvar(node_var(c));
            if (clev + 1 != flev) return false;
            stack.push_back(c);
        }
    }
    return true;
}

#endif
