# Module 读取记录与 Rule proposal 版本匹配（临时决策）

## Module 读取记录

Module Work 及它选中调用的 Rule Work，凡读取 Queue 的 `current`，都登记为该 Module 的读取依赖。读空的 `tryPeek()` 也要登记；子 Module 内的 Rule 则登记到子 Module 自己名下。

对应 Queue 的本轮读取标记使用 `Module.readGen`，不用 tick：Module 隔几拍没运行，订阅仍须有效。

读取时可先把 `(QueueId, ModuleId)` 收集到本轮记录，Work 完成后统一发布新集合或新代号。并行 Work 时也避免直接争写 Queue 的读者表。

Module 重新 Work 后，旧的读取集合由这次实际读取的集合替换。例如原来读了 Q1、Q2，这次只读 Q2，之后 Q1 的变化就不该再唤醒它。

可以用 Module 的读取代号做懒失效：

```text
Module.readGen = 8
Q1.readers[M] = 7   // 旧记录，忽略
Q2.readers[M] = 8   // 当前有效
```

每次 Work 完成后，把实际读过的 Queue 条目写为新代号，最后发布 Module 的 `readGen`。若复用了缓存、没有重新 Work，就不推进代号，原订阅仍然有效。

Queue 通知时，只认条目中保存的读取代号与该 Module 当前已发布的读取代号一致的记录：

```cpp
for (auto [moduleId, readGen] : Q1.readers) {
    if (readGen == modules[moduleId].readGen)
        wakeup(moduleId);
}
```

`Module.readGen` 判断读取关系是否仍有效；`Queue.stateVersion` 判断缓存的计算是否仍有效。

懒失效会在 Queue 中留下旧条目，Queue array 的动态下标可能积累很多旧记录。也可以拿新旧读取集合做差集，直接删除旧关系。两种办法语义相同，先用差集更容易检查正确性。

## Rule proposal 的版本匹配

每个 Queue 保存 `stateVersion`。Rule Work 时，记录实际读取的 Queue，以及 pop/revise 目标的版本：

```python
rule.deps[q] = q.stateVersion
```

Rule 再次被调用时，检查记录的版本是否全部与 Queue 当前版本匹配。全部匹配则复用已有 proposal；没有候选或任一版本不匹配，则清理旧候选涉及的全部 Queue，重新 Work，并记录新的依赖版本。

```python
if rule.has_candidate and all(
    q.stateVersion == version
    for q, version in rule.deps.items()
):
    reuse_proposal()
else:
    discard_old_proposal()  # 清理旧候选涉及的全部 Queue
    rule.deps.clear()
    rule.Work()            # 重新计算 proposal，并记录新的依赖版本
```
