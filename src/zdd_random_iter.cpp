#include "bdd.h"
#include "zdd_random_iter.h"
#include <random>

namespace kyotodd {


// Explicit instantiation for std::mt19937_64 (used by Python bindings).
template class ZddRandomIterator<std::mt19937_64>;
template class ZddRandomRange<std::mt19937_64>;

} // namespace kyotodd
