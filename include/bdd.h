#ifndef KYOTODD_BDD_H
#define KYOTODD_BDD_H

#include <cstdio>
#include <iosfwd>
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

void bddexport(FILE* strm, bddp* p, int lim);
void bddexport(FILE* strm, const std::vector<bddp>& v);
void bddexport(std::ostream& strm, bddp* p, int lim);
void bddexport(std::ostream& strm, const std::vector<bddp>& v);

int bddimport(FILE* strm, bddp* p, int lim);
int bddimport(FILE* strm, std::vector<bddp>& v);
int bddimport(std::istream& strm, bddp* p, int lim);
int bddimport(std::istream& strm, std::vector<bddp>& v);
int bddimportz(FILE* strm, bddp* p, int lim);
int bddimportz(FILE* strm, std::vector<bddp>& v);
int bddimportz(std::istream& strm, bddp* p, int lim);
int bddimportz(std::istream& strm, std::vector<bddp>& v);

int bddisbdd(bddp f);
int bddiszbdd(bddp f);

bddp bddrcache(uint8_t op, bddp f, bddp g);
void bddwcache(uint8_t op, bddp f, bddp g, bddp result);
bddp bddrcache3(uint8_t op, bddp f, bddp g, bddp h);
void bddwcache3(uint8_t op, bddp f, bddp g, bddp h, bddp result);

inline BDD BDD::operator&(const BDD& other) const {
    BDD b(0);
    b.root = bddand(root, other.root);
    return b;
}

inline BDD& BDD::operator&=(const BDD& other) {
    root = bddand(root, other.root);
    return *this;
}

#endif
