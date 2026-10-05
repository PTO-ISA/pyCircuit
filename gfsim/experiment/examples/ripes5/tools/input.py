"""Validate the host input protocol before constructing the reference CPU."""
from ..isa import decode
from ..records import INVALID, HALT

MARKER = 0x7ff00013


def validate(case):
    for name in ('words', 'registers', 'data'):
        if not isinstance(case[name], list) or any(type(v) is not int or not 0 <= v <= 0xffffffff for v in case[name]):
            raise ValueError(f'{name} must be a list of uint32 values')
    words = case['words']
    end = case['end_pc']
    if len(case['registers']) != 32 or case['registers'][0] != 0:
        raise ValueError('32 registers required; x0 must be zero')
    if (type(case['data_base']) is not int or case['data_base'] % 4
            or case['data_base'] < len(words) * 4 or not case['data']
            or case['data_base'] + len(case['data']) * 4 > 2**32):
        raise ValueError('aligned, disjoint nonempty data region required')
    if end % 4 or not 0 <= end < len(words) * 4 or words[end // 4] != MARKER:
        raise ValueError('end_pc must identify the reserved marker')
    if words[end // 4 + 1:] != [0x6f, 0x13, 0x13] or words.count(MARKER) != 1:
        raise ValueError('marker must be unique and followed by the safe loop suffix')
    if any(decode(w).op in (INVALID, HALT) for w in words):
        raise ValueError('unsupported instruction (HALT/ECALL excluded)')
    if type(case['max_cycles']) is not int or case['max_cycles'] < 1:
        raise ValueError('positive max_cycles required')
