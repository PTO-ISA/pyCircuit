"""XiangShan KunMingHu default parameters.

Extracted from:
  - designs/XiangShan/src/main/scala/xiangshan/Parameters.scala
  - designs/XiangShan/src/main/scala/top/Configs.scala
  - designs/XiangShan/src/main/scala/xiangshan/frontend/FrontendParameters.scala

All values are plain Python constants / dicts — no hardware constructs.
"""

from __future__ import annotations

import math

# ---------------------------------------------------------------------------
# ISA configuration
# ---------------------------------------------------------------------------

XLEN = 64
VLEN = 128
ELEN = 64
HSXLEN = 64

HAS_M_EXTENSION = True
HAS_C_EXTENSION = True
HAS_H_EXTENSION = True
HAS_FPU = True
HAS_VPU = True
HAS_DIV = True
HAS_DCACHE = True

# ---------------------------------------------------------------------------
# Address space
# ---------------------------------------------------------------------------

ADDR_BITS = 64
PADDR_BITS_MAX = 56
VADDR_BITS_SV39 = 39
VADDR_BITS_SV48 = 48
CACHE_LINE_SIZE = 512  # bits (64 bytes)
CACHE_LINE_BYTES = 64

# ---------------------------------------------------------------------------
# Pipeline widths
# ---------------------------------------------------------------------------

DECODE_WIDTH = 8
RENAME_WIDTH = 8
COMMIT_WIDTH = 8
ROB_COMMIT_WIDTH = 8
RAB_COMMIT_WIDTH = 8
MAX_UOP_SIZE = 65

# ---------------------------------------------------------------------------
# Frontend
# ---------------------------------------------------------------------------

FETCH_BLOCK_SIZE = 64  # bytes
FETCH_PORTS = 2
INST_BYTES = 2  # minimum instruction size (RVC)
FETCH_BLOCK_INST_NUM = FETCH_BLOCK_SIZE // INST_BYTES  # 32

# BPU parameters
BPU_S0_S1_S2_S3_STAGES = 4
PREDICT_WIDTH = FETCH_BLOCK_SIZE // INST_BYTES  # 32 (max instructions per fetch block)
CFI_POSITION_WIDTH = max(1, (PREDICT_WIDTH - 1).bit_length())  # 5

# uBTB (Micro BTB) parameters
UBTB_NUM_ENTRIES = 32
UBTB_TAG_WIDTH = 22
UBTB_TARGET_WIDTH = 22
UBTB_USEFUL_CNT_WIDTH = 2

# Branch attribute encoding widths
BRANCH_TYPE_WIDTH = 2  # None/Conditional/Direct/Indirect
RAS_ACTION_WIDTH = 2  # None/Pop/Push/PopAndPush

# TAGE tables: (size, ways, history_length)
TAGE_TABLE_INFOS = [
    (4096, 2, 6),
    (4096, 2, 12),
    (4096, 2, 17),
    (4096, 2, 31),
    (4096, 2, 44),
    (4096, 2, 65),
    (4096, 2, 90),
    (4096, 2, 130),
]

# ITTAGE tables: (size, history_length)
ITTAGE_TABLE_INFOS = [
    (256, 4),
    (256, 8),
    (512, 16),
    (512, 32),
]
ITTAGE_TAG_WIDTH = 9

# RAS
RAS_COMMIT_STACK_SIZE = 32
RAS_SPEC_QUEUE_SIZE = 64

# FTQ
FTQ_SIZE = 64

# ICache: sets x ways x blockBytes = total
ICACHE_SETS = 256
ICACHE_WAYS = 4
ICACHE_BLOCK_BYTES = 64
ICACHE_TOTAL_KB = (ICACHE_SETS * ICACHE_WAYS * ICACHE_BLOCK_BYTES) // 1024  # 64 KB

# IBuffer
IBUFFER_SIZE = 48
IBUFFER_NUM_WRITE_BANK = 4

# IFU
IFU_REDIRECT_NUM = 1

# ---------------------------------------------------------------------------
# Backend — register file
# ---------------------------------------------------------------------------

INT_LOGIC_REGS = 32
FP_LOGIC_REGS = 34  # 32 + 1 (I2F) + 1 (stride)
VEC_LOGIC_REGS = 47  # 32 + 15 (tmp)
V0_LOGIC_REGS = 1
VL_LOGIC_REGS = 1

INT_PHYS_REGS = 224  # intPreg.numEntries
INT_PREG_BANKS = 4
FP_PHYS_REGS = 256  # fpPreg.numEntries
VF_PHYS_REGS = 128  # vfPreg.numEntries
V0_PHYS_REGS = 22
VL_PHYS_REGS = 32

INT_REG_CACHE_SIZE = 24
MEM_REG_CACHE_SIZE = 12
REG_CACHE_SIZE = INT_REG_CACHE_SIZE + MEM_REG_CACHE_SIZE

# ---------------------------------------------------------------------------
# Backend — scheduler / issue
# ---------------------------------------------------------------------------

ISSUE_QUEUE_SIZE = 20
ISSUE_QUEUE_COMP_ENTRY_SIZE = 12
RENAME_SNAPSHOT_NUM = 4

# ROB
ROB_SIZE = 352
RAB_SIZE = 352
VTYPE_BUFFER_SIZE = 64

# ---------------------------------------------------------------------------
# Memory subsystem
# ---------------------------------------------------------------------------

LOAD_PIPELINE_WIDTH = 3
STORE_PIPELINE_WIDTH = 2
VEC_LOAD_PIPELINE_WIDTH = 2
VEC_STORE_PIPELINE_WIDTH = 2

# Load queue
VIRTUAL_LOAD_QUEUE_SIZE = 72
LOAD_QUEUE_RAR_SIZE = 72
LOAD_QUEUE_RAW_SIZE = 32
LOAD_QUEUE_REPLAY_SIZE = 72
LOAD_UNCACHE_BUFFER_SIZE = 16
LOAD_QUEUE_N_WRITE_BANKS = 8
ROLLBACK_GROUP_SIZE = 8

# Store queue
STORE_QUEUE_SIZE = 56
SQ_UNALIGN_QUEUE_SIZE = 2
STORE_QUEUE_N_WRITE_BANKS = 8

# Store buffer
STORE_BUFFER_SIZE = 16
STORE_BUFFER_THRESHOLD = 7
ENSBUFFER_WIDTH = 2

LOAD_DEPENDENCY_WIDTH = 2

# ---------------------------------------------------------------------------
# DCache
# ---------------------------------------------------------------------------

DCACHE_SETS = 256
DCACHE_WAYS = 8
DCACHE_BLOCK_BYTES = 64
DCACHE_TOTAL_KB = (DCACHE_SETS * DCACHE_WAYS * DCACHE_BLOCK_BYTES) // 1024  # 128 KB
DCACHE_N_MISS_ENTRIES = 16
DCACHE_N_PROBE_ENTRIES = 8
DCACHE_N_RELEASE_ENTRIES = 18
DCACHE_N_MAX_PREFETCH_ENTRY = 6

# ---------------------------------------------------------------------------
# TLB
# ---------------------------------------------------------------------------

ITLB_WAYS = 48
LDTLB_WAYS = 48
STTLB_WAYS = 48
HYTLB_WAYS = 48

ASID_LENGTH = 16
VMID_LENGTH = 14

# ---------------------------------------------------------------------------
# L2 Cache (CoupledL2)
# ---------------------------------------------------------------------------

L2_WAYS = 8
L2_SETS = 1024
L2_BLOCK_BYTES = 64
L2_TOTAL_KB = (L2_WAYS * L2_SETS * L2_BLOCK_BYTES) // 1024  # 512 KB
L2_N_BANKS = 1

# ---------------------------------------------------------------------------
# Derived widths
# ---------------------------------------------------------------------------

PTAG_WIDTH_INT = math.ceil(math.log2(INT_PHYS_REGS))  # 8
PTAG_WIDTH_FP = math.ceil(math.log2(FP_PHYS_REGS))  # 8
PTAG_WIDTH_VF = math.ceil(math.log2(VF_PHYS_REGS))  # 7
ROB_IDX_WIDTH = math.ceil(math.log2(ROB_SIZE))  # 9
FTQ_IDX_WIDTH = math.ceil(math.log2(FTQ_SIZE))  # 6
LQ_IDX_WIDTH = math.ceil(math.log2(VIRTUAL_LOAD_QUEUE_SIZE))  # 7
SQ_IDX_WIDTH = math.ceil(math.log2(STORE_QUEUE_SIZE))  # 6
VL_WIDTH = math.ceil(math.log2(VLEN)) + 1  # 8

PC_WIDTH = VADDR_BITS_SV39  # 39 bits

# Number of ALUs / execution units
NUM_ALU = 6
NUM_BRU = 3  # BJU0, BJU1, BJU2
NUM_MUL = 2
NUM_DIV = 1  # shared with ALU1
NUM_LDU = 3
NUM_STA = 2
NUM_STD = 2
NUM_FMAC = 3
NUM_FDIV = 2
