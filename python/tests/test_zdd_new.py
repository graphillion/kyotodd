import pytest
import kyotodd
from kyotodd import ZDD


def _make_singleton(var):
    """Create a ZDD representing the family {{var}}."""
    z = ZDD(1)  # {{}}
    return z.change(var)


class TestZDDToQDD:
    def test_terminal_empty(self):
        qdd = ZDD(0).to_qdd()
        assert isinstance(qdd, kyotodd.QDD)
        assert qdd.is_zero

    def test_terminal_single(self):
        qdd = ZDD(1).to_qdd()
        assert isinstance(qdd, kyotodd.QDD)
        assert qdd.is_one

    def test_singleton(self):
        kyotodd.new_var()
        z = _make_singleton(1)
        qdd = z.to_qdd()
        assert qdd.top_var == 1
        assert qdd.to_zdd() == z


class TestZDDCount:
    def test_empty(self):
        assert ZDD(0).count() == 0.0

    def test_single(self):
        assert ZDD(1).count() == 1.0

    def test_family(self):
        kyotodd.new_var()
        kyotodd.new_var()
        a = _make_singleton(1)
        b = _make_singleton(2)
        u = a + b  # {{1}, {2}}
        assert u.count() == 2.0

    def test_power_set(self):
        kyotodd.new_var()
        kyotodd.new_var()
        kyotodd.new_var()
        ps = ZDD.power_set(3)
        assert ps.count() == 8.0


class TestZDDUniformSample:
    def test_returns_list(self):
        kyotodd.new_var()
        z = _make_singleton(1)
        result = z.uniform_sample(seed=42)
        assert isinstance(result, list)
        assert result == [1]

    def test_single(self):
        result = ZDD(1).uniform_sample(seed=0)
        assert result == []

    def test_family(self):
        kyotodd.new_var()
        kyotodd.new_var()
        a = _make_singleton(1)
        b = _make_singleton(2)
        u = a + b  # {{1}, {2}}
        result = u.uniform_sample(seed=42)
        assert result == [1] or result == [2]


class TestZDDEnumerate:
    def test_empty(self):
        assert ZDD(0).enumerate() == []

    def test_single(self):
        assert ZDD(1).enumerate() == [[]]

    def test_singleton(self):
        kyotodd.new_var()
        z = _make_singleton(1)
        assert z.enumerate() == [[1]]

    def test_family(self):
        kyotodd.new_var()
        kyotodd.new_var()
        a = _make_singleton(1)
        b = _make_singleton(2)
        u = a + b + ZDD(1)  # {{}, {1}, {2}}
        sets = u.enumerate()
        assert len(sets) == 3
        assert [] in sets
        assert [1] in sets
        assert [2] in sets


class TestZDDHasEmpty:
    def test_empty_family(self):
        assert not ZDD(0).has_empty()

    def test_unit_family(self):
        assert ZDD(1).has_empty()

    def test_singleton_no_empty(self):
        kyotodd.new_var()
        z = _make_singleton(1)
        assert not z.has_empty()

    def test_with_empty(self):
        kyotodd.new_var()
        z = _make_singleton(1) + ZDD(1)  # {{1}, {}}
        assert z.has_empty()


class TestZDDSingleton:
    def test_basic(self):
        kyotodd.new_var()
        z = ZDD.singleton(1)
        assert z.to_str() == "{1}"
        assert z == _make_singleton(1)

    def test_different_vars(self):
        kyotodd.new_var()
        kyotodd.new_var()
        z1 = ZDD.singleton(1)
        z2 = ZDD.singleton(2)
        assert z1 != z2
        assert z1.to_str() == "{1}"
        assert z2.to_str() == "{2}"


class TestZDDSingleSet:
    def test_empty_list(self):
        z = ZDD.single_set([])
        assert z == ZDD(1)  # {{}}

    def test_one_var(self):
        kyotodd.new_var()
        z = ZDD.single_set([1])
        assert z == _make_singleton(1)

    def test_two_vars(self):
        kyotodd.new_var()
        kyotodd.new_var()
        z = ZDD.single_set([1, 2])
        assert z.enumerate() == [[1, 2]]


class TestZDDPowerSet:
    def test_zero(self):
        ps = ZDD.power_set(0)
        assert ps == ZDD(1)  # {{}}

    def test_one(self):
        kyotodd.new_var()
        ps = ZDD.power_set(1)
        assert ps.count() == 2.0

    def test_two(self):
        kyotodd.new_var()
        kyotodd.new_var()
        ps = ZDD.power_set(2)
        assert ps.count() == 4.0
        assert ps.has_empty()

    def test_vars_version(self):
        kyotodd.new_var()
        kyotodd.new_var()
        kyotodd.new_var()
        ps = ZDD.power_set_vars([1, 3])
        assert ps.count() == 4.0
        sets = ps.enumerate()
        assert len(sets) == 4
        assert [] in sets
        assert [1] in sets
        assert [3] in sets
        assert sorted([3, 1]) == sorted([s for s in sets if len(s) == 2][0])


class TestZDDFromSets:
    def test_empty(self):
        z = ZDD.from_sets([])
        assert z == ZDD(0)

    def test_single_empty_set(self):
        z = ZDD.from_sets([[]])
        assert z == ZDD(1)

    def test_singletons(self):
        kyotodd.new_var()
        kyotodd.new_var()
        z = ZDD.from_sets([[1], [2]])
        assert z.count() == 2.0

    def test_complex(self):
        kyotodd.new_var()
        kyotodd.new_var()
        kyotodd.new_var()
        z = ZDD.from_sets([[1, 2], [3], []])
        assert z.count() == 3.0
        assert z.has_empty()


class TestZDDCombination:
    def test_c_3_0(self):
        kyotodd.new_var()
        kyotodd.new_var()
        kyotodd.new_var()
        z = ZDD.combination(3, 0)
        assert z == ZDD(1)  # {{}}

    def test_c_3_1(self):
        kyotodd.new_var()
        kyotodd.new_var()
        kyotodd.new_var()
        z = ZDD.combination(3, 1)
        assert z.count() == 3.0

    def test_c_3_2(self):
        kyotodd.new_var()
        kyotodd.new_var()
        kyotodd.new_var()
        z = ZDD.combination(3, 2)
        assert z.count() == 3.0

    def test_c_3_3(self):
        kyotodd.new_var()
        kyotodd.new_var()
        kyotodd.new_var()
        z = ZDD.combination(3, 3)
        assert z.count() == 1.0


class TestZDDGetnode:
    def test_simple(self):
        kyotodd.new_var()
        lo = ZDD(0)
        hi = ZDD(1)
        n = ZDD.getnode(1, lo, hi)
        assert n.top_var == 1
        assert n.child0() == lo
        assert n.child1() == hi

    def test_zero_suppression(self):
        """ZDD reduction: hi == empty => return lo."""
        kyotodd.new_var()
        lo = ZDD(1)
        hi = ZDD(0)
        n = ZDD.getnode(1, lo, hi)
        assert n == lo  # zero-suppression rule applied


class TestZDDChildAccessors:
    def test_child0_child1(self):
        kyotodd.new_var()
        lo = ZDD(0)
        hi = ZDD(1)
        n = ZDD.getnode(1, lo, hi)
        assert n.child0() == lo
        assert n.child1() == hi

    def test_child_by_index(self):
        kyotodd.new_var()
        lo = ZDD(0)
        hi = ZDD(1)
        n = ZDD.getnode(1, lo, hi)
        assert n.child(0) == lo
        assert n.child(1) == hi

    def test_raw_child(self):
        kyotodd.new_var()
        lo = ZDD(0)
        hi = ZDD(1)
        n = ZDD.getnode(1, lo, hi)
        assert n.raw_child0() == lo
        assert n.raw_child1() == hi
        assert n.raw_child(0) == lo
        assert n.raw_child(1) == hi

    def test_complement_resolution(self):
        """ZDD complement only affects lo child."""
        kyotodd.new_var()
        lo = ZDD(0)
        hi = ZDD(1)
        n = ZDD.getnode(1, lo, hi)
        cn = ~n  # complement
        # With ZDD semantics, complement flips lo only
        assert cn.child0() == ~lo
        assert cn.child1() == hi


class TestZDDSharedSize:
    def test_single(self):
        kyotodd.new_var()
        z = _make_singleton(1)
        assert ZDD.shared_size([z]) == 1

    def test_multiple(self):
        kyotodd.new_var()
        kyotodd.new_var()
        z1 = _make_singleton(1)
        z2 = _make_singleton(2)
        size = ZDD.shared_size([z1, z2])
        assert size == 2

    def test_shared_plain_size(self):
        kyotodd.new_var()
        z = _make_singleton(1)
        assert ZDD.shared_plain_size([z]) >= 1


class TestZDDTerminalProperties:
    def test_is_terminal(self):
        assert ZDD(0).is_terminal
        assert ZDD(1).is_terminal

    def test_is_not_terminal(self):
        kyotodd.new_var()
        z = _make_singleton(1)
        assert not z.is_terminal

    def test_is_zero(self):
        assert ZDD(0).is_zero
        assert not ZDD(1).is_zero

    def test_is_one(self):
        assert ZDD(1).is_one
        assert not ZDD(0).is_one


class TestZDDBinaryIO:
    def test_roundtrip_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        z = _make_singleton(1) + _make_singleton(2)
        data = z.export_binary_str()
        assert isinstance(data, bytes)
        z2 = ZDD.import_binary_str(data)
        assert z == z2

    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        z = _make_singleton(1)
        path = str(tmp_path / "test.zdd")
        z.export_binary_file(path)
        z2 = ZDD.import_binary_file(path)
        assert z == z2

    def test_terminal_roundtrip(self):
        for val in [0, 1]:
            z = ZDD(val)
            data = z.export_binary_str()
            z2 = ZDD.import_binary_str(data)
            assert z == z2


class TestZDDBinaryMultiIO:
    def test_roundtrip_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        zdds = [_make_singleton(1), _make_singleton(2),
                _make_singleton(1) + _make_singleton(2)]
        data = ZDD.export_binary_multi_str(zdds)
        assert isinstance(data, bytes)
        zdds2 = ZDD.import_binary_multi_str(data)
        assert len(zdds2) == len(zdds)
        for a, b in zip(zdds, zdds2):
            assert a == b

    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        kyotodd.new_var()
        zdds = [_make_singleton(1), _make_singleton(2)]
        path = str(tmp_path / "multi.zdd")
        ZDD.export_binary_multi_file(zdds, path)
        zdds2 = ZDD.import_binary_multi_file(path)
        assert len(zdds2) == 2
        assert zdds[0] == zdds2[0]
        assert zdds[1] == zdds2[1]

    def test_empty_list(self):
        data = ZDD.export_binary_multi_str([])
        zdds = ZDD.import_binary_multi_str(data)
        assert zdds == []


class TestZDDSapporoIO:
    def test_roundtrip_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        z = _make_singleton(1) + _make_singleton(2)
        s = z.export_sapporo_str()
        assert isinstance(s, str)
        z2 = ZDD.import_sapporo_str(s)
        assert z == z2

    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        z = _make_singleton(1)
        path = str(tmp_path / "test.sapporo")
        z.export_sapporo_file(path)
        z2 = ZDD.import_sapporo_file(path)
        assert z == z2


class TestZDDGraphillionIO:
    def test_roundtrip_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        z = _make_singleton(1) + _make_singleton(2)
        s = z.export_graphillion_str()
        assert isinstance(s, str)
        z2 = ZDD.import_graphillion_str(s)
        assert z == z2

    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        z = _make_singleton(1)
        path = str(tmp_path / "test.graphillion")
        z.export_graphillion_file(path)
        z2 = ZDD.import_graphillion_file(path)
        assert z == z2

    def test_offset_export(self):
        kyotodd.new_var()
        kyotodd.new_var()
        z = _make_singleton(1)
        s_no_offset = z.export_graphillion_str()
        s_with_offset = z.export_graphillion_str(offset=1)
        # With offset=1, variable numbers in output are shifted
        assert s_no_offset != s_with_offset


class TestZDDGraphviz:
    def test_graphviz_str(self):
        kyotodd.new_var()
        z = _make_singleton(1)
        dot = z.save_graphviz_str()
        assert "digraph" in dot

    def test_graphviz_raw(self):
        kyotodd.new_var()
        z = _make_singleton(1)
        dot = z.save_graphviz_str(raw=True)
        assert "digraph" in dot

    def test_graphviz_file(self, tmp_path):
        kyotodd.new_var()
        z = _make_singleton(1)
        path = str(tmp_path / "test.dot")
        z.save_graphviz_file(path)
        with open(path) as f:
            content = f.read()
        assert "digraph" in content

    def test_graphviz_file_raw(self, tmp_path):
        kyotodd.new_var()
        z = _make_singleton(1)
        path = str(tmp_path / "test_raw.dot")
        z.save_graphviz_file(path, raw=True)
        with open(path) as f:
            content = f.read()
        assert "digraph" in content
