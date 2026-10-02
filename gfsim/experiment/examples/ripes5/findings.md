# Ripes5 验收与问题记录

2026-10-02，本机 aarch64。原版 Ripes 与 VSRTL 的完整 SHA 见 [版本锁](reference/version.json)，机器可读验收摘要见 [results.json](results.json)。当前已完成生成式写法重构，寄存器表达在已覆盖程序上逐拍一致。级间 Queue 仍全部使用 revise，没有验证输入消费或容量背压；现有对齐结果不能作为消费型流水可表达性的证明。

源码与运行器现集中在本目录：原版源码为 `reference/upstream/Ripes/`，构建产物与构建依赖为 `reference/build/`；Qt SDK 仍在 `/tmp/gfsim-ripes-qt`。迁移时重新生成 CMake 配置，原版与子模块 SHA 保持版本锁中的值。迁移后原版 CLI smoke、12 程序 × 4 配置逐拍比较和 Ripes5 的 3 项测试全部通过；新原始轨迹保存于 `review-output/migration/`。

## 逐拍验收

12 个完整程序，每个程序分别使用缓存开／关和 Module 正序／反序，48 次执行均匹配同一个程序的原版参考轨迹。比较含初始状态、每拍 valid／有效 PC、PC mux 与实际下一 PC、stall／flush／前递选择、寄存器、全部数据字、store 和退休事件；无程序相关的周期平移。

| 程序 | 周期 | 退休（包括结束标记） |
| --- | ---: | ---: |
| alu | 15 | 11 |
| forward_priority | 12 | 8 |
| wb_id_store | 15 | 11 |
| array_sum | 120 | 80 |
| jumps_branches | 31 | 15 |
| load_branch_wrong_path | 25 | 13 |
| jalr_forwarded | 17 | 8 |
| no_false_load_use | 11 | 7 |
| initial_and_x0_load | 14 | 8 |
| mixed_7 | 179 | 130 |
| mixed_42 | 156 | 114 |
| mixed_2026 | 167 | 117 |

数组程序写入 1～8 并得到 36，有八个 load-use 停顿。错误路径程序仅发生两个 store（4096←17、4100←18），错误路径寄存器保持 0。额外验证了同拍 WB→ID、ALU／load→JALR、LW x0 不触发停顿，以及立即数字段不会被当作源寄存器。

原版 CLI smoke 使用独立的小程序及原版 ECALL 出口，确认 x3=12、11 周期、5 条退休和流水报告均可用。它只验证 CLI 可用性；逐拍对照使用约定标记退休，完全不依赖 ECALL/HALT。

`test_first_divergence_report_on_complete_program` 在一个完整程序的结果中故意改变第 3 拍前递选择，确认工具报告准确字段、首拍和前后窗口。这是报告器测试，不是真实未解决差异。真实验收没有出现首个分歧。

本次重构后的全量回归：`RIPES5_REQUIRE_NATIVE=1 python3 -m unittest discover -s gfsim/experiment -v`，34 项测试全部通过，零跳过，84.860 秒。包括原有弹性 CPU、通用队列网络、1100 级长流水与可视化观察一致性检查；完整日志位于 `review-output/generated-style/regression.log`。

## 问题分类与结论

| 类别 | 发现 | 结论 |
| --- | --- | --- |
| 构建环境 | 系统无 Qt 6.8；Qt 配置还需要 OpenGL 开发包 | Qt 6.8.3、Charts、GL 开发依赖均装入 `/tmp`，精确包 URL 已锁定；原版成功构建 |
| 适配器接口 | 直接实例化 RV5S 需先包含 `isa/rv32isainfo.h`，否则实例化抽象 ISAInfo | 已补齐头文件，不改变上游 CPU |
| 原版 CLI 输出 | `--json` 的 stdout 仍可能混入程序退出文字；字段用 pretty name，寄存器值是字符串 | smoke 用原版 `--output` 独立保存 JSON，并解析真实格式 |
| 原版模型特性 | enable 优先于 clear；WB 离开流水的沿计退休；store 在 MEM 沿提交 | Python 使用 current 组合控制与 Queue 同沿 revise 对齐 |
| 原版模型特性 | MEM 前递直接选 ALU，WB 前递选最终写回值；WB→ID 是组合旁路 | 分别实现并逐拍比较前递选择与效果 |
| 原版模型特性 | 源码将 JALR 映射为 ADD，未清除目标 bit 0 | 本轮只接受对齐指令地址，不声称覆盖奇地址 JALR 或完整 ISA 合规性 |
| 观察边界 | 无效槽中的原版 ALU/PC 残留可能非零（ALU NOP 为 0xDEADBEEF） | 保留在原始 `raw` 中，无效 payload 不作为有效指令比较；有效控制与结果全部比较 |
| Python 模型错误 | 已覆盖程序无未解决差异 | 不以最终寄存器正确替代逐拍验收 |
| 模型表达限制／待验证契约 | 级间 Queue 始终非空，只读 current 并 revise，没有消息输入消费或动态容量阻塞 | 当前结果仅验证寄存器表达；消费型流水、load-use 停顿与背压的合法表达尚待验证，不能据此判定不存在框架缺口 |
| 重复计算成本 | Fetch、Decode 各计算控制和 EX，Execute 再计算一次 EX | 每拍三个 EX 组合求值，无隐藏缓存；每阶段显式下一拍事件 |

持久 CPU 状态全部在 Queue；Python 列表只保存构造关系、ROM 和工具轨迹。观察器读取不参与 Rule 控制。每阶段只修改自己的流水寄存器，存储和寄存器写各有唯一来源，因此 Module 顺序变化不影响提交结果。

## 生成式写法重构

五个阶段已改为独立普通类，取消 Stage／ClockedStage 继承及 observe／put／controls 包装。每条 Rule 显式登记读取、读取 current、提出 revise、请求下一拍事件并结束候选计算；必要值缺失时显式撤销。阶段仅持有绑定 Queue 与固定配置，不依赖 CPU 对象回调。五条 Rule 的入口、仲裁入口和资源绑定直接列在构造代码中。核心、编译契约、原弹性 CPU、参考源码及比较接口未修改。

重构前后 12 程序 × 4 配置的 3,096 行轨迹（含初始状态）逐字段一致；重构后再次与原版 Ripes 的同一输入逐拍比较，48 组全部通过。四份 array_sum HTML 均由通用 ReviewTrace 生成，并随生成过程再次通过原版对照。证据保存在 `review-output/generated-style/`，包括旧源码与轨迹 SHA256、逐字段结果、新原始参考轨迹和 HTML。

本轮仅改变写法；共享纯函数仍分别调用，每拍三次 EX 组合求值，每阶段保留原有时钟唤醒。未增加共享缓存或针对耗时的优化。后续消费型实现不能把读输入后的普通业务返回擅自改成 abort；停顿必须能从现有编译契约的选择、必要读取或资源许可中表达。

## 关闭诊断的初步耗时（重构前历史基线）

[baseline.json](baseline.json) 保存生成式写法重构之前的五次独立进程样本、主机和 Python 版本。本轮没有重新计时，以下数字不代表当前重构代码的耗时。程序固定为已逐拍通过的 `array_sum`（120 周期），C++ 由 GCC 14.4 Release 构建，Python 为 3.11.16；未同时运行回归或编译。下表是五次中位数，单位 ms。

| 模型 | 时钟循环 | 整个进程 |
| --- | ---: | ---: |
| C++ Ripes | 0.936 | 22.604 |
| Python GFSim cache=True reverse=False | 24.767 | 116.262 |
| Python GFSim cache=True reverse=True | 24.791 | 116.375 |
| Python GFSim cache=False reverse=False | 24.636 | 116.017 |
| Python GFSim cache=False reverse=True | 24.556 | 115.877 |

时钟循环计时包含推进时钟及检查结束标记，不含输入加载、构造、逐拍快照、JSONL、ReviewTrace 或结果输出。整个进程计时包含启动、Python 导入／动态库加载、输入、构造、循环、输出和退出。GFSim 内建统计计数与 Ripes 内建信号机制仍存在，Ripes 关闭反向历史存储。GFSim 的五个 Rule 各执行 120 次，缓存命中为 0：每拍候选都获准并丢弃，没有保留候选供跨拍命中。

这是两个不同语言的完整模型、很短的单个程序和不同运行时的初步基线，不能据此宣称 GFSim 调度性能优劣。后续正式比较需要同语言、统一诊断和计时范围、更多输入及稳定采样。

## 范围与后续

仅支持现有实验的整数子集和对齐的一拍访存。未覆盖压缩指令、乘除、异常、系统调用、自修改代码、非对齐访存、可变存储延迟或回溯。随机程序是有界覆盖，不是穷举证明。

消费型级间队列、容量背压和 load-use 停顿的契约表达留到下一轮独立改造，需要重新进行全部逐拍验收。C++ GFSim CPU、编译器自动生成版本和正式性能对照尚未实施；它们可复用同一输入 JSON、结束标记和逐拍比较工具。
