MLZBDDV Operations
==================

MLZBDDV (Multi-Level ZBDDV) extracts common sub-expressions from a
ZBDDV (multi-output ZDD vector) using algebraic division and kernel
extraction, producing a multi-level representation that can reduce
overall ZBDD size.

The factorization proceeds in two phases:

1. **Inter-output division** — Each output function is divided by every
   other output. When output *i* is a factor of output *j*, the
   occurrence is replaced by a sub-expression variable.

2. **Kernel extraction** — Each output function is repeatedly factored
   via ``ZBDD::Divisor()``. Extracted kernels become new sub-expression
   definitions and are applied across all outputs.

Header: ``#include "mlzbddv.h"``

Requires ``BDDV_Init()`` to have been called.

.. note:: No Python binding is provided for this class.

MLZBDDV Class
--------------

.. cpp:class:: MLZBDDV

   Multi-level representation of a ZBDDV after algebraic factorization.

   **Constructors**

   .. cpp:function:: MLZBDDV()

      Default constructor. All counters zero, empty ZBDDV.

   .. cpp:function:: MLZBDDV(ZBDDV& zbddv)

      Auto-detect constructor. Infers *pin* from
      ``BDD_LevOfVar(zbddv.Top())`` and *out* from
      ``zbddv.Last() + 1``, then delegates to the three-argument
      constructor.

   .. cpp:function:: MLZBDDV(ZBDDV& zbddv, int pin, int out)

      Construct with explicit parameters and perform two-phase
      factorization.

      :param zbddv: The input ZBDDV. If null (``ZDD(-1)``), all
         counters are set to 0 and the null ZBDDV is stored.
      :param pin: Level of the top input variable (includes system
         variable levels).
      :param out: Number of output functions.

   **Assignment**

   .. cpp:function:: MLZBDDV& operator=(const MLZBDDV& v)

      Copy assignment.

   **Accessors**

   .. cpp:function:: int N_pin() const

      Number of primary input variable levels.

   .. cpp:function:: int N_out() const

      Number of output functions.

   .. cpp:function:: int N_sin() const

      Number of extracted sub-expression inputs (from both phases).

   .. cpp:function:: ZBDDV GetZBDDV() const

      Return the internal ZBDDV. Indices 0 to ``N_out()-1`` hold the
      (possibly rewritten) output functions. Indices ``N_out()`` and
      above hold sub-expression definitions from phase 2.

   **Output**

   .. cpp:function:: void Print() const

      Print summary (``pin:``, ``out:``, ``sin:``) followed by
      ``ZBDDV::Print()`` output.
