BDD Basics
==========

This tutorial covers the fundamental BDD operations available in KyotoDD.

Building BDDs
-------------

Create variables and combine them with logical operators:

.. code-block:: python

   import kyotodd

   # Create three variables
   x1 = kyotodd.new_var()
   x2 = kyotodd.new_var()
   x3 = kyotodd.new_var()

   a = kyotodd.BDD.var(x1)
   b = kyotodd.BDD.var(x2)
   c = kyotodd.BDD.var(x3)

   # Logical operations
   f = a & b          # AND
   g = a | b          # OR
   h = a ^ b          # XOR
   n = ~a             # NOT
   ite = kyotodd.BDD.ite(a, b, c)  # if a then b else c

Constants
~~~~~~~~~

.. code-block:: python

   false = kyotodd.BDD.false_  # constant false
   true  = kyotodd.BDD.true_   # constant true

   # Verify
   assert (a & kyotodd.BDD.false_) == kyotodd.BDD.false_
   assert (a | kyotodd.BDD.true_)  == kyotodd.BDD.true_

Cofactoring
-----------

Cofactoring restricts a variable to a specific value:

.. code-block:: python

   f = a & b | c

   # Restrict x1 to 0
   f0 = f.at0(x1)  # equivalent to f with x1=0

   # Restrict x1 to 1
   f1 = f.at1(x1)  # equivalent to f with x1=1

   # Generalized cofactor
   g = f.cofactor(a)  # constrain f by a

Quantification
--------------

Eliminate variables by existential or universal quantification:

.. code-block:: python

   f = a & b | c

   # Existential: f|x1=0 OR f|x1=1
   e1 = f.exist(x1)            # single variable
   e2 = f.exist([x1, x2])      # list of variables
   e3 = f.exist(a & b)         # cube BDD (conjunction)

   # Universal: f|x1=0 AND f|x1=1
   u1 = f.univ(x1)
   u2 = f.univ([x1, x2])

Support Set
-----------

The support set is the set of variables a BDD depends on:

.. code-block:: python

   f = a & b  # depends on x1 and x2

   # As a list of variable numbers
   vars = f.support_vec()
   print(vars)  # [2, 1] (descending level order)

   # As a BDD (conjunction of variables)
   sup = f.support()

Implication
-----------

Check whether one BDD implies another:

.. code-block:: python

   f = a & b  # x1 AND x2
   g = a | b  # x1 OR x2

   print(f.imply(g))  # 1 (f => g is true)
   print(g.imply(f))  # 0 (g => f is false)

Variable Shifting
-----------------

Rename all variables by a constant offset:

.. code-block:: python

   f = a & b  # uses x1, x2

   # Shift variables up by 2: x1->x3, x2->x4
   g = f << 2

   # Shift back down
   h = g >> 2
   assert h == f

Import / Export
---------------

Save and load BDDs to/from strings or files:

.. code-block:: python

   f = a & b | c

   # String I/O
   s = f.export_str()
   f2 = kyotodd.BDD.import_str(s)
   assert f2 == f

   # File I/O
   f.export_file("my_bdd.txt")
   f3 = kyotodd.BDD.import_file("my_bdd.txt")
   assert f3 == f

Node Size
---------

Query the DAG size:

.. code-block:: python

   f = a & b & c
   print(f"Nodes: {f.raw_size}")
   print(f"Top variable: {f.top_var}")
