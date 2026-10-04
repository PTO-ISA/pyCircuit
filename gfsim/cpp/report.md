# GFSim 静态调度重构结果

记录日期：2026-10-04。基线为 `ce2d413`，环境为 aarch64、Clang/LLVM/MLIR 22.1.8、GCC 14 sysroot。源码、二进制、测试和全部性能样本见 [static-results.json](static-results.json)。

前文记录首次静态调度重构，含当时的行数和性能；后续仅改变 Queue 来源槽位定位的结果见本文末节及 [slot-results.json](slot-results.json)。

## 实现与验证

- C++ 核心从 **1041 行降到 702 行**，减少 32.6%。统计包括 src 和 include 的全部实现，按现有 clang-format 格式化，包含空行与注释。没有新增隐藏的运行时 helper。
- 删除实际读取登记、资源/Module 稠密映射、Rule dirty/参数缓存、自定义仲裁回调、DFS 栈和环检测。静态资源变化直接激活下一拍 Module；获准 pop 沿固定边推进 delta 重仲裁，睡眠 Module 的完整 pending 保留。
- Queue 存储、来源槽位和 revise/Xfer 实现保持原样；移除读取钩子及 DFS 专用查询。显式延迟事件仍在整条 Rule 获准时按 acceptanceTick+delay 发布。
- Release 与 ASan/UBSan/LeakSanitizer **各 14/14 CTest 通过**；最终补充容量汇合测试也在两种构建下通过。语言套件 25 项，独立安装链接通过。
- Queue OoO 每构建 5 程序 × 2 写回配置 × 3 生成版本 × 2 Module 顺序 = **60 次完整运行**。逐条提交对照独立 RV32I 解释器，全部 10 组基准逐拍哈希与重构前完全一致。Ripes5 13 程序 × 2 顺序的五方逐拍对照通过；既有 OoO 20 场景 × 2 顺序 × 2 生成版本通过。
- 三个 CPU 的直接生成与 MLIR 重载生成的 cpp/hpp/support 逐字一致；单独执行组件的 857 个输入以及 42 个内存 helper 用例通过。CPU 模型源码没有改变，Python 实验引擎没有迁移。

## Queue OoO 同模型对照

两个程序均扩大到 4096 次循环，先核对独立解释器及新旧完整轨迹，再以相同固定 N 次 step() 计时。CPU 191，一次进程预热、七次串行轮换；统一 `-O3 -DNDEBUG -std=gnu++20`，无 LTO。K=0；计时排除构造、装载、结束判断、快照和 JSON，包含首次 Signal 初始化。以下均为同批中位数。

| 程序 | 周期 / 指令 | 旧 ns/tick | 静态 ns/tick | 加速 | 旧/新构造 ms | 旧/新峰值 RSS KiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| window | 61486 / 40967 | 7208.6 | 3901.3 | 1.85× | 3446.2 / 78.6 | 50416 / 37940 |
| branches | 24662 / 12302 | 5373.8 | 3012.2 | 1.78× | 3419.3 / 78.3 | 50436 / 37940 |

RSS 是整进程峰值；构造耗时不包含输入解析。旧版附加计数只在计时之后输出；原始旧可执行文件和附加计数版的身份都保存在结果中。

| 程序 | Module Work 旧→新 | Rule Work 旧→新 | 事件入堆 旧→新 | 接受 Rule 旧/新 |
| --- | ---: | ---: | ---: | ---: |
| window | 532943 → 729570 | 410028 → 594354 | 1226630 → 0 | 336248 / 336248 |
| branches | 197302 → 262914 | 135809 → 193203 | 400671 → 0 | 107047 / 107047 |

静态连接扩大了通知范围，Work 和部分仲裁尝试增加；旧版在这些程序里仅命中 1 次候选缓存。省去依赖登记、位图、参数检查和普通事件堆操作仍带来净收益。容量通知不重新 Work 的性质由专门的跨拍链和多输出容量汇合测试验证。这里是整组简化的效果，不能单独归因某一个被删除的函数。

## 与原生 skyzh 的剩余差距

原生仍固定在 `8989a09c357a69b68612f653380d60816f5176c2`，以独立解释器为正确性准绳。两种 CPU 的 ROB 容量、流水和恢复策略不同；这个比较不是纯调度器比值。

| 程序 | 模型 | cycles | IPC | ns/tick | 架构指令/秒 |
| --- | --- | ---: | ---: | ---: | ---: |
| window | 静态 Queue/GFSim | 61486 | 0.666 | 3901.3 | 170786 |
| window | 原生 skyzh | 40991 | 0.999 | 325.7 | 3068332 |
| branches | 静态 Queue/GFSim | 24662 | 0.499 | 3012.2 | 165602 |
| branches | 原生 skyzh | 12328 | 0.998 | 343.5 | 2905184 |

静态版每拍仍慢约 8.8–12.0 倍，按架构指令吞吐仍慢约 17.5–18.0 倍。剩余成本包含 CPU 组合扫描、Queue/proposal 虚调用与查找、值拷贝和 revise 回调；本次没有单独测量这些部分，也没有为追赶原生而更改微架构。

## Ripes5 与编译成本

五个相同长程序先核对新旧完整逐拍哈希及独立计算的最终状态。固定 CPU 191，K=1024 预热后计时 N 拍，每版一次进程预热、七次轮换。编译成本各采三次，串行测量；包含编译工具进程启动，不含首次构建 MLIR 工具。

| 程序 | 测量 tick | 旧 ns/tick | 静态 ns/tick | 加速 |
| --- | ---: | ---: | ---: | ---: |
| independent_integer | 142979 | 2557.3 | 1335.4 | 1.92× |
| forwarding_chain | 142979 | 2609.9 | 1376.7 | 1.90× |
| branch_flush | 142979 | 2376.1 | 1308.3 | 1.82× |
| load_use | 190979 | 2399.7 | 1247.9 | 1.92× |
| consecutive_memory | 190979 | 2717.2 | 1431.7 | 1.90× |

| 编译指标 | 旧 | 静态 |
| --- | ---: | ---: |
| 前端与发射 ms | 230.65 | 227.01 |
| 生成 C++ 编译 ms | 5126.46 | 4971.33 |
| 前端峰值 RSS KiB | 19948.00 | 19900.00 |
| C++ 编译峰值 RSS KiB | 158632.00 | 152936.00 |
| Ripes5 生成字节 cpp/hpp/support | 81354 | 79908 |
| Queue OoO 生成字节 cpp/hpp/support | 200543 | 195001 |

## 重现

按 [编译器 README](../../pycircuit/README.md) 分别构建 Release 和开启 `GFSIM_SANITIZERS` 的 Debug。基线可用 `git archive ce2d413` 保存到独立目录，再用同一工具链构建；不要用修改后的源码重新构建旧目录。当前测量所用基线源码为 `/tmp/gfsim-static-baseline/src`，原始构建和二进制保存在 `/tmp/acpy-mlir-build`。

```bash
ctest --test-dir /tmp/gfsim-static-build --output-on-failure -j4
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir /tmp/gfsim-static-asan --output-on-failure -j4
python3 -m pycircuit.examples.skyzh_ooo.benchmark \
  --generated /tmp/gfsim-static-build/examples/skyzh_ooo/acpy-skyzh-compiled \
  --baseline /tmp/acpy-mlir-build/examples/skyzh_ooo/acpy-skyzh-compiled \
  --reference /tmp/skyzh-mlir-reference-final/skyzh-reference \
  --output /tmp/gfsim-static-skyzh-benchmark --cpu 191 --repeats 7
python3 -m pycircuit.benchmark_migration \
  --baseline-source /tmp/gfsim-static-baseline/src --baseline-build /tmp/acpy-mlir-build \
  --build /tmp/gfsim-static-build --cxx /home/lc/opt/pycircuit-dev/bin/c++ \
  --output /tmp/gfsim-static-ripes-benchmark --cpu 191 --repeats 7
```

基线原始 runner 不输出调度计数，上述命令仍可复现性能及逐拍检查；本批附加计数版由相同旧模型/runtime 静态库链接，仅增加计时外 JSON 输出。脚本支持有无计数字段。Ripes5 的旧输入协议 v1 仅在 benchmark 基线适配器中使用；当前 runner 协议 v2 删除 cache 字段。

测试日志为 `/tmp/gfsim-static-ctest.log`、`/tmp/gfsim-static-asan-ctest.log`，CPU 大轨迹位于各构建目录的 `examples/skyzh_ooo/evidence`。历史动态 Signal 结果见 [signal-static-report.md](signal-static-report.md)；它不代表当前调度器。

## 单独优化 Queue 来源槽位

以 `/tmp/gfsim-static-build` 的静态版为基线，仅将 Queue 按 RuleId 二分查找槽位改为 freeze 时建立的只读直接索引。proposal、仲裁、接受和撤销使用同一张映射；来源检查、参与列表、pop 去重、revise 回调、Xfer 和调度顺序保持不变。三个 CPU 的生成 cpp/hpp/MLIR/support 均与基线逐字一致。修改前源码保存在 `/tmp/gfsim-slots-baseline/src`，基线构建没有重新编译。

同来源集合的 Queue 共用映射，只为首末非零 RuleId 之间的区间分配索引，空隙标记为无连接。Queue OoO 的 65,701 个 Queue 共用 18 张表、60 个索引项（480 字节索引数据，不含容器、键及分配器开销）；每个 Queue 增加一个映射指针。来源 ID 很稀疏时，表空间仍会随 ID 区间跨度增长，详见 [存储约定](../spec.md#records)。

CPU 191，统一 `-O3 -DNDEBUG -std=gnu++20`、无 LTO；两组 4096 次循环程序，完整轨迹先对照独立解释器和基线，再各预热一次、七轮串行交替计时。计时区间仍只有固定次数的 step/tick，排除构造、装载和观察。以下为同批中位数，原始样本、四分位数、源码和二进制指纹见 [slot-results.json](slot-results.json)。

| 程序 | 二分查找 ns/tick | 直接索引 ns/tick | 耗时降低 | 加速 | 构造 ms 旧→新 | 峰值 RSS KiB 旧→新 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| window | 3890.1 | 3693.3 | 5.06% | 1.053× | 78.1 → 84.2 | 37924 → 37932 |
| branches | 3024.2 | 2868.1 | 5.16% | 1.054× | 77.7 → 82.5 | 37936 → 37908 |

所有样本的 Module/Rule Work、仲裁次数、delta 轮数、Queue 检查、接受和事件计数与基线完全相同。该优化带来约 5.1% 的每拍耗时下降，构造增加约 4.8–6.1 ms，整进程峰值 RSS 基本不变。这是槽位定位单项的收益；新版本与同批原生 skyzh 的完整程序计时仍分别相差约 17.1×、16.7×。

Release 和 ASan/UBSan/LeakSanitizer 各 14/14 CTest 通过。补充测试覆盖 RuleId 空隙与越界、相同 Rule 在不同 Queue 的不同槽位、重复声明、共享映射和无来源 Queue。完整 OoO 每构建 60 次运行通过，全部 10 组程序/写回配置的逐拍摘要与修改前一致。日志分别为 `/tmp/gfsim-slots-ctest.log`、`/tmp/gfsim-slots-asan-ctest.log`。

```bash
LD_LIBRARY_PATH=/home/lc/opt/gcc14/lib python3 -m pycircuit.examples.skyzh_ooo.benchmark \
  --generated /tmp/gfsim-slots-build/examples/skyzh_ooo/acpy-skyzh-compiled \
  --baseline /tmp/gfsim-static-build/examples/skyzh_ooo/acpy-skyzh-compiled \
  --reference /tmp/skyzh-mlir-reference-final/skyzh-reference \
  --output /tmp/gfsim-slots-skyzh-benchmark --cpu 191 --repeats 7
```

## 槽位优化之后的瓶颈实测（2026-10-04）

本轮只分析，不修改生产版 runtime、编译器和 CPU。实验源码及二进制在 `/tmp/gfsim-slots-analysis`，基线仍为上节的直接索引版本。完整样本、源码指纹、诊断补丁、构建参数、实验脚本及逐拍检查摘要保存在 [slot-analysis-results.json](slot-analysis-results.json)。结论是：当前主要成本来自生成代码与 GFSim 之间的通用事务处理，包括临时对象、proposal 登记、仲裁及 Xfer；静态通知过宽也增加 Work。单个槽位查询或统计判断不能解释剩余差距。

### 先区分每拍成本和模拟周期数

CPU 190 上用 perf FIFO 在固定 step/tick 循环前后启停计数，排除构造、装载与最终观察。每项三次，以下为硬件计数中位数推导。这里的“宿主 IPC”是机器指令/机器时钟，与模拟 CPU 的架构 IPC 不同。

| 程序 | Queue 宿主指令/模拟拍 | 原生宿主指令/模拟拍 | 指令数倍率 | Queue / 原生宿主 IPC | Queue / 原生模拟周期 |
| --- | ---: | ---: | ---: | ---: | ---: |
| window | 16000 | 1775 | 9.02× | 1.90 / 2.29 | 61486 / 40991 |
| branches | 12848 | 1782 | 7.21× | 1.98 / 2.19 | 24662 / 12328 |

每拍差距主要体现为执行了更多宿主指令，宿主执行效率的差距较小；不能据此排除内存访问和分支预测成本。完整程序还额外乘上 1.50× / 2.00× 的模拟周期数。原生 skyzh 也扫描保留站并维护前后状态，但其阶段直接调用、更新专用数据结构；Queue 模型分成 Issue、执行、写回、唤醒等阶段，每次状态效果都经过通用 Rule 事务。两者的流水和发射行为不同，完整程序倍率不能全部归因调度器。

### 通过替换实现测量因果，而不只看热点符号

继续使用 CPU 191、`-O3 -DNDEBUG -std=gnu++20`、一次进程预热、七次串行轮换及固定 tick 区间。每个百分比与其同批基线比较，不把不同实验收益相加。除 LTO 项以外编译选项相同。

| 单独改变的实现 | window 耗时降低 | branches 耗时降低 |
| --- | ---: | ---: |
| 固定 Queue 端口列表改为内联存储，消除每次构造的 `shared_ptr<vector>` | 12.5% | 14.0% |
| pop 去重临时列表改为内联存储 | 4.3% | 4.2% |
| revise 捕获值内联存储，消除大 `std::function` 的堆分配 | 11.1% | 12.1% |
| Fetch 借用不可变 words，不复制整个 vector | −0.4% | 2.5% |
| 以上临时容器与回调优化合并 | 29.0% | 31.8% |
| 仅开启 ThinLTO（同时使用 lld） | 11.0% | 9.8% |
| 仅移除统计计数更新 | 2.1% | 1.2% |
| 仅移除 Rule 执行上下文检查 | 1.8% | 1.5% |
| Queue 内部直接读 count，减少重复/虚函数 size 查询 | 3.4% | 3.0% |
| 强制内联 `slotIndex` | 0.3% | 0.9% |

生成代码的固定列表来源见 [QueueRefs](../../pycircuit/support.hpp)，pop 去重使用同文件的 `Pops`；revise 存储见 [Queue::Slot](include/gfsim/queue.hpp)。这是对象表示及生成方式的问题，不是 C++ 数学运算缺少优化。作为另一条验证路径，仅将完整 payload revise 改为带类型的 replacement 槽位，保持部分更新顺序，耗时也降低 7.0% / 8.3%。

单独拦截计时区间的 C++ new/delete 后，window 的 61486 拍发生 **979777 次分配（15.94 次/拍）**，branches 的 24662 拍发生 **328895 次（13.34 次/拍）**。window 中，固定列表、pop 去重、words 复制、大 revise 回调分别贡献 409874、229617、41028、299178 次；合并实验剩 80 次。branches 合并后剩 65 次。残留主要是可复用容器的初次增长。分配统计二进制没有参与速度对比，避免计数本身影响结论。

这些替换几乎消除了本模型稳定运行中的堆分配，但耗时只下降约三成，说明它们是最大的已验证可消除成本组，仍不能解释整个差距。固定列表/pop 实验上限为 16，回调内联区为 128 字节且要求捕获可平凡复制/销毁；这是诊断用实现，不是可直接推广到所有 ACPy 模型的定稿。分配、布局、拷贝和内联机会同时改变，因此百分比也不是 malloc/free 独占时间。

### 静态依赖过宽确实会放大 Work

Commit 每次提交修改整个 `CPUControl` Queue，至少更新 `head`；只读取 epoch/stopped/target 的模块也会收到通知。临时模型通过已有的纯 Signal 投影这三个字段，保持 Dispatch、load Issue、Commit 对原始 control 的依赖。没有加入动态依赖登记或候选缓存。所有检查的逐拍状态、提交结果和周期数保持一致，接受 Rule 总数不变。

| 程序 | Module Work 原版→投影 | Rule Work 原版→投影 | 投影单项耗时降低 | 原版 ns/tick | 消除分配并投影 ns/tick | 合计降低 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| window | 729570 → 553437 | 594354 → 450979 | 7.4% | 3691.6 | 2459.5 | 33.4% |
| branches | 262914 → 189183 | 193203 → 131761 | 13.3% | 2895.3 | 1771.3 | 38.8% |

Module Work 减少 24.1% / 28.0%，耗时下降较少，说明被省掉的空输入检查等 Work 相对便宜。这个实验只验证 control 粒度，不能外推为所有静态过度激活的占比。合并实验的完整程序仍比同批原生慢约 **11.3× / 10.3×**。

### 剩余成本贯穿 proposal、仲裁和提交

单独计数的 window 每拍有约 11.87 次 Module Work、9.67 次 Rule Work、53.14 次槽位定位、108.39 次 size 查询、19.13 次 canAccept、12.87 次 accept、11.00 次 Queue Xfer。一次效果在 [prepare](src/simulator.cpp)、[arbitrate](src/simulator.cpp) 和 [Queue Xfer](include/gfsim/queue.hpp) 中被多次访问，伴随参与者列表维护、虚函数调用、状态检查和回调。槽位已经固定，但通用执行路径仍有这些成本。

独立的阶段计时实验只插入每拍四次时钟读取：window 的 Work/仲裁/Xfer 与通知约占 **57.4% / 23.4% / 19.2%**，branches 为 **60.2% / 20.7% / 19.1%**。Work 包含 proposal 构造、临时对象和 Rule 生命周期，不能等同于 CPU 纯组合计算。该计时有插桩开销，只用于判断阶段分布，不用于报告加速比。

当前生产二进制的三轮 perf 自身样本归组为：scheduler 约 39–40%，Queue 相关符号约 25–27%，生成模型及其内联代码约 19%，libc 约 13–15%。模板符号和内联会混合归属，因此不是精确的引擎/模型成本划分。尤其 `slotIndex` 自身样本可达 6–7%，但强制内联实际只有约 0–1% 收益；不能直接把热点百分比当作可获得的加速。

下一步应优先把固定端口表、输入去重和带类型的 revise 数据表达为静态/内联存储，再减少同一 proposal 在多个阶段的重复查询、间接访问和遍历。control 投影可独立保留为模型层改进。仅关掉统计或合法性检查收益很小；已有候选缓存、动态读依赖登记已经删除，这两个程序的事件入堆计数也都是零，不能再用它们解释当前剩余成本。

### 验证与重现

每个速度实验先跑五个程序 × 两种写回配置 × 两种 Module 执行顺序，完整输出逐字对照生产版，并以独立 RV32I 解释器核对；长程序的每个计时样本检查寄存器、内存、停止状态、指令数和周期数。原模型的优化实验还保持全部调度计数一致；control 投影按预期改变 Work/部分仲裁计数，但接受总数一致。合并临时容器/revise 实验另经 ASan/UBSan/LeakSanitizer，20 条短程序配置的逐拍输出及两个长程序均通过。此处验证限于所测模型，未把诊断代码合入生产实现。

本机重跑命令如下；JSON 的 `evidence_scripts` 保存脚本文本，`variants` 保存相对基线的补丁和编译命令，`projected_acpy_patch` 保存 Signal 实验的前端修改。重建投影版需先用同一 MLIR 工具生成模型；其合并版补丁相对投影版，其余 C++ 补丁相对 base。JSON 也保存源文件/二进制指纹和所有样本，避免依赖临时文件才能审阅结论。

```bash
PYTHONPATH=/home/lc/tmp python3 /tmp/gfsim-slots-analysis/bench.py \
  original base containers inline_revise no_stats no_validation direct_counts unity lto combined reference
PYTHONPATH=/home/lc/tmp python3 /tmp/gfsim-slots-analysis/bench-more.py \
  original base fixed_lists fixed_pops borrow_words whole_revise whole_combined slot_inline combined combined_lto reference
PYTHONPATH=/home/lc/tmp python3 /tmp/gfsim-slots-analysis/bench-activation.py \
  original base projected combined projected_combined reference
PYTHONPATH=/home/lc/tmp python3 /tmp/gfsim-slots-analysis/profile/measure.py
python3 /tmp/gfsim-slots-analysis/run_counts.py
python3 /tmp/gfsim-slots-analysis/check_asan.py
```
