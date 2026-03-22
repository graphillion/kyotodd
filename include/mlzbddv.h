#ifndef KYOTODD_MLZBDDV_H
#define KYOTODD_MLZBDDV_H

/**
 * @file mlzbddv.h
 * @brief MLZBDDV class — Multi-Level ZBDDV (algebraic factorization).
 *
 * MLZBDDV extracts common sub-expressions from a ZBDDV (multi-output
 * ZDD vector) using algebraic division and kernel extraction, producing
 * a multi-level representation that can reduce overall ZBDD size.
 *
 * This header is NOT included by the umbrella header @c bdd.h.
 * You must explicitly @c \#include @c "mlzbddv.h" to use this class.
 *
 * Requires BDDV_Init() to have been called (from bddv.h).
 *
 * No Python binding is provided for this class.
 */

#include "zbddv.h"

class MLZBDDV {
    int _pin;     // number of primary input variables
    int _out;     // number of output functions
    int _sin;     // number of extracted sub-expression inputs
    ZBDDV _zbddv; // multi-level ZBDDV (outputs + sub-expression defs)

public:
    /** @brief Default constructor. All counters zero, empty ZBDDV. */
    MLZBDDV();

    /**
     * @brief Construct from ZBDDV, auto-detecting pin and out.
     *
     * pin = BDD_LevOfVar(zbddv.Top()), out = zbddv.Last() + 1.
     * Delegates to MLZBDDV(zbddv, pin, out).
     */
    MLZBDDV(ZBDDV& zbddv);

    /**
     * @brief Construct from ZBDDV with explicit pin and out.
     *
     * This is the main constructor that performs the two-phase
     * algebraic factorization:
     * - Phase 1: inter-output division
     * - Phase 2: iterative kernel extraction via Divisor()
     */
    MLZBDDV(ZBDDV& zbddv, int pin, int out);

    /** @brief Destructor. */
    ~MLZBDDV();

    /** @brief Copy assignment. */
    MLZBDDV& operator=(const MLZBDDV& v);

    /** @brief Number of primary input variables. */
    int N_pin() const;

    /** @brief Number of output functions. */
    int N_out() const;

    /** @brief Number of extracted sub-expression inputs. */
    int N_sin() const;

    /** @brief Return the internal ZBDDV (outputs + sub-expression defs). */
    ZBDDV GetZBDDV() const;

    /** @brief Print summary (pin, out, sin) followed by ZBDDV contents. */
    void Print() const;
};

#endif
