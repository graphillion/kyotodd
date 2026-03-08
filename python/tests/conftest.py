import pytest
import kyotodd


@pytest.fixture(autouse=True)
def reset_bdd():
    """Re-initialize the BDD library before each test."""
    kyotodd.init(1024)
    yield
