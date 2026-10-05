# 端到端 ACPy Ripes5

[model.py](model.py) 表达 Fetch、Decode、Execute、Memory、Writeback 五个阶段，Queue 保存流水和寄存器状态，Signal 共享 EX 计算和 load-use 控制。[logic.py](logic.py) 保存纯译码与运算；C++ 由 ACPy → MLIR → EmitC 生成，宿主 runner 只装载、驱动和观察。

根目录仅保留微架构描述和纯计算辅助代码。子目录分工如下：

| 目录 | 内容 |
| --- | --- |
| [tests/](tests/) | 五方逐拍对照、测试程序、固定周期接口与流式观察测试 |
| [tools/](tools/) | CMake 构建接入、统一构建脚本及 benchmark 工具 |

[tests/verify.py](tests/verify.py) 将直接编译、MLIR 重载、手写 GFSim C++、Python 参考与固定原生 Ripes 做逐拍对照。编译器正常构建和验收见 [使用说明](../../README.md)；下面是统一编译选项的三方 benchmark 入口。Python 测试和工程入口均从仓库根目录使用 `python3 -m pycircuit.examples.ripes5.tests.<名称>` 或 `python3 -m pycircuit.examples.ripes5.tools.<名称>` 调用。

从仓库根目录执行以下命令。本机使用现有 GCC 14 和 `/tmp/gfsim-ripes-qt`；首次安装原生依赖见 [参考构建说明](../../../gfsim/experiment/examples/ripes5/README.md)。

```bash
# 统一 GCC 14、C++20、-O3 -DNDEBUG、armv8-a/generic，关闭 LTO。
export LD_LIBRARY_PATH=/home/lc/opt/gcc14/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
# LLVM 的导出配置包含系统库路径；优先链接匹配 GCC 14 的 libstdc++。
export LDFLAGS="-L/home/lc/opt/gcc14/lib${LDFLAGS:+ $LDFLAGS}"
python3 -m pycircuit.examples.ripes5.tools.build_benchmark

# 包括输入协议、13 程序 × 两种 Module 顺序和固定周期边界测试。
ctest --test-dir reference/builds/ripes5-benchmark \
  --output-on-failure \
  --output-junit ctest.xml

# 自动重做短程序验收和五个长程序逐拍检查，再串行轮换测速。
python3 -m pycircuit.examples.ripes5.tools.bench \
  --generated-runner reference/builds/ripes5-benchmark/acpy-ripes5-compiled \
  --cpp-runner reference/builds/ripes5-benchmark/gfsim/gfsim-ripes5
```

`build_benchmark.py --build PATH --native-build PATH --cxx PATH --qt-prefix PATH -j N` 可覆盖路径及并发数。原生源码默认在 `reference/ripes-reference/`，原生构建在 `reference/builds/ripes-reference/`；复用固定且校验为干净的源码和依赖；不修改上游源码。构建清单保存实际编译及链接命令、编译器版本、翻译单元与二进制散列。测速会核对清单和原生提交身份；统一构建后运行，不能拿其他构建的二进制替换。

`bench.py --verify-only` 完成同样的全部正确性门槛，只写验证摘要。`--runner` 指定原生二进制，`--manifest` 指定统一构建清单，`--cpu` 选择允许 affinity 内的 CPU，`--warmup-cycles` 改 K，`--repeats` 改采样次数（默认 15），`--evidence` 和 `--output` 改结果位置。正式默认 K=1024；每个窗口要求 N≥100,000。运行失败不会产生新的性能汇总；已有文件应按其中时间和散列识别。

## 程序及正确性门槛

[tests/benchmark_programs.py](tests/benchmark_programs.py) 复用现有汇编器、`make_case` 和 JSON／数字输入协议。五个独立程序均有显式初始寄存器和内存，以 12,000 次有界循环产生有效计算，末尾使用原有唯一 marker 与安全循环后缀：

| 程序 | 主要行为 | 独立结果检查 |
| --- | --- | --- |
| independent_integer | 八路独立整数累加 | x1…x8 为迭代次数的 1…8 倍 |
| forwarding_chain | 连续 ALU 前递依赖 | 按最终迭代值计算整条依赖链结果 |
| branch_flush | 交替 taken 分支、JAL、错误路径 store | 计数和累加结果；错误路径不得破坏 sentinel |
| load_use | 四组紧邻 load→use | 固定初值乘迭代次数 |
| consecutive_memory | 连续地址加载、累加、存储 | 四个内存字及对应寄存器累计值 |

全套 13 程序 × 两种 Module 顺序仍比较生成 C++、ACIR 重载 C++、手写 C++、Python 和原生版。长程序对比三方逐拍状态、控制、退休及 store 事件；另同时运行原生 `--observe`，核对观察通知开启和关闭的所有字段（包括 raw）。流式读取四个子进程，只保留三拍历史、当前拍及首个差异后三拍，成功仅保存摘要、散列及 K/T 边界。临时 stderr 使用文件，超时会终止生产者。

短程序测试覆盖旧调用、通知切换、K=0／不同 K、第一拍／流水填充／marker 前一拍／marker 提交边界，以及负数、非整数、溢出、N=0、超出输入 max_cycles 和参数个数错误。每次固定模式采样再核对总周期、窗口退休增量以及最终流水线、寄存器和内存状态。

## 计时接口

```bash
# GFSim 共用 runner：Module 顺序由 v2 数字输入指定。
acpy-ripes5-compiled --benchmark-fixed K N < input.txt
gfsim-ripes5 --benchmark-fixed K N < input.txt

# 原生适配 runner：默认关闭两种观察通知和反向历史。
ripes5-reference input.json --benchmark-fixed K N
ripes5-reference input.json --observe  # 仅用于通知开启的轨迹验收
```

N 必须大于 0，K≥0，K+N≤输入 max_cycles。固定模式执行恰好 K+N 拍，不自行搜索 marker；测速脚本先通过正常逐拍执行取得 marker 提交周期 T，再令 N=T−K。计时区间内仅循环 `step()` 或 `clockUnguarded()`，无宿主逐拍检查、结束判断或快照。模型内部检查和统计保持原样。

JSON 输出包含 `warmup_cycles`、`measured_cycles`、`cycles`、`retired_before`、`retired`、`retired_delta`、`run_ns`、`construct_ns` 和 `final_state`。GFSim 保留调度统计，原生输出通知设置。`construct_ns` 包括模型构造与装载，不含输入解析。`run_ns` 只含 N 拍核心执行。脚本的 `process_ns` 是另外测量的子进程耗时，包含进程启动、输入、构造、预热、输出及退出。

每版先从相同初态完整执行一次预热；再重新启动进程采样 15 次，按三方循环轮换的顺序串行执行。主表使用 Module 正序。退休吞吐使用 `retired_delta / run_ns`，不把 K 拍的退休数计入。报告给出中位数和 [Q1,Q3]（inclusive 线性插值），速度比为同批中位耗时之比。

## Benchmark 摘要

**时序对齐：**13 个短程序 × 2 种 Module 顺序通过五方逐拍对照；下面五个长程序的生成 C++、手写 GFSim 和原生 Ripes 周期数、逐拍状态、退休及 store 事件一致，原生观察通知开关对照和固定周期接口检查也全部通过。

**运行速度：**2026-10-04，aarch64 CPU 0，统一 GCC 14.4.0、C++20、`-O3 -DNDEBUG -march=armv8-a -mtune=generic`，无 LTO。每个程序循环 12,000 次，三方串行轮换 15 轮取中位数，使用 Module 正序；ns/tick 越低越快。

| 程序 | 三方共同总周期 | ACPy ns/tick | 手写 GFSim ns/tick | 原生 Ripes ns/tick |
| --- | ---: | ---: | ---: | ---: |
| independent_integer | 144,003 | 1,434.9 | 1,103.4 | 7,029.9 |
| forwarding_chain | 144,003 | 1,452.6 | 1,138.9 | 7,092.4 |
| branch_flush | 144,003 | 1,389.4 | 1,066.3 | 7,491.6 |
| load_use | 192,003 | 1,360.8 | 1,035.0 | 7,598.4 |
| consecutive_memory | 192,003 | 1,528.0 | 1,174.6 | 7,090.6 |

本组程序中 ACPy 生成模型比原生 Ripes 快 **4.64–5.58 倍**，每拍耗时比手写 GFSim 高约 **28%–31%**。计时排除前 1,024 拍预热、构造、输入及轨迹输出；表中周期数包含预热。原始样本和构建指纹保存在本地 `reference/benchmarks/ripes5/timing-fixed.json`，测量产物不纳入 Git。

## 结果位置

- `reference/builds/ripes5-benchmark/build-manifest.json`：实际编译、链接参数及构建身份。
- `reference/builds/ripes5-benchmark/ctest.xml`：本次构建回归结果。
- `reference/benchmarks/ripes5/acceptance/`：13×2 完整验收及短程序轨迹。
- `reference/benchmarks/ripes5/<程序>/`：长程序输入、流式验证摘要，失败时保存首个差异。
- `reference/benchmarks/ripes5/fixed-runner-tests.json`：固定周期接口验收。
- `reference/benchmarks/ripes5/timing-fixed.json`：源码与二进制散列、原始样本、顺序及统计结果。

结果按实际源码和二进制重新生成，示例目录不保存历史测速快照。固定参考的行为范围、输入格式及 JALR 差异见 [参考说明](../../../gfsim/experiment/examples/ripes5/README.md)。
