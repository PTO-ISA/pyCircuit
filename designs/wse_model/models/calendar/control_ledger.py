# ruff: noqa: N802, F823, F841 -- pyCircuit declarations and captured state.
"""Persistent 8-bit epoch bank and resident CalReg installation checks.

The epoch width is the accepted test profile. CalReg content stays opaque;
this module implements installation/lookup admission, not a timeslot schedule.
`drained` is an external precondition, not an inferred network measurement.
An invalid request reports fault and leaves persistent state unchanged.
"""

# 本文件维护两类跨周期控制状态：每节点/类别的 epoch（通信轮次），
# 以及 CalReg（Calendar 配置寄存器）的“已安装类别”位图。
# 当前实现只保存配置的存在性与安装准入，具体 COPY 路由由机器指令携带。
#
# @ac.module 分配独立硬件状态；ac.table 是持久状态表；@ac.method 暴露操作。
# 方法只在被调用的路径执行，读写加入调用者 Rule 的同一事务，不另占一个周期。
# request 是结构化输入，返回结构同时带正常值和 fault（协议故障标志）。
# drained 是调用者给出的“旧通信已排空”条件，在这里并不自行测量网络占用。

import pycircuit as ac


# advance=True 推进轮次，False 只查询；opcode 是 3 位通信类别，
# drained 表示调用者已确认旧通信排空，回绕轮次时使用。
@ac.struct
class EpochRequest:
    advance: bool
    opcode: ac.u3
    drained: bool


# value 是当前操作计算出的 8 位轮次；fault 表示本次推进被拒绝。
# 8 位最多表示 255；0 是初始值，实际非零轮次使用 1..255。
@ac.struct
class EpochResult:
    value: ac.u8
    fault: bool


# ndf: implements=WSEMIG-EPOCH-001
# 40 个节点×8 个类别位置=320 项；表中每个元素是一个 8 位计数器。
# 独立节点和类别分开记轮次，避免不同通信上下文相互推进。
@ac.module
def EpochLedger():
    counters = ac.table[320, ac.u8](init=0)

    @ac.method
    def process(node: ac.u6, request: EpochRequest) -> EpochResult:
        assert node < 40, "epoch node in 40-node profile"
        # 二维逻辑索引展平为 node*8+opcode。用 16 位承接乘法和加法结果。
        # node 已经过断言验证，opcode 的 u3 类型保证其在 0..7。
        bank: ac.u16 = node
        opcode: ac.u16 = request.opcode
        index: ac.u16 = bank * 8 + opcode
        value: ac.u8 = counters[index % 320]
        fault = False
        # 查询时直接返回表内值；推进时仅类别 1..6 合法。
        # 255 回绕到 1 需要 drained；一般递增不在本方法内要求 drained。
        # 主机器自身对 ADVANCE 另有更强要求：只有排空才调用推进。
        if request.advance:
            if request.opcode == 0 or request.opcode == 7:
                fault = True
            elif value == 255 and not request.drained:
                fault = True
            else:
                # 0 首次推进成 1；1..254 各加 1；255 在允许回绕时回到 1。
                # 先更新局部 value，再赋回表，因此返回值与准备提交的新轮次一致。
                value = 1 if value == 255 else value + 1
                counters[index % 320] = value
        return EpochResult(value=value, fault=fault)


# kind：0 不操作，1 清空节点配置位图，2 安装一个类别，3 查找一个类别。
# opcode 选择位图中的一个位置；drained 是清空和安装的排空前提。
@ac.struct
class CalRegRequest:
    # 0 idle, 1 clear bank, 2 install one slot, 3 lookup one slot.
    kind: ac.u2
    opcode: ac.u3
    drained: bool


# installed 是 8 位类别位图；admitted 表示本次非空操作没有协议故障。
# 它不是物理链路 ready 信号，也不表示已经获得某个 Calendar 时间槽。
@ac.struct
class CalRegResult:
    installed: ac.u8
    admitted: bool
    fault: bool


# ndf: implements=WSEMIG-CALREG-001
# 每节点一个 8 位位图，第 opcode 位为 1 表示该类别已经安装。
# 与 EpochLedger 的每类别计数器相比，这里每个节点只占一项。
@ac.module
def CalRegLedger():
    banks = ac.table[40, ac.u8](init=0)

    @ac.method
    def process(node: ac.u6, request: CalRegRequest) -> CalRegResult:
        assert node < 40, "configuration node in 40-node profile"
        installed: ac.u8 = banks[node % 40]
        # 先生成 8 位的 1，再左移 opcode 位；installed & bit 非零表示类别存在。
        bit: ac.u8 = 1
        bit = bit << request.opcode
        exists = (installed & bit) != 0
        fault = False
        # 清空需要 drained。安装还要求类别合法、旧通信已排空、该类别尚未安装。
        # 查找只验证类别合法并且已经存在；它不会添加新的配置。
        if request.kind == 1:
            if request.drained:
                installed = 0
            else:
                fault = True
        elif request.kind != 0:
            if request.opcode == 0 or request.opcode == 7:
                fault = True
            elif request.kind == 2:
                if not request.drained or exists:
                    fault = True
                else:
                    installed = installed | bit
            elif not exists:
                fault = True
        # 写回局部 installed。失败分支未修改它，所以故障时保留原配置位图。
        # kind=0 返回 admitted=False；其余类别成功时 admitted=True。
        banks[node % 40] = installed
        return CalRegResult(
            installed=installed, admitted=request.kind != 0 and not fault, fault=fault
        )
