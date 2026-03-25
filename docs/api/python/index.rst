Python API
==========

.. toctree::
   :maxdepth: 2

   functions
   bdd
   zdd
   qdd
   unreduced_dd
   pidd
   rotpidd
   seqbdd
   mvdd
   mtbdd
   memo

C++ only classes
----------------

The following classes are available only in C++ and do not have Python bindings:

- **BDDV** (``bddv.h``) — Array of BDDs encoded as a single meta-BDD (obsolete)
- **ZBDDV** (``zbddv.h``) — Sparse array of ZBDDs encoded as a single meta-ZBDD (obsolete)
- **MLZBDDV** (``mlzbddv.h``) — Multi-Level ZBDDV algebraic factorization
- **BtoI** (``btoi.h``) — Binary-to-Integer mapping using BDD vectors
- **CtoI** (``ctoi.h``) — Combination-to-Integer mapping using ZBDDs
- **SOP** (``sop.h``) — Sum-of-Products representation on ZBDDs
- **SOPV** (``sop.h``) — Vector of SOPs stored in a ZBDDV
