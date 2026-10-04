# Signal 静态依赖验证与性能对照

2026-10-03。实现与调度契约见 [spec](../spec.md)，原始样本、输入、源文件和二进制指纹见 [signal-static-results.json](signal-static-results.json)。本次基线取自修改前的工作区快照 `/tmp/signal-static-before/source.tar`，包含原有未提交工作，不使用 Git HEAD 代替基线。

## 实现

Queue→Signal 输入连接在 freeze 时固定、去重；全部 Queue Xfer 完成后，每个受影响 Signal 最多求值一次。输出变化才应用按 Module 分组的固定 Rule dirty 掩码，并安排下一拍 Work。`declareInput(ruleId, signal)` 隐含所属 Module 激活，`declareResource(moduleId, signal)` 只激活 Module。候选提交、取消、缓存和未执行分支均不修改连接。

Signal 读值和 helper 内 Queue 读取不登记动态依赖；Queue 对 Module／Rule 的动态追踪保持原有行为。ACPy 从现有 ACIR 参数目标集及资源引用生成 Rule 绑定，独立 ACIR 重载使用同一路径；Work 传入普通值仍使用参数比较。Python 参考、手写模型及观察器同步更新，求值次数改为独立 evaluations 计数。

## 验证

| 检查 | 结果 |
| --- | --- |
| GCC 14.4 Release CTest | 13/13 通过，包含 19 项编译器语言／电路测试 |
| Clang 22.1.8 Debug + ASan/UBSan/LSan CTest | 13/13 通过 |
| Python 参考及观察器 | 31/31 通过，最终静态声明／观察器专项 5/5 通过 |
| Ripes5 五方逐拍对照 | Release、Debug 各 13 程序 × 4 配置，52/52 通过 |
| 乱序 CPU | Release、Debug 各 20 程序 × 2 生成路径 × 4 配置，160/160 通过；保留独立架构参考 |
| 长程序前后对照 | 5 程序，144003 或 192003 拍，前后生成版／手写版与固定 Ripes 逐拍哈希全部一致 |

新增测试覆盖未选输入分支、Queue 数组及别名、多输入同拍变化、完整 Xfer 屏障、输出不变过滤、未读 Signal 的 Rule、只绑定 Module、跨第 63／64／129 位掩码，以及缓存、提交、abort、取消与未选中生命周期。Debug 捕获缺失输入声明、缺失 Module／Rule 绑定和非法 Signal helper 操作。ACIR 测试删除原源码后重载，验证不同调用位置、分支和资源选择的目标并集，以及普通值参数的缓存命中。

使用 `-DNDEBUG -E` 检查 Release 的 `Simulator::observe`：Signal 求值路径没有输入声明表访问、二分查找、输入代号写入或依赖位图登记；Signal 读值路径也直接返回。相关声明检查仅在 Debug 编译。未初始化、跨 Signal 读取和非法状态修改错误仍保留。

日志与 JUnit 位于 `/tmp/signal-static-release-tests.*`、`/tmp/signal-static-asan-complete.*` 和 `/tmp/signal-static-python.log`，摘要及哈希写入结果 JSON。LSan 在沙箱内受 ptrace 限制，获准在沙箱外重跑完整套件后通过；没有关闭泄漏检测。

## 同核对照

前后均为 GCC 14.4.0、C++20、`-O3 -DNDEBUG`，CMake 默认架构设置，未启用 LTO。固定 CPU 0；缓存开启、Module 正序。复用现有五个 12000 次循环长程序，先运行 1024 拍，再测到 marker 的固定 N 拍；每版预热一次，四版串行轮换采样 15 次。构造、载入、快照和终止判断不计入计时窗口。每个样本均校验最终状态。

下表为中位 ns/拍，变化率是 `(后 / 前 - 1) × 100%`。完整耗时、四分位数与全部样本在 JSON 中。

| 程序 | 测量拍数 | 生成版 前→后 | 变化 | 手写版 前→后 | 变化 |
| --- | ---: | ---: | ---: | ---: | ---: |
| independent_integer | 142979 | 2560.5 → 2514.3 | -1.80% | 2190.1 → 2102.5 | -4.00% |
| forwarding_chain | 142979 | 2608.6 → 2575.0 | -1.29% | 2210.9 → 2144.6 | -3.00% |
| branch_flush | 142979 | 2366.7 → 2303.6 | -2.67% | 2003.8 → 1940.5 | -3.16% |
| load_use | 190979 | 2443.9 → 2381.3 | -2.56% | 2009.7 → 1935.1 | -3.71% |
| consecutive_memory | 190979 | 2776.3 → 2708.9 | -2.43% | 2302.9 → 2231.5 | -3.10% |

本批生成版耗时降低 1.29%–2.67%，手写版降低 3.00%–4.00%。这是这些负载与本次构建的测量结果，不外推为通用加速比例。

以下计数包含初始化和 1024 拍预热；四个版本在本批长程序中一致，15 次采样也均稳定。因此本批耗时变化没有伴随求值、Module Work 或 Rule 重算次数的减少。分支输入测试中，求值次数已按新规则增加，不能要求其他模型的旧调度计数不变。

| 程序 | Signal 求值总数（两实例） | Module Work | Rule 重算 |
| --- | ---: | ---: | ---: |
| independent_integer | 288007（144003+144004） | 708010 | 708010 |
| forwarding_chain | 288007（144003+144004） | 708010 | 708010 |
| branch_flush | 288007（144003+144004） | 690010 | 690010 |
| load_use | 384007（192003+192004） | 948010 | 948010 |
| consecutive_memory | 384007（192003+192004） | 948010 | 948010 |

## 复现

从仓库根目录运行；before 构建目录必须来自修改前快照，以相同编译器和 Release 选项构建。

```bash
cmake -S pycircuit -B /tmp/signal-static-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=/home/lc/opt/gcc14/bin/aarch64-conda-linux-gnu-g++
cmake --build /tmp/signal-static-release -j4
ctest --test-dir /tmp/signal-static-release --output-on-failure
PYTHONPATH=gfsim/experiment RIPES5_REQUIRE_NATIVE=1 \
  python3 -m unittest discover -s gfsim/experiment -v
python3 pycircuit/examples/ripes5/bench_signals.py \
  --before-build /tmp/signal-static-before/gcc14 \
  --after-build /tmp/signal-static-release \
  --native gfsim/experiment/examples/ripes5/reference/build/ripes5-reference \
  --evidence /tmp/signal-static-benchmark \
  --output /tmp/signal-static-repeated.json
```

测速脚本先核对全部长轨迹，再固定 CPU 采样；可用 `--verify-only` 和 `--reuse-gates` 分阶段运行。复用证据时会检查输入和所有二进制指纹。Debug 使用同一工程，增加 `-DCMAKE_BUILD_TYPE=Debug -DGFSIM_SANITIZERS=ON`，编译器指定 Clang；测试设置 `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`。
