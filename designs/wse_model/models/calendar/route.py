# ruff: noqa: N802, N803 -- pyCircuit module and static parameter names.
"""Decode the Calendar baseline's 16-byte register-pair entry.

The 40-node layout is fixed by Calendar §1.7. The 48-node layout is still Q1;
this module does not emit or reinterpret an entry for that configuration.
"""

# 这个组件解码 Calendar 的 16 字节路由表项。它是组件入口，主机器的 COPY
# 则由 machine_core.py 的 decode 从 64 字节指令中提取路由；二者装载格式不同。
#
# 16 字节=128 位：lo 保存低 64 位，hi 保存高 64 位。
# - 整体低 80 位：40 个节点各两位，低位 L 表示落地，高位 P 表示参与路径。
# - 整体 [80:88]：落点计数；[88:96]：标志；[96:128]：预期字节数。
# 因此节点 0..31 的两位来自 lo，节点 32..39 的两位来自 hi 的低 16 位。
#
# @ac.struct 组合定宽信号，@ac.module 声明模块，@ac.rule 注册无状态解码逻辑。
# NODE 是编译时指定的目标节点，不是每周期变化的输入。u7 能表示最大移位量 78。
# 位切片 x[a:b] 的 b 不包含在内，最低位从 0 编号。

import pycircuit as ac


# 软件/宿主提供的一对 64 位寄存器值，共同携带一个 128 位表项。
@ac.struct
class RouteRegisters:
    lo: ac.u64
    hi: ac.u64


# pair 是选中节点的 {P,L} 两位；其余字段是整张表项的公共元数据。
# fault 检查该节点是否为非法 01；不在这里验证整张路径是否连通。
@ac.struct
class DecodedEntry:
    pair: ac.u2
    land_count: ac.u8
    flags: ac.u8
    expected_bytes: ac.u32
    fault: bool


# ndf: implements=WSEMIG-ROUTE-001
@ac.module
def DecodeEntry(entry: RouteRegisters, *, NODE: int = 0) -> DecodedEntry:
    @ac.rule
    def decode() -> DecodedEntry:
        # The largest low shift is 78, so seven bits carry every shift exactly.
        node: ac.u7 = NODE
        assert node < 40, "Calendar entry requires node 0..39"
        # 两个 64 位字分别右移，把目标节点标志对齐低两位，再按位或。
        # node<32 时 hi 移 64 位贡献零；node>=32 时 lo 移至少 64 位贡献零，
        # hi 按 2*node-64 对齐。这里描述定宽硬件移位，不是跨周期扫描。
        low = entry.lo >> (2 * node)
        high = entry.hi >> (2 * node - 64 if node >= 32 else 64)
        pair = low[0:2] | high[0:2]
        return DecodedEntry(
            pair=pair,
            # hi 的位 16 对应整体位 80，依此取出落点数、标志和预期字节数。
            land_count=entry.hi[16:24],
            flags=entry.hi[24:32],
            expected_bytes=entry.hi[32:64],
            fault=pair == 1,
        )

    # 把已注册的解码规则结果连接为模块输出，不需要显式 decode()。
    return decode
