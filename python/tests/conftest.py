import gc

import pytest
import kyotodd


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item, call):
    """Release traceback frames after reporting so that BDD/ZDD objects
    captured in exception frames can be garbage-collected."""
    outcome = yield
    if call.excinfo is not None:
        tb = call.excinfo.tb
        while tb is not None:
            try:
                tb.tb_frame.clear()
            except RuntimeError:
                pass
            tb = tb.tb_next
        call.excinfo = None


@pytest.fixture(autouse=True)
def reset_bdd():
    """Re-initialize the BDD library before each test."""
    gc.collect()
    try:
        kyotodd.finalize()
    except RuntimeError:
        pass
    kyotodd.init(1024)
    yield
