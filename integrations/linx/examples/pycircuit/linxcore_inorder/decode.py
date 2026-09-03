from __future__ import annotations

from dataclasses import dataclass

from pycircuit import Circuit, Wire, function, unsigned
from pycircuit.hw import cat

from .isa import (
    OP_ADD,
    OP_ADDI,
    OP_ADDIW,
    OP_ADDTPC,
    OP_ADDW,
    OP_AND,
    OP_ANDI,
    OP_ANDIW,
    OP_ANDW,
    OP_B_FPATR,
    OP_B_IOR,
    OP_B_IOS,
    OP_B_IOT,
    OP_B_TEXT,
    OP_BSTART_STD_CALL,
    OP_BSTART_STD_COND,
    OP_BSTART_STD_DIRECT,
    OP_BSTART_STD_FALL,
    OP_BSTART_STD_ICALL,
    OP_BSTART_TMA,
    OP_BXS,
    OP_BXU,
    OP_C_ADD,
    OP_C_ADDI,
    OP_C_AND,
    OP_C_BSTART_COND,
    OP_C_BSTART_DIRECT,
    OP_C_BSTART_STD,
    OP_C_BSTOP,
    OP_C_LDI,
    OP_C_LWI,
    OP_C_MOVI,
    OP_C_MOVR,
    OP_C_OR,
    OP_C_SDI,
    OP_C_SETC_EQ,
    OP_C_SETC_NE,
    OP_C_SETC_TGT,
    OP_C_SETRET,
    OP_C_SEXT_W,
    OP_C_SUB,
    OP_C_SWI,
    OP_C_ZEXT_W,
    OP_CMP_ANDI,
    OP_CMP_EQ,
    OP_CMP_EQI,
    OP_CMP_GEI,
    OP_CMP_GEUI,
    OP_CMP_LT,
    OP_CMP_LTI,
    OP_CMP_LTU,
    OP_CMP_LTUI,
    OP_CMP_NE,
    OP_CMP_NEI,
    OP_CMP_ORI,
    OP_CSEL,
    OP_DIV,
    OP_DIVU,
    OP_DIVUW,
    OP_DIVW,
    OP_EBREAK,
    OP_FENTRY,
    OP_FEXIT,
    OP_FRET_RA,
    OP_FRET_STK,
    OP_HL_LB_PCR,
    OP_HL_LBU_PCR,
    OP_HL_LD_PCR,
    OP_HL_LH_PCR,
    OP_HL_LHU_PCR,
    OP_HL_LUI,
    OP_HL_LW_PCR,
    OP_HL_LWU_PCR,
    OP_HL_SB_PCR,
    OP_HL_SD_PCR,
    OP_HL_SH_PCR,
    OP_HL_SSRSET,
    OP_HL_SW_PCR,
    OP_INVALID,
    OP_LB,
    OP_LBI,
    OP_LBU,
    OP_LBUI,
    OP_LD,
    OP_LDI,
    OP_LH,
    OP_LHI,
    OP_LHU,
    OP_LHUI,
    OP_LUI,
    OP_LW,
    OP_LWI,
    OP_LWU,
    OP_LWUI,
    OP_MADD,
    OP_MADDW,
    OP_MCOPY,
    OP_MSET,
    OP_MUL,
    OP_MULW,
    OP_OR,
    OP_ORI,
    OP_ORIW,
    OP_ORW,
    OP_REM,
    OP_REMU,
    OP_REMUW,
    OP_REMW,
    OP_SB,
    OP_SBI,
    OP_SD,
    OP_SDI,
    OP_SETC_AND,
    OP_SETC_ANDI,
    OP_SETC_EQ,
    OP_SETC_EQI,
    OP_SETC_GE,
    OP_SETC_GEI,
    OP_SETC_GEU,
    OP_SETC_GEUI,
    OP_SETC_LT,
    OP_SETC_LTI,
    OP_SETC_LTU,
    OP_SETC_LTUI,
    OP_SETC_NE,
    OP_SETC_NEI,
    OP_SETC_OR,
    OP_SETC_ORI,
    OP_SETRET,
    OP_SH,
    OP_SHI,
    OP_SLL,
    OP_SLLI,
    OP_SLLIW,
    OP_SRA,
    OP_SRAIW,
    OP_SRL,
    OP_SRLIW,
    OP_SSRSET,
    OP_SUB,
    OP_SUBI,
    OP_SUBIW,
    OP_SUBW,
    OP_SW,
    OP_SWI,
    OP_XOR,
    OP_XORIW,
    OP_XORW,
    PTO_C_BSTART_STD_BRTYPES,
    PTO_CUBE_HEADER_MATCHES,
    PTO_FPATR_GROUP_N_CODES,
    PTO_FPATR_PREQUANT_MODES,
    PTO_FPATR_RELU_MODES,
    PTO_TEPL_SELECTORS,
    PTO_TLSU_HEADER_MATCHES,
    REG_INVALID,
)
from .util import masked_eq


@dataclass(frozen=True)
class Decode:
    op: Wire
    len_bytes: Wire
    regdst: Wire
    srcl: Wire
    srcr: Wire
    srcr_type: Wire
    shamt: Wire
    srcp: Wire
    imm: Wire


@function
def decode_window(m: Circuit, window: Wire) -> Decode:
    c = m.const
    zero3 = c(0, width=3)
    zero2 = c(0, width=2)
    zero6 = c(0, width=6)
    zero64 = c(0, width=64)
    reg_invalid = c(REG_INVALID, width=6)
    insn16 = window[0:16]
    insn32 = window[0:32]
    insn48 = window[0:48]
    low4 = insn16[0:4]
    is_hl = low4 == 14
    is32 = insn16[0]
    in32 = ~is_hl & is32
    in16 = ~is_hl & ~is32
    rd32 = insn32[7:12]
    rs1_32 = insn32[15:20]
    rs2_32 = insn32[20:25]
    srcr_type_32 = insn32[25:27]
    shamt5_32 = insn32[27:32]
    srcp_32 = insn32[27:32]
    shamt6_32 = insn32[20:26]
    imm12_u64 = insn32[20:32]
    imm12_s64 = insn32[20:32].as_signed()
    imm20_s64 = insn32[12:32].as_signed()
    imm20_u64 = unsigned(insn32[12:32])
    swi_lo5 = insn32[7:12]
    swi_hi7 = insn32[25:32]
    # Build a true 12-bit immediate; avoid narrow-width shift truncation.
    simm12_raw = cat(unsigned(swi_lo5), unsigned(swi_hi7))
    simm12_s64 = simm12_raw.as_signed()
    simm17_s64 = insn32[15:32].as_signed()
    pfx16 = insn48[0:16]
    main32 = insn48[16:48]
    imm_hi12 = pfx16[4:16]
    imm_lo20 = main32[12:32]
    imm32 = unsigned(imm_hi12).shl(amount=20) | unsigned(imm_lo20)
    imm_hl_lui = imm32.as_signed()
    rd_hl = main32[7:12]
    rd16 = insn16[11:16]
    rs16 = insn16[6:11]
    simm5_11_s64 = insn16[11:16].as_signed()
    simm5_6_s64 = insn16[6:11].as_signed()
    simm12_s64_c = insn16[4:16].as_signed()
    uimm5 = insn16[6:11]
    brtype = insn16[11:14]
    op = c(OP_INVALID, width=12)
    len_bytes = zero3
    regdst = reg_invalid
    srcl = reg_invalid
    srcr = reg_invalid
    srcr_type = zero2
    shamt = zero6
    srcp = reg_invalid
    imm = zero64
    cond = in16 & masked_eq(m, insn16, mask=63, match=12)
    if cond:
        op = OP_C_ADDI
        len_bytes = 2
        regdst = 31
        srcl = rs16
        imm = simm5_11_s64
    cond = in16 & masked_eq(m, insn16, mask=63, match=8)
    if cond:
        op = OP_C_ADD
        len_bytes = 2
        regdst = 31
        srcl = rs16
        srcr = rd16
    cond = in16 & masked_eq(m, insn16, mask=63, match=24)
    if cond:
        op = OP_C_SUB
        len_bytes = 2
        regdst = 31
        srcl = rs16
        srcr = rd16
    cond = in16 & masked_eq(m, insn16, mask=63, match=40)
    if cond:
        op = OP_C_AND
        len_bytes = 2
        regdst = 31
        srcl = rs16
        srcr = rd16
    cond = in16 & masked_eq(m, insn16, mask=63, match=22)
    if cond:
        op = OP_C_MOVI
        len_bytes = 2
        regdst = rd16
        imm = simm5_6_s64
    cond = in16 & masked_eq(m, insn16, mask=63551, match=20502)
    if cond:
        op = OP_C_SETRET
        len_bytes = 2
        regdst = 10
        imm = unsigned(uimm5).shl(amount=1)
    cond = in16 & masked_eq(m, insn16, mask=63, match=42)
    if cond:
        op = OP_C_SWI
        len_bytes = 2
        srcl = rs16
        srcr = 24
        imm = simm5_11_s64
    cond = in16 & masked_eq(m, insn16, mask=63, match=58)
    if cond:
        op = OP_C_SDI
        len_bytes = 2
        srcl = rs16
        srcr = 24
        imm = simm5_11_s64
    cond = in16 & masked_eq(m, insn16, mask=63, match=10)
    if cond:
        op = OP_C_LWI
        len_bytes = 2
        srcl = rs16
        imm = simm5_11_s64
    cond = in16 & masked_eq(m, insn16, mask=63, match=26)
    if cond:
        op = OP_C_LDI
        len_bytes = 2
        regdst = 31
        srcl = rs16
        imm = simm5_11_s64
    cond = in16 & masked_eq(m, insn16, mask=63, match=6)
    if cond:
        op = OP_C_MOVR
        len_bytes = 2
        regdst = rd16
        srcl = rs16
    cond = in16 & masked_eq(m, insn16, mask=63, match=38)
    if cond:
        op = OP_C_SETC_EQ
        len_bytes = 2
        srcl = rs16
        srcr = rd16
    cond = in16 & masked_eq(m, insn16, mask=63, match=54)
    if cond:
        op = OP_C_SETC_NE
        len_bytes = 2
        srcl = rs16
        srcr = rd16
    cond = in16 & masked_eq(m, insn16, mask=63551, match=28)
    if cond:
        op = OP_C_SETC_TGT
        len_bytes = 2
        srcl = rs16
    cond = in16 & masked_eq(m, insn16, mask=63, match=56)
    if cond:
        op = OP_C_OR
        len_bytes = 2
        regdst = 31
        srcl = rs16
        srcr = rd16
    cond = in16 & masked_eq(m, insn16, mask=63551, match=20508)
    if cond:
        op = OP_C_SEXT_W
        len_bytes = 2
        regdst = 31
        srcl = rs16
    cond = in16 & masked_eq(m, insn16, mask=63551, match=26652)
    if cond:
        op = OP_C_ZEXT_W
        len_bytes = 2
        regdst = 31
        srcl = rs16
    cond = in16 & masked_eq(m, insn16, mask=63551, match=44)
    if cond:
        op = OP_CMP_EQI
        len_bytes = 2
        regdst = 31
        srcl = 24
        imm = simm5_6_s64
    cond = in16 & masked_eq(m, insn16, mask=63551, match=2092)
    if cond:
        op = OP_CMP_NEI
        len_bytes = 2
        regdst = 31
        srcl = 24
        imm = simm5_6_s64
    cond = in16 & masked_eq(m, insn16, mask=15, match=2)
    if cond:
        op = OP_C_BSTART_DIRECT
        len_bytes = 2
        imm = simm12_s64_c.shl(amount=1)
    cond = in16 & masked_eq(m, insn16, mask=15, match=4)
    if cond:
        op = OP_C_BSTART_COND
        len_bytes = 2
        imm = simm12_s64_c.shl(amount=1)
    # C.BSTART.{SYS,MPAR,MSEQ,VPAR,VSEQ} FALL fixed forms.
    # pyCircuit models these as standard fall-through block starts.
    cond = in16 & (
        masked_eq(m, insn16, mask=0xFFFF, match=0x0840)
        | masked_eq(m, insn16, mask=0xFFFF, match=0x08C0)
        | masked_eq(m, insn16, mask=0xFFFF, match=0x48C0)
        | masked_eq(m, insn16, mask=0xFFFF, match=0x88C0)
        | masked_eq(m, insn16, mask=0xFFFF, match=0xC8C0)
    )
    if cond:
        op = OP_C_BSTART_STD
        len_bytes = 2
        imm = 0
    c_bstart_std_brtype_valid = brtype == PTO_C_BSTART_STD_BRTYPES[0]
    for selector in PTO_C_BSTART_STD_BRTYPES:
        c_bstart_std_brtype_valid = c_bstart_std_brtype_valid | (brtype == selector)
    cond = in16 & masked_eq(m, insn16, mask=51199, match=0) & c_bstart_std_brtype_valid
    if cond:
        op = OP_C_BSTART_STD
        len_bytes = 2
        imm = brtype
    cond = in16 & masked_eq(m, insn16, mask=65535, match=0)
    if cond:
        op = OP_C_BSTOP
        len_bytes = 2
    cond = in32 & masked_eq(m, insn32, mask=28799, match=65)
    if cond:
        op = OP_FENTRY
        len_bytes = 4
        srcl = insn32[15:20]
        srcr = insn32[20:25]
        uimm_hi = unsigned(insn32[7:12])
        uimm_lo = unsigned(insn32[25:32])
        imm = uimm_hi.shl(amount=10) | uimm_lo.shl(amount=3)
    cond = in32 & masked_eq(m, insn32, mask=28799, match=4161)
    if cond:
        op = OP_FEXIT
        len_bytes = 4
        srcl = insn32[15:20]
        srcr = insn32[20:25]
        uimm_hi = unsigned(insn32[7:12])
        uimm_lo = unsigned(insn32[25:32])
        imm = uimm_hi.shl(amount=10) | uimm_lo.shl(amount=3)
    cond = in32 & masked_eq(m, insn32, mask=28799, match=8257)
    if cond:
        op = OP_FRET_RA
        len_bytes = 4
        srcl = insn32[15:20]
        srcr = insn32[20:25]
        uimm_hi = unsigned(insn32[7:12])
        uimm_lo = unsigned(insn32[25:32])
        imm = uimm_hi.shl(amount=10) | uimm_lo.shl(amount=3)
    cond = in32 & masked_eq(m, insn32, mask=28799, match=12353)
    if cond:
        op = OP_FRET_STK
        len_bytes = 4
        srcl = insn32[15:20]
        srcr = insn32[20:25]
        uimm_hi = unsigned(insn32[7:12])
        uimm_lo = unsigned(insn32[25:32])
        imm = uimm_hi.shl(amount=10) | uimm_lo.shl(amount=3)
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=536870961)
    if cond:
        op = OP_MCOPY
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        imm = unsigned(srcp_32)
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=536875057)
    if cond:
        op = OP_MSET
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        imm = unsigned(srcp_32)
    # Decoupled/tile/vector headers.
    # PTO ISA 0.58 carries TEPL directly as Mode[1:0] + Function[4:0].
    # The pyc example core still reuses one decoupled-header control path
    # (OP_BSTART_TMA) for all public direct tile/vector header families.
    tepl_selector = unsigned(insn32[20:27])
    tepl_selector_valid = tepl_selector == PTO_TEPL_SELECTORS[0]
    for selector in PTO_TEPL_SELECTORS:
        tepl_selector_valid = tepl_selector_valid | (tepl_selector == selector)
    cond = (
        in32
        & masked_eq(m, insn32, mask=0x000FFFFF, match=0x00019181)
        & tepl_selector_valid
    )  # BSTART.TEPL
    if cond:
        op = OP_BSTART_TMA
        len_bytes = 4
    tile_header_valid = masked_eq(
        m, insn32, mask=0x07FFFFFF, match=PTO_TLSU_HEADER_MATCHES[0]
    )
    for header_match in PTO_TLSU_HEADER_MATCHES:
        tile_header_valid = tile_header_valid | masked_eq(
            m, insn32, mask=0x07FFFFFF, match=header_match
        )
    for header_match in PTO_CUBE_HEADER_MATCHES:
        tile_header_valid = tile_header_valid | masked_eq(
            m, insn32, mask=0x07FFFFFF, match=header_match
        )
    cond = in32 & tile_header_valid
    if cond:
        op = OP_BSTART_TMA
        len_bytes = 4
    cond = in32 & masked_eq(m, insn32, mask=0xFBFFFFFF, match=0x00001181)  # BSTART.MPAR
    if cond:
        op = OP_BSTART_TMA
        len_bytes = 4
    cond = in32 & masked_eq(m, insn32, mask=0xFBFFFFFF, match=0x00009181)  # BSTART.MSEQ
    if cond:
        op = OP_BSTART_TMA
        len_bytes = 4
    cond = in32 & masked_eq(m, insn32, mask=0xFBFFFFFF, match=0x00021181)  # BSTART.VPAR
    if cond:
        op = OP_BSTART_TMA
        len_bytes = 4
    cond = in32 & masked_eq(m, insn32, mask=0xFBFFFFFF, match=0x00029181)  # BSTART.VSEQ
    if cond:
        op = OP_BSTART_TMA
        len_bytes = 4
    # Decoupled body pointer: B.TEXT (simm25 in halfwords; target = PC + (simm25 << 1)).
    # QEMU metadata: mask=0x7f, match=0x03.
    cond = in32 & masked_eq(m, insn32, mask=0x0000007F, match=0x00000003)
    if cond:
        op = OP_B_TEXT
        len_bytes = 4
        imm = insn32[7:32].as_signed()
    cond = in32 & masked_eq(m, insn32, mask=0xF00871FF, match=0x00001013)
    if cond:
        op = OP_B_IOS
        len_bytes = 4
    cond = in32 & masked_eq(m, insn32, mask=0x0000707F, match=0x00004013)
    if cond:
        op = OP_B_IOT
        len_bytes = 4
    cond = in32 & masked_eq(m, insn32, mask=0xFC00707F, match=0x00005013)
    if cond:
        op = OP_B_IOT
        len_bytes = 4
    cond = in32 & masked_eq(m, insn32, mask=0xFFF0707F, match=0x00006013)
    if cond:
        op = OP_B_IOT
        len_bytes = 4
    cond = in32 & masked_eq(m, insn32, mask=0x0600707F, match=0x00000013)
    if cond:
        op = OP_B_IOR
        len_bytes = 4
    fpatr_prequant = unsigned(insn32[26:32])
    fpatr_prequant_valid = fpatr_prequant == PTO_FPATR_PREQUANT_MODES[0]
    for selector in PTO_FPATR_PREQUANT_MODES:
        fpatr_prequant_valid = fpatr_prequant_valid | (fpatr_prequant == selector)
    fpatr_relu = unsigned(insn32[23:26])
    fpatr_relu_valid = fpatr_relu == PTO_FPATR_RELU_MODES[0]
    for selector in PTO_FPATR_RELU_MODES:
        fpatr_relu_valid = fpatr_relu_valid | (fpatr_relu == selector)
    fpatr_group_n = unsigned(insn32[19:23])
    fpatr_group_n_valid = fpatr_group_n == PTO_FPATR_GROUP_N_CODES[0]
    for selector in PTO_FPATR_GROUP_N_CODES:
        fpatr_group_n_valid = fpatr_group_n_valid | (fpatr_group_n == selector)
    cond = (
        in32
        & masked_eq(m, insn32, mask=0x00007FFF, match=0x00002023)
        & fpatr_prequant_valid
        & fpatr_relu_valid
        & fpatr_group_n_valid
    )
    if cond:
        op = OP_B_FPATR
        len_bytes = 4
    cond = in32 & masked_eq(m, insn32, mask=127, match=23)
    if cond:
        op = OP_LUI
        len_bytes = 4
        regdst = rd32
        imm = imm20_s64.shl(amount=12)
    cond = in32 & masked_eq(m, insn32, mask=28799, match=5)
    if cond:
        op = OP_ADD
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=4101)
    if cond:
        op = OP_SUB
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=8197)
    if cond:
        op = OP_AND
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=12293)
    if cond:
        op = OP_OR
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=16389)
    if cond:
        op = OP_XOR
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=8213)
    if cond:
        op = OP_ANDI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=8245)
    if cond:
        op = OP_ANDIW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=12309)
    if cond:
        op = OP_ORI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=12341)
    if cond:
        op = OP_ORIW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=16437)
    if cond:
        op = OP_XORIW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=71)
    if cond:
        op = OP_MUL
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=8263)
    if cond:
        op = OP_MULW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=100692095, match=24647)
    if cond:
        op = OP_MADD
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcp = srcp_32
    cond = in32 & masked_eq(m, insn32, mask=100692095, match=28743)
    if cond:
        op = OP_MADDW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcp = srcp_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=87)
    if cond:
        op = OP_DIV
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=4183)
    if cond:
        op = OP_DIVU
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=8279)
    if cond:
        op = OP_DIVW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=12375)
    if cond:
        op = OP_DIVUW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=16471)
    if cond:
        op = OP_REM
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=20567)
    if cond:
        op = OP_REMU
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=24663)
    if cond:
        op = OP_REMW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=28759)
    if cond:
        op = OP_REMUW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=28677)
    if cond:
        op = OP_SLL
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=20485)
    if cond:
        op = OP_SRL
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=24581)
    if cond:
        op = OP_SRA
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4227887231, match=28693)
    if cond:
        op = OP_SLLI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        shamt = shamt6_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=20533)
    if cond:
        op = OP_SRLIW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        shamt = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=24629)
    if cond:
        op = OP_SRAIW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        shamt = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=4261441663, match=28725)
    if cond:
        op = OP_SLLIW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        shamt = rs2_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=103)
    if cond:
        op = OP_BXS
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = insn32[26:32]
        srcp = insn32[20:26]
    cond = in32 & masked_eq(m, insn32, mask=28799, match=4199)
    if cond:
        op = OP_BXU
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = insn32[26:32]
        srcp = insn32[20:26]
    cond = in32 & masked_eq(m, insn32, mask=4160778367, match=24645)
    if cond:
        op = OP_CMP_LTU
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=85)
    if cond:
        op = OP_CMP_EQI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=4181)
    if cond:
        op = OP_CMP_NEI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=8277)
    if cond:
        op = OP_CMP_ANDI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=12373)
    if cond:
        op = OP_CMP_ORI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=16469)
    if cond:
        op = OP_CMP_LTI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=20565)
    if cond:
        op = OP_CMP_GEI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=24661)
    if cond:
        op = OP_CMP_LTUI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_u64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=28757)
    if cond:
        op = OP_CMP_GEUI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_u64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=117)
    if cond:
        op = OP_SETC_EQI
        len_bytes = 4
        srcl = rs1_32
        shamt = rd32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=4213)
    if cond:
        op = OP_SETC_NEI
        len_bytes = 4
        srcl = rs1_32
        shamt = rd32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=8309)
    if cond:
        op = OP_SETC_ANDI
        len_bytes = 4
        srcl = rs1_32
        shamt = rd32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=12405)
    if cond:
        op = OP_SETC_ORI
        len_bytes = 4
        srcl = rs1_32
        shamt = rd32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=16501)
    if cond:
        op = OP_SETC_LTI
        len_bytes = 4
        srcl = rs1_32
        shamt = rd32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=20597)
    if cond:
        op = OP_SETC_GEI
        len_bytes = 4
        srcl = rs1_32
        shamt = rd32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=24693)
    if cond:
        op = OP_SETC_LTUI
        len_bytes = 4
        srcl = rs1_32
        shamt = rd32
        imm = imm12_u64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=28789)
    if cond:
        op = OP_SETC_GEUI
        len_bytes = 4
        srcl = rs1_32
        shamt = rd32
        imm = imm12_u64
    cond = in32 & masked_eq(m, insn32, mask=4160778367, match=101)
    if cond:
        op = OP_SETC_EQ
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
    cond = in32 & masked_eq(m, insn32, mask=4160778367, match=4197)
    if cond:
        op = OP_SETC_NE
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
    cond = in32 & masked_eq(m, insn32, mask=4160778367, match=8293)
    if cond:
        op = OP_SETC_AND
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
    cond = in32 & masked_eq(m, insn32, mask=4160778367, match=12389)
    if cond:
        op = OP_SETC_OR
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
    cond = in32 & masked_eq(m, insn32, mask=4160778367, match=16485)
    if cond:
        op = OP_SETC_LT
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
    cond = in32 & masked_eq(m, insn32, mask=4160778367, match=24677)
    if cond:
        op = OP_SETC_LTU
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
    cond = in32 & masked_eq(m, insn32, mask=4160778367, match=20581)
    if cond:
        op = OP_SETC_GE
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
    cond = in32 & masked_eq(m, insn32, mask=4160778367, match=28773)
    if cond:
        op = OP_SETC_GEU
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
    cond = in32 & masked_eq(m, insn32, mask=4293951487, match=16443)
    if cond:
        op = OP_C_SETC_TGT
        len_bytes = 4
        srcl = rs1_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=16409)
    if cond:
        op = OP_LBUI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=25)
    if cond:
        op = OP_LBI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=4121)
    if cond:
        op = OP_LHI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=20505)
    if cond:
        op = OP_LHUI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=24601)
    if cond:
        op = OP_LWUI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=9)
    if cond:
        op = OP_LB
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=16393)
    if cond:
        op = OP_LBU
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=4105)
    if cond:
        op = OP_LH
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=20489)
    if cond:
        op = OP_LHU
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=8201)
    if cond:
        op = OP_LW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=24585)
    if cond:
        op = OP_LWU
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=12297)
    if cond:
        op = OP_LD
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=12313)
    if cond:
        op = OP_LDI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=32767, match=4097)
    if cond:
        op = OP_BSTART_STD_FALL
        len_bytes = 4
    cond = in32 & masked_eq(m, insn32, mask=32767, match=8193)
    if cond:
        op = OP_BSTART_STD_DIRECT
        len_bytes = 4
        imm = simm17_s64.shl(amount=1)
    cond = in32 & masked_eq(m, insn32, mask=32767, match=12289)
    if cond:
        op = OP_BSTART_STD_COND
        len_bytes = 4
        imm = simm17_s64.shl(amount=1)
    cond = in32 & masked_eq(m, insn32, mask=32767, match=20481)
    if cond:
        op = OP_C_BSTART_STD
        len_bytes = 4
        imm = 5
    cond = in32 & masked_eq(m, insn32, mask=32767, match=28673)
    if cond:
        op = OP_C_BSTART_STD
        len_bytes = 4
        imm = 7
    cond = in32 & masked_eq(m, insn32, mask=28799, match=16421)
    if cond:
        op = OP_XORW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=8229)
    if cond:
        op = OP_ANDW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=12325)
    if cond:
        op = OP_ORW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=37)
    if cond:
        op = OP_ADDW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=4133)
    if cond:
        op = OP_SUBW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = shamt5_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=119)
    if cond:
        op = OP_CSEL
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        srcp = srcp_32
    cond = in32 & masked_eq(m, insn32, mask=28799, match=89)
    if cond:
        op = OP_SBI
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        imm = simm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=4185)
    if cond:
        op = OP_SHI
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        imm = simm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=8281)
    if cond:
        op = OP_SWI
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        imm = simm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=12377)
    if cond:
        op = OP_SDI
        len_bytes = 4
        srcl = rs1_32
        srcr = rs2_32
        imm = simm12_s64
    cond = in32 & masked_eq(m, insn32, mask=32767, match=73)
    if cond:
        op = OP_SB
        len_bytes = 4
        srcp = insn32[27:32]
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = 0
    cond = in32 & masked_eq(m, insn32, mask=32767, match=4169)
    if cond:
        op = OP_SH
        len_bytes = 4
        srcp = insn32[27:32]
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = 1
    cond = in32 & masked_eq(m, insn32, mask=32767, match=8265)
    if cond:
        op = OP_SW
        len_bytes = 4
        srcp = insn32[27:32]
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = 2
    cond = in32 & masked_eq(m, insn32, mask=32767, match=12361)
    if cond:
        op = OP_SD
        len_bytes = 4
        srcp = insn32[27:32]
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
        shamt = 3
    cond = in32 & masked_eq(m, insn32, mask=28799, match=8217)
    if cond:
        op = OP_LWI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_s64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=53)
    if cond:
        op = OP_ADDIW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_u64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=4149)
    if cond:
        op = OP_SUBIW
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_u64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=21)
    if cond:
        op = OP_ADDI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_u64
    cond = in32 & masked_eq(m, insn32, mask=28799, match=4117)
    if cond:
        op = OP_SUBI
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        imm = imm12_u64
    cond = in32 & masked_eq(m, insn32, mask=4160778367, match=69)
    if cond:
        op = OP_CMP_EQ
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
    cond = in32 & masked_eq(m, insn32, mask=4160778367, match=4165)
    if cond:
        op = OP_CMP_NE
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
    cond = in32 & masked_eq(m, insn32, mask=4160778367, match=16453)
    if cond:
        op = OP_CMP_LT
        len_bytes = 4
        regdst = rd32
        srcl = rs1_32
        srcr = rs2_32
        srcr_type = srcr_type_32
    # SSRSET: write SrcL to SSR_ID (base 32-bit form).
    cond = in32 & masked_eq(m, insn32, mask=0x00007FFF, match=0x0000103B)
    if cond:
        op = OP_SSRSET
        len_bytes = 4
        srcl = rs1_32
        imm = unsigned(insn32[20:32])
    cond = in32 & masked_eq(m, insn32, mask=4043309055, match=1052715)
    if cond:
        op = OP_EBREAK
        len_bytes = 4
    cond = in32 & masked_eq(m, insn32, mask=127, match=7)
    if cond:
        op = OP_ADDTPC
        len_bytes = 4
        regdst = rd32
        imm = imm20_s64.shl(amount=12)
        if rd32 == 10:
            op = OP_SETRET
            regdst = 10
            imm = imm20_u64.shl(amount=1)
    cond = in32 & masked_eq(m, insn32, mask=32767, match=16385)
    if cond:
        op = OP_BSTART_STD_CALL
        len_bytes = 4
        imm = simm17_s64.shl(amount=1)
    cond = in32 & masked_eq(m, insn32, mask=0xF83FFFFF, match=0x50166001)
    if cond:
        op = OP_BSTART_STD_ICALL
        len_bytes = 4
        imm = unsigned(insn32[22:27]).shl(amount=1) + 2
    hl_bstart_hi12 = unsigned(pfx16[4:16])
    hl_bstart_lo17 = unsigned(insn48[31:48])
    hl_bstart_simm_hw = cat(hl_bstart_hi12, hl_bstart_lo17, c(0, width=1)).as_signed()
    hl_bstart_off = hl_bstart_simm_hw
    cond = is_hl & masked_eq(m, insn48, mask=2147418127, match=268501006)
    if cond:
        op = OP_BSTART_STD_FALL
        len_bytes = 6
    cond = is_hl & masked_eq(m, insn48, mask=2147418127, match=536936462)
    if cond:
        op = OP_BSTART_STD_DIRECT
        len_bytes = 6
        imm = hl_bstart_off
    cond = is_hl & masked_eq(m, insn48, mask=2147418127, match=805371918)
    if cond:
        op = OP_BSTART_STD_COND
        len_bytes = 6
        imm = hl_bstart_off
    cond = is_hl & masked_eq(m, insn48, mask=2147418127, match=1073807374)
    if cond:
        op = OP_BSTART_STD_CALL
        len_bytes = 6
        imm = hl_bstart_off
    # HL.SSRSET: write SrcL to extended SSR_ID.
    cond = is_hl & masked_eq(m, insn48, mask=0x00007FFF000F, match=0x0000103B000E)
    if cond:
        op = OP_HL_SSRSET
        len_bytes = 6
        srcl = main32[15:20]
        imm_hi12 = unsigned(pfx16[4:16])
        imm_lo12 = unsigned(main32[20:32])
        imm = imm_hi12.shl(amount=12) | imm_lo12
    cond = is_hl & masked_eq(m, insn48, mask=8323087, match=3735566)
    if cond:
        len_bytes = 6
        regdst = insn48[23:28]
        simm_hi12 = unsigned(pfx16[4:16])
        simm_lo17 = unsigned(insn48[31:48])
        simm29 = cat(simm_hi12, simm_lo17).as_signed()
        imm = simm29
        funct3 = insn48[28:31]
        op = OP_HL_LW_PCR
        if funct3 == 0:
            op = OP_HL_LB_PCR
        if funct3 == 1:
            op = OP_HL_LH_PCR
        if funct3 == 2:
            op = OP_HL_LW_PCR
        if funct3 == 3:
            op = OP_HL_LD_PCR
        if funct3 == 4:
            op = OP_HL_LBU_PCR
        if funct3 == 5:
            op = OP_HL_LHU_PCR
        if funct3 == 6:
            op = OP_HL_LWU_PCR
    cond = is_hl & masked_eq(m, insn48, mask=8323087, match=6881294)
    if cond:
        len_bytes = 6
        srcl = insn48[31:36]
        simm_hi12 = unsigned(pfx16[4:16])
        simm_mid5 = unsigned(insn48[23:28])
        simm_lo12 = unsigned(insn48[36:48])
        simm29 = cat(simm_hi12, simm_mid5, simm_lo12).as_signed()
        imm = simm29
        funct3 = insn48[28:31]
        op = OP_HL_SW_PCR
        if funct3 == 0:
            op = OP_HL_SB_PCR
        if funct3 == 1:
            op = OP_HL_SH_PCR
        if funct3 == 2:
            op = OP_HL_SW_PCR
        if funct3 == 3:
            op = OP_HL_SD_PCR
    cond = is_hl & masked_eq(m, insn48, mask=1887371279, match=538247182)
    if cond:
        op = OP_ANDI
        len_bytes = 6
        regdst = rd_hl
        srcl = main32[15:20]
        imm_hi12 = pfx16[4:16]
        imm_lo12 = main32[20:32]
        imm24 = unsigned(imm_hi12).shl(amount=12) | unsigned(imm_lo12)
        imm = imm24.as_signed()
    cond = is_hl & masked_eq(m, insn48, mask=8323087, match=1507342)
    if cond:
        op = OP_HL_LUI
        len_bytes = 6
        regdst = rd_hl
        imm = imm_hl_lui
    return Decode(
        op=op,
        len_bytes=len_bytes,
        regdst=regdst,
        srcl=srcl,
        srcr=srcr,
        srcr_type=srcr_type,
        shamt=shamt,
        srcp=srcp,
        imm=imm,
    )
