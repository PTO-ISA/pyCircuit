# GFSim 框架规格

本文汇总当前已确认的设计契约，供编译器生成代码、C++ runtime 实现和调度实验共同使用。本轮动态读者位图与 dirty 机制已在 [Python experiment](experiment/README.md) 实现；[C++20 核心](cpp/README.md) 保留上一版版本检查机制，尚未迁移。编译器及其他未决能力不据此视为完成。未确定的内容集中在 [待决问题](open-questions.md)，不由本文补齐。

核心方案：**Work 更新候选，DFS 解决容量依赖，Xfer 提交状态。读取状态变化或明确事件驱动 Module Work；获准 pop 只通知已有完整候选的唯一生产者仲裁。候选可以跨 tick 保留；Queue 变化经 Module 的实际读者位图标记 Rule dirty，调用参数变化也标记同一个 dirty 位。缓存命中不扫描 Queue 版本、不续订读取。**

同 tick 仲裁采用带访问标记的 DFS：按实际容量依赖先处理消费者，再仲裁生产者。不预生成全局拓扑顺序，不因静态关系有环拒绝模型；实际必需的容量依赖形成动态环时才报错。

Queue 支持同 tick 的 pop/push：有合法 pop 就可以复用释放的空间，包括同一 Rule 的 pop/push。本文中的伪代码和 C++ 示例表达职责与生命周期，不规定最终 ABI。

每个 Queue 保存固定的可能读者 Module 链接；每个 Module 保存局部资源表、动态 Rule 读者位图和 dirty 位图。静态声明决定可访问范围，实际读取决定通知范围。

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
| Module 表 | ModuleId → 实例及 Work 入口、可访问 QueueId 表、所属 RuleId 表 |
| Queue 可能读者链接 | 排序的 `(ModuleId, moduleResourceSlot)`；运行前固定，供变化通知遍历 |
| Queue 槽位映射 | `moduleSlots[ModuleId]` 直接定位局部资源槽位，未声明为 -1；不代表实际订阅 |
| Rule 表 | RuleId → 所属 Module、Work、仲裁入口、可能修改资源及允许操作 |
| Queue 来源索引 | SourceId → 固定 proposal 槽位，只注册可能操作该 Queue 的来源 |
| `Q.popRuleId` | 唯一可能向 Q pop 的 Rule，可为空；供 DFS 查找容量前驱 |
| `Q.pushRuleId` | 唯一可能向 Q push 的 Rule，可为空；供获准 pop 通知生产者 |

静态记录覆盖全部可能分支和动态下标。Python `assemble(..., module_queues=...)` 显式声明每个 Module 可访问的 Queue，含私有状态、输入、输出和只读引用；同 Queue 别名合并。省略时为兼容旧例子允许全部 Queue，生成代码应提供精确表。实际读者位图、participants 和通知由执行路径确定。运行前不要求建立或保存静态容量图、全局拓扑顺序。可能连接形成静态环不拒绝构造；仲裁时结合实际 proposal 和 Queue 容量判断动态依赖，见第 7 节。

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

底层 `peek()` / `tryPeek()` 读取旧队首，在 Work 内自动登记读取依赖，但不消费元素、不修改 current。反复读取不移动位置，提出 pop 后也不跳到下一元素。空 Queue 不得读取 payload；`empty()` / `full()` / `size()` 只查询 current。

编译器按访问角色生成操作：

| 访问 | 是否生成消费 proposal |
| --- | --- |
| Rule 实际读取消息输入 payload，包括分支条件 | 另行生成一次 pop，只有整条 Rule 获准才消费 |
| Rule 读取绑定输出 Queue 或只读状态引用的 payload | 不生成 pop，仍登记读取依赖 |
| 寄存器读取、Queue 状态查询 | 不生成 pop |
| Module 向 Rule 传递普通组合值 | 不生成 pop |

消息 input Queue 的 payload 不支持只观察而不消费：实际读到 payload 就形成 pop proposal，即使该路径最终不产生输出；必要读取失败或整条 Rule 未获准时仍不消费。消费角色由当前 Rule 的资源绑定确定，不能仅根据 `peek()` 调用生成 pop。同一 Queue 可以是 EX Rule 的 output、MEM Rule 的 input：EX 观察旧输出进行前递只登记依赖，MEM 实际读取后生成 pop。Module 的读取范围仍遵守第 3.1 节。

同一输入在路径中反复读取只生成一个消费需求；资源别名也按 QueueId 归一化。未选中输入、未产生输出不参与本次资源许可。底层重复 proposePop/proposePush 不能靠覆盖槽位静默成为合法操作。

Module 和选中 Rule 凡实际读取 current，包括读空，都登记为所属 Module 的订阅。Rule 的读取在 Module 的资源读者位图设置该 Rule 位，Rule 保存实际 readSlots；pop/revise 目标也登记依赖。纯 push 不自动依赖输出旧内容，但若显式查询过输出状态，该查询仍是依赖。

<a id="records"></a>

## 4. 运行记录

记录属于 runtime；Rule 本身仍是成员函数。

### 4.1 ModuleRecord

| 字段 | 用途 |
| --- | --- |
| `workedTick` | 同 tick Work 去重 |
| `resourceQids` / `ruleIds` | 固定局部资源表、所属 Rule 表 |
| `readGen` / `controlReads[slot]` | 仅用于 Module 控制读取的已发布代号与每资源代号 |
| `ruleReaders[slot][word]` | 当前实际读取此资源的本 Module Rule 位图 |
| `dirtyWords[word]` | 本 Module 各 Rule 是否需要重新计算 |
| `selectedRules` / `previousSelectedRules` | 最近选择及重新 Work 前的旧选择，供取消未再选中 Rule |

局部 Rule 编号 i 对应 word=`i // 64`、bit=`1 << (i % 64)`，wordCount=`ceil(RuleCount/64)`。Python 使用扁平 list 存储每个 64 位字，对应未来 C++ 的连续 uint64 数组；不承诺 C++ 已实现。零 Rule 的 Module 仍可有控制读取。

### 4.2 RuleRecord

| 字段 | 用途 |
| --- | --- |
| `readSlots` | 上次尝试实际读取的 Module 局部资源槽位，取消／重算时清除对应读者位 |
| `wordIndex` / `bit` | 本 Rule 在所属 Module 位图中的固定位置 |
| `participants` | 实际提出 proposal 的 QueueId，按资源去重 |
| `wakeRequests` | 尚未发布的 `(ModuleId, delay)` 请求 |
| `candidateArgs` / `callArgs` | 已缓存候选参数／最近调用参数，按值保存 |
| `complete` / `executing` | 完成性及 Work 生命周期 |
| `selectedTick` / `acceptedTick` | 同 tick 调用／提交去重 |

readSlots 与 participants 独立：只读资源可以没有 proposal，纯 push 可以不依赖输出 current。Proposal payload 只存在 Queue 槽位。读取位首次由 0 置 1 时才追加 readSlots，重复读取和资源别名不重复登记。

候选有效果当且仅当 `participants` 或 `wakeRequests` 非空。同一 RuleId 同 tick 重复调用参数必须相同，由模型保证。selectedTick 不要求等于当前 tick 才能仲裁；完整、有实际效果、未 dirty、未在本 tick 获准的保留候选可以直接仲裁。

### 4.3 QueueRecord

| 字段 | 用途 |
| --- | --- |
| `current` / `capacity` | 持久元素及容量 |
| `stateVersion` | 数据／元素身份变化的观察计数，不参与缓存判断 |
| `readers` / `readerSlots` | 固定的可能读者 ModuleId 与其局部资源槽位；供通知遍历 |
| `moduleSlots[ModuleId]` | 构造时分配的直接索引表；未声明访问为 -1，运行期不变 |
| 来源索引及 proposal 槽位 | 该资源可能修改来源的 pop、push、revise 与状态 |
| accepted 摘要及槽位列表 | 本拍获准操作及 hasAcceptedPop |
| `popRuleId` / `pushRuleId` | 唯一可能 pop/push 来源 |
| `usedTick` | 有获准操作的 Queue 加入 usedQueues 时去重 |

槽位生命周期为 `Empty → Pending → Accepted → Xfer 后 Empty`；取消未获准候选也回到 Empty。来源 0 是合法保留编号，无来源使用 optional／有效位。C++ 时间与代号溢出终止策略见 [Q11](open-questions.md#q11)。Python 使用任意精度整数。

### 4.4 调度记录

调度器持有 `activeModule` 与 `activeRule` 两个临时执行上下文，不属于电路持久状态。只有真正执行 Rule body 时设置 activeRule，complete/abort 后恢复 Module 上下文；缓存命中不进入 Rule 上下文。Module Work 所有退出路径（含异常）均清除上下文；嵌套 Rule 执行报错。

调度器持有当前 tick 和 eventQueue。eventQueue 保存 `(wakeTick, ModuleId)`，提供按 wakeTick 取出到期事件的操作；既承接状态变化后的下一 tick 激活，也承接 Rule 获准后的未来唤醒。

Module Work 任务按 ModuleId 去重，在 tick 开始由到期事件及初始化／明确驱动合并得到。Rule 仲裁只用一份任务列表，按 RuleId 在本 tick 去重，读取游标推进时允许追加。任务记录使用运行前分配的固定容量 ID 数组、有效长度和入队代号；避免逐 tick 全表清零。

DFS 使用按 RuleId 索引的定长 `visitedTick`、`visitState` 数组。visitedTick 不匹配表示尚未访问，匹配时状态为 Visiting 或 Done。Done 包括仲裁成功和失败，在整个 tick 去重；acceptedTick 记录已获准结果。访问标记不替代 Module.readGen 或 Queue.stateVersion。不保存等待 Queue、反向等待者或下一 tick 端口重试任务。

usedQueues 用 QueueId 列表和 usedTick 去重。Module 选择列表、Rule readSlots 和 participants 使用可复用 vector；具体去重辅助表示不在本文规定。

<a id="prepare"></a>

## 5. Module 订阅与 Rule 候选准备

### 5.1 动态读取登记

Module 控制读取只更新 `controlReads[slot] = readGen + 1`，Work 结束后发布新 readGen；未重登的控制读取自然失效。Rule 读取设置 `ruleReaders[slot][R.wordIndex] |= R.bit`，仅首次置位追加 R.readSlots。两种关系独立；控制读取不直接将全部 Rule 标脏。

Python 的 `peek/try_peek/empty/full/size/current` 在 Work 内根据执行上下文自动登记：activeRule 存在时登记该 Rule 的读者位，否则登记 activeModule 的控制读取；Work 外观察不建立订阅。读空先登记，再返回空／抛出 NeedInput。读取不会自动提出 pop，消息消费仍由生成代码按绑定角色提出。

Pop/revise 即使没有先 peek 也登记目标依赖，已登记时以位测试去重；纯 push 不自动订阅。读取接口内部及仲裁容量检查直接检查存储字段，避免内部方法调用再次登记。显式 `record_read(mid, qid, rid)` 保留为旧代码兼容入口，共用同一底层登记逻辑。

Python 直接读取构造好的 `Q.moduleSlots[mid]`，未声明访问报错；登记实际 Rule 读者用位测试去重。Rule 读取关系持续有效，直到重算或取消，不能受 Module 控制 readGen 推进影响。

### 5.2 Queue 变化通知

全部 Queue Xfer 后，对每个变化 Queue 的可能读者链接调用带资源槽位的 wakeup 入口：

```python
for mid, slot in Q.readerLinks:
    wakeup(mid, tick + 1, changedSlot=slot)
```

wakeup 在通知当下将该资源的实际 Rule 读者位 OR 到 Module.dirtyWords；有实际 Rule 读取或有效控制读取才登记未来激活。只有实际读者被标记，不按静态可访问范围标记全部 Rule。无变化 revise 不通知；pop 后 push 相同 payload 仍是变化。多资源同时通知均须先合并 dirty，不能因已安排 Module 激活而跳过后续资源；到期 Work 按 ModuleId 去重。完整入口见第 7.5 节。

### 5.3 Module 再次调用 Rule

同 tick 首次调用登记选择并保存 callArgs；未 dirty 时按值比较参数，变化时置本 Rule dirty。已 dirty 时必定重算，跳过参数比较并在重算前保存新参数。参数变化与 Queue 变化使用同一 dirty 位，不为普通 var 建 slot。值比较必须保留 AC 类型语义；不以对象地址或二进制 padding 比较。

```python
if not dirty(R) and not sameParameters(R.candidateArgs, args):
    M.dirtyWords[R.wordIndex] |= R.bit
valid = cacheEnabled and R.complete and not dirty(R)
if valid:
    return  # 保留 proposal、事件和全部读取关系；不遍历 readSlots，不续订
clearCandidate(R)  # 仅清 proposal、participants、未发布事件和完成状态
clearReads(R)      # 遍历 readSlots，清除各资源的 R.bit
clearDirty(R)
R.candidateArgs = copyValue(args)
runRuleBody(R, args)  # 重新登记实际读取，显式 complete/abort
```

Module 控制读取的 Queue 变化先唤醒 Module；传入 Rule 的派生值不变时，Rule 可继续复用。反之 Rule 自己读过的 Queue 变化会直接置 dirty，即使调用参数相同也重算。

abort 清除部分 proposal 和未发布事件，但保留此次尝试已登记的所有读取，包括读空。完整无效果的结果同样保留订阅并可复用；只有仲裁入口检查实际效果，无效果不 firing、不更新 acceptedTick。提交后清候选而保留订阅，否则后续数据变化可能无法唤醒 Module。

Module Work 结束时，对旧选择中未再选中的 Rule 同时清候选、读者位和 dirty。控制读取独立发布新代号。Module 必要读取失败不影响此前已选中的独立 Rule。

### 5.4 容量通知直接仲裁

获准 pop 只安排唯一生产者的已有完整、未 dirty 候选仲裁，不标记 dirty、不运行 Work、不比较参数、不续订。不存在完整候选时跳过。该机制依赖全部 Module Work 先于仲裁的屏障；此前 Queue 变化通知已让相应 Module 重算或取消。仲裁入口也拒绝 dirty 候选。

本轮只支持同一 Module Work 向成员 Rule 传普通 var 参数。Module→Module var 传播、因果顺序与 delta Work 留待下一步，不引入新纯组合 Module 类型。

<a id="arbitration"></a>

## 6. Queue 资源与 Rule 原子仲裁

### 6.1 Queue 操作与接口职责

| 接口 | 语义 |
| --- | --- |
| `registerSource(id)` | 运行前登记来源和固定槽位 |
| `peek/tryPeek/empty/full/size/current` | 读 current，Work 内自动登记依赖，不消费 |
| `capacity` | 固定配置，不建立动态依赖 |
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
    if R.acceptedTick == tick or not R.complete or not hasCandidateEffects(R) or dirty(R):
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

任一许可检查返回失败都不部分确认，保留完整 proposal、wakeRequests、参数和读取关系。全部检查成功后，整体 accept 与通知之间不插入其他 Rule 仲裁，Accepted 本 tick 不撤回。

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
    return R.complete and hasCandidateEffects(R) and R.acceptedTick != tick and not dirty(R)


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
        notifyChanged(qid)  # 按实际 Rule 读者置 dirty，并唤醒有实际读取的 Module
    clearCommittedCandidates(acceptedRules)
    usedQueues.clear()  # 保留列表容量，usedTick 以 tick 区分下一轮
    ruleTasks.clear()  # 保留缓冲区，下个 tick 使用新的入队代号
    moduleWorkTasks.clear()
```

获准 pop 只通知唯一生产者，并检查其完整候选和实际 push。没有候选、不完整或已获准则跳过，不运行 Work，不复活已提交候选。任务只保存 RuleId，执行时读取当前记录；selectedTick 不要求等于当前 tick。

例如只有 C 的 Module 本 tick Work，B/A 有跨 tick 保留的完整候选，C 的 pop 可将 B 加入同一任务列表，B 的 pop 再加入 A；B/A 的 Module 不需要 Work。递归访问与通知入队共享本 tick 的访问标记，不重复仲裁。

状态变化调用 wakeup(M, tick + 1, changedSlot)，入口标记 dirty 并对有效订阅登记事件，与定时事件共用 eventQueue；不因端口重置安排仲裁。不保存 waitingQueueId 或 Queue.waiters：唯一 pop/push 来源不会被另一 Rule 占用同类端口，容量释放已在本 tick 由获准 pop 传播；其他端口竞争不在本版范围。

<a id="events"></a>

### 7.5 事件请求、登记与到期

时间以整数 tick 表示。Rule Work 将实际分支请求的 `(ModuleId, delay)` 保存到 wakeRequests，要求 delay ≥ 1。只有整条 Rule 获准时，才按获准 tick 计算 `wakeTick = tick + delay`；候选在等待期间复用，不改变这一起算点。

事件统一经调度器 wakeup 入口登记：

```python
def wakeup(moduleId, targetTick, changedSlot=None):
    # 前提：targetTick > scheduler.tick
    if changedSlot is not None:
        M = modules[moduleId]
        live = M.controlReads[changedSlot] != 0 and M.controlReads[changedSlot] == M.readGen
        for word in range(M.wordCount):
            readers = M.ruleReaders[changedSlot][word]
            M.dirtyWords[word] |= readers
            live |= readers != 0
        if not live:
            return
    eventQueue.add(targetTick, moduleId)
```

wakeup 不立即执行 Work。Queue 变化通过 changedSlot 在通知当下标记 dirty，再登记未来激活；不把失效处理推迟到事件到期。普通定时事件不带 changedSlot，只登记激活，不将 Rule 标脏。调度器可因 Queue 状态变化等原因自行调用；其他主动请求只能来自获准 Rule，Module Work 不直接调用此入口。

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

| 情况 | 候选 | 动态 Rule 读取关系与 dirty |
| --- | --- | --- |
| 缓存命中 | 完整保留 | 保留，不扫描、不续订 |
| 必要读取失败／显式 abort | 清部分 proposal、participants、未发布事件 | 保留此次尝试读取，包括空输入 |
| 参数变化／Queue 变化导致重算 | 清旧候选，执行新路径 | 清旧读者位与 dirty，登记新路径 |
| Module 不再选中 Rule | 清候选 | 清读者位与 dirty |
| 完整候选许可失败 | 保留 | 保留 |
| 完整但无效果 | 不 firing；完整且未 dirty 时可复用 | 保留读取 |
| 整体获准及 Xfer | 事件按获准 tick 发布；Xfer 后清候选 | 保留读取；自身修改也可将自己置 dirty 并唤醒下一 tick |

Module 控制读取使用独立代号，不因单条 Rule 的 abort、提交或缓存命中而清理。

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

设 A_M 为 Module 可访问资源数，R_M 为所属 Rule 数，W_M=ceil(R_M/64)。固定元数据包括每条可能 Queue→Module 链接、每 Module 的 A_M 个控制读取代号、A_M×W_M 个 Rule 读者字和 W_M 个 dirty 字。另外增加 QueueCount×ModuleCount 个槽位映射项，以空间换取读取时直接索引；变化通知仍只遍历可能读者链接。空间不因历次动态路径累计增长；实际 readSlots 仅保存最近尝试路径。静态声明很宽、Module 的 Rule 很多时，位图空间与扫描成本仍须测量。

| 操作 | 遍历范围 |
| --- | --- |
| Rule 缓存命中 | 未 dirty 时比较参数，再做 complete/dirty 固定检查；不扫描读取 |
| Rule 实际读取登记 | 固定数组直接定位槽位 O(1)，位测试去重 O(1) |
| Rule 重算／取消读取 | 上次实际读取槽位 d，逐位清除 |
| 状态变化通知 | 变化 Queue 的 K 个可能读者，每个读取其 W_M 个字 |
| 候选许可／确认／清理 | 实际 participants p，另加 Queue 局部成本 |
| DFS | 每条被访问 Rule 本 tick 至多一次，扫描实际 participants |
| 获准 pop 传播 | 唯一 pushRuleId 的候选检查 |
| 事件及 Xfer | 获准事件／到期事件、usedQueues 及其 accepted 操作 |

纯 push 的容量变化不自动成为计算依赖；显式读取 output current 时仍是普通动态依赖。Queue array 的动态下标只登记实际表项，下标来自 var 则参与参数比较，来自 Queue 则登记该 Queue。

取消候选与取消读取均不是 O(1)。位图降低缓存命中路径成本，但会增加每次读取、重算和通知的操作，不能由表示方式直接断言实际提速；[实验报告](experiment/report.md)记录 Python 测量。C++ 性能留待实际实现后验证。

<a id="validation"></a>

## 11. 验收要求与实现状态

### 11.1 核心验收场景

| 场景 | 必须验证的结果 |
| --- | --- |
| Module 与 Rule 分支 | 只准备被调用 Rule，只检查实际 participants，未选中旧候选取消 |
| 读空与不完整尝试 | 不访问无效 payload，部分 proposal 清理，后续输入变化可激活 Module |
| Module 读取集合变化 | 原来读 Q1/Q2、本次只读 Q2 后，Q1 的旧条目不再激活 Module |
| Rule 缓存命中 | 参数未变且未 dirty 时复用，读取位保持，不扫描版本或续订 |
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
| 跨 tick 候选 | Pending 可直接重试仲裁，不要求 selectedTick 等于当前 tick；Queue 变化预先标记 dirty，调用时比较参数 |
| Work 屏障与旧候选 | 先完成全部 Module Work；DFS 不提交被取消候选，替换的候选按新记录仲裁 |
| 初始激活 | tick 0 所有 Module 各 Work 一次，包括读空订阅；全部 Work 完成后才仲裁 |
| 容量通知但没有完整候选 | 跳过，不运行 Module／Rule Work，不复活已提交候选 |
| 纯事件 Rule | 无 participants 也可确认并发布事件，每 tick 至多一次；既无 proposal 又无事件请求则不 firing |
| 事件与原子确认 | 任一 Queue 检查失败不发布事件；重算、取消或读取失败清理未发布请求 |
| 延迟与跨 tick 候选 | 延迟以获准 tick 为起点；如 tick 10 计算、tick 20 获准、delay=3，则在 tick 23 激活 |
| 事件与 Work 屏障 | 仅允许未来 tick；到期事件在 Work 前处理；同 Module 同 tick 的事件及状态通知只 Work 一次 |
| 固定读者记录 | 可能读者链接固定；动态下标替换读者位；别名去重；70 条 Rule 跨字不串位；无实际读取不通知 |
| 固定任务及实际依赖 | 任务缓冲复用、proposal 槽位身份稳定；Rule readSlots 和 participants 只保存实际资源 |

子 Module、已登记事件的取消／覆盖、并行执行和跨版本 ABI 等仍有待决问题；不能仅靠上述核心场景通过宣称这些能力已经验收。单线程 C++ 源码接口见第 11.3 节。

### 11.2 Python experiment

[实验说明](experiment/README.md)给出可运行入口，[报告](experiment/report.md)记录逐拍对比与性能。

Python 已实现上述动态位图、参数 dirty、持久读取关系、固定槽位映射和基于 Work 上下文的自动读取登记；riscv 使用普通阶段类和显式核心调用。核心小电路验证多 Rule 精确失效、跨 64 位、动态数组、读空、提交／取消生命周期、缓存命中不遍历、容量重试；riscv 完整程序按独立顺序解释器验证退休结果，另比较 36 组改造前后逐拍 Queue、获准集合和事件。微架构及 ISA 范围没有扩展。槽位映射、自动读取分别测量，并通过 37 项测试及相同的 36 组逐拍对照；见 [两项改进报告](experiment/read-tracking.md)。

旧 pipeline、packets、pairs、memory、feedback、lookup、retry Python examples 及专用参考辅助代码已移除。ripes5 保留原文件，本轮未重新验收原生 Ripes 对照。Module 间 var、C++ 移植、编译器生成、层级、Cell、事件取消等未据此完成。

### 11.3 C++20 核心（待迁移）

[现有 C++ 库](cpp/README.md)及[历史报告](cpp/report.md)对应上一版设计：Queue 保存 ModuleCount 个读取代号，Rule 保存 Queue 版本依赖，缓存命中重登订阅。本轮没有修改 C++ runtime 或执行其测试。随旧 Python examples 删除，移除依赖它们的 Python/C++ 对比脚本和 CMake 注册；原生测试及 C++ examples 保留。

后续迁移需要实现局部资源表、读者／dirty 位图及本规格生命周期，重新运行原生测试和端到端对照，不能以历史 CTest 成功声称当前新方案已经在 C++ 验收。
