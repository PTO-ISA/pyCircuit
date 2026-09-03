"""AXI4 protocol definitions for XiangShan-pyc.

Defines port helpers for AXI4 five channels (AW/W/B/AR/R).
Used at the SoC boundary (XSTop outward interface).
"""

from __future__ import annotations

from pycircuit import CycleAwareCircuit

# ---------------------------------------------------------------------------
# AXI4 default widths (typical SoC configuration)
# ---------------------------------------------------------------------------

AXI_ADDR_WIDTH = 36
AXI_DATA_WIDTH = 256  # 32 bytes per beat
AXI_STRB_WIDTH = 32  # DATA_WIDTH / 8
AXI_ID_WIDTH = 8
AXI_LEN_WIDTH = 8  # burst length (0-255, actual = LEN+1)
AXI_SIZE_WIDTH = 3  # 2^SIZE bytes per transfer
AXI_BURST_WIDTH = 2  # FIXED=0, INCR=1, WRAP=2
AXI_LOCK_WIDTH = 1
AXI_CACHE_WIDTH = 4
AXI_PROT_WIDTH = 3
AXI_QOS_WIDTH = 4
AXI_RESP_WIDTH = 2  # OKAY=0, EXOKAY=1, SLVERR=2, DECERR=3
AXI_USER_WIDTH = 1

# Response codes
AXI_RESP_OKAY = 0
AXI_RESP_EXOKAY = 1
AXI_RESP_SLVERR = 2
AXI_RESP_DECERR = 3

# Burst types
AXI_BURST_FIXED = 0
AXI_BURST_INCR = 1
AXI_BURST_WRAP = 2


# ---------------------------------------------------------------------------
# Port helpers
# ---------------------------------------------------------------------------


def axi4_aw_ports(
    m: CycleAwareCircuit,
    prefix: str,
    *,
    direction: str = "output",
    addr_w: int = AXI_ADDR_WIDTH,
    id_w: int = AXI_ID_WIDTH,
):
    """Declare AXI4 Write Address (AW) channel ports."""
    io = m.input if direction == "input" else m.output
    return {
        "valid": io(f"{prefix}aw_valid", width=1),
        "id": io(f"{prefix}aw_id", width=id_w),
        "addr": io(f"{prefix}aw_addr", width=addr_w),
        "len": io(f"{prefix}aw_len", width=AXI_LEN_WIDTH),
        "size": io(f"{prefix}aw_size", width=AXI_SIZE_WIDTH),
        "burst": io(f"{prefix}aw_burst", width=AXI_BURST_WIDTH),
        "lock": io(f"{prefix}aw_lock", width=AXI_LOCK_WIDTH),
        "cache": io(f"{prefix}aw_cache", width=AXI_CACHE_WIDTH),
        "prot": io(f"{prefix}aw_prot", width=AXI_PROT_WIDTH),
        "qos": io(f"{prefix}aw_qos", width=AXI_QOS_WIDTH),
    }


def axi4_w_ports(
    m: CycleAwareCircuit,
    prefix: str,
    *,
    direction: str = "output",
    data_w: int = AXI_DATA_WIDTH,
):
    """Declare AXI4 Write Data (W) channel ports."""
    strb_w = data_w // 8
    io = m.input if direction == "input" else m.output
    return {
        "valid": io(f"{prefix}w_valid", width=1),
        "data": io(f"{prefix}w_data", width=data_w),
        "strb": io(f"{prefix}w_strb", width=strb_w),
        "last": io(f"{prefix}w_last", width=1),
    }


def axi4_b_ports(
    m: CycleAwareCircuit,
    prefix: str,
    *,
    direction: str = "input",
    id_w: int = AXI_ID_WIDTH,
):
    """Declare AXI4 Write Response (B) channel ports."""
    io = m.input if direction == "input" else m.output
    return {
        "valid": io(f"{prefix}b_valid", width=1),
        "id": io(f"{prefix}b_id", width=id_w),
        "resp": io(f"{prefix}b_resp", width=AXI_RESP_WIDTH),
    }


def axi4_ar_ports(
    m: CycleAwareCircuit,
    prefix: str,
    *,
    direction: str = "output",
    addr_w: int = AXI_ADDR_WIDTH,
    id_w: int = AXI_ID_WIDTH,
):
    """Declare AXI4 Read Address (AR) channel ports."""
    io = m.input if direction == "input" else m.output
    return {
        "valid": io(f"{prefix}ar_valid", width=1),
        "id": io(f"{prefix}ar_id", width=id_w),
        "addr": io(f"{prefix}ar_addr", width=addr_w),
        "len": io(f"{prefix}ar_len", width=AXI_LEN_WIDTH),
        "size": io(f"{prefix}ar_size", width=AXI_SIZE_WIDTH),
        "burst": io(f"{prefix}ar_burst", width=AXI_BURST_WIDTH),
        "lock": io(f"{prefix}ar_lock", width=AXI_LOCK_WIDTH),
        "cache": io(f"{prefix}ar_cache", width=AXI_CACHE_WIDTH),
        "prot": io(f"{prefix}ar_prot", width=AXI_PROT_WIDTH),
        "qos": io(f"{prefix}ar_qos", width=AXI_QOS_WIDTH),
    }


def axi4_r_ports(
    m: CycleAwareCircuit,
    prefix: str,
    *,
    direction: str = "input",
    data_w: int = AXI_DATA_WIDTH,
    id_w: int = AXI_ID_WIDTH,
):
    """Declare AXI4 Read Data (R) channel ports."""
    io = m.input if direction == "input" else m.output
    return {
        "valid": io(f"{prefix}r_valid", width=1),
        "id": io(f"{prefix}r_id", width=id_w),
        "data": io(f"{prefix}r_data", width=data_w),
        "resp": io(f"{prefix}r_resp", width=AXI_RESP_WIDTH),
        "last": io(f"{prefix}r_last", width=1),
    }
