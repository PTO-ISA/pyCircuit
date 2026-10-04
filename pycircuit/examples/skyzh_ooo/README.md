# Queue 版 skyzh 参考乱序 CPU

**当前状态：完整 CPU 尚未集成，受两个已复现的前端表达缺口阻塞。**
按约定记录问题并暂停依赖它们的部分；本轮没有修改编译器、ACIR 或 GFSim。
现有成果是参考适配、RV32I 译码/执行组件、完整程序和可重复检查，不能当作 CPU 端到端验收。

## 目标结构

参考 [skyzh 固定提交 8989a09](https://github.com/skyzh/RISCV-Simulator/tree/8989a09c357a69b68612f653380d60816f5176c2)，
独立定义流水时序，架构结果以独立 RV32I 解释器核对。

```text
Fetch → Queue → Dispatch/Rename ──push──→ ROB FIFO(12) ──pop──→ Commit
                    │                                      ↑
                    └─push→ RS 槽位 Queue(1) → 执行请求 → 执行级 → 完成 Queue → Writeback
```

- 4 个整数、3 个 Load、3 个 Store 保留站槽位；槽位空满表示占用，分配 push、发射 pop。
- 单取指、分配、写回和提交。写回从完成 Queue 中选择最老指令，选择逻辑属于模型。
- 级间 Queue 容量 1，同一 Rule 原子地消费输入、产生输出；满队列可复用同拍获准 pop 的空间。
- 寄存器堆、RAT、完成记录和预测表使用寄存器式 Queue。
- 整数执行一拍；Load 使用地址、读取、返回三阶段；Store 仅在提交时写内存。
- Load 等所有较老 Store 提交，不做 Store 转发与访存推测。
- 分支在提交时恢复，epoch 标记旧路径，各 Queue 的原消费者丢弃旧消息。
- 条件分支预测计划使用 64 项局部表，每项两位历史及四个两位计数器。
- 目标内存默认 256 KiB，可通过构造参数调整；字节/半字/字访问使用小端，非对齐访问记为 fault。
- 不实现 CSR、特权态、缓存与 M/A/C 扩展；测试用 EBREAK 表示停机。

这些是**目标结构**。当前只实现下表中标为完成的部分。

## 已完成与受阻部分

| 内容 | 文件 | 状态 |
| --- | --- | --- |
| RV32I 译码、整数/分支/地址计算、Load/Store 字节操作 helper | [logic.py](logic.py) | 已编译并检查 |
| pop 输入、push 输出的执行级；同一消费者丢弃旧 epoch | [execute.py](execute.py) | 已通过反压组件检查 |
| 保留站空满选择、ROB/保留站原子分配、CPU 顶层连接 | [findings.md](findings.md) 的 E01/E02 | 阻塞，未添加替代结构 |
| Fetch、重命名、保留站唤醒、完整 LSU、提交与恢复 | — | 未集成为 CPU |
| 原生参考构建和观察入口 | [reference](reference/) | 固定版本，可运行 |
| 独立解释器及真实汇编程序 | [oracle.py](oracle.py)、[programs](programs/) | 可运行 |
| 完整 CPU 逐拍检查及 GFSim/skyzh benchmark | — | 等待完整集成 |

### 表达缺口

- E01：Rule 函数体无法引用由自身 return 创建的输出 Queue，保留站分配选择无法自然表达。
- E02：同一 Rule 的各输出不能独立设容量，ROB=12、槽位=1 的原子分配受阻。
- E03：Python 标识符 `unsigned` 原样进入 C++，发生关键字冲突；组件改用同义参数名 `is_unsigned`，缺陷仍保留复现。

详细意图、自然写法、复现、影响及参考实现问题统一写在 [findings.md](findings.md)。

## 运行

以下命令在仓库根目录执行。需要 Python、C++20 编译器，以及支持 RV32I 的 `clang`、`llvm-objcopy`、`ld.lld`。
所有新程序通过 LLVM 汇编器生成机器码，没有额外编写专用指令编码器。

```bash
# 诊断现有表达能力；仍有缺口时退出码为 2。
python3 -m pycircuit.examples.skyzh_ooo.diagnose

# 组件检查；这不是 CPU 验收。
python3 -m pycircuit.examples.skyzh_ooo.check_components

# 固定参考；仓库在 /tmp，未修改上游核心源码。
git clone --depth 1 --single-branch --branch out-of-order \
  https://github.com/skyzh/RISCV-Simulator.git /tmp/skyzh-riscv-reference
git -C /tmp/skyzh-riscv-reference rev-parse HEAD
python3 -m pycircuit.examples.skyzh_ooo.reference.build \
  --source /tmp/skyzh-riscv-reference --output /tmp/skyzh-reference-build

# 原生参考对照独立解释器；存在结果差异时退出码为 1。
python3 -m pycircuit.examples.skyzh_ooo.verify_reference \
  --runner /tmp/skyzh-reference-build/skyzh-reference --source /tmp/skyzh-riscv-reference
```

构建脚本要求 HEAD 为上述完整提交号且 tracked 源码干净。将来分支发生变化时，需显式获取该提交再构建。
结果默认保存在本例的 `output/`，生成 C++、二进制和大型输出均忽略版本控制。
当前结果摘要见 [results.json](results.json)。

组件检查通过两条路径：直接编译、保存 ACIR 后独立重载。执行流水分别运行缓存开/关和 Module 正/反序，
核对消费序列及逐拍进度，并确实触发反压、同拍替换和旧 epoch 丢弃。
其操作数来自完整程序的独立解释轨迹，因此只验证执行组件，不验证取指、重命名或乱序调度。

原生参考检查 5 个新程序及原 `main.cpp` 所用的 7 个 `.hex` 程序。比较全部架构寄存器和所有修改过的内存字节，
不只比较 x10 低八位。原 `out-of-order-1` 是无限循环，`out-of-order-2` 没有结束 Store，二者明确不纳入结束状态对照。
上游 `main.cpp` 引用的 `data/*.data` 不包含在该 checkout 中，尚未运行那些程序。

本次发现原参考的 AUIPC、SRAI、LB 和重叠访存问题。参考错误不作为新 CPU 的预期语义。
原参考真实 ROB 可用容量为 7，内存为 4 MiB；与新模型的目标配置不同。

## 后续完整验收

先解除 E01/E02，再连接目标组件。完整程序需覆盖乱序发射/完成、顺序提交、窗口填满、槽位复用、
阻塞期间操作数保持、访存依赖、错误路径 Store 及有在途结果时的恢复。
每次提交对照独立解释器；缓存/执行顺序/ACIR 重载变体逐拍一致。

之后才开展两模型 benchmark：相同程序和输入、统一编译配置，排除初始化和观察，
分别报告周期数、IPC、ns/模拟周期、架构指令/秒和内存占用。因为微架构不同，不将时间比称为调度引擎加速比。
原生 adapter 已提供 `IMAGE MAX_CYCLES --fixed N` 入口，固定 N 拍循环内只有 `Session::tick()`。
