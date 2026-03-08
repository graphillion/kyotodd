#ifndef KYOTODD_BDD_BASE_H
#define KYOTODD_BDD_BASE_H

#include "bdd_types.h"

void bddinit(uint64_t node_count = 256, uint64_t node_max = UINT64_MAX);
inline void BDD_Init(uint64_t node_count = 256, uint64_t node_max = UINT64_MAX) { bddinit(node_count, node_max); }
bddvar bddnewvar();
inline bddvar BDD_NewVar() { return bddnewvar(); }
bddvar bddnewvaroflev(bddvar lev);
bddvar bddlevofvar(bddvar var);
bddvar bddvaroflev(bddvar lev);
bddvar bddvarused();
bddvar bddtop(bddp f);
inline bddp bddcopy(bddp f) { return f; }
void bddfree(bddp f);
uint64_t bddused();
uint64_t bddsize(bddp f);
uint64_t bddvsize(bddp* p, int lim);
uint64_t bddvsize(const std::vector<bddp>& v);

bddp BDD_UniqueTableLookup(bddvar var, bddp lo, bddp hi);
void BDD_UniqueTableInsert(bddvar var, bddp lo, bddp hi, bddp node_id);
bddp getnode(bddvar var, bddp lo, bddp hi);
bddp getznode(bddvar var, bddp lo, bddp hi);
bddp bddprime(bddvar v);
BDD BDD_ID(bddp p);
BDD BDDvar(bddvar v);

bddp bddrcache(uint8_t op, bddp f, bddp g);
void bddwcache(uint8_t op, bddp f, bddp g, bddp result);
bddp bddrcache3(uint8_t op, bddp f, bddp g, bddp h);
void bddwcache3(uint8_t op, bddp f, bddp g, bddp h, bddp result);

// Garbage collection
void bddgc();
void bddgc_protect(bddp* p);
void bddgc_unprotect(bddp* p);
void bddgc_setthreshold(double threshold);
double bddgc_getthreshold();
uint64_t bddlive();

// Obsolete: always throws. Retained for API compatibility.
int bddisbdd(bddp f);
int bddiszbdd(bddp f);

#endif
