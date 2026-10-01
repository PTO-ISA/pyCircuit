# GFSim 完整记录方案

目前推演出的完整方案是：**Module 管选择和激活；Rule 管候选计算与原子仲裁；Queue 保存状态、proposal 和反向通知关系。**

## 1. 运行前的静态记录

由编译器生成：

| 静态记录 | 用途 |
|---|---|
| ModuleId → Module Work | 激活后的执行入口 |
| RuleId → 所属 Module、Work、仲裁入口 | 定位 Rule 成员函数 |
| Rule 可能修改的 Queue、允许操作 | 建立 proposal 来源槽位 |
| Queue → 可能向它 push 的 Rule | 获准 pop 后通知上游 |
| 消费先行的仲裁顺序 | 支持同 tick 复用获准 pop 释放的容量 |

静态记录包含所有可能分支；运行时只记录本次实际走到的路径。

## 2. Module 保存什么

| 记录 | 用途 |
|---|---|
| `workedTick` | 同 tick 至多执行一次 Module Work |
| `readGen` | 已发布的读取订阅代号 |
| 本轮选中的 RuleId 列表 | 后续 delta 可以重试哪些 Rule |
| 上轮选中的 RuleId 列表 | 清理本轮没有再选中的旧候选 |

调用参数可以放在固定的 RuleRecord 中。

**Module 不必保存读过的 Queue 列表。** Work 期间准备 `newReadGen`，直接把订阅写到实际读取的 Queue；结束后发布它。

Module 没有重新 Work，就不推进 `readGen`。

## 3. Rule 保存什么

Rule 仍然是 Module 的成员函数，下面都是引擎维护的数据记录。

| 记录 | 用途 |
|---|---|
| `deps: vector<QueueId, stateVersion>` | 判断旧计算是否有效 |
| `participants: vector<QueueId>` | 找到实际 proposal，进行预约、释放、取消 |
| 候选参数、本轮调用参数 | 判断参数变化是否使缓存失效 |
| `complete` | 候选是否计算完整 |
| `selectedTick` | 本 tick 是否被 Module 选中 |
| `preparedEpoch` | 同一 tick／delta 避免重复准备 |
| `acceptedTick` | 同 tick 至多获准一次 |
| 阻塞 QueueId、等待位置 | 登记和删除等待关系 |

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
| 等待 RuleId 列表 | 端口释放后通知等待者 |
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

**只有整体获准的 pop 才释放同 tick 容量。** 它沿静态生产者关系安排下一 delta；已经 Work 的 Module 只重试选中的 Rule。

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

端口释放另外通知等待 Rule 所属的 Module，即使值未变化。

已提交的 Rule 候选清空；未获准的完整候选保留。

## 6. Queue array 的存储原则

**记录量按实际关系增长，不按整张表展开：**

- Rule 的依赖列表只记录实际读取的表项。
- Rule 的参与列表只记录实际提出 proposal 的表项。
- Queue 的反向读者记录按需建立，可以放在 Queue array 的独立稀疏表中。
- 不分配完整的 Module × Queue 读取矩阵。

懒失效的旧读者记录需要后续清理。若要求立即回收，Module 就需要额外保存可遍历的订阅关系。

最后区分一下实现进度：**上述是最新推演方案；当前 experiment 仍保留 `Module.reads` 和字典形式的 Rule 依赖，尚未改成这里的读取记录布局。**
