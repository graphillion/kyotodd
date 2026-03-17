#ifndef KYOTODD_BDD_IO_H
#define KYOTODD_BDD_IO_H

#include <cstdio>
#include <iosfwd>
#include <vector>
#include "bdd_types.h"

/**
 * @brief Export BDD/ZDD nodes to a FILE stream (array version).
 *
 * Writes the DAG structure of nodes in @p p to the stream in a text format
 * that can be read back by bddimport. A @c bddnull entry acts as a
 * sentinel: export stops at the first @c bddnull, so only elements before
 * it are written.
 *
 * @param strm Output FILE stream.
 * @param p Array of node IDs to export. @c bddnull terminates the array.
 * @param lim Maximum number of elements to scan in @p p.
 */
void bddexport(FILE* strm, const bddp* p, int lim);

/**
 * @brief Export BDD/ZDD nodes to a FILE stream (vector version).
 * @param strm Output FILE stream.
 * @param v Vector of node IDs to export. @c bddnull terminates the sequence.
 */
void bddexport(FILE* strm, const std::vector<bddp>& v);

/**
 * @brief Export BDD/ZDD nodes to an output stream (array version).
 *
 * @copydetails bddexport(FILE*, const bddp*, int)
 * @param strm Output stream.
 * @param p Array of node IDs to export. @c bddnull terminates the array.
 * @param lim Maximum number of elements to scan in @p p.
 */
void bddexport(std::ostream& strm, const bddp* p, int lim);

/**
 * @brief Export BDD/ZDD nodes to an output stream (vector version).
 * @param strm Output stream.
 * @param v Vector of node IDs to export. @c bddnull terminates the sequence.
 */
void bddexport(std::ostream& strm, const std::vector<bddp>& v);

/**
 * @brief Import BDD nodes from a FILE stream (array version).
 *
 * Reads BDD nodes from the stream and reconstructs them using BDD
 * reduction rules. The imported nodes are stored in @p p.
 *
 * @param strm Input FILE stream.
 * @param p Array to store imported node IDs.
 * @param lim Maximum number of nodes to import.
 * @return The number of nodes successfully imported.
 */
int bddimport(FILE* strm, bddp* p, int lim);

/**
 * @brief Import BDD nodes from a FILE stream (vector version).
 * @param strm Input FILE stream.
 * @param v Vector to store imported node IDs.
 * @return The number of nodes successfully imported.
 */
int bddimport(FILE* strm, std::vector<bddp>& v);

/**
 * @brief Import BDD nodes from an input stream (array version).
 * @param strm Input stream.
 * @param p Array to store imported node IDs.
 * @param lim Maximum number of nodes to import.
 * @return The number of nodes successfully imported.
 */
int bddimport(std::istream& strm, bddp* p, int lim);

/**
 * @brief Import BDD nodes from an input stream (vector version).
 * @param strm Input stream.
 * @param v Vector to store imported node IDs.
 * @return The number of nodes successfully imported.
 */
int bddimport(std::istream& strm, std::vector<bddp>& v);

/**
 * @brief Import ZDD nodes from a FILE stream (array version).
 *
 * Reads nodes from the stream and reconstructs them using ZDD
 * reduction rules. The imported nodes are stored in @p p.
 *
 * @param strm Input FILE stream.
 * @param p Array to store imported node IDs.
 * @param lim Maximum number of nodes to import.
 * @return The number of nodes successfully imported.
 */
int bddimportz(FILE* strm, bddp* p, int lim);

/**
 * @brief Import ZDD nodes from a FILE stream (vector version).
 * @param strm Input FILE stream.
 * @param v Vector to store imported node IDs.
 * @return The number of nodes successfully imported.
 */
int bddimportz(FILE* strm, std::vector<bddp>& v);

/**
 * @brief Import ZDD nodes from an input stream (array version).
 * @param strm Input stream.
 * @param p Array to store imported node IDs.
 * @param lim Maximum number of nodes to import.
 * @return The number of nodes successfully imported.
 */
int bddimportz(std::istream& strm, bddp* p, int lim);

/**
 * @brief Import ZDD nodes from an input stream (vector version).
 * @param strm Input stream.
 * @param v Vector to store imported node IDs.
 * @return The number of nodes successfully imported.
 */
int bddimportz(std::istream& strm, std::vector<bddp>& v);

/**
 * @brief Import a single ZDD from a FILE stream.
 * @param strm Input FILE stream.
 * @return The imported ZDD.
 */
ZDD ZDD_Import(FILE* strm);

/**
 * @brief Import multiple ZDDs from a FILE stream.
 * @param strm Input FILE stream.
 * @param v Vector to store imported ZDDs.
 * @return The number of ZDDs successfully imported.
 */
int ZDD_Import(FILE* strm, std::vector<ZDD>& v);

/**
 * @brief Import a single ZDD from an input stream.
 * @param strm Input stream.
 * @return The imported ZDD.
 */
ZDD ZDD_Import(std::istream& strm);

/**
 * @brief Import multiple ZDDs from an input stream.
 * @param strm Input stream.
 * @param v Vector to store imported ZDDs.
 * @return The number of ZDDs successfully imported.
 */
int ZDD_Import(std::istream& strm, std::vector<ZDD>& v);

/**
 * @brief Dump the internal structure of a single BDD/ZDD to stdout.
 * @param f A node ID to dump.
 */
void bdddump(bddp f);

/**
 * @brief Dump the internal structure of multiple BDD/ZDDs to stdout.
 *
 * A @c bddnull entry acts as a sentinel: only elements before the first
 * @c bddnull are dumped as nodes, and roots are printed up to and
 * including that @c bddnull.
 *
 * @param p Array of node IDs to dump. @c bddnull terminates the array.
 * @param n Maximum number of elements to scan in @p p.
 */
void bddvdump(bddp *p, int n);

/** @brief @deprecated Always throws. Retained for API compatibility. */
void bddgraph0(bddp f);
/** @brief @deprecated Always throws. Retained for API compatibility. */
void bddgraph(bddp f);
/** @brief @deprecated Always throws. Retained for API compatibility. */
void bddvgraph0(bddp* ptr, int lim);
/** @brief @deprecated Always throws. Retained for API compatibility. */
void bddvgraph(bddp* ptr, int lim);

#endif
