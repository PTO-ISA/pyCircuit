# 表达与参考实现问题

E01–E03 来自迁移前的自然写法探针，已在通用 MLIR 编译链中修复。R01–R05 保留固定原生参考的独立发现。

## E01：输出 Queue 前向引用（已修复）

Module 静态构造先预声明全部 Rule 输出，再编译 Work/Rule，因此 Rule 可以观察自己绑定的输出。
复现 [output_feedback.py](repro/output_feedback.py) 现在可编译，完整小电路测试检查实际运行与消费。

## E02：独立输出容量（已修复）

分别显式声明 ROB=12 和保留站=1，再以 `rob, reservation = allocate(...)` 绑定同一 Rule 的输出。
原子性保持不变。复现 [output_capacities.py](repro/output_capacities.py) 已改用确定的声明语法；
原探针的 `capacity=(12, 1)` 不是语言接口。隐式输出仍默认容量 1、初始为空。

## E03：C++ 关键字（已修复）

通用符号映射转义 `unsigned`、`switch` 等关键字以及转义命名前缀，应用于参数、字段、函数和资源。
[cpp_keyword.py](repro/cpp_keyword.py) 与编译器 Keywords 电路共同覆盖生成和执行。

`python3 -m pycircuit.examples.skyzh_ooo.diagnose` 重新检查三个表达探针；完整 CPU 验收由 verify 负责。

## R01：skyzh 的 ROB 容量以源码为准

固定提交：`8989a09c357a69b68612f653380d60816f5176c2`。
README 写 12 项；实际 `src/Pipeline/OoOExecute.h` 定义 `ROB_SIZE = 8`，
指针使用 1…8，`next(rear) == front` 判满，可用容量为 7。
新模型采用 12 个可用项，两者是不同配置，不进行逐拍等价声明。

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

完整 Queue CPU 已连接，逐条提交对照独立解释器，并对优化、缓存、Module 顺序和 MLIR 重载变体进行逐拍对照。
复现命令及双模型 benchmark 口径见 [README](README.md)，本次验证数据见 [MLIR 报告](../../mlir/results.md)。
