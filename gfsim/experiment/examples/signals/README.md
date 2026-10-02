# Signal：共享组合计算与流式电路

本例验证 [spec](../../../spec.md) 的 Signal 扩展。Queue 仍由 Rule 修改；Signal 的纯 helper 观察 Queue current，计算只读组合值。Module 和 Rule 均可读取 Signal，并复用现有读取槽位、唤醒与 dirty 位图。

## 运行与阅读

仓库根目录：

```bash
PYTHONPATH=gfsim/experiment python3 -m examples.signals.run
PYTHONPATH=gfsim/experiment python3 -m unittest examples.signals.test_model -v
PYTHONPATH=gfsim/experiment python3 -m examples.signals.bench
```

`run` 验证全部 120 拍，与独立参考模型比较 Queue 和 Signal 值，输出 16 个事务；可视化写到本例的 [review-output/signals.html](review-output/signals.html)。`--frequent` 改为频繁更新配置，`--output PATH` 指定 HTML。生成的 HTML 被 Git 忽略，可随时重建。

| 文件 | 职责 |
| --- | --- |
| [model.py](model.py) | 普通 Module 类、成员 Rule、纯 helper 和显式连接 |
| [reference.py](reference.py) | 独立时钟模型，直接计算传输与下一状态，不使用引擎订阅或 proposal |
| [test_model.py](test_model.py) | 完整流式运行与用于检查订阅的时钟驱动小电路 |
| [run.py](run.py) | 运行、逐拍对照、通用 review 导出 |
| [bench.py](bench.py) | 仅计时仿真循环，独立参考核对放在计时区间外 |

## 电路与接口

```text
Source0 → bank0 ─┐
                 ├→ Processor → output → Sink
Source1 → bank1 ─┘       ↑
                    enabled / selected Signals
                         ↑ 观察 Queue current
                    config、bank0、bank1
                         ↑
                     Controller
```

两个 Source 从不可变输入序列产生事务。Controller 的 Rule 按不可变配置程序修改寄存器 Queue，切换 bank、暂停处理、改变偏置。Sink 用获准 Rule 的定时事件表达延迟消费，形成真实反压。持久位置和退休结果均由 Queue 保存，测试不直接写状态或注入唤醒。

`enabled` 由 Processor 的 Module 控制流读取，决定是否选择传输 Rule；`selected` 由传输 Rule 直接读取，返回有效位、bank、序号与加上偏置后的数据。helper 的观察不消费消息，传输 Rule 仍须对实际 bank 提出 pop。

```python
helpers = Combinational(config, banks)
enabled = Signal(helpers.enabled)
selected = Signal(helpers.selected)

# Module.Work 内：控制订阅
if enabled.value:
    self.transfer()

# Rule body 内：Rule 订阅，Signal 变化直接标记本 Rule dirty
value = selected.value
```

构造器显式接收 `signals`、`module_signals[mid]` 和 `signal_queues[sid]`。后两张表表示可能访问范围，运行期只登记实际读取。Signal 之间暂不允许读取；不实现前端装饰器。

每拍先 Work、仲裁、完成全部 Queue Xfer，再去重计算受影响的 Signal。Signal helper 的 Queue 读取以独立 readGen 更新；即使结果相同也替换输入依赖。结果变化才通知 Module／Rule；下一 tick 读取的是一致的 Queue 和派生 Signal。首次 Work 前先求全部初值。

## 验证结果

2026-10-02，Python 3.11.16 / aarch64：

- 原 39 项测试及新增 8 项测试共 **47 项通过**。
- 完整流式电路：稀疏／频繁控制变化 × 缓存开关 × Module 正反序，共 8 组；逐拍 Queue、Signal 与独立参考一致，获准集合与缓存／顺序变化无关。
- 反压中的旧 proposal 因偏置 Signal 变化重新计算，暂停后取消，恢复时使用新 bank 的值；不能因下游 pop 批准旧数据。
- 多个输入同拍改变只求值一次，helper 看到全部 Xfer 后的状态。
- 输入从 a 切换到等值 b 时不唤醒下游，但读取集合仍替换；之后 a 变化不再触发 helper。
- 70 条 Rule 跨两个字，只有实际读取 Signal 的位变 dirty；同时控制读取通知不会误标其余 Rule。同拍多通知只 Work 一次。
- 读空后新输入会重算；定时唤醒可复用完整无效果结果，不产生空 firing。
- 非法 helper 输入、跨 Signal 读取、未初始化访问及异常终止均验证。
- 诊断开关下状态、事件、计数一致。生成的 120 帧在 Node 的 DOM 桩中逐帧执行渲染代码无异常；这不等同于浏览器外观验收。
- 原 CPU 的 36 组配置与 `c811811` 基线逐拍 Queue、获准集合、事件全部一致；CPU 模型没有修改。

当前没有在上述契约和场景中发现新的调度正确性缺陷。几个实现边界已显式保留：helper 的纯性依靠生成代码契约，Python 不阻止任意私有字段／全局变量的违规访问；空输入必须显式表达有效位；Signal 只比较派生值，不代替 Queue 消息身份和消费；Signal 间依赖、Module delta Work、C++ 移植未实现。

## 代码量与性能

相对 `c811811`，核心 [engine.py](../../engine.py) 从 549 行到 **620 行，净增 71 行**；构造从 57 行到 83 行，净增 26 行，均含空行和注释。`begin_rule` 与容量 DFS 无新增判断；`wakeup` 仅把诊断资源标识改为通用 ID。新增成本集中在读取上下文、Signal 输入代号与 Xfer 后求值。

### 无 Signal 的旧 CPU

原始数据、源码哈希、测量脚本保存在 [engine-results.json](engine-results.json)。基线为 `c811811` 的 engine/construction/model，候选为本轮版本；36 组配置。两个常驻进程绑定同一 CPU 串行执行，一次预热、7 轮交替采样，每轮均值取 3 次新建 CPU 的执行；构造、GC 预清理、诊断和 JSON 不计时，仅 `step()` 与 HALT 检查计时。

| 程序 | 新版／基线耗时比的中位数 |
| --- | ---: |
| ALU | 1.0052 |
| sum | 1.0097 |
| control | 1.0075 |
| 全部 36 组 | **1.0070** |

这次测量约增加 0.7%，接近小幅计时波动，不据此宣称零开销或稳定回归百分比。JSON 中包含 coordinator 与 worker 源码；重现时将 coordinator 的基线／工作树路径指向两个实验目录即可。逐拍轨迹对照也可使用现有 `bench.py --compare`。

### 使用 Signal 的电路

[results.json](results.json) 保存本例实测；运行上面的 `examples.signals.bench` 可重现。1200 拍、双输入共 300 个事务持续制造反压；1 次预热、7 次采样，仅计时 `sim.step()`，观察器关闭。两种负载各自在计时外逐拍核对独立参考。

| 配置变化 | 时间中位数 | 实际 Signal 求值 | 每拍全量求值次数（含初始化） |
| --- | ---: | ---: | ---: |
| 稀疏 | 82.467 ms | 678 | 2402 |
| 频繁 | 137.506 ms | 2377 | 2402 |

2402 是全量求值的次数对照，并非另一个引擎的实测耗时。两种负载的配置写入、Module/Rule Work 和通知数量不同，不能用时间差推导 Signal 缓存的独立加速倍数。实际缓存命中复用由小电路测试覆盖；这两个持续传输负载的 Rule cache_hits 均为 0。

Module 槽位直接索引仍占 `(QueueCount + SignalCount) × ModuleCount` 个整数；Signal 的 Queue 输入记录额外按可能连接数分配，不增加 QueueCount × SignalCount 全量表。这里是 Python 实验结果，不代表 C++ 性能。
