"""Blocking bus and four-state adapter boundaries; no ISA arithmetic oracle."""

import pytest
from pycircuit._driver import _DriverError
from pycircuit._memory_run import PendingMemory, pack, unpack


def test_completion_waits_and_reset_cancels_without_a_transaction():
    memory = PendingMemory([4, 0])
    request = (0x20, 1, 0x3, 0xABCD)
    assert not memory.response(request, 0)
    memory.finish(request, False, 0)
    assert not memory.response(request, 3)
    assert memory.response(request, 4)
    with pytest.raises(_DriverError, match="changed"):
        memory.finish((0x24, 1, 0x3, 0xABCD), False, 3)
    memory.finish(request, True, 4)
    assert memory.count == 1
    assert memory.response(request, 5)
    memory.finish(request, True, 5)
    assert memory.count == 2
    memory.finish(request, False, 6)
    memory.finish(None, False, 7, cancel=True)
    assert memory.count == 2 and memory.pending is None


def test_unknown_output_is_not_zero_or_a_successful_event():
    port = {"width": 5, "fields": [{"path": ["value"], "width": 5, "low": 0}]}
    assert pack({"value": 17}, port) == "10001"
    assert unpack("10001", port) == {"value": 17}
    with pytest.raises(_DriverError, match="unknown"):
        unpack("1000x", port)
    with pytest.raises(_DriverError, match="width"):
        pack({"value": 32}, port)
