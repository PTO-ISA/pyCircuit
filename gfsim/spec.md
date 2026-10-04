# GFSim 静态调度契约

本规格对应 `gfsim/cpp` 和 ACPy MLIR 编译器。Python `gfsim/experiment` 保留为历史参考实现，仍含动态订阅、缓存及 DFS，不定义当前 C++ 接口。当前核心只有一条执行路径；不提供缓存开关。

<a id="model"></a>

## 1. 对象与执行边界

- Module 是静态实例，持有控制流入口 `Work()`；不同 Module 独立激活。
- Rule 是 Module 的成员函数，同一实例内共享一个 RuleId。每条 Rule 实际提出的 Queue proposal 与未来事件组成原子事务。
- Queue 持有持久 current 与各静态来源的 proposal。寄存器可用容量 1、已有初值、只 revise 的 Queue 表示。
- Signal 保存纯 helper 的当前组合值；输入可以是 Queue 或 Signal，配置在构造期固定，不参与 proposal。

一个 tick 按以下屏障执行：

```text
到期显式事件 + 上拍资源变化 → 激活 Module
  → 全部 Module Work 完成
  → Rule 原子仲裁；获准 pop 沿固定容量边推进下一 delta
  → 全部获准 Queue Xfer
  → 受影响 Signal 按静态拓扑序去重求值
  → 下一 tick
```

所有 Work 看到同一份冻结的 current。仲裁中的获准 pop 只提供容量资格，不修改 current；新 push 不能在本 tick 为其他 Rule 提供 payload。delta 只重新仲裁，不执行 Work 或 Signal helper。

<a id="construction"></a>

## 2. 静态连接

构造阶段注册 Module、Rule、Queue、Signal，再声明连接，最后 `freeze()`：

| 接口 | 连接 |
| --- | --- |
| `declareResource(module, resource)` | Queue/Signal 状态变化 → 下一 tick 激活 Module |
| `declareInput(signal, queue_or_signal)` | 输入变化 → 全部 Queue Xfer 后按拓扑序重新求值 Signal |
| `bind(rule, queue, operations)` | 固定 proposal 来源和操作；同时建立 Queue → Rule 所属 Module 的连接 |

声明覆盖所有可能分支、只读引用、消息输入、固定输出及动态索引的全部可能元素。同 Queue 别名共享身份。Queue 查询和只读观察同样需要静态声明；pure push 的目标也有 Module 连接。资源数组每项有独立身份，运行时下标只决定实际读取与实际 proposal，不能缩小静态连接。

每个 Queue 的 pop 来源至多一个 Rule，push 来源至多一个 Rule，覆盖所有分支和 tick；二者可为同一 Rule。这是模型前提，当前不做冲突检测或多来源端口仲裁。不同 Rule 的重叠 revise 也不属于已定义的冲突语义。

构造时追加连接，`freeze()` 对每张邻接表排序去重，建立 Queue 来源槽位和固定容量任务缓冲。没有 QueueCount×ModuleCount 映射、Module 资源矩阵、Rule 读者位图或 dirty 位图。来源 0 保留，尚无外部驱动协议。

`freeze()` 对 Signal→Signal 子图建立固定拓扑序；组合环报 `Signal dependency cycle`，不进入执行期。Queue→Signal 边不增加此子图的入度：所有 Queue 已在求值前完成 Xfer。Queue 反馈经过持久状态形成时序边界，可以存在；Rule 容量环仍沿用原有 pending 语义。

Queue 来源槽位按 RuleId 排序，freeze 后不再增删。冻结时建立 RuleId→槽位的只读直接索引，同一来源集合的 Queue 共享一张表。来源 0 单独对应保留槽位；其他索引覆盖最小到最大来源 ID 的区间，空隙标记为无连接。proposal、仲裁、接受及撤销使用同一映射，执行阶段没有来源搜索。

静态容量边由 `Queue.popRule → Queue.pushRule` 表示。无需预先拓扑排序，不检测环，不构造 SCC。实际没有任何 Rule 能获准的环可以保持 pending；宿主以周期上限等条件报告不前进，编译器不保障活性。

<a id="work"></a>
<a id="prepare"></a>

## 3. Work 与 proposal 生命周期

tick 0 初始化全部 Signal，然后激活全部 Module。以后只执行激活集合中的 Module，每 tick 至多一次。

Module 开始 Work 时，清除其全部旧 Rule proposal 和未发布事件。控制流重新选择 Rule，并计算本次普通参数。未选择的 Rule 自然不留下旧效果。没有激活的 Module 保留完整 pending proposal，可以跨 tick 等待容量。

`beginRule(rid)` 只检查执行上下文和同 tick 去重，不比较参数、不命中缓存。同一 RuleId 同 tick 多次调用须有一致参数和连接，由模型保证；只执行第一次。普通参数就是普通函数参数，组合值不成为运行时资源记录。

Rule 正常出口调用 `completeRule`。仅完整且有实际效果的 Rule 进入仲裁：实际效果指非空 participants 或非空事件请求。完整但无效果不会 firing，不更新 acceptedTick，也不生成仲裁任务。不能以普通 return 代替 complete/abort。

必要读取失败调用 `abortRule`，撤销该次尝试的全部 pop、push、revise 和未发布事件。Module 控制读取失败只结束当前 Work，此前完成的其他 Rule 保持有效。Rule 不能嵌套执行。

| 情况 | proposal |
| --- | --- |
| Module 未激活 | 完整 pending 原样保留 |
| Module 开始 Work | 清全部旧 proposal；选中 Rule 重新执行 |
| Rule abort | 清该 Rule 部分效果和未发布事件 |
| 仲裁失败 | 完整 pending 保留；不发布事件 |
| 整体获准 | 每 tick 至多一次；事件发布，Queue 等待 Xfer |
| Xfer 后 | 清已获准 Rule 的 proposal 与事件请求 |

静态连接不随以上生命周期改变。没有实际读取登记、动态订阅、参数存储、候选有效性缓存或 dirty 判断。

## 4. 读取与 Signal

Queue 的 `peek/tryPeek/at/size/empty/full` 直接读取 current，不访问调度器、不登记依赖。`tryPeek` 为空返回空指针；必要 `peek` 为空抛 `NeedInput`。生成代码优先在实际分支内显式检查并 abort/return，正常缺输入无需 C++ 异常。pop/revise 本身也检查旧目标是否存在。

ACPy 消息输入实际读取 payload 才提出一次 pop；同一路径多个参数别名按 Queue 身份去重。内部状态、输出观察、Module 观察、Signal helper 和 Queue 查询不自动消费。未执行分支不会产生读取、消费或其他 proposal。

ACIR `read/query` 本轮继续保留保守的 `MemRead + MemWrite`，不标记 Pure 或可推测执行，以维持分支和缺输入语义。它不是运行时依赖登记的描述；优化精化不在本次重构范围。

Signal helper 只读全部静态声明范围内的 Queue、上游 Signal 与固定配置，不读取 tick 或可变宿主状态，不提出 proposal 和事件。编译器保证输入完整性与纯性；手写 C++ 须完整声明依赖。runtime 不逐次查找读声明，仍拒绝 helper 内的 proposal/事件操作。

tick 0 在 Work 前按拓扑序求全部 Signal 的初值。任一声明输入 Queue 改变时，将 Signal 去重加入求值集合；所有 Queue Xfer 完成后扫描固定拓扑序，只计算集合内的 Signal。输出值改变才将下游 Signal 加入同一轮求值集合，并激活静态关联 Module。下游必在拓扑序的后面，每轮最多求值一次；菱形汇合看到全部上游的最终值。初始化与当拍 Xfer 后更新是两个阶段，tick 0 可以各求值一次。

Signal 链不增加流水拍或 delta。Signal 下游没有 Rule 掩码，即使上次执行未读该 Signal，连接也始终有效。没有变化的输出不额外激活 Module 或下游 Signal；下游若另有其他变化的输入，仍会计算。

<a id="arbitration"></a>

## 5. 原子仲裁与 delta

只有完整、有效果、当前 tick 尚未获准的 pending Rule 可仲裁。先检查全部实际 participants，全部通过后才 accept 任一 Queue；事件到期时间溢出也在 accept 之前检查。

| 操作 | 许可 |
| --- | --- |
| pop | current 有旧队首 |
| revise | current 有旧队尾 |
| push | current 未满，或本 Queue 已有整体获准 pop，或本 Rule 同时合法 pop |

检查同 Rule pop/push 时，可以使用其自身合法 pop 的空间。其他 Rule 的 pop 意愿不能提供容量，必须整条消费者已获准。缺少其他输入或受其他输出反压的消费者不能提前释放空间。

仲裁使用两个去重 Rule 任务列表：

1. Work 的 `completeRule` 将新完整、有实际效果的候选加入当前列表。
2. 依次原子检查，失败的 proposal 原样保留。
3. Rule 获准后，对其**实际获准 pop** 的每个 Queue，将固定 push 来源的已有完整 pending Rule 加入下一 delta。
4. 当前列表处理完，清除其去重标记，交换两个列表，继续直到无任务。

同一个 Rule 在不同 delta 可再次尝试，但每 tick 至多获准一次。消费者可以早于生产者在当前列表获准；生产者检查时可使用该已获准容量。多输出或菱形依赖可以在后续 delta 重试，不使用跨 delta 的全局 visited。只有实际获准 pop 能添加后续任务，因此无进展的环不会自旋。

不扫描所有保留 pending，不递归拉取消费者，不设置 waitingQueue、DFS 栈或访问状态。容量释放引起的重试不重新运行生产者 Work。

<a id="xfer"></a>

## 6. Xfer 与静态激活

全部仲裁完成后，获准使用的 Queue 各 Xfer 一次：

1. 按 proposal 顺序 revise 旧队尾。
2. pop 旧队首。
3. push 新队尾。
4. 清已接受来源槽位。

固定容量环形存储、按来源绑定分配槽位以及 Queue 的 revise 适配保持不变。字段修改和动态下标修改捕获索引与新值，在 Xfer 操作旧队尾；不得捕获 Work 局部引用或在提交时重新读计算输入。

no-op revise 不改变版本、不通知。pop 后 push 相同 payload 仍是元素变化。current 引用/指针不能跨 Xfer 保存。

Queue 变化沿静态邻接表将关联 Module 加入下一 tick 激活集合，并标记下游 Signal。Signal 输出变化沿同样的静态邻接表通知 Module 与 Signal。每个 Module 用一个去重标志；同拍多个资源变化和到期显式事件只产生一次 Work。普通资源通知从不进入事件堆。

<a id="events"></a>

## 7. 显式延迟事件

Rule 内 `requestWakeup(rid, target, delay)` 只保存请求，`delay ≥ 1`。整条 Rule 获准时，才向 event queue 发布 `(acceptanceTick + delay, ModuleId)`。等待容量的 proposal 延迟从实际获准 tick 起算，不能从提出请求的 tick 起算。

纯事件 Rule 可获准。不完整、abort 或被 Module 重算替换的候选丢弃未发布请求。已经发布的事件不因候选清理失效，不支持取消/覆盖；重复显式事件可存在堆中，但到期后的 Module 激活去重。事件本身激活完整 Work，不存在参数缓存命中。

<a id="records"></a>

## 8. 记录、成本与失败边界

Module 保存实例指针、Work 入口、所属 RuleId 与观测计数。Rule 保存 owner、实际参与 QueueId、未发布事件、完成状态、selectedTick/acceptedTick 与观测计数。Queue 保存固定 pop/push 来源及 Queue→Module、Queue→Signal 邻接表；Signal 保存 Signal→Module、Signal→Signal 邻接表。Simulator 另保存 Signal 拓扑序；有待重算 Signal 的 tick 扫描此序列，无运行时建图或递归求值。

调度器保存 Module/Signal 激活集合、两个 delta Rule 列表、已使用 Queue 和已获准 Rule 列表，以及显式未来事件堆。固定任务集合在 freeze 时预留空间；proposal/事件使用可复用 vector。邻接和槽位空间随实际声明连接与来源数增长，没有资源×全部 Module 的稠密映射。共享来源索引表的空间另取决于每种不同来源集合的 ID 区间跨度；不为每个 Queue 分配覆盖全部 Rule 的表。

`step()` 返回的获准 Rule span 在下次 step 前有效。`module/rule/stats/events`、Queue 版本、Signal 求值次数供 testbench 观察。统计区分 Work、仲裁尝试、delta 轮次、Queue 检查和显式事件；这些计数不参与调度决策。

tick、状态版本、事件时间和观测计数使用 uint64，溢出报错而不回绕。Work、helper、仲裁、Xfer 异常使实例 failed，后续 step 被拒绝；不承诺异常时回滚。容量环本身不触发专门异常。

Queue/Signal 地址必须稳定且存活到 Simulator 析构完成。冻结后不得新增连接、移动实例或重新注册。Module 在执行期必须有效；同实例不支持并行或嵌套 step。未定义重启、快照恢复、独立 Cell 或外部来源 0。待决项见 [open-questions](open-questions.md)，可复现验证见 [C++ 说明](cpp/README.md)。
