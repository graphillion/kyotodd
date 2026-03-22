#include "mlzbddv.h"
#include <iostream>
#include <stdexcept>

// ============================================================
// Constructors, destructor, assignment
// ============================================================

MLZBDDV::MLZBDDV()
    : _pin(0), _out(0), _sin(0), _zbddv() {}

MLZBDDV::MLZBDDV(ZBDDV& zbddv) {
    int top = zbddv.Top();
    int pin = (top > 0) ? static_cast<int>(BDD_LevOfVar(
                              static_cast<bddvar>(top)))
                        : 0;
    int out = zbddv.Last() + 1;
    MLZBDDV tmp(zbddv, pin, out);
    _pin = tmp._pin;
    _out = tmp._out;
    _sin = tmp._sin;
    _zbddv = tmp._zbddv;
}

MLZBDDV::MLZBDDV(ZBDDV& zbddv, int pin, int out) {
    // Error value check
    if (zbddv.GetMetaZBDD().GetID() == bddnull) {
        _pin = 0;
        _out = 0;
        _sin = 0;
        _zbddv = zbddv;
        return;
    }

    _pin = pin;
    _out = out;
    _sin = 0;
    _zbddv = zbddv;

    // --------------------------------------------------------
    // Phase 1: inter-output division
    // --------------------------------------------------------
    for (int i = 0; i < _out; i++) {
        _sin++;
        int plev = _pin + _sin;

        // Create new variable if needed
        while (plev > static_cast<int>(BDD_TopLev())) {
            BDD_NewVar();
        }

        ZDD p = _zbddv.GetZBDD(i);
        if (p.GetID() == bddempty) continue;

        bddvar p_top_lev = (p.Top() > 0)
            ? BDD_LevOfVar(p.Top()) : 0;

        for (int j = 0; j < _out; j++) {
            if (j == i) continue;

            ZDD f = _zbddv.GetZBDD(j);
            if (f.GetID() == bddempty) continue;

            bddvar f_top_lev = (f.Top() > 0)
                ? BDD_LevOfVar(f.Top()) : 0;

            if (f_top_lev < p_top_lev) continue;

            ZDD q = f / p;
            if (q.GetID() == bddempty) continue;

            bddvar v = BDD_VarOfLev(static_cast<bddvar>(plev));
            ZDD new_f = q.Change(v) + (f % p);

            // Update _zbddv: subtract old, add new
            _zbddv -= ZBDDV(f, j);
            _zbddv += ZBDDV(new_f, j);

            if (_zbddv.GetMetaZBDD().GetID() == bddnull) {
                throw std::overflow_error("MLZBDDV: overflow in phase 1");
            }
        }
    }

    // --------------------------------------------------------
    // Phase 2: iterative kernel extraction
    // --------------------------------------------------------
    for (int i = 0; i < _out; i++) {
        for (;;) {
            ZDD f = _zbddv.GetZBDD(i);
            ZDD p = f.Divisor();

            // Stop if p is constant or p == f (trivial factor)
            if (p.Top() == 0) break;
            if (p == f) break;

            _sin++;
            int plev = _pin + _sin;

            while (plev > static_cast<int>(BDD_TopLev())) {
                BDD_NewVar();
            }

            // Store sub-expression definition at ZBDDV index (_sin - 1).
            // Phase 1 allocated _out entries, so the first phase-2
            // entry (_sin = _out+1) goes to index _out, consistent
            // with section 1.2 of the spec.
            _zbddv += ZBDDV(p, _sin - 1);

            bddvar v = BDD_VarOfLev(static_cast<bddvar>(plev));
            ZDD new_f = (f / p).Change(v) + (f % p);

            // Replace output i
            _zbddv -= ZBDDV(f, i);
            _zbddv += ZBDDV(new_f, i);

            if (_zbddv.GetMetaZBDD().GetID() == bddnull) {
                throw std::overflow_error("MLZBDDV: overflow in phase 2");
            }

            bddvar p_top_lev = (p.Top() > 0)
                ? BDD_LevOfVar(p.Top()) : 0;

            // Apply same sub-expression to other outputs
            for (int j = 0; j < _out; j++) {
                if (j == i) continue;

                ZDD fj = _zbddv.GetZBDD(j);
                if (fj.GetID() == bddempty) continue;

                bddvar fj_top_lev = (fj.Top() > 0)
                    ? BDD_LevOfVar(fj.Top()) : 0;

                if (fj_top_lev < p_top_lev) continue;

                ZDD qj = fj / p;
                if (qj.GetID() == bddempty) continue;

                ZDD new_fj = qj.Change(v) + (fj % p);

                _zbddv -= ZBDDV(fj, j);
                _zbddv += ZBDDV(new_fj, j);

                if (_zbddv.GetMetaZBDD().GetID() == bddnull) {
                    throw std::overflow_error("MLZBDDV: overflow in phase 2");
                }
            }
        }
    }
}

MLZBDDV::~MLZBDDV() {}

MLZBDDV& MLZBDDV::operator=(const MLZBDDV& v) {
    _pin = v._pin;
    _out = v._out;
    _sin = v._sin;
    _zbddv = v._zbddv;
    return *this;
}

// ============================================================
// Information accessors
// ============================================================

int MLZBDDV::N_pin() const { return _pin; }
int MLZBDDV::N_out() const { return _out; }
int MLZBDDV::N_sin() const { return _sin; }
ZBDDV MLZBDDV::GetZBDDV() const { return _zbddv; }

// ============================================================
// Print
// ============================================================

void MLZBDDV::Print() const {
    std::cout << "pin:" << _pin << std::endl;
    std::cout << "out:" << _out << std::endl;
    std::cout << "sin:" << _sin << std::endl;
    _zbddv.Print();
}
