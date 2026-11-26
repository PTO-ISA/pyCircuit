# ruff: noqa: N802 -- pyCircuit module names.
"""Host and program interfaces, banked memory and helpers shared by both WSE machines."""

import pycircuit as ac
from wse_calendar.ledger import Segment


# 宿主到机器的请求接口。字段组合是硬件输入，不是文件路径。
# kind：0 无宿主操作；1 写数据；2 写程序的一段；3 启动；4 读数据。
# 当前分支也允许 kind=5，但它没有独立处理分支，会继续按 stage 执行。
# node/address：节点编号与节点内字节地址；data：512 位数据；wide=True 传 64 字节，否则低 64 位传 8 字节。
# program_count：启动时指定实际指令条数，允许 1..1024。
# landing_permit：40 位目的写入许可，第 n 位控制节点 n 的远端落点许可。
# 它供暂停/背压测试使用，不代表宿主提前提供待搬运的数据。
@ac.struct
class HostRequest:
    kind: ac.u3
    node: ac.u6
    address: ac.u32
    wide: bool
    data: ac.bits[512]
    program_count: ac.u16
    landing_permit: ac.bits[40]


# 机器返回的观测值。pc 是程序计数器（当前指令序号），stage 是执行阶段。
# selected_node/offset：当前遍历或写入的节点，以及片段内的字节偏移。
# counted/active/complete/pending/epoch：宿主所选节点的接收账户状态。
# fault/fault_code：机器是否故障及原因；done：是否到达正常结束状态。
# read_bytes/write_bytes：模型实际读/写的数据字节累计，源读一次可写多个目的。
# 这些字段用于功能检查；累计字节与执行周期须分别解读。
@ac.struct
class MachineResult:
    data: ac.bits[512]
    pc: ac.u16
    stage: ac.u5
    selected_node: ac.u6
    offset: ac.u32
    counted: ac.u32
    active: bool
    complete: bool
    pending: ac.u8
    epoch: ac.u8
    fault: bool
    fault_code: ac.u8
    done: bool
    read_bytes: ac.u64
    write_bytes: ac.u64


# 一条运行时指令占 512 位，即 64 字节。大多数字段按 32 位装载。
# kind：1 安装配置，2 推进轮次，3 POST 接收登记，4 COPY 搬运，
#       5 WAIT 等待接收完成，6 RETIRE 释放接收账户，7 END 结束程序。
# node：配置/接收操作针对的节点；opcode：通信类别编号，合法类别为 1..6。
# epoch：该类别的通信轮次；seq：片段序号；两者帮助识别数据所属的通信。
# source/src/dst/nbytes：源节点、源字节地址、目的字节地址、片段字节数。
# capacity：接收区域容量；expected：本轮应接收的远端字节数；
# compiled_expected：程序生成端给出的预期值，POST 时要求二者相同。
# route：每节点两个标志，共 40×2=80 位；低位 L 表示落地，高位 P 表示参与路径。
# route_high 同时覆盖原编码的高 32 位，用于检查路由字段剩余高位是否越界。
# flags=1 额外写源节点本地副本；flags=0 只写远端落点。
@ac.struct
class Command:
    kind: ac.u32
    node: ac.u32
    opcode: ac.u32
    epoch: ac.u32
    seq: ac.u32
    source: ac.u32
    src: ac.u32
    dst: ac.u32
    nbytes: ac.u32
    capacity: ac.u32
    expected: ac.u32
    compiled_expected: ac.u32
    route: ac.bits[80]
    route_high: ac.u32
    flags: ac.u32


# 把指令位串拆成有名字的字段。这里全是位选和连线，没有跨周期状态。
# 注意 route 与 route_high 有 16 位重叠：前者取完整 80 位路径，后者还用于
# 检查该 32 位编码字中超过 40 节点路由范围的高 16 位。
def decode(raw: ac.bits[512]) -> Command:
    return Command(
        kind=raw[0:32],
        node=raw[32:64],
        opcode=raw[64:96],
        epoch=raw[96:128],
        seq=raw[128:160],
        source=raw[160:192],
        src=raw[192:224],
        dst=raw[224:256],
        nbytes=raw[256:288],
        capacity=raw[288:320],
        expected=raw[320:352],
        compiled_expected=raw[352:384],
        route=raw[384:464],
        route_high=raw[448:480],
        flags=raw[480:512],
    )


# 位图的第 n 位代表节点 n。数末尾连续零的个数就得到最低有效位的节点号。
# 例如 0b101000 返回 3。调用处保证有待选节点，不把全零结果当有效节点。
def first_node(mask: ac.u40) -> ac.u6:
    return ac.count_trailing_zeros(mask)


# 从指令构造接收账本所需的元数据。Segment 是片段描述，不包含载荷字节。
# 位宽截取在机器相应路径的完整值检查配合下使用。
# committed=True 表示此描述按已完成片段参与账本判定；inspect 只检查，
# 真正增加计数的 commit 在最后一个数据拍写入时调用。
# self_sourced=False：这份描述给远端接收使用，源本地副本另走 flags 分支。
def segment_of(command: Command) -> Segment:
    return Segment(
        opcode=command.opcode[0:3],
        epoch=command.epoch[0:8],
        address=command.dst,
        nbytes=command.nbytes,
        source=command.source[0:6],
        seq=command.seq[0:16],
        committed=True,
        self_sourced=False,
    )


# 给存储子模块的读地址：先给逻辑节点，再给节点内地址。
# 它把两个总是一起使用的字段组合成一个结构化接口。
@ac.struct
class ReadAddress:
    node: ac.u6
    address: ac.u32


# 40 个节点各 65536 字节，总计 2621440 字节。
# 逻辑上节点独占各自区域；实现上把它们放进一份字节存储，方便功能仿真。
# 读/写类型 ac.bits[512] 意味着端口宽度为 64 字节，但 depth 和地址仍以字节计。
# 例如节点 1 的地址 16，对应底层地址 1*65536+16。
# 读操作对齐到 64 字节边界，再选出请求起始字节；窄写用字节掩码保留邻居。
# 共享服务方式是此功能实现的选择，不用来推断真实芯片存储端口拓扑。
@ac.module
def BankedMemory(request: ReadAddress) -> ac.bits[512]:
    memory = ac.byte_mem[ac.bits[512]](depth=2621440)

    # 自主读规则：检查节点、8 字节对齐、末地址不超过 65528。
    # 65528+8=65536，恰好读到节点区域末尾。u22 足以表示整份内存地址。
    # 左移 16 位等于乘 65536，随后加节点内地址得到底层字节地址。
    @ac.rule
    def load() -> ac.bits[512]:
        assert (
            request.node < 40 and request.address <= 65528 and request.address[0:3] == 0
        ), "node-local read range"
        bank: ac.u22 = request.node
        local: ac.u22 = request.address[0:22]
        aligned = local & 4194240
        shift: ac.u9 = request.address[0:6]
        return memory.read((bank << 16) + aligned) >> (shift * 8)

    # 可调用写方法：与读规则使用相同地址布局。None 表示没有数据返回。
    # write 的实际写入随调用者事务提交；遇到断言失败不会只提交一部分效果。
    @ac.method
    def write(node: ac.u6, address: ac.u32, data: ac.bits[512], wide: bool) -> None:
        assert node < 40 and address <= 65528 and address[0:3] == 0, "node-local write range"
        bank: ac.u22 = node
        local: ac.u22 = address[0:22]
        assert not wide or address[0:6] == 0, "wide write alignment"
        mask: ac.u64 = 18446744073709551615 if wide else 255
        memory.write((bank << 16) + local, data, mask=mask)

    return load


def transfer_size(source: ac.u32, target: ac.u32, remaining: ac.u32) -> ac.u32:
    return 64 if source[0:6] == 0 and target[0:6] == 0 and remaining >= 64 else 8
