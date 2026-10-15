# MLIR 编译流程重构

目标：ACPy → 源语义 MLIR → 事务 MLIR → RTL 友好 MLIR → 逐拍执行验证。
语义以 `core-design.md` 为准；本轮不包含 SystemVerilog、激活调度、跨拍缓存或自动仲裁。

## RV5S 表达与独立参考验证（2026-10-15）

新增 `examples/ripes5/`，以 Cell 表达标准 RV5S 的固定流水线寄存器，Module 分级，
Stage 接收 Cell 输入观察拍初快照，显式前递／WB→ID 旁路、load-use 和 EX 冲刷。
`demo.py` 是带汇编注释的机器指令输入；参数化模型不依赖旧调度器或旧前端。

随后将四个级间 Cell 统一移到顶层，通过 `if_id = fetch(...)` 等 Stage return 绑定固定输出，
Fetch 的 `None` 保持输出，Decode 的气泡返回显式清空有效位。9 个程序的全部状态和 fire
在资源路径重命名后与迁移前逐拍相同，原生参考、优化开关和阶段重载也通过。
迁移结果保存在 `builds/benchmarks/ripes5-ports/`；独立事务仍是五级更新加一个观测，共 6 个。

9 个完整程序共 260 拍，与保存的原生 Ripes 参考逐拍匹配，包含 PC、各级有效 PC、
控制选择、寄存器／内存、气泡标记和退休／store 事件；生成 C++ 与两层 IR 解释执行也匹配。
参考来自原生 Ripes RV5S，不是旧 pyCircuit 模型；本次未重新运行缺少 Qt 依赖的原生 runner。
出处和 SHA256 随参考保存，结果在 `builds/benchmarks/ripes5/`。
完整 CTest 的 70 项回归通过；随后新增的可读 Demo 入口也单独完成原生参考／C++／IR 对照。

这次独立对照暴露并修复三个通用问题：嵌套部分返回的 continuation、struct 字段的
文件作用域限定，以及 C++ 共享条件需求排布的人为环。后者在无法采用统一求值排序时
保留原始短路顺序的局部分支，不改变事务图。已补充最小回归，表达边界与源码见示例 README。

## 基线与保存

2026-10-13：开始时存在 83 个已修改文件及 3 个未跟踪 C++ 文件。
原始 HEAD、状态清单、完整二进制 patch、未跟踪文件归档保存于
`builds/benchmarks/compiler-refactor-baseline/`。现有改动作为独立基线提交保存。
开始时的链路为 v7 源 MLIR → C++ analysis lowering → program.json → Python 图分析；无执行器。
本 checkout 为 `/home/lc/tmp`，不适用仅允许在 `/home/lc/pyCircuit` 构建的 pyc6 skill。
继续使用 README 指定的 Clang 22、GCC 14 与 MLIR 22。

## 阶段记录

当前版本：ACIR v9。C++ lowering、源名称与逻辑资源单选访问重构已实现；本轮验证与测量见本文末尾及 [C++ lowering 审阅](cpp-lowering-review.md)。下表及中间章节保留历史阶段证据。

| 阶段 | 状态 | 验证与剩余工作 |
| --- | --- | --- |
| 1. class Module 与统一 Rule、源语义 IR | 已实现 | 仓库全部示例可生成 v8 源 MLIR；2 项源层回归通过，含原生验证、作用域、赋值和旧 API 拒绝 |
| 2. 事务提取与条件传播 | 已实现 | C++ pass 直接读取 MLIR；调用点独立、嵌套原子、共享 SSA、失败不进入 else、按资源合并消费均通过逐拍回归 |
| 3. MLIR 契约、EHR 与调度分析 | 已实现 | 声明沿调用栈传播，C／SBR 同路径诊断，EHR 固定视图、未声明可见性拒绝，分支／后继与合并 DAG 保存于 IR |
| 4. RTL 友好 lowering | 已实现 | Cell／Queue／EHR 化为 current、组合选择与 next；Signal 无 RTL 状态；独立 verifier 检查残留操作、状态类型与调度 |
| 5. MLIR 执行、CLI 与清理 | 已实现 | 当前 31 项回归、CTest 1/1、8 个模型各 32 拍；两层执行、优化与三阶段重载一致；旧编译路径已删除 |

## 验收

覆盖 Queue 反压／同拍腾空间／原子取消；共享读取、嵌套分支、读取失败和多个调用点；
Signal、EHR 可见性；OoO 分配／等待／发射／广播／退休；指令 CPU 与独立 ISA。
每阶段更新本文件并独立提交。语义缺口记录最小复现和影响，不通过改写行为绕过。

## 下一步

本轮五阶段完成。后续可独立开展 SystemVerilog 后端、动态循环的硬件表达、自动仲裁、
激活调度及跨拍缓存优化；这些工作不纳入本轮完成范围。

## 阶段 1 验证

class Module、独立静态 __init__、固定 __call__ 连接、显式接口、隐式实例作用域与统一 Rule 已迁移。
规则关系保存为模块内 `acir.contract`，分支仍保留 scf.if／CFG。源 MLIR 原生验证通过。
旧 Method／函数 Module 输入明确拒绝；内部 Work 命名也已移除。

迁移发现：旧 EHRSlots 的 release → available → allocate 把 available 的读取与 allocate 的写入
放在同一个 allocate 事务的不同端口，违反核心设计 §5。保留该例及诊断，新增合法共享读取模型验收。
最小复现为 A SB B、B SB C，C 的 guard 调用 B.read 且 C.write 同一个 EHR。
这属于旧模型与新核心契约不一致，不能通过任意顺序或临时旁路放行。

## 阶段 2 验证

事务 IR 由原生 C++ pass 建立，完整条件成为 SSA。模块共享定义保留单一来源；
分支合流恢复无关计算的控制作用域，合流值自身保留读取就绪条件。嵌套效果全部由外层 fire 控制。
消费请求按事务与真实 Queue 合并；普通效果性返回值跨独立事务时报错。

14 项新回归通过：包括两层一致、Queue 多级反压／腾空间／取消、读取失败与无关事务、
嵌套分支／重复消费、Signal 取消、EHR 固定视图、CPU 独立 ISA 和定向退休周期、OoO 顺序退休。
源／事务／RTL 均可独立重载；CPU 和 OoO 的优化开关结果一致。新日志位于
`builds/benchmarks/refactor/`。后续阶段继续扩展契约诊断、端口约束及结构检查。

设计参考：[BSC AAddScheduleDefs.hs](https://github.com/B-Lang-org/bsc/blob/main/src/comp/AAddScheduleDefs.hs)
明确／隐式条件合成 CAN_FIRE，效果使能与 WILL_FIRE 关联。本轮不仲裁，按核心设计以完整条件作为 fire。

## 阶段 3 验证

21 项回归通过。新增无共享资源的声明顺序、C／SBR 嵌套调用拒绝、
release → allocate 合法 EHR guard、无声明时读取旧值、共享读取对所有使用者的依赖传播、
拒绝端口排名引入的未声明可见性，以及 Queue 数组真实资源选择／重复消费合并。
事务 IR 的 `acir.path` 保存分支条件 SSA，边保存源分支和目标分支集合，fire 保存后继。
固定节点级拓扑顺序保存于 circuit；分支边合并形成环时明确拒绝。

同拍端口竞争保存为 conflicts 报告，不自动仲裁。对同事务禁用关系，仅在实际路径可能重合时拒绝；
布尔 SSA 证明无法处理过大的条件时采取保守诊断，不增加 EHR 可见性。

## 阶段 4 验证

RTL lowering 已独立为 RTLLowering.cpp，事务 pass 独立为 Transactions.cpp。
Queue 化为固定存储数组、count、消费移位和入队索引更新；Cell/EHR 为组合选择链与 next-state。
Signal 只有组合 data/valid，不留状态声明。共享读取使用同一转换映射，保持固定端口。
RTL verifier 检查无残留调用／控制流／状态效果、状态类型正确、调度包含全部 fire 且满足顺序边。
原生 pass 串联、默认构造配置、冻结配置重载、CLI 执行和损坏调度文件的拒绝均已回归。
25 项测试与 CTest（1/1）通过；完整日志与 JUnit 位于 builds/benchmarks/refactor/。

范围限制最小复现：`for i in range(counter): ...` 中 counter 来自运行时 Cell。
源层可以保存循环，当前有限组合 RTL lowering 对循环 CFG 报错；不能静态确定展开次数时不生成动态执行硬件。
字面量固定循环已支持。此限制不影响本轮验收模型，也不通过改变循环行为绕过。

## 阶段 5 验证与清理

2026-10-13：26 项回归全部通过，CTest 1/1 通过。事务与 RTL 执行器直接读取 MLIR，
每拍保存 fire 与提交状态；CLI 支持分阶段停止、独立重载、静态配置与指定拍数执行。
CPU 用独立指令位解码的 RV32I 参考核对寄存器和内存，退休拍为 4、5、9、13、14、18。
OoO 最终寄存器为 [0,42,6,7,48,13,61,0]，退休标签顺序为 1、2、3、4；
同时检查分配、操作数等待、ADD／MUL 发射和完成广播确实发生。

`tests/run_acceptance.py` 对 dual_channel、shared_state、mixed_ports、signal_link、
cell_feedback、ehr_shared、riscv_pipeline、ooo_scheduler 各执行 32 拍。
所有模型的事务／RTL 轨迹、优化开启／关闭、源／事务／RTL 独立重载均一致。
EHRSlots 的同事务跨端口操作、ExclusiveRing 与 QueueRing 的分支合并环均得到预期诊断。
完整轨迹、配置及汇总保存在 `builds/benchmarks/compiler-acceptance/`；
回归日志与 JUnit 保存在 `builds/benchmarks/refactor/`。

删除 program.json 导出、Python 图分析／契约／EHR 主链路及旧 Method／Work 测试，
以新接口的源层与逐拍验收替代。更新 ACPy／ACIR 规范、编译意图、CLI 使用文档和图查看工具。
仓库保留的示例输出已重新生成三个 MLIR 层与事务图；过时的 readable／Module 图输出已清理。
JSON 仅作报告，不作为编译或重载输入。原始未提交内容仍可由基线提交及归档恢复。

## 补充修正：只提供 Rule 的组件无需主体

2026-10-13：放宽前端对 `__call__` 的强制要求。只有状态和公开 Rule 的组件可省略主体，
实例化后直接调用其 Rule，不要求额外写 `rf()`，也不生成空 `acir.body` 或自动调度 Rule。
需要固定输入／输出或每拍选择逻辑的 Module 仍使用显式 `__call__` 连接。
未定义主体却调用实例或访问 `.outputs` 会报告具体原因；未连接输出也有明确诊断。

CPU 中 RegisterFile／DataMemory、DualChannel 的 Observer 已移除导出包装／空主体及无意义的连接调用。
内部 RF／内存状态直接由逐拍轨迹观察。同步更新核心设计、ACPy／ACIR、使用文档和保存的三层产物。
新增回归覆盖无主体实例的 guard／嵌套原子性、实例身份、未调用 Rule 不执行和无主体 top 的重载；
已有 EHR 固定共享读取回归也改为无主体组件，继续验证声明关系及端口依赖传播。

29 项回归和 CTest 1/1 通过，8 个示例的两层执行、优化开关及三阶段重载验收继续通过。
CPU 与 DualChannel 改写前后各 32 拍的 fire 和状态完全一致，CPU 的独立 ISA 与退休周期预期不变。
修正前轨迹、对比结果、回归日志和 JUnit 位于 `builds/benchmarks/rf-call-inspection/`。

## 补充修正：顶层声明连接组件，删除旧输出列表

2026-10-13：此前仅移除 RF 的空主体，仍保留阶段内部 out 和 .outputs，未完成接口迁移。
现改为在 Pipeline.__init__ 明确声明 if_id／id_ex／ex_mem／mem_wb，再通过
`if_id = fetch()`、`id_ex = decode_stage(if_id, rf, mem_wb)` 等赋值绑定输出。
Module 的组件返回类型声明输出接口，主体返回 Rule 的输出；目的地身份和构造配置不变，
容量条件及输出效果归入原 Rule 事务，不生成额外搬运事务。支持条件化 return／None、
多输出绑定、嵌套 Module 输出转发，以及输出组件作为显式参数传入。

删除 .outputs 访问、Instance.outputs、内部组件引用导出、隐式输出 Queue 构造及旧输出推断路径。
旧输入明确诊断，不保留兼容分支。全部示例迁移到明确声明的连接资源；
OoO 的发射／完成 Queue 和广播 Signal 由顶层声明，保留站观察通过公开 peek Rule。
同步更新核心设计、ACPy／ACIR、示例说明及三层 MLIR／图报告。

31 项回归和 CTest 1/1 通过。新增逐拍模型核对顶层资源身份、嵌套转发、满时同拍腾空间、
反压时生产者状态的原子取消、优化及三层重载；显式输出参数版本也得到同一轨迹。
负例覆盖 .outputs、隐式目的地、内部组件导出、输出形状和类型错误。
8 个示例各 32 拍的两层执行、优化开关及重载继续通过；与修正前对照，
仅将已移动组件的旧名字映射到新名字，全部 fire 和状态逐拍一致。CPU 独立 ISA 及退休周期、
OoO 退休次序继续通过。基线、名字映射、对照结果、日志和 JUnit 保存在
`builds/benchmarks/explicit-connections/`。

## 设计确认：统一多维 array

2026-10-14：正式确认 [array 设计](array-design.md)，替代旧 array、vector、qarray。
统一静态多维 shape 和完整构造，match 只计算 mask，对外调用传选取后的 array；
无效单选访问传播未就绪，禁止部分选取的 array 直接跨拍保存。
事务分析按静态关系保守排序，高层及仿真保留结构，到 RTL 友好层再物理展开。

本次仅整理正式设计与文档引用，尚未迁移实现。验证为文档链接、示例语法及格式检查；
下一步按该设计实现前端、MLIR 操作及两层执行验收，不继承旧数组的兼容处理。

## 实施：前端收紧与统一 array（2026-10-14）

基线提交 `50dae6c` 保存实施前全部未提交设计文档。实施顺序为严格前端、结构化控制流与 Module 调用、统一 array、事务分析、RTL 与执行器、示例迁移和清理，每阶段独立验证提交。

本轮采用普通坐标及仲裁 `(found, position)` 返回值，不增加公开 index 类型或 is_selected。所有链式赋值禁止，Rule 不接受 Module 参数，选取后的 payload array 可传普通 helper，跨拍保存仍要求完整 payload。

阶段一首批完成：严格条件、参数、转换、helper 权限和声明边界；新增负例覆盖，34 项回归通过。静态绑定、array 及后续编译链仍待完成。

阶段二首批完成：源层显式 module.call；事务提取仅从顶层调用位置进入子主体，传播路径及有效性。新增恒假、失败读取和动态启用的多层 Module 测试。发现源层优化删除调用会改变事务身份及 fire 轨迹，改为事务提取后优化并保留 fire 观察点。35 项回归通过，新增场景的 raw/opt 和三阶段重载一致。结构化前端及静态绑定仍在后续实施中。

阶段三主路径完成：统一多维 array 描述符与 MLIR 类型，删除 vector/qarray 类型和 Queue 专用构造；引入显式 selected/ mask 内部类型，公开下标仍为普通整数。实现完整形状构造、多维索引与更新、match、选取传递、同值多选写入和普通 tuple 仲裁返回。CPU/OoO 与数组测试迁移。40 项回归通过，覆盖 Queue[array]、局部 SSA 更新及动态越界取消。RTL 数组物理展开、完整 verifier 和其余前端结构化工作尚在实施中。

阶段四完成：前端直接构造 scf.if，多返回和可选输出通过独立 SSA 合流；删除 Python CFG 恢复及回退。先收集静态连接再展开行为，支持实例互相绑定、Module 数组的静态连接及动态公开 Rule 选择；按定义、实例和实际类型分别缓存 helper/Rule，调用点仍独立。OoO RF 改为构造绑定，不再保留空调用。

事务层保留 array/selected/mask 及 shape，记录 Module 数组和资源数组的真实身份；实际访问只要求所选资源就绪。新增 Wires.cpp 在 RTL lowering 展开全部聚合状态、Queue 固定存储与占用、选取使能和 mux。执行轨迹由 MLIR 内的逻辑状态映射恢复原坐标，无 JSON 编译输入。

52 项回归通过。新增覆盖 shape/初值及损坏 MLIR 的 verifier、选取穿过 helper/struct、多维 SSA 更新、动态越界原子取消、Queue[array]、空 mask、别名重复消费、EHR 数组固定共享视图、Signal 数组选取，以及 age matrix 发射的反压原子取消。测试发现动态组件引用被再次导入为构造引用的问题，已按 SSA 对象身份修复，保留回归；未使用的文件级组件构造也明确拒绝。9 个示例各 32 拍的两层执行、优化开关和三阶段重载已通过，最终文档及生成产物清理待下一阶段提交。

阶段五完成：增加 tuple 返回、struct 字段更新和 struct 数组中选取域保留的回归，禁止借助聚合包装绕过跨拍存储限制。53 项回归通过，最终 CTest 1/1 通过；9 个示例各 32 拍的事务／RTL、优化开关和 source／transaction／rtl 独立重载全部一致。CPU 的独立 ISA、定向退休周期与 OoO 顺序退休预期继续通过。三项故意非法模型仍报告 EHR 跨端口或 branch-union 环。

最终证据：`builds/benchmarks/array-ctest.log`、`array-ctest.xml`、`array-acceptance.log` 和 `compiler-acceptance/summary.json`；完整逐拍轨迹在同目录的各模型 raw／opt 子目录。示例输出重新生成，新增 `builds/array_arbiter/`，删除 model.acir.mlir 重复产物。删除未使用的旧 transfer 模型，测试文件移除 v8 命名；ACPy、ACIR、核心设计、array、使用说明和示例说明已同步。

实施提交：`45441c6` 严格前端，`3b839df` Module 调用路径传播，`994eda7` 多维 array 核心，`15fd507` 结构化前端及数组到标量 RTL。基线仍保存在 `50dae6c`。本轮不引入公开 index 类型、is_selected、独立 mask 接口、类型转换或旧语法兼容；Queue[array] 首版空初始化，不自动推断 RAM，不实现自动仲裁、SystemVerilog、激活调度或跨拍缓存。

## 示例简化：ROB 槽位数组

2026-10-14：OoO 的四个 RobSlot 改为 `ac.array(RobSlot, shape=(4,))`，使用 `slots[(tag - 1) & 3]` 调用公开 Rule，删除分配、查询、完成和退休中的四组逐槽位分支。各槽位独立状态、guard 和外层事务原子性保持不变，无需修改编译器。

保存改写前 32 拍基线，仅将 `top.rob.slotN.state` 映射为 `top.rob.slots.N.state` 后，全部状态及 fire 逐拍一致；事务／RTL、优化开关及三个阶段重载全部一致。现有 OoO 定向回归通过，最终寄存器和 1、2、3、4 退休顺序不变。三层示例产物及图报告已重新生成；证据位于 `builds/benchmarks/ooo-rob-array/`。

## 示例与测试精简

2026-10-14：示例入口由 10 个收敛为 array_arbiter、ehr_shared、riscv_pipeline、ooo_scheduler 四个。删除六个早期小模型及其八套生成产物；混合可选输出、互斥路径的顺序环和非法 EHR 端口组合转入短回归。共享实例、Signal、Cell 反馈等由已有事务、静态互相绑定和完整模型覆盖。EHR 回归直接复用保留的示例。

测试从五个文件归并为 frontend、pipeline、array 三类，support.py 统一执行和重载检查；Module 路径、静态绑定及严格前端负例全部保留。独立 Queue 流水线合入显式 Module 流水线验收，聚合 Cell 字段更新合入多维更新验收，简单 found／坐标仲裁合入 age matrix 验收。测试数量由 53 变为 50，示例和测试 Python 代码共减少 455 行。

50 项回归、CTest 1/1 通过，四个示例各 32 拍的事务／RTL、优化开关及三阶段重载一致。独立 ISA、OoO 顺序退休、age matrix 反压及 EHR 固定共享视图的预期保留。证据：`builds/benchmarks/examples-consolidation-ctest.log`、对应 XML、`examples-consolidation-acceptance.log`；当前验收汇总仍在 `compiler-acceptance/summary.json`。

## 事务与 RTL 清理优化

2026-10-14：新增 Optimizations.cpp，将 canonicalize／CSE 封装为可独立运行的
`acir-cleanup` pipeline，事务提取后和 RTL lowering 后共用；`--no-opt` 关闭清理。
aggregate／extract 的 fold 支持固定下标、结构字段取值及完整聚合重建消除，
不折叠动态下标或改变其就绪条件，空 aggregate 的内部零值语义保持不变。

新增完整 Packet Queue 流水线回归，核对动态数组读取、聚合重建、满时反压取消、
同拍 pop／push、优化开关、三层重载及独立 cleanup pipeline 的逐拍结果。
51 项回归、CTest 1/1 通过，四个示例各 32 拍的两层执行、优化开关和重载一致。

重新生成示例产物。RTL 操作数（不计 builtin.module）由 array_arbiter 的 3842 降为 797，
riscv_pipeline 的 3981 降为 1654，ooo_scheduler 的 7019 降为 3165，ehr_shared 的 29 降为 26。
统计位于 `builds/benchmarks/rtl-cleanup/summary.json`，CTest 结果位于
`builds/compiler/cleanup-ctest.xml`，完整示例验收位于 `builds/benchmarks/compiler-acceptance/`。

## C++ 静态调度仿真后端（2026-10-14）

实施目标：直接从事务 MLIR 的 schedule、fire、读取和效果生成独立 C++20 模型，
每拍静态尝试所有 Rule，拍末提交；无激活调度、跨拍缓存或自动仲裁。

已实现：`CppEmitter.cpp` 独立 emitter、`CppRuntime.h` 通用运行时、
`--emit-cpp`／`analyze(emit_cpp=True)`，产出 `cpp/model.hpp`、`runtime.hpp`、`sim.cpp`。
具体类型和固定数组承载状态，逐事务具名函数保留 Rule 注释；按需 SSA 缓存每拍清空，
短路条件不执行未选分支；全部效果参数求值后才发布。程序支持 reset、逐拍 fire/state
轨迹和关闭轨迹执行，不需要 MLIR、LLVM、Python 或 JSON 解析器。

图语义先行修复并增加三项回归：拒绝效果性返回值控制 Module 分支；嵌套调用栈中的
SBR／C 不再被外层 SB 遮蔽；共享只读调用以独立 SSA 锚点保留声明身份，将声明关系
传播给 payload／ready 的所有实际消费者，包含纯常量返回的调用。

最终验证：59 项回归、CTest 1/1 通过。所有既有执行回归接入 C++ 对照，包含 Queue
反压与原子取消、Signal 有效性、EHR 端口与共享视图、数组选择、CPU 独立 ISA／退休
顺序／周期断言及 OoO 顺序退休。新增 8/16/32/64 位整数边界、i1 最低位截断、未选
分支的非法除法／下标／断言、reset 重跑、输入阶段及不支持操作诊断，以及选取数组
局部更新后保留选取域和取消无效读取的回归。

四个完整模型各 32 拍的 C++／事务／RTL 全部 fire 和状态逐拍一致；raw／opt 及
source／transaction 独立重载生成一致。四个模型均通过生成代码的 ASan／UBSan 和
reset 检查。LeakSanitizer 在本机沙箱的 ptrace 环境中无法工作，最终 sanitizer 验证
在获准的沙箱外完成。动态依赖检查只发现系统 C／C++ 运行时，不含 LLVM、MLIR 或 Python。

SSA 缓存使用具体类型值字段和一块固定有效位数组，每拍清零有效位。限制 SSA 访问
函数内联，避免大图展开；分离有效位消除了大量独立 optional 字段造成的编译优化开销。

最终证据：`builds/benchmarks/cpp-build.log`、`cpp-ctest.log`、`cpp-ctest.xml`、
`cpp-unit-tests.log`（完整用例输出）、`cpp-acceptance.log`、`cpp-dependencies.log`，
以及 `compiler-acceptance/summary.json`。最终编译器的四模型 raw／opt 共八套 C++
文件与通过验收的文件逐字节一致，记录在 `compiler-acceptance/final-emission-check/summary.json`。
完整生成文件、可执行文件、轨迹和 raw／opt 测量在各模型子目录；示例三层 MLIR 与图报告已更新。

本机 aarch64、Clang 22，事务优化开启、C++20 `-O1` 的单次测量如下。代码量为三个
生成文件之和；时间含进程启动，可能与测试并发，不代表隔离环境基准或性能达标倍数。

| 模型 | 生成代码（bytes） | C++ 编译（秒） | 无轨迹 10000 拍（毫秒） |
| --- | ---: | ---: | ---: |
| array_arbiter | 165149 | 2.57 | 38.85 |
| ehr_shared | 10102 | 0.92 | 2.32 |
| riscv_pipeline | 113699 | 2.26 | 10.44 |
| ooo_scheduler | 394174 | 6.32 | 52.38 |

前端负数 `%` 常量折叠与运行时 `arith.remsi` 的差异不在本后端改写，见 README。

## 寄存器堆改为 Cell 数组

两个 CPU 示例的寄存器值及 scoreboard／重命名表改为 `ac.array(ac.cell[T], shape=(...))`。
流水线寄存器堆有 64 个独立 Cell，OoO 有 24 个；Rule 仍按 `registers[index]` 读写。
两个端到端测试通过，包含独立 ISA、退休周期／顺序、优化开关和阶段重载生成 C++。
保留迁移前的轨迹作为参考，32 拍 fire 和还原后的全部状态均一致，C++／事务／RTL 逐拍一致。
两个生成程序的 ASan／UBSan 和 reset 重跑通过；本次关闭 LeakSanitizer。
生成结果更新于对应示例的 builds 目录；迁移证据在 `builds/benchmarks/registerfile-cells/`。

当前动态 Cell 下标展开为各候选 Cell 的读取选择与写使能，未保留 C++ 的原生数组索引。
相较整表 Cell，生成代码量增加：流水线三文件合计 373534 bytes，OoO 475212 bytes。
这是后续代码生成优化点，不需要改写前端表达来绕过。

## C++ lowering、源名称与逻辑资源访问重构（2026-10-14）

本轮基线完整保存于 `builds/benchmarks/cpp-structure-refactor/baseline/`，包含开始时的
tracked patch、未跟踪文件归档、状态清单及四模型旧生成文件；原有未提交改动保留。

`CppLowering.cpp/h` 独立分析事务使用范围、共享边界及条件依赖。前端绑定用
`acir.bind` 保存 SSA 身份，名称与函数作用域写在 NameLoc，参数分别进入 guard/body；
helper 展开附加 CallSiteLoc。名称不作为属性参与 CSE、契约或路径互斥判断。
CSE 合并后采用 IR 中稳定的主名称；C++ 关键字、重复绑定及与生成局部量的冲突加后缀。

事务 `acir.read/effect` 携带候选资源、一个扁平坐标、可选选择域及逐候选固定 EHR 端口。
调度分析只临时枚举候选访问，分析结束后删除这些访问点。事务 IR 解释执行与 C++
直接索引；独立 `acir-expand-resources` pass 才为 RTL lowering 展开资源选择。
动态别名按真实资源身份合并 pop；越界／未选域仍取消整个事务。批量选取写保留现有
逐候选条件效果，未把该操作改成单选访问。

C++ emitter 使用 Module 实例定义、原 Rule 名称和 payload 字段；组件数组保存为
固定 `std::array`，别名通过资源引用分派，异构 Module 数组仍按实例分别组织结构。
每个事务先判定 fire、准备效果参数，再发布效果；tick 仍使用公共层固定 schedule。
只由一个事务使用的值是普通局部变量，字面量与简单匿名单次表达式内联；潜在非法
索引、除法、断言及会改变读取视图的共享计算按实际路径安排。条件需求用编译期
布尔决策图合并，避免重复展开；生成程序没有布尔决策图运行时或第二套调度器。
缓存只留在跨事务共享边界，读取的 data/ready 共同捕获一个固定视图；拍首清空缓存。
原逐 SSA 求值函数、逐 SSA 成员缓存与全量 valid 表均已删除。

参考源码放在 `reference/bsc/src/comp/`：
[SimMakeCBlocks.hs](https://github.com/B-Lang-org/bsc/blob/main/src/comp/SimMakeCBlocks.hs)
的 rule_fns／cvtARule，
[SimCOpt.hs](https://github.com/B-Lang-org/bsc/blob/main/src/comp/SimCOpt.hs)
的 moveDefsOntoStack，以及
[SimBlocksToC.hs](https://github.com/B-Lang-org/bsc/blob/main/src/comp/SimBlocksToC.hs)
的表达式／字面量输出作为代码组织参照。这里没有引入 BSC 的仲裁或调度机制。

验证证据与逐事务审阅见本轮 benchmark 目录及 `docs/cpp-lowering-review.md`。
事务执行和 RTL 友好层执行在报告中明确标为 **IR 解释执行**。

最终验证：63 项回归全部通过，CTest 1/1；四模型各 32 拍的 C++、事务 IR 解释执行、
RTL 友好 IR 解释执行完全一致，raw／opt 与三阶段独立重载一致，ASan／UBSan 与 reset 通过。
四个重构前保存的程序也与本轮轨迹逐拍一致。所有示例 C++ 与三层 MLIR 已重新生成；
`structural-review.json` 保存各事务 fire 检查、准备／发布区域及无 SSA 缓存函数的检查结果。

本机 aarch64、Clang 22、C++20 -O1 的前后测量如下。生成代码为三个文件总 bytes；
缓存为字段数量；编译为一次测量，无轨迹 10000 拍为五次运行中位数，含启动时间。

| 模型 | bytes 前→后 | 缓存 前→后 | 编译秒 前→后 | 无轨迹毫秒 前→后 |
| --- | ---: | ---: | ---: | ---: |
| array_arbiter | 165149→79502 | 1202→10 | 2.81→1.25 | 39.08→2.22 |
| ehr_shared | 10102→11062 | 17→6 | 0.84→0.89 | 2.33→1.93 |
| riscv_pipeline | 373534→93007 | 2647→2 | 5.26→1.36 | 41.85→3.25 |
| ooo_scheduler | 475212→158330 | 3457→30 | 7.00→1.83 | 70.50→5.39 |

小型 EHR 模型的语义结构与共享块使总文件略增；其余三个模型代码量下降。测量未隔离
并行验证，不作性能倍数保证。原始采样、SHA256 和编译时间位于 `comparison.json`。
最终日志为 `build.log`、`ctest.log`／`ctest.xml`、`acceptance.log` 和 `comparison.log`。


## 2026-10-15：公共源层与结构化事务提取

本轮以已有未提交重构为基线，快照在 `builds/benchmarks/bsc-inspired-compiler/baseline/`。
源层新增 `acir-source-opt`，整理纯绑定、常量、结构分支并保留 Module 静态调用点。
事务提取直接处理 scf.if／yield，保留无环 CFG 输入；互斥分支按原工作表顺序访问，保持旧事务编号和 fire 标签。
共享结构值按使用次数保留整体，恢复 `Decoded inst` 和字段访问。新增的界限、索引、ready 等值不继承源绑定名。

公共布尔条件 pass 在需求路径证明通过时归一化 fire／enable；断言移为原使能保护的 fire 检查依赖，
验证器要求检查只能成功返回 true 或抛断言错误。C++ 与 RTL 都执行这些依赖，源 IR 不含 C++ 语句／缓存或 RTL 物理展开。
C++ 使用已成功 fire 的事实准备效果；消除恒真使能、重复检查、空准备块，只有可能重合的 pop 才保留运行时去重。

原生源层到 RTL 的独立管线、零结果分支断言、容量失败前的断言和非法检查依赖加入回归。
实际 BSC 发布包的 fetch 生成代码与本项目的组织方式对照见 `docs/cpp-lowering-review.md`；
它不是完整 CPU 等价或性能对比。最终逐拍对照、结构统计和测量在本轮 benchmark 目录。

本轮最终测量（与本轮开始的已重构基线比较）：

| 模型 | 总 bytes 前→后 | 缓存字段 前→后 | C++ 编译秒 前→后 | 10000 拍毫秒 前→后 |
| --- | ---: | ---: | ---: | ---: |
| array_arbiter | 79502→36760 | 10→6 | 1.24→1.10 | 2.31→2.14 |
| ehr_shared | 11062→10905 | 6→4 | 1.03→0.97 | 1.97→1.71 |
| riscv_pipeline | 93007→40475 | 2→0 | 1.43→1.18 | 3.35→2.81 |
| ooo_scheduler | 158330→75232 | 30→52 | 1.81→1.48 | 4.88→2.95 |

C++ 编译采用 Clang 22、C++20 -O1；运行时间为五次中位数，包含进程启动，验证并行且未隔离，不能据此保证性能倍数。
OoO 保留结构后共享边界增多，缓存字段从 30 增为 52；它们仍是共享输出／固定读取快照的 optional，不是全量 SSA 缓存。
四个保存的旧 C++ 程序与本轮全部 32 拍的状态和 fire 标签完全一致。

| CPU Rule | 主体行数 前→后 | fire 前行数 前→后 | 布尔声明 前→后 |
| --- | ---: | ---: | ---: |
| top.fetch.fetch#0 | 35→19 | 18→5 | 16→4 |
| top.decode_stage.read_operands#1 | 554→103 | 352→81 | 428→37 |
| top.execute.execute#2 | 30→14 | 14→2 | 13→2 |
| top.memory_stage.access#3 | 92→46 | 51→12 | 52→14 |
| top.writeback.commit#4 | 58→31 | 20→6 | 23→5 |

最终验证：66 项回归通过，CTest 1/1；四模型各 32 拍的 C++、事务 IR 解释执行和 RTL 友好 IR 解释执行一致，
raw／opt、三个阶段独立重载、ASan／UBSan 和 reset 通过。保存的旧 C++ 也与全部 state／fire 字段一致。
独立 ISA、退休周期／顺序、原子取消、Signal、EHR、动态数组与短路的已有独立预期继续通过。
生成示例已更新，且与本轮验证过的 C++ 文件逐字节一致。
