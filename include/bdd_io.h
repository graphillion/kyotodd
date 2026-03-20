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

// --- Knuth format save/load (obsolete) ---

/**
 * @deprecated Use Sapporo or Binary format instead.
 * @brief Export a single BDD in Knuth format to a FILE stream.
 *
 * Knuth format is a text format with level headers (#1, #2, ...) and
 * node lines (id:lo,hi). Complement edges are expanded. Provided for
 * backward compatibility only.
 *
 * @param strm   Output FILE stream.
 * @param f      BDD root node ID.
 * @param is_hex If true, use hexadecimal; otherwise decimal.
 * @param offset Level offset: level = N + 1 - knuth_level + offset.
 */
void bdd_export_knuth(FILE* strm, bddp f, bool is_hex = false, int offset = 0);

/** @deprecated @copybrief bdd_export_knuth */
void bdd_export_knuth(std::ostream& strm, bddp f, bool is_hex = false, int offset = 0);

/**
 * @deprecated Use Sapporo or Binary format instead.
 * @brief Import a single BDD from Knuth format from a FILE stream.
 */
bddp bdd_import_knuth(FILE* strm, bool is_hex = false, int offset = 0);

/** @deprecated @copybrief bdd_import_knuth */
bddp bdd_import_knuth(std::istream& strm, bool is_hex = false, int offset = 0);

/** @deprecated @copybrief bdd_export_knuth */
void zdd_export_knuth(FILE* strm, bddp f, bool is_hex = false, int offset = 0);

/** @deprecated @copybrief bdd_export_knuth */
void zdd_export_knuth(std::ostream& strm, bddp f, bool is_hex = false, int offset = 0);

/** @deprecated @copybrief bdd_import_knuth */
bddp zdd_import_knuth(FILE* strm, bool is_hex = false, int offset = 0);

/** @deprecated @copybrief bdd_import_knuth */
bddp zdd_import_knuth(std::istream& strm, bool is_hex = false, int offset = 0);

// --- Binary format save/load ---

/**
 * @brief Export a single BDD in BDD binary format to a FILE stream.
 *
 * Writes the BDD in the binary format defined by bdd_binary_format.md.
 * Uses little-endian byte ordering, 2 terminals, 2 arcs, and
 * negative (complement) arcs.
 *
 * @param strm Output FILE stream.
 * @param f    BDD root node ID.
 */
void bdd_export_binary(FILE* strm, bddp f);

/** @brief Export a single BDD in BDD binary format to an output stream. */
void bdd_export_binary(std::ostream& strm, bddp f);

/**
 * @brief Import a single BDD from BDD binary format from a FILE stream.
 * @param strm Input FILE stream.
 * @return The imported BDD root node ID.
 */
bddp bdd_import_binary(FILE* strm);

/** @brief Import a single BDD from BDD binary format from an input stream. */
bddp bdd_import_binary(std::istream& strm);

/**
 * @brief Export a single ZDD in BDD binary format to a FILE stream.
 * @param strm Output FILE stream.
 * @param f    ZDD root node ID.
 */
void zdd_export_binary(FILE* strm, bddp f);

/** @brief Export a single ZDD in BDD binary format to an output stream. */
void zdd_export_binary(std::ostream& strm, bddp f);

/**
 * @brief Import a single ZDD from BDD binary format from a FILE stream.
 * @param strm Input FILE stream.
 * @return The imported ZDD root node ID.
 */
bddp zdd_import_binary(FILE* strm);

/** @brief Import a single ZDD from BDD binary format from an input stream. */
bddp zdd_import_binary(std::istream& strm);

// --- QDD binary format ---

/** @brief Export a single QDD in BDD binary format to a FILE stream. */
void qdd_export_binary(FILE* strm, bddp f);
/** @brief Export a single QDD in BDD binary format to an output stream. */
void qdd_export_binary(std::ostream& strm, bddp f);
/** @brief Import a single QDD from BDD binary format from a FILE stream. */
bddp qdd_import_binary(FILE* strm);
/** @brief Import a single QDD from BDD binary format from an input stream. */
bddp qdd_import_binary(std::istream& strm);

// --- UnreducedDD binary format ---

/** @brief Export a single UnreducedDD in BDD binary format to a FILE stream. */
void unreduced_export_binary(FILE* strm, bddp f);
/** @brief Export a single UnreducedDD in BDD binary format to an output stream. */
void unreduced_export_binary(std::ostream& strm, bddp f);
/** @brief Import a single UnreducedDD from BDD binary format from a FILE stream. */
bddp unreduced_import_binary(FILE* strm);
/** @brief Import a single UnreducedDD from BDD binary format from an input stream. */
bddp unreduced_import_binary(std::istream& strm);

// --- Sapporo format save/load ---

/**
 * @brief Export a single BDD in Sapporo format to a FILE stream.
 * @param strm Output FILE stream.
 * @param f    BDD root node ID.
 */
void bdd_export_sapporo(FILE* strm, bddp f);

/** @brief Export a single BDD in Sapporo format to an output stream. */
void bdd_export_sapporo(std::ostream& strm, bddp f);

/**
 * @brief Import a single BDD from Sapporo format from a FILE stream.
 * @param strm Input FILE stream.
 * @return The imported BDD root node ID, or bddfalse on failure.
 */
bddp bdd_import_sapporo(FILE* strm);

/** @brief Import a single BDD from Sapporo format from an input stream. */
bddp bdd_import_sapporo(std::istream& strm);

/**
 * @brief Export a single ZDD in Sapporo format to a FILE stream.
 * @param strm Output FILE stream.
 * @param f    ZDD root node ID.
 */
void zdd_export_sapporo(FILE* strm, bddp f);

/** @brief Export a single ZDD in Sapporo format to an output stream. */
void zdd_export_sapporo(std::ostream& strm, bddp f);

/**
 * @brief Import a single ZDD from Sapporo format from a FILE stream.
 * @param strm Input FILE stream.
 * @return The imported ZDD root node ID, or bddempty on failure.
 */
bddp zdd_import_sapporo(FILE* strm);

/** @brief Import a single ZDD from Sapporo format from an input stream. */
bddp zdd_import_sapporo(std::istream& strm);

// --- Graphillion format save/load for ZDD ---

/**
 * @brief Export a single ZDD in Graphillion format to a FILE stream.
 *
 * Writes the ZDD in the Graphillion text format. Complement edges are
 * expanded into separate nodes. Variable numbers are reversed so that
 * lower Graphillion variable numbers correspond to higher KyotoDD levels
 * (closer to the root).
 *
 * @param strm Output FILE stream.
 * @param f    ZDD root node ID.
 * @param offset Level offset: level = N + 1 - graphillion_var + offset.
 */
void zdd_export_graphillion(FILE* strm, bddp f, int offset = 0);

/**
 * @brief Export a single ZDD in Graphillion format to an output stream.
 * @copydetails zdd_export_graphillion(FILE*, bddp, int)
 */
void zdd_export_graphillion(std::ostream& strm, bddp f, int offset = 0);

/**
 * @brief Import a single ZDD from Graphillion format from a FILE stream.
 *
 * Reads a ZDD in the Graphillion text format. Variable numbers are
 * reversed so that Graphillion variable 1 (root-side) maps to the
 * highest KyotoDD level.
 *
 * @param strm   Input FILE stream.
 * @param offset Level offset: level = N + 1 - graphillion_var + offset.
 * @return The imported ZDD root node ID.
 */
bddp zdd_import_graphillion(FILE* strm, int offset = 0);

/**
 * @brief Import a single ZDD from Graphillion format from an input stream.
 * @copydetails zdd_import_graphillion(FILE*, int)
 */
bddp zdd_import_graphillion(std::istream& strm, int offset = 0);

// --- Graphviz DOT format ---

/** @brief Save BDD as Graphviz DOT to a FILE stream. */
void bdd_save_graphviz(FILE* strm, bddp f,
                       GraphvizMode mode = GraphvizMode::Expanded);
/** @brief Save BDD as Graphviz DOT to an output stream. */
void bdd_save_graphviz(std::ostream& strm, bddp f,
                       GraphvizMode mode = GraphvizMode::Expanded);
/** @brief Save ZDD as Graphviz DOT to a FILE stream. */
void zdd_save_graphviz(FILE* strm, bddp f,
                       GraphvizMode mode = GraphvizMode::Expanded);
/** @brief Save ZDD as Graphviz DOT to an output stream. */
void zdd_save_graphviz(std::ostream& strm, bddp f,
                       GraphvizMode mode = GraphvizMode::Expanded);

/** @brief @deprecated Always throws. Retained for API compatibility. */
void bddgraph0(bddp f);
/** @brief @deprecated Always throws. Retained for API compatibility. */
void bddgraph(bddp f);
/** @brief @deprecated Always throws. Retained for API compatibility. */
void bddvgraph0(bddp* ptr, int lim);
/** @brief @deprecated Always throws. Retained for API compatibility. */
void bddvgraph(bddp* ptr, int lim);

#endif
