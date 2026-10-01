# GFSim 完整记录方案

当前采用的简化方案是：**跨 tick 按 Module 读取订阅激活、按 Rule 参数和依赖版本判断复用；同 tick 按获准 pop 传播容量，已有候选直接重试仲裁。**

Module 管选择和激活；Rule 管候选计算与原子仲裁；Queue 保存状态、proposal 和 Module 读取订阅。只保留 Queue → Module 的动态读取关系，不增加 Rule 的 `ruleGen`、`dirty` 或 Queue → Rule 的读取订阅。

## 1. 运行前的静态记录

由编译器生成：

| 静态记录 | 用途 |
|---|---|
| ModuleId → Module Work | 激活后的执行入口 |
| RuleId → 所属 Module、Work、仲裁入口 | 定位 Rule 成员函数 |
| Rule 可能修改的 Queue、允许操作 | 建立 proposal 来源槽位 |
| Queue → 可能向它提出 proposal 的 Rule（`sources`） | 端口重置后查找阻塞在它上面的 Rule |
| Queue → 可能向它 push 的 Rule | 获准 pop 后通知上游 |
| 消费先行的仲裁顺序 | 支持同 tick 复用获准 pop 释放的容量 |

静态记录包含所有可能分支；运行时只记录本次实际走到的路径。

## 2. Module 保存什么

| 记录 | 用途 |
|---|---|
| `workedTick` | 同 tick 至多执行一次 Module Work |
| `readGen` | 已发布的读取订阅代号 |
| 本轮选中的 RuleId 列表 | 后续 delta 可以重试哪些 Rule 的仲裁 |
| 上轮选中的 RuleId 列表 | 清理本轮没有再选中的旧候选 |

调用参数可以放在固定的 RuleRecord 中。

**Module 不必保存读过的 Queue 列表。** Work 期间准备 `newReadGen`，直接把订阅写到实际读取的 Queue；结束后发布它。

Module 没有重新 Work，就不推进 `readGen`。

## 3. Rule 保存什么

Rule 仍然是 Module 的成员函数，下面都是引擎维护的数据记录。

| 记录 | 用途 |
|---|---|
| `deps: vector<ReadDep>` | 保存实际 QueueId 和 stateVersion，判断旧计算是否有效 |
| `participants: vector<QueueId>` | 找到实际 proposal，进行预约、释放、取消 |
| 候选参数、本轮调用参数 | 判断参数变化是否使缓存失效 |
| `complete` | 候选是否计算完整 |
| `selectedTick` | 本 tick 是否被 Module 选中 |
| 本批任务入队标记 | 同一 delta 避免重复仲裁；容量通知不重新准备候选 |
| `acceptedTick` | 同 tick 至多获准一次 |
| `waitingQueueId` | 保存第一个预约失败的 Queue；重新仲裁或取消时清空 |

两份 vector 含义不同：

```text
deps：
    实际读过的 Queue，以及 pop/revise 目标

participants：
    实际提出 proposal 的 Queue
```

只读 Queue 可以只在 `deps` 中。纯 push 的输出 Queue 可以只在 `participants` 中。

**Proposal 的值存在 Queue，Rule 不复制一份。**

## 4. Queue 保存什么

| 记录 | 用途 |
|---|---|
| `current` | 持久状态 |
| `stateVersion` | 状态或元素身份变化时推进 |
| 读者条目 `[ModuleId, readGen]` | 状态变化后查找应激活的 Module |
| 按 RuleId 区分的 proposal 槽位 | 保存 pop／push／revise 操作和值 |
| 端口归属 | 防止超出本 tick 的端口限制 |
| 已获准来源列表 | tick 末 Xfer |
| 静态可能来源 `sources` | 端口释放后筛选 `waitingQueueId` 匹配的 Rule，不维护等待列表 |
| `usedTick` | 去重登记本 tick 使用过的 Queue |

Proposal 状态：

```text
empty → pending → reserved → accepted
                   │
                   └─ 预约失败释放 → pending
```

- 取消候选：清空对应槽位。
- 提交完成：清空 accepted 槽位。
- 完整但未获准的 pending：可以跨 tick 保留。

## 5. 整个生命周期

### ① Module 开始 Work

```cpp
newReadGen = module.readGen + 1;
```

重新执行控制流，确定本轮调用哪些 Rule。

### ② 实际读取 Queue

Module 和其 Rule 的读取都登记：

```cpp
Q.readers[moduleId] = newReadGen;
```

Rule 内的读取另外登记：

```cpp
rule.deps.record(Q.id, Q.stateVersion);  // 去重
```

读空也登记。必要读取失败时，清理未完成候选，但已经登记的 Module 订阅保留。

### ③ 被调用的 Rule 判断缓存

```text
候选完整
且参数相同
且所有依赖版本匹配
→ 复用 proposal
```

缓存命中时，遍历 `deps`，把这些 Queue 的 Module 订阅重新登记到 `newReadGen`。

否则清理旧 proposal、等待关系和依赖，再重新计算。

调用参数必须比较：Module 可能因自己的控制 Queue 变化向同一条 Rule 传入新参数，而这条 Rule 自己读取的 Queue 都没有变化。这里只在 Module 本 tick 首次选择该 Rule 时检查；同 tick 后续 delta 不再调用 Rule Work 入口，也不再检查参数或依赖版本。

### ④ Module Work 结束

清理上轮选中、本轮未选中的 Rule，然后发布：

```cpp
module.readGen = newReadGen;
```

旧 Queue 读者条目因代号不匹配而失效。

### ⑤ Rule 原子仲裁

遍历 `participants` 预约：

- 任一失败：释放全部临时预约，保留完整候选，登记到第一个阻塞 Queue。
- 全部成功：整体 accept。

**只有整体获准的 pop 才释放同 tick 容量。** 它沿静态生产者关系安排下一 delta：

- 生产者所属 Module 尚未 Work：先执行一次 Module Work，确定实际选择。
- Module 已 Work，生产者被选中且有完整候选、尚未获准：直接重试仲裁。
- 未被选中、候选不完整或已经获准：跳过。

同 tick 的 current、选择和参数不变，不重跑 Rule 业务计算。必要输入不足的尝试不因容量通知重算；新 push 的数据下一 tick 才可读。若生产者已在当前仲裁批次中，消费先行顺序使它稍后检查容量，不重复安排下一 delta。

### ⑥ tick 末统一提交

所有 Work、仲裁结束后，各 Queue 执行：

```text
revise → pop → push
```

整个 tick 的 Work 都读取旧 `current`，不读取本 tick 新 push 的数据。

### ⑦ 提交后安排下一 tick

所有 Queue 提交完成后，状态变化的 Queue 判断：

```cpp
for (auto [moduleId, savedGen] : Q.readers) {
    if (savedGen == modules[moduleId].readGen)
        enqueueNextTick(moduleId);
}
```

端口释放另外通知等待 Rule 所属的 Module，即使值未变化：

```cpp
for (auto ruleId : Q.sources) {
    if (rules[ruleId].waitingQueueId == Q.id)
        enqueueNextTick(ownerModule(ruleId));
}
```

只保存 Rule 的 `waitingQueueId`，取消时将它清空，不维护 Queue 等待者列表和位置。扫描成本按 Q 的静态可能来源数量计算。

已提交的 Rule 候选清空；未获准的完整候选保留。

用户未写仲裁规则却产生端口竞争，属于用户模型错误；引擎不报错，也不保证哪个候选获胜。内部顺序不构成用户可依赖的优先级契约，端口限制和整条 Rule 原子性仍须保证。

## 6. Queue array 的存储原则

**记录量按实际关系增长，不按整张表展开：**

- Rule 的依赖列表只记录实际读取的表项。
- Rule 的参与列表只记录实际提出 proposal 的表项。
- Queue 的反向读者记录按需建立，可以放在 Queue array 的独立稀疏表中。
- 不分配完整的 Module × Queue 读取矩阵。

懒失效的旧读者记录需要后续清理。若要求立即回收，Module 就需要额外保存可遍历的订阅关系。

## 7. 记录容器和实现进度

Module／Rule 的任务用固定容量 ID 数组、有效长度和入队标记；Module 的选择及 Rule 的依赖、参与 Queue 用可复用 vector。订阅按实际关系稀疏建立，不因 Queue array 的规模分配整张读取矩阵。

当前 experiment 已使用固定任务数组和 Rule 参数／版本比较，但仍保留 `Module.reads`、字典形式的 Rule 依赖、Queue 等待者列表和位置；后续 delta 仍进入 Rule Work 的缓存保护入口。这些尚未改成本文的简化布局和直接仲裁流程，本次更新只修改设计文档。
