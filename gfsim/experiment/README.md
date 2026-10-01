# Python GFSim 调度实验

实验模拟编译器生成后的 Module 成员函数、静态表和运行记录，验证调度及记录生命周期。仅依赖 Python 3.11 标准库。

## 运行和代码入口

从仓库根目录执行：

```bash
python3 -m unittest discover -s gfsim/experiment -v
python3 gfsim/experiment/bench.py
```

- `engine.py`：约 500 行，包含纯数据记录、固定任务缓冲区、Queue 和调度引擎。
- `construction.py`：运行前构造函数表、静态关系、仲裁顺序和固定槽位。
- `models.py`：普通 Module 类，展示显式的生成代码入口和读取登记。
- `test_engine.py` / `bench.py`：定向测试、随机对比和三类性能负载。

原来使用 Rule 可调用对象的实验已保存在提交 `96242be`。

## 模拟生成的成员函数

以 `models.Move` 为例，`Work()` 直接调用 `self.work_move()`。Rule 的计算入口明确写出：

```python
def work_move(self):
    e = self.engine
    if not e.begin_rule(self.rid):
        return  # 去重或缓存命中。
    try:
        e.record_read(self.mid, self.source.qid, self.rid)
        value = self.source.peek()
        self.source.propose_pop(self.rid)
        self.target.propose_push(self.rid, value)
    except NeedInput:
        e.abort_rule(self.rid)
        return
    e.complete_rule(self.rid)

def arbitrate_move(self):
    return self.engine.arbitrate_rule(self.rid)
```

正常业务提前返回也须先调用 `complete_rule`；必要读取失败调用 `abort_rule`。引擎检查函数是否完成收尾，漏写会报错。无 proposal 的完整计算不产生 firing。Module 必要读取失败只停止其后续调用，已经选中的独立 Rule 仍可提交。

Queue 的 `peek/try_peek/empty/full/size` 是纯读。每次业务读取前，生成函数显式调用 `record_read(mid, qid, rid)`；Module 控制条件的读取省略 `rid`。读空也要登记。提出 pop/revise 时，Queue 的公共实现另外登记目标依赖和实际参与者。纯 push 不引入输出旧内容依赖。

Rule 缓存命中时仍调用成员函数并执行 `begin_rule`；其后的业务计算被跳过。缓存依赖重新合入 Module 本轮读取集合。

## 运行前的静态构造

固定 ModuleId、QueueId 从 0 开始，RuleId 从 1 开始，规则表下标 0 留空。以下代码在该目录下运行，或先将该目录加入 Python 导入路径：

```python
from construction import assemble
from engine import Queue, RuleEntry
from models import Move

queues = [Queue(initial=(7,)), Queue()]
module = Move(0, 1, queues[0], queues[1])
rules = (None, RuleEntry(0, module.work_move, module.arbitrate_move,
                         pops=(0,), pushes=(1,)))
sim = assemble(queues, [module], rules)
print(sim.step())      # (1,)：实际获准的 RuleId。
print(sim.snapshot())  # ((), (7,))
```

`RuleEntry` 是静态数据记录，绑定所属 Module、Work／仲裁成员函数及所有可能分支的修改资源。读取集合由显式生成代码在运行时登记。

`assemble` 只进行机械构造：Queue 的可能生产者、来源索引、允许操作、固定 proposal 槽位；消费先行的拓扑顺序；Module／Rule 记录数组；成员函数入口表。它不分析函数体，也不使用方法发现、装饰器或 Rule 执行包装。构造完成后才创建 Simulator，`step` 不建图或注册来源。

静态表及资源声明由测试视作编译器输出，运行期间保持不变。`Simulator` 也可以直接接收预构造的 Queue、StaticTables 和记录数组。

## 记录和调度

| 记录 | 内容及用途 |
| --- | --- |
| `ModuleRecord` | 本 tick 是否已 Work、读取代号、实际读取 QueueId 集合、本轮／上轮选中 RuleId 列表；`collecting` 标记本轮订阅尚未发布 |
| `RuleRecord` | 依赖版本、实际参与 QueueId、候选参数、本轮调用参数、选中 tick、完整性、准备时间戳、获准 tick、阻塞 QueueId 及等待位置；`executing` 检查生成函数收尾 |
| Queue | current、版本、ModuleId → 读取代号、固定 proposal 槽位、端口归属、accepted RuleId 列表、等待 RuleId 列表、used tick |

Proposal 的值只存在 Queue 槽位。RuleRecord 不保存 payload 副本。Proposal 状态为 empty → pending → reserved → accepted；失败预约回到 pending，废弃或提交后槽位原地清空，来源索引及槽位身份持续保留。

### 对齐 C++ 的记录布局

- Module／Rule 的 delta 任务使用两组交替的 `IdList`；下一 tick 的 Module 任务另有一组。每组在运行前分配固定容量 ID 数组和代号数组，使用有效长度，插入时按 ID 去重，清空只推进代号。当前批次与下一批次分别保存标记。
- `used_queues` 使用 QueueId 列表，通过 Queue 的 `used_tick` 去重。
- Module 的选择记录使用两个交替列表，RuleRecord 的 `selected_tick` 判断是否选中；本轮 `call_args` 与缓存候选的 `args` 分开，参数变化仍会使缓存失效。
- Queue 的等待者使用 RuleId 列表，RuleRecord 保存位置。删除时把最后一个等待者移到空位并更新其位置，插入和删除均为 O(1)（追加按摊销计算）；遍历通知仍为 O(等待者数量)。
- Rule 的参与 Queue 列表继续通过固定 proposal 槽位去重。

普通列表对应 C++ 的可复用 vector，Python 不模拟其容量保留。`IdList` 的底层数组身份和长度保持不变；Python 排序仍会产生临时列表。读取集合、依赖版本和 Queue 读者表暂保留 set/dict；静态构造也仍可使用集合。进一步固定读取关系需要补充编译期可能读取的元数据，不展开 Module × Queue 的稠密表。

调度顺序：

```text
取出本 delta 的 Module / RuleId 任务
  → 首次激活的 Module 执行 Work，收集实际选择和读取
  → 清理未选中的旧候选，发布新读取代号
  → 按静态顺序调用本批选中 Rule 的仲裁成员函数
  → 获准 pop 通知下一 delta
没有同 tick 任务
  → 所有 used Queue 统一 Xfer
  → 发布下一 tick 的读取变化及端口重置通知
```

同一 Module 每 tick 至多 Work 一次，Rule 每 tick 至多获准一次。后续 delta 使用静态 Work 入口和保存的参数重试，入口按版本复用候选。缓存失效时清理旧候选及等待关系后重算。

用户未写仲裁规则却产生端口竞争，视为用户模型错误；引擎不报错，也不保证哪个候选获胜。当前内部排序及激活批次可能影响获胜者，不构成用户可依赖的优先级规则。竞争仍须满足端口容量限制和整条 Rule 的原子性。

每条 Rule 逐 Queue 预约，失败则释放全部临时预约、保留完整候选，并等待第一个失败 Queue。全部成功后统一 accept，获准 pop 才公开其容量效果。下一 delta 查静态生产者；下一 tick 的读取通知只认读取代号匹配的 Module。端口重置也通知等待者，即使 Queue 值未变。

Xfer 顺序为 revise 旧队尾 → pop 旧队首 → push 新元素。数据或元素身份变化推进 Queue 版本；无变化的 revise 不推进。Pending 候选可跨 tick 保留；已提交候选清除。不逐 tick 扫描全部记录重置。

## 范围和约束

- 平级 Module、单线程、B 容量策略、单 pop／单 push／单整值 revise；允许自身 pop/push 复用满 Queue 的空间。
- 所有 Work 读取同一份 current，新 push 的数据下一 tick 才可读。静态容量环及同一旧元素的 revise/pop 组合报错。
- 值和参数限于整数、布尔值、递归不可变 tuple。Rule 业务计算只依赖登记的 Queue、参数及固定配置，使用值语义，不修改外部状态。
- 测试通过 `step(wake=(module_id, ...))` 显式驱动 tick。时间值由被显式唤醒的 Module 作为 Rule 参数传入；Rule 业务计算不直接读取时间。
- 不包含字段修改、子 Module、定时事件、并行执行或编译器接入。模型错误终止运行，不支持异常后恢复。

## 验证和性能计数

28 项测试包含原有预期结果测试，以及同一 Module 的多个 Rule、相同成员函数代码的不同实例、固定槽位身份、纯读接口及显式入口收尾的检查。任务数组复用用例检查空 tick 后重复唤醒仍只执行一次；等待者用例检查中间删除、位置更新和后续取消／获准。跨 delta 端口竞争用例只检查合法提交和原子性，不要求特定获胜者。随机测试用 12 个固定种子，各运行 100 tick，逐 tick 比较获准序列、Queue 状态、版本、实际读取集合和下一 tick 激活集合。

另有独立的 pop/push 原子提交模型：用固定种子生成 300 组随机连接，各运行 10 tick，全量激活 Module，用整组约束判断和函数式状态更新比较获准序列、Queue 状态及版本；不调用引擎的预约、回滚或 Xfer 实现。它覆盖空输入、多个参与 Queue、端口竞争、容量依赖和跨 tick 候选缓存，不覆盖 revise、选择性唤醒或全局优先级。

`reference=True` 使用同样的成员函数入口和仲裁顺序，关闭候选缓存，扫描正向记录寻找通知对象。它与索引模式共享预约和提交代码；这些共享部分由定向预期测试检查。

| 计数 | 含义 |
| --- | --- |
| `module_work` / `rule_entries` / `rule_work` | Module Work 次数 / Rule 进入 begin_rule 次数（含去重、缓存命中）/ Rule 实际业务计算次数 |
| `cache_hits` / `cache_invalidations` / `version_checks` | 候选复用次数 / 参数或依赖失效次数 / 实际版本比较次数 |
| `reader_checks` / `producer_checks` / `waiter_checks` | 索引通知遍历的条目数 |
| `module_scans` / `rule_scans` | 参考通知查询遍历的对象数 |
| `arbitrations` / `reservation_checks` / `reservations` | Rule 仲裁次数 / Queue 预约检查次数 / 成功预约次数 |
| `accepted` / `deltas` / `ticks` | 获准次数 / 有任务的 delta 批次数 / 推进 tick 数 |
| `incomplete` / `module_incomplete` | Rule / Module 必要读取不足次数 |

性能参数：`--ticks 200 --size 32 --idle 1000 --work 2000 --repeat 3`。初次订阅前的构造阶段及首 tick 不计入耗时，重复运行取中位数。每组实验检查两种模式的获准序列和最终状态一致。

三类负载分别测量满流水链、每 20 tick 解除一次输出背压的较重计算，以及 1000 个休眠 Module 配少量活动 Queue。背压负载让 Module 读取不断变化的控制 Queue，但 Rule 的输入在等待期间保持稳定，测量计算复用。

2026-09-30 固定任务数组版本，本机 Python 3.11.16、上述默认参数的一次结果（三次运行取中位数）：

| 负载 | 索引模式 | 参考模式 | 参考 / 索引 |
| --- | --- | --- | --- |
| 32 级满流水链 | 144.55 ms | 197.00 ms | 1.36 |
| 长期背压，计算循环 2000 次 | 13.52 ms | 55.35 ms | 4.09 |
| 1000 个休眠 Module | 11.80 ms | 148.35 ms | 12.57 |

背压负载两种模式都进入 Rule 函数 410 次，实际计算为索引模式 220 次、参考模式 410 次，缓存命中 190 次。稀疏负载读取通知从扫描 400800 个 Module 降到检查 400 个读者条目；两种模式的计算和获准次数一致。

耗时仅描述这些 Python 负载，不设速度门槛，不据此推导 C++ 的运行性能。
