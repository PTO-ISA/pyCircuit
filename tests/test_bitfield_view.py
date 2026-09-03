"""Gate for single-signal bitfield views (ASL alignment TODO T1).

``BitfieldSpec`` is pure front-end sugar over ``slice``/``cat``: a named-field
read must emit byte-identical MLIR to a hand-written ``signal[lsb:msb+1]`` and a
read-modify-write ``update`` must match a hand-written ``cat(...)``. Fields may
overlap (alternate views of one register, matching ASL's overlapping bitfields).
Both plain ``Wire`` and ``CycleAwareSignal`` inputs are supported; the result
keeps the input's type and (for cycle-aware) its cycle tag.
"""

from __future__ import annotations

import pytest
from pycircuit import (
    BitfieldSignal,
    BitfieldSpec,
    BitfieldView,
    Circuit,
    CycleAwareCircuit,
    cas,
    wire_of,
)

# Scalar (domain-free) concat: the top-level ``cat`` is the cycle-aware variant
# (rejects all-scalar operands), so the hand-written scalar references below use
# the ``hw`` module's ``cat`` directly.
from pycircuit.hw import cat

INSTR = BitfieldSpec(
    width=32,
    fields={
        "opcode": (31, 26),
        "rd": (25, 21),
        "rs": (20, 16),
        "imm16": (15, 0),
        "imm26": (25, 0),  # overlaps rd/rs/imm16 -- allowed (alternate view)
    },
)


# --- spec-level validation --------------------------------------------------


def test_overlapping_fields_are_allowed() -> None:
    # imm26 overlaps rd/rs/imm16; construction must not raise.
    assert INSTR.field_slices()["imm26"] == (0, 26)
    assert INSTR.field_slices()["opcode"] == (26, 6)
    assert INSTR.field_width("rd") == 5


@pytest.mark.parametrize(
    "fields, match",
    [
        ({"a": (32, 0)}, "out of range"),
        ({"a": (3, 5)}, "msb >= lsb"),
        ({"a": (5, -1)}, "lsb must be >= 0"),
        ({}, "at least one field"),
    ],
)
def test_bad_field_defs_raise(fields, match) -> None:
    with pytest.raises(ValueError, match=match):
        BitfieldSpec(width=32, fields=fields)


def test_zero_width_raises() -> None:
    with pytest.raises(ValueError, match="width must be > 0"):
        BitfieldSpec(width=0, fields={"a": (0, 0)})


# --- Wire read == hand-written slice (byte-level MLIR equivalence) ----------


def _wire_read(field: str) -> str:
    m = Circuit("t")
    instr = m.input("instr", width=32)
    m.output("y", INSTR.view(instr)[field])
    return m.emit_mlir()


def _wire_manual_slice(msb: int, lsb: int) -> str:
    m = Circuit("t")
    instr = m.input("instr", width=32)
    m.output("y", instr[lsb : msb + 1])
    return m.emit_mlir()


@pytest.mark.parametrize(
    "field, msb, lsb",
    [("opcode", 31, 26), ("rd", 25, 21), ("imm16", 15, 0), ("imm26", 25, 0)],
)
def test_field_read_matches_manual_slice(field, msb, lsb) -> None:
    assert _wire_read(field) == _wire_manual_slice(msb, lsb)


def test_multi_field_read_matches_manual_cat() -> None:
    def via_spec() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        m.output("y", INSTR.view(instr)["opcode", "rd"])
        return m.emit_mlir()

    def manual() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        m.output("y", cat(instr[26:32], instr[21:26]))
        return m.emit_mlir()

    assert via_spec() == manual()


def test_callable_shorthand_matches_view() -> None:
    def via_call() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        m.output("y", INSTR(instr).opcode)
        return m.emit_mlir()

    assert via_call() == _wire_manual_slice(31, 26)


def test_attribute_read_matches_item_read() -> None:
    def via_attr() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        m.output("y", INSTR.view(instr).opcode)
        return m.emit_mlir()

    assert via_attr() == _wire_manual_slice(31, 26)


# --- Wire update == hand-written cat (byte-level MLIR equivalence) ----------


def test_update_one_field_matches_manual_cat() -> None:
    def via_spec() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        rd = m.input("rd", width=5)
        m.output("y", INSTR.update(instr, rd=rd))
        return m.emit_mlir()

    def manual() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        rd = m.input("rd", width=5)
        m.output("y", cat(instr[26:32], rd, instr[0:21]))
        return m.emit_mlir()

    assert via_spec() == manual()


def test_update_two_disjoint_fields_matches_manual_cat() -> None:
    def via_spec() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        rd = m.input("rd", width=5)
        rs = m.input("rs", width=5)
        m.output("y", INSTR.update(instr, rd=rd, rs=rs))
        return m.emit_mlir()

    def manual() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        rd = m.input("rd", width=5)
        rs = m.input("rs", width=5)
        # MSB-first: [31:26] gap, rd[25:21], rs[20:16], [15:0] gap
        m.output("y", cat(instr[26:32], rd, rs, instr[0:16]))
        return m.emit_mlir()

    assert via_spec() == manual()


def test_update_lsb_field_has_no_trailing_gap() -> None:
    def via_spec() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        imm = m.input("imm", width=16)
        m.output("y", INSTR.update(instr, imm16=imm))
        return m.emit_mlir()

    def manual() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        imm = m.input("imm", width=16)
        m.output("y", cat(instr[16:32], imm))
        return m.emit_mlir()

    assert via_spec() == manual()


# --- update error handling --------------------------------------------------


def test_update_overlapping_writes_raise() -> None:
    m = Circuit("t")
    instr = m.input("instr", width=32)
    rd = m.input("rd", width=5)
    imm = m.input("imm", width=26)
    with pytest.raises(ValueError, match="overlap"):
        INSTR.update(instr, rd=rd, imm26=imm)


def test_update_wrong_field_width_raises() -> None:
    m = Circuit("t")
    instr = m.input("instr", width=32)
    bad = m.input("bad", width=6)  # rd is 5 bits
    with pytest.raises(ValueError, match="width"):
        INSTR.update(instr, rd=bad)


def test_update_constant_out_of_range_raises() -> None:
    m = Circuit("t")
    instr = m.input("instr", width=32)
    with pytest.raises(ValueError, match="does not fit"):
        INSTR.update(instr, rd=32)  # 5-bit field, max 31


def test_update_constant_in_range_ok() -> None:
    m = Circuit("t")
    instr = m.input("instr", width=32)
    out = INSTR.update(instr, rd=7)
    assert out.width == 32


def test_unknown_field_raises() -> None:
    m = Circuit("t")
    instr = m.input("instr", width=32)
    with pytest.raises(KeyError, match="unknown bitfield"):
        INSTR.view(instr)["nope"]


def test_width_mismatch_raises() -> None:
    m = Circuit("t")
    short = m.input("short", width=16)
    with pytest.raises(ValueError, match="does not match"):
        INSTR.view(short)


def test_view_is_read_only() -> None:
    m = Circuit("t")
    instr = m.input("instr", width=32)
    f = INSTR.view(instr)
    with pytest.raises(AttributeError):
        f.opcode = 1  # type: ignore[misc]


def test_view_mapping_protocol() -> None:
    m = Circuit("t")
    instr = m.input("instr", width=32)
    f = INSTR.view(instr)
    assert isinstance(f, BitfieldView)
    assert set(f.keys()) == {"opcode", "rd", "rs", "imm16", "imm26"}
    assert "rd" in f
    assert "nope" not in f
    assert {name for name, _ in f.items()} == set(f.keys())


# --- cycle-aware support ----------------------------------------------------


def test_cas_view_preserves_type_and_cycle() -> None:
    m = CycleAwareCircuit("t")
    d = m.create_domain("clk")
    instr = cas(d, m.input("instr", width=32))
    f = INSTR.view(instr)
    op = f["opcode"]
    assert op.cycle == instr.cycle
    assert op._w.width == 6


def test_cas_update_preserves_type_and_cycle() -> None:
    m = CycleAwareCircuit("t")
    d = m.create_domain("clk")
    instr = cas(d, m.input("instr", width=32))
    rd = cas(d, m.input("rd", width=5))
    out = INSTR.update(instr, rd=rd)
    assert out.cycle == instr.cycle
    assert out._w.width == 32


def test_cas_update_cross_cycle_field_raises() -> None:
    m = CycleAwareCircuit("t")
    d = m.create_domain("clk")
    instr = cas(d, m.input("instr", width=32))
    d.next()
    rd_later = cas(d, m.input("rd", width=5))  # lives in a later cycle
    with pytest.raises(ValueError, match="cycle"):
        INSTR.update(instr, rd=rd_later)


# --- bound signal (BitfieldSignal) ------------------------------------------


def test_bind_read_matches_manual_slice() -> None:
    def via_bound() -> str:
        m = Circuit("t")
        instr = INSTR.bind(m.input("instr", width=32))
        m.output("y", instr["opcode"])
        return m.emit_mlir()

    assert via_bound() == _wire_manual_slice(31, 26)


def test_bind_attribute_read_matches_manual_slice() -> None:
    def via_attr() -> str:
        m = Circuit("t")
        instr = INSTR.bind(m.input("instr", width=32))
        m.output("y", instr.rd)
        return m.emit_mlir()

    assert via_attr() == _wire_manual_slice(25, 21)


def test_bind_multi_field_read_matches_manual_cat() -> None:
    def via_bound() -> str:
        m = Circuit("t")
        instr = INSTR.bind(m.input("instr", width=32))
        m.output("y", instr["opcode", "rd"])
        return m.emit_mlir()

    def manual() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        m.output("y", cat(instr[26:32], instr[21:26]))
        return m.emit_mlir()

    assert via_bound() == manual()


def test_bind_int_slice_still_bit_slices() -> None:
    def via_bound() -> str:
        m = Circuit("t")
        instr = INSTR.bind(m.input("instr", width=32))
        m.output("y", instr[0:8])
        return m.emit_mlir()

    def manual() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        m.output("y", instr[0:8])
        return m.emit_mlir()

    assert via_bound() == manual()


def test_bind_arithmetic_delegates() -> None:
    def via_bound() -> str:
        m = Circuit("t")
        instr = INSTR.bind(m.input("instr", width=32))
        m.output("y", instr + 1)
        return m.emit_mlir()

    def manual() -> str:
        m = Circuit("t")
        instr = m.input("instr", width=32)
        m.output("y", instr + 1)
        return m.emit_mlir()

    assert via_bound() == manual()


def test_bind_update_stays_bound() -> None:
    m = Circuit("t")
    instr = INSTR.bind(m.input("instr", width=32))
    rd = m.input("rd", width=5)
    out = instr.update(rd=rd)
    assert isinstance(out, BitfieldSignal)
    assert out.width == 32
    # the rebuilt value still exposes fields
    assert out["rd"].width == 5


def test_bind_width_mismatch_raises() -> None:
    m = Circuit("t")
    with pytest.raises(ValueError, match="does not match"):
        INSTR.bind(m.input("short", width=16))


def test_bind_is_read_only() -> None:
    m = Circuit("t")
    instr = INSTR.bind(m.input("instr", width=32))
    with pytest.raises(AttributeError):
        instr.opcode = 1  # type: ignore[misc]


def test_bind_exposes_raw_and_spec() -> None:
    m = Circuit("t")
    w = m.input("instr", width=32)
    instr = INSTR.bind(w)
    assert instr.raw is w
    assert instr.spec is INSTR


# --- declaration-time binding via factories ---------------------------------


def test_input_fields_infers_width_and_reads() -> None:
    def via_input() -> str:
        m = Circuit("t")
        instr = m.input("instr", fields=INSTR)  # width inferred from spec
        assert isinstance(instr, BitfieldSignal)
        m.output("y", instr["opcode"])
        return m.emit_mlir()

    assert via_input() == _wire_manual_slice(31, 26)


def test_input_fields_width_conflict_raises() -> None:
    m = Circuit("t")
    with pytest.raises(ValueError, match="does not match"):
        m.input("instr", width=16, fields=INSTR)


def test_domain_signal_fields_binds_and_feeds_back() -> None:
    def counter(m, domain):
        cnt = domain.signal(name="cnt", fields=INSTR)  # width from spec
        cnt <<= cnt + 1
        m.output("opcode", wire_of(cnt["opcode"]))
        m.output("rd", wire_of(cnt.rd))

    from pycircuit import compile_cycle_aware

    mlir = compile_cycle_aware(counter, name="c", eager=True).emit_mlir()
    # feedback register + two field extracts, no crash
    assert mlir.count("pyc.reg") == 1
    assert "pyc.extract %" in mlir
    assert mlir.count("pyc.extract") == 2


def test_input_inline_dict_fields_binds_and_reads() -> None:
    def via_input() -> str:
        m = Circuit("t")
        instr = m.input(
            "instr",
            width=32,
            fields={"opcode": (31, 26), "rd": (25, 21)},
        )
        assert isinstance(instr, BitfieldSignal)
        m.output("y", instr["opcode"])
        return m.emit_mlir()

    assert via_input() == _wire_manual_slice(31, 26)


def test_domain_signal_inline_dict_fields() -> None:
    def counter(m, domain):
        cnt = domain.signal(
            name="cnt",
            width=32,
            fields={"opcode": (31, 26), "rd": (25, 21)},
        )
        assert isinstance(cnt, BitfieldSignal)
        cnt <<= cnt + 1
        m.output("opcode", wire_of(cnt["opcode"]))
        m.output("rd", wire_of(cnt.rd))

    from pycircuit import compile_cycle_aware

    mlir = compile_cycle_aware(counter, name="c", eager=True).emit_mlir()
    assert mlir.count("pyc.reg") == 1
    assert mlir.count("pyc.extract") == 2


def test_inline_dict_fields_requires_width() -> None:
    m = Circuit("t")
    with pytest.raises(TypeError, match="requires width"):
        m.input("instr", fields={"opcode": (31, 26)})


def test_output_accepts_wire_backed_bound_signal_directly() -> None:
    m = Circuit("t")
    instr = m.input("instr", fields=INSTR)
    # whole-register (wire-backed) bound signal is unwrapped by output()
    m.output("whole", instr)
    assert "func.return" in m.emit_mlir()
