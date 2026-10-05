# 与原生 skyzh 逐拍对齐的 ACPy CPU

[model.py](model.py) 用 Queue 保存状态与槽位占用，Signal 表达译码、分派、执行广播和提交控制，23 个独立 Module 分别更新自己拥有的状态。全部硬件行为由 ACPy 经 MLIR 生成；测试 runner 只装载程序、驱动和观察。

目标参考是未修改的 skyzh `out-of-order` 提交 `8989a09c357a69b68612f653380d60816f5176c2`。本例保留该版本的微架构行为及已知错误，因此**参考逐拍一致与 RV32I 正确是两项独立检查**，见 [分析记录](docs/findings.md)。

## 目录与结构

| 文件 | 职责 |
| --- | --- |
| [types.py](types.py)、[logic.py](logic.py) | 数据类型、译码、定宽 ALU 和分支判断 |
| [frontend.py](frontend.py) | 取指、源操作数、空槽选择、分派和 PC 更新 |
| [execution.py](execution.py) | 4 路 ALU、3 个 Store / 3 个 Load 站的独立执行、广播、提交控制 |
| [storage.py](storage.py) | ROB entry、保留站、指针、RAT/RF、内存及预测器的状态更新 |
| [model.py](model.py) | Queue、Signal 和 Module 的静态连接 |
| [tests/](tests/) | 两个独立模型的观察适配、汇编输入、逐拍比较、独立 ISA 解释器 |
| [tools/](tools/) | CMake 构建接入；benchmark 先验证时序，再串行交替计时 |
| [docs/](docs/) | 模型边界、性能问题与参考兼容行为记录 |

ROB 是 8 个容量 1 的 Queue，环形 head/tail 保留一格，可用 7 项。10 个保留站各有容量 1 的 Queue。内存和预测器均与参考同规模：4 MiB 内存、4 MiB 历史字节、4 MiB 计数器，按 256 字节地址页组织 Queue；运行时扫描保留为循环。

```mermaid
flowchart LR
  PC[PC / 内存 Queue] --> F[取指 Signal]
  RF[RAT / RF / ROB Queue] --> O[源操作数 Signal]
  F --> D[分派 Signal]
  O --> D
  RS[保留站 Queue] --> D
  RS --> E[ALU / LSU Signal]
  E --> B[完成广播 Signal]
  ROB[ROB Queue] --> C[提交 Signal]
  D --> U[各状态所属 Module]
  B --> U
  C --> U
  U --> X[统一 Xfer]
```

Signal 链在初始化和 Queue Xfer 后按静态拓扑序计算，不增加流水拍。每拍只使用旧占用/ready 状态选择分派、执行和提交：

- 当拍释放的 ROB/保留站不能立即再分配；当拍新完成的 ROB 下一拍才能提交。
- 广播同时唤醒已有条目和当拍新分派条目；新就绪操作数下一拍才能执行。
- 同拍重命名与提交写同一寄存器时，先考虑新 tag，再决定是否清除 RAT Busy。
- Load 分为地址、读内存、完成三个阶段；Store 分为地址、数据完成两个阶段。
- 分支误预测和 JALR 在提交拍恢复，一次 Xfer 清空各 entry Queue 并复位指针/LSU 阶段，覆盖当拍分派。
- JALR 原子占用两个 ALU 站和两个 ROB 项，分别计算链接值与目标地址。

## 构建与验收

需要 [编译器工具链](../../README.md) 和固定原生参考；以下命令从仓库根目录执行。已有参考仓库时跳过 clone。

```bash
git clone --branch out-of-order https://github.com/skyzh/RISCV-Simulator.git reference/skyzh-riscv-reference
git -C reference/skyzh-riscv-reference checkout 8989a09c357a69b68612f653380d60816f5176c2
export LD_LIBRARY_PATH=/home/lc/opt/gcc14/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
cmake -S pycircuit -B reference/builds/acpy-release -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/home/lc/opt/pycircuit-dev/bin/cc \
  -DCMAKE_CXX_COMPILER=/home/lc/opt/pycircuit-dev/bin/c++ \
  -DMLIR_DIR=/home/lc/opt/llvm-22.1.8/lib/cmake/mlir
cmake --build reference/builds/acpy-release -j6
ctest --test-dir reference/builds/acpy-release --output-on-failure -j4
```

CMake 构建原生适配器并检查参考版本和工作区未修改；可用 `-DSKYZH_REFERENCE_SOURCE=/path/to/checkout` 指定位置。单独运行 CPU 验收：

```bash
python3 -m pycircuit.examples.skyzh_ooo.tests.verify \
  --compiled reference/builds/acpy-release/examples/skyzh_ooo/acpy-skyzh-compiled \
  --emitted reference/builds/acpy-release/examples/skyzh_ooo/acpy-skyzh-emitted \
  --no-opt reference/builds/acpy-release/examples/skyzh_ooo/acpy-skyzh-noopt \
  --reference reference/builds/acpy-release/examples/skyzh_ooo/reference/skyzh-reference \
  --output reference/benchmarks/skyzh-alignment/check
```

每个程序比较直接编译、MLIR 重载、关闭优化三版及 Module 正反序，共 42 次生成模型运行。逐拍核对 PC、ROB 指针和占用、有效 ROB/RS 字段、RAT、RF、LSU 阶段、程序范围内预测器与每条提交；最终核对全部内存变化和完整预测器哈希。空槽和未就绪值等无效字段归零后比较。原生适配始终调用 `Session::tick()`，没有复刻其调度。

正常程序另逐条提交对照独立 RV32I 解释器；三个已知错误程序要求复现参考偏差，避免误报 ISA 通过。覆盖断言包括 ROB 满、同拍多路完成、槽位复用、Load 三阶段、JALR 双分派、分派/提交重命名冲突、分派旁路和在途 flush。

| 程序 | ACPy / 原生周期 | 独立 ISA |
| --- | ---: | --- |
| alignment | 83 / 83 | 通过 |
| full_window | 29 / 29 | 通过 |
| window | 671 / 671 | 通过 |
| branches | 136 / 136 | 通过 |
| integer | 27 / 27 | 预期复现 SRAI 错误 |
| memory | 23 / 23 | 预期复现 LB / 重叠访存错误 |
| auipc | 40 / 40（固定观察窗口） | 预期停滞，未完成程序 |

CTest 完整输出可用 `--output-junit ctest.xml` 保存在构建目录；CPU 轨迹和摘要在该构建的 `examples/skyzh_ooo/evidence/`。Sanitizer 构建命令见 [编译器说明](../../README.md)。

## 性能比较

```bash
python3 -m pycircuit.examples.skyzh_ooo.tools.benchmark \
  --generated reference/builds/acpy-release/examples/skyzh_ooo/acpy-skyzh-compiled \
  --reference reference/builds/acpy-release/examples/skyzh_ooo/reference/skyzh-reference \
  --output reference/benchmarks/skyzh-aligned --repeats 7 --iterations 4096
```

仅使用两边都通过 ISA 检查的 window/branches。先比较扩展程序完整逐拍轨迹，再计时同一固定 N 次 `step()/tick()`。两边统一 Clang 22、C++20、`-O3 -DNDEBUG`，绑定同一 CPU，各预热一个进程，然后串行交替七轮。构造、装载、停止检查、快照和 JSON 均在计时外；GFSim 首次 Signal 初始化计入。报告周期、IPC、ns/tick、架构指令/秒及七轮原始样本，构造耗时和进程峰值 RSS 另列。

测量结果写入 `reference/benchmarks/skyzh-aligned/results.json`，包含源码/二进制指纹和编译参数。旧 CPU 的流水、ROB 容量和内存配置不同，其性能数字不能直接当成本次模型的前后优化比。

**时序对齐：**7 个短程序 × 3 种生成方式 × 2 种 Module 顺序，共 42 次运行逐拍一致；下面两个长程序的完整逐拍状态和架构提交也与原生一致，并通过独立 RV32I 检查。AUIPC、SRAI、LB 等参考已知错误仍按上面的验收表单独记录。

**运行速度：**2026-10-04，aarch64 CPU 0，Clang 22.1.8、`-O3 -DNDEBUG`、无 LTO；每个程序循环 4096 次，串行交替七轮取中位数。ns/tick 越低越快，耗时倍数为 ACPy / 原生：

| 程序 | 两边共同周期 | 共同 IPC | ACPy ns/tick | 原生 ns/tick | 耗时倍数 |
| --- | ---: | ---: | ---: | ---: | ---: |
| window | 40,991 | 0.9994 | 14,662.3 | 297.6 | 49.26× |
| branches | 12,328 | 0.9979 | 13,315.8 | 316.8 | 42.03× |

当前生成模型的模拟耗时约为原生的 **42–49 倍**，两边模拟周期数相同。构造、装载及轨迹输出不在计时范围内；原始样本、源码指纹和额外指标保存在本地 `reference/benchmarks/skyzh-aligned/results.json`，测量产物不纳入 Git。

后续采样与 A/B 实验已记录在 [性能问题记录](docs/findings.md#performance-findings)：生成 C++ 的多余清零／聚合值拷贝和无变化 revise 是已确认的优化项。实验合并后耗时下降约 59%–61%，仍保留 17–19 倍差距；这些修改尚未应用到正式实现。

旧模型和实验已归档到 `reference/benchmarks/skyzh-before-alignment/`；通用表达探针移到 [编译器 fixtures](../../tests/fixtures/skyzh/)。示例目录只保留当前模型、验收及 benchmark 入口。
