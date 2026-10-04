from pycircuit import ac
from ..ripes5.logic import decode, Instruction, INVALID, ADD, ADDI, SUB, AND, OR, XOR, SLT, LUI, LW, SW, BEQ, BNE, JAL, JALR, HALT

class Tag:
    epoch: ac.u64 = 0
    sequence: ac.u64 = 0

class Control:
    epoch: ac.u64 = 1
    head: ac.u64 = 1
    target: ac.u32 = 0
    stopped: bool = False

class Front:
    epoch: ac.u64 = 0
    pc: ac.u32 = 0

class Fetched:
    epoch: ac.u64 = 0
    pc: ac.u32 = 0
    word: ac.u32 = 0
    fault: ac.u32 = 0

class Tail:
    epoch: ac.u64 = 1
    sequence: ac.u64 = 1

class Operand:
    ready: bool = True
    value: ac.u32 = 0
    producer: Tag

class Entry:
    tag: Tag
    pc: ac.u32 = 0
    word: ac.u32 = 0
    fault: ac.u32 = 0
    ins: Instruction
    left: Operand
    right: Operand

class Operands:
    tag: Tag
    left: Operand
    right: Operand

class Pick:
    valid: bool = False
    index: ac.u32 = 0
    sequence: ac.u64 = 0

class Request:
    tag: Tag
    pc: ac.u32 = 0
    word: ac.u32 = 0
    fault: ac.u32 = 0
    left: ac.u32 = 0
    right: ac.u32 = 0

class Completion:
    tag: Tag
    pc: ac.u32 = 0
    word: ac.u32 = 0
    value: ac.u32 = 0
    address: ac.u32 = 0
    store_value: ac.u32 = 0
    redirect: bool = False
    target: ac.u32 = 0
    fault: ac.u32 = 0

class Pending:
    remaining: ac.u32 = 0
    result: Completion

class Retirement:
    count: ac.u64 = 0
    tag: Tag
    pc: ac.u32 = 0
    word: ac.u32 = 0
    rd: ac.u32 = 0
    value: ac.u32 = 0
    store: bool = False
    address: ac.u32 = 0
    store_value: ac.u32 = 0
    fault: ac.u32 = 0
    halt: bool = False

class Flush:
    count: ac.u64 = 0
    tag: Tag
    target: ac.u32 = 0


def same(a: Tag, b: Tag) -> bool:
    return a.epoch == b.epoch and a.sequence == b.sequence


def slot(tag: Tag) -> ac.u32:
    return ac.u32((tag.sequence - 1) % 8)


def live(entry: Entry, ctl: Control) -> bool:
    return entry.tag.epoch == ctl.epoch and entry.tag.sequence >= ctl.head and not ctl.stopped


def memory_op(op: ac.u32) -> bool:
    return op in (LW, SW)


def calculate(req: Request) -> Completion:
    ins = decode(req.word)
    a = req.left
    b = req.right
    op = ins.op
    result = Completion(tag=req.tag, pc=req.pc, word=req.word, fault=req.fault)
    if op == ADD:
        result.value = a + b
    elif op == ADDI:
        result.value = a + ins.immediate
    elif op == SUB:
        result.value = a - b
    elif op == AND:
        result.value = a & b
    elif op == OR:
        result.value = a | b
    elif op == XOR:
        result.value = a ^ b
    elif op == SLT:
        result.value = ac.u32(ac.i32(a) < ac.i32(b))
    elif op == LUI:
        result.value = ins.immediate
    elif op in (JAL, JALR):
        result.value = req.pc + 4
    if op in (JAL, BEQ, BNE):
        result.target = req.pc + ins.immediate
    elif op == JALR:
        result.target = (a + ins.immediate) & 0xfffffffe
    taken = op in (JAL, JALR) or (op == BEQ and a == b) or (op == BNE and a != b)
    result.redirect = taken and result.target != req.pc + 4
    result.address = a + ins.immediate
    result.store_value = b
    return result
