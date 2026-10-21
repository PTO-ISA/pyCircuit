# [MLIR] 支持同级模块 Rule/Method 调用与 after 顺序传播

类型：设计提案；主负责层：MLIR，涉及前端、IR 和 Link。状态：已提交 [Issue #347](https://github.com/PTO-ISA/pyCircuit/issues/347)，本地候选正在实现与独立验收。远端版本关联 #324、#319，并用结果摘要替代本地证据链接。

## 当前候选与验收入口

当前验收预期：既有 Counter 通过 `counter: Counter` 传给 Writer/Reader；
声明的 `increment -> read` 按实际实例提升到外层事务，首拍读到 6。
[目标示例](../../examples/sibling_calls/README.md) 保留初值 5、上限 7，
四拍预期为 6、7、7、7。[原始对照复现](../after_call_order/README.md)
的两个入口现在使用相同独立预期；本候选工具链运行该对照复现通过，
两个入口的 C++ workers 1/2 与生成 RTL 均得到 6、7、7、7。

`tests/system/test_sibling_calls_and_order.py` 负责独立源单元接口编译、
别名和嵌套传参、声明传递闭包、条件调用静态边、重复调用与写冲突、
整事务候选/写意图、返回可用性、可移植 anchor 重载、四态和失败零提交。
新语义使用源事务摘要 schema 2 和 routed contract 2；旧 contract 1 不兼容。
2026-10-21 本候选针对性矩阵 **32 项全部通过**（pytest 四个并行 worker，
66.69 秒），包括全部生成 C++ workers 1/2 与 Icarus 执行、直接与保存 anchor
的结果可用性、四态、reset/hold/discard、失败零提交和元数据篡改。
新增无 writer、无 guarded 结果的最小返回用例：Counter.old() 经 Reader Rule
返回后，在删除三个源单元与 Python 源码、迁移并重载 anchor 后，六次观测均为 5。
组合结果反馈环和跨资源条件调用静态环均得到依赖环拒绝，并保护已有输出。
完整结果位于本地 `docs/gates/logs/issue347/targeted-sibling-accepted.json` 和同名日志，
产物保留在 `targeted-accepted-artifacts/`；该运行记录确认测试内容及实际工具二进制
哈希在运行前后完全一致。此前失败记录另行保留，不覆盖为通过记录。

## 历史问题与影响（878f17 基线）

1. **无法直接调用同级模块的方法。** 当前可通过子实例调用，但不支持将已有实例传给另一个模块，例如 `Sender(receiver)`，再在 Sender 的 Rule 中调用 `receiver.put(value)`。
2. **被调用方法的顺序不传播。** 当前 `after` 作用于本地外层 Rule 注册；组件内部声明的 `increment → read` 不会自动建立调用者之间的顺序。已有复现中，只有内部 `after` 时读到旧值 5，外层补写 `after` 才读到候选值 6。

这使组件无法独立封装操作顺序，调用者需要重复协调，难以组合跨模块的原子操作。

## 目标写法

以下是本次实现和验收使用的目标语法；878f17 基线不能完整运行：

```python
@ac.module
def Counter():
    count: ac.u8 = 5

    @ac.method
    def increment() -> None:
        count = count + 1

    @ac.method(after=(increment,))
    def read() -> ac.u8:
        return count


@ac.module
def Writer(counter: Counter):
    @ac.rule
    def step() -> None:
        counter.increment()
    step()


@ac.module
def Reader(counter: Counter):
    @ac.rule
    def step() -> None:
        value = counter.read()
        ac.log("info", "count", value)
    step()


@ac.system
def Demo():
    counter = Counter()
    writer = Writer(counter)
    reader = Reader(counter)
```

三个实例同级。`counter: Counter` 表示编译期绑定的既有实例引用，不复制状态；普通 `ac.u8` 参数仍传递数据。编译器根据 `increment → read` 推导 `writer.step → reader.step`，Reader 首拍应读到 6，无需再次声明顺序。

## 实现要求

1. **静态实例引用。** 扩展模块参数与调用解析，保留真实实例身份；MLIR 自动生成跨层参数、结果和事务控制连线。被调用方的更新属于调用者的原子事务。
2. **接口保留顺序。** 将 Rule/Method 声明的 `after` 保存在源 IR 和发布接口中，独立编译、链接后仍可恢复并验证。
3. **顺序传播。** 先求方法声明的传递关系，再按实际实例与调用位置提升到外层事务。`a → b → c` 即使没有调用 b，也保留 `a → c`；不同实例互不混淆。
4. **统一前递与验证。** 顺序作用于整个事务涉及的状态；后继的 guard 和计算均读取正确的候选值。复用候选数据 D 与有效写使能 E，通过 `E ? D : 旧值` 实现前递，再由同一最终 IR 生成 C++ 和 RTL。

前递必须由**整个调用者事务的 fire 和对应状态的写意图**控制：任一必需调用阻塞，都不能泄漏局部更新。前驱正常不 fire 时，后继仍可使用最近有效候选值或旧值；assert 失败仍遵循全系统零提交，不改成 guard。

首版限静态实例、同一时钟域和可证明无环的顺序。条件分支中的可能调用也参与排序；内部调用逆序、跨资源顺序冲突和组合依赖环明确报错。两个事务调用同一个 `add` 不会自动获得顺序，仍须满足写冲突检查；不增加隐式仲裁或额外周期。

实现与验收按上述四个边界独立检查，不以源码能捕获代替执行证据。

## 验收与已有证据

- 同级调用保持唯一状态与跨模块原子性；别名、不同实例、嵌套调用正确区分。
- 内部 `after` 自动传播；验证后继 guard、传递顺序、前驱晚期阻塞时不泄漏更新。
- 无序多写、逆序和循环依赖拒绝；reset、hold、Discard、四态及失败零提交保持。
- 独立源单元编译和显式链接通过；同一最终 IR 的 C++ workers 1/2 与 RTL、生成的 System 行为一致。

已有顺序传播缺口的源码、运行结果与复现命令见 [after_call_order](../after_call_order/README.md)；无序同方法多写的历史拒绝保留在原 main 的 `issues/repeated_method_calls/README.md`。参考合同：[Rule authoring](../../docs/development/agent-frontend-guide.md#rule-authoring-contract)。

## 保留的 878f17 历史记录

最初提案和缺口证据基于 `878f17ddb1308a49368023167787c165245c4562`，
含此前的 SmallVector 构建修复。当时只整理提案，未修改编译器、未运行新方案；
当时的验收项均未完成。那次 `CalleeAfterOnly` 读数为 5、6、7、7，
`CallerAfter` 为 6、7、7、7。此段描述历史结果，不是当前候选的预期或通过证据。
原 main 中的 `issues/` 与生成记录保留不动；本工作树没有复制历史 generated 目录。
