Concepts
========

This page explains the key concepts behind KyotoDD's implementation.

Node ID Encoding
----------------

Each node in KyotoDD is identified by a 48-bit node ID (stored as ``uint64_t``).
Two special bits control interpretation:

- **Bit 47 (constant flag)**: When set, the remaining bits encode a terminal
  value. The two standard terminals are:

  - ``bddfalse`` / ``bddempty`` = ``0x800000000000`` (value 0)
  - ``bddtrue`` / ``bddsingle`` = ``0x800000000001`` (value 1)

- **Bit 0 (complement flag)**: When set on a non-terminal node ID, the edge
  is a complement (negated) edge. The actual node is at the even address
  (ID with bit 0 cleared).

Regular (non-terminal, non-complemented) node IDs are always even:
2, 4, 6, etc. The node array index is computed as ``node_id / 2 - 1``.

Complement Edges
----------------

Complement edges allow representing negated functions without allocating
extra nodes. Both BDD and ZDD use complement edges, but they differ in
semantics.

BDD Complement Edges
~~~~~~~~~~~~~~~~~~~~

A complement edge on a BDD negates the entire Boolean function:

- If traversing node ``(var, lo, hi)`` through a complement edge,
  apply NOT to **both** ``lo`` and ``hi``.
- Normalization rule: the ``lo`` child stored in a node is always
  non-complemented. If the caller supplies a complemented ``lo``,
  both children are flipped and the complement is moved to the
  returned edge.

ZDD Complement Edges
~~~~~~~~~~~~~~~~~~~~

A complement edge on a ZDD toggles membership of the empty set (∅):

- If traversing node ``(var, lo, hi)`` through a complement edge,
  apply NOT to **lo only**; ``hi`` stays unchanged.
- The empty set is reachable only through the chain of ``lo`` (0-edge)
  children, so complementing only ``lo`` toggles whether ∅ is in the
  family.

Variables and Levels
--------------------

KyotoDD distinguishes between **variable numbers** and **levels**:

- **Variable number**: An identifier for a Boolean variable, starting from 1.
- **Level**: Determines the position in the diagram. Terminal nodes are at
  level 0; non-terminal levels start from 1 upward.

By default, variable *i* maps to level *i* (identity ordering). This mapping
can be modified using :cpp:func:`bddnewvaroflev` to insert variables at
specific levels, enabling variable reordering.

Use :cpp:func:`bddlevofvar` and :cpp:func:`bddvaroflev` to convert between
variable numbers and levels.

Garbage Collection
------------------

KyotoDD uses **mark-and-sweep** garbage collection:

1. **Mark phase**: Starting from registered GC roots, all reachable nodes
   are marked as live.
2. **Sweep phase**: Unmarked nodes are added to a free list for reuse.
   Unique tables are rebuilt from live nodes, and the operation cache is cleared.

GC roots are registered automatically by BDD and ZDD class constructors.
Raw ``bddp`` values can be protected with :cpp:func:`bddgc_protect` and
:cpp:func:`bddgc_unprotect`.

GC is triggered automatically when the node array is full and the live node
ratio exceeds the threshold (default: 0.9). Use :cpp:func:`bddgc` for manual
invocation, or :cpp:func:`bddgc_setthreshold` to adjust the automatic trigger.

BDD vs ZDD
-----------

Both BDD and ZDD are DAG-based data structures for representing Boolean
functions, but they use different reduction rules:

- **BDD**: A node is eliminated when its ``lo`` and ``hi`` children are
  identical (the variable has no effect). Used for general Boolean functions.
- **ZDD**: A node is eliminated when its ``hi`` child is the 0-terminal
  (the variable never appears in any set). Used for representing sparse
  families of sets.

Choose BDD when working with Boolean functions (logic design, model checking).
Choose ZDD when working with combinatorial set families (hypergraphs, itemsets,
network paths).
