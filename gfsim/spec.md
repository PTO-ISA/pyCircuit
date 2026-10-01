# GFSim 框架规格

本文汇总当前已确认的设计契约，供编译器生成代码、C++ runtime 实现和调度实验共同使用。它不表示 C++ 框架已经实现。未确定的内容集中在 [待决问题](open-questions.md)，不由本文补齐。

核心方案：**Work 更新候选，DFS 解决容量依赖，Xfer 提交状态。读取状态变化或明确事件驱动 Module Work；获准 pop 只通知已有完整候选的唯一生产者仲裁。候选可以跨 tick 保留，只有 Module 再次调用 Rule 时才检查参数及实际依赖版本。**

同 tick 仲裁采用带访问标记的 DFS：按实际容量依赖先处理消费者，再仲裁生产者。不预生成全局拓扑顺序，不因静态关系有环拒绝模型；实际必需的容量依赖形成动态环时才报错。

Queue 支持同 tick 的 pop/push：有合法 pop 就可以复用释放的空间，包括同一 Rule 的 pop/push。本文中的伪代码和 C++ 示例表达职责与生命周期，不规定最终 ABI。

存储方案以 Module 和 Rule 数量远小于 Queue 数量为前提。每个 Queue 的 Module 读者记录采用按 ModuleId 索引的定长 readGen 数组。

每个 Queue 的 pop 来源至多一个 Rule，push 来源至多一个 Rule；二者可以不同，也可以是同一个 Rule。该约束针对所有可能分支和 tick 的 Rule 来源，不只是本 tick 实际调用的数量；选择逻辑写在相应 Rule 的控制流中。当前不加检测，默认用户代码遵守，违反时不保证运行结果。多 revise 等资源端口竞争也不在本版范围。下文调度和原子性契约以没有这类竞争为前提。

## 目录

- [1. 对象与执行模型](#model)
- [2. 编译器与运行前构造](#construction)
- [3. Work 的控制与读取语义](#work)
- [4. 运行记录](#records)
- [5. Module 订阅与 Rule 候选准备](#prepare)
- [6. Queue 资源与 Rule 原子仲裁](#arbitration)
- [7. tick 调度与通知](#schedule)
- [8. Xfer 与候选生命周期](#commit)
- [9. Struct 与字段修改](#struct)
- [10. Queue array、存储与成本](#storage)
- [11. 验收要求与实现状态](#validation)

<a id="model"></a>

## 1. 对象与执行模型

| 对象 | 职责 | 调度身份 |
| --- | --- | --- |
| Module | 保存连接、内部状态对象和控制逻辑，选择本轮调用的 Rule | 激活和 Work 单位，使用 ModuleId |
| Rule | 计算 Queue proposal 和未来唤醒请求，整体确认本次效果 | Module 的成员函数，使用独立 RuleId |
| Queue | 保存持久状态、来源 proposal、端口及订阅记录 | 状态和 Xfer 单位，使用 QueueId |
| 调度器 | 执行 Module Work，调用 Rule 仲裁，传播容量通知，统一提交 | 管理 tick、eventQueue、DFS 和任务列表 |

一个 Module 实例对应一个 SimObject。Rule 不需要独立 SimObject；编译器生成 `work_<rule>(...)` 和 `arbitrate_<rule>()`，两者绑定同一 RuleId。不同实例可复用函数代码，但记录相互独立。

当前持久状态使用 Queue。FIFO 保存多个元素；寄存器可用容量 1、初始化并始终保持一个元素的受限 Queue 表示，只允许 revise。容量 1 但允许空满切换的流水级仍是 FIFO。连接两端引用同一 Queue 对象。独立 Cell 对象尚未定义，见 [Q03](open-questions.md#q03)。

Module 有自己的控制流，Rule 内也有控制流。Module 不是原子事务：它调用的不同 Rule 可以分别成功或失败；每条 Rule 本次实际提出的 Queue 修改和未来唤醒请求构成原子单位。嵌套 Rule 和跨 Rule 联合原子组不在本规格的支持范围。

tick 是电路周期。全部 Work 读取本 tick 不变的 Queue `current`；Work 完成后候选固定，仲裁阶段只推进容量许可。任务耗尽后统一 Xfer，下一 tick 才看到新状态。不再划分 delta 仲裁批次。

```text
初始化／eventQueue 到期事件 → tick 开始完成激活的 Module Work
    → 新准备的候选与已有候选的仲裁任务合并
    → DFS 先处理容量前驱，再原子仲裁
        获准 pop → 通知唯一生产者
        获准未来唤醒请求 → 登记 eventQueue
    → 任务耗尽 → 全部 Queue Xfer
        状态变化 → wakeup(M, tick + 1) → eventQueue
```

每个 Module 每 tick 至多 Work 一次，每个 Rule 每 tick 至多获准一次。Accepted 本 tick 不撤回；新 push 的数据不能同 tick 转发给其他 Work。

Module 本 tick 可以不 Work，继续保留最近一次 Work 的选择和完整 pending 候选。容量通知不因 workedTick 不是本 tick 而重新运行 Module；候选不存在或不完整时跳过。

子 Module 只通过输入、输出 Queue 交换效果；其内部读取记到子 Module 自己名下。父子 Module 的具体调用、激活和构造方式尚未确定，见 [Q02](open-questions.md#q02)。

<a id="construction"></a>

## 2. 编译器与运行前构造

编译器输出成员函数、资源绑定和静态记录；运行前完成实例化与机械构造。调度过程中不分析函数体，也不依赖反射或装饰器发现关系。

| 静态记录 | 内容及用途 |
| --- | --- |
| Module 表 | ModuleId → 实例及 Work 入口 |
| Rule 表 | RuleId → 所属 Module、Work、仲裁入口、可能修改资源及允许操作 |
| Queue 来源索引 | SourceId → 固定 proposal 槽位，只注册可能操作该 Queue 的来源 |
| `Q.popRuleId` | 唯一可能向 Q pop 的 Rule，可为空；供 DFS 查找容量前驱 |
| `Q.pushRuleId` | 唯一可能向 Q push 的 Rule，可为空；供获准 pop 通知生产者 |

静态记录覆盖全部可能分支；实际 deps、participants 和通知由本次执行路径确定。运行前不要求建立或保存静态容量图、全局拓扑顺序。可能连接形成静态环不拒绝构造；仲裁时结合实际 proposal 和 Queue 容量判断动态依赖，见第 7 节。

ModuleId、QueueId 标识资源实例；RuleId 从 1 开始，同时作为 Queue 的 SourceId。身份不能依赖函数名、参数名或 SSA 打印名称。引用同一 Queue 的别名按同一资源处理。

来源索引、槽位地址和对象句柄在运行期间保持稳定。同一来源重复注册不增加槽位；不为每个 Queue 按全局 Rule 总数分配槽位。

每个 Queue 保留来源 0，供外部驱动或隔离测试使用，不是普通生成 Rule。生成的电路状态修改都必须归属 RuleId；不能用来源 0 绕过原子仲裁。不同 Queue 的来源 0 不自动构成跨 Queue 事务，也不能隐式参加 Xfer。popRuleId/pushRuleId 只记录生成 Rule，不对来源 0 查询 ownerModule。外部驱动入口尚未定义，见 [Q07](open-questions.md#q07)。

编译输入需要保留 Module 选择、受条件保护的读取、实际 proposal、正常完成条件和原子边界，详见 [ACIR 编译契约](../acir/rule.md)。Queue array 的每个表项按普通 Queue 构造，见第 10 节。

<a id="work"></a>

## 3. Work 的控制与读取语义

### 3.1 Module Work

Module `Work()` 直接调用选中的 `work_<rule>(...)`，不调用仲裁入口，也不提供 Module 总仲裁。调用顺序不表示提交顺序，调用返回不表示本拍已经提交。

除调度器自身的 wakeup 外，只有 Rule 可以主动请求未来事件。Module Work 不直接登记事件，通过选择相应 Rule 表达时间驱动。

Module 控制流可读取寄存器 Queue、Queue 的当前空满等状态，并传递组合值。它不消费元素，不代 Rule 预读消息输入 payload，不直接提出状态修改；所有持久修改归属某个 Rule。Module 控制流不能读取 accepted pop、`canPush` 等仲裁阶段变化的许可。

Module 提前返回或必要读取失败只停止后续调用，先前已选中的独立 Rule 仍可仲裁。若某个条件需要阻止一组修改，应在调用前判断，或将修改放入一条原子 Rule。被选中的 Rule 后来失败，不自动改选其他分支。

Module 选择和参数保留到下一次 Work。每 tick 开始，先完成因读取状态变化或明确事件激活的全部 Module Work，再开始仲裁；仲裁期间不重新选择 Rule。若生产者需要等待同拍 pop 腾出空间，不能仅因输出 `current.full()` 而排除其调用；输出容量应在仲裁时检查。

### 3.2 Rule Work

Rule Work 沿实际分支计算，读取 current、提出 Queue proposal 和未来唤醒请求、记录完成性。它不 accept、不修改 current，也不直接写入 eventQueue。

| 路径结果 | 含义 |
| --- | --- |
| 必要读取失败 | 不完整，清理部分 proposal 和事件请求；不能独立提交已提出的效果 |
| 正常返回，包括业务提前返回和 `return None` | 完整；输出缺失不等于失败 |
| 完整且无 Queue proposal、无事件请求 | 不形成 firing，不更新 acceptedTick |
| 完整且有 Queue proposal 或事件请求 | 进入后续原子仲裁；可以只有 pop，也可以只有事件请求 |

Rule 计算只依赖登记的 Queue、调用参数及固定配置。时间或其他可变外部值必须明确表达为输入或参数；未来唤醒请求只保存目标 ModuleId 和延迟，时间语义见 [事件登记](#events)。

### 3.3 读取与消费

底层 `peek()` / `tryPeek()` 始终纯读旧队首。反复读取不移动位置，提出 pop 后也不跳到下一元素。空 Queue 不得读取 payload；`empty()` / `full()` / `size()` 只查询 current。

编译器按访问角色生成操作：

| 访问 | 是否生成消费 proposal |
| --- | --- |
| Rule 实际读取消息输入 payload，包括分支条件 | 另行生成一次 pop，只有整条 Rule 获准才消费 |
| 寄存器读取、Queue 状态查询 | 不生成 pop |
| Module 向 Rule 传递普通组合值 | 不生成 pop |

同一输入在路径中反复读取只生成一个消费需求；资源别名也按 QueueId 归一化。未选中输入、未产生输出不参与本次资源许可。底层重复 proposePop/proposePush 不能靠覆盖槽位静默成为合法操作。

Module 和选中 Rule 凡实际读取 current，包括读空，都登记为所属 Module 的订阅。Rule 的读取还登记到其 deps；pop/revise 目标也登记依赖。纯 push 不自动依赖输出旧内容，但若显式查询过输出状态，该查询仍是依赖。

<a id="records"></a>

## 4. 运行记录

记录属于 runtime；Rule 本身仍是成员函数。

### 4.1 ModuleRecord

| 字段 | 用途 |
| --- | --- |
| `workedTick` | 同 tick Work 去重 |
| `readGen` | 当前已发布的读取订阅代号 |
| `selectedRules` | 最近一次 Work 实际选中的 RuleId 列表，未重新 Work 时跨 tick 保留 |
| `previousSelectedRules` | Module 再次 Work 时保存旧选择，用于取消未再选中的候选 |

Module 不要求保存正向 Queue 读取列表。Work 期间准备 `newReadGen`，完成后发布。没有重新 Work 就不推进 readGen；readGen 不是 tick。

### 4.2 RuleRecord

| 字段 | 用途 |
| --- | --- |
| `deps: vector<ReadDep>` | 实际 QueueId 及 stateVersion，按 QueueId 去重 |
| `participants: vector<QueueId>` | 实际提出 proposal 的 Queue，按 QueueId 去重 |
| `wakeRequests: vector<WakeRequest>` | 未发布的未来唤醒请求，每项保存目标 ModuleId 和 delay |
| `candidateArgs` / `callArgs` | 已缓存候选的参数／最近一次 Module Work 的调用参数，按值保存 |
| `complete` | 计算是否完整 |
| `selectedTick` | 最近一次被 Module 调用的 tick，仅用于 Work 入口的重复调用去重 |
| `acceptedTick` | 本 tick 是否已获准 |

deps 与 participants 不同：只读 Queue 可以仅在 deps 中，纯 push 输出可以仅在 participants 中。Proposal 的 payload 只存在 Queue 槽位，Rule 不复制一份。

候选是否有效果统一按以下条件判断，供缓存和仲裁使用：

```python
def hasCandidateEffects(R):
    return bool(R.participants or R.wakeRequests)
```

同一 RuleId 本 tick 的重复调用只准备一次，参数必须相同，由用户代码保证。本版不处理同 tick、同 RuleId 使用不同参数的情况，违反时不保证运行结果。跨 tick 的参数变化仍按第 5.2 节验证缓存并重新计算。不增加 Rule.readGen、dirty、Queue → Rule 动态读取表或 Queue 反向等待表。

selectedTick 不是仲裁资格。未选中、失效、不完整或已提交的候选会被清理；完成本 tick 的 Module Work 阶段后，仍存在的完整 pending 候选属于 Module 最近一次 Work 的有效选择，可以直接仲裁。

### 4.3 QueueRecord

| 字段 | 用途 |
| --- | --- |
| `current` / `capacity` | 持久元素及容量 |
| `stateVersion` | 状态或元素身份变化的版本 |
| `readers[ModuleCount]` | 按 ModuleId 索引的定长 readGen 数组，0 表示尚未登记 |
| 来源索引及 proposal 槽位 | 各来源本次 pop、push 值、revise 动作与状态 |
| accepted 摘要 | 记录整体获准的操作，包括 hasAcceptedPop |
| accepted 槽位列表 | Xfer 时只处理已获准操作 |
| `popRuleId` / `pushRuleId` | 第 2 节定义的唯一 pop/push 来源 |
| `usedTick` | 将本 tick 有获准操作的 Queue 去重加入 usedQueues |

Proposal 是增量，不复制整个 current。槽位状态如下：

```text
Empty → Pending → Accepted → Xfer 后 Empty

未获准候选 cancel → Empty
```

来源 0 是合法来源编号，“无 Rule 来源”须使用 optional 或有效位。时间戳也须能表示 tick 0；代号溢出与重启边界见 [Q11](open-questions.md#q11)。

### 4.4 调度记录

调度器持有当前 tick 和 eventQueue。eventQueue 保存 `(wakeTick, ModuleId)`，提供按 wakeTick 取出到期事件的操作；既承接状态变化后的下一 tick 激活，也承接 Rule 获准后的未来唤醒。

Module Work 任务按 ModuleId 去重，在 tick 开始由到期事件及初始化／明确驱动合并得到。Rule 仲裁只用一份任务列表，按 RuleId 在本 tick 去重，读取游标推进时允许追加。任务记录使用运行前分配的固定容量 ID 数组、有效长度和入队代号；避免逐 tick 全表清零。

DFS 使用按 RuleId 索引的定长 `visitedTick`、`visitState` 数组。visitedTick 不匹配表示尚未访问，匹配时状态为 Visiting 或 Done。Done 包括仲裁成功和失败，在整个 tick 去重；acceptedTick 记录已获准结果。访问标记不替代 Module.readGen 或 Queue.stateVersion。不保存等待 Queue、反向等待者或下一 tick 端口重试任务。

usedQueues 用 QueueId 列表和 usedTick 去重。Module 选择列表、Rule deps 和 participants 使用可复用 vector；具体去重辅助表示不在本文规定。

<a id="prepare"></a>

## 5. Module 订阅与 Rule 候选准备

### 5.1 订阅的登记、发布与匹配

单线程 Work 期间不提交 current、不发送状态变化通知，可直接写 Queue 的反向读者条目：

每个 Queue 在运行前分配 ModuleCount 个槽位，只保存代号，ModuleId 由数组下标表示。数组初始化为 0；Module.readGen 初始为 0，实际发布的读取代号从 1 开始。通知时跳过未登记的 0，避免未运行 Module 被误判为有效读者。

```cpp
std::array<ReadGen, ModuleCount> readers{};
```

```python
# Module 开始 Work
newReadGen = M.readGen + 1

# Module 或选中的 Rule 实际读取 Q，包括读空
Q.readers[M.id] = newReadGen
# Rule 读取及 pop/revise 目标另外记录
R.deps.record(Q.id, Q.stateVersion)  # 按 QueueId 去重

# Module Work 结束：先取消本次未选中的旧候选，再发布
M.readGen = newReadGen

# 全部 Xfer 完成后，对状态变化的 Q
for moduleId, savedGen in enumerate(Q.readers):
    if savedGen != 0 and savedGen == modules[moduleId].readGen:
        wakeup(moduleId, tick + 1)
```

重新 Work 后，读取关系由这次实际读取集合替换：本次读到的 Queue 覆盖该 Module 的原槽位，未读到的 Queue 保留旧代号，通知时忽略。每个 `(QueueId, ModuleId)` 始终只有一个固定槽位，不保存历代代号、不增删条目，也不需要回收旧订阅。Module 隔若干 tick 未 Work，原订阅仍有效。必要读取失败不撤销已经登记的 Module 订阅。

### 5.2 Module Work 再次调用 Rule

生成的 Rule Work 入口按 selectedTick 对本 tick 调用去重。首次调用将 RuleId 登记到 Module.selectedRules，设置 selectedTick，并按值保存 callArgs，再判断旧候选。候选复用条件：

```python
valid = (R.complete and hasCandidateEffects(R)
         and sameParameters(R.candidateArgs, args)
         and all(queues[d.queueId].stateVersion == d.stateVersion
                 for d in R.deps))

if valid:
    for d in R.deps:
        queues[d.queueId].readers[M.id] = newReadGen
    # 保留 proposal 和 wakeRequests，跳过业务计算
else:
    discardCandidate(R)  # 清理旧 proposal、deps、participants、wakeRequests
    R.candidateArgs = copyValue(args)
    runRuleBody(R, args)  # 显式记录读取、proposal 和 complete/abort
```

参数比较必要：Module 可能读取控制 Queue 后把新值传给 Rule，该控制 Queue 不一定在 Rule.deps 中。参数与捕获值使用 AC 类型的值语义；Queue 引用使用稳定资源句柄，不保存 Work 局部变量的悬空引用。

缓存命中仍需重新登记 deps 的 Module 订阅，否则 Module 新 readGen 发布后会漏掉这些读取关系。只有完整且有 Queue proposal 或事件请求的候选可以跨 tick 复用；缓存保留 delay，不保留按计算 tick 推出的绝对到期时间。

Module 本次 Work 结束时，取消 previousSelectedRules 中本次未选中的旧候选，发布 readGen。Rule 必要读取失败须在仲裁前完成部分候选清理，但保留已登记的 Module 订阅。

### 5.3 容量通知直接仲裁

无论候选在本 tick 形成还是从之前 tick 保留，获准 pop 的容量通知都只对已有完整 pending 候选安排仲裁，不进入 Module／Rule Work，也不再次比较参数和版本。Module 本 tick 没有 Work 不妨碍该候选仲裁。

这一点依赖阶段顺序：Rule.deps 中的读取及 pop/revise 目标已登记为 Module 订阅，前一 tick 的变化会激活本 tick Module Work；所有被激活 Module 必须先完成候选验证、重算或取消，再处理仲裁任务。Module 控制读取与明确事件同样先更新选择和参数。

Module 未重新 Work 时，readGen 不推进，原订阅持续有效。没有读取状态变化或明确事件，候选计算和最近一次选择仍有效，无需仅为进入新 tick 重做缓存验证。

不完整或已清空的候选不因容量变化重算；新数据只能下一 tick 读取。已获准 Rule 不重复仲裁或提交。

<a id="arbitration"></a>

## 6. Queue 资源与 Rule 原子仲裁

### 6.1 Queue 操作与接口职责

| 接口 | 语义 |
| --- | --- |
| `registerSource(id)` | 运行前登记来源和固定槽位 |
| `peek/tryPeek/empty/full/size/capacity` | 纯读 current |
| `proposePop(id)` | 请求删除旧队首 |
| `proposePush(id, value)` | 按值保存追加元素 |
| `proposeRevise<Path...>(id, value)` | 保存对旧队尾的延迟修改 |
| `hasProposal(id)` | 查询该来源实际操作是否非空 |
| `hasPendingPop/hasPendingPush(id)` | 查询该来源的实际 Pending 操作，供容量依赖和通知使用 |
| `hasPushSpace(id)` | 查询第 6.2 节的容量条件，不改变记录 |
| `canAccept(id)` | 纯检查这个 Queue 上该来源的整组操作，不预约、不改变记录 |
| `accept(id)` | 将 Pending 确认为 Accepted，更新整体获准摘要 |
| `cancel(id)` | 清理该来源未获准的 proposal |
| `Xfer()` | 提交 Accepted 并清理摘要，保留其他 Pending |

每个 Queue 每 tick 至多一个 pop、一个 push。Pop 要求旧队首存在；push 要求容量；revise 要求合法旧队尾和受支持的修改范围。寄存器模式禁止 pop/push。Revise 不作用于本 tick 新 push 的元素。

Propose 不修改 current。某来源在 Queue 第一次形成非空槽位时，将 QueueId 登记到 Rule.participants；整个来源在该 Queue 的操作作为一组检查。静态唯一来源记录不代表该来源本次一定提出操作，必须查询实际槽位。

Revise/pop 可以指向同一旧元素，按第 8 节的顺序先 revise 再 pop。空槽位的无操作不能替代 Rule.complete 判定。

### 6.2 push 的容量条件

Queue 有原有空位，或同 tick 有合法 pop，就具备一个 push 的容量。满 Queue 的 pop/push 可以属于同一 Rule，也可以属于不同 Rule；所有操作仍须满足整条 Rule 的原子性。

```text
hasPushSpace = !current.full()
               || hasAcceptedPop()
               || thisCandidate.hasLegalPop()
```

其他 Rule 的 pop 必须已经整体获准，才能提供空间；同一候选的 pop/push 一起检查、一起获准。只有 pop 意愿不足以许可其他 Rule 的 push。

若 current 为空，不能消费本 tick 新 push 的数据。

### 6.3 先检查，后整体确认

本版按单线程描述，且不考虑资源端口竞争。DFS 返回后，先纯检查全部 participants，再统一 accept；检查与确认之间不插入其他 Rule 仲裁，不需要临时预约、Reserved 状态或 release。

```python
def arbitrateRule(rid, tick):
    R = rules[rid]
    if R.acceptedTick == tick or not R.complete or not hasCandidateEffects(R):
        return False

    for qid in R.participants:
        if not queues[qid].canAccept(rid):
            return False  # 未改变任何槽位，完整候选继续 Pending

    for qid in R.participants:
        queues[qid].accept(rid)
    R.acceptedTick = tick
    for request in R.wakeRequests:
        wakeup(request.moduleId, tick + request.delay)
    return True
```

任一许可检查返回失败都不部分确认，保留完整 proposal、wakeRequests、参数和 deps。全部检查成功后，整体 accept 与通知之间不插入其他 Rule 仲裁，Accepted 本 tick 不撤回。

未来唤醒请求只在整体获准后发布。纯事件 Rule 的 participants 可以为空，无 Queue 资源需要检查，直接确认并发布事件；仍受每 tick 至多一次获准限制。

accept 或 Xfer 发生执行异常时，直接终止仿真，不恢复、不回滚，也不继续使用部分完成的状态。本版不要求额外的无异常类型约束或异常后恢复机制。

<a id="schedule"></a>

## 7. tick 调度与通知

### 7.1 Work 屏障

启动时，在 tick 0 将所有已构造 Module 的 ModuleId 加入初始 Work 列表，各 Work 一次。首次 Work 建立实际读取订阅，并按正常流程准备候选、参与首个 tick 的仲裁和 Xfer。各 Work／仲裁时间戳初始为无效 tick，使 tick 0 的执行不会被误判为已完成。

启动激活作为 runTick 的 initialModules 传入。后续 tick 取出 eventQueue 中到期事件，与调度器明确提供的激活合并，按 ModuleId 去重后完成 Work。外部驱动的剩余问题见 [Q07](open-questions.md#q07)。

`runModuleOnce` 执行第 5 节的完整流程：标记 workedTick、准备新读取代号、执行选择和候选准备、清理未选中候选、发布读取代号。选中的新候选及复用候选加入仲裁任务列表；全部 Module Work 完成后才开始仲裁。仲裁期间不运行 Module／Rule Work，不改变候选和调用参数。

### 7.2 唯一容量前驱

生产者 P 实际向满 Queue Q push，且没有整体获准 pop、自身也没有合法 pop 时，检查 Q.popRuleId 指向的唯一消费者 C。C 有完整 Pending 候选、且实际 pop Q，才形成容量依赖 `C → P`。DFS 从 P 访问 C，返回后仲裁 P。

**可以递归仲裁未在初始任务列表中的消费者，只要它有有效完整候选。**消费者所属 Module 本 tick 未 Work 不影响资格；所有状态变化激活的 Module 已在屏障前完成候选验证、重算或取消。

```python
def hasCompletePendingCandidate(rid, tick):
    R = rules[rid]
    return R.complete and hasCandidateEffects(R) and R.acceptedTick != tick


def actualCapacityPredecessors(rid, tick):
    for qid in rules[rid].participants:
        Q = queues[qid]
        if not Q.hasPendingPush(rid) or Q.hasPushSpace(rid):
            continue
        consumer = Q.popRuleId
        if (consumer is not None and consumer != rid
                and hasCompletePendingCandidate(consumer, tick)
                and Q.hasPendingPop(consumer)):
            yield consumer
```

按实际槽位及当前 accepted 摘要逐个查找前驱，不预存静态容量图。Queue 已有空间、已有获准 pop 或同一候选可合法 pop/push 时，不形成容量边。只读订阅、未选中分支、无候选及不完整候选也不形成容量边。

满 Queue 没有可提供 pop 的候选时，生产者正常仲裁失败并保留候选；这本身不是动态环。

### 7.3 按 tick 去重的 DFS

从仲裁任务列表的任意节点开始，先访问全部实际容量前驱，再执行第 6 节的纯检查和整体确认。不先生成拓扑排序列表。

```python
def visitRule(rid, tick, acceptedRules):
    if not hasCompletePendingCandidate(rid, tick):
        return
    if visitedTick[rid] == tick:
        if visitState[rid] == "Visiting":
            raise RuntimeError("dynamic capacity cycle")
        return  # Done：本 tick 已处理，失败也不重试

    visitedTick[rid] = tick
    visitState[rid] = "Visiting"
    for consumer in actualCapacityPredecessors(rid, tick):
        visitRule(consumer, tick, acceptedRules)

    accepted = arbitrateRule(rid, tick)
    visitState[rid] = "Done"
    if not accepted:
        return

    acceptedRules.append(rid)
    for qid in actualAcceptedPops(rid):
        Q = queues[qid]
        producer = Q.pushRuleId
        if (producer is not None
                and hasCompletePendingCandidate(producer, tick)
                and Q.hasPendingPush(producer)):
            ruleTasks.enqueue(producer)
```

Visiting 表示仍在递归栈上；沿实际必需的容量依赖再次遇到 Visiting，报动态容量环。Done 表示本 tick 已处理，成功和失败都去重。visitedTick 初始使用无效 tick，避免 tick 0 误判。

失败可以保持到本 tick 结束：current 和候选不变，pop/push 来源唯一，且 DFS 已递归检查全部必需消费者。若某个 push 仍缺空间，其唯一消费者已失败或没有候选，不会后来由另一消费者释放空间。其他旧目标检查也不会因本 tick 新 push 改变。这个结论依赖无端口竞争及完整前驱访问，不能用于此前只访问部分批次的流程。

```text
数据流：A → Q1 → B → Q2 → C
实际容量依赖：C → B → A

若 A/B 需要下游 pop，且三条 Rule 都有完整候选：
即使初始任务只有 A，也访问 A → B → C，返回时按 C、B、A 仲裁。
以后任务列表再次遇到 B/C，直接跳过。
```

静态连接有环，但实际分支不成环，或 Queue 已有空间而无需互相等待时，允许正常仲裁。实际必需的容量依赖成环才报错；不引入跨 Rule 联合原子提交来解环。

### 7.4 一份任务列表与状态通知

```python
def runTick(tick, initialModules):
    scheduler.tick = tick
    moduleWorkTasks.enqueueAll(initialModules)
    for event in eventQueue.popDue(tick):
        moduleWorkTasks.enqueue(event.moduleId)
    acceptedRules = []

    for mid in moduleWorkTasks.takeAll():
        if modules[mid].workedTick != tick:
            runModuleOnce(mid, tick)
            ruleTasks.enqueueAll(modules[mid].selectedRules)

    while ruleTasks:
        rid = ruleTasks.takeOne()  # 使用游标取出，允许遍历时追加
        visitRule(rid, tick, acceptedRules)

    changedQueues = xferAll(usedQueues)  # 全部提交后，才安排下一 tick
    for qid in changedQueues:
        for mid, savedGen in enumerate(queues[qid].readers):
            if savedGen != 0 and savedGen == modules[mid].readGen:
                wakeup(mid, tick + 1)
    clearCommittedCandidates(acceptedRules)
    usedQueues.clear()  # 保留列表容量，usedTick 以 tick 区分下一轮
    ruleTasks.clear()  # 保留缓冲区，下个 tick 使用新的入队代号
    moduleWorkTasks.clear()
```

获准 pop 只通知唯一生产者，并检查其完整候选和实际 push。没有候选、不完整或已获准则跳过，不运行 Work，不复活已提交候选。任务只保存 RuleId，执行时读取当前记录；selectedTick 不要求等于当前 tick。

例如只有 C 的 Module 本 tick Work，B/A 有跨 tick 保留的完整候选，C 的 pop 可将 B 加入同一任务列表，B 的 pop 再加入 A；B/A 的 Module 不需要 Work。递归访问与通知入队共享本 tick 的访问标记，不重复仲裁。

状态变化对有效订阅调用 wakeup(M, tick + 1)，与定时事件共用 eventQueue；不因端口重置安排仲裁。不保存 waitingQueueId 或 Queue.waiters：唯一 pop/push 来源不会被另一 Rule 占用同类端口，容量释放已在本 tick 由获准 pop 传播；其他端口竞争不在本版范围。

<a id="events"></a>

### 7.5 事件请求、登记与到期

时间以整数 tick 表示。Rule Work 将实际分支请求的 `(ModuleId, delay)` 保存到 wakeRequests，要求 delay ≥ 1。只有整条 Rule 获准时，才按获准 tick 计算 `wakeTick = tick + delay`；候选在等待期间复用，不改变这一起算点。

事件统一经调度器 wakeup 入口登记：

```python
def wakeup(moduleId, targetTick):
    # 前提：targetTick > scheduler.tick
    eventQueue.add(targetTick, moduleId)
```

wakeup 只登记未来激活，不立即执行 Work。调度器可因 Queue 状态变化等原因自行调用；其他主动请求只能来自获准 Rule，Module Work 不直接调用此入口。

到期事件全部取出，Module Work 列表按 ModuleId 去重，同一 Module 同一 tick 只 Work 一次；不同 tick 的事件分别保存。到期事件不会在本 tick 仲裁中插入 Work。已登记事件的取消／覆盖策略仍见 [Q06](open-questions.md#q06)。

<a id="commit"></a>

## 8. Xfer 与候选生命周期

### 8.1 提交顺序

所有同 tick Work 和仲裁结束后，只处理 usedQueues。每个 Queue 按操作类别提交全部 accepted 槽位：

```text
全部 revise：修改旧队尾 → pop：删除旧队首 → push：追加新元素
```

不能按来源逐个执行完整三步。Xfer 不重新仲裁，不运行 Work；全部 Queue 提交完成后再发送下一 tick 通知。

Queue 只有一个旧元素时，旧队首和旧队尾是同一元素；若 revise 和 pop 都已获准，先修改该元素，再删除它。两项操作可以来自同一 Rule，也可以来自不同 Rule，均按上述顺序提交。

stateVersion 在 current 数据或元素身份改变时推进。Pop 后 push 相同 payload 仍改变元素身份，必须推进；无变化 revise 和 tick 推进不改变版本。

Xfer 清理 accepted 槽位和 accepted 摘要；其他 Pending 保留。已提交 Rule 清空候选，不能下一 tick 再提交同一结果。

### 8.2 清理与保留

| 情况 | proposal 与 Rule 记录 | Module 读取订阅 |
| --- | --- | --- |
| 必要读取失败 | 清理部分 proposal、deps、participants、wakeRequests | 保留本次已登记关系 |
| 参数或依赖不匹配 | 取消旧候选，再计算新候选 | 按新 Work 及读取代号登记 |
| Module 本次未选中旧 Rule | 取消旧候选 | 未重登的旧关系按代号失效 |
| 完整候选许可检查失败 | 不改变槽位，保留完整候选及未发布事件请求 | 有效关系继续保留 |
| 完整但无 Queue proposal、无事件请求 | 不形成 firing | 已读关系保留 |
| 整体获准 | Queue Accepted 留到 Xfer；事件按获准 tick 发布；本 tick 不取消或重复提交 | 已读关系保留 |
| Xfer 完成 | 清理已提交候选；其他完整 Pending 可以跨 tick 保留 | 状态变化后按代号通知 |

跨 tick 保留不要求每 tick 重新调用 Rule。若本 tick 没有相关状态变化或明确事件，容量通知可直接仲裁旧候选；若 Module 需要 Work，则先更新选择和候选，再处理仲裁任务。

取消或重新计算必须遍历 participants 清理 Queue 槽位，并释放其中保存的捕获值。槽位身份及可复用缓冲容量保留。只把 Rule 标记 invalid 不足以清理多个 Queue 上的 payload。

未来唤醒请求先保存在候选内，只在整条 Rule 获准后发布；不完整、失效或未选中候选丢弃未发布请求。Xfer 后清理已提交候选时，也清空 wakeRequests，包括纯事件 Rule；已发布记录由 eventQueue 持有，独立于候选清理。不能仅因 Module 再次 Work 或 readGen 改变就忽略已发布事件。

<a id="struct"></a>

## 9. Struct 与字段修改

元素可以是 AC 标量或按值嵌套 struct。GFSim 使用普通 C++ public 字段，保留对应 AC 类型的位宽和值语义；局部 struct 赋值是即时组合计算，Queue current 仍只读。

字段路径在编译期用数据成员指针表示，新值在 Work 按目标字段类型计算并按值保存，目标是 Xfer 时的旧队尾：

```cpp
struct Meta { UInt<8> epoch{}; };
struct Entry { Meta meta{}; UInt<32> value{}; };

queue.proposeRevise<&Entry::meta, &Meta::epoch>(ruleId, nextEpoch);
queue.proposeRevise<&Entry::value>(ruleId, nextValue);
queue.proposeRevise<&Entry::meta>(ruleId, nextMeta);
queue.proposeRevise<>(ruleId, nextEntry); // 空路径表示整值替换，也适用标量
```

现有 C++ 实现示意使用 `std::function<void(Element&)>` 保存延迟赋值：

```cpp
template<auto Member, auto... Rest, class Object>
decltype(auto) fieldAt(Object& object) {
    if constexpr (sizeof...(Rest) == 0)
        return (object.*Member);
    else
        return fieldAt<Rest...>(object.*Member);
}

template<auto... Path, class Value>
void proposeRevise(SourceId source, Value value) {
    pendingProposal(source).revises.emplace_back(
        [saved = std::move(value)](Element& target) {
            if constexpr (sizeof...(Path) == 0)
                target = saved;
            else
                fieldAt<Path...>(target) = saved;
        });
}
```

示例省略候选状态、参与者登记和目标依赖登记；这些仍必须遵守前述记录契约。延迟动作只能赋入已经确定的值，不能在 Xfer 重读其他 Queue、重新执行业务条件或使用失效引用。

路径每层类型必须匹配，最终字段必须可赋值。当前路径方案不包含 bit-field、运行时数组索引、指针解引用或可选对象解包。Queue array 的动态元素选择与 struct 内字段路径是不同问题。

`std::function` 方案要求捕获值可复制构造，示例赋值从捕获值的 const 引用读取。Xfer 赋值发生执行异常时，按第 6.3 节直接终止仿真。

<a id="storage"></a>

## 10. Queue array、存储与成本

Queue array 是普通 Queue 引用的数组，每个表项都是独立、有 QueueId 的 Queue。`table[i].peek()`、`table[i].proposePush(ruleId, value)` 等操作先按下标定位实际 Queue，再调用普通 Queue 接口。

每个表项分别构造来源绑定、proposal 槽位和读者数组，分别遵守唯一 pop/push 来源约束。读取登记到实际 QueueId，proposal 保存在该 Queue 的槽位，参与者也记录实际 QueueId；调度、取消和提交沿用普通 Queue 的流程。QueueCount 包含全部数组表项。

设计假设 ModuleCount、RuleCount 远小于 QueueCount。Queue 读者记录使用定长数组：每个 Queue 固定保存 ModuleCount 个 readGen，读取登记按 ModuleId 直接覆盖，状态通知顺序扫描该数组。

读者数组总空间为 `QueueCount × ModuleCount × sizeof(ReadGen)`，运行期间不因读取路径变化而增长。动态下标从 Q1 切换到 Q2 时，Q2 对应槽位写入新代号，Q1 对应槽位留下旧代号；旧值按匹配规则失效，不需要清理或释放槽位。

状态通知只遍历实际变化的 Queue，每个 Queue 扫描 ModuleCount 个槽位，不扫描全部 Queue。

Rule 的 deps 和 participants 仍使用可复用 vector，只记录实际读取或修改的 QueueId。动态下标只登记实际访问表项；计算下标所读取的状态仍按普通读取登记。若下标作为 Rule 调用参数传入，则参与第 5.2 节的参数比较。

设 d 为 Rule 实际依赖数、p 为实际参与 Queue 数：

| 操作 | 遍历范围 |
| --- | --- |
| Module Work 中的候选验证及订阅重登 | d，另加参数比较成本 |
| Rule 许可检查、确认、候选取消 | p，另加各 Queue 的局部操作成本 |
| 每 tick 的 DFS | 每个被访问 Rule 至多处理一次；前驱查找扫描其实际 participants |
| 状态变化通知 | 该 Queue 的固定 ModuleCount 个读者槽位 |
| 获准 pop 传播 | 直接检查唯一 pushRuleId 及其实际候选 |
| 事件发布／到期 | 获准 Rule 的实际 wakeRequests／当前 tick 到期的事件 |
| tick 末提交 | usedQueues 及其 accepted 操作 |

直接仲裁跳过业务计算和参数／依赖验证，仍须检查全部实际参与资源。整体取消不是 O(1)；单个槽位状态修改可为常数成本，但释放多个槽位的捕获值须逐个处理。

current 的容器、队首删除方式、字段动作存储以及 DFS 前驱查找和遍历栈可在实现中优化；这里不根据 Python 的相对耗时推断 C++ 性能。

<a id="validation"></a>

## 11. 验收要求与实现状态

### 11.1 核心验收场景

| 场景 | 必须验证的结果 |
| --- | --- |
| Module 与 Rule 分支 | 只准备被调用 Rule，只检查实际 participants，未选中旧候选取消 |
| 读空与不完整尝试 | 不访问无效 payload，部分 proposal 清理，后续输入变化可激活 Module |
| Module 读取集合变化 | 原来读 Q1/Q2、本次只读 Q2 后，Q1 的旧条目不再激活 Module |
| Rule 缓存命中 | 参数和版本匹配时复用，缓存 deps 重登 Module 新读取代号 |
| 参数或动态下标变化 | 清理旧候选并重新计算，不使用旧目标或旧输出值 |
| 原子许可检查失败 | 多 Queue 任一失败不改变任何槽位，完整候选保留，不部分提交 |
| 满流水链 | 消费先行，其他 Rule 只有整体获准 pop 才提供空间；同一 Rule 的 pop/push 整体检查 |
| DFS 顺序与去重 | 从生产者开始仍先处理容量前驱；前驱不在初始任务列表也递归访问，共享前驱本 tick 只处理一次 |
| 静态环与动态环 | 静态连接有环但实际依赖无环时允许执行；已确定必需的容量依赖成环时报错；有空间、自身 pop/push、无消费者不误报 |
| 失败结果复用 | 完整前驱 DFS 后失败，本 tick 不重复处理；下一 tick 使用新的访问标记 |
| 容量传播 | 获准 pop 将唯一生产者加入同一任务列表；跨 tick 候选直接仲裁，不增加 Module／Rule Work |
| 无同 tick 数据转发 | 空输入不能读取本 tick push；获准每 tick 至多一次 |
| Xfer 与元素身份 | revise → pop → push；pop/push 相同 payload 仍推进版本 |
| Revise/pop 指向同一旧元素 | 允许提交，先 revise 再 pop；操作来自同一 Rule 或不同 Rule 都遵守该顺序 |
| 无变化 revise | 不推进版本，不产生状态变化通知或端口重置仲裁任务 |
| 跨 tick 候选 | Pending 可直接重试仲裁，不要求 selectedTick 等于当前 tick；Module 再次调用时才验证参数和依赖 |
| Work 屏障与旧候选 | 先完成全部 Module Work；DFS 不提交被取消候选，替换的候选按新记录仲裁 |
| 初始激活 | tick 0 所有 Module 各 Work 一次，包括读空订阅；全部 Work 完成后才仲裁 |
| 容量通知但没有完整候选 | 跳过，不运行 Module／Rule Work，不复活已提交候选 |
| 纯事件 Rule | 无 participants 也可确认并发布事件，每 tick 至多一次；既无 proposal 又无事件请求则不 firing |
| 事件与原子确认 | 任一 Queue 检查失败不发布事件；重算、取消或读取失败清理未发布请求 |
| 延迟与跨 tick 候选 | 延迟以获准 tick 为起点；如 tick 10 计算、tick 20 获准、delay=3，则在 tick 23 激活 |
| 事件与 Work 屏障 | 仅允许未来 tick；到期事件在 Work 前处理；同 Module 同 tick 的事件及状态通知只 Work 一次 |
| 固定读者记录 | 每个 Queue 读者数组长度为 ModuleCount；反复切换动态下标只覆盖代号，不增加槽位；未登记的 0 不产生通知 |
| 固定任务及实际依赖 | 任务缓冲复用、proposal 槽位身份稳定；Rule deps 和 participants 只保存实际 QueueId |

子 Module、已登记事件的取消／覆盖和 C++ runtime 接口等尚有待决问题；不能仅靠上述核心场景通过宣称框架整体能力已经验收。

### 11.2 Python experiment

[实验说明](experiment/README.md)记录代码入口和范围，[端到端实验报告](experiment/report.md)记录覆盖矩阵、独立参考方法、性能结果和限制。

实验已按本文重构：使用定长 Queue 读者数组、Rule 实际依赖 vector、Work 屏障、按 tick 去重的显式栈容量 DFS、纯检查后整体确认和事件堆；跨 tick 完整候选可以直接仲裁。已实现 Queue array、纯事件 Rule、静态字段路径修改以及同一旧元素的 revise/pop。

验收通过真实 Module/Rule 组成的流水、包处理、双输入原子处理、分 bank 存储、在线查表、反馈与重试电路完成。独立参考模型重新描述组件事务，用固定点许可与独立状态提交逐拍核对，不调用被测成员函数或核心 DFS。实验不保证端口竞争行为。

实验仍是单线程、平级 Module 的 Python 模型，未实现编译器接入、父子激活、Cell、已发布事件取消、外部来源 0 协议、代号回绕和最终 C++ ABI。完整框架的未决项仍见 [open-questions](open-questions.md)，不能仅因核心电路通过而关闭。
