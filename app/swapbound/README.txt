swapbound - k-swap reachable permutations via PiDD
===================================================

Given a permutation size n and a non-negative integer k, this sample
builds the set of permutations in S_n reachable from the identity by
at most k transpositions (Cayley distance <= k), using PiDD.

Approach
--------
PiDD represents a set of permutations via a ZDD whose variables
correspond to transpositions (x, y) with y < x. The canonical
encoding of a permutation pi uses exactly n - c(pi) transpositions,
where c(pi) is the number of cycles of pi. Hence

  SwapBound(k) on the full S_n == { pi : n - c(pi) <= k }
                               == { pi : Cayley distance(pi) <= k }.

The program:

  1. Builds step = identity + every transposition (i, j), 1 <= j < i <= n.
  2. Iterates R_0 = {identity}, R_i = (R_{i-1} * step).swap_bound(i).
     After i iterations, R_i is the set of permutations expressible
     as a product of <= i transpositions.
  3. Prints |R_i| (permutation count) and the internal ZDD node count
     for i = 0..k, then enumerates R_k when it is small and/or saves
     the internal ZDD as SVG.

For i >= n - 1 the set saturates to the full symmetric group S_n
(n! permutations).

Usage
-----
    swapbound <n> <k> [svg_out]

    n         permutation size (1 <= n <= 254; in practice limited by
              n! and ZDD node count, say n <= 10)
    k         non-negative bound on the number of transpositions
    svg_out   optional output path for the internal ZDD as SVG
              (transposition labels are drawn as "(x,y)")

Examples (run from the build directory)
---------------------------------------
    ./swapbound 3 1                      # identity + 3 transpositions (card=4)
    ./swapbound 3 2                      # full S_3 (card=6)
    ./swapbound 4 2                      # card=18 (= 1 + 6 + 11)
    ./swapbound 4 3 /tmp/s4.svg          # full S_4, SVG saved
    ./swapbound 5 10                     # saturates at k=4 to card=120
    ./swapbound 7 3                      # |R_3|=232, a non-trivial PiDD size

Expected output (small cases)
-----------------------------
    n=3, k=2:  card progression 1, 4, 6           (size 0, 3, 3)
    n=4, k=3:  card progression 1, 7, 18, 24      (size 0, 6, 8, 6)
    n=5, k=4:  card progression 1, 11, 46, 96, 120

Counts at distance exactly d coincide with the unsigned Stirling
numbers of the first kind c(n, n - d); the cumulative count at
distance <= k is therefore sum_{d=0..k} c(n, n - d).

Notes
-----
* PiDD::card() returns uint64_t. Values saturate at UINT64_MAX; for
  n <= 20 the count n! fits in uint64. Practical usage targets much
  smaller n.
* Enumeration output uses PiDD::enumerate()'s compact "one-line" form:
  a permutation is shown as [a_1 a_2 ... a_n] where a_i is the image
  of position i, with '.' indicating a_i == i. The identity is "1".
* The optional SVG visualises the internal ZDD. Labels "(x,y)" come
  from PiDD::svg_var_name_map().
