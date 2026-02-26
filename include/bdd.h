#ifndef KYOTODD_BDD_H
#define KYOTODD_BDD_H

#include <cstdint>
#include "bdd_node.h"

extern BddNode* bdd_nodes;
extern uint64_t bdd_node_count;
extern uint64_t bdd_node_max;

void BDD_Init(uint64_t node_count = 256, uint64_t node_max = UINT64_MAX);

#endif
