Getting Started
===============

This tutorial introduces the basic workflow for both the C++ and Python APIs.

C++ Quick Start
---------------

Include the umbrella header and initialize the library:

.. code-block:: cpp

   #include "bdd.h"
   #include <iostream>

   int main() {
       // Initialize with 256 initial nodes, unlimited max
       bddinit(256, UINT64_MAX);

       // Create two variables
       bddvar x1 = bddnewvar();  // variable 1
       bddvar x2 = bddnewvar();  // variable 2

       // Build BDDs for variables
       BDD a = BDDvar(x1);
       BDD b = BDDvar(x2);

       // Compute f = x1 AND x2
       BDD f = a & b;

       // Compute g = x1 OR x2
       BDD g = a | b;

       // Check: f implies g?
       std::cout << "f => g: " << f.Imply(g) << std::endl;  // 1

       // Node count
       std::cout << "f has " << f.Size() << " nodes" << std::endl;

       return 0;
   }

Python Quick Start
------------------

The Python API mirrors the C++ API with Pythonic naming:

.. code-block:: python

   import kyotodd

   # Create variables (library auto-initializes on first use)
   x1 = kyotodd.new_var()
   x2 = kyotodd.new_var()

   # Build BDDs for variables
   a = kyotodd.BDD.var(x1)
   b = kyotodd.BDD.var(x2)

   # Compute f = x1 AND x2
   f = a & b

   # Compute g = x1 OR x2
   g = a | b

   # Check: f implies g?
   print(f"f => g: {f.imply(g)}")  # 1

   # Node count
   print(f"f has {f.raw_size} nodes")

Key Differences
~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - C++
     - Python
     - Description
   * - ``BDDvar(v)``
     - ``BDD.var(v)``
     - Create a variable BDD
   * - ``f.Size()``
     - ``f.raw_size``
     - Node count (property in Python)
   * - ``f.At0(v)``
     - ``f.at0(v)``
     - Cofactor (snake_case in Python)
   * - ``f.Exist(cube)``
     - ``f.exist(cube)``
     - Quantification
   * - ``BDD::Ite(f,g,h)``
     - ``BDD.ite(f,g,h)``
     - If-then-else (static method)
