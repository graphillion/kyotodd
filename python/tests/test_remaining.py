"""Tests for remaining unimplemented Python bindings:
QDD binary I/O, UnreducedDD binary I/O, PiDD /= %= print enum enum2,
RotPiDD print enum enum2 contradictionMaximization,
SeqBDD export_to print_seq, module-level new_vars gc_rootcount top_level.
"""
import kyotodd
from kyotodd import (
    BDD, ZDD, QDD, UnreducedDD, PiDD, RotPiDD, SeqBDD,
)


# ================================================================
# Module-level functions
# ================================================================

class TestNewVars:
    def test_basic(self):
        vars = kyotodd.new_vars(3)
        assert len(vars) == 3
        assert vars[0] < vars[1] < vars[2]

    def test_zero(self):
        vars = kyotodd.new_vars(0)
        assert vars == []

    def test_reverse(self):
        vars = kyotodd.new_vars(3, reverse=True)
        assert len(vars) == 3
        # In reverse mode, all vars are created at level 1
        # so levels are assigned differently
        for v in vars:
            assert v > 0


class TestGcRootcount:
    def test_returns_int(self):
        count = kyotodd.gc_rootcount()
        assert isinstance(count, int)
        assert count >= 0


class TestTopLevel:
    def test_basic(self):
        kyotodd.new_var()
        lev = kyotodd.top_level()
        assert lev >= 1


# ================================================================
# QDD binary I/O
# ================================================================

class TestQDDBinaryIO:
    def test_roundtrip_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        b = BDD.var(1) & BDD.var(2)
        q = b.to_qdd()
        data = q.export_binary_str()
        assert isinstance(data, bytes)
        q2 = QDD.import_binary_str(data)
        assert q == q2

    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        b = BDD.var(1)
        q = b.to_qdd()
        path = str(tmp_path / "test.qdd")
        q.export_binary_file(path)
        q2 = QDD.import_binary_file(path)
        assert q == q2

    def test_terminal_roundtrip(self):
        for val in [0, 1]:
            q = QDD(val)
            data = q.export_binary_str()
            q2 = QDD.import_binary_str(data)
            assert q == q2


class TestQDDBinaryMultiIO:
    def test_roundtrip_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        qdds = [BDD.var(1).to_qdd(), BDD.var(2).to_qdd(),
                (BDD.var(1) & BDD.var(2)).to_qdd()]
        data = QDD.export_binary_multi_str(qdds)
        assert isinstance(data, bytes)
        qdds2 = QDD.import_binary_multi_str(data)
        assert len(qdds2) == len(qdds)
        for a, b in zip(qdds, qdds2):
            assert a == b

    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        kyotodd.new_var()
        qdds = [BDD.var(1).to_qdd(), BDD.var(2).to_qdd()]
        path = str(tmp_path / "multi.qdd")
        QDD.export_binary_multi_file(qdds, path)
        qdds2 = QDD.import_binary_multi_file(path)
        assert len(qdds2) == 2
        assert qdds[0] == qdds2[0]
        assert qdds[1] == qdds2[1]

    def test_empty_list(self):
        data = QDD.export_binary_multi_str([])
        qdds = QDD.import_binary_multi_str(data)
        assert qdds == []


# ================================================================
# UnreducedDD binary I/O
# ================================================================

class TestUnreducedDDBinaryIO:
    def test_roundtrip_str(self):
        kyotodd.new_var()
        u = UnreducedDD.getnode(1, UnreducedDD(0), UnreducedDD(1))
        data = u.export_binary_str()
        assert isinstance(data, bytes)
        u2 = UnreducedDD.import_binary_str(data)
        # UnreducedDD nodes are not unique-tabled, so IDs differ.
        # Verify structure by reducing both and comparing.
        assert u.reduce_as_bdd() == u2.reduce_as_bdd()

    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        u = UnreducedDD.getnode(1, UnreducedDD(0), UnreducedDD(1))
        path = str(tmp_path / "test.udd")
        u.export_binary_file(path)
        u2 = UnreducedDD.import_binary_file(path)
        assert u.reduce_as_bdd() == u2.reduce_as_bdd()

    def test_terminal_roundtrip(self):
        for val in [0, 1]:
            u = UnreducedDD(val)
            data = u.export_binary_str()
            u2 = UnreducedDD.import_binary_str(data)
            assert u == u2  # terminals have the same ID


# ================================================================
# PiDD: /=, %=, print, enum, enum2
# ================================================================

class TestPiDDInplaceDivMod:
    def test_itruediv(self):
        kyotodd.pidd_newvar()
        kyotodd.pidd_newvar()
        p = PiDD(1)
        p1 = p.swap(1, 2)
        both = p + p1
        expected = both / p
        both /= p
        assert both == expected

    def test_imod(self):
        kyotodd.pidd_newvar()
        kyotodd.pidd_newvar()
        p = PiDD(1)
        p1 = p.swap(1, 2)
        both = p + p1
        expected = both % p
        both %= p
        assert both == expected


class TestPiDDPrint:
    def test_print(self):
        kyotodd.pidd_newvar()
        kyotodd.pidd_newvar()
        p = PiDD(1).swap(1, 2)
        s = p.print()
        assert isinstance(s, str)
        assert len(s) > 0


class TestPiDDEnum:
    def test_enum(self):
        kyotodd.pidd_newvar()
        kyotodd.pidd_newvar()
        p = PiDD(1).swap(1, 2)
        s = p.enum()
        assert isinstance(s, str)
        assert "2" in s and "1" in s

    def test_enum2(self):
        kyotodd.pidd_newvar()
        kyotodd.pidd_newvar()
        p = PiDD(1).swap(1, 2)
        s = p.enum2()
        assert isinstance(s, str)
        assert len(s) > 0


# ================================================================
# RotPiDD: print, enum, enum2, contradiction_maximization
# ================================================================

class TestRotPiDDPrint:
    def test_print(self):
        kyotodd.rotpidd_newvar()
        kyotodd.rotpidd_newvar()
        r = RotPiDD(1).left_rot(2, 1)
        s = r.print()
        assert isinstance(s, str)
        assert len(s) > 0


class TestRotPiDDEnum:
    def test_enum(self):
        kyotodd.rotpidd_newvar()
        kyotodd.rotpidd_newvar()
        r = RotPiDD(1).left_rot(2, 1)
        s = r.enum()
        assert isinstance(s, str)
        assert "2" in s

    def test_enum2(self):
        kyotodd.rotpidd_newvar()
        kyotodd.rotpidd_newvar()
        r = RotPiDD(1).left_rot(2, 1)
        s = r.enum2()
        assert isinstance(s, str)
        assert len(s) > 0


class TestRotPiDDContradictionMaximization:
    def test_identity_zero(self):
        kyotodd.rotpidd_newvar()
        kyotodd.rotpidd_newvar()
        kyotodd.rotpidd_newvar()
        # Identity permutation with identity weight matrix: contradiction = 0
        r = RotPiDD(1)
        w = [[0]*4 for _ in range(4)]
        w[1][1] = 1; w[2][2] = 1; w[3][3] = 1
        result = r.contradiction_maximization(3, w)
        assert result == 0

    def test_all_perms(self):
        kyotodd.rotpidd_newvar()
        kyotodd.rotpidd_newvar()
        kyotodd.rotpidd_newvar()
        # All permutations of S3
        all_perms = RotPiDD(1)
        for perm in [[2,1,3],[1,3,2],[3,2,1],[2,3,1],[3,1,2]]:
            all_perms += kyotodd.rotpidd_from_perm(perm)
        # Weight: prefer position i to have value i
        w = [[0]*4 for _ in range(4)]
        w[1][1] = 1; w[2][2] = 1; w[3][3] = 1
        result = all_perms.contradiction_maximization(3, w)
        assert isinstance(result, int)


# ================================================================
# SeqBDD: export_to, print_seq
# ================================================================

class TestSeqBDDExport:
    def test_export_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        s = SeqBDD.from_list([1, 2])
        text = s.export_str()
        assert isinstance(text, str)
        assert len(text) > 0

    def test_export_file(self, tmp_path):
        kyotodd.new_var()
        kyotodd.new_var()
        s = SeqBDD.from_list([1, 2])
        path = str(tmp_path / "test.seq")
        s.export_file(path)
        with open(path) as f:
            content = f.read()
        assert len(content) > 0


class TestSeqBDDPrintSeq:
    def test_print_seq(self):
        kyotodd.new_var()
        kyotodd.new_var()
        s = SeqBDD.from_list([1, 2])
        text = s.print_seq()
        assert isinstance(text, str)
        assert "1" in text
        assert "2" in text

    def test_print_seq_multiple(self):
        kyotodd.new_var()
        kyotodd.new_var()
        s1 = SeqBDD.from_list([1, 2])
        s2 = SeqBDD.from_list([2, 1])
        u = s1 + s2
        text = u.print_seq()
        assert isinstance(text, str)
        # Output contains both sequences in some format
        assert "1" in text
        assert "2" in text
