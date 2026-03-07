#ifndef KYOTODD_BDD_OPS_H
#define KYOTODD_BDD_OPS_H

#include "bdd_types.h"

inline bddp bddnot(bddp p) { if (p == bddnull) return bddnull; return p ^ BDD_COMP_FLAG; }
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
bddp bddexist(bddp f, bddvar v);
bddp bdduniv(bddp f, bddp g);
bddp bdduniv(bddp f, const std::vector<bddvar>& vars);
bddp bdduniv(bddp f, bddvar v);
bddp bddlshift(bddp f, bddvar shift);
bddp bddrshift(bddp f, bddvar shift);
bddp bddcofactor(bddp f, bddp g);

// ZDD operations
bddp bddoffset(bddp f, bddvar var);
bddp bddonset(bddp f, bddvar var);
bddp bddonset0(bddp f, bddvar var);
bddp bddchange(bddp f, bddvar var);
bddp bddunion(bddp f, bddp g);
bddp bddintersec(bddp f, bddp g);
bddp bddsubtract(bddp f, bddp g);
bddp bdddiv(bddp f, bddp g);
bddp bddsymdiff(bddp f, bddp g);
bddp bddjoin(bddp f, bddp g);
bddp bddmeet(bddp f, bddp g);
bddp bdddelta(bddp f, bddp g);
bddp bddremainder(bddp f, bddp g);
bddp bdddisjoin(bddp f, bddp g);
bddp bddjointjoin(bddp f, bddp g);
bddp bddrestrict(bddp f, bddp g);
bddp bddpermit(bddp f, bddp g);
bddp bddnonsup(bddp f, bddp g);
bddp bddnonsub(bddp f, bddp g);

// Unary ZDD operations
bddp bddmaximal(bddp f);
bddp bddminimal(bddp f);
bddp bddminhit(bddp f);
bddp bddclosure(bddp f);

// ZDD counting
uint64_t bddcard(bddp f);

#endif
