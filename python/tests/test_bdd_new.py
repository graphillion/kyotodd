import pytest
import kyotodd


class TestBDDToQDD:
    def test_terminal_false(self):
        qdd = kyotodd.BDD(0).to_qdd()
        assert isinstance(qdd, kyotodd.QDD)
        assert qdd.is_zero

    def test_terminal_true(self):
        qdd = kyotodd.BDD(1).to_qdd()
        assert isinstance(qdd, kyotodd.QDD)
        assert qdd.is_one

    def test_single_var(self):
        v1 = kyotodd.new_var()
        bdd = kyotodd.BDD.var(v1)
        qdd = bdd.to_qdd()
        assert qdd.top_var == v1
        assert qdd.to_bdd() == bdd

    def test_two_var(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        bdd = kyotodd.BDD.var(v1) & kyotodd.BDD.var(v2)
        qdd = bdd.to_qdd()
        assert qdd.to_bdd() == bdd


class TestBDDCount:
    def test_false(self):
        assert kyotodd.BDD(0).count(3) == 0.0

    def test_true(self):
        assert kyotodd.BDD(1).count(3) == 8.0

    def test_single_var(self):
        v1 = kyotodd.new_var()
        bdd = kyotodd.BDD.var(v1)
        assert bdd.count(1) == 1.0

    def test_and(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        bdd = kyotodd.BDD.var(v1) & kyotodd.BDD.var(v2)
        assert bdd.count(2) == 1.0


class TestBDDExactCount:
    def test_false(self):
        assert kyotodd.BDD(0).exact_count(3) == 0

    def test_true(self):
        assert kyotodd.BDD(1).exact_count(3) == 8

    def test_returns_python_int(self):
        result = kyotodd.BDD(1).exact_count(3)
        assert isinstance(result, int)

    def test_large_count(self):
        v1 = kyotodd.new_var()
        bdd = kyotodd.BDD.var(v1)
        # With 64 variables, count = 2^63
        result = bdd.exact_count(64)
        assert result == 2**63


class TestBDDUniformSample:
    def test_returns_list(self):
        v1 = kyotodd.new_var()
        bdd = kyotodd.BDD.var(v1)
        result = bdd.uniform_sample(1, seed=42)
        assert isinstance(result, list)
        assert result == [v1]

    def test_true_bdd(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        bdd = kyotodd.BDD(1)
        result = bdd.uniform_sample(2, seed=0)
        # Result is some subset of {v1, v2}
        assert all(v in (v1, v2) for v in result)

    def test_and(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        bdd = kyotodd.BDD.var(v1) & kyotodd.BDD.var(v2)
        # Only one satisfying assignment: {v1, v2}
        result = bdd.uniform_sample(2, seed=0)
        assert sorted(result) == sorted([v1, v2])


class TestBDDPrime:
    def test_prime(self):
        v1 = kyotodd.new_var()
        p = kyotodd.BDD.prime(v1)
        assert p == kyotodd.BDD.var(v1)

    def test_prime_not(self):
        v1 = kyotodd.new_var()
        pn = kyotodd.BDD.prime_not(v1)
        assert pn == ~kyotodd.BDD.var(v1)


class TestBDDCubeClause:
    def test_cube_single(self):
        v1 = kyotodd.new_var()
        c = kyotodd.BDD.cube([int(v1)])
        assert c == kyotodd.BDD.var(v1)

    def test_cube_negated(self):
        v1 = kyotodd.new_var()
        c = kyotodd.BDD.cube([-int(v1)])
        assert c == ~kyotodd.BDD.var(v1)

    def test_cube_two(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        c = kyotodd.BDD.cube([int(v1), int(v2)])
        assert c == (kyotodd.BDD.var(v1) & kyotodd.BDD.var(v2))

    def test_clause_single(self):
        v1 = kyotodd.new_var()
        c = kyotodd.BDD.clause([int(v1)])
        assert c == kyotodd.BDD.var(v1)

    def test_clause_two(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        c = kyotodd.BDD.clause([int(v1), int(v2)])
        assert c == (kyotodd.BDD.var(v1) | kyotodd.BDD.var(v2))

    def test_cube_mixed(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        c = kyotodd.BDD.cube([int(v1), -int(v2)])
        assert c == (kyotodd.BDD.var(v1) & ~kyotodd.BDD.var(v2))


class TestBDDGetnode:
    def test_simple(self):
        v1 = kyotodd.new_var()
        b0 = kyotodd.BDD(0)
        b1 = kyotodd.BDD(1)
        n = kyotodd.BDD.getnode(v1, b0, b1)
        assert n.top_var == v1
        assert n.child0() == b0
        assert n.child1() == b1

    def test_jump_rule(self):
        """BDD reduction: lo == hi => return lo."""
        v1 = kyotodd.new_var()
        b0 = kyotodd.BDD(0)
        n = kyotodd.BDD.getnode(v1, b0, b0)
        assert n == b0  # jump rule applied


class TestBDDChildAccessors:
    def test_child0_child1(self):
        v1 = kyotodd.new_var()
        b0 = kyotodd.BDD(0)
        b1 = kyotodd.BDD(1)
        n = kyotodd.BDD.getnode(v1, b0, b1)
        assert n.child0() == b0
        assert n.child1() == b1

    def test_child_by_index(self):
        v1 = kyotodd.new_var()
        b0 = kyotodd.BDD(0)
        b1 = kyotodd.BDD(1)
        n = kyotodd.BDD.getnode(v1, b0, b1)
        assert n.child(0) == b0
        assert n.child(1) == b1

    def test_raw_child(self):
        v1 = kyotodd.new_var()
        b0 = kyotodd.BDD(0)
        b1 = kyotodd.BDD(1)
        n = kyotodd.BDD.getnode(v1, b0, b1)
        assert n.raw_child0() == b0
        assert n.raw_child1() == b1
        assert n.raw_child(0) == b0
        assert n.raw_child(1) == b1


class TestBDDTerminalProperties:
    def test_is_terminal(self):
        assert kyotodd.BDD(0).is_terminal
        assert kyotodd.BDD(1).is_terminal

    def test_is_not_terminal(self):
        v1 = kyotodd.new_var()
        n = kyotodd.BDD.var(v1)
        assert not n.is_terminal

    def test_is_zero(self):
        assert kyotodd.BDD(0).is_zero
        assert not kyotodd.BDD(1).is_zero

    def test_is_one(self):
        assert kyotodd.BDD(1).is_one
        assert not kyotodd.BDD(0).is_one


class TestBDDSharedSize:
    def test_single(self):
        v1 = kyotodd.new_var()
        b = kyotodd.BDD.var(v1)
        assert kyotodd.BDD.shared_size([b]) == 1

    def test_multiple(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        b1 = kyotodd.BDD.var(v1)
        b2 = kyotodd.BDD.var(v2)
        size = kyotodd.BDD.shared_size([b1, b2])
        assert size == 2

    def test_shared_plain_size(self):
        v1 = kyotodd.new_var()
        b = kyotodd.BDD.var(v1)
        assert kyotodd.BDD.shared_plain_size([b]) >= 1


class TestBDDBinaryIO:
    def test_roundtrip_str(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        b = kyotodd.BDD.var(v1) & kyotodd.BDD.var(v2)
        data = b.export_binary_str()
        assert isinstance(data, bytes)
        b2 = kyotodd.BDD.import_binary_str(data)
        assert b == b2

    def test_roundtrip_file(self, tmp_path):
        v1 = kyotodd.new_var()
        b = kyotodd.BDD.var(v1)
        path = str(tmp_path / "test.bdd")
        b.export_binary_file(path)
        b2 = kyotodd.BDD.import_binary_file(path)
        assert b == b2

    def test_terminal_roundtrip(self):
        for val in [0, 1]:
            b = kyotodd.BDD(val)
            data = b.export_binary_str()
            b2 = kyotodd.BDD.import_binary_str(data)
            assert b == b2


class TestBDDBinaryMultiIO:
    def test_roundtrip_str(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        bdds = [kyotodd.BDD.var(v1), kyotodd.BDD.var(v2),
                kyotodd.BDD.var(v1) & kyotodd.BDD.var(v2)]
        data = kyotodd.BDD.export_binary_multi_str(bdds)
        assert isinstance(data, bytes)
        bdds2 = kyotodd.BDD.import_binary_multi_str(data)
        assert len(bdds2) == len(bdds)
        for a, b in zip(bdds, bdds2):
            assert a == b

    def test_roundtrip_file(self, tmp_path):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        bdds = [kyotodd.BDD.var(v1), kyotodd.BDD.var(v2)]
        path = str(tmp_path / "multi.bdd")
        kyotodd.BDD.export_binary_multi_file(bdds, path)
        bdds2 = kyotodd.BDD.import_binary_multi_file(path)
        assert len(bdds2) == 2
        assert bdds[0] == bdds2[0]
        assert bdds[1] == bdds2[1]

    def test_empty_list(self):
        data = kyotodd.BDD.export_binary_multi_str([])
        bdds = kyotodd.BDD.import_binary_multi_str(data)
        assert bdds == []


class TestBDDSapporoIO:
    def test_roundtrip_str(self):
        v1 = kyotodd.new_var()
        v2 = kyotodd.new_var()
        b = kyotodd.BDD.var(v1) | kyotodd.BDD.var(v2)
        s = b.export_sapporo_str()
        assert isinstance(s, str)
        b2 = kyotodd.BDD.import_sapporo_str(s)
        assert b == b2

    def test_roundtrip_file(self, tmp_path):
        v1 = kyotodd.new_var()
        b = kyotodd.BDD.var(v1)
        path = str(tmp_path / "test.sapporo")
        b.export_sapporo_file(path)
        b2 = kyotodd.BDD.import_sapporo_file(path)
        assert b == b2


class TestBDDGraphviz:
    def test_graphviz_str(self):
        v1 = kyotodd.new_var()
        b = kyotodd.BDD.var(v1)
        dot = b.save_graphviz_str()
        assert "digraph" in dot

    def test_graphviz_raw(self):
        v1 = kyotodd.new_var()
        b = kyotodd.BDD.var(v1)
        dot = b.save_graphviz_str(raw=True)
        assert "digraph" in dot

    def test_graphviz_file(self, tmp_path):
        v1 = kyotodd.new_var()
        b = kyotodd.BDD.var(v1)
        path = str(tmp_path / "test.dot")
        b.save_graphviz_file(path)
        with open(path) as f:
            content = f.read()
        assert "digraph" in content

    def test_graphviz_file_raw(self, tmp_path):
        v1 = kyotodd.new_var()
        b = kyotodd.BDD.var(v1)
        path = str(tmp_path / "test_raw.dot")
        b.save_graphviz_file(path, raw=True)
        with open(path) as f:
            content = f.read()
        assert "digraph" in content
