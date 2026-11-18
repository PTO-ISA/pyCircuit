# ruff: noqa: N802 -- pyCircuit module name.
"""Calendar single-hop bitmap forwarding on the explicit 5-by-8 profile.

No link latency, queue capacity or CalReg schedule is assigned here. Complete
routes still require the core's induced-tree validation before injection.
"""

# 这是单跳路由判定模块：给当前节点及完整路由位图，算出哪些邻居是出口。
# 它不自己移动数据、保存队列或增加时钟延迟；主机器 stage=2 使用它遍历整条路径。
#
# 5×8 网格按行编号：第一行 0..7，第二行 8..15，最后一行 32..39。
# 上/下邻居编号差 8，左/右邻居差 1；边界处没有对应邻居。
# route 中每节点占 2 位：第 2*n 位 L（落地），第 2*n+1 位 P（参与路径）。
# 合法组合：00 不参与，10 只转发，11 参与且落地；01 非法。
# 二进制写法按高位到低位展示，因此 10 中 P=1、L=0。
#
# @ac.struct 定义信号组合；@ac.module 定义模块；@ac.rule 定义自主计算。
# 本模块没有持久状态。局部 out 和 peer 在一次规则求值中计算，形成组合逻辑。
# 完整路径的连通性和树形约束由 machine.py 检查，这里处理一个节点的邻接关系。

import pycircuit as ac


# route 是全网 80 位路径；node 是本跳节点。
# has_ingress 表示是否有来路（源节点没有）；ingress 给出父节点编号。
# 父节点是“上一跳节点”，不是一个端口编号。
@ac.struct
class HopInput:
    route: ac.bits[80]
    node: ac.u6
    has_ingress: bool
    ingress: ac.u6


# egress 是 40 位出口节点集合，第 n 位为 1 表示应走向邻居 n。
# lands 表示本节点需要落地；fault 表示路径编码或当前节点不合法。
@ac.struct
class HopResult:
    egress: ac.u40
    lands: bool
    fault: bool


# 把节点的两位标志右移到最低端，取 P 位判断是否参与路径。
# offset 用 7 位，因为最大移位量 2*39=78，需要能表示到 78。
def passes(route: ac.bits[80], node: ac.u6) -> bool:
    offset: ac.u7 = node
    shifted = route >> (2 * offset)
    return shifted[1:2] == 1


# ndf: implements=WSEMIG-FORWARD-001
@ac.module
def ForwardHop(request: HopInput) -> HopResult:
    @ac.rule
    def forward() -> HopResult:
        # 十六进制 5 是二进制 0101，所以这个掩码选中所有偶数位，也就是所有 L 位。
        # route 右移一位后 P 对齐 L；L & ~P 非零说明存在“落地却不参与”的非法节点。
        # 此检查扫描整张路由位图，不只检查当前节点。
        land_mask: ac.bits[80] = 0x55555555555555555555
        illegal = ((request.route & land_mask) & ~((request.route >> 1) & land_mask)) != 0
        # 当前节点必须在 0..39，并且它的 P 位要置位。
        # 故障时 out 保持零，最后 lands 也被屏蔽为 False。
        fault = request.node >= 40 or illegal or not passes(request.route, request.node)
        out: ac.u40 = 0
        one: ac.u40 = 1
        offset: ac.u7 = request.node
        pair = request.route >> (2 * offset)
        if not fault:
            # 向上：第一行以外才有上邻居。邻居必须参与路径，而且不能是父节点。
            # 排除来路避免立刻回头；其他环路仍需主机器的 visited/frontier 检查。
            if request.node >= 8:
                peer = request.node - 8
                if passes(request.route, peer) and (
                    not request.has_ingress or peer != request.ingress
                ):
                    out = out | (one << peer)
            # 向左：8 列网格的列号等于 node 的低 3 位；列号非零才可减 1。
            # 这样节点 8 不会错误地把上一行的节点 7 当左邻居。
            if request.node[0:3] != 0:
                peer = request.node - 1
                if passes(request.route, peer) and (
                    not request.has_ingress or peer != request.ingress
                ):
                    out = out | (one << peer)
            # 向右：列号不是 7 才可加 1，防止跨到下一行。
            if request.node[0:3] != 7:
                peer = request.node + 1
                if passes(request.route, peer) and (
                    not request.has_ingress or peer != request.ingress
                ):
                    out = out | (one << peer)
            # 向下：最后一行从 32 开始，只有前四行有下邻居。
            # 四个方向是独立 if，一个分支节点可以同时选中多个出口。
            if request.node < 32:
                peer = request.node + 8
                if passes(request.route, peer) and (
                    not request.has_ingress or peer != request.ingress
                ):
                    out = out | (one << peer)
        # 出口位图、落地标志和故障标志一起返回；数据本体由主机器另行处理。
        return HopResult(egress=out, lands=not fault and pair[0:1] == 1, fault=fault)

    # 返回已注册规则的结果连接；不用 forward() 手动调用它。
    return forward
