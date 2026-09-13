from enum import Enum

import agentic_circuit as ac


@ac.encoding(width=64)
class WideOpcode(Enum):
    LOW = 9223372036854775807
    HIGH = 9223372036854775808
    MAX = 18446744073709551615


@ac.encoding(width=7)
class Lane(Enum):
    L00 = 0
    L01 = 1
    L02 = 2
    L03 = 3
    L04 = 4
    L05 = 5
    L06 = 6
    L07 = 7
    L08 = 8
    L09 = 9
    L10 = 10
    L11 = 11
    L12 = 12
    L13 = 13
    L14 = 14
    L15 = 15
    L16 = 16
    L17 = 17
    L18 = 18
    L19 = 19
    L20 = 20
    L21 = 21
    L22 = 22
    L23 = 23
    L24 = 24
    L25 = 25
    L26 = 26
    L27 = 27
    L28 = 28
    L29 = 29
    L30 = 30
    L31 = 31
    L32 = 32
    L33 = 33
    L34 = 34
    L35 = 35
    L36 = 36
    L37 = 37
    L38 = 38
    L39 = 39
    L40 = 40
    L41 = 41
    L42 = 42
    L43 = 43
    L44 = 44
    L45 = 45
    L46 = 46
    L47 = 47
    L48 = 48
    L49 = 49
    L50 = 50
    L51 = 51
    L52 = 52
    L53 = 53
    L54 = 54
    L55 = 55
    L56 = 56
    L57 = 57
    L58 = 58
    L59 = 59
    L60 = 60
    L61 = 61
    L62 = 62
    L63 = 63
    L64 = 64
    EMPTY = 65
    ERROR = 66


@ac.struct
class StressInput:
    mask: ac.array[65, bool]
    raw: ac.u64


@ac.struct
class StressOutput:
    lane: Lane
    present: bool
    conflict: bool
    decoded: WideOpcode
    valid: bool


@ac.rule
def decode(value: StressInput) -> StressOutput:
    lane = ac.onehot_enum(
        value.mask,
        members=(
            Lane.L00, Lane.L01, Lane.L02, Lane.L03, Lane.L04,
            Lane.L05, Lane.L06, Lane.L07, Lane.L08, Lane.L09,
            Lane.L10, Lane.L11, Lane.L12, Lane.L13, Lane.L14,
            Lane.L15, Lane.L16, Lane.L17, Lane.L18, Lane.L19,
            Lane.L20, Lane.L21, Lane.L22, Lane.L23, Lane.L24,
            Lane.L25, Lane.L26, Lane.L27, Lane.L28, Lane.L29,
            Lane.L30, Lane.L31, Lane.L32, Lane.L33, Lane.L34,
            Lane.L35, Lane.L36, Lane.L37, Lane.L38, Lane.L39,
            Lane.L40, Lane.L41, Lane.L42, Lane.L43, Lane.L44,
            Lane.L45, Lane.L46, Lane.L47, Lane.L48, Lane.L49,
            Lane.L50, Lane.L51, Lane.L52, Lane.L53, Lane.L54,
            Lane.L55, Lane.L56, Lane.L57, Lane.L58, Lane.L59,
            Lane.L60, Lane.L61, Lane.L62, Lane.L63, Lane.L64,
        ),
        empty=Lane.EMPTY,
        conflict=Lane.ERROR,
    )
    decoded = ac.checked(value.raw, WideOpcode, fallback=WideOpcode.LOW)
    return StressOutput(
        lane=lane.value,
        present=lane.present,
        conflict=lane.conflict,
        decoded=decoded.value,
        valid=decoded.valid,
    )


@ac.system
def enum_stress(value: StressInput) -> StressOutput:
    result = decode(value)
    return result
