# ruff: noqa: N802 -- pyCircuit declarations.
"""Original control tick interfaces over the shared node-indexed cores."""

# 这是旧组件测试入口的薄适配层，完整机器与它复用同一套账本算法。
# 完整机器有 40 个节点，使用实例的 process(node,request) 方法选择节点；
# 这里固定 node=0，使旧测试可以继续每次只提供一个 request。
#
# @ac.module 分配子模块实例；@ac.rule 把每次请求接到子模块的方法。
# 方法调用在该规则的同一事务内执行；return step 输出规则计算的结果。
# 完整机器直接导入 control_ledger.py 的 CalRegLedger/EpochLedger。
# 理解算法请读 control_ledger.py；此文件没有另一份轮次或配置实现。

import pycircuit as ac
from wse_calendar.control_ledger import (
    CalRegLedger,
    CalRegRequest,
    CalRegResult,
    EpochLedger,
    EpochRequest,
    EpochResult,
)


# 建立独立轮次账本实例，并把输入连接到节点 0。
# request.advance 决定查询还是推进；状态由子模块保存。
@ac.module
def EpochBank(request: EpochRequest) -> EpochResult:
    epochs = EpochLedger()

    @ac.rule
    def step() -> EpochResult:
        return epochs.process(0, request)

    return step


# 建立独立配置账本实例，并把输入连接到节点 0。
# 两个模块各自有独立状态，并不是访问一个宿主 Python 全局字典。
@ac.module
def CalRegAdmission(request: CalRegRequest) -> CalRegResult:
    configuration = CalRegLedger()

    @ac.rule
    def step() -> CalRegResult:
        return configuration.process(0, request)

    return step
