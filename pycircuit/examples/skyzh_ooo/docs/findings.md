# skyzh 模型边界、性能问题与参考兼容行为

本例对齐未修改的 skyzh 提交 `8989a09c357a69b68612f653380d60816f5176c2`。模型、时序验收和计时入口见 [README](../README.md)。参考一致不能代替独立 ISA 检查。

<a id="modular-probes"></a>

## Signal 与 Queue 的组合边界

Signal 的 Queue/Signal 输入和捕获依赖形成静态 DAG，在初始化及 Queue Xfer 后按拓扑序传播，不增加流水拍。Module 导出内部 Signal 的固定引用；完成广播被多个状态所属 Module 同拍读取，新分派条目在写入自己的 Queue 前应用旁路。

Signal 的 valid 表示组合条件满足。若生产事务可能受反压阻塞，必须显式表达 ready/firing 协议。本 CPU 的分派 Signal 先检查全部旧空槽和 ROB 空间，各槽位只有一个更新 Rule，提交 flush 具有统一优先级。

ROB 使用 8 个独立容量 1 的 Queue，每项一个状态更新 Module。正常退役和 flush 使用该项的同一 pop 来源；flush 一次 Xfer 清空全部占用。语言及组合能力回归在 [编译器测试](../../../tests/test_compiler.py)，小电路输入在 [fixtures](../../../tests/fixtures/skyzh/)。

当前模型的模块间组合数据与控制主要由 Signal 传递，包含实际 payload：

| Signal | 传递的信息 |
| --- | --- |
| Frontend 内部的 `instruction`、`sources` | 译码指令和源操作数值／依赖 tag，供分派使用 |
| `allocation` | 分派目标、待写 ROB／RS 条目、重命名信息及下一 PC／tail |
| ExecutionCluster 内部的 `integer`、`memory_lanes` | ALU 完成值与 LSU 推进信息；阶段更新仅供执行簇内部使用 |
| `completion` | 各执行单元完成值、ROB tag 和 Store 地址更新，导出给 ROB；同拍广播也供执行簇内已有／新分派条目使用 |
| `retirement` | 提交条目、寄存器／内存写入、分支结果及 flush 控制 |

Queue 保存跨拍状态和槽位占用，包括 PC、ROB、RS、LSU 阶段、RAT/RF、内存和预测器。Module/Rule 读取 Signal 后提出 Queue 变更，统一在 Xfer 提交。因此逻辑关系是「Queue 当前状态 → Signal 组合网 → Module/Rule 下一状态 proposal → Queue Xfer」。Signal 串联不增加流水拍，也不提供消息排队或消费语义；需要等待或保留的信息由 Queue 承担。这个分工对应当前 skyzh 微架构，其他流水模型仍可用 Queue 传递带缓冲的消息。

<a id="performance-findings"></a>

## 性能问题记录（2026-10-04，待修复）

对组件重构前的逐拍对齐模型进行了硬件采样、计数插桩及四组 A/B 实验。aarch64 CPU 0，Clang 22、C++20、`-O3 -DNDEBUG`、无 LTO；同一固定周期循环，串行交替七次取中位数，构造与轨迹输出不计入。window 为 40,991 拍，branches 为 12,328 拍。

| 版本 | window：ns/tick | branches：ns/tick |
| --- | ---: | ---: |
| 原生 skyzh | 297.9 | 313.4 |
| 当前生成模型 | 14,724.6 | 13,337.4 |
| 仅跳过无变化的 revise | 11,921.1 | 10,960.1 |
| 仅去掉临时结构体多余清零 | 10,216.6 | 8,916.4 |
| 去掉多余清零并减少聚合值拷贝 | 8,753.1 | 8,023.7 |
| 合并上述优化 | 5,675.7 | 5,427.0 |

已确认的问题：

1. **生成 C++ 的临时对象处理。** [Compiler.cpp](../../../mlir/Compiler.cpp) 给 payload 字段生成默认初始化，EmitC 函数中的大量聚合临时对象因此发生多余清零。一个 ROB 更新函数进入 `beginRule` 前的指令数从 233 降到 18。聚合值的字段／下标更新和提取还保留整数组拷贝，汇编确认 `-O3` 未消除。仅去掉多余清零降低整体耗时约 31%–33%，加上聚合值拷贝调整后约为 40%。实验保留显式构造的初始化；正式修复应在保证 SSA 定义与真实默认值语义的前提下处理临时变量。
2. **无变化的 Queue 事务。** [RegisterFile](../register_file.py)、[ROB](../reorder_buffer.py) 和 [执行簇](../execution.py) 中 RAT 每拍 revise 31 项，ROB 保留目标、占用 ROB 和等待 RS 也有重复写入。每拍约 48–51 个 Queue 进入 Xfer，实际只有 11–12 个变化，约 76%–77% 无变化。变化检测在 Xfer 才发生，之前已经支付 proposal、仲裁、回调等成本。实验只在 RAT、保留目标、ROB 和等待 RS 四处增加值变化判断，耗时下降约 18%–19%；不能据此无条件省略任意多次／部分字段 revise。
3. **广播激活与通用事务开销。** 23 个 Module 和 7 个 Signal 几乎每拍都运行；每拍仅 1 轮 delta、0 个延迟事件，没有反复重算。当前已经没有动态依赖登记和候选缓存，Queue 来源槽位也已直接映射。原版约 42%–43% 的平坦采样落在 GFSim 调度与 Queue 函数，合并实验后约为 56%；Module 函数还包含内联事务代码，不能把其样本都算成纯 CPU 逻辑。原版 Signal 计算函数约占 12%–13%，不能单独解释整体差距。

合并实验保持原来的 23 个 Module、Queue/Signal 边界和 GFSim 库，耗时降低约 59%–61%，相对原生的差距由 42–49 倍缩小到 17–19 倍。四版通过 7 个短程序 × 两种 Module 顺序（56 次）及两个长程序（8 次）的逐拍对照。不同实验的改善比例不能当作互不重叠的耗时占比。

**以上是诊断实验，正式编译器、模型和 GFSim 尚未应用这些优化。** 聚合值实验在生成 C++ 中去掉 payload 字段默认初始化、将 `OperandView` 构造改成原地填充，并折叠 47 处单次使用的字段／下标提取；这还不是通用编译 pass。后续优先处理临时对象与聚合值 lowering，再消除无效 revise，随后利用静态连接与容量信息简化 Queue 事务路径。

本地原始证据保存在 [`reference/benchmarks/skyzh-profile-aligned/`](../../../../reference/benchmarks/skyzh-profile-aligned/)：`analysis.json` 汇总归因，`variants.json` 保存七轮样本，`profile.json` 和 `after-combined/profile.json` 保存前后采样，`assembly-comparison.json` 保存汇编对比，`validation/` 保存逐拍验收。复现脚本为 `prepare.py`、`measure.py`、`diagnostics.py`、`variants.py`、`compare_variants.py` 和 `summarize.py`；脚本内记录工具链与本次构建路径，均只在 benchmark 目录写实验副本。这些本地测量产物不随源码提交，本节保留问题与测量摘要。

## 原生行为与已知错误

| 编号 | 固定原生实现 | 本例处理 |
| --- | --- | --- |
| R01 | `ROB_SIZE=8`，1…8 环形指针，保留一格，实际可用 7 项；上游 README 的 12 项不符 | 与源码同配置，验收确实填满 7 项 |
| R02 | `Issue.cpp` 的 AUIPC 条件重复写成 LUI opcode | AUIPC 同样停滞；固定观察 40 拍，不计作程序完成 |
| R03 | Load 只检查较老 Store 的 `Dest == load_address`，未检查宽度重叠 | 保留判定；`memory.s` 要求复现错误，并单独报告 ISA 不通过 |
| R04 | SRAI 分派检查立即数 bit 9，应为 bit 10 | 保留原错误移位选择；`integer.s` 要求复现 |
| R05 | LB 用 plain `char` 转换，当前 aarch64/Clang 的 char 为 unsigned | 当前宿主对齐为零扩展；其他 char 符号平台需重新验证 |

具体复现：`integer.s` 对 -16 执行 `srai ...,3`，应为 `0xfffffffe`，原生和本例均为 `0x1ffffffe`。`memory.s` 的 LB 读 `0xff` 得到 `0x000000ff`；较老 SH 与随后 LW 地址不同但范围重叠，LW 得到旧高半字。测试不把这些结果当作 RV32I 正确答案。

原生 ROB 的 `Dest` 在退役/flush 后保留，而且新 Store 分配不初始化它。在 Store 地址计算完成前，该旧值仍会参与 Load 依赖判断。为严格复现这一行为，每个 entry 另有一个 `retained_dest` 状态 Queue；占用仍完全由 ROB entry Queue 空满表示，观察器逐拍核对此字段。JALR 的两个微操作也分别保留，ISA 检查器只在比较架构提交时合并它们。

目前输入限定为参考支持的 RV32I 子集、4 MiB 范围内对齐访存和对齐取指。未实现 CSR、特权态、M/A/C 扩展。程序通过向 `0x30004` 写入非零字节结束，停止检测由宿主在计时窗口外进行。没有为原生未定义的越界/非对齐 C++ 访存建立等价性声明。
