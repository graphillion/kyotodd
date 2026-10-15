TdZdd Specs
===========

`TdZdd <https://github.com/kunisura/TdZdd>`_ describes a decision diagram
top-down with a *spec*: a state per node, a root level, and a transition
``getChild(state, level, value)`` that returns the child level, ``0`` for
the 0-terminal or ``-1`` for the 1-terminal. KyotoDD can use such specs
directly (C++ only):

* ``UnreducedDD::construct(spec)`` builds the spec into KyotoDD's node
  table; reduce the result with ``reduce_as_zdd()`` or ``reduce_as_bdd()``.
* ``ZDD::subset(spec)`` restricts an existing ZDD to the sets the spec
  accepts (TdZdd's ``zddSubset``), without building the spec's own ZDD.
  ``UnreducedDD::zdd_subset(spec)`` is the same operation on an unreduced
  input with an unreduced result.

Both need the TdZdd submodule (``third_party/TdZdd``; see
:doc:`installation`) and the header ``unreduced_dd_tdzdd.h``, which is not
part of ``bdd.h``:

.. code-block:: cpp

   #include "bdd.h"
   #include "unreduced_dd_tdzdd.h"

   using namespace kyotodd;

TdZdd levels run from the root level (the first decision) down to 1 (the
last decision). With KyotoDD's default variable order, TdZdd level ``i``
is variable ``i``; ``level_offset`` shifts the mapping if needed.

The complete program of this tutorial is ``app/TdZddSample``.

Writing a spec
--------------

The example problem: ``n`` items with weights ``w[1..n]``. The first spec
accepts every subset whose total weight is at most ``C``. Its state is the
weight chosen so far:

.. code-block:: cpp

   class WeightBound : public tdzdd::DdSpec<WeightBound, int, 2> {
       int n_;
       int capacity_;
       std::vector<int> w_;   // w_[i] = weight of item i
   public:
       WeightBound(const std::vector<int>& w, int capacity)
           : n_(static_cast<int>(w.size()) - 1), capacity_(capacity), w_(w) {}

       int getRoot(int& weight) const {
           weight = 0;
           return n_;                                 // top level
       }
       int getChild(int& weight, int level, int value) const {
           if (value) {
               weight += w_[level];
               if (weight > capacity_) return 0;      // rejected
               if (weight == capacity_) return -1;    // nothing more fits
           }
           return (level == 1) ? -1 : level - 1;
       }
   };

``hashCode`` / ``equalTo`` default to the raw bytes of the state, which
is right for a plain ``int``. Note the ``weight == capacity_`` shortcut: it
jumps from a high level straight to the 1-terminal. In TdZdd such a
level-skipping edge means "all skipped items are 0".

Building with construct()
-------------------------

.. code-block:: cpp

   BDD_Init(1 << 16, UINT64_MAX);
   std::vector<int> w = {0, 3, 5, 2, 4, 1, 3};   // w[0] unused; n = 6
   int capacity = 7;

   WeightBound bound(w, capacity);
   UnreducedDD u = UnreducedDD::construct(bound);   // creates variables 1..6
   ZDD light = u.reduce_as_zdd();
   std::cout << light.exact_count() << " subsets\n";   // 23

``construct()`` runs the same breadth-first, level-by-level algorithm as
TdZdd's ``DdBuilder`` (including ``mergeStates`` and the handling of
states that reach the 1-terminal), but writes nodes straight into
KyotoDD's node array. The result is unreduced; ``reduce_as_zdd()`` gives
the canonical ZDD. It equals what the classic operations produce:

.. code-block:: cpp

   ZDD check = ZDD::power_set(6).cost_bound_le(w, capacity);
   assert(light.id() == check.id());

Level-skipping edges and SkipPadding
------------------------------------

A ZDD reads a skipped level as "variable = 0", a BDD reads it as
"don't care". ``reduce_as_zdd()`` therefore gives the intended family,
but ``reduce_as_bdd()`` on the same unreduced DD would accept extra
assignments. ``SkipPadding::Zdd`` inserts the missing nodes with
``hi = 0`` so that both readings agree:

.. code-block:: cpp

   UnreducedDD padded = UnreducedDD::construct(bound, SkipPadding::Zdd);
   BDD expected = light.to_bdd(6);               // characteristic function
   assert(u.reduce_as_bdd().id()      != expected.id());   // don't-care reading
   assert(padded.reduce_as_bdd().id() == expected.id());

``SkipPadding::Bdd`` does the opposite (``lo == hi``, don't-care) for
specs written with BDD semantics in mind. Edges to the 0-terminal are
never padded.

Subsetting with ZDD::subset()
-----------------------------

The second spec accepts subsets with no two consecutive items. Its state
is whether the previous item was chosen:

.. code-block:: cpp

   class NoAdjacent : public tdzdd::DdSpec<NoAdjacent, int, 2> {
       int n_;
   public:
       explicit NoAdjacent(int n) : n_(n) {}
       int getRoot(int& prev) const { prev = 0; return n_; }
       int getChild(int& prev, int level, int value) const {
           if (value && prev) return 0;
           prev = value;
           return (level == 1) ? -1 : level - 1;
       }
   };

Applying it to the family built above keeps only the sets the spec
accepts:

.. code-block:: cpp

   ZDD sparse = light.subset(NoAdjacent(6));
   std::cout << sparse.exact_count() << " subsets\n";   // 16

   // Same result, but this builds the spec's whole ZDD first.
   ZDD via_intersection = light & UnreducedDD::construct(NoAdjacent(6)).reduce_as_zdd();
   assert(sparse.id() == via_intersection.id());

``subset()`` walks the input ZDD and the spec together, one level at a
time, pairing each input node with a spec state; only branches on which
both reach the 1-terminal survive. This is TdZdd's ``zddSubset``. When
the two sides sit at different levels the 0-edge of the higher one is
followed, so input variables outside the spec's level range are forced
to 0: sets containing them are dropped.

``UnreducedDD::zdd_subset()`` is the same operation on an unreduced input
(for example the output of ``construct()`` before reduction) and returns
an unreduced DD:

.. code-block:: cpp

   UnreducedDD sparse_u = u.zdd_subset(NoAdjacent(6));
   assert(sparse_u.reduce_as_zdd().id() == sparse.id());

Other spec kinds
----------------

Any binary ``tdzdd::DdSpecBase`` subclass works: ``DdSpec`` (one state
object), ``StatelessDdSpec``, ``PodArrayDdSpec`` (array state) and
``HybridDdSpec``. ``mergeStates`` is honored as in TdZdd. See
:doc:`../api/cpp/ops_unreduced_dd` for the full reference.
