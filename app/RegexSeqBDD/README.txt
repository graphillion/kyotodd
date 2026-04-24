RegexSeqBDD -- Simple regex expansion via SeqBDD
=================================================

Builds the SeqBDD representing every string over {a..z} that matches a
simple regular expression, truncated to a maximum length N. The regex
grammar supports concatenation (juxtaposition), alternation (|),
Kleene star (*), and parentheses. A SeqBDD is a ZDD-based data
structure for finite sets of strings, so the Kleene star has to be
bounded in length.

Usage
-----
    regex_seqbdd <regex> <max_len> [<svg_output>]

Arguments
---------
    regex        Regular expression over lowercase letters a..z, plus
                 operators | * ( ). An empty regex is not allowed.
    max_len      Non-negative integer. All accepted strings longer
                 than this are discarded; Kleene-star iterations stop
                 once no shorter extension remains.
    svg_output   Optional output path for the internal ZDD visualized
                 as SVG. Defaults to "regex_seqbdd.svg".

Output
------
    * The parsed AST (S-expression form)
    * The alphabet (sorted, one BDD variable per character)
    * Statistics: card (# accepted strings), size (# ZDD nodes),
      lit (total symbols), len (longest accepted string)
    * Up to 100 accepted strings (remainder summarized if more)
    * The internal ZDD as SVG, with a..z labels on variables

Examples
--------
    ./regex_seqbdd 'a|b'     2
    ./regex_seqbdd '(a|b)*c' 3  r.svg
    ./regex_seqbdd 'a*b*'    4

Grammar (BNF)
-------------
    regex  := union
    union  := concat ('|' concat)*
    concat := star (star)*
    star   := atom ('*')*
    atom   := [a-z] | '(' regex ')'

Notes
-----
    * Kleene star is truncated: (a)* with max_len=3 yields the four
      strings {epsilon, a, aa, aaa}.
    * Characters outside a..z (uppercase, digits, symbols other than
      the four operators) cause a parse error.
    * Enumeration order follows the ZDD structure (shortest first,
      then alphabetic within a length when variables are assigned in
      a..z order), not strict lexicographic order.
    * Length pruning uses ZDD::size_le(N), which counts 1-edges along
      each path. For SeqBDD, that count equals the string length even
      when the same variable appears more than once along the path.
