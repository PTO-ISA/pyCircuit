# C++ 后端验收与性能报告

当前 ACPy／手写 C++／原生 Ripes 的性能结论以 [三方同批固定周期报告](../../pycircuit/examples/ripes5/benchmark-report.md) 为准。本文计时保留历史测量口径，不与新样本计算速度比。

验证日期：2026-10-03。核心按现行 [spec](../spec.md) 实现；Ripes5 是按未来编译器输出形态手写的目标样例。前端和代码生成器接入不在本次交付范围。

当前证据：[构建、检查及代码量](results.json)、[52 配置三方比较](examples/ripes5/results.json)、[全部计时样本](examples/ripes5/timing.json)。完整逐拍 JSONL 位于 `examples/ripes5/output/gfsim-cpp-release` 和 `output/gfsim-cpp-clang-asan`，可按 [构建说明](README.md) 重建。

## 验收

| 检查 | 结果 |
| --- | --- |
| GCC 10.3.1 Release | 5/5 CTest 通过，`-O3 -DNDEBUG` |
| Clang 22.1.8 ASan/UBSan/LSan | 5/5 CTest 通过，Debug + `-O1 -g`，泄漏检查开启，错误立即终止 |
| Ripes5 三方逐拍 | 两种构建均通过 13 程序 × 缓存开关 × Module 正反序，共 52 配置 |
| 原生参考必需 | 缺失 runner 和错误 commit 均按失败处理，单独测试门禁 |
| 安装迁移 | 安装 prefix 搬迁后独立查找、链接、运行通过；Release 和 sanitizer 均覆盖 |
| 独立共享库 | 关闭 tests/examples，不查找 Python；GCC 构建、安装和外部消费通过 |
| Python 回归 | 现有实验 28 项测试通过 |

原生版本为 Ripes `5b8a616edcb6f0a2ddb07e78951348b72497f1e1`、VSRTL `8497dd14fe80e57efcff4c424a9a3b6363d93eb7`。两种构建和测速记录二进制 SHA256 与源码指纹；生成报告时确认这些源码指纹与工作区一致。

系统 GCC 10 缺少 ASan/UBSan 库，内存构建因此改用本机 Clang 22。沙箱的 ptrace 限制使 LSan 退出检查失败；获得执行授权后，在沙箱外运行相同测试，保持 `detect_leaks=1`，全部通过。未关闭泄漏检查。

逐拍比较包含 cycle、五阶段 valid/pc、fetch/next PC、前递、stall/flush、寄存器、内存、退休与 store。只移除原生 `raw` 诊断，不移动周期或忽略有效字段。失败时保存首个不同字段、邻近三拍及全部输入指令。C++ runner 使用标准库数值流解析和 JSONL 输出，汇编及输入 schema 验证复用 Python。

## 调度与生命周期覆盖

- FIFO：跨 tick 保留、无 Work 的容量传播、延迟从获准 tick 起算、1100 级正反序完整排空、静态环、自身 pop/push 和动态容量环终止。
- 请求/存储/响应：动态 Queue array、多拍事件、部分 proposal 后读空 abort、原子取消事件、候选替换/未选中清理、嵌套 bool/array/整值字段修改与响应 scoreboard。
- 配置处理：Module 分支、多 Rule、70 条 Rule 跨 64 位位图、参数变化、无效果缓存、未选路径不订阅、Signal 输入切换但值不变、全部 Xfer 后只求值一次。
- 窄边界：tick/读代号/Signal 代号/版本/统计/事件加法溢出，Work/Xfer 异常、遗漏 complete/abort、重复 pop、零延迟、非法声明和 helper 效果，失败后禁止继续。
- 生命周期：任务、控制数组、读者位图、来源槽位地址稳定；Simulator 析构后资源观察不访问悬空回调。current 引用不能跨 Xfer，资源须存活到 Simulator 析构完成。

模型检查：五阶段业务文件只经 Queue/Signal/Rule 接口读取、提出效果和完成；没有模型维护 dirty、订阅、事件堆或提交状态。状态全部是 Queue，其他成员为固定配置、引用或 runtime 记录。静态 Module/Rule 表、可访问资源、Signal 输入及操作绑定均在构造中列出，可由编译信息机械生成。补充调度电路已移到 tests，公开 examples 仅保留 Ripes5。

## 性能

主机：`Linux-5.10.0-136.12.0.86.r1526_92.hce2.aarch64-aarch64-with-glibc2.34`。选定 CPU：0。同 CPU、串行子进程，每组一次预热和七次采样，轮换九组顺序。C++ 使用 GCC Release。每个程序先验证所有被测配置，再计时。

`run_ns` 是 clock loop、marker 检查与 runtime 计数，包含首次 Signal 初始化；排除构造、快照、轨迹、JSON 和进程启动。`process_ns` 另存完整子进程耗时。原生 Ripes 没有 GFSim 调度计数，未伪造对应值。

下表展示缓存开启、Module 正序的每周期耗时中位数（ns）；全部四种配置及原始七次样本见 timing.json。速度比仅代表这三个完整模型在本机的测量。

| 程序 | 周期 | C++ GFSim ns/cycle | Python GFSim ns/cycle | 原生 Ripes ns/cycle | Python/C++ | Ripes/C++ |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| array_sum | 120 | 2398.0 | 208640.8 | 7909.1 | 87.01 | 3.30 |
| mixed_2026 | 167 | 2347.7 | 210141.2 | 7910.2 | 89.51 | 3.37 |
| memory_loop_256 | 2054 | 2071.9 | 204444.5 | 7545.3 | 98.68 | 3.64 |

对应 C++ 调度计数（缓存开、正序；计数在各采样中一致）：

| 程序 | Module Work | Rule Work | Cache hit | Signal 求值 | Accepted | 登记事件 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| array_sum | 580 | 580 | 0 | 241 | 549 | 1424 |
| mixed_2026 | 806 | 806 | 0 | 335 | 782 | 1926 |
| memory_loop_256 | 10009 | 10009 | 0 | 4109 | 9241 | 24387 |

这些短程序的测量含真实调度及函数调用成本，不能推导所有电路或缓存机制的普遍加速倍数。缓存关闭仍保留未激活 Module 的完整 pending 候选，以遵守跨 tick 仲裁语义。

## 代码量

统计物理行，包含注释/空行；排除文档、JSON、轨迹和复用的 Python 实验。逐文件明细在 results.json。

| 范围 | 行数 |
| --- | ---: |
| 核心：include/gfsim + src | 971 |
| 模型：logic/stages/model.hpp | 475 |
| 工具：runner.cpp、verify.py、bench.py | 360 |
| 测试：原生断言、补充电路、外部 consumer、输入门禁 | 1907 |
| 构建：CMake 与安装配置 | 83 |

支持边界与发现的 lowering 缺口已补充到 [Q15](../open-questions.md#q15)。仍未定义任意位宽值类型、动态 struct 字段路径、重叠字段修改的前端冲突语义、外部驱动、事件取消、父子 Module 和恢复协议。
