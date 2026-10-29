# ruff: noqa: F821, F823, F842, N802 -- Compiler-owned hardware bindings.
import pycircuit as ac


@ac.module
def BatchAccumulator():
    total: ac.u16 = 0
    samples: ac.u8 = 0

    # Module 组合逻辑：只读本地旧状态，不创建寄存器。
    slots_left = 4 - samples
    can_add = slots_left != 0
    complete = slots_left == 0

    # public Value Method：直接返回已计算的组合值。
    @ac.method
    def remaining() -> ac.u8:
        return slots_left

    # public Action Method：使用组合条件作为 guard。
    @ac.method(guard=lambda: can_add)
    def add(x: ac.u8) -> None:
        total = total + x
        samples = samples + 1

    # public ActionValue Method：返回本批结果，同时清空批次状态。
    @ac.method(guard=lambda: complete)
    def take() -> ac.u16:
        result = total
        total = 0
        samples = 0
        return result


@ac.module
def BatchController(inp: ac.u8, acc: ac.module) -> ac.u16:
    accepted: ac.u8 = 0

    # Module 组合逻辑供 Rule 和 Method 使用。
    next_accepted = accepted + 1
    slots_left = 4 - accepted

    @ac.method
    def remaining() -> ac.u8:
        return slots_left

    @ac.rule
    def collect(src: ac.u8) -> None:
        x = src
        acc.add(x)
        accepted = next_accepted

    @ac.rule
    def emit() -> ac.u16:
        result = acc.take()
        accepted = 0
        return result

    # 声明两个事务及其互斥的启用条件。
    if accepted < 4:
        collect(inp)
    else:
        out = emit()

    return out


@ac.module
def BatchPipeline(inp: ac.u8) -> ac.u16:
    acc: BatchAccumulator
    controller: BatchController

    out = controller(inp, acc)
    return out
