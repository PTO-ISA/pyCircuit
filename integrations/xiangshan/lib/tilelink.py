"""TileLink protocol definitions for XiangShan-pyc.

Defines channel port helpers and protocol constants for TileLink-UH/UL.
These are used for core-to-L2 and L2-to-L3 interconnects.
"""

from __future__ import annotations

from pycircuit import CycleAwareCircuit

# ---------------------------------------------------------------------------
# TileLink protocol constants
# ---------------------------------------------------------------------------

# Channel A opcodes
TL_A_OPCODE_GET = 4
TL_A_OPCODE_PUT_FULL = 0
TL_A_OPCODE_PUT_PARTIAL = 1
TL_A_OPCODE_ARITHMETIC = 2
TL_A_OPCODE_LOGICAL = 3
TL_A_OPCODE_INTENT = 5
TL_A_OPCODE_ACQUIRE_BLOCK = 6
TL_A_OPCODE_ACQUIRE_PERM = 7

# Channel B opcodes
TL_B_OPCODE_PROBE_BLOCK = 6
TL_B_OPCODE_PROBE_PERM = 7

# Channel C opcodes
TL_C_OPCODE_PROBE_ACK = 4
TL_C_OPCODE_PROBE_ACK_DATA = 5
TL_C_OPCODE_RELEASE = 6
TL_C_OPCODE_RELEASE_DATA = 7

# Channel D opcodes
TL_D_OPCODE_ACCESS_ACK = 0
TL_D_OPCODE_ACCESS_ACK_DATA = 1
TL_D_OPCODE_HINT_ACK = 2
TL_D_OPCODE_GRANT = 4
TL_D_OPCODE_GRANT_DATA = 5
TL_D_OPCODE_RELEASE_ACK = 6

# Channel E — no opcode, just sink ID

# Permission params (Cap)
TL_PARAM_TO_T = 0  # toT — Trunk (exclusive read-write)
TL_PARAM_TO_B = 1  # toB — Branch (shared read-only)
TL_PARAM_TO_N = 2  # toN — Nothing (invalid)

# Grow params
TL_PARAM_GROW_NTO_B = 0
TL_PARAM_GROW_NTO_T = 1
TL_PARAM_GROW_BTO_T = 2

# Shrink params
TL_PARAM_SHRINK_TTO_B = 0
TL_PARAM_SHRINK_TTO_N = 1
TL_PARAM_SHRINK_BTO_N = 2

# Report params
TL_PARAM_REPORT_TTO_T = 3
TL_PARAM_REPORT_BTO_B = 4
TL_PARAM_REPORT_NTO_N = 5

# Default widths (KunMingHu / CoupledL2 configuration)
TL_OPCODE_WIDTH = 3
TL_PARAM_WIDTH = 3
TL_SIZE_WIDTH = 4  # log2(max transfer size in bytes) — up to 64B cache line
TL_SOURCE_WIDTH = 7  # source ID width (configurable)
TL_SINK_WIDTH = 7  # sink ID width (configurable)
TL_ADDRESS_WIDTH = 36  # physical address width
TL_DATA_WIDTH = 256  # 32 bytes per beat (256 bits)
TL_MASK_WIDTH = 32  # byte mask = DATA_WIDTH / 8


# ---------------------------------------------------------------------------
# Port definition helpers
# ---------------------------------------------------------------------------


def tl_a_ports(
    m: CycleAwareCircuit,
    prefix: str,
    *,
    direction: str = "input",
    addr_w: int = TL_ADDRESS_WIDTH,
    data_w: int = TL_DATA_WIDTH,
    source_w: int = TL_SOURCE_WIDTH,
):
    """Declare TileLink channel A ports. Returns dict of wires."""
    mask_w = data_w // 8
    io = m.input if direction == "input" else m.output
    return {
        "valid": io(f"{prefix}a_valid", width=1),
        "ready": (
            (m.output if direction == "input" else m.input)(f"{prefix}a_ready", width=1)
            if False
            else None
        ),
        "opcode": io(f"{prefix}a_opcode", width=TL_OPCODE_WIDTH),
        "param": io(f"{prefix}a_param", width=TL_PARAM_WIDTH),
        "size": io(f"{prefix}a_size", width=TL_SIZE_WIDTH),
        "source": io(f"{prefix}a_source", width=source_w),
        "address": io(f"{prefix}a_address", width=addr_w),
        "mask": io(f"{prefix}a_mask", width=mask_w),
        "data": io(f"{prefix}a_data", width=data_w),
        "corrupt": io(f"{prefix}a_corrupt", width=1),
    }


def tl_b_ports(
    m: CycleAwareCircuit,
    prefix: str,
    *,
    direction: str = "input",
    addr_w: int = TL_ADDRESS_WIDTH,
    data_w: int = TL_DATA_WIDTH,
    source_w: int = TL_SOURCE_WIDTH,
):
    """Declare TileLink channel B ports."""
    mask_w = data_w // 8
    io = m.input if direction == "input" else m.output
    return {
        "valid": io(f"{prefix}b_valid", width=1),
        "opcode": io(f"{prefix}b_opcode", width=TL_OPCODE_WIDTH),
        "param": io(f"{prefix}b_param", width=TL_PARAM_WIDTH),
        "size": io(f"{prefix}b_size", width=TL_SIZE_WIDTH),
        "source": io(f"{prefix}b_source", width=source_w),
        "address": io(f"{prefix}b_address", width=addr_w),
        "mask": io(f"{prefix}b_mask", width=mask_w),
        "data": io(f"{prefix}b_data", width=data_w),
        "corrupt": io(f"{prefix}b_corrupt", width=1),
    }


def tl_c_ports(
    m: CycleAwareCircuit,
    prefix: str,
    *,
    direction: str = "input",
    addr_w: int = TL_ADDRESS_WIDTH,
    data_w: int = TL_DATA_WIDTH,
    source_w: int = TL_SOURCE_WIDTH,
):
    """Declare TileLink channel C ports."""
    io = m.input if direction == "input" else m.output
    return {
        "valid": io(f"{prefix}c_valid", width=1),
        "opcode": io(f"{prefix}c_opcode", width=TL_OPCODE_WIDTH),
        "param": io(f"{prefix}c_param", width=TL_PARAM_WIDTH),
        "size": io(f"{prefix}c_size", width=TL_SIZE_WIDTH),
        "source": io(f"{prefix}c_source", width=source_w),
        "address": io(f"{prefix}c_address", width=addr_w),
        "data": io(f"{prefix}c_data", width=data_w),
        "corrupt": io(f"{prefix}c_corrupt", width=1),
    }


def tl_d_ports(
    m: CycleAwareCircuit,
    prefix: str,
    *,
    direction: str = "input",
    data_w: int = TL_DATA_WIDTH,
    source_w: int = TL_SOURCE_WIDTH,
    sink_w: int = TL_SINK_WIDTH,
):
    """Declare TileLink channel D ports."""
    io = m.input if direction == "input" else m.output
    return {
        "valid": io(f"{prefix}d_valid", width=1),
        "opcode": io(f"{prefix}d_opcode", width=TL_OPCODE_WIDTH),
        "param": io(f"{prefix}d_param", width=TL_PARAM_WIDTH),
        "size": io(f"{prefix}d_size", width=TL_SIZE_WIDTH),
        "source": io(f"{prefix}d_source", width=source_w),
        "sink": io(f"{prefix}d_sink", width=sink_w),
        "denied": io(f"{prefix}d_denied", width=1),
        "data": io(f"{prefix}d_data", width=data_w),
        "corrupt": io(f"{prefix}d_corrupt", width=1),
    }


def tl_e_ports(
    m: CycleAwareCircuit,
    prefix: str,
    *,
    direction: str = "input",
    sink_w: int = TL_SINK_WIDTH,
):
    """Declare TileLink channel E ports."""
    io = m.input if direction == "input" else m.output
    return {
        "valid": io(f"{prefix}e_valid", width=1),
        "sink": io(f"{prefix}e_sink", width=sink_w),
    }
