TdZddSample — TdZdd specs with KyotoDD
======================================

Shows how to use a TdZdd spec (a top-down description of a decision
diagram) with KyotoDD:

  * UnreducedDD::construct(spec)   builds the spec directly into
                                   KyotoDD's node table, then
                                   reduce_as_zdd() / reduce_as_bdd();
  * SkipPadding::Zdd               pins level-skipping edges to "0"
                                   before a BDD reduction;
  * ZDD::subset(spec)              restricts an existing ZDD to the sets
                                   the spec accepts (TdZdd's zddSubset);
  * UnreducedDD::zdd_subset(spec)  the same on an unreduced input.

Problem: n items with small weights. Enumerate the subsets of total
weight <= C (WeightBound spec), then keep those with no two consecutive
items (NoAdjacent spec). Each result is cross-checked against the
classic KyotoDD operations (cost_bound_le, intersection).

Requires the TdZdd submodule: clone with --recursive or run
    git submodule update --init

Usage:
    tdzdd_sample [n] [C]        (default n = 12, C = 20)
