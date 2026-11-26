# ruff: noqa: N802, N803 -- pyCircuit declarations.
"""Original one-context tick interface over the shared Calendar ledger."""

# 这是旧的单接收上下文组件入口，供组件回归使用。完整机器直接实例化
# ledger.py 的 ReceiveLedger，并按 40 个节点组织状态。
#
# @ac.module 定义硬件；关键字参数 EARLY_SLOTS/SEEN_SLOTS 是编译时资源规模。
# @ac.rule 定义自动注册的规则，每次求值把输入交给同一个账本实例。
# return step 返回结果连接。跨周期账户和身份保存在 ledger 中，不在局部变量里。
# Segment 的重复导入形式用于保留原有对外名称；行为算法只有 ledger.py 一份。

import pycircuit as ac
from wse_calendar.ledger import ReceiveLedger, Request, Result
from wse_calendar.ledger import Segment as Segment


# ndf: implements=WSEMIG-RX-001
# 本入口只有一个逻辑节点，所以总槽数等于每节点槽数。
# ROW_BITS=SEEN_SLOTS*22，因为每个去重身份占源节点 6 位加序号 16 位。
@ac.module
def ReceiveEngine(request: Request, *, EARLY_SLOTS: int, SEEN_SLOTS: int) -> Result:
    ledger = ReceiveLedger(
        NODES=1,
        EARLY_SLOTS=EARLY_SLOTS,
        SEEN_SLOTS=SEEN_SLOTS,
        TOTAL_EARLY=EARLY_SLOTS,
        TOTAL_SEEN=SEEN_SLOTS,
        ROW_BITS=SEEN_SLOTS * 22,
    )

    @ac.rule
    def step() -> Result:
        # 固定处理节点 0，process 会优先排空匹配早到项，再尝试接受当前请求。
        # 因此需要查看返回的 accepted/status，不能只凭调用发生就认为请求已处理。
        return ledger.process(0, request)

    return step
