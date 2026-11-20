# ruff: noqa: N802, F823, F841 -- captured hardware state and System names.
"""Literal vectors crossing the low/high register boundary, including faults."""

import pycircuit as ac
from wse_calendar.route import DecodeEntry, RouteRegisters


@ac.struct
class Scenario:
    lo: ac.u64
    hi: ac.u64
    node0: ac.u2
    node31: ac.u2
    node32: ac.u2
    node39: ac.u2
    land_count: ac.u8
    flags: ac.u8
    expected_bytes: ac.u32


def vectors() -> ac.table[4, Scenario]:
    rows: ac.table[4, Scenario] = (
        Scenario(
            lo=0,
            hi=0,
            node0=0,
            node31=0,
            node32=0,
            node39=0,
            land_count=0,
            flags=0,
            expected_bytes=0,
        ),
        # Published Calendar §2.2.1 route bytes AA 03 20 00 E0 ...;
        # independently chosen metadata exercises all tail boundaries.
        Scenario(
            lo=0xE0002003AA,
            hi=0x1234567805020000,
            node0=2,
            node31=0,
            node32=0,
            node39=0,
            land_count=2,
            flags=5,
            expected_bytes=0x12345678,
        ),
        Scenario(
            lo=0xC000000000000002,
            hi=0x0000100003078003,
            node0=2,
            node31=3,
            node32=3,
            node39=2,
            land_count=7,
            flags=3,
            expected_bytes=4096,
        ),
        Scenario(
            lo=0x4000000000000001,
            hi=0x0000000000004001,
            node0=1,
            node31=1,
            node32=1,
            node39=1,
            land_count=0,
            flags=0,
            expected_bytes=0,
        ),
    )
    return rows


@ac.system
def RouteSystem():
    phase: ac.u2 = 0
    rows = vectors()
    row = rows[phase]
    entry = RouteRegisters(lo=row.lo, hi=row.hi)
    n0 = DecodeEntry(entry, NODE=0)
    n31 = DecodeEntry(entry, NODE=31)
    n32 = DecodeEntry(entry, NODE=32)
    n39 = DecodeEntry(entry, NODE=39)

    # ndf: verifies=WSEMIG-ROUTE-VER-001
    @ac.rule
    def check() -> None:
        assert n0.pair == row.node0, "node 0 pair"
        assert n31.pair == row.node31, "node 31 pair"
        assert n32.pair == row.node32, "node 32 pair"
        assert n39.pair == row.node39, "node 39 pair"
        assert n0.land_count == row.land_count, "land count"
        assert n0.flags == row.flags, "flags"
        assert n0.expected_bytes == row.expected_bytes, "expected bytes"
        assert n0.fault == (row.node0 == 1), "node 0 illegal pair"
        assert n31.fault == (row.node31 == 1), "node 31 illegal pair"
        assert n32.fault == (row.node32 == 1), "node 32 illegal pair"
        assert n39.fault == (row.node39 == 1), "node 39 illegal pair"
        ac.log("info", "route_checked", phase)
        phase = phase + 1
