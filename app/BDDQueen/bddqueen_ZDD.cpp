/****************************************
*  Generating ZDDs for N-Queens problem *
*  (ZDD class version)                  *
* Based on SAPPOROBDD example           *
****************************************/

#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <vector>
#include "bdd.h"

int main(int argc, char *argv[])
{
  int q, n, i, j, i2;
  char s[256];
  bddp bddlimit;

  /**** Read parameters ****/
  if(argc < 2 || argc > 3)
  {
    printf("bddqueen_ZDD <problem_size> [<bdd_node_limit>]\n");
    return 1;
  }

  q = atoi(argv[1]);
  if(argc == 3) bddlimit = strtoull(argv[2], NULL, 10);
  else bddlimit = bddnull;

  printf("problem_size:  %d\n", q);
  sprintf(s, "%" PRIu64, (uint64_t)(bddlimit));
  printf("bdd_node_limit:%s\n", s);

  /**** Initialize BDD package ****/
  if(bddinit(64, bddlimit))
  {
    printf("memory allocation failed.\n");
    return 1;
  }

  /**** Create variables for each cell ****/
  /* X[i][j] = variable number for cell (row i, column j) */
  std::vector<std::vector<bddvar> > X(q, std::vector<bddvar>(q));
  for(i=0; i<q; i++)
    for(j=0; j<q; j++)
      X[i][j] = bddnewvar();

  /**** Repeat ZDD generation for 3 times ****/
  for(n=0; n<3; n++)
  {
    printf("\n--------[%d]--------\n", n+1);

    /**** Build solution family row by row ****/
    ZDD F = ZDD::Single; /* {{}} - family containing empty set */

    for(i=0; i<q; i++)
    {
      ZDD G; /* Empty family */
      for(j=0; j<q; j++)
      {
        ZDD H = F;

        /* Filter out partial solutions that conflict with (i, j) */
        for(i2=0; i2<i; i2++)
        {
          int dj;

          /* Column constraint: remove solutions with queen at (i2, j) */
          H = H.Offset(X[i2][j]);

          /* Upward diagonal: row i2, column j-(i-i2) */
          dj = j - (i - i2);
          if(dj >= 0 && dj < q)
            H = H.Offset(X[i2][dj]);

          /* Downward diagonal: row i2, column j+(i-i2) */
          dj = j + (i - i2);
          if(dj >= 0 && dj < q)
            H = H.Offset(X[i2][dj]);
        }

        /* Add queen at (i, j) to each surviving partial solution */
        H = H.Change(X[i][j]);

        /* Union with solutions from other column choices */
        G += H;
      }
      F = G;
      printf(".");
      fflush(stdout);
    }

    sprintf(s, "%" PRIu64, (uint64_t)F.raw_size());
    printf(" size:%s", s);
    sprintf(s, "%" PRIu64, (uint64_t)bddused());
    printf(" used:%s", s);
    sprintf(s, "%.0f", F.count());
    printf(" solutions:%s\n", s);

    /**** Check memory overflow ****/
    if(F == ZDD::Null) printf("Memory overflow.\n");
  }

  return 0;
}
