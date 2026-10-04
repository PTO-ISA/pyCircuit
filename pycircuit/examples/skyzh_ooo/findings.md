# 表达与参考实现问题

本例以 Queue 空满表示保留站占用，以 pop/push 表示分配、发射和流水传递。
遇到阻塞时保存自然写法和复现，不改变 CPU 结构来规避，也不修改编译器或 GFSim。

## E01：Rule 无法引用由自身 return 创建的输出 Queue

- **意图**：分配器查看各保留站槽位，选择一个目标并 push；目标选定后的容量许可交给仲裁。
- **自然写法**：`first, second = allocate(source)`，`allocate` 中查询 `first.empty()`。
- **复现**：[output_feedback.py](repro/output_feedback.py)。当前编译报 `unknown name: first`。
- **位置**：前端在第一次 Rule 调用时立即编译函数体，随后才建立输出连接变量。
- **影响**：无法直接表达基于输出空满的保留站分配。普通线性流水不受此问题影响。
- **需要的能力**：函数体可引用已确定的静态输出连接；不需要读取 pending proposal 或仲裁状态。
- **状态**：阻塞分配器和 CPU 顶层连接。没有改成轮询分配，也没有添加占用影子表。

## E02：同一 Rule 的输出容量不能独立声明

- **意图**：分配 Rule 在一次原子事务内向 12 项 ROB 和 1 项保留站 push。
- **当前行为**：`@ac.rule(capacity=12)` 使所有输出均为 12 项，没有单独绑定输出容量的入口。
- **复现**：[output_capacities.py](repro/output_capacities.py) 尝试 `capacity=(12, 1)`，前端接受该常量，生成阶段出现 `TypeError`。
- **说明**：元组是表达能力探针，不是已定义的语言语法。问题是缺少独立输出容量；修复不一定采用该语法。
- **影响**：目标分配事务受阻。拆成两条 Rule 会改变原子性；增加中转级会改变计划内结构，因此均未采用。
- **状态**：阻塞 CPU 集成。

## E03：合法 Python 标识符与 C++ 关键字冲突

- **意图**：用布尔参数 `unsigned` 表示 Load 是否进行零扩展。
- **复现**：[cpp_keyword.py](repro/cpp_keyword.py)。ACPy 编译和生成通过，C++ 编译在 `bool unsigned` 处失败。
- **位置**：后端没有为用户名字转义 C++ 关键字。
- **影响**：表达语义本身不受限制；组件参数采用同义且更清楚的 `is_unsigned`，没有添加状态或改变时序。
- **状态**：保留未修复的后端问题及复现；该命名问题不阻塞组件检查。

运行 `python3 -m pycircuit.examples.skyzh_ooo.diagnose` 可重新检查，结果默认保存在
`output/expression.json`。存在阻塞时退出码为 2，不将“复现到错误”当成 CPU 验收成功。

## R01：skyzh 的 ROB 容量以源码为准

固定提交：`8989a09c357a69b68612f653380d60816f5176c2`。
README 写 12 项；实际 `src/Pipeline/OoOExecute.h` 定义 `ROB_SIZE = 8`，
指针使用 1…8，`next(rear) == front` 判满，可用容量为 7。
新模型计划采用 12 个可用项，两者是不同配置，不进行逐拍等价声明。

## R02：AUIPC 无法进入对应发射路径

`src/Pipeline/Issue.cpp` 中 LUI 与 AUIPC 的分派条件都写为 `0b0110111`。
AUIPC 的正确 opcode 是 `0b0010111`。原参考未打补丁；该指令不能作为原版一致性验收条件。
后续架构正确性以独立解释器为准，参考差异单独报告。

## R03：不同宽度重叠访存出现架构结果差异

`LoadStoreUnit::no_store_in_rob` 只比较较老 Store 的 `Dest` 与 Load 地址是否相等。
完整程序 [memory.s](programs/memory.s) 中，较老 `SH` 应将高半字写为 `0xfffe`，
随后的 `LW x10` 应获得 `0xfffe8034`，原参考实际得到 `0x80ff8034`。
最终内存与解释器一致，但 x10 保留旧高半字。此结果差异已复现；地址相等判定不足以
识别重叠依赖是源码分析，尚未用内部 Load/Store 时序 trace 单独证明具体发生周期。
未知 Store 地址的问题仍待确认。

## R04：SRAI 选择了逻辑右移

[integer.s](programs/integer.s) 的 `srai x18, x1, 3`，x1 为 -16：
应得到 `0xfffffffe`，原参考得到 `0x1ffffffe`。
`Issue.cpp` 的立即数移位分派检查 `inst.imm & (1 << 9)`；SRAI 的区别位实际在立即数位 10。

## R05：LB 依赖宿主 char 的符号

同一 [memory.s](programs/memory.s) 中，`LB` 读到 `0xff` 应得到 `0xffffffff`，
当前 aarch64 宿主上的原参考得到 `0x000000ff`。
`LoadStoreUnit.cpp` 使用 `(char)`，而当前编译器的 plain char 为 unsigned。
该问题与宿主有关，不能推广成所有平台上同样失败。

## 运行状态

完整 ACPy CPU 尚未集成，尚无 GFSim 与 skyzh 的性能比。
表达探针、执行组件检查和参考程序运行分别报告，不能替代完整 CPU 端到端验收。
