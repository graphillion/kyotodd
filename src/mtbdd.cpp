#include "mtbdd.h"
#include <vector>
#include <stdexcept>

// --- Dynamic op code allocation ---

uint8_t mtbdd_next_op_code = 70;  // start past all static codes (max existing: 67)

uint8_t mtbdd_alloc_op_code() {
    if (mtbdd_next_op_code == 255) {
        throw std::overflow_error(
            "mtbdd: operation cache op code space exhausted");
    }
    return mtbdd_next_op_code++;
}

// --- Global terminal table registry ---

static std::vector<MTBDDTerminalTableBase*>& mtbdd_terminal_registry() {
    static auto* instance = new std::vector<MTBDDTerminalTableBase*>();
    return *instance;
}

void mtbdd_register_terminal_table(MTBDDTerminalTableBase* table) {
    mtbdd_terminal_registry().push_back(table);
}

void mtbdd_clear_all_terminal_tables() {
    for (auto* t : mtbdd_terminal_registry()) {
        t->clear();
    }
    // Reset dynamic op code counter
    mtbdd_next_op_code = 70;
}
