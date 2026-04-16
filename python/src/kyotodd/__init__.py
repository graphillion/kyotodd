"""KyotoDD - BDD/ZDD library for Python."""

import atexit as _atexit
import os as _os
import tempfile as _tempfile


def _show_svg(self):
    """Display the DD as an SVG diagram.

    In Jupyter/Colab: renders inline using IPython.display.
    Otherwise: opens in the default web browser.
    """
    svg = self.save_svg_str()
    try:
        from IPython.display import SVG, display
        display(SVG(data=svg))
    except ImportError:
        import webbrowser
        fd, path = _tempfile.mkstemp(suffix=".svg")
        try:
            _os.write(fd, svg.encode("utf-8"))
        finally:
            _os.close(fd)
        _atexit.register(_os.unlink, path)
        webbrowser.open("file://" + path)


from kyotodd._core import (
    BDD,
    ZDD,
    QDD,
    UnreducedDD,
    PiDD,
    RotPiDD,
    SeqBDD,
    BddCountMemo,
    ZddCountMemo,
    CostBoundMemo,
    WeightMode,
    WeightedSampleMemo,
    init,
    finalize,
    new_var,
    new_vars,
    new_var_of_level,
    var_count,
    to_level,
    to_var,
    node_count,
    gc,
    live_nodes,
    gc_rootcount,
    gc_set_threshold,
    gc_get_threshold,
    top_level,
    pidd_newvar,
    pidd_var_used,
    rotpidd_newvar,
    rotpidd_var_used,
    rotpidd_from_perm,
    MVDDVarInfo,
    MVDDVarTable,
    MVBDD,
    MVZDD,
    MTBDDFloat,
    MTBDDInt,
    MTZDDFloat,
    MTZDDInt,
)

for _cls in (BDD, ZDD, QDD, UnreducedDD, PiDD, RotPiDD, SeqBDD,
             MVBDD, MVZDD, MTBDDFloat, MTBDDInt, MTZDDFloat, MTZDDInt):
    _cls.show = _show_svg

__all__ = [
    "BDD",
    "ZDD",
    "QDD",
    "UnreducedDD",
    "PiDD",
    "RotPiDD",
    "SeqBDD",
    "BddCountMemo",
    "ZddCountMemo",
    "CostBoundMemo",
    "WeightMode",
    "WeightedSampleMemo",
    "init",
    "finalize",
    "new_var",
    "new_vars",
    "new_var_of_level",
    "var_count",
    "to_level",
    "to_var",
    "node_count",
    "gc",
    "live_nodes",
    "gc_rootcount",
    "gc_set_threshold",
    "gc_get_threshold",
    "top_level",
    "pidd_newvar",
    "pidd_var_used",
    "rotpidd_newvar",
    "rotpidd_var_used",
    "rotpidd_from_perm",
    "MVDDVarInfo",
    "MVDDVarTable",
    "MVBDD",
    "MVZDD",
    "MTBDDFloat",
    "MTBDDInt",
    "MTZDDFloat",
    "MTZDDInt",
]
