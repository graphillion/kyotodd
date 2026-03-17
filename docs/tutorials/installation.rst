Installation
============

C++ Library
-----------

KyotoDD uses CMake as its build system and requires a C++11 compiler.

.. code-block:: bash

   git clone https://github.com/graphillion/kyotodd.git
   cd kyotodd
   mkdir build && cd build
   cmake ..
   cmake --build .

This produces:

- ``libkyotodd_lib.a`` — the static library
- ``kyotodd_test`` — the test executable

To run the tests:

.. code-block:: bash

   ctest

To use KyotoDD in your project, add the ``include/`` directory to your
include path and link against the static library. Include the umbrella
header:

.. code-block:: cpp

   #include "bdd.h"

Python Package
--------------

The Python package uses `scikit-build-core <https://scikit-build-core.readthedocs.io/>`_
with pybind11. Install directly from GitHub:

.. code-block:: bash

   pip install git+https://github.com/graphillion/kyotodd.git#subdirectory=python

Or install from a local clone in editable (development) mode:

.. code-block:: bash

   git clone https://github.com/graphillion/kyotodd.git
   cd kyotodd/python
   pip install -e .

Requirements:

- Python >= 3.7
- CMake >= 3.14
- A C++11-capable compiler

Verify the installation:

.. code-block:: python

   import kyotodd
   x = kyotodd.newvar()
   print(f"Created variable {x}")
