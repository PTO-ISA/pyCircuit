# ruff: noqa: N802, N803, F823, F841 -- pyCircuit declarations and captured state.
"""Finite Calendar ingress; capacities are mandatory consumer parameters.

One resident receive context per logical node is an explicit resource limit,
not a Calendar architecture default. Unmatched epochs share that node's
EARLY_SLOTS pending entries.
Matching early entries drain one per cycle before another request is accepted.
Faults are terminal until reset. No payload memory is represented here.
The 16-bit sequence field is a test carrier (0..65535), not a WSE header
specification. A producer must not wrap/reuse an identity in a resident context.
Dedup exhaustion terminates simulation with a model-capacity diagnostic; it
does not report a WSE protocol fault or wait for space that cannot be freed.
"""

# 本文件是接收账本，不保存数据载荷。可以把每节点账户理解为一张收货单：
# “本轮允许写入哪段地址、应收多少字节、已收多少、哪些片段已经收过”。
# 数据字节由 machine_core.py 的 BankedMemory 保存；账本维护与它一致的接收进度。
#
# 第一次阅读：先看 Segment/Request/Account/Result，再读 ReceiveLedger 的表，
# 然后按 observe → inspect → commit → process 理解。
# - observe：查询现状；inspect：判定一个片段是否合法、重复或可计数。
# - commit：把判定结果写到账本；process：处理登记、交付、退休和早到项。
# 主机器的 COPY 用 inspect 预检，在目的末拍数据写入时才调用 commit。
# 组件测试和单上下文 ReceiveEngine 则可通过 process 逐步提供元数据。
#
# pyCircuit 语法：@ac.struct 定义定宽字段组合；@ac.module 分配硬件和持久状态；
# @ac.method 定义由外层 Rule 调用的方法。方法调用沿用外层事务，不额外走一周期。
# ac.table[N,T](init=0) 是 N 项 T 类型状态表，初始各字段清零。
# 方法内部局部变量只是本次计算的暂存；赋回表才提出持久更新。
# 同一事务中后续语句可读取前面提出的新值，失败时整笔事务不发布部分更新。
#
# 位向量 x[a:b] 取第 a..b-1 位；a 从最低位 0 开始。
# | 是按位或，& 是按位与，~ 是按位取反，<< 和 >> 分别左移和右移。
# 例如 seen_valid | (1 << slot) 把某个去重槽标成有效。
# 容量是本功能配置的显式上限；早到槽保存片段元数据，不保存提前到达的载荷。

import pycircuit as ac
from wse_calendar.receive_checks import ReceiveChecks


# 片段元数据：opcode 是通信类别，epoch 是通信轮次，两者定位接收上下文。
# address/nbytes 是目的字节范围；source/seq 是源节点与片段序号。
# source 6 位加 seq 16 位组成 22 位去重身份；同一上下文内不得回绕复用身份。
# committed 标记载荷是否已提交；self_sourced 标记是否是源自身副本。
# 未提交片段及自身副本有各自状态码，不增加远端接收字节计数。
@ac.struct
class Segment:
    opcode: ac.u3
    epoch: ac.u8
    address: ac.u64
    nbytes: ac.u32
    source: ac.u6
    seq: ac.u16
    committed: bool
    self_sourced: bool


# 给 process 的操作：kind=0 无新请求，1 POST 建立接收，2 DELIVER 交付元数据，
# 3 RETIRE 结束接收。segment 提供类别、轮次及范围；后三项主要由 POST 使用。
# expected 是应收远端字节数，capacity 是目的容量，二者单位都是字节。
# compiled_expected 是程序生成端给的期望值，用来核对请求中的 expected。
@ac.struct
class Request:
    # 0 idle, 1 post, 2 deliver, 3 retire.
    kind: ac.u2
    segment: Segment
    capacity: ac.u32
    expected: ac.u32
    compiled_expected: ac.u32


# 每个逻辑节点只有一个驻留账户。
# active 表示账户已建立；fault 表示该账户发生协议错误，复位前保持错误。
# dst/capacity 界定允许落地范围；expected/counted 表示应收/已收远端字节数。
# 即使达到 expected，仍要 RETIRE 释放账户和去重有效位，才能交给下一轮使用。
@ac.struct
class Account:
    active: bool
    fault: bool
    opcode: ac.u3
    epoch: ac.u8
    dst: ac.u64
    capacity: ac.u32
    expected: ac.u32
    counted: ac.u32


# 一个早到槽只有有效位与片段元数据。当前类别/轮次尚未 POST 的片段，
# 可先记在本节点早到槽，匹配账户建立后再转入计数判定。
@ac.struct
class Pending:
    valid: bool
    segment: Segment


# accepted：当前外部请求是否被处理；并不等于一定成功，例如非法 POST
# 也可能 accepted=True 且 fault=True，必须结合 status/fault 一起读。
# status 数值：0 空闲，1 已登记，2 新片段计数，3 重复片段，4 源自身片段，
# 5 载荷未提交，6 存入早到槽，7 已退休，8 暂时无法接收，9 协议故障。
# complete 要求账户仍活跃且 counted>=expected；pending 是有效早到槽个数。
@ac.struct
class Result:
    accepted: bool
    # 0 idle, 1 posted, 2 counted, 3 duplicate, 4 self, 5 uncommitted,
    # 6 deferred, 7 retired, 8 resource backpressure, 9 fault.
    status: ac.u4
    fault: bool
    active: bool
    counted: ac.u32
    complete: bool
    pending: ac.u8


# inspect 的判定结果：状态码、可用去重槽编号、新的累计字节数。
# 只有 status=2 时，slot 与 total 才用于提交一个新的有效身份。
@ac.struct
class Decision:
    status: ac.u4
    slot: ac.u8
    total: ac.u32


@ac.struct
class FreeSlot:
    found: bool
    slot: ac.u8


# 在八个连续的 22 位身份中并行比较。每个身份低 16 位是 seq，高 6 位是 source。
def matching_octet(row: ac.bits[176], valid: ac.u8, identity: ac.bits[22]) -> bool:
    pair = ac.concat(identity, identity)
    four = ac.concat(pair, pair)
    diff = row ^ ac.concat(four, four)
    # 将每个身份的 22 个差异位（16 + 4 + 2）归约到该身份的最低位。
    two = diff | (diff >> 1)
    four_bits = two | (two >> 2)
    eight = four_bits | (four_bits >> 4)
    sixteen = eight | (eight >> 8)
    mismatches = sixteen | (four_bits >> 16) | (two >> 20)
    lanes = ac.concat(
        mismatches[154:155],
        mismatches[132:133],
        mismatches[110:111],
        mismatches[88:89],
        mismatches[66:67],
        mismatches[44:45],
        mismatches[22:23],
        mismatches[0:1],
    )
    return ((~lanes) & valid) != 0


# 这些关键字参数是编译时常量，决定硬件表的规模，不是运行时输入信号。
# NODES：节点数；EARLY_SLOTS/SEEN_SLOTS：每节点的早到/去重槽数。
# TOTAL_*：对应表的总项数；ROW_BITS：一节点去重身份行的位数。
# 完整机器用 40/2/56；单节点组件入口则传 NODES=1。
@ac.module
def ReceiveLedger(
    *,
    NODES: int,
    EARLY_SLOTS: int,
    SEEN_SLOTS: int,
    TOTAL_EARLY: int,
    TOTAL_SEEN: int,
    ROW_BITS: int,
):
    # 容量诊断子模块只提供一个断言方法，把去重槽耗尽与协议故障区分开。
    # accounts 按节点索引；pending 把“节点×该节点槽位”展平成一维表。
    checks = ReceiveChecks()
    accounts = ac.table[NODES, Account](init=0)
    pending = ac.table[TOTAL_EARLY, Pending](init=0)
    # Each slot stores concat(source:u6, seq:u16), with slot zero at the low end.
    # 每节点把所有身份打包成一行：slot 0 放在最低 22 位，依次向高位排列。
    # 例如第 i 个身份位于 [22*i : 22*(i+1)]，其中低 16 位为 seq，高 6 位为 source。
    # 有效位单独保存在 seen_valid；RETIRE 一次清空有效位即可逻辑释放整行。
    # 遗留的身份位内容无须立刻清零，再分配槽时覆盖。
    identity_rows = ac.table[NODES, ac.bits[ROW_BITS]](init=0)
    # Retiring a context clears its validity in one update. Identity payloads
    # remain stored, as they did when validity lived in each identity entry.
    seen_valid = ac.table[NODES, ac.bits[SEEN_SLOTS]](init=0)

    # 检查调用者给的位宽能否精确容纳所有 22 位身份。
    # 用显式 u32 承接静态参数，在捕获的硬件检查里比较。
    @ac.method
    def require_layout() -> None:
        slot_count: ac.u32 = SEEN_SLOTS
        row_bits: ac.u32 = ROW_BITS
        assert row_bits == slot_count * 22, "identity row matches slot capacity"

    # 检查节点与表布局。后续 node % NODES 只是生成范围内的表索引，
    # 这里仍先验证原 node，避免把非法节点静默当成另一个合法节点。
    @ac.method
    def require_node(node: ac.u6) -> None:
        require_layout()
        node_count: ac.u16 = NODES
        early_count: ac.u16 = EARLY_SLOTS
        seen_count: ac.u16 = SEEN_SLOTS
        total_early: ac.u16 = TOTAL_EARLY
        total_seen: ac.u16 = TOTAL_SEEN
        assert node < NODES, "receive node in profile"
        assert total_early == node_count * early_count, "early table matches node capacity"
        assert total_seen == node_count * seen_count, "identity table matches node capacity"

    # 纯计算式的方法：重建一组 8 个身份，即 8×22=176 位。
    # 从高槽位向低槽位处理；每步把原行或 replacement 的一个 22 位身份接到低端。
    # ac.concat(a,b) 把 a 放高位、b 放低位；迭代后身份仍按原槽顺序排列。
    # 固定循环在本次调用中形成有限组合计算，不隐含 8 个执行周期。
    @ac.method
    def replace_identity_chunk(
        row: ac.bits[ROW_BITS], slot: ac.u8, replacement: ac.bits[22], base_slot: ac.u16
    ) -> ac.bits[176]:
        # Eight 22-bit identities form a bounded combinational concatenation.
        rebuilt: ac.bits[198] = 0
        selected_slot: ac.u16 = slot
        for index in range(8):
            forward: ac.u16 = index
            # reverse 是这一组内从高到低扫描的绝对槽号。
            # 只有槽号与 selected_slot 相同才换成新身份，其他槽从旧行原样保留。
            reverse: ac.u16 = base_slot + 7
            reverse = reverse - forward
            offset: ac.u16 = reverse * 22
            shifted = row >> offset
            word = replacement if selected_slot == reverse else shifted[0:22]
            rebuilt = ac.concat(rebuilt[0:176], word)
        return rebuilt[0:176]

    # 在整行中只替换一个去重槽。按每组 8 个身份分块，减少串接依赖深度。
    # (SEEN_SLOTS+7)>>3 等于向上取整的组数。最后取低 ROW_BITS 位丢掉补齐高位。
    # 用选择和拼接保留未修改位的 X/Z（未知/高阻）状态，而不把它们算术清零。
    @ac.method
    def replace_identity_row(
        row: ac.bits[ROW_BITS], slot: ac.u8, replacement: ac.bits[22]
    ) -> ac.bits[ROW_BITS]:
        require_layout()
        # Selection and concat preserve X/Z in both retained and replaced data.
        # Grouping bounds the dependency depth; high padding is discarded.
        rebuilt: ac.bits[ROW_BITS + 176] = 0
        for index in range((SEEN_SLOTS + 7) >> 3):
            forward: ac.u16 = index
            reverse: ac.u16 = ((SEEN_SLOTS + 7) >> 3) - 1
            reverse = reverse - forward
            base_slot: ac.u16 = reverse * 8
            chunk = replace_identity_chunk(row, slot, replacement, base_slot)
            rebuilt = ac.concat(rebuilt[0:ROW_BITS], chunk)
        return rebuilt[0:ROW_BITS]

    # 返回账户记录的 epoch。它是接收账户所属轮次；全局每类别轮次则在
    # control_ledger.py 的 EpochLedger 中保存，由主机器检查两者一致性。
    @ac.method
    def epoch_of(node: ac.u6) -> ac.u8:
        require_node(node)
        account = accounts[node % NODES]
        return account.epoch

    # WAIT 使用的匹配检查：账户必须仍活跃，类别和轮次都相同。
    @ac.method
    def matches(node: ac.u6, opcode: ac.u3, epoch: ac.u8) -> bool:
        require_node(node)
        account = accounts[node % NODES]
        return account.active and account.opcode == opcode and account.epoch == epoch

    # 只查询账户与早到槽占用，不消费请求、不增加计数。
    # base=node*EARLY_SLOTS，定位此节点在一维 pending 表内的第一个槽。
    @ac.method
    def observe(node: ac.u6) -> Result:
        require_node(node)
        account = accounts[node % NODES]
        base: ac.u16 = node
        slots: ac.u16 = EARLY_SLOTS
        base = base * slots
        occupancy: ac.u8 = 0
        # 这里只统计有效槽数量，不检查片段是否匹配，也不释放槽。
        for index in range(EARLY_SLOTS):
            wide_index_1: ac.u16 = index
            if pending[(base + wide_index_1) % (TOTAL_EARLY)].valid:
                occupancy = occupancy + 1
        return Result(
            accepted=False,
            status=9 if account.fault else 0,
            fault=account.fault,
            active=account.active,
            counted=account.counted,
            complete=account.active and account.counted >= account.expected,
            pending=occupancy,
        )

    # 按原顺序延续上一组的查找，保留有效位含 X/Z 时 found 和 slot 的四态结果。
    @ac.method
    def first_free_octet(occupied: ac.u8, base: ac.u8, previous: FreeSlot) -> FreeSlot:
        found = previous.found
        slot = previous.slot
        for lane in range(8):
            index: ac.u8 = lane
            used = (occupied >> index) & 1 != 0
            if not used and not found:
                found = True
                slot = base + index
        return FreeSlot(found=found, slot=slot)

    # 预检片段：扫描身份表找重复身份和第一个空槽，再检查范围、标志及累计容量。
    # 这一步不写账户/身份表，但会执行容量断言；容量不足会使外层事务失败。
    # 主机器在写载荷前调用它，确保所有远端落点都可接收。
    @ac.method
    def inspect(node: ac.u6, segment: Segment) -> Decision:
        require_node(node)
        account = accounts[node % NODES]
        status: ac.u4 = 0
        validity = seen_valid[node % NODES]
        identities = identity_rows[node % NODES]
        identity = ac.concat(segment.source, segment.seq)
        choice = FreeSlot(found=False, slot=0)
        # 尾组补齐位不能匹配，也不能分配，所以查重补 0，找空槽补 1。
        row_padding: ac.bits[176] = 0
        invalid: ac.u8 = 0
        occupied: ac.u8 = 255
        padded_rows = ac.concat(row_padding, identities)
        valid_rows = ac.concat(invalid, validity)
        occupied_rows = ac.concat(occupied, validity)
        duplicate = False
        for index in range((SEEN_SLOTS + 7) >> 3):
            group: ac.u16 = index
            row = padded_rows >> (group * 176)
            bit_offset = group * 8
            valid = valid_rows >> bit_offset
            used = occupied_rows >> bit_offset
            choice = first_free_octet(used[0:8], bit_offset[0:8], choice)
            hit = matching_octet(row[0:176], valid[0:8], identity)
            duplicate = duplicate or hit
        # Widen before addition: the comparison must not wrap at 64/32 bits.
        # 先加宽再求和：地址 64 位加长度用 65 位，计数字节 32 位加长度用 33 位。
        # 例如超出最大值的求和若先截成原位宽，可能回绕成小数而错误通过范围检查。
        address_bytes: ac.bits[65] = segment.nbytes
        address_capacity: ac.bits[65] = account.capacity
        end: ac.bits[65] = segment.address
        end = end + address_bytes
        limit: ac.bits[65] = account.dst
        limit = limit + address_capacity
        count_bytes: ac.u33 = segment.nbytes
        count_capacity: ac.u33 = account.capacity
        total: ac.u33 = account.counted
        total = total + count_bytes
        # 按顺序判定：先检查账户/类别/轮次/目的范围，再识别未提交、自身和重复片段，
        # 然后检查新增计数是否超过容量。顺序有意义：重复片段即使不计数，也要先满足
        # 当前账户与地址范围的基本条件。
        if (
            account.fault
            or not account.active
            or segment.opcode != account.opcode
            or segment.epoch != account.epoch
            or segment.opcode == 0
            or segment.opcode == 7
            or segment.address < account.dst
            or end > limit
        ):
            status = 9
        elif not segment.committed:
            status = 5
        elif segment.self_sourced:
            status = 4
        elif duplicate:
            status = 3
        elif total > count_capacity:
            status = 9
        else:
            # Assertion failure aborts the whole transaction before publication.
            # Retain existing identities, count and the pending entry on failure.
            # 新身份必须有空槽；这里用断言终止容量不足的事务。
            # 容量只会在上下文退休时整体释放，等待不会凭空产生空间。
            # 正常可接收返回 status=2，total 通过容量检查后再截回 32 位。
            checks.require_identity_capacity(choice.found)
            status = 2
        return Decision(status=status, slot=choice.slot, total=total[0:32])

    # 正式更新账本：再次 inspect，协议错误则锁存 account.fault；
    # 新片段则写身份、置有效位并更新 counted，三者随外层事务一起提交。
    # 重复/自身/未提交状态不进入新身份分支，因此不会重复增加计数。
    @ac.method
    def commit(node: ac.u6, segment: Segment) -> ac.u4:
        decision = inspect(node, segment)
        account = accounts[node % NODES]
        if decision.status == 9:
            account.fault = True
            accounts[node % NODES] = account
        elif decision.status == 2:
            identity_rows[node % NODES] = replace_identity_row(
                identity_rows[node % NODES],
                decision.slot,
                # 拼接顺序：source 在高 6 位，seq 在低 16 位，对应 inspect 中的字段切片。
                ac.concat(segment.source, segment.seq),
            )
            one: ac.bits[SEEN_SLOTS] = 1
            seen_valid[node % NODES] = seen_valid[node % NODES] | (one << decision.slot)
            account.counted = decision.total
            accounts[node % NODES] = account
        return decision.status

    # 逐次请求接口。顺序是：定位节点 → 扫描早到项 → 优先处理匹配早到项 →
    # 若没有待排空项，再处理本次 POST/DELIVER/RETIRE → 写回账户并返回观测。
    # 它是被调用的方法；是否每周期调用由上层 Rule 决定。
    @ac.method
    def process(node: ac.u6, request: Request) -> Result:
        require_node(node)
        account = accounts[node % NODES]
        early_base: ac.u16 = node
        early_width: ac.u16 = EARLY_SLOTS
        early_base = early_base * early_width
        assert node < NODES, "receive node in profile"
        early_slots: ac.u16 = EARLY_SLOTS
        seen_slots: ac.u16 = SEEN_SLOTS
        assert early_slots > 0 and early_slots <= 255, "explicit early capacity 1..255"
        assert seen_slots > 0 and seen_slots <= 255, "explicit seen capacity 1..255"
        accepted = False
        status: ac.u4 = 0
        found = False
        drain_slot: ac.u8 = 0
        free = False
        free_slot: ac.u8 = 0
        # 扫描本节点早到槽：找到第一个与当前活跃账户匹配的项，另找第一个空槽。
        # found/free 使搜索各自只选择一次。一次 process 最多排空一个匹配早到项。
        for index in range(EARLY_SLOTS):
            wide_index_3: ac.u16 = index
            entry = pending[(early_base + wide_index_3) % (TOTAL_EARLY)]
            if (
                account.active
                and entry.valid
                and entry.segment.opcode == account.opcode
                and entry.segment.epoch == account.epoch
                and not found
            ):
                found = True
                drain_slot = index
            if not entry.valid and not free:
                free = True
                free_slot = index
        if account.fault:
            status = 9
        # 匹配早到项优先于新请求，所以该分支 accepted 保持 False。
        # 先 commit，再清早到有效位；若 commit 的容量断言失败，清槽也不会发布。
        # 重新读取 account 是为了接着使用 commit 在同一事务中提出的账户更新。
        elif found:
            # A capacity assertion aborts before pending removal can commit.
            drain_wide: ac.u16 = drain_slot
            status = commit(node, pending[(early_base + drain_wide) % (TOTAL_EARLY)].segment)
            account = accounts[node % NODES]
            pending[(early_base + drain_wide) % (TOTAL_EARLY)].valid = False
        # POST：要求有效类别、非零轮次、expected<=capacity，且 expected 与
        # compiled_expected 一致。若账户尚未完成却以同类别/同轮次再次 POST，报故障。
        # 其他已占用情况返回状态 8，提示调用者本次暂未接受。
        elif request.kind == 1:
            if (
                request.segment.opcode == 0
                or request.segment.opcode == 7
                or request.segment.epoch == 0
                or request.expected > request.capacity
                or request.expected != request.compiled_expected
            ):
                accepted = True
                account.fault = True
                status = 9
            elif account.active:
                if (
                    account.opcode == request.segment.opcode
                    and account.epoch == request.segment.epoch
                    and account.counted < account.expected
                ):
                    accepted = True
                    account.fault = True
                    status = 9
                else:
                    status = 8
            else:
                accepted = True
                account.active = True
                # 空闲账户被完整初始化为新接收上下文，counted 从零开始。
                # 此时并未搬运数据；它只建立后续数据可被接收和计数的条件。
                account = Account(
                    active=True,
                    fault=False,
                    opcode=request.segment.opcode,
                    epoch=request.segment.epoch,
                    dst=request.segment.address,
                    capacity=request.capacity,
                    expected=request.expected,
                    counted=0,
                )
                status = 1
        # DELIVER：非法类别直接故障；与活跃账户匹配则 commit；
        # 尚无匹配账户时尝试放进早到槽；早到槽满返回 8，不接受本次请求。
        elif request.kind == 2:
            if request.segment.opcode == 0 or request.segment.opcode == 7:
                accepted = True
                account.fault = True
                status = 9
            elif (
                account.active
                and request.segment.opcode == account.opcode
                and request.segment.epoch == account.epoch
            ):
                status = commit(node, request.segment)
                account = accounts[node % NODES]
                accepted = True
            elif free:
                accepted = True
                free_wide: ac.u16 = free_slot
                pending[(early_base + free_wide) % (TOTAL_EARLY)] = Pending(
                    valid=True, segment=request.segment
                )
                status = 6
            else:
                status = 8
        # RETIRE：账户活跃且已达到 expected 才能释放。
        # 清 seen_valid 使所有旧身份失效；identity_rows 中的旧位串仍可留存。
        # 提前到达其他轮次的元数据不会在此处被全部删除。
        elif request.kind == 3:
            accepted = True
            if not account.active or account.counted < account.expected:
                account.fault = True
                status = 9
            else:
                account.active = False
                seen_valid[node % NODES] = 0
                status = 7
        # 最后写回本次账户，并重新计算有效早到槽数量用于返回。
        # 若排空了早到项，occupancy 使用本事务刚更新的有效位状态。
        accounts[node % NODES] = account
        occupancy: ac.u8 = 0
        for index in range(EARLY_SLOTS):
            wide_index_5: ac.u16 = index
            if pending[(early_base + wide_index_5) % (TOTAL_EARLY)].valid:
                occupancy = occupancy + 1
        return Result(
            accepted=accepted,
            status=status,
            fault=account.fault,
            active=account.active,
            counted=account.counted,
            complete=account.active and account.counted >= account.expected,
            pending=occupancy,
        )
