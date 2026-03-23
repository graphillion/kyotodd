/****************************************
*  Generating BDDs for N-Queens problem *
*  (BDD class version)                  *
* (c) Shin-ichi Minato (2011/10/15)     *
****************************************/

#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <vector>
#include "bdd.h"

int main(int argc, char *argv[])
{
  int q, n, i, j, x, y;
  char s[256];
  bddp bddlimit;

  /**** Read parameters ****/
  if(argc < 2 || argc > 3)
  {
    printf("bddqueen_BDD <problem_size> [<bdd_node_limit>]\n");
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

  /**** Generate BDD variables for respective queens ****/
  std::vector<std::vector<BDD> > X(q, std::vector<BDD>(q));
  for(i=0; i<q; i++)
    for(j=0; j<q; j++)
      X[i][j] = BDDvar(bddnewvar());

  /**** Repeat BDD generation for 3 times ****/
  for(n=0; n<3; n++)
  {
    printf("\n--------[%d]--------\n", n+1);
    BDD F = BDD::True;

    /**** Generating BDDs for row constraints ****/
    for(i=0; i<q; i++)
    {
      BDD F0 = BDD::True;
      BDD F1 = BDD::False;
      for(j=0; j<q; j++)
      {
        BDD X0 = ~X[i][j];
        BDD G0 = F0 & X0;
        BDD G01 = F0 & X[i][j];
        BDD G11 = F1 & X0;
        F0 = G0;
        F1 = G01 | G11;
      }
      F &= F1;
      printf(".");
      fflush(stdout);
    }
    for(i=0; i<q-3; i++) putchar(' ');
    sprintf(s, "%" PRIu64, (uint64_t)F.raw_size());
    printf("size:%s ", s);
    sprintf(s, "%" PRIu64, (uint64_t)bddused());
    printf("used:%s\n", s);

    /**** Generating BDDs for column constraints ****/
    for(i=0; i<q; i++)
    {
      BDD F0 = BDD::True;
      BDD F1 = BDD::False;
      for(j=0; j<q; j++)
      {
        BDD X0 = ~X[j][i];
        BDD G0 = F0 & X0;
        BDD G01 = F0 & X[j][i];
        BDD G11 = F1 & X0;
        F0 = G0;
        F1 = G01 | G11;
      }
      F &= F1;
      printf(".");
      fflush(stdout);
    }
    for(i=0; i<q-3; i++) putchar(' ');
    sprintf(s, "%" PRIu64, (uint64_t)F.raw_size());
    printf("size:%s ", s);
    sprintf(s, "%" PRIu64, (uint64_t)bddused());
    printf("used:%s\n", s);

    /**** Generating BDDs for upward diagonal constraints ****/
    for(i=0; i<q*2-3; i++)
    {
      BDD F0 = BDD::True;
      BDD F1 = BDD::False;
      for(j=0; j<q; j++)
      {
        x = i - (q -2) + j;
        y = q - 1 - j;
        if(x < 0 || x >= q) continue;
        if(y < 0 || y >= q) continue;
        BDD X0 = ~X[x][y];
        BDD G0 = F0 & X0;
        BDD G01 = F0 & X[x][y];
        BDD G11 = F1 & X0;
        F0 = G0;
        F1 = G01 | G11;
      }
      BDD G = F0 | F1;
      F &= G;
      printf(".");
      fflush(stdout);
    }
    sprintf(s, "%" PRIu64, (uint64_t)F.raw_size());
    printf("size:%s ", s);
    sprintf(s, "%" PRIu64, (uint64_t)bddused());
    printf("used:%s\n", s);

    /**** Generating BDDs for downward diagonal constraints ****/
    for(i=0; i<q*2-3; i++)
    {
      BDD F0 = BDD::True;
      BDD F1 = BDD::False;
      for(j=0; j<q; j++)
      {
        x = i - (q -2) + j;
        y = j;
        if(x < 0 || x >= q) continue;
        if(y < 0 || y >= q) continue;
        BDD X0 = ~X[x][y];
        BDD G0 = F0 & X0;
        BDD G01 = F0 & X[x][y];
        BDD G11 = F1 & X0;
        F0 = G0;
        F1 = G01 | G11;
      }
      BDD G = F0 | F1;
      F &= G;
      printf(".");
      fflush(stdout);
    }
    sprintf(s, "%" PRIu64, (uint64_t)F.raw_size());
    printf("size:%s ", s);
    sprintf(s, "%" PRIu64, (uint64_t)bddused());
    printf("used:%s\n", s);

    /**** Check memory overflow ****/
    if(F == BDD::Null) printf("Memory overflow.\n");
  }

  return 0;
}
