# 表达能力与原生参考差异

本例对齐未修改的 skyzh 提交 `8989a09c357a69b68612f653380d60816f5176c2`。模型、时序验收和计时入口见 [README](README.md)。参考一致不能代替独立 ISA 检查。

## 通用编译能力

原输出前向引用、独立输出容量、C++ 关键字问题均已修复，最小输入迁至 [编译器 fixtures](../../tests/fixtures/skyzh/)。隐式输出容量仍是 1；需要其他容量时显式声明 Queue。

<a id="modular-probes"></a>

### Signal 组合与 entry Queue

Signal 的显式 Queue/Signal 输入和捕获依赖形成静态 DAG。GFSim 在初始化及 Queue Xfer 后按拓扑序重算受影响节点，输出变化才通知下游，组合环报错。链式组合不增加流水拍。正式回归在 [signal_circuits.py](../../tests/signal_circuits.py)、[test_compiler.py](../../tests/test_compiler.py)。

Module 可以导出内部 Signal 的固定引用。完成广播被多个状态所属 Module 同拍读取；新分派条目在写入自己的 Queue 前应用旁路。普通 Work 局部值不能直接成为 Module 的组合输出，应定义 Signal。Rule 返回的 Queue 始终构成时序边界。

Signal 的 valid 表示组合条件满足，不表示另一个 Rule 已被仲裁接受。若生产事务可能受反压阻塞，必须显式表达 ready/firing 协议。本 CPU 的分派 Signal 先检查全部旧空槽和 ROB 空间，各槽位只有一个更新 Rule，提交 flush 具有统一优先级。

ROB 现已改为 8 个独立容量 1 的 Queue，每项一个状态更新 Module。正常退役和 flush 使用该项的同一 pop 来源，flush 一次 Xfer 清空全部占用。无需单 FIFO clear 或整核 Rule。原最小探针在 [rob_entries.py](../../tests/fixtures/skyzh/rob_entries.py)，完整 CPU 已覆盖分派、完成和恢复并发。

历史探针及旧模型保存在 `reference/benchmarks/skyzh-before-alignment/`；保留的本机探针脚本已指向 fixtures。最小 Signal 案例可单独编译：

```bash
LD_LIBRARY_PATH=/home/lc/opt/gcc14/lib \
ACPY_MLIR_COMPILER="$PWD/reference/builds/skyzh-aligned-release/mlir/acir-compile" \
python3 -m pycircuit compile pycircuit/tests/fixtures/skyzh/modular.py \
  --top SignalChain --output reference/benchmarks/signal-dag-probes/manual
```

### 循环内聚合值更新（本次修复）

自然写法 `result.lanes[i] = value`、`result.field += value` 必须把局部结构体/数组作为循环携带的 SSA 值。此前前端只识别直接赋给变量名的目标，导致字段更新没有带入下一轮，甚至被优化删掉。现在沿 Attribute/Subscript 赋值目标找到普通值的根变量并携带；Queue revise 仍是资源效果。独立回归 `test_loop_carried_aggregate_fields` 覆盖零次迭代、嵌套循环、条件更新和优化开关。CPU 不需要强制展开扫描来绕过此问题。

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
