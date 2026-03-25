#include "mtbdd.h"
#include <vector>

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
}
