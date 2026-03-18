(build)
cd <project_root>
mkdir -p build && cd build
cmake ..
cmake --build . --target bddqueen bddqueen_BDD bddqueen_ZDD

(usage)
bddqueen     <problem_size> [<bdd_node_limit>]
bddqueen_BDD <problem_size> [<bdd_node_limit>]
bddqueen_ZDD <problem_size> [<bdd_node_limit>]

(example)
bddqueen 8 100000 .. Generate BDDs for 8-queens up to 100000 nodes.
bddqueen 12 ........ Generate BDDs for 12-queens until memory limit.

(variants)
bddqueen     ... Original version using raw bddp operations (C style).
bddqueen_BDD ... BDD class version with operator overloading (C++).
bddqueen_ZDD ... ZDD class version, builds solution families row by row (C++).
