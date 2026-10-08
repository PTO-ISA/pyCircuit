from history_features.reset_invalidate_order_smoke import ResetInvalidateOrder
from history_features.trace_dsl_smoke import TraceDsl
from history_features.xz_value_model_smoke import XzValueModel
from pycircuit import log, rule, system, u8


@rule
def advance(phase):
    phase = phase + 1


@system
def ResetInvalidateOrderSystem():
    phase: u8 = 0
    dut = ResetInvalidateOrder()

    @rule
    def drive():
        dut(en=1)
        assert dut.y == (phase if phase < 2 else 2), "counter old Q"
        log("info", "counter", dut.y)

    drive()
    advance(phase)


@system
def TraceDslSystem():
    phase: u8 = 0
    dut = TraceDsl()

    @rule
    def drive():
        dut(in_x=0x12 if phase == 0 else 0x34)
        assert dut.y0 == (
            0 if phase == 0 else (0x12 if phase == 1 else 0x34)
        ), "first leaf old Q"
        assert dut.y1 == (
            0 if phase == 0 else (0x12 if phase == 1 else 0x34)
        ), "second leaf old Q"
        log("info", "trace", dut.y0, dut.y1)

    drive()
    advance(phase)


@system
def XzValueModelSystem():
    phase: u8 = 0
    dut = XzValueModel()

    @rule
    def drive():
        dut(in_a=0x12 if phase == 0 else 0x56)
        assert dut.y == (
            0 if phase == 0 else (0x12 if phase == 1 else 0x56)
        ), "capture old Q"
        log("info", "capture", dut.y)

    drive()
    advance(phase)
