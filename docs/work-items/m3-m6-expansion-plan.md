# M3/M6 后续扩展计划

修订：A，2026-10-01。状态：规划冻结候选，独立结论见
[审阅记录](../reviews/20261001-m3-m6-expansion-review.md)；不是接口批准或实施完成。
产品基线：`4584ad0b`，分支 `codex/gfsim-source-units`；规划基线：`be68c6b3`。
用户要求：继续扩展 M3 和 M6 的计划，保留有界交付、后续补齐的原则。

## 1. 已完成范围与边界

M5 标量 profile、M6-01 发布进程恢复/前缀迁移、M6-02 两轴规模/增量基线、
M7-01 本地 macOS 预览已验收。这些不是全能力矩阵通过，不能再次排成未完成主干。

当前路线固定为：每源 Python capture → MLIR 语义/推导 → body/header → link
→ 同一 verified final IR → C++/Verilog。模块函数、嵌套 rule、普通值和注解
保持 Pythonic；不引入 self、公开 Queue/FIFO/Reg/Interface 或调度 DSL。
模块连线保持 ac.reg identity，rule 无状态，Q 只在 Xfer 改变，未使能 proposal
在原语中 discard。实现前增加共用 verifier/pass，两个 backend 不各自补语义。

授权基线是 C1/C2/C3、R1/M1、C2-DECL、N1、C3-SM 和 Decision 0283 的
精确批准记录。SYSTEM/EXPECT 修订 B 仍未批准；现有其他 agent 的未提交
提案不由本计划接管或改写。新 op、Python/CLI/IR/ABI/schema/时序变化必须先
形成精确增补、独立设计审阅和批准记录，不能用“扩展计划通过”替代批准。

状态用语：

- **可派发**：合同明确，可做所述测试/测量；实际实现仍需候选绑定。
- **合同核对**：先证明现有批准覆盖精确 source/header/final/产物形状。
- **待设计批准**：仅准备设计和 oracle，不写产品实现。
- **能力 backlog**：有验收责任，尚未进入当前批次，不默认为退役或已支持。

当前证据的三个层次须分开：`test_source_unit_packet.py` 的 record header/default/
field-read 与 native `RegContractsTest` 的三个具体 list 元素是已存在的局部基础；
`migration_c1/expected.json` 的 Bank 2/4 是独立 oracle、backend 仍 UNRUN；
`test_driver_compile_link.py` 和 `test_source_module_units.py` 目前仍拒绝非空 static
bindings，不允许悄悄变为 empty key。参见 [M5 profile](../development/m5-migration.md)
和 [R1/M1 精确批准边界](../rfcs/migration/approvals/c2-r1-m1-interface.md)。

## 2. 排序与并行方式

| 波次 | M3 能力 lane | M6 加固 lane | 收尾条件 |
| --- | --- | --- | --- |
| A，立即准备 | M3-P01 合同/证据核对，SYSTEM 与 EXPECT 各自归档 | M6-03 首次发布故障矩阵；M6-04 RSS/运行测量 | 一份明确的 M3 准入表；两个有界 M6 证据包 |
| B | M3-E01 record/helper；M3-E02 Bank 2/4 固定集合/特化，按批准准入 | M6-05 选择性后端重编设计；M6-06 Unicode 工具复现 | 具体用例 source→同一 IR→两个 backend；正确性不让位于性能 |
| C | 批准后 M3-E03 一等 system/EXPECT；M3-E04 reg buffer 库 | Linux/macOS-15/Windows 分别跑 M6-07/08/09 | 当前能力与实际平台 evidence 分别关闭 |
| D，按需求 | typed DUT、memory、多域/CDC、四态、资源事务 | M6-10 实际并行/V44；M6-11 完整 SDK replay | 每项先明确合同与真实用例，不整轮一起实现 |

独立任务可同时推进，但不是每个任务同时拥有完整三人团队。当前主机四个
并发 slot：PM + 三个子 agent。设计/调查与现有合同测试可并行；实现、独立
测试、独立审阅分批复用 slot。共享 ODS、registry、link pipeline、runtime
公共头、发布 validator、root CMake 与状态表各有唯一 integration owner。
测试结果依赖的源文件冻结后不再改写；修复后按影响重测，不编辑运行中的脚本。

### 2.1 每包共同 checklist

按本包实际变化选择适用项。P01 文档/合同调查、M6-04 纯测量等不改变硬件
语义的包，对不适用项记录“不适用＋理由”，复用必要 correctness oracle；
不能为了勾选完整表而再跑整套闭环或虚构 product PASS。

- [ ] 写明唯一实际用例、非目标、适用合同与批准哈希，不自行开放 profile。
- [ ] 固定 HEAD/dirty 清单、实际模型/角色、exclusive writable files 和输出目录。
- [ ] 独立测试作者先给 literal oracle、有效反例与失败阶段；不能只比较两个 backend。
- [ ] MLIR import/link/final 边界验证 type、owner、alias、effects、checks 和实际 SSA。
- [ ] 每源独立 producer；parent 无 child source/body 可编译；保留 source-owned TU。
- [ ] 保存 final IR 后新进程 emit，CPP/RTL 真正编译运行；IR 手写仅作聚焦负例。
- [ ] 检查物理 reg 数、current/next、enable/hold/discard、reset/rerun、全树失败不提交。
- [ ] 坏输入在规定阶段拒绝、诊断有源位置、已有产物字节保留；不以任意 nonzero 算通过。
- [ ] 独立审阅绑定最终字节；PM 验收更新 profile/文档/gate，仅关闭本包范围。

## 3. M3：按用例补能力

H01–H05 仍是原能力分类；以下 P/E 编号是后续执行包，不重定义旧编号或重开 M2。
现有 native/handwritten IR 子集、历史 oracle 与当前 public source 支持必须分列。

| 包 | 目标与首个用例 | 准入 | 主要 owner / 对应责任 |
| --- | --- | --- | --- |
| M3-P01 | 当前实现/批准/旧断言对账；两份 B 提案归档与 rebase | 可派发只读准备 | PM + Astra validation；T03/T17/T21 |
| M3-E01 | 普通 immutable record 与纯 helper 的有限值计算 | 合同核对 | frontend/MLIR，独立双后端 tests；T07/T08/T14 |
| M3-E02 | Bank 2/4 固定 owned/reference reg 集合、静态循环、同源多特化 | 合同核对；未冻结 carrier 另批 | MLIR/link/source groups；T08/T10/T13/T14 |
| M3-E03 | 一等 system、设计/testbench 用途分离、EXPECT 字段语义 | 两份 B 各自待批准 | ODS/link + runtime/codegen；V24–V30/V41–V43 |
| M3-E04 | reg-only circular-buffer 模块库，容量 1/3 | 新库/protocol 待设计批准；依赖 E02 | MLIR/library + CPP/RTL；T09/T10/T12 |
| M3-E05 | 外部 typed DUT/标准 testbench，明确输入采样边沿 | 待设计批准；组合测试依赖 E03 | frontend/IR/runtime ABI；T11/T19 |
| M3-E06 | 真实依赖图的 alias/driver/comb-cycle/logic-depth 约束与定位 | 先审计现有合同/缺口 | 共用 analyses/verifiers；T03/T10/T12/T14 |
| M3-E07 | 单 bank memory，明确 latency/RDW/mask/bounds | 新 source/IR/runtime 合同待批准 | MLIR/memory/两 backend；T11/T12 |
| M3-E08 | 多 clock/reset 与 CDC | 新时序合同待批准 | MLIR/domain/runtime；T11/T12 |
| M3-E09 | 四态与未初始化值的 value/known/Z | 新逻辑值合同待批准 | 共用 types/semantics + CPP/RTL；T07/T11 |
| M3-E10 | Enum/tuple/value-array、dependent types、Slot/Table/multilane/多 owner 事务 | 分成独立需求驱动包；不能捆绑批准 | 类型/库/事务 owner；T09/T10/T14 |

### 3.1 M3-P01：实施前对账

交付：一行一能力的“精确批准 → 当前 source/IR/产物 → 尚缺 gate → 最小下一包”表。
查验 owning header、SpecKey/StateID、source snapshots、现有 reader/verifier 和
cpp/RTL 真实 source fixture。记录所有新增 op/type/attribute/CLI/ABI 字段的
差异清单；差异不在精确批准内，则只准备提案。

- [ ] SYSTEM B 与 EXPECT B 的文本/哈希、作者、独立 reviewer 和结论产物可归档。
- [ ] rebase 到 `4584ad0b` 后所有旧路径/文件引用重新定位，不复制退役 target。
- [ ] 用户方向选择、approval-ready、精确批准、实施/执行 PASS 分开记录。
- [ ] 不改 other-agent 提案；需要新修订时新建拥有明确 owner 的派生包。
- [ ] E01/E02 分别给出准入结论，不因“C1 曾提及集合/参数”就开放未冻结 carrier。

### 3.2 M3-E01：record/helper

首例：两字段有限整数记录，固定字段顺序 `(lo, hi)`，初值 `(3,17)`；
同一 rule 基于旧 Q 提出 `(lo, hi)' = (hi, lo+1)`，预期提交后依次
`(17,4)`、`(4,18)`；reset 恢复 `(3,17)`。
这是数学/逐拍 oracle，不预先规定新的 Python source 语法或 IR 字段。
纯 helper 的 local candidate 复用与再次读 enclosing reg 的旧 Q 必须区分。

- [ ] 证明普通 nominal record/构造/default/helper 已批准的精确部分；超出部分另提案。
- [ ] 不可空、完整初始化、不可原地修改、同形不同 nominal type 不互换。
- [ ] 字段顺序/default/kwargs、signedness/range、不可安全求值路径有独立反例。
- [ ] source header-only parent、保存 final/reparse 和两 backend 的字段布局/值通过。
- [ ] 失败后的 Q/所有 proposal/观察符合现有全树失败协议；不只验证 emitter 接受。

### 3.3 M3-E02：Bank 2/4

首例继承已冻结 Bank oracle：`entries=2, initial=0` 与 `entries=4, initial=5`，
首个输出 `(0,5)`，随后在既有输入序列下 `(9,13)`，reset image 分别为
`[0,0]` 与 `[5,5,5,5]`。旧 C1 class 拼写只作历史 oracle；按已批准模块函数/
嵌套 rule 方向重基，不能恢复旧作者路线。特化各实例两份，同参数复用代码、
state 独立；reference view 必须 alias 原 reg，不能复制 storage。

补充 enable/hold/discard 和非均匀 reset 场景，不取代原 oracle：例如长度 2/4
的 `[3,7]`、`[11,13,17,19]`，受使能目标一次加 1，其他元素 hold，下一拍
不得混入 disabled 的旧 proposal。若补充例子需要新 constructor/carrier，先核对批准。

- [ ] 有限正长度、owned/ref、index、静态循环的 source/interface/final 形状受批准覆盖。
- [ ] 非空 `--parameters` 的 carrier、specialization header/receipt 形状核对后才开放。
- [ ] 负 index、越界、动态拓扑、不合法 runtime shape、错误 alias/owner 拒绝。
- [ ] bool index、0 长度、runtime shape/reset、initial 超范围有反例；C2 的
  `entries=257` 反例针对初始化表达式生成 Word 值 256，不能误禁全部长度 257 的合法 list。
- [ ] 0 次循环中的局部绑定、每次 ordinal 的来源/effect/check 和非均匀 reset 验证。
- [ ] 两种特化同属一个 Python source/.ac/file group，不能造 per-case 文件或整设计 split。
- [ ] 源、header、final、物理 reg inventory、TU/CMake 图与双后端逐元素 oracle 一致。

### 3.4 M3-E03：system 与 EXPECT

两份 B 是两个批准对象。先取精确批准，再依 ODS/header authority → import/link/final
→ 两 backend/role 传播 → public artifact 整合分片；不先从 C++ 构造 system 语义。

- [ ] `ac.system`/import 及旧 marker 的精确替换按批准方案；不自行新增 primitive。
- [ ] design/testbench role 与 root 类型正交，不从文件名、assert、log/report 推断用途。
- [ ] 独立 DUT 与 testbench source/producer 分离；testbench 通过 ac.reg 连接共享 DUT。
- [ ] EXPECT 的 condition/path 来自真实 SSA；不能以 condition 关闭失败或 DCE 掉检查。
- [ ] header/body 跨种类漂移、旧表示残留、错误 role/owning declaration 拒绝且不覆盖。
- [ ] 复用既有五周期值/reg 数 oracle、全树 failure/reset/rerun；B 不捆绑外部 typed ABI。

### 3.5 M3-E04：reg buffer 库

只定义普通模块与 reg 状态/连接；Python 无 Queue/FIFO 或手写 Interface API。
MLIR 从实际 read/write/bindings 推导接口与 effects，不按 head/tail/ready 名字猜协议。
容量 1/3、首个单读/单写用例；multilane 和复杂事务不塞入首包。

- [ ] 精确冻结 empty/full、同拍读写、满时 discard、输出 hold、reset 与边沿行为。
- [ ] 使用独立 deque 状态机，覆盖 wrap、容量 1、非 2 次幂容量 3、连续 reset。
- [ ] full/empty 边界没有部分消费/发布；producer 不通过隐式 guard 获得反压语义。
- [ ] 一个唯一物理 reg owner，借用 alias 不增加 storage/latency，两 backend 从同一 IR。
- [ ] 现行退役路线保持删除；确需 compiler-internal library lowering 接口时单独审阅批准，
  不恢复已退役 public Queue/SimQueue 路线，不将此前 `ac.queue` 方向当作新 op 已获批准。

### 3.6 后续能力的最小验收

- E05：一个有限标量输入/输出 DUT，明确 host 数据何时进入 reg、谁拥有 input、
  生命周期/错误/Reset/缓冲区有效期；标准 executor 驱动，不能直接偷偷改 current Q。
- E06：真实依赖边验证反馈/循环、不同实例同名状态、重导出/rename、driver overlap；
  时钟寄存器反馈不是组合环。诊断给出第一个具体 rule/instance/source，不能仅 grep。
- E07：一个读端口/写端口，latency 与 read-during-write 模式先批准，逐拍 oracle
  覆盖同地址/异地址、mask、bounds、enabled hold；禁止 backend-only RAM 补时序。
- E08：先两 clock 的合法/非法 crossing 与 reset 次序，再 CDC 原语/异步 buffer；
  domain 关系在共同 IR 校验，不能把 default clock 多跑两次冒充多域支持。
- E09：先固定小位宽 `value/known/Z` 真值表、未知控制/reset/未初始化传播；C++/RTL
  各自对独立四态 oracle，不以字段更名或两 backend 同样清零宣称实现。
- E10：每种 enum/aggregate/dependent-type/资源/事务需求单列精确布局/编码/协议；
  不用普通 reg collection 自动获得 Table/Slot 或默认仲裁，成功 all-or-none、失败零提交。
  E10 是分类入口，不能整体作为实现任务派发；具体类型或单个资源各建独立包。

## 4. M6：按风险加固

M6-01/02 保持 done；新包按实测缺口准备，不重复认领已有门槛。

| 包 | 目标 | 状态/前置 | 第一责任与关键验证 |
| --- | --- | --- | --- |
| M6-03 | new final/CPP/RTL 首次发布与恢复中断 | 可派发现有 C3 测试 | publication 独立 tests；V48 |
| M6-04 | per-phase RSS 与足够长的有限运行测量 | 可派发测量，非新产品 API | performance；T20 |
| M6-05 | 保持 C3 原子发布的选择性后端/TU 重编 | 待内部方案/合同核对 | integration/publication；V45/V48 |
| M6-06 | Unicode generated source/build host dirs | 可派发工具复现；修复方案另审 | tool/build integration；V47 |
| M6-07 | Linux x86_64 实机 SDK/回归 | 需要对应 runner，不用 ARM64 替代 | platform/packaging；V47/V48 |
| M6-08 | formal macOS-15 minimum/SDK | 需要对应构建及运行环境 | platform/packaging；V47 |
| M6-09 | Windows x86_64 文件系统/锁/DLL/SDK | 需要真实 Windows；现有 skips 不算 PASS | platform/FS/packaging；V47/V48 |
| M6-10 | 实际并行 Work 与两类 V44 验证 | scheduler 设计核对，精确变化另批 | runtime/MLIR/independent tests；V44 |
| M6-11 | full SDK/平台 replay 与发布前核验 | 各平台包就绪；稳定版本/发布属 M7 | packaging/integration；V47/V49 |

### 4.1 M6-03：扩展故障状态表

- [ ] 以 source-unit/linked-final/CPP-bundle/RTL-bundle × first/replace × 可达
  fault point 建表；明确初次发布没有 previous 的不可达点，不能计入 PASS。
- [ ] 补 new final/new CPP/new RTL 的真实 SIGKILL；检查 journal、stage、destination、
  previous 布局与现有 C3 commit point，验证共享读/后续写的分别恢复语义。
- [ ] 恢复也能在允许点中断再恢复；old bytes、lock inode、owner 与有效读者结果正确。
- [ ] invalid journal/owner、两个 writer、reader/writer 用 barrier 同步，不以 sleep 证明锁。
- [ ] 不加公开 fault flag、receipt/journal 字段或新 owner/作者认证机制；若实际缺陷
  需要这些变化，先回到精确接口提案。普通数据保护回归仍立即修，不延期。

### 4.2 M6-04：RSS/运行测量

复用 shared 1/16/64、distinct 1/8/32，不重新发明 benchmark source 语言。

- [ ] 每个阶段记录实际执行命令、wall time、采样规则/单位/间隔及可见进程树。
- [ ] 区分单进程峰值、采样进程树 RSS 和累计 child 高水位；记录共享页重复计量、
  漏峰及平台差异。无可用数据写 unavailable，不补 0，不冒称完整峰值。
- [ ] 长有限运行按几何增加 epoch，重复采样；保留独立结果/Reset oracle、明确
  观察出口开关与 I/O 成本。三周期/startup timing 不是 steady-state throughput。
- [ ] 报告 amortized cycles/s 与启动影响；未隔离 CPU/热状态/资源竞争时明确说明。
- [ ] 不增加 public runtime counters/config/ABI 或 IR digest；gate 内数据只作开发证据。
- [ ] 不凭一次数字设速度承诺；先给可复现分布，再对具体回归选基准和容忍范围。
- [ ] 测量准备可并行，正式比较时预留资源；无法隔离时记录并发 workload，
  受竞争影响的样本不用于建立回归阈值，不把不同机器/工具版本的数字直接比较。

### 4.3 M6-05：选择性后端失效

现状：leaf source 失效具有选择性，但完整 emit 重写全部文件，所有 CPP TU 重编。
先比较可选内部方案与 C3 契约，冻结所选方案后才写 code；不先增加 cache API。

- [ ] 输出 staging 仍完整、native final 验证不跳过、commit/rollback 保持现有原子性。
- [ ] 相同 bytes 的未受影响文件可保留稳定性，但方案不能借旧 receipt 免做 owner/IR 校验。
- [ ] 从真实 include/ABI/layout/source-map 依赖推导受影响 TU；不承诺任何 leaf edit
  必然仅编一个 TU。共享 header 真变动时 parent 重编是正确行为。
- [ ] body/header/type、新增/删除 child、alias/root变化、toolchain版本各有失效矩阵。
- [ ] content/mtime/执行命令与独立 source/group inventory同时核对；禁止 touch 时间伪造 no-op。
- [ ] first/replace、clean/rebuild、失败恢复与用户改动保护回归重跑；不以性能优化绕过发布保护。
- [ ] 冷构建/重复构建 CPP/RTL 逐拍结果相同；不做整设计 compile 后拆文件或旧 emitter fallback。

### 4.4 M6-06：Unicode 工具路径

- [ ] 在源码/构建/安装路径分别覆盖 ASCII spaces 与 Unicode，含组合场景；SourceOwner
  名称合法化和 host path 是不同能力，已有 moved Unicode prefix 不能代替此测试。
- [ ] 先用正式 workflow 已选的 Verilator 5.048 重现；不假设新版本已经修好。
- [ ] 捕获 vendor JSON 原文与 configure/build/run 失败点；5.044 octal-escape 缺陷有独立对照。
- [ ] 若依旧失败，优先上游可复现修复；framework workaround 必须有明确版本/范围/恢复行为
  和独立测试，不做任意字符串替换或偷偷降低 JSON/文件路径校验。
- [ ] CPP/RTL 都在原 Unicode 路径实跑；复制到 ASCII 路径不算关闭缺口。

### 4.5 平台与 SDK：M6-07/08/09/11

每个平台独立报告工具链、OS/arch、最低系统/运行库、source pin、artifact 字节、
构建/安装/运行与动态依赖。macOS 26 上通过或改 wheel tag 不证明 macOS-15 minimum。

- [ ] runner 符合既有 version-map/profile；构建本候选，不能复制另一个 worktree 工具链。
- [ ] Runtime-only 无 LLVM/MLIR discover；CompilerDev 精确 22.1.8，header/link 闭包完整。
- [ ] source/body/header 不可用后，保存 final 双 emit；每源 TU/CMake、重实例、原语状态 oracle。
- [ ] 真正 moved prefix、干净 Python/loader/CMake 环境、外部 C/CPP consumer 和 wheel smoke。
- [ ] Windows 三个跳过项及 rename/reparse/directory-handle/锁/DLL 行为在实机执行；
  不能用 Linux 宏模拟、跨编译或 skip 关闭其验收责任。
- [ ] 平台 manifest/consumer-lock/release-index 按既有精确合同核验，不能新造 preview schema。
- [ ] 发布 replay 与所有平台证据齐备后才进入 M7 稳定发布准备；当前已存在的 `v6.1.0`
  指向旧修订，不删除/移动 tag 或自行改版本。稳定版本/外部发布另按授权处理。

### 4.6 M6-10：真正并行与 V44

实现对象是同一 IR/system/executor 的 Work 执行策略，不是新 Python scheduler。
已有 reg/check/event 时序不能因线程改变；精确 runtime/CLI/配置变化先核对批准。

- [ ] Q 在整个 Work 只读；每个 rule 的 proposal/check/observation buffer 互不污染。
- [ ] precommit 是全树 barrier；任何失败阻止本拍全部 DriveNext/提交。
- [ ] 每个 owner 恰一次 Xfer；disabled/stale proposal 丢弃；Reset 等待/清除未完成工作。
- [ ] error/event 的确定排序来自现有结构 identity，不使用线程完成先后作为 writer priority。
- [ ] 确实启动多 worker、覆盖多 rule/child 与重复实例，有 barrier/instrumentation 证明 overlap；
  `-j4` 编译、置换手写访问列表或多进程独立 runner 都不算并行仿真。
- [ ] 同一源码的 runtime 调度变化严格比较 Q/error/events；源码重排按显式语义身份映射
  比较，各自使用其合法结构排序；两个 oracle 不能混用旧 AST identity。
- [ ] 覆盖失败、冲突、未使能、无写 clocked rule、零 rule、reset/rerun；可用平台做 race 检测。
- [ ] 两个 V44 selector 只有真实执行通过后才从 deselect 变为验收；不要删/放宽断言。

## 5. 派发与关闭规则

近期默认：M3-P01 只读合同核对 + M6-03 测试扩面；M6-04 可在资源允许时并行准备。
E01/E02 的合同准入结论就绪后选择其中一个真实用例，不要求所有 E 包先完成。
M6-05/10 不与 publication/runtime 公共头的其他 writer 并发；平台 lane 用独立
checkout/build，传递源修订与证据，不复制本地 compiler/artifact 验证另一台机器。

每次交付在新 work item写清“本包完成 / 仍开放 / 未声称”；基于实际失败调整顺序，
不虚构工期、百分比或性能阈值。本计划中的 checklist 是未来验收要求，未执行的
项保持未勾选。当前 plan 审阅及 docs checks只验证规划本身。
