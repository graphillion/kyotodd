#ifndef KYOTODD_BDD_OPS_H
#define KYOTODD_BDD_OPS_H

#include "bdd_types.h"

inline bddp bddnot(bddp p) { return p ^ BDD_COMP_FLAG; }
bddp bddand(bddp f, bddp g);
bddp bddor(bddp f, bddp g);
bddp bddxor(bddp f, bddp g);
bddp bddnand(bddp f, bddp g);
bddp bddnor(bddp f, bddp g);
bddp bddxnor(bddp f, bddp g);

bddp bddat0(bddp f, bddvar v);
bddp bddat1(bddp f, bddvar v);
bddp bddite(bddp f, bddp g, bddp h);

int bddimply(bddp f, bddp g);
bddp bddsupport(bddp f);
std::vector<bddvar> bddsupport_vec(bddp f);
bddp bddexist(bddp f, bddp g);
bddp bddexist(bddp f, const std::vector<bddvar>& vars);
bddp bdduniv(bddp f, bddp g);
bddp bdduniv(bddp f, const std::vector<bddvar>& vars);
bddp bddlshift(bddp f, bddvar shift);
bddp bddrshift(bddp f, bddvar shift);
bddp bddcofactor(bddp f, bddp g);

// ZDD operations
bddp bddoffset(bddp f, bddvar var);

#endif
