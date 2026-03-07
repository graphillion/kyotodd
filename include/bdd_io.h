#ifndef KYOTODD_BDD_IO_H
#define KYOTODD_BDD_IO_H

#include <cstdio>
#include <iosfwd>
#include <vector>
#include "bdd_types.h"

void bddexport(FILE* strm, const bddp* p, int lim);
void bddexport(FILE* strm, const std::vector<bddp>& v);
void bddexport(std::ostream& strm, const bddp* p, int lim);
void bddexport(std::ostream& strm, const std::vector<bddp>& v);

int bddimport(FILE* strm, bddp* p, int lim);
int bddimport(FILE* strm, std::vector<bddp>& v);
int bddimport(std::istream& strm, bddp* p, int lim);
int bddimport(std::istream& strm, std::vector<bddp>& v);
int bddimportz(FILE* strm, bddp* p, int lim);
int bddimportz(FILE* strm, std::vector<bddp>& v);
int bddimportz(std::istream& strm, bddp* p, int lim);
int bddimportz(std::istream& strm, std::vector<bddp>& v);

#endif
