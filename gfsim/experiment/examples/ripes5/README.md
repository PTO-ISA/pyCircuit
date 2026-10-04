# Ripes RV32_5S 逐拍对照

本目录独立表达固定版本 Ripes 的五级流水。模型、汇编器与译码器均在本目录，使用上层通用调度器。参考端是原版 `vsrtl::core::RV5S<uint32_t>`，不是 Python 参考流水线。

当前使用两个独立 Signal 共享组合计算，五个普通阶段类保留显式 Rule，由实际读取的 Queue／Signal 变化唤醒，保持寄存器表达的逐拍行为。四个级间 Queue 仍通过 `revise` 更新，不消费、不产生容量背压；本次验收不代表消费型级间队列验证完成。后续需要独立解决输入读取形成 pop、背压及 load-use 停顿在编译契约下的表达。

## 构建与运行

版本由 [reference/version.json](reference/version.json) 固定：Ripes `5b8a616edcb6f0a2ddb07e78951348b72497f1e1`，VSRTL `8497dd14fe80e57efcff4c424a9a3b6363d93eb7`。同时记录全部 FetchContent 依赖 SHA。

本机 aarch64 / Python 3.11 / CMake 3.31 / GCC 14.4，Qt 6.8.3。初次配置确认系统缺少 Qt；通过 conda-forge 在 `/tmp` 安装 Qt，补齐 OpenGL 开发包。无需安装系统软件。Qt Svg 已包含于 `qt6-main`，不存在独立的 `qt6-svg=6.8` 包。可复用完整的 [Qt 环境锁文件](reference/qt-linux-aarch64.lock)：

```bash
/home/lc/opt/miniforge3/bin/conda create -y -p /tmp/gfsim-ripes-qt \
  --file gfsim/experiment/examples/ripes5/reference/qt-linux-aarch64.lock
python3 gfsim/experiment/examples/ripes5/reference/build.py
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.reference.smoke
```

构建脚本首先递归下载源码（默认本目录的 `reference/upstream/Ripes/`），固定提交并校验已有源码没有修改，再配置和构建原版 `Ripes` 及 `ripes5-reference`。已有非指定版本会报错，不会重置用户 checkout。`--source / --build / --qt-prefix / --cxx / -j / --fresh` 可覆盖本机路径。依赖下载需要网络。构建产物默认本目录的 `reference/build/`。原版源码、子模块、FetchContent 下载的依赖和二进制均在此 example 下，其中 `reference/upstream/`、`reference/build/` 由 Git 忽略；Qt SDK 仍由 `/tmp/gfsim-ripes-qt` 提供。版本锁和适配器代码由主仓保存。

目录位置：

```text
ripes5/
  reference/
    upstream/Ripes/         # 固定原版源码，含 external/VSRTL 子模块
    build/                  # Ripes、ripes5-reference 及 CMake 构建依赖
    runner.cpp              # 本仓的逐拍观察适配器
    build.py                # 构建入口
    version.json            # 原版与子模块版本锁
```

原版模型入口为 [rv5s.h](reference/upstream/Ripes/src/processors/RISC-V/rv5s/rv5s.h)，前递和冒险单元也在同一目录。

迁移已有 CMake 构建树后运行 `build.py --fresh`，重新生成绝对路径，并复用已校验的 `_deps/*-src` checkout。

通过 `CMAKE_PROJECT_Ripes_INCLUDE` 添加本目录的 C++ 观察器目标，原版源码没有补丁。观察器只装载初始状态、读取公开端口、调用 `clockUnguarded()`，不替换译码、前递、冒险、存储或状态提交。

当前适配器默认通过公共接口关闭端口变化通知、时钟观察通知，并保持反向历史为 0；`INPUT.json --observe` 恢复两种通知以核对轨迹。新增 `INPUT.json --benchmark-fixed K N`：先执行 K 拍，再仅计时 N 次时钟调用，最后输出状态和退休增量。旧轨迹及 `--benchmark` 调用仍有效。统一 GCC 14 构建、通知切换验收、五类长程序和三方同批计时见 [ACPy Ripes5 测速说明](../../../../pycircuit/examples/ripes5/README.md)。历史计时文件保留各自原有口径。

单命令比较全部程序（原版轨迹每个程序只生成一次）：

```bash
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.run
```

每个程序运行缓存开／关、Module 正序／反序四种配置。`--case array_sum --html` 生成通用 ReviewTrace 页面；`--input input.json` 接受同一输入格式；`--runner` 指定参考二进制。非默认 Qt 路径使用 `RIPES_QT_PREFIX`。默认输出在本目录被 Git 忽略的 `review-output/`：

- 每个程序的 `input.json` 和未经改写的 `ripes.raw.jsonl`、stderr。
- 四份 GFSim JSONL；不匹配时的 `*.mismatch.json` 包含首个差异字段、前后三拍、完整指令字和汇编。
- `summary.json` 保存版本、二进制 SHA256、周期与退休计数。
- 可选的通用 HTML 页面展示 Queue、Rule，以及 `ex_result`／`load_use_stall` 的输入、求值、变化和读者通知；这不是性能计时路径。

验收和耗时入口：

```bash
RIPES5_REQUIRE_NATIVE=1 python3 -m unittest discover -s gfsim/experiment -v
# 保存 56fe061 源码，并逐拍比较旧 Python、新 Python 和原生 Ripes
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.evidence
# 默认三个程序、每组一次预热和七次采样；自动选择一个可用 CPU
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.bench
# 单独生成包含两个 Signal 的四配置可视化
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.run --case array_sum --html
```

原版未构建时常规 unittest 明确 skip 原版测试；`RIPES5_REQUIRE_NATIVE=1` 将其变为失败。`run`、`bench` 均要求真实参考存在且版本匹配，绝不自动替换参考。

## 状态、控制与时钟边界

| 已登记 current | 组合控制（固定版本源码） | 同一拍末更新 |
| --- | --- | --- |
| PC、IF/ID、ID/EX、EX/MEM、MEM/WB | `HazardUnit` 的 EX load 目的寄存器与 ID 源寄存器比较；x0 不触发冒险 | load-use 时保持 PC、IF/ID，清 ID/EX；旧 EX 仍进入 MEM |
| ID/EX、EX/MEM、MEM/WB | EX 前递先 MEM ALU，再 WB 选择值；源 x0 禁止前递 | ALU 结果、前递后的 store 数据进入 EX/MEM |
| ID/EX 与前递值 | EX BEQ/BNE/JAL/JALR 产生重定向，`controlflow_or` 选择 PC mux | 更新 PC，同时清 IF/ID 和 ID/EX；旧 EX/MEM/WB 继续前进 |
| IF/ID、寄存器 Queue、MEM/WB | `RegisterFile<32,true>` 的 WB→ID 旁路 | 解码值进入 ID/EX，WB 的寄存器写在同一个沿提交 |
| EX/MEM、数据 Queue | 同拍组合读一拍存储；ALU/PC+4/读数据选 WB 值 | store 在 MEM 沿写数据；结果进入 MEM/WB |
| MEM/WB | 寄存器写控制与 valid 分别观察 | WB 沿写寄存器并计退休；marker 的此沿为结束边界 |

对应上游文件位于 `src/processors/RISC-V/rv5s/rv5s.h`、`rv5s_hazardunit.h`、`rv5s_forwardingunit.h`、`../rv_registerfile.h` 及 VSRTL `vsrtl_register.h`。`RegisterClEn` 先判 enable，再判 clear；本范围没有 ECALL，ID/EX 总使能，EX/MEM 不清零，MEM/WB 总前进。ID/EX 的 stalled 独立于 clear 保存，然后逐级传递。

`model.py` 显式构造每个资源、五个阶段实例及五条 `RuleEntry`，列出空的 `pops/pushes` 和唯一 `revises` 来源。PC、四个流水寄存器、32 个寄存器、每个数据字、退休和存储事件计数都用容量为 1 的非空 Queue。valid 为 `Slot` 字段，气泡不清空 Queue。规则不能看到其他规则的 proposal。

## 阶段和 Signal 的阅读顺序

[stages.py](stages.py) 保留 Fetch、Decode、Execute、Memory、Writeback 五个普通类，不使用基类、装饰器或提交包装。每条 Rule 明确展示 `begin_rule → peek/Signal.value → propose_revise → complete_rule`。`peek()` 自动登记实际读取，`propose_revise()` 自动登记目标依赖。阶段不再请求下一拍自唤醒。Queue／Signal 变化通过实际读者依赖安排下一 tick 的 Work；获准 revise 同值不通知，依赖稳定时可以省略重复计算。store／退休的事件计数每次实际发生时递增，重复值写入也会保留事件并产生相应依赖通知。

[model.py](model.py) 的两个普通成员 helper 直接读取 Queue 并调用 [logic.py](logic.py) 的纯值函数，再绑定现有通用 `Signal`：

| Signal | 输入 Queue | 固定返回类型 | 阶段读者 |
| --- | --- | --- | --- |
| `ex_result` | ID_EX、EX_MEM、MEM_WB | `ExResult(next_slot, redirect, forward_a, forward_b)` | Fetch、Decode、Execute |
| `load_use_stall` | IF_ID、ID_EX | bool | Fetch、Decode |

两个 Signal 互不依赖。`ExResult` 和 `Slot` 都是不可变 NamedTuple，分支目标直接取 `next_slot.result`，没有可空目标或字典返回值。`module_queues` 只列出阶段实际可能读取或 revise 的 Queue；`module_signals` 与 `signal_queues` 也精确列出。Module 反序时绑定按实际 mid 重排。

Fetch 读取两个 Signal 和 PC，先判断 enable，再处理 clear；Decode 按 redirect/stall 插入气泡，否则读取 IF_ID、寄存器及 WB→ID 旁路；Execute 只提交 `ex_result.next_slot`。Memory 和 Writeback 保留原业务及同沿副作用。没有增加 valid 过滤、手动补唤醒、隐藏缓存或直接状态写入。调度器、仲裁、原版 Ripes 都未修改。

Signal 在首个 `step()` 的 Work 之前初始化；随后每个 Signal 每次 Xfer 最多求值一次。阶段 Rule 不再调用 EX 组合计算。观测仍从 Queue 用纯值 `control()` 生成全部字段，因此第 0 拍不读取未初始化 Signal，也不提前推进时钟。观测会额外进行纯值计算，但完全排除在仿真循环计时之外。

## 基线、验收和测速证据

运行 `evidence`／`bench` 后，证据生成到被 Git 忽略的 `review-output/dependency-wakeup/`：

- `before/source/`：从 `56fe061` 导出的实验源码，`before/identity.json` 保存提交及源码 SHA256。旧模型仍使用原有自唤醒；当前模型依靠 Queue／Signal 依赖。
- `before/traces/`、`after/`：13 程序 × 4 配置的旧／新 JSONL，新目录另含原生原始输出；`comparison.json` 记录三方逐字段通过结果、轨迹散列及源码行数。
- `after/array_sum/*.html`：四份通用 review，包含两个 Signal 的输入、求值和通知。
- `timing.json`：正式三方计时，包括全部样本、预热、执行顺序、CPU affinity、每拍耗时、Rule／Module／Signal 计数、资源变化通知及事件计数、源码和二进制散列。已跟踪的副本见 [timing.json](timing.json)，验收摘要见 [results.json](results.json)。

`evidence` 可重复导出固定提交；若保留的源码被改动则报错，不覆盖异内容。它用当前输入同时运行固定提交模型与当前模型，比较器保持原实现，报告首个差异周期及上下文。

`bench` 默认测 `array_sum`、`mixed_2026`、`memory_loop_256`。每个程序有原生 C++ 一组、旧／新 Python 各四组；绑定同一可用 CPU，子进程串行运行，每组一次预热、七次正式采样，各轮轮换顺序。`--cpu N`、`--repeats N`、重复的 `--case NAME`、`--runner`、`--evidence` 和 `--output` 可覆盖配置。

循环计时保留原结束检查、地址范围检查与内建计数，排除构造、快照、JSON 和 review。Python 第一次 `step()` 的 Signal 初始化计入循环；整个进程计时另含解释器／动态库启动、导入、输入、构造、结果输出及退出。旧／新 Python 共用同一个 worker 与解释器，只切换源码导入路径。Python／C++ 比值描述不同语言的完整模型，不能解释为调度器性能差距。这里测量的原生 Ripes C++ 没有 GFSim Rule／Signal 计数。

过期生成目录及早期 `baseline.json` 已清理。`results.json`、`timing.json` 保留最近一次正式记录，对应源码身份以文件内散列为准。2026-10-03 将 ISA 支持文件从旧 riscv 示例移入本目录；重新运行工具会记录新的路径和散列，不改写原有测量。固定提交 `56fe061` 的旧源码仍可由 `evidence` 从 Git 导出。

## 输入与观察接口

输入 JSON 包含 `words`（从 PC 0 开始的 32 位机器字）、`registers`（32 个 uint32，x0=0）、`data_base`、`data`（uint32 字数组）、`end_pc`、`max_cycles`；`name/source` 仅供工具报告。两端加载同一 JSON，不分别重新汇编。`programs.make_case()` 添加唯一的 `ADDI x0,x0,2047` 结束标记，之后是 `JAL x0,0` 无副作用循环及两个 NOP。HALT、ECALL 不在接口内。

JSONL 第 0 行是初始化并完成组合传播后的状态。第 n 行是第 n 个沿之后的状态，同时也是下一沿的拍前状态；`retire/store` 表示刚发生的沿。没有按程序移动周期：

- `stages` 是 IF/ID/EX/MEM/WB 的 valid 和有效 PC，无效 PC 统一 null；valid 同时要求 PC 位于可执行代码范围。
- `fetch_pc` 为实际 PC current；`next_fetch` 是 PC mux 输出，`next_pc` 考虑 enable 后将提交的 PC。停顿时二者可以不同。
- `control` 包含 stall、两处 flush、enable、EX/MEM clear、重定向目标、两个前递选择（0=ID，1=MEM，2=WB）。`stalled` 是各阶段停顿气泡标记。
- `registers/data` 是沿后的全部建筑状态；`retired` 是累计数，`retire` 含 PC、机器字及可选寄存器写，`store` 为可选 `[地址, 值]`，即使写回相同值也记录事件。
- 原版 `raw` 额外保存所有阶段残留 PC、raw valid、ALU 和 WB 值。比较器只移除这个明确的诊断字段，不比较无效槽的残留 payload。

代码区与数据区分离；首轮仅 aligned RV32I 子集 ADD/ADDI/SUB/AND/OR/XOR/SLT/LUI/LW/SW/BEQ/BNE/JAL/JALR，不测自修改代码、压缩指令、陷阱、异常或非对齐访存／跳转。包括非零初始寄存器和内存、溢出及负数。

## 程序与结论

`programs.py` 保存可读汇编、三个有界固定种子混合程序和统一机器字生成入口。新增 `memory_loop_256` 进行 256 次有界访存累计，每次含 load-use、store 和条件分支，最终写入 768，运行 2,054 拍。程序覆盖顺序吞吐、连续覆盖前递优先级、x0、WB→ID、store 数据前递、数组填充求和、load-use、分支循环、跳转链接、load 后分支及错误路径写入。

Python 验收结果与成本限制见 [findings.md](findings.md)。[C++ GFSim 后端](../../../cpp/examples/ripes5/README.md) 已复用本 JSON 输入、13 个程序和逐拍接口，另有 C++／当前 Python／原生 Ripes 的完整验收及测速。[独立 ACPy 编译器](../../../../pycircuit/README.md) 进一步生成完整模型并加入五方对照；完整模型的耗时对照不等同于调度器性能比较。
