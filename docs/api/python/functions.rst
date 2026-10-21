Module Functions
================

Initialization
--------------

.. py:function:: kyotodd.init(node_count=256, node_max=2**64-1)

   Initialize the BDD library.

   Must be called before creating any BDD or ZDD nodes. If not called
   explicitly, the library is automatically initialized on first use.

   :param int node_count: Initial number of node slots to allocate.
   :param int node_max: Maximum number of node slots allowed.

.. py:function:: kyotodd.finalize()

   Finalize the BDD library and release all resources.

   Raises :py:exc:`RuntimeError` if called while BDD or ZDD objects exist.
   Safe to call multiple times when no objects are alive.

Variable Management
-------------------

.. py:function:: kyotodd.new_var()

   Create a new variable and return its variable number.

   Variables are numbered starting from 1. Each call creates a new
   variable with the next available number.

   :return: The variable number of the newly created variable.
   :rtype: int

.. py:function:: kyotodd.new_vars(n, reverse=False)

   Create multiple new variables at once.

   :param int n: Number of variables to create.
   :param bool reverse: If True, insert each at level 1 (reverses var/level ordering).
   :return: A list of the newly created variable numbers.
   :rtype: list[int]

.. py:function:: kyotodd.new_var_of_level(lev)

   Create a new variable at the specified level.

   Inserts a new variable at level *lev*, shifting existing variables
   at that level and above.

   :param int lev: The level to insert the new variable at.
   :return: The variable number of the newly created variable.
   :rtype: int

.. py:function:: kyotodd.to_level(var)

   Return the level of the given variable.

   :param int var: Variable number.
   :return: The level of the variable.
   :rtype: int

.. py:function:: kyotodd.to_var(level)

   Return the variable at the given level.

   :param int level: Level number.
   :return: The variable number at that level.
   :rtype: int

.. py:function:: kyotodd.var_count()

   Return the number of variables created so far.

   :rtype: int

.. py:function:: kyotodd.top_level()

   Return the maximum level number (equal to the number of variables).

   :rtype: int

Node Statistics
---------------

.. py:function:: kyotodd.node_count()

   Return the number of used node slots (including dead nodes awaiting GC).

   This is **not** the initial capacity or the array size — it is the
   count of node slots that have been occupied at least once.

   :rtype: int

.. py:function:: kyotodd.live_nodes()

   Return the number of live (non-garbage) nodes.

   :rtype: int

Garbage Collection
------------------

.. py:function:: kyotodd.gc()

   Manually invoke garbage collection.

   Reclaims dead nodes that are no longer referenced.

.. py:function:: kyotodd.gc_set_threshold(threshold)

   Set the GC threshold.

   When the ratio of live nodes to maximum capacity exceeds this threshold,
   garbage collection is triggered automatically.

   :param float threshold: A value between 0.0 and 1.0 (default: 0.9).

.. py:function:: kyotodd.gc_get_threshold()

   Get the current GC threshold.

   :return: The current threshold value.
   :rtype: float

.. py:function:: kyotodd.gc_rootcount()

   Return the number of registered GC root pointers.

   :rtype: int

Import Limits
-------------

.. py:function:: kyotodd.set_import_limit(max_nodes=None, max_levels=None, max_outputs=None)

   Set the safety limits applied by every import function
   (``import_str`` / ``import_file``, ``import_binary_*``,
   ``import_sapporo_*``, ``import_graphillion_*`` and the deprecated
   ``import_knuth_*``).

   An import rejects a file whose header declares more nodes, levels or
   roots than these limits before allocating anything, so that a corrupted
   or malicious file cannot trigger a huge allocation. Pass 0 to disable a
   limit; omit an argument to keep its current value. Raise or disable the
   limits only for trusted files, for example to load a DD with more than
   ten million nodes that was written by this library::

      kyotodd.set_import_limit(max_nodes=0)   # no node bound
      big = ZDD.import_binary_file(f)

   :param int max_nodes: Maximum total node count (default 10,000,000; 0 = unlimited).
   :param int max_levels: Maximum level count (default 1,000,000; 0 = unlimited).
   :param int max_outputs: Maximum root count for multi-root formats
      (default 1,000,000; 0 = unlimited).

.. py:function:: kyotodd.get_import_limit()

   Get the safety limits currently applied by the import functions.

   :return: A dict with keys ``'max_nodes'``, ``'max_levels'`` and
      ``'max_outputs'``. A value of 0 means that limit is disabled.
   :rtype: dict[str, int]

Random Generation
-----------------

.. py:function:: kyotodd.zdd_random(lev, density=50)

   Generate a random ZDD over the lowest *lev* levels.

   Recursively builds a random family of sets. Each terminal is
   independently set to 1 with probability *density* / 100.

   .. note::
      Variables for levels 1 through *lev* must have been created
      (via :py:func:`new_var`) before calling this function.

   :param int lev: Number of variable levels to use.
   :param int density: Probability (0--100) for each terminal to be 1 (default: 50).
   :return: A random ZDD.
   :rtype: ZDD
