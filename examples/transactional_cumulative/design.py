# ruff: noqa: F821, F823, F842, N802 -- Compiler-owned hardware bindings.
import pycircuit as ac


# 外部 Rule：通过显式输入输出连接。
@ac.rule
def add_one(src: ac.u8) -> ac.u8:
    x = src
    return x + 1


@ac.module
def BiasStage(inp: ac.u8) -> ac.u8:
    out = add_one(inp)  # 注册 Rule，输出默认 Queue
    return out  # 导出已有输出对象


@ac.module
def Accumulator():
    total: ac.u16 = 0
    samples: ac.u8 = 0

    @ac.method(guard=lambda: samples < 4)
    def add(x: ac.u8) -> ac.u16:
        new_total = total + x
        total = new_total
        samples = samples + 1
        return new_total  # 返回同拍计算值


@ac.module
def SumStage(inp: ac.u8, acc: ac.module) -> ac.u16:
    # acc 是上层传入的实例引用，具体模块在连接时确定。
    accepted: ac.u8 = 0

    @ac.rule
    def accumulate(src: ac.u8) -> ac.u16:
        x = src
        result = acc.add(x)
        accepted = accepted + 1  # 直接操作所在 Module 的寄存器
        return result

    out = accumulate(inp)  # 显式注册内部 Rule
    return out


@ac.module
def Pipeline(inp: ac.u8) -> ac.u16:
    acc: Accumulator  # 显式创建具名实例
    summer: SumStage

    intermediate = BiasStage(inp)  # 隐式创建实例，绑定输出 Queue
    out = summer(intermediate, acc)  # 连接已有实例，传入模块引用
    return out
