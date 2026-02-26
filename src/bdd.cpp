#include "bdd.h"
#include <cstdlib>

BddNode* bdd_nodes = nullptr;
uint64_t bdd_node_count = 0;
uint64_t bdd_node_max = 0;

uint32_t* var2level = nullptr;
uint32_t* level2var = nullptr;
uint32_t bdd_varcount = 0;

void BDD_Init(uint64_t node_count, uint64_t node_max) {
    bdd_node_count = node_count;
    bdd_node_max = node_max;
    bdd_nodes = static_cast<BddNode*>(std::malloc(sizeof(BddNode) * node_count));
}

uint32_t BDD_NewVar() {
    bdd_varcount++;
    uint32_t var = bdd_varcount;
    var2level = static_cast<uint32_t*>(std::realloc(var2level, sizeof(uint32_t) * (var + 1)));
    level2var = static_cast<uint32_t*>(std::realloc(level2var, sizeof(uint32_t) * (var + 1)));
    var2level[var] = var;  // var i <-> level i
    level2var[var] = var;
    return var;
}
