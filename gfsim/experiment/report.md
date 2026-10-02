# 动态 Rule 依赖与 dirty 位图实验报告

本文首先保留第一轮 dirty 改造的历史基线与结果（包括当时无效果结果不复用的策略）。随后固定槽位映射和自动读取登记的 37 项测试及分步计时见 [读取登记报告](read-tracking.md)。最新 begin_rule／wakeup 改动、39 项测试和计时见[本次更新](#begin-rule-wakeup)，原始数据分别保存。

Python 已实现 Module 管理动态 Rule 读者位图、Queue 变化精确标记 dirty、普通 var 参数变化标记同一 dirty 位。候选缓存命中不扫描 Queue 版本、不续订依赖。riscv 展开为普通阶段类和显式引擎调用，微架构保持不变。

本轮功能验证通过；性能未提升。36 配置的中位耗时比旧实现整体高约 17.7%，详见下文。C++ runtime、Module 间 var 传播及原生 Ripes 对照不在此次验收范围。

## 验证

```bash
PYTHONPATH=gfsim/experiment python3 -m unittest test_engine examples.riscv.test_model review.test_review -v
python3 gfsim/experiment/bench.py --compare gfsim/experiment/review-output/dirty/before-traces.json
PYTHONPATH=gfsim/experiment python3 -m examples.riscv.run gfsim/experiment/examples/riscv/programs/sum.s --latency 5 --html
```

29 个测试方法通过：11 个引擎小电路测试、15 个 CPU 测试、3 个观察器测试。CPU 保留原有全部 14 个方法（含 64 种依赖窗口、8 个随机种子），新增完整快照矩阵。观察器开关下状态、事件和统计一致；失败容量环保留现场，之后不能继续执行。sum.s 的 5 拍访存 CLI 运行到 HALT，退休 98 条、232 周期，sum=55，已导出可视化。

| 场景 | 验证内容 |
| --- | --- |
| 一个 Module 的 70 条 Rule | 精确 dirty；跨 64 位不串位；重复读取／别名只登记一次 |
| Module 控制读取与 var | 控制变化唤醒，派生参数相同命中缓存，参数变化重算 |
| 缓存命中 | 测试将读取列表迭代设为失败，命中仍成功；保留边能接收后续 Queue 变化 |
| 动态数组与条件路径 | 下标切换后清旧读取；旧表项变化不再误唤醒／误标脏 |
| 必要输入为空 | 部分输出清理，读空关系保留，新输入下一拍唤醒 |
| 提交与无效果 complete | 提交后仍有读取订阅；无效果不 firing、不作为缓存候选 |
| 未选中与提前返回 | 取消未再选中的候选、读者位和 dirty；Module 读空不取消此前独立 Rule |
| 容量通知 | 保留候选直接提交，Module／Rule Work 次数不增加 |
| 元素身份与脏候选 | 相同值 revise 不通知；相同 payload pop/push 通知；dirty 候选不能仲裁 |
| 资源声明 | 未声明读取／修改拒绝，默认兼容旧调用方式 |

## 改造前后逐拍对照

旧版基线为提交 `90ebc8dd56966c099688bb281c7321c5cfac1eb0`，在修改引擎前采集。三个程序在 [bench.py](bench.py) 定义：连续 ALU 依赖链、数组求和、load/store/分支/JAL/JALR 混合程序。延迟 1/3/5 × 缓存开关 × Module 正反序，共 36 配置。

比较初始状态及每拍全部 Queue 内容、获准 RuleId 集合和未来事件，36 组完全一致，没有周期移动。另在每次退休与独立指令解释器核对退休记录和全部寄存器，HALT 时检查全部数据存储。矩阵测试对 Module 反序的事件 ID 映射回固定阶段编号；改造前后的同配置对比使用原始 ID。

| 程序 | 延迟 1 周期数 | 延迟 3 周期数 | 延迟 5 周期数 |
| --- | ---: | ---: | ---: |
| ALU | 54 | 54 | 54 |
| sum | 148 | 190 | 232 |
| control | 22 | 26 | 32 |

本地大轨迹保存于被 Git 忽略的 `review-output/dirty/`；计时原始样本、基线完整提交号、引擎 SHA256 和轨迹哈希保存在 [results.json](results.json)。新机器可从固定提交重现基线，无需依赖此次临时目录：

```bash
baseline_dir=$(mktemp -d /tmp/gfsim-before.XXXXXX)
git archive 90ebc8dd56966c099688bb281c7321c5cfac1eb0 gfsim/experiment | tar -x -C "$baseline_dir"
cp gfsim/experiment/bench.py "$baseline_dir/gfsim/experiment/bench.py"
python3 "$baseline_dir/gfsim/experiment/bench.py" --output "$baseline_dir/results"
python3 gfsim/experiment/bench.py --compare "$baseline_dir/results/traces.json"
```

这里将同一个测量程序放到旧 checkout 中，加载旧 engine、构造代码和 CPU。当前 bench 的功能捕获与计时循环与修改前保存的测量程序保持相同边界。对比失败会报告首个快照和字段，原始双方轨迹保留用于调查。

## Python 耗时

环境为 aarch64、Python 3.11.16。每配置独立构造，5 次运行取中位数；计时只包含从首拍到 HALT 的 `sim.step()` 循环和 HALT 检查。进程启动、导入、构造、解释器检查、快照采集、JSON、HTML 均不计时；内置统计计数开启、观察器关闭。旧／新基线分别采集，不宣称消除系统负载与运行时噪声；测量比较包含引擎与生成式阶段改写的总影响，不单独归因到某个函数。

下表取访存延迟 5、缓存开启、Module 正序；全部 36 配置的原始样本见 results.json。

| 程序 | 旧版中位数 | 新版中位数 | 新／旧 |
| --- | ---: | ---: | ---: |
| ALU | 9.457 ms | 11.077 ms | 1.171 |
| sum | 25.304 ms | 29.783 ms | 1.177 |
| control | 3.257 ms | 3.834 ms | 1.177 |

按每程序的 12 配置分别计算新／旧比值中位数：ALU 1.174、sum 1.182、control 1.176；全部 36 配置中位比值 1.177。改造前后这些程序均无缓存命中，因此它们没有测到“省去缓存版本扫描”的收益。新实现增加位图登记、清旧读取、逐字通知和 dirty 检查；这是额外工作量的观察，未通过 profiler 将总差值逐项分解。

读取位图的语义优势已由有背压的小电路验证，性能优势尚未验证。本轮不新增重计算基准来替代用户指定的 riscv 验收，也不由 Python 时间推断 C++ 性能。

## 当前边界与成本

- 静态可能访问声明影响空间和查找，实际动态读者位影响失效。每 Module A 个资源、R 条 Rule，需要 A×ceil(R/64) 个读者字；大数组与多 Rule 的组合仍可能昂贵。
- Rule 重算／取消需遍历实际 readSlots，Queue 变化扫描可能读者的位图字；缓存命中才是参数比较加固定标志检查。
- 事件堆仍在到期时去重 Module，重复未来事件的空间和处理成本未改。
- 旧 Python examples 及专用参考框架已移除，当前不再宣称运行了旧 1100 级 Python 回归。窄引擎测试有少量只读断言／内部仲裁资格检查；电路状态变化均通过实际 Rule 和 Xfer。
- riscv 不代表完整 ISA 或任意微架构。ripes5 原文件未改，本轮没有重新运行其原生逐拍验收。
- C++ runtime 保留旧机制，只移除了依赖已删除 Python examples 的跨语言测试脚本和 CMake 注册；其历史报告明确标记为历史。本轮未构建或测试 C++。
- Module→Module var、delta Work、编译器自动生成、事件取消、层级激活、外部驱动和恢复仍未完成，见 [待决问题](../open-questions.md)。


<a id="begin-rule-wakeup"></a>

## begin_rule 与 Queue 变化 wakeup 更新

普通 var 保留为 args，不增加资源 slot。begin_rule 先读 dirty；已 dirty 时跳过参数比较，否则参数变化标记同一 dirty 位。缓存条件为 `cacheEnabled && complete && !dirty`，完整无效果结果也可复用；仲裁仍检查实际效果，不产生空 firing。选择登记、同 tick 去重、重算清旧读取及执行上下文继续保留。

Queue Xfer 后沿固定链接调用 `_wakeup(mid, tick + 1, changed_slot=slot)`。入口当下合并该资源的实际 Rule 读者位，并检查 Module 控制订阅；有实际读取才安排激活。普通定时事件不带槽位，只激活、不标脏。多个变化资源都先处理失效，到期 Module Work 再去重。

正式工作区的 39 项测试通过：原有 37 项，加上无效果缓存随后被 Queue／参数变化失效、定时事件保持无效果结果 clean 两项。旧测试改名为无效果结果在 Queue 改变后重算、不 firing，避免名称继续暗示一律不可缓存。观察器的缓存原因同步更新。

测速基线是本轮应用前的未提交 dirty 引擎快照；不是上一轮报告中的旧提交。三个临时版本为 before、仅 begin_rule 改动、begin_rule 加 wakeup 封装。正式引擎和观察器与实测 combined 版本逐字节一致。三程序 × 延迟 1/3/5 × 缓存开关 × Module 正反序共 36 配置，每拍 Queue、获准集合、事件完全相同，全部统计也一致；三个版本规范化轨迹 SHA256 相同。

环境为 aarch64、Python 3.11.16，三个串行 worker 固定到同一允许的 CPU（编号 0）。每配置每版本预热一次，随后交替顺序测量 7 组，每组独立构造 3 个实例，取每次运行平均耗时，再取 7 组中位数。只计 sim.step 循环与 HALT 检查；构造、进程启动、参考解释器、轨迹和 JSON 在计时外。保留相同内置计数，关闭 observer。

| 版本 | 36 配置新／旧耗时比值的中位数 | 相对 before 耗时变化 |
| --- | ---: | ---: |
| before | 1.0000 | — |
| 仅 begin_rule | 0.9678 | 降低约 3.2% |
| 加 wakeup 封装 | 0.9639 | 降低约 3.6% |

以下是延迟 5、缓存开启、Module 正序的运行中位数，单位 ms：

| 程序 | before | 仅 begin_rule | combined |
| --- | ---: | ---: | ---: |
| ALU | 11.104 | 10.783 | 10.741 |
| sum | 29.789 | 28.758 | 28.774 |
| control | 3.850 | 3.711 | 3.713 |

wakeup 封装的额外差值较小，不能排除计时噪声。这些 CPU 配置全部为零缓存命中，测到的是入口和通知成本，未测得无效果缓存的性能收益。begin_rule 仍为 32 行；逻辑优化未显著减少代码行数。此结果不代表 C++ 性能，也没有重新运行原生 Ripes 对照。

[begin-rule-results.json](begin-rule-results.json) 保存原始样本、统计、轨迹哈希、源文件哈希、两个版本补丁和完整测量脚本。复现时从本提交复制 experiment 为 before/begin/combined 三份；before 反向应用 combined 补丁，begin 在 before 上应用 begin 补丁，combined 保持原样。将 JSON 中的 worker.py、evaluate.py 写到三个目录的同级，再运行 evaluate.py。正式测试新增不影响计时模型；源文件哈希区分本次各个检查点。

<a id="signals"></a>

## Signal 扩展（2026-10-02）

新增纯 helper 派生的只读 Signal，全部 Queue Xfer 后去重求值；Module 控制读取和 Rule 直接读取共用局部资源槽位、wakeup 和 dirty 位图。Queue 仍仅由 Rule 修改。没有增加 Module delta Work，也没有扩展容量仲裁。

核心 engine 净增 71 行、构造净增 26 行（含空行与注释）；原 39 项加新增 8 项测试共 47 项通过。旧 CPU 36 组配置与 `c811811` 逐拍状态、获准集合及事件一致；新流式电路 8 组完整执行与独立时钟模型一致。

无 Signal CPU 的新／旧耗时比中位数为 1.0070；新电路 1200 拍，稀疏／频繁配置变化分别求值 678／2377 次，全量每拍计算的次数为 2402。完整测量方法、限制、原始数据及可视化入口见 [signals 示例报告](examples/signals/README.md)。未移植 C++，未重新验收原生 Ripes。
