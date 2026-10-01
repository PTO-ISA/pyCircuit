# Module 读取订阅与 Rule 版本匹配

本文记录当前采用的简化决策。完整记录见 [scheduler-records.md](scheduler-records.md)，同 tick 仲裁见 [schedule-B.md](schedule-B.md)。

## Module：只保存读取代号

Module Work 及它选中调用的 Rule，凡读取 Queue 的 current，都登记为该 Module 的依赖。读空也登记；子 Module 的读取登记到子 Module 自己名下。

单线程 Work 期间不提交状态、不发布状态变化通知，因此可直接写 Queue 的读者条目，不要求 Module 保存正向读取列表：

```cpp
// 开始 Module Work。
auto newReadGen = module.readGen + 1;

// Module 或选中 Rule 实际读取 Q。
Q.readers[moduleId] = newReadGen;

// Module Work 完成后发布。
module.readGen = newReadGen;
```

读取代号不是 tick。Module 隔几拍没 Work，订阅仍然有效。重新 Work 后，本轮未登记的旧条目因代号不匹配而失效。并行实现需要保证订阅写入安全，不能无保护地共同扩展读者容器。

所有 Queue Xfer 完成后，对发生状态变化的 Q：

```cpp
for (auto [moduleId, savedGen] : Q.readers) {
    if (savedGen == modules[moduleId].readGen)
        enqueueNextTick(moduleId); // 去重
}
```

懒失效不会自动回收旧条目，可在通知或后续清理中删除。若要求立即回收，Module 需要额外保存可遍历的订阅关系。Queue array 的读者条目按需建立，不分配 Module × Queue 的稠密表。

## Rule：实际依赖列表和参数比较

Rule 保存 `deps = [(QueueId, stateVersion), ...]`，只记录实际读取项及 pop/revise 目标，按 QueueId 去重。纯 push 不引入输出旧状态依赖。Queue 状态或元素身份变化才推进版本，端口重置和 tick 推进不改变版本。

Module 本 tick 首次调用 Rule 时：

```text
完整且有 proposal 的候选存在
且调用参数相同
且所有 deps 版本匹配
    → 复用候选
否则
    → 清理旧候选、waitingQueueId、deps 和 participants
    → 重新 Work，记录新依赖和 proposal
```

Module 读取控制状态并向 Rule 传参时，该状态不一定是 Rule 的读取依赖，因此不能省略参数比较。Rule 业务计算只依赖登记的 Queue、调用参数和固定配置；时间或其他可变外部值须通过明确的输入或参数表达。

缓存命中时仍遍历 deps，把对应 Queue 的 Module 订阅登记到本轮 newReadGen。必要输入失败时清理未完成 proposal，但保留已登记的 Module 订阅。

不增加 Rule.ruleGen、Rule.dirty 或 Queue → Rule 的读取订阅。

## 同 tick：已有完整候选直接仲裁

所有 Work 看到相同 current，Module 每 tick 至多 Work 一次，选择和参数保持不变。后续 delta 不重跑 Rule Work，也不再次比较参数或版本，只对已选中、完整且尚未获准的候选重试仲裁。

若 Module 本 tick 尚未 Work，容量通知先激活它执行一次 Work。候选不完整时不能因新 push 或容量变化同 tick 补齐数据。
