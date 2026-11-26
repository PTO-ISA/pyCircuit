"""Model-resource diagnostics, distinct from WSE protocol faults."""

# 这个小模块集中定义“去重身份槽耗尽”的模型容量诊断。
# 去重身份用于防止同一片段被重复计数；每个驻留上下文只有有限槽位。
#
# @ac.module 声明硬件模块，@ac.method 声明供账本调用的方法。
# 方法和调用者属于同一事务；断言失败时，外层事务中的计数、身份写入、
# 早到槽移除以及相关数据写入都不会部分提交。
# 这是仿真终止式容量诊断。协议故障则由账本 fault 字段记录，含义不同。

import pycircuit as ac


# ndf: implements=WSEMIG-RX-CAPACITY-001
@ac.module
def ReceiveChecks():  # noqa: N802 -- pyCircuit module name.
    # available 由 ReceiveLedger.inspect 的空槽扫描给出。
    # None 表示只有检查效果，没有数据返回。达到此调用且 available=False 时，
    # 后端报告下方诊断并终止该失败事务；不把容量不足假装成可等待释放的空位。
    @ac.method
    def require_identity_capacity(available: bool) -> None:
        assert available, "WSE_MODEL_CAPACITY: dedup identity slots exhausted"
