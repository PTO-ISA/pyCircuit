# Ripes5 三方公平性能对比

比较 ACPy 生成版、手写 GFSim C++ 与固定原生 Ripes5 的完整模型执行效率。当前结果见 [benchmark-report.md](benchmark-report.md)，原始样本见 [timing-fixed.json](timing-fixed.json)。模型、调度器及上游流水线实现不因测速改变。

从仓库根目录执行以下命令。本机使用现有 GCC 14 和 `/tmp/gfsim-ripes-qt`；首次安装原生依赖见 [参考构建说明](../../../gfsim/experiment/examples/ripes5/README.md)。

```bash
# 统一 GCC 14、C++20、-O3 -DNDEBUG、armv8-a/generic，关闭 LTO。
python3 pycircuit/examples/ripes5/build_benchmark.py

# 包括旧输入测试、13 程序 × 四配置，以及新增固定周期边界测试。
ctest --test-dir pycircuit/examples/ripes5/output/fair-build \
  --output-on-failure \
  --output-junit /home/lc/tmp/pycircuit/examples/ripes5/output/fair-build/ctest.xml

# 自动重做短程序验收和五个长程序逐拍检查，再串行轮换测速。
python3 pycircuit/examples/ripes5/bench.py \
  --generated-runner pycircuit/examples/ripes5/output/fair-build/acpy-ripes5-compiled \
  --cpp-runner pycircuit/examples/ripes5/output/fair-build/gfsim/gfsim-ripes5

python3 pycircuit/examples/ripes5/report.py --benchmark-only
```

`build_benchmark.py --build PATH --native-build PATH --cxx PATH --qt-prefix PATH -j N` 可覆盖路径及并发数。默认复用已固定、校验为干净的原生源码和依赖；不修改上游源码。构建清单保存实际编译及链接命令、编译器版本、翻译单元与二进制散列。测速会核对清单和原生提交身份；统一构建后运行，不能拿其他构建的二进制替换。

`bench.py --verify-only` 完成同样的全部正确性门槛，只写验证摘要。`--runner` 指定原生二进制，`--manifest` 指定统一构建清单，`--cpu` 选择允许 affinity 内的 CPU，`--warmup-cycles` 改 K，`--repeats` 改采样次数（默认 15），`--evidence` 和 `--output` 改结果位置。正式默认 K=1024；每个窗口要求 N≥100,000。运行失败不会产生新的性能汇总；已有文件应按其中时间和散列识别。

## 程序及正确性门槛

[benchmark_programs.py](benchmark_programs.py) 复用现有汇编器、`make_case` 和 JSON／数字输入协议。五个独立程序均有显式初始寄存器和内存，以 12,000 次有界循环产生有效计算，末尾使用原有唯一 marker 与安全循环后缀：

| 程序 | 主要行为 | 独立结果检查 |
| --- | --- | --- |
| independent_integer | 八路独立整数累加 | x1…x8 为迭代次数的 1…8 倍 |
| forwarding_chain | 连续 ALU 前递依赖 | 按最终迭代值计算整条依赖链结果 |
| branch_flush | 交替 taken 分支、JAL、错误路径 store | 计数和累加结果；错误路径不得破坏 sentinel |
| load_use | 四组紧邻 load→use | 固定初值乘迭代次数 |
| consecutive_memory | 连续地址加载、累加、存储 | 四个内存字及对应寄存器累计值 |

全套 13 程序 × 四配置仍比较生成 C++、ACIR 重载 C++、手写 C++、Python 和原生版。长程序对比三方逐拍状态、控制、退休及 store 事件；另同时运行原生 `--observe`，核对观察通知开启和关闭的所有字段（包括 raw）。流式读取四个子进程，只保留三拍历史、当前拍及首个差异后三拍，成功仅保存摘要、散列及 K/T 边界。临时 stderr 使用文件，超时会终止生产者。

短程序测试覆盖旧调用、通知切换、K=0／不同 K、第一拍／流水填充／marker 前一拍／marker 提交边界，以及负数、非整数、溢出、N=0、超出输入 max_cycles 和参数个数错误。每次固定模式采样再核对总周期、窗口退休增量以及最终流水线、寄存器和内存状态。

## 计时接口

```bash
# GFSim 共用 runner：缓存和 Module 顺序由数字输入指定。
acpy-ripes5-compiled --benchmark-fixed K N < input.txt
gfsim-ripes5 --benchmark-fixed K N < input.txt

# 原生适配 runner：默认关闭两种观察通知和反向历史。
ripes5-reference input.json --benchmark-fixed K N
ripes5-reference input.json --observe  # 仅用于通知开启的轨迹验收
```

N 必须大于 0，K≥0，K+N≤输入 max_cycles。固定模式执行恰好 K+N 拍，不自行搜索 marker；测速脚本先通过正常逐拍执行取得 marker 提交周期 T，再令 N=T−K。计时区间内仅循环 `step()` 或 `clockUnguarded()`，无宿主逐拍检查、结束判断或快照。模型内部检查和统计保持原样。

JSON 输出包含 `warmup_cycles`、`measured_cycles`、`cycles`、`retired_before`、`retired`、`retired_delta`、`run_ns`、`construct_ns` 和 `final_state`。GFSim 保留调度统计，原生输出通知设置。`construct_ns` 包括模型构造与装载，不含输入解析。`run_ns` 只含 N 拍核心执行。脚本的 `process_ns` 是另外测量的子进程耗时，包含进程启动、输入、构造、预热、输出及退出。

每版先从相同初态完整执行一次预热；再重新启动进程采样 15 次，按三方循环轮换的顺序串行执行。主表仅缓存开、Module 正序。退休吞吐使用 `retired_delta / run_ns`，不把 K 拍的退休数计入。报告给出中位数和 [Q1,Q3]（inclusive 线性插值），速度比为同批中位耗时之比。

## 证据位置与历史口径

- `output/fair-build/build-manifest.json`：有效编译、链接命令及构建身份。
- `output/fair-build/ctest.xml`：当前构建回归结果。
- `output/fair-benchmark/acceptance/`：13×4 完整验收及短程序轨迹。
- `output/fair-benchmark/<程序>/`：长程序 JSON／数字输入、流式验证摘要；失败时首个差异。
- `output/fair-benchmark/fixed-runner-tests.json`：固定周期接口验收。
- `timing-fixed.json`：本批输入、源码和二进制散列、原始样本、预热、顺序、统计和比值。
- `benchmark-report.md`：本批报告。

历史 [timing.json](timing.json) 与 [report.md](report.md) 保留短程序、七次采样、含结束检查和首次 Signal 初始化的测量口径。原生观察通知设置也与历史不同，不能用跨批数字推算当前三方速度比。新结果描述完整模型；调度器各部分性能归因留待后续剖析。
