# ACPy 前端语义

本文描述当前已经实现的前端写法，编译命令和支持边界见 [编译器 README](../pycircuit/README.md)。后续重构目标见 [ACIR 基础设计](../acir/basic-design.md)；其中删除 Queue revise 和原生结构操作等变更尚未全部落实。Python 只作为静态 AST 输入，不执行任意宿主代码。

`@ac.module def` 的函数体声明资源、实例、组合值并选择 rule。构造调用绑定具体实例，`m.state` 和 `m.method(...)` 分别访问状态和公开 method，`m.outputs` 访问固定返回结构。声明的实例通过本地 import/from-import/别名共享；重新构造获得独立身份。资源身份与读取所得 SSA 值分开。层次路径用点分隔，保留名字中的下划线，例如 `a.b.state` 与 `a_b.state` 身份不同。helper/guard 捕获定义所属组件的环境，不捕获调用者的同名参数。

`@ac.rule def` 定义原子事务，`@ac.method def` 定义可组合操作。method 不独立发射，状态效果和 guard 归入最外层 rule。rule/method 可以捕获静态资源并直接访问外部实例状态。

`@ac.rule(guard=ready)` 和 `@ac.method(guard=ready)` 的 ready 函数接收同样的参数，返回 bool，可捕获静态状态。`guard=enabled` 也支持布尔变量：引用声明作用域当时的 SSA 定义，即使调用者有同名参数或后续重新赋值也不替换该定义，不生成额外 helper。guard 仅观察状态，不消费或修改；其计算来源、状态读取、分支及状态视图参与依赖分析。

Module 可直接调用只读 method，以及其只读 helper、嵌套 method 和 guard，无需新增装饰器。Module 准备计算不能产生 pop、push、revise 或 write，含提交效果的调用仍要求外层 Rule。共享结果的有效性和就绪要求约束使用该结果的 Rule，未使用的观察不会阻塞其它 Rule。

`@ac.method(After=("release",))` 表示同一组件的公开 `release` 必须先于当前 method（SB）。`After` 支持单个字符串或字符串 tuple/list，可与 `guard=` 同时使用。无声明且没有内部必要顺序的接口对默认 CF。用户关系与基础组件及嵌套调用推导的关系合并；不添加冲突、竞争检查或仲裁。同一 rule 可以组合多个 SB 操作，仍是一个事务。

| 端口 | 输入 | 输出 |
| --- | --- | --- |
| `ac.queue[T]` | 实际路径读取 payload 时隐式 pop | 返回 payload 时向绑定 Queue push |
| `ac.cell[T]` | 读取周期开始的旧值 | 返回 payload 时 write，下周期可见 |
| `ac.signal[T]` | 参数传递资源引用，值表达式读取当前值 | 返回 payload 时 set 已初始化的 Signal |
| `ac.var[T]` / 普通类型 | 普通 SSA 值 | Rule 不支持普通值输出；helper/method 可返回 |

Queue 输入仅查询状态不消费；重复和别名读取按最外层事务、实际资源身份只消费一次。未执行分支不消费。消费性角色沿 helper/method 参数传播；普通 method Queue 参数和直接捕获资源默认观察。guard 始终观察。`ac.ref[T]` 声明 rule 的观察性 Queue 参数。

混合返回用 `tuple[...]` 注解并逐位置绑定。Queue/Cell/Signal 返回 None 省略该位置效果，其他效果仍属于同一事务。未注解输出按 Queue 推导，Rule 的普通值返回注解被拒绝。Cell 和 Signal 输出必须绑定已声明且有初值的对应资源；资源不能由运行时分支创建。

```python
from pycircuit import ac

@ac.module
def Counter():
    count = ac.cell[ac.u32](initial=0)

    @ac.method
    def add(value: ac.u32) -> ac.u32:
        count.value = count.value + value
        return value

    return count

@ac.module
def Top():
    counter = Counter()
    incoming = ac.queue[ac.u32](initial=7)
    outgoing = ac.queue[ac.u32]()

    @ac.rule
    def transfer(message: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return counter.add(message.value)

    outgoing = transfer(incoming)
    return outgoing
```

这条 transfer 事务同时消费 incoming、写 counter.count、产生 outgoing。method 返回普通 payload，不创建独立事务。


`ac.signal[T](initial=...)` 声明跨 tick 保持值的状态对象，必须提供初值，不接受 capacity。赋值左侧若已经是 Signal，表示写入该对象并保持身份；普通值表达式中的 Signal 表示读取，新的局部变量接收 payload SSA。`target = source` 读取源 Signal 并写入目标 Signal，不把两个对象合并。传给 `ac.signal[T]` 参数或作为 Module 返回连线时保留资源引用。

```python
@ac.module
def Source():
    result = ac.signal[ac.u32](initial=0)
    @ac.rule
    def update(out: ac.signal[ac.u32], value: ac.u32):
        out = value
        saved = out  # 同 Rule 前序临时写入可见。
    update(result, 7)
    return result

@ac.module
def Reader(source: ac.signal[ac.u32]):
    saved = source  # 普通 payload SSA，可用于 guard、参数和共享计算。
```

底层契约为 `set SB get`，前端无需显式调用 get/set。写入只允许属于 Rule 事务（包括其 helper/method）；Module 外层允许读取及组合计算。整个 Rule 获准后才发布写入，失败时保留当前值；同 Rule 的前序临时 set 对后序 get 可见，最终一起提交或取消。跨 Rule 读者等待写者完成判断，无需写者成功。Signal 读取没有 writer-fire 准入要求，没有 pop、容量检查或 EHR 端口；读取自身的路径和 method guard 仍保留。

`-> ac.signal[T]` 返回 payload 并写入预先声明的 Signal；`result = update(...)` 的调用结果绑定只恢复资源引用，不产生 Module 写入。普通计算使用 SSA，跨 tick 保存最新值使用 Signal，需要逐条传递并消费消息时使用 Queue。Rule-var 结果提取及 `var.validity` 已移除，普通参数、helper/method 返回值和 Module 共享 SSA 继续支持。完整示例见 [signal_link.py](../pycircuit/examples/signal_link.py)。

`ac.cell[T](initial=...)` 声明旧值寄存器；read 与 write 默认 CF，赋值下周期生效。`ac.queue[T](capacity=..., initial=...)` 声明 Pipeline FIFO；read/pop/revise 要求旧状态非空，`.value` 只观察旧队头，新 push 的数据下拍才可读。赋值保留旧队尾 revise。push 在旧状态未满或本拍成功 pop 释放容量时可用。默认顺序为 `read SB revise SB pop SB push`，并计算闭包；empty/size 查询旧状态；full 查询内部 EHR 的晚端口容量视图。

`ac.ehr[T](initial=...)` 声明可按 method 阶段旁路的状态，通过 `.value` 读写。它复用 Cell 引用类型，不接受用户端口编号。前序端口的最后一次有效写入对后序端口可见；没有有效前序写入时返回周期开始的旧值，同端口写入对读取不可见。普通 Cell 保持旧值语义。

阶段按具体 EHR 实例和 method 关系闭包分配连续端口。影响读写可见性的明确先后关系要求不同端口，未规定关系的访问保持同阶段；纯读取之间的排序不增加可见性。无法同时满足同阶段和先后约束时，编译错误列出资源、method 和来源。条件和事务是否获准不改变编号。helper 继承所属 method 的阶段，嵌套 method 保留自己的阶段；Rule 和 Module 准备计算中的直接访问使用端口 0。

跨 Rule 旁路要求写入所在的整个事务获准。单条 Rule 内，前序 method 的临时写入可供后序读取，最终整个事务一起提交或取消。读后阶段 EHR 不可以参与旧 empty/full 排斥证明。

Queue 的 `full()` 和 push 隐式 guard 使用同一容量 EHR 晚端口。容量视图由旧数量初始化，有效 pop 在早端口写 false；它不是独立提交的 bool 寄存器，拍末 Queue 数量仍由实际 pop/push 效果计算。payload 的 read→revise→pop 保持旧数据顺序，empty/size 也保持旧状态；空 Queue 不从本拍 push 获得新数据。

Module 的条件、参数和共享表达式保留 SSA/CFG，状态读取不移出实际分支。外层 if 限定各 rule 的选择，每个 rule 仍独立成事务。互斥位置可合并同一 rule 实例，保留参数选择；运行时重复选择和递归不支持。

编译器将选择条件、guard、参数和捕获值所需的准备区域关联到 Rule，保留分支、循环与 method 状态视图边界。同一 SSA 定义的共享保留来源和使用者；独立调用不自动合并。派生标记不成为事务节点，也不提供运行时缓存或调度能力，目标见 [编译阶段的设计意图](../pycircuit/docs/compiler-intent.md)。

固定一维 Queue 阵列沿用 `ac.array(ac.queue[T], shape=(N,), ...)` 或固定 Queue 列表。常量下标精确识别，动态下标保守覆盖可能元素。普通 `for ... in range(...)` 保存 CFG；构造资源的常量循环在静态阶段展开。定宽类型、结构体和 payload 数组保持原有表达能力。

当前仅实现静态编译和依赖分析，不提供运行时激活、仲裁或执行后端。
