ZDD Basics
==========

This tutorial covers ZDD (Zero-suppressed Decision Diagram) operations.
ZDDs represent *families of sets* — collections of subsets of a universe.

Building ZDDs
-------------

A ZDD family is built by combining singleton sets:

.. code-block:: python

   import kyotodd

   x1 = kyotodd.new_var()
   x2 = kyotodd.new_var()
   x3 = kyotodd.new_var()

   # Start with the unit family {∅} (contains only the empty set)
   base = kyotodd.ZDD.single

   # Create a family containing a single set {x1}
   s1 = base.change(x1)  # toggles x1 into the empty set

   # Create {x2}
   s2 = base.change(x2)

   # Create {x1, x2} (a set with two elements)
   s12 = s1.change(x2)

Constants
~~~~~~~~~

.. code-block:: python

   empty  = kyotodd.ZDD.empty   # ∅ (no sets)
   single = kyotodd.ZDD.single  # {∅} (contains only the empty set)

   assert empty.exact_count == 0
   assert single.exact_count == 1

Set Operations
--------------

Combine families with set-theoretic operations:

.. code-block:: python

   # F = {{x1}, {x2}}
   F = s1 + s2

   # G = {{x2}, {x1,x2}}
   G = s2 + s12

   # Union: {{x1}, {x2}, {x1,x2}}
   union = F + G
   print(f"Union: {union.exact_count} sets")  # 3

   # Intersection: {{x2}}
   inter = F & G
   print(f"Intersection: {inter.exact_count} sets")  # 1

   # Difference: {{x1}}
   diff = F - G
   print(f"Difference: {diff.exact_count} sets")  # 1

   # Symmetric difference
   symdiff = F ^ G

Selection Operations
--------------------

Select subsets of a family based on variable membership:

.. code-block:: python

   # F = {{x1}, {x2}, {x1,x2}}
   F = s1 + s2 + s12

   # Offset: sets NOT containing x1 → {{x2}}
   off = F.offset(x1)
   assert off.exact_count == 1

   # Onset: sets containing x1, keeping x1 → {{x1}, {x1,x2}}
   on = F.onset(x1)
   assert on.exact_count == 2

   # Change: toggle x1 in all sets
   ch = F.change(x1)

Algebraic Operations
--------------------

.. code-block:: python

   # Join (cross product with union): for each (A,B), include A∪B
   joined = F * G

   # Division: quotient Q such that Q * G ⊆ F (Q and G are disjoint)
   quotient = F / G

   # Remainder: F - (F / G) * G
   remainder = F % G

Cross-Product Operations
------------------------

.. code-block:: python

   # Disjoint product: A∪B only when A∩B = ∅
   disj = F.disjoin(G)

   # Joint join: A∪B for overlapping pairs (A∩B ≠ ∅)
   jj = F.jointjoin(G)

   # Delta: symmetric difference A△B for each pair
   d = F.delta(G)

Filtering Operations
--------------------

Filter families based on subset/superset relationships:

.. code-block:: python

   # Restrict: keep sets that are subsets of some set in G
   r = F.restrict(G)

   # Permit: keep sets whose elements are all in some set in G
   p = F.permit(G)

   # Nonsup: remove sets that are supersets of some set in G
   ns = F.nonsup(G)

   # Nonsub: remove sets that are subsets of some set in G
   nb = F.nonsub(G)

Unary Operations
----------------

.. code-block:: python

   # F = {{x1}, {x2}, {x1,x2}}
   F = s1 + s2 + s12

   # Maximal sets: no proper superset in the family
   mx = F.maximal()
   assert mx.exact_count == 1  # {{x1,x2}}

   # Minimal sets: no proper subset in the family
   mn = F.minimal()
   assert mn.exact_count == 2  # {{x1}, {x2}}

   # Downward closure: all subsets of sets in the family
   cl = F.closure()
   assert cl.exact_count == 4  # {{}, {x1}, {x2}, {x1,x2}}

   # Minimum hitting sets
   mh = F.minhit()

Counting
--------

.. code-block:: python

   F = s1 + s2 + s12
   print(f"Family has {F.exact_count} sets")  # 3

Import / Export
---------------

.. code-block:: python

   F = s1 + s2 + s12

   # String I/O
   s = F.export_str()
   F2 = kyotodd.ZDD.import_str(s)
   assert F2 == F

   # File I/O
   F.export_file("my_zdd.txt")
   F3 = kyotodd.ZDD.import_file("my_zdd.txt")
   assert F3 == F
