import pytest
import kyotodd
from kyotodd import BDD, ZDD, QDD, UnreducedDD, SeqBDD, BddCountMemo, ZddCountMemo


class TestBDDExportImportStr:
    def test_roundtrip_false(self):
        s = BDD.false_.export_str()
        assert BDD.import_str(s) == BDD.false_

    def test_roundtrip_true(self):
        s = BDD.true_.export_str()
        assert BDD.import_str(s) == BDD.true_

    def test_roundtrip_var(self):
        kyotodd.new_var()
        x = BDD.var(1)
        s = x.export_str()
        result = BDD.import_str(s)
        assert result == x

    def test_roundtrip_complex(self):
        kyotodd.new_var()
        kyotodd.new_var()
        kyotodd.new_var()
        x, y, z = BDD.var(1), BDD.var(2), BDD.var(3)
        f = (x & y) | (~x & z)
        s = f.export_str()
        result = BDD.import_str(s)
        assert result == f

    def test_export_str_not_empty(self):
        kyotodd.new_var()
        x = BDD.var(1)
        s = x.export_str()
        assert len(s) > 0


class TestBDDExportImportFile:
    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        kyotodd.new_var()
        x, y = BDD.var(1), BDD.var(2)
        f = x & y
        path = str(tmp_path / "bdd.txt")
        f.export_file(path)
        result = BDD.import_file(path)
        assert result == f

    def test_import_nonexistent_file(self):
        with pytest.raises(RuntimeError):
            BDD.import_file("/nonexistent/path/file.txt")


class TestZDDExportImportStr:
    def test_roundtrip_empty(self):
        s = ZDD.empty.export_str()
        assert ZDD.import_str(s) == ZDD.empty

    def test_roundtrip_single(self):
        s = ZDD.single.export_str()
        assert ZDD.import_str(s) == ZDD.single

    def test_roundtrip_family(self):
        kyotodd.new_var()
        kyotodd.new_var()
        a = ZDD(1).change(1)  # {{1}}
        b = ZDD(1).change(2)  # {{2}}
        f = a + b  # {{1}, {2}}
        s = f.export_str()
        result = ZDD.import_str(s)
        assert result == f

    def test_roundtrip_complex(self):
        kyotodd.new_var()
        kyotodd.new_var()
        kyotodd.new_var()
        a = ZDD(1).change(1)
        b = ZDD(1).change(2)
        c = ZDD(1).change(3)
        f = (a * b) + c  # {{1,2}, {3}}
        s = f.export_str()
        result = ZDD.import_str(s)
        assert result == f


class TestZDDExportImportFile:
    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        kyotodd.new_var()
        a = ZDD(1).change(1)
        b = ZDD(1).change(2)
        f = a + b
        path = str(tmp_path / "zdd.txt")
        f.export_file(path)
        result = ZDD.import_file(path)
        assert result == f

    def test_import_nonexistent_file(self):
        with pytest.raises(RuntimeError):
            ZDD.import_file("/nonexistent/path/file.txt")


class TestBDDKnuthIO:
    def test_roundtrip_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        x, y = BDD.var(1), BDD.var(2)
        f = x & y
        s = f.export_knuth_str()
        assert len(s) > 0
        result = BDD.import_knuth_str(s)
        assert result == f

    def test_roundtrip_str_hex(self):
        kyotodd.new_var()
        x = BDD.var(1)
        s = x.export_knuth_str(is_hex=True)
        result = BDD.import_knuth_str(s, is_hex=True)
        assert result == x

    def test_roundtrip_str_terminal(self):
        s = BDD.false_.export_knuth_str()
        assert BDD.import_knuth_str(s) == BDD.false_
        s = BDD.true_.export_knuth_str()
        assert BDD.import_knuth_str(s) == BDD.true_

    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        kyotodd.new_var()
        x, y = BDD.var(1), BDD.var(2)
        f = x | y
        path = str(tmp_path / "bdd_knuth.txt")
        f.export_knuth_file(path)
        result = BDD.import_knuth_file(path)
        assert result == f

    def test_import_nonexistent_file(self):
        with pytest.raises(RuntimeError):
            BDD.import_knuth_file("/nonexistent/path/file.txt")


class TestZDDKnuthIO:
    def test_roundtrip_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        a = ZDD(1).change(1)
        b = ZDD(1).change(2)
        f = a + b
        s = f.export_knuth_str()
        assert len(s) > 0
        result = ZDD.import_knuth_str(s)
        assert result == f

    def test_roundtrip_str_hex(self):
        kyotodd.new_var()
        a = ZDD(1).change(1)
        s = a.export_knuth_str(is_hex=True)
        result = ZDD.import_knuth_str(s, is_hex=True)
        assert result == a

    def test_roundtrip_str_terminal(self):
        s = ZDD.empty.export_knuth_str()
        assert ZDD.import_knuth_str(s) == ZDD.empty
        s = ZDD.single.export_knuth_str()
        assert ZDD.import_knuth_str(s) == ZDD.single

    def test_roundtrip_file(self, tmp_path):
        kyotodd.new_var()
        kyotodd.new_var()
        a = ZDD(1).change(1)
        b = ZDD(1).change(2)
        f = a + b
        path = str(tmp_path / "zdd_knuth.txt")
        f.export_knuth_file(path)
        result = ZDD.import_knuth_file(path)
        assert result == f

    def test_import_nonexistent_file(self):
        with pytest.raises(RuntimeError):
            ZDD.import_knuth_file("/nonexistent/path/file.txt")


class TestBDDPrint:
    def test_print_returns_string(self):
        kyotodd.new_var()
        x = BDD.var(1)
        s = x.print()
        assert isinstance(s, str)
        assert len(s) > 0

    def test_print_terminal(self):
        s = BDD.true_.print()
        assert isinstance(s, str)


class TestZDDPrint:
    def test_print_returns_string(self):
        kyotodd.new_var()
        a = ZDD(1).change(1)
        s = a.print()
        assert isinstance(s, str)
        assert len(s) > 0

    def test_print_terminal(self):
        s = ZDD.single.print()
        assert isinstance(s, str)


class TestUnreducedDDZeroOne:
    def test_zero(self):
        z = UnreducedDD.zero()
        assert z == UnreducedDD(0)
        assert z.is_zero

    def test_one(self):
        o = UnreducedDD.one()
        assert o == UnreducedDD(1)
        assert o.is_one


class TestSeqBDDSeqStr:
    def test_seq_str_equals_str(self):
        kyotodd.new_var()
        kyotodd.new_var()
        s = SeqBDD.from_list([1, 2])
        assert s.seq_str() == str(s)

    def test_seq_str_empty(self):
        s = SeqBDD(0)
        result = s.seq_str()
        assert isinstance(result, str)


class TestFinalizeResetsPiDDGlobals:
    def test_pidd_var_used_reset(self):
        kyotodd.init()
        kyotodd.pidd_newvar()
        assert kyotodd.pidd_var_used() > 0
        kyotodd.finalize()
        kyotodd.init()
        assert kyotodd.pidd_var_used() == 0

    def test_rotpidd_var_used_reset(self):
        kyotodd.init()
        kyotodd.rotpidd_newvar()
        assert kyotodd.rotpidd_var_used() > 0
        kyotodd.finalize()
        kyotodd.init()
        assert kyotodd.rotpidd_var_used() == 0


class TestBDDExportImportMultiStr:
    def test_roundtrip(self):
        kyotodd.new_vars(3)
        a, b, c = BDD.var(1), BDD.var(2), BDD.var(3)
        f = a & b
        bdds = [a, f, c]
        s = BDD.export_multi_str(bdds)
        result = BDD.import_multi_str(s)
        assert len(result) == 3
        assert result[0] == a
        assert result[1] == f
        assert result[2] == c

    def test_single(self):
        kyotodd.new_var()
        x = BDD.var(1)
        s = BDD.export_multi_str([x])
        result = BDD.import_multi_str(s)
        assert len(result) == 1
        assert result[0] == x

    def test_empty(self):
        s = BDD.export_multi_str([])
        result = BDD.import_multi_str(s)
        assert len(result) == 0

    def test_terminals(self):
        bdds = [BDD.false_, BDD.true_]
        s = BDD.export_multi_str(bdds)
        result = BDD.import_multi_str(s)
        assert len(result) == 2
        assert result[0] == BDD.false_
        assert result[1] == BDD.true_


class TestBDDExportImportMultiFile:
    def test_roundtrip(self, tmp_path):
        kyotodd.new_vars(2)
        a, b = BDD.var(1), BDD.var(2)
        f = a | b
        path = str(tmp_path / "bdd_multi.txt")
        BDD.export_multi_file([a, b, f], path)
        result = BDD.import_multi_file(path)
        assert len(result) == 3
        assert result[0] == a
        assert result[1] == b
        assert result[2] == f


class TestZDDExportImportMultiStr:
    def test_roundtrip(self):
        kyotodd.new_vars(3)
        x = ZDD.singleton(1)
        y = ZDD.singleton(2)
        z = x + y
        s = ZDD.export_multi_str([x, y, z])
        result = ZDD.import_multi_str(s)
        assert len(result) == 3
        assert result[0] == x
        assert result[1] == y
        assert result[2] == z

    def test_single(self):
        kyotodd.new_var()
        x = ZDD.singleton(1)
        s = ZDD.export_multi_str([x])
        result = ZDD.import_multi_str(s)
        assert len(result) == 1
        assert result[0] == x

    def test_empty(self):
        s = ZDD.export_multi_str([])
        result = ZDD.import_multi_str(s)
        assert len(result) == 0

    def test_terminals(self):
        zdds = [ZDD.empty, ZDD.single]
        s = ZDD.export_multi_str(zdds)
        result = ZDD.import_multi_str(s)
        assert len(result) == 2
        assert result[0] == ZDD.empty
        assert result[1] == ZDD.single


class TestZDDExportImportMultiFile:
    def test_roundtrip(self, tmp_path):
        kyotodd.new_vars(2)
        x = ZDD.singleton(1)
        y = ZDD.singleton(2)
        z = x + y
        path = str(tmp_path / "zdd_multi.txt")
        ZDD.export_multi_file([x, y, z], path)
        result = ZDD.import_multi_file(path)
        assert len(result) == 3
        assert result[0] == x
        assert result[1] == y
        assert result[2] == z
