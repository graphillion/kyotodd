#ifndef KYOTODD_MTBDD_H
#define KYOTODD_MTBDD_H

// Umbrella header for the MTBDD/MTZDD API. The implementation is split
// across three internal headers to keep each file under the 2000-line
// limit from CLAUDE.md:
//
//   - mtbdd_core.h  : terminal table + apply / ITE templates +
//                     from_bdd/from_zdd conversion + counting helpers
//   - mtbdd_class.h : MTBDD<T> / MTZDD<T> class definitions
//   - mtbdd_io.h    : binary export/import, SVG forward decls, operators
//
// End users keep including "mtbdd.h" — no public API has changed.

#include "mtbdd_core.h"
#include "mtbdd_class.h"
#include "mtbdd_io.h"

#endif
