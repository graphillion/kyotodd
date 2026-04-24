/***************************************************************
 *  Simple regular-expression expansion via SeqBDD.              *
 *                                                               *
 *  Grammar (minimal regex):                                     *
 *      regex  := union                                          *
 *      union  := concat ('|' concat)*                           *
 *      concat := star ( star )*                                 *
 *      star   := atom ( '*' )*                                  *
 *      atom   := [a-z] | '(' regex ')'                          *
 *                                                               *
 *  SeqBDD represents finite sets of strings, so Kleene star is  *
 *  truncated at a user-supplied maximum length N: the result is *
 *  L(e) intersected with Sigma^{<= N}. All intermediate values  *
 *  are pruned with ZDD::size_le(N) (which counts 1-edges along  *
 *  a path = string length, correctly handling SeqBDD's repeated *
 *  variables).                                                  *
 *                                                               *
 *  Output:                                                      *
 *    * AST as an S-expression                                   *
 *    * Assigned alphabet (sorted, one bddvar per symbol)        *
 *    * card (# accepted strings), size (# ZDD nodes),           *
 *      lit (total symbols), len (longest string)                *
 *    * Up to 100 accepted strings (remainder summarized)        *
 *    * Internal ZDD as SVG (with a..z variable labels)          *
 *                                                               *
 *  Usage:                                                       *
 *      regex_seqbdd <regex> <max_len> [<svg_output>]            *
 ***************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "bdd.h"
#include "seqbdd.h"
#include "svg_export.h"

using namespace kyotodd;

// -------------------------------------------------------------
// AST
// -------------------------------------------------------------

enum class NodeKind { Char, Concat, Union, Star };

struct Node {
  NodeKind kind;
  char ch;                       // Char only
  std::shared_ptr<Node> left;    // Concat/Union left; Star child
  std::shared_ptr<Node> right;   // Concat/Union right
};
using NodePtr = std::shared_ptr<Node>;

static NodePtr mk_char(char c) {
  auto n = std::make_shared<Node>();
  n->kind = NodeKind::Char;
  n->ch = c;
  return n;
}
static NodePtr mk_concat(NodePtr l, NodePtr r) {
  auto n = std::make_shared<Node>();
  n->kind = NodeKind::Concat;
  n->left = std::move(l);
  n->right = std::move(r);
  return n;
}
static NodePtr mk_union(NodePtr l, NodePtr r) {
  auto n = std::make_shared<Node>();
  n->kind = NodeKind::Union;
  n->left = std::move(l);
  n->right = std::move(r);
  return n;
}
static NodePtr mk_star(NodePtr e) {
  auto n = std::make_shared<Node>();
  n->kind = NodeKind::Star;
  n->left = std::move(e);
  return n;
}

// -------------------------------------------------------------
// Recursive-descent parser
// -------------------------------------------------------------

struct Parser {
  const std::string& s;
  std::size_t pos;
  explicit Parser(const std::string& src) : s(src), pos(0) {}

  bool eof() const { return pos >= s.size(); }
  char peek() const { return s[pos]; }

  static void err(const std::string& msg, std::size_t at) {
    std::ostringstream oss;
    oss << msg << " at position " << at;
    throw std::runtime_error(oss.str());
  }

  NodePtr parse_regex() {
    NodePtr n = parse_union();
    if (!eof()) err(std::string("unexpected character '") + peek() + "'", pos);
    return n;
  }

  NodePtr parse_union() {
    NodePtr lhs = parse_concat();
    while (!eof() && peek() == '|') {
      std::size_t bar = pos;
      ++pos;  // consume '|'
      if (eof() || peek() == '|' || peek() == ')') {
        err("empty sub-expression after '|'", bar);
      }
      NodePtr rhs = parse_concat();
      lhs = mk_union(std::move(lhs), std::move(rhs));
    }
    return lhs;
  }

  NodePtr parse_concat() {
    if (eof() || peek() == '|' || peek() == ')') {
      err("empty sub-expression", pos);
    }
    NodePtr lhs = parse_star();
    while (!eof() && peek() != '|' && peek() != ')') {
      NodePtr rhs = parse_star();
      lhs = mk_concat(std::move(lhs), std::move(rhs));
    }
    return lhs;
  }

  NodePtr parse_star() {
    NodePtr a = parse_atom();
    while (!eof() && peek() == '*') {
      ++pos;
      a = mk_star(std::move(a));
    }
    return a;
  }

  NodePtr parse_atom() {
    if (eof()) err("expected atom", pos);
    char c = peek();
    if (c == '*') err("'*' not preceded by an expression", pos);
    if (c == '|') err("empty sub-expression before '|'", pos);
    if (c >= 'a' && c <= 'z') {
      ++pos;
      return mk_char(c);
    }
    if (c == '(') {
      std::size_t open_at = pos;
      ++pos;
      if (!eof() && peek() == ')') err("empty sub-expression in '()'", open_at);
      NodePtr inner = parse_union();
      if (eof() || peek() != ')') err("unbalanced parenthesis", open_at);
      ++pos;
      return inner;
    }
    err(std::string("unexpected character '") + c + "'", pos);
    return nullptr;  // unreachable
  }
};

static NodePtr parse_regex(const std::string& src) {
  if (src.empty()) throw std::runtime_error("empty regex");
  Parser p(src);
  return p.parse_regex();
}

// -------------------------------------------------------------
// AST to S-expression
// -------------------------------------------------------------

static void ast_to_sexpr(const NodePtr& n, std::ostream& os) {
  switch (n->kind) {
    case NodeKind::Char:
      os << n->ch;
      break;
    case NodeKind::Concat:
      os << "(concat ";
      ast_to_sexpr(n->left, os);
      os << ' ';
      ast_to_sexpr(n->right, os);
      os << ')';
      break;
    case NodeKind::Union:
      os << "(union ";
      ast_to_sexpr(n->left, os);
      os << ' ';
      ast_to_sexpr(n->right, os);
      os << ')';
      break;
    case NodeKind::Star:
      os << "(star ";
      ast_to_sexpr(n->left, os);
      os << ')';
      break;
  }
}

// -------------------------------------------------------------
// Alphabet collection
// -------------------------------------------------------------

static void collect_chars(const NodePtr& n, std::set<char>& out) {
  switch (n->kind) {
    case NodeKind::Char:
      out.insert(n->ch);
      break;
    case NodeKind::Concat:
    case NodeKind::Union:
      collect_chars(n->left, out);
      collect_chars(n->right, out);
      break;
    case NodeKind::Star:
      collect_chars(n->left, out);
      break;
  }
}

// -------------------------------------------------------------
// AST -> SeqBDD
// -------------------------------------------------------------

static SeqBDD prune_len(const SeqBDD& f, int max_len) {
  // ZDD::size_le counts 1-edges, which for a SeqBDD path equals the string
  // length (even when the same variable appears more than once on the path).
  return SeqBDD(f.get_zdd().size_le(max_len));
}

static SeqBDD build_seqbdd(const NodePtr& n,
                           const std::map<char, bddvar>& ch2var,
                           int max_len) {
  switch (n->kind) {
    case NodeKind::Char: {
      if (max_len < 1) return SeqBDD(0);
      bddvar v = ch2var.at(n->ch);
      return SeqBDD(1).push(static_cast<int>(v));
    }
    case NodeKind::Concat: {
      SeqBDD l = build_seqbdd(n->left, ch2var, max_len);
      SeqBDD r = build_seqbdd(n->right, ch2var, max_len);
      return prune_len(l * r, max_len);
    }
    case NodeKind::Union: {
      SeqBDD l = build_seqbdd(n->left, ch2var, max_len);
      SeqBDD r = build_seqbdd(n->right, ch2var, max_len);
      return l + r;
    }
    case NodeKind::Star: {
      SeqBDD e = build_seqbdd(n->left, ch2var, max_len);
      SeqBDD acc(1);    // {epsilon}
      SeqBDD power(1);  // e^0 = {epsilon}
      for (int i = 0; i <= max_len; ++i) {
        SeqBDD next = prune_len(power * e, max_len);
        if (next == SeqBDD(0)) break;
        SeqBDD merged = acc + next;
        if (merged == acc) break;
        acc = merged;
        power = next;
      }
      return acc;
    }
  }
  return SeqBDD(0);  // unreachable
}

// -------------------------------------------------------------
// String enumeration with a..z labels
// -------------------------------------------------------------

static void enum_rec(std::ostream& os, SeqBDD f,
                     std::vector<char>& buf,
                     const std::map<bddvar, char>& var2ch,
                     int& printed, int limit) {
  if (f == SeqBDD(0)) return;
  // Emit the current string if epsilon is in f, then strip epsilon so
  // it is not re-emitted through the offset branch of the recursion.
  if ((f & SeqBDD(1)) == SeqBDD(1)) {
    if (printed < limit) {
      os << "  ";
      if (buf.empty()) {
        os << "(epsilon)";
      } else {
        for (char c : buf) os << c;
      }
      os << '\n';
    }
    ++printed;
    f = f - SeqBDD(1);
    if (f == SeqBDD(0)) return;
  }
  int t = f.top();
  if (t == 0) return;
  bddvar vt = static_cast<bddvar>(t);
  SeqBDD f1 = f.onset0(t);
  SeqBDD f0 = f.offset(t);
  buf.push_back(var2ch.at(vt));
  enum_rec(os, f1, buf, var2ch, printed, limit);
  buf.pop_back();
  enum_rec(os, f0, buf, var2ch, printed, limit);
}

static void enumerate_strings(std::ostream& os, const SeqBDD& f,
                              const std::map<bddvar, char>& var2ch,
                              int limit) {
  std::vector<char> buf;
  int printed = 0;
  enum_rec(os, f, buf, var2ch, printed, limit);
  if (printed > limit) {
    os << "  ... and " << (printed - limit) << " more\n";
  }
}

// -------------------------------------------------------------
// main
// -------------------------------------------------------------

static void usage(const char* prog) {
  std::fprintf(stderr,
      "Usage: %s <regex> <max_len> [<svg_output>]\n"
      "  <regex>       simple regex over [a-z] with operators | * ( )\n"
      "  <max_len>     non-negative integer, truncation length for Kleene star\n"
      "  <svg_output>  optional path to save the internal ZDD as SVG\n"
      "                (default: regex_seqbdd.svg)\n",
      prog);
}

int main(int argc, char* argv[]) {
  if (argc != 3 && argc != 4) {
    usage(argv[0]);
    return 1;
  }
  const std::string regex_str = argv[1];
  int max_len = std::atoi(argv[2]);
  const char* svg_path = (argc == 4) ? argv[3] : "regex_seqbdd.svg";

  if (max_len < 0) {
    std::fprintf(stderr, "Error: max_len must be >= 0\n");
    return 1;
  }

  if (bddinit(1024, bddnull)) {
    std::fprintf(stderr, "Error: BDD memory allocation failed.\n");
    return 1;
  }

  int rc = 0;
  try {
    NodePtr ast = parse_regex(regex_str);

    std::cout << "Regex      : " << regex_str << '\n';
    std::cout << "Max length : " << max_len << '\n';
    std::cout << "AST        : ";
    ast_to_sexpr(ast, std::cout);
    std::cout << '\n';

    std::set<char> chars;
    collect_chars(ast, chars);

    std::map<char, bddvar> ch2var;
    std::map<bddvar, char> var2ch;
    std::cout << "Alphabet   : {";
    bool first = true;
    for (char c : chars) {
      bddvar v = bddnewvar();
      ch2var[c] = v;
      var2ch[v] = c;
      if (!first) std::cout << ',';
      std::cout << c;
      first = false;
    }
    std::cout << "}  (bddvar ";
    first = true;
    for (char c : chars) {
      if (!first) std::cout << ',';
      std::cout << c << "=" << ch2var[c];
      first = false;
    }
    std::cout << ")\n";

    SeqBDD result = build_seqbdd(ast, ch2var, max_len);

    std::cout << "card       : " << result.card() << " (accepted strings)\n";
    std::cout << "size       : " << result.size() << " (ZDD nodes)\n";
    std::cout << "lit        : " << result.lit() << " (total symbols)\n";
    std::cout << "len        : " << result.len() << " (longest string)\n";

    std::cout << "Strings:\n";
    enumerate_strings(std::cout, result, var2ch, /*limit=*/100);

    SvgParams params;
    for (const auto& kv : ch2var) {
      params.var_name_map[kv.second] = std::string(1, kv.first);
    }
    result.save_svg(svg_path, params);
    std::cout << "SVG saved to: " << svg_path << '\n';
  } catch (const std::exception& e) {
    std::fprintf(stderr, "Error: %s\n", e.what());
    rc = 1;
  }

  bddfinal();
  return rc;
}
