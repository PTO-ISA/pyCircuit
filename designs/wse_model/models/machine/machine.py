# ruff: noqa: N802, N803, F823, F841 -- captured hardware state.
"""Runtime-programmed, serialized WSE communication functional profile.

One native byte memory stores forty disjoint logical 64 KiB banks. Its shared
service port and the walker steps are functional serialization, not physical
RAM/NoC timing. Runtime programs can address only node-local ranges.
"""

# 阅读入口：这份文件把 WSE（Wafer-Scale System，晶圆级系统）的通信行为
# 描述成可编译的电路。它是通信功能模型：负责程序、内存、路由与接收协议。
# 这里的串行步骤用于把功能做对；实际片上网络的并发带宽和物理延迟另行分析。
#
# 建议顺序：先读本段 → machine_core.py 的 HostRequest/Command、BankedMemory → 本文件的 WseMachine。
# 遇到子模块再打开 ../calendar/ 下的 control_ledger.py、forward.py、ledger.py。
#
# 第一次读 pyCircuit，需要先分清以下几件事：
# 1. import pycircuit as ac：ac 只是库的别名。此文件使用 Python 语法描述硬件，
#    由编译器捕获并生成电路，并不是用普通 Python 逐条模拟这些函数。
# 2. @ac.struct：声明一组有名字、有固定位宽的信号字段。它用于模块之间传数据，
#    不是一个带软件对象指针的容器。ac.u6 表示 6 位无符号数，范围 0..63；
#    ac.u32 是 32 位无符号数；ac.bits[80] 是 80 位位向量；bool 是布尔条件。
#    固定位宽加法可能回绕，因此下面检查地址上界时常先加宽一位再相加。
# 3. 位切片 x[a:b] 取第 a 到 b-1 位，最低位编号为 0。
#    例如 x[0:3] == 0 意味着最低三位为零，即地址是 8 的整数倍。
# 4. @ac.module：声明硬件模块。模块体中分配子模块、内存和持久状态。
#    例如 stage: ac.u5 = 0 是跨周期保存的状态；普通表达式绑定是连线/计算结果。
# 5. @ac.rule：声明由编译器注册的自主规则。Rule 内按语句顺序计算，后面的语句
#    能看到前面提出的新值；状态更新在事务成功时提交。独立 Rule 观察已提交状态。
#    if/elif 每次只进入一个分支；即使分支里改了 stage，也不会立刻再进入另一个
#    elif 阶段。下一次执行才按新的 stage 推进。局部变量不会自动跨周期保存。
# 6. @ac.method：子模块提供的可调用操作。它在调用者的事务内读写该子模块的状态；
#    不是调用一次就另过一个时钟周期。断言和资源检查决定整个事务能否提交。
# 7. return execute 返回嵌套 Rule 的结果连接；不是在宿主 Python 中调用 execute()。
#    同理，memory = BankedMemory(...) 既保留该实例身份，也绑定它的读输出，
#    所以 payload = memory 取输出，而 memory.write(...) 调用同一实例的方法。
# 8. ac.table 是固定大小的持久状态表；ac.byte_mem 是以字节地址访问的存储。
#    byte_mem.read 读当前已提交的数据；write 提出写操作，随成功事务提交。
#    对 clock/reset（时钟/复位）和提交控制的底层接线由编译器生成。
#
# 本文件存在两层命令，不要混淆：
# - host.kind：宿主驱动怎样装载数据、装载程序、启动和读回。
# - command.kind：机器从程序内存取出后执行的通信指令。
# 一次完整运行：宿主装数据 → 装程序 → 启动 → 机器逐条执行 → 完成后宿主读回。
#
# 阶段速查（stage 是状态机状态，即控制器记住自己执行到哪里）：
# 0 空闲：等待宿主装载或启动。
# 1 取指/控制：安装配置、推进轮次、POST、WAIT、RETIRE、END，或启动 COPY。
# 2 路由：逐节点遍历路径，检查连通树，收集所有远端目的。
# 3 预检：扫描 40 个节点，确认目的接收范围、轮次、容量和身份。
# 4 读：从源内存取最多 64 字节，保存到 payload。
# 5 写：把同一份 payload 逐个写到目的；本拍结束再读下一拍。
# 6 完成：允许宿主读回结果。7 故障：停止正常推进，允许诊断读回。
# “一个数据拍”在这里就是一次处理的 64 字节（短片段或非 64 字节对齐时为 8 字节），不等于真实网络的传输粒度。
#
# 具体例子：把节点 0 的 16 字节复制到节点 1，不包含源本地副本。
# 程序先为相关节点安装类别、推进轮次，并 POST 节点 1 的 16 字节接收账户。
# COPY 在 stage=1 被读出，随后 stage=2 走过 0→1，targets 只含节点 1。
# stage=3 完成预检后，offset=0：stage=4 读前 8 字节，stage=5 写节点 1。
# 还没到最后一拍，所以 counted 此时仍为 0；offset 变成 8，再走 stage=4/5。
# 后 8 字节写入时，账本一次增加整个片段的 16 字节，pc 前进，回到 stage=1。
# 后续 WAIT 看到完成，RETIRE 释放账户，END 才把机器置成 stage=6。
# 若写阶段目的许可为 0，offset、写进度与计数保持；许可恢复后接着原位置推进。
# 这个例子说明功能先后关系；运行步数还包含取指、遍历、预检等控制开销。

import pycircuit as ac
from wse_calendar.control_ledger import CalRegLedger, CalRegRequest, EpochLedger, EpochRequest
from wse_calendar.forward import ForwardHop, HopInput
from wse_calendar.ledger import ReceiveLedger, Request
from wse_machine.machine_core import (
    BankedMemory,
    Command,
    HostRequest,
    MachineResult,
    ReadAddress,
    decode,
    first_node,
    segment_of,
    transfer_size,
)


# ndf: implements=WSEMIG-MACHINE-SHARED-001,WSEMIG-MACHINE-PROGRAM-001
# 主机器：分配下面所有持久资源，并用一个 execute Rule 顺序控制它们。
# 这里没有写 AllGather 专有的行阶段/列阶段。具体地址和多播树来自运行时程序。
@ac.module
def WseMachine(host: HostRequest) -> MachineResult:
    # 程序内存：65536 字节 / 每条 64 字节 = 最多 1024 条指令。
    # 读一次得到整条 512 位指令；宿主可一次写整条 64 字节，也支持 8 字节窄写。
    program = ac.byte_mem[ac.bits[512]](depth=65536)
    # 实例化子模块会给当前机器创建独立的状态。
    # ReceiveLedger 保存 40 个接收账户、80 个早到元数据槽、2240 个去重身份槽。
    # ROW_BITS=56*22：每个身份由 6 位源节点号和 16 位片段序号组成。
    # EpochLedger 按节点/通信类别保存轮次；CalRegLedger 保存已安装类别的位图。
    ledger = ReceiveLedger(
        NODES=40, EARLY_SLOTS=2, SEEN_SLOTS=56, TOTAL_EARLY=80, TOTAL_SEEN=2240, ROW_BITS=1232
    )
    epochs = EpochLedger()
    configuration = CalRegLedger()
    # 以下带类型的模块级变量是持久状态，成功提交后保留到下一周期。
    # pc 指向下一条要执行的指令；count 是本次启动装入的实际指令数。
    pc: ac.u16 = 0
    count: ac.u16 = 0
    # 0 idle, 1 execute, 2 route, 3 preflight, 4 read, 5 write, 6 done, 7 fault.
    # stage=2..5 的拆分让载荷和遍历进度能跨模型周期保存。
    stage: ac.u5 = 0
    # selected：路由遍历、预检查或数据写入当前处理哪个节点。
    # offset：当前 COPY 内已经走到的源/目的字节偏移，每个数据拍增加 beat_bytes（64 或 8）。
    # current：保存已取出的整条指令，供后续多个阶段使用。
    selected: ac.u6 = 0
    offset: ac.u32 = 0
    # Command() 构造初始零值字段；实际指令在取指阶段覆盖 current。
    current: Command = Command(
        kind=0,
        node=0,
        opcode=0,
        epoch=0,
        seq=0,
        source=0,
        src=0,
        dst=0,
        nbytes=0,
        capacity=0,
        expected=0,
        compiled_expected=0,
        route=0,
        route_high=0,
        flags=0,
    )
    # 以下 u40 是节点集合位图；用 | 增加成员，用 & ~ 清除成员。
    # active_nodes：仍持有接收上下文的节点；END 要求它为空。
    # frontier：已发现但还没访问的路由节点；visited：已经访问的路由节点。
    # targets：本次 COPY 要落地的节点；remote_mask：最终远端目标的固定集合。
    # beat_remaining：本个数据拍还没写到的目标，会逐个清除。
    # parents：记录路由遍历的来路，转发时排除父节点，避免立即沿原路返回。
    # payload：把源读出的最多 64 字节存到跨周期状态，供后续多个目的写入复用。
    active_nodes: ac.u40 = 0
    frontier: ac.u40 = 0
    visited: ac.u40 = 0
    targets: ac.u40 = 0
    remote_mask: ac.u40 = 0
    beat_remaining: ac.u40 = 0
    parents = ac.table[40, ac.u6](init=0)
    payload: ac.bits[512] = 0
    # 故障码：1 宿主请求，2 指令格式/程序越界，3 配置安装，4 推进轮次，
    # 5 接收/配置/轮次匹配，6 路由，7 COPY 参数，9 END 时接收账户仍活跃。
    # 这是本模型的诊断编码。出现故障后 stage=7，供宿主读回定位。
    fault_code: ac.u8 = 0
    read_bytes: ac.u64 = 0
    write_bytes: ac.u64 = 0
    # 源/目的与偏移在一拍所有落点完成前保持不变，因此拍长由当前状态直接确定。
    beat_bytes = transfer_size(current.src + offset, current.dst + offset, current.nbytes - offset)
    # 普通表达式绑定是由输入和状态算出的条件，没有再分配一份持久状态。
    # 宿主仅能在空闲、完成或故障时读内存，防止和执行期的源读取争用接口。
    # 无效节点的观测选择先回退到节点 0，最终返回处再屏蔽无效观测。
    host_node_valid = host.node < 40
    host_address_valid = host.address <= 65528 and host.address[0:3] == 0
    host_address_valid = host_address_valid and (not host.wide or host.address[0:6] == 0)
    host_read_window = stage == 0 or stage == 6 or stage == 7
    host_read = host.kind == 4 and host_node_valid and host_address_valid and host_read_window
    observation_node = host.node if host_node_valid else 0
    # 选择本周期的内存读地址：优先合法宿主读，其次 stage=4 的 COPY 源读，
    # 其他情况送安全的节点 0/地址 0。子模块的读输出供 execute 使用。
    # 这是模块间连线选择，不是宿主在这里提前读取 Python 数组。
    memory = BankedMemory(
        ReadAddress(
            node=host.node if host_read else current.source[0:6] if stage == 4 else 0,
            address=(host.address if host_read else current.src + offset if stage == 4 else 0),
        )
    )
    # 转发子模块一直连接 current 指令、selected 节点和该节点的父节点。
    # 主规则在 stage=2 消费其输出：能去哪些相邻节点、当前节点是否落地、是否非法。
    # 父节点索引取模用于形成表索引；节点合法性仍由协议检查保证。
    hop = ForwardHop(
        HopInput(
            route=current.route,
            node=selected,
            has_ingress=selected != current.source[0:6],
            ingress=parents[selected % 40],
        )
    )

    # 整个主控制器只有这一条自主规则。它先处理宿主装载/启动请求，
    # 再按已保存的 stage 处理一个阶段，最后构造本次观测输出。
    # 以下分支中更新的 pc/stage/表/内存属于同一外层事务，成功后共同提交。
    @ac.rule
    def execute() -> MachineResult:
        # 用显式 40 位的 1 生成节点位图，避免使用过窄的整数再移位。
        # 例如 one << 33 可以表示节点 33。
        one: ac.u40 = 1
        host_in_range = host_node_valid and host_address_valid
        # 先拒绝非法宿主请求。host.node 即使在无操作时也要求落在 0..39。
        # 故障状态会保持，直到外部复位；这里没有自动清错指令。
        if (
            host.kind > 5
            or not host_node_valid
            or (host.kind == 4 and (not host_address_valid or not host_read_window))
        ):
            fault_code = 1
            stage = 7
        # 宿主写数据：只在 stage=0 允许。每条请求写节点内的 64 或 8 字节。
        # 这用于初始化载荷，属于运行准备，不是 COPY 路由执行。
        elif host.kind == 1:
            if stage != 0 or not host_in_range:
                fault_code = 1
                stage = 7
            else:
                memory.write(host.node, host.address, host.data, host.wide)
        # 宿主写程序：宽请求一次装好 64 字节；窄请求只更新低八个字节通道。
        elif host.kind == 2:
            if stage != 0 or not host_address_valid:
                fault_code = 1
                stage = 7
            else:
                word: ac.bits[512] = host.data
                program_mask: ac.u64 = 18446744073709551615 if host.wide else 255
                program.write(host.address[0:16], word, mask=program_mask)
        # 宿主启动：保存实际指令数并进入取指阶段。本 A 实现仅从初始空闲启动；
        # pc 的初值是 0。完成后重新开始另一程序的方式由外部驱动复位处理。
        elif host.kind == 3:
            if stage != 0 or host.program_count == 0 or host.program_count > 1024:
                fault_code = 1
                stage = 7
            else:
                count = host.program_count
                stage = 1
        # 阶段 1：取指和执行简单控制指令。
        # 每次从 pc*64 字节地址读一条指令，并保存到 current。
        # 直接完成的指令令 pc+1；COPY 则转去阶段 2..5，完成后才令 pc+1。
        elif stage == 1:
            if pc >= count:
                fault_code = 2
                stage = 7
            else:
                address: ac.u16 = pc
                command = decode(program.read(address * 64))
                current = command
                if command.kind == 0 or command.kind > 7 or command.flags > 1:
                    fault_code = 2
                    stage = 7
                # END：只有所有 POST 建立的接收账户都已经 RETIRE，才能正常结束。
                # 否则锁存故障 9，避免把仍有协议状态的运行误报为完成。
                elif command.kind == 7:
                    if active_nodes != 0:
                        fault_code = 9
                        stage = 7
                    else:
                        stage = 6
                elif command.node >= 40 or (
                    command.kind != 6 and (command.opcode == 0 or command.opcode >= 7)
                ):
                    fault_code = 2
                    stage = 7
                else:
                    node = command.node[0:6]
                    opcode = command.opcode[0:3]
                    # 先读当前节点账户；drained 表示没有活跃账户，也没有早到元数据。
                    # 配置安装和轮次回绕等操作需要这些旧通信状态已清空。
                    observed = ledger.observe(node)
                    drained = not observed.active and observed.pending == 0
                    # INSTALL：把该 opcode 安装进节点的配置位图。
                    # 此模块验证“配置存在且允许安装”，具体路由内容仍在 COPY 指令内。
                    if command.kind == 1:
                        installed = configuration.process(
                            node, CalRegRequest(kind=2, opcode=opcode, drained=drained)
                        )
                        if installed.fault:
                            fault_code = 3
                            stage = 7
                        else:
                            pc = pc + 1
                    # ADVANCE：旧接收状态排空后推进该节点/类别的 epoch（通信轮次）。
                    # 轮次由子模块保存；这条指令成功才前进到下一条。
                    elif command.kind == 2:
                        if not drained:
                            fault_code = 4
                            stage = 7
                        else:
                            epoch = epochs.process(
                                node,
                                EpochRequest(advance=True, opcode=opcode, drained=drained),
                            )
                            if epoch.fault:
                                fault_code = 4
                                stage = 7
                            else:
                                pc = pc + 1
                    # POST：先确认类别已安装、轮次一致，接收区域在节点 64 KiB 范围内且对齐。
                    # 用 33 位计算 dst+capacity，防止 32 位加法溢出后看似落回合法范围。
                    # 之后才建立接收账户：目的起址、容量和本轮预期远端字节数。
                    elif command.kind == 3:
                        installed_post = configuration.process(
                            node, CalRegRequest(kind=3, opcode=opcode, drained=drained)
                        )
                        epoch_post = epochs.process(
                            node,
                            EpochRequest(advance=False, opcode=opcode, drained=drained),
                        )
                        end: ac.u33 = command.dst
                        capacity: ac.u33 = command.capacity
                        end = end + capacity
                        if (
                            command.epoch == 0
                            or command.epoch > 255
                            or epoch_post.value != command.epoch[0:8]
                            or installed_post.fault
                            or command.dst >= 65536
                            or command.dst[0:3] != 0
                            or end > 65536
                        ):
                            fault_code = 5
                            stage = 7
                        else:
                            post = ledger.process(
                                node,
                                Request(
                                    kind=1,
                                    segment=segment_of(command),
                                    capacity=command.capacity,
                                    expected=command.expected,
                                    compiled_expected=command.compiled_expected,
                                ),
                            )
                            if post.fault or not post.accepted:
                                fault_code = 5
                                stage = 7
                            else:
                                # 账本接受 POST 后，主机器同步标记此节点有活跃上下文。
                                active_nodes = active_nodes | (one << node)
                                pc = pc + 1
                    # COPY：在搬运前检查整个源/目的范围、对齐、长度、身份和路由高位。
                    # flags=1 时会向源节点也写本地副本，因此额外检查源/目的区域重叠。
                    # 同址复制允许；其他部分重叠会被拒绝。加宽计算防止地址和长度求和回绕。
                    elif command.kind == 4:
                        source_end: ac.u33 = command.src
                        destination_end: ac.u33 = command.dst
                        source_start: ac.u33 = command.src
                        destination_start: ac.u33 = command.dst
                        length: ac.u33 = command.nbytes
                        source_end = source_end + length
                        destination_end = destination_end + length
                        overlap = (
                            source_start < destination_end
                            and destination_start < source_end
                            and command.src != command.dst
                        )
                        if (
                            command.source >= 40
                            or command.epoch == 0
                            or command.epoch > 255
                            or command.seq > 65535
                            or command.route_high > 65535
                            or command.src[0:3] != 0
                            or command.dst[0:3] != 0
                            or command.nbytes == 0
                            or command.nbytes[0:3] != 0
                            or source_end > 65536
                            or destination_end > 65536
                            or (command.flags == 1 and overlap)
                        ):
                            fault_code = 7
                            stage = 7
                        else:
                            # 源节点也必须安装此通信类别，且源节点当前轮次要与命令一致。
                            # 通过后清空本次遍历用的集合，选择源节点，转入 stage=2。
                            source_admission = configuration.process(
                                command.source[0:6],
                                CalRegRequest(kind=3, opcode=opcode, drained=False),
                            )
                            source_epoch = epochs.process(
                                command.source[0:6],
                                EpochRequest(advance=False, opcode=opcode, drained=False),
                            )
                            if source_admission.fault or source_epoch.value != command.epoch[0:8]:
                                fault_code = 5
                                stage = 7
                            else:
                                selected = command.source[0:6]
                                offset = 0
                                frontier = 0
                                visited = 0
                                targets = 0
                                stage = 2
                    # WAIT：必须等待正确节点、类别和轮次的接收账户。
                    # 未完成时既不令 pc+1，也不改变 stage；后续周期继续检查同一条指令。
                    # 此串行机器不会越过 WAIT 去执行后面的 COPY，程序本身必须安排好依赖。
                    elif command.kind == 5:
                        epoch_wait = epochs.process(
                            node,
                            EpochRequest(advance=False, opcode=opcode, drained=drained),
                        )
                        matching = ledger.matches(node, opcode, command.epoch[0:8])
                        if (
                            command.epoch == 0
                            or command.epoch > 255
                            or not matching
                            or epoch_wait.value != command.epoch[0:8]
                        ):
                            fault_code = 5
                            stage = 7
                        elif observed.complete:
                            pc = pc + 1
                    # RETIRE：要求接收已完成，再释放账户及其去重有效位。
                    # 成功时主机器也清掉 active_nodes 对应位，END 才能确认所有账户已释放。
                    elif command.kind == 6:
                        retired = ledger.process(
                            node,
                            Request(
                                kind=3,
                                segment=segment_of(command),
                                capacity=0,
                                expected=0,
                                compiled_expected=0,
                            ),
                        )
                        if retired.fault:
                            fault_code = 5
                            stage = 7
                        else:
                            active_nodes = active_nodes & ~(one << node)
                            pc = pc + 1
        # 阶段 2：在写任何载荷前遍历完整路径，并收集远端落点。
        # 每次处理 selected 一个节点，把它的相邻参与节点加入 frontier。
        # 已访问或已在 frontier 的节点再次被其他边发现，表示树形约束被破坏。
        # 此模型要求参与节点形成从源可达的树；这些检查会拒绝环路和断开的路径。
        elif stage == 2:
            bit = one << selected
            if hop.fault or (visited & bit) != 0:
                fault_code = 6
                stage = 7
            else:
                visited = visited | bit
                if hop.lands and selected != current.source[0:6]:
                    targets = targets | bit
                collision = False
                passing: ac.u40 = 0
                # 这是固定界限循环：编译器把本次计算中 40 个节点的判定表示为电路。
                # 它不是每个 peer 自动耗费一个模型周期。跨周期遍历由 selected/stage 控制。
                # passing 汇总整张路由位图的 P 位；hop.egress 则只包含当前节点的出口。
                for peer in range(40):
                    peer_index: ac.u6 = peer
                    peer_bit = one << peer_index
                    route_offset: ac.u7 = peer_index
                    route_pair = current.route >> (route_offset * 2)
                    if route_pair[1:2] != 0:
                        passing = passing | peer_bit
                    if (hop.egress & peer_bit) != 0:
                        if ((visited | frontier) & peer_bit) != 0:
                            collision = True
                        else:
                            frontier = frontier | peer_bit
                            parents[peer] = selected
                if collision:
                    fault_code = 6
                    stage = 7
                # 待访问集合为空时，比较 visited 与整张路径的 passing 集合。
                # 相等说明所有被标记参与的节点都从源访问到了；随后开始逐节点预检查。
                elif frontier == 0:
                    if visited != passing:
                        fault_code = 6
                        stage = 7
                    else:
                        selected = 0
                        stage = 3
                else:
                    selected = first_node(frontier)
                    frontier = frontier & ~(one << selected)
        # 阶段 3：依次扫描节点 0..39，预检每个远端落点。
        # 确认目的配置与轮次正确、没有待处理早到项、接收范围与计数规则允许。
        # inspect 不增加接收计数。重复身份从 targets 中删除，避免重复写数据。
        # 所有目标通过后才读源载荷，避免先写一部分目的再发现其他目标有协议错误。
        elif stage == 3:
            target_bit = one << selected
            if (targets & target_bit) != 0:
                receive = ledger.observe(selected)
                admission = configuration.process(
                    selected,
                    CalRegRequest(kind=3, opcode=current.opcode[0:3], drained=False),
                )
                receive_epoch = epochs.process(
                    selected,
                    EpochRequest(advance=False, opcode=current.opcode[0:3], drained=False),
                )
                decision = ledger.inspect(selected, segment_of(current))
                if (
                    admission.fault
                    or receive_epoch.value != current.epoch[0:8]
                    or receive.pending != 0
                    or decision.status == 9
                ):
                    fault_code = 5
                    stage = 7
                elif decision.status == 3:
                    targets = targets & ~target_bit
                elif decision.status != 2:
                    fault_code = 5
                    stage = 7
            if stage == 3:
                if selected == 39:
                    # 保留远端目标全集用于每拍动态许可检查，然后按 flags 选择是否加源本地副本。
                    # 源副本不走远端接收计数。若所有远端均重复且不需要本地副本，此 COPY 直接完成。
                    remote_mask = targets
                    if current.flags == 1:
                        targets = targets | (one << current.source[0:6])
                    if targets == 0:
                        pc = pc + 1
                        stage = 1
                    else:
                        stage = 4
                else:
                    selected = selected + 1
        # 阶段 4：从 current.src+offset 读源节点的最多 64 字节，并保存到 payload。
        # 把全部目标复制到 beat_remaining，下一阶段逐个写同一份数据。
        # 因此多播在此功能实现中是“读一次，串行写多个目的”。
        elif stage == 4:
            payload = memory
            read_increment: ac.u64 = beat_bytes
            read_bytes = read_bytes + read_increment
            beat_remaining = targets
            selected = first_node(targets)
            stage = 5
        # 阶段 5：将 payload 写到 selected 节点的 current.dst+offset。
        # 只有 remote_mask 中所有远端目标都给许可才写；任一个撤销许可，整个阶段保持。
        # 这是对固定远端集合的检查，已写过的目标也仍在本拍许可集合里。
        elif stage == 5:
            if (host.landing_permit & remote_mask) == remote_mask:
                memory.write(selected, current.dst + offset, payload, beat_bytes == 64)
                write_increment: ac.u64 = beat_bytes
                write_bytes = write_bytes + write_increment
                # 只有该目的最后一个数据拍才提交整片段的身份与字节计数。
                # 数据末拍写入和 ledger.commit 处于同一事务，成功后一起可见。
                # 本地源副本跳过远端账本。断言要求预检成功的独占接收只被计数一次。
                if offset + beat_bytes == current.nbytes and selected != current.source[0:6]:
                    committed = ledger.commit(selected, segment_of(current))
                    assert committed == 2, "exclusive prepared receive commits once"
                # 写完一个目的清掉其位：还有目的就换 selected；本拍全写完后，
                # 若整个片段也结束则 pc+1 回 stage=1，否则 offset+beat_bytes 回 stage=4 读下一拍。
                beat_remaining = beat_remaining & ~(one << selected)
                if beat_remaining != 0:
                    selected = first_node(beat_remaining)
                elif offset + beat_bytes == current.nbytes:
                    pc = pc + 1
                    stage = 1
                else:
                    offset = offset + beat_bytes
                    stage = 4

        # 汇总观测输出。默认 data=0，合法宿主读才返回存储输出。
        # 账本观测选择 host.node，而不是 selected，因此宿主可指定要看哪个接收账户。
        # 这里是按 Rule 内当前计算值形成的输出；持久更新仍由事务提交控制。
        data: ac.bits[512] = 0
        if host_read:
            narrow_data: ac.bits[512] = memory[0:64]
            data = memory if host.wide else narrow_data
        account = ledger.observe(observation_node)
        account_epoch = ledger.epoch_of(observation_node)
        return MachineResult(
            data=data,
            pc=pc,
            stage=stage,
            selected_node=selected,
            offset=offset,
            counted=account.counted if host_node_valid else 0,
            active=account.active and host_node_valid,
            complete=account.complete and host_node_valid,
            pending=account.pending if host_node_valid else 0,
            epoch=account_epoch if host_node_valid else 0,
            fault=stage == 7,
            fault_code=fault_code,
            done=stage == 6,
            read_bytes=read_bytes,
            write_bytes=write_bytes,
        )

    # 把自主规则的返回值接到 WseMachine 输出。Rule 已在定义时注册，
    # 不需要写 execute() 来触发，也不需要在这里手写时钟或寄存器接线。
    return execute
