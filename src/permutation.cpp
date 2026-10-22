#include "permutation.h"
#include <algorithm>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace kyotodd {

Permutation::Permutation(const std::vector<int>& v)
    : v_(v)
{
    const int n = static_cast<int>(v_.size());
    std::vector<char> seen(static_cast<size_t>(n), 0);
    for (int i = 0; i < n; i++) {
        const int x = v_[i];
        if (x < 1 || x > n) {
            std::ostringstream msg;
            msg << "Permutation: element " << x << " at position " << (i + 1)
                << " is outside 1.." << n;
            throw std::invalid_argument(msg.str());
        }
        if (seen[x - 1]) {
            std::ostringstream msg;
            msg << "Permutation: element " << x << " appears more than once";
            throw std::invalid_argument(msg.str());
        }
        seen[x - 1] = 1;
    }
}

Permutation Permutation::identity(int n)
{
    if (n < 0) throw std::invalid_argument("Permutation::identity: n must be >= 0");
    std::vector<int> v(static_cast<size_t>(n));
    for (int i = 0; i < n; i++) v[i] = i + 1;
    return Permutation(v, Unchecked());
}

int Permutation::operator()(int i) const
{
    if (i < 1) throw std::out_of_range("Permutation: position must be >= 1");
    if (i > size()) return i;
    return v_[i - 1];
}

std::string Permutation::to_string() const
{
    std::ostringstream os;
    os << "[";
    for (int i = 0; i < size(); i++) {
        if (i > 0) os << " ";
        os << v_[i];
    }
    os << "]";
    return os.str();
}

Permutation Permutation::inverse() const
{
    std::vector<int> r(v_.size());
    for (int i = 0; i < size(); i++) r[v_[i] - 1] = i + 1;
    return Permutation(r, Unchecked());
}

Permutation Permutation::operator*(const Permutation& q) const
{
    const int n = std::max(size(), q.size());
    std::vector<int> r(static_cast<size_t>(n));
    for (int i = 1; i <= n; i++) r[i - 1] = q((*this)(i));
    return Permutation(r, Unchecked());
}

bool Permutation::is_even() const
{
    /* parity = (n - number of cycles) mod 2 */
    const int n = size();
    std::vector<char> visited(static_cast<size_t>(n), 0);
    int transpositions = 0;
    for (int i = 0; i < n; i++) {
        if (visited[i]) continue;
        int len = 0;
        for (int j = i; !visited[j]; j = v_[j] - 1) {
            visited[j] = 1;
            len++;
        }
        transpositions += len - 1;
    }
    return (transpositions % 2) == 0;
}

std::size_t Permutation::hash() const
{
    /* Hash only up to the last non-fixed point, so that permutations that
       differ only in trailing fixed points (which compare equal) hash
       equally. */
    int m = size();
    while (m > 0 && v_[m - 1] == m) m--;
    uint64_t h = 1469598103934665603ULL;   // FNV-1a offset basis
    for (int i = 0; i < m; i++) {
        h ^= static_cast<uint64_t>(v_[i]);
        h *= 1099511628211ULL;             // FNV-1a prime
    }
    return static_cast<std::size_t>(h);
}

bool operator==(const Permutation& a, const Permutation& b)
{
    const int n = std::max(a.size(), b.size());
    for (int i = 1; i <= n; i++) {
        if (a(i) != b(i)) return false;
    }
    return true;
}

bool operator<(const Permutation& a, const Permutation& b)
{
    const int n = std::max(a.size(), b.size());
    for (int i = 1; i <= n; i++) {
        const int x = a(i), y = b(i);
        if (x != y) return x < y;
    }
    return false;
}

std::ostream& operator<<(std::ostream& os, const Permutation& p)
{
    return os << p.to_string();
}

} // namespace kyotodd
