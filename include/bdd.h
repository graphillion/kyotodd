#ifndef KYOTODD_BDD_H
#define KYOTODD_BDD_H

#include <cstdint>
#include "bdd_node.h"

extern BddNode* bdd_nodes;
extern uint64_t bdd_node_count;
extern uint64_t bdd_node_max;

// Variable-level mapping
// Level 0 = terminal, level i = var i (initial ordering: var i <-> level i)
extern uint32_t* var2level;  // var2level[var] = level
extern uint32_t* level2var;  // level2var[level] = var
extern uint32_t bdd_varcount;

void BDD_Init(uint64_t node_count = 256, uint64_t node_max = UINT64_MAX);
uint32_t BDD_NewVar();

#endif
