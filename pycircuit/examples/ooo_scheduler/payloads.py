"""Tagged operands and transactions; ADD/MUL are micro-ops, not ISA encodings."""
from pycircuit import ac
ADD = 0
MUL = 1

class MicroOp:
    kind: ac.u32 = 0
    rd: ac.u32 = 0
    rs1: ac.u32 = 0
    rs2: ac.u32 = 0

class Operand:
    ready: bool = False
    tag: ac.u32 = 0
    value: ac.u32 = 0

class Entry:
    valid: bool = False
    kind: ac.u32 = 0
    tag: ac.u32 = 0
    rd: ac.u32 = 0
    lhs: Operand
    rhs: Operand

class Completion:
    tag: ac.u32 = 0
    rd: ac.u32 = 0
    value: ac.u32 = 0

class RobEntry:
    valid: bool = False
    done: bool = False
    tag: ac.u32 = 0
    rd: ac.u32 = 0
    value: ac.u32 = 0
