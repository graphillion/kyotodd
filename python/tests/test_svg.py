"""SVG export keyword arguments (review-SVG-20261021 item 20)."""
import io
import re

import pytest

import kyotodd
from kyotodd import BDD, ZDD, MTBDDFloat


def _circles(svg):
    """(cx, cy, r) of every <circle> drawn in the body (markers excluded)."""
    body = svg[svg.find("</defs>"):] if "</defs>" in svg else svg
    return [(float(a), float(b), float(c)) for a, b, c in
            re.findall(r'<circle cx="([-\d.]+)" cy="([-\d.]+)" r="(\d+)"', body)]


def _viewbox(svg):
    return [float(v) for v in re.search(r'viewBox="([^"]+)"', svg).group(1).split()]


class TestZDDSvgKwargs:
    def test_defaults(self):
        kyotodd.new_vars(3)
        svg = ZDD.combination(3, 2).save_svg_str()
        assert svg.startswith("<svg") and svg.rstrip().endswith("</svg>")
        assert 'font-family="sans-serif"' in svg
        assert len(_circles(svg)) == 4
        assert all(r == 20 for _, _, r in _circles(svg))

    def test_var_name_map(self):
        v = kyotodd.new_vars(3)
        svg = ZDD.combination(3, 2).save_svg_str(var_name_map={v[2]: "top", v[0]: "bottom"})
        assert ">top<" in svg and ">bottom<" in svg
        assert ">%d<" % v[2] not in svg

    def test_var_name_map_rejects_non_dict(self):
        v = kyotodd.new_var()
        with pytest.raises(TypeError):
            ZDD.singleton(v).save_svg_str(var_name_map=["a"])

    def test_sizes_and_colours(self):
        v = kyotodd.new_var()
        z = ZDD.singleton(v)
        svg = z.save_svg_str(node_radius=30, font_size=12, node_fill="#ffffff",
                             node_stroke="#ff0000", font_family="serif")
        assert all(r == 30 for _, _, r in _circles(svg))
        assert 'font-size="12"' in svg
        assert 'fill="#ffffff"' in svg
        assert 'stroke="#ff0000"' in svg
        assert '#1b3966' not in svg            # arrowheads follow node_stroke too
        assert 'font-family="serif"' in svg

    def test_skip_unused_levels(self):
        v = kyotodd.new_vars(5)
        z = ZDD.singleton(v[4])               # node at level 5, levels 1..4 empty
        compact = _viewbox(z.save_svg_str())
        full = _viewbox(z.save_svg_str(skip_unused_levels=False))
        assert full[3] > compact[3] + 300    # four empty rows of 80 px

    def test_intervals(self):
        kyotodd.new_vars(3)
        z = ZDD.combination(3, 2)
        narrow = _viewbox(z.save_svg_str(node_interval_x=10, node_interval_y=10))
        wide = _viewbox(z.save_svg_str(node_interval_x=60, node_interval_y=80))
        assert wide[2] > narrow[2] and wide[3] > narrow[3]

    def test_invalid_values(self):
        v = kyotodd.new_var()
        z = ZDD.singleton(v)
        with pytest.raises(ValueError):
            z.save_svg_str(node_radius=0)
        with pytest.raises(ValueError):
            z.save_svg_str(font_size=-1)
        with pytest.raises(ValueError):
            z.save_svg_str(node_interval_y=-5)

    def test_file_matches_str(self):
        v = kyotodd.new_vars(2)
        z = ZDD.singleton(v[0]) + ZDD.singleton(v[1])
        buf = io.StringIO()
        z.save_svg_file(buf, raw=True, var_name_map={v[0]: "a"}, node_radius=25)
        assert buf.getvalue() == z.save_svg_str(raw=True, var_name_map={v[0]: "a"}, node_radius=25)


class TestOtherClassesSvgKwargs:
    def test_bdd(self):
        v = kyotodd.new_vars(2)
        f = BDD.var(v[0]) & BDD.var(v[1])
        svg = f.save_svg_str(var_name_map={v[0]: "x", v[1]: "y"}, node_stroke="#00aa00")
        assert ">x<" in svg and ">y<" in svg and 'stroke="#00aa00"' in svg

    def test_mtbdd(self):
        v = kyotodd.new_var()
        m = MTBDDFloat.ite(v, MTBDDFloat.terminal(1.5), MTBDDFloat.terminal(2.5))
        svg = m.save_svg_str(var_name_map={v: "c"}, font_size=18)
        assert ">c<" in svg and 'font-size="18"' in svg
        # a long name is still drawn, at a smaller size that fits the node
        svg = m.save_svg_str(var_name_map={v: "condition"}, font_size=18)
        assert ">condition<" in svg and 'font-size="18"' not in svg
