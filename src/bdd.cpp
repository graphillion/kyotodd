#include "bdd.h"
#include <cstdlib>

BddNode* bdd_nodes = nullptr;
uint64_t bdd_node_count = 0;
uint64_t bdd_node_max = 0;

void BDD_Init(uint64_t node_count, uint64_t node_max) {
    bdd_node_count = node_count;
    bdd_node_max = node_max;
    bdd_nodes = static_cast<BddNode*>(std::malloc(sizeof(BddNode) * node_count));
}
