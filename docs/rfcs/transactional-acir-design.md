# 事务级 ACIR 设计：从前端例子到链接与编译目标

记录日期：2026-10-26。

本文记录此前讨论确定的 ACIR 设计，补充[前端简化语义](frontend-simplification.md)，不替换该文档。AC 是 **Architecture Concept** 的缩写；ACIR 的主要表达层级是事务，而不是已经展开的寄存器引脚和使能网络。

下面的 Python 是目标写法，MLIR 是便于讨论的示意语法，不是当前编译器生成物或可以直接执行的测试。操作的具体拼写、常量和类型标注有所简化；实际支持与执行证据另见[迁移状态](../development/transactional-frontend-status.md)，实施安排见[实现计划](../development/transactional-frontend-replacement-plan.md)。

## 1. 编译过程

```text
Python 薄捕获
  → 未链接 ACIR：保留 Module、Rule、Method、组合计算、资源和调用
  → link：绑定实例与端口，具体化资源与方法，闭合静态连接图
  → 已链接事务级 ACIR
  → 公共依赖、fire、更新与冲突分析
      ├─ 直接生成新 gfsim
      └─ 目标 lowering → 尝试适配现有 C++ / RTL 后端
```

ACIR 尽量保留前端的结构与含义。link 负责确定“连到谁、是什么组件、调用什么方法”，不要求同时消除全部 Rule/Method，或展开为物理引脚。

允许模块分开编译。因此，**未链接 ACIR 可以带有待定的资源和模块引用；link 完成后的实例连接图必须确定。** 待定不是运行时动态绑定。

## 2. 完整例子：加一后，每两个样本输出一次和

这个例子同时包含外部 Rule、内部 Rule、autorule、三类 Method、平级实例调用、组合逻辑，以及 Queue/State 两类输出。

```python
import pycircuit as ac


@ac.rule
def add_one(src: ac.u16) -> ac.u16:
    x = src
    return x + 1


@ac.module
def BiasStage(inp: ac.u16) -> ac.u16:
    out = add_one(inp)
    return out


@ac.module
def Accumulator():
    total: ac.u16 = 0
    samples: ac.u8 = 0
    can_add = samples < 2
    complete = samples == 2

    @ac.method
    def value() -> ac.u16:                 # Value：读取同拍值
        return total

    @ac.method(guard=lambda: can_add)
    def add(x: ac.u16) -> None:            # Action：提出更新
        total = total + x
        samples = samples + 1

    @ac.method(guard=lambda: complete)
    def take() -> ac.u16:                  # ActionValue：返回值并提出更新
        result = value()                  # 同一实例内嵌套调用 Method
        total = 0
        samples = 0
        return result


@ac.module
def Controller(inp: ac.u16, enabled: ac.u1,
               acc: ac.module) -> tuple[ac.u16, ac.u16]:
    accepted: ac.u8 = 0
    latest: ac.u16 = 0
    next_accepted = accepted + 1           # 组合值，不创建存储

    @ac.autorule(guard=lambda: enabled and accepted < 2)
    def collect() -> None:
        x = inp
        acc.add(x)
        accepted = next_accepted

    @ac.rule
    def emit() -> tuple[ac.u16, ac.u16]:
        result = acc.take()
        accepted = 0
        return result, result

    if accepted == 2:
        out, latest = emit()              # 分别写输出 Queue 和已有 State
    return out, latest                    # 导出资源，不再次写入


@ac.module
def Pipeline(inp: ac.u16) -> tuple[ac.u16, ac.u16]:
    enabled: ac.u1 = 1

    @ac.method
    def set_enabled(value: ac.u1) -> None:
        enabled = value

    acc: Accumulator
    controller: Controller
    middle = BiasStage(inp)
    out, latest = controller(middle, enabled, acc)
    return out, latest
```

输入 `3、5、2、7` 经 BiasStage 变为 `4、6、3、8`，两批结果为 `10、11`。`latest` 在对应结果成功入队时一起更新；输出阻塞则保留整个批次。`enabled` 只暂停收集，不阻止已经完成的批次输出。这是语义推演，不是已运行的仿真结果。

`acc` 与 `controller` 都由 Pipeline 创建，是平级实例。传入 `acc` 只传递引用，不在 Controller 中重新创建累加器。`set_enabled` 供外层事务调用；没有调用时 enabled 保持初值 1。

## 3. ACIR 中分别记录什么

| 前端概念 | ACIR 中的含义 |
| --- | --- |
| Module | 结构、资源所有权、输入输出绑定、组合计算和 Rule 启用关系 |
| Rule | private 原子事务定义；输入输出是资源接口 |
| Rule 调用 | 在所属 Module 静态注册事务，并绑定资源和启用路径 |
| Method | public 同拍接口；参数和返回值是普通 SSA 值，效果属于调用者事务 |
| Queue / State | 有独立身份和所有者的资源对象 |
| 普通表达式 | SSA 组合值；反复使用一个 SSA 值不重复读取或消费资源 |
| guard | 与 body 分开的纯计算 region；省略表示 true |
| Module return | 导出或绑定资源，不提出事务更新 |
| Rule return | 向调用处绑定的输出资源提出写入 |
| Method return | 返回同拍计算值，可用性由调用所在事务管理 |

资源引用和数据值分开：`!ac.queue<T>`、`!ac.state<T>` 指向组件；调用得到的 `T` 是普通值。组件分配分别用 `ac.queue`、`ac.state` 示意，**对组件的操作统一用 `ac.method.call`**，包括 Queue 的 pop/push 和 State 的 read/write。

Rule 自身就是事务边界，不需要再用 `ac.transaction` 包住一次 Rule 调用。Module 的组合逻辑和控制流直接保留在 Module body 中，不额外包一层 `ac.work`。`Work` 是后端组织方式。

## 4. 未链接 ACIR：允许组件类型待定

独立编译 add_one 时，已知载荷是 u16，但不知道调用者会连接 Queue 还是 State。使用待定资源 `!ac.primitive<T>`，保留抽象 read/write：

```mlir
// 后文沿用这些类型别名；bits 保留四态值，不简化成两态整数。
!u1  = !ac.bits<1>
!u8  = !ac.bits<8>
!u16 = !ac.bits<16>
!P16 = !ac.primitive<!u16>
!Q16 = !ac.queue<!u16>
!S1  = !ac.state<!u1>
!S8  = !ac.state<!u8>
!S16 = !ac.state<!u16>

ac.rule @add_one
    ins(%src: !P16) outs(%dst: !P16)
    guard {
      ac.yield true
    }
    body {
      %x = ac.method.call %src::@read() : () -> !u16
      %result = ac.add %x, 1 : !u16
      ac.method.call %dst::@write(%result) : (!u16) -> ()
      ac.end
    }

ac.module @BiasStage
    ins(%inp: !P16) outs(%out: !P16) {
  ac.rule.call @add_one ins(%inp) outs(%out)
  ac.end
}
```

`outs(%out: ...)` 让输出资源在 body 中有名字，Rule 可以直接向它写入。这只是接口声明，不是在 BiasStage 内创建输出 Queue。

Controller 的 `acc: ac.module` 同样记录为静态模块引用以及所需方法约束：需要 `add(u16) -> ()` 和 `take() -> u16`。它不要求独立编译时就知道具体实例是 Accumulator；link 检查实际实例的方法、签名和效果。Method guard 和效果信息通过被链接的定义获得，不能因引用待定而丢弃。

独立单元保留 body、接口、资源绑定关系及方法要求。`!ac.primitive` 不是纯值：在类型未定时，read 可能最终成为 pop，不能提前当作无副作用读取消除。

## 5. link：资源具体化、分配与导出

link 先传播实际连接和显式声明，再对仍需新建的未指定连接采用默认 Queue。**默认 Queue 深度为 2**，允许 `ac.queue[T](depth=N)` 显式配置正整数深度。

| 抽象调用 | 连接 Queue 后 | 连接 State 后 |
| --- | --- | --- |
| `read()` | `pop()` | `read()` |
| `write(x)` | `push(x)` | `write(x)` |

显式写出的 `push`、`pop` 或 `add` 等方法仍须由实际对象提供，不能任意换成另一种操作。guard 和 Module 组合逻辑只能进行纯观察，不能因具体化而变成 pop；本例 enabled 最终绑定 State，guard 对它的读取是纯读取。

本例的所有权与连接为：

| 对象 | 所有者 | 连接或导出 |
| --- | --- | --- |
| 输入 Queue | 外层连接者 | Pipeline → BiasStage |
| `middle` Queue | Pipeline | BiasStage → Controller |
| 最终输出 Queue | 连接 Pipeline 的外层 Module/System | Controller → Pipeline → 外层 |
| `enabled` State | Pipeline | 作为 Controller 输入 |
| `accepted`、`latest` State | Controller | latest 经 Pipeline 导出 |
| `total`、`samples` State | Accumulator | 由 acc 的 Method 访问 |

新 Queue 在建立连接的那一层分配，不能让每个生产者自行创建自己的输出 Queue。已有资源的返回、导出和别名只保留同一个对象；尤其是 `latest`，不能在父层再分配寄存器或默认 Queue。

link 后 Pipeline 的结构可示意为：

```mlir
ac.module @Pipeline
    ins(%inp: !Q16) outs(%out: !Q16, %latest: !S16) {
  %enabled = ac.state initial(1) : !S1
  %middle = ac.queue depth(2) : !Q16
  %acc = ac.instance @Accumulator

  %bias = ac.instance @BiasStage
      ins(inp = %inp) outs(out = %middle)
  %controller = ac.instance @Controller
      ins(inp = %middle, enabled = %enabled, acc = %acc)
      outs(out = %out)
  ac.export %controller::@latest as %latest

  ac.method @set_enabled(%value: !u1) -> ()
      body {
        ac.method.call %enabled::@write(%value) : (!u1) -> ()
        ac.end
      }
  ac.end
}
```

这里 `ac.instance`、具名 ins/outs 和 `ac.export` 示意静态实例、端口绑定与资源别名，不是运行时函数调用。`latest` 从子实例导出，`out` 则绑定外层提供的 Queue。两者的来源均须在 link 后确定。

同一个 add_one 定义若在不同位置分别连接 Queue 和 State，link 可按具体绑定进行特化；不能改动共享模板，使一个调用位置错误影响另一个位置。闭合后的可执行实例不能保留未绑定的资源或方法。

## 6. 已链接 ACIR：保留 Method、Rule 和组合控制流

为便于阅读，以下沿用上面的类型别名与简写，省略默认为 true 的 guard。

Accumulator 保留自身的状态、组合表达式和三个 Method：

```mlir
ac.module @Accumulator ins() outs() {
  %total = ac.state initial(0) : !S16
  %samples = ac.state initial(0) : !S8
  %n = ac.method.call %samples::@read() : () -> !u8
  %can_add = ac.cmp ult, %n, 2 : !u8
  %complete = ac.cmp eq, %n, 2 : !u8

  ac.method @value() -> !u16
      body {
        %old = ac.method.call %total::@read() : () -> !u16
        ac.yield %old
      }

  ac.method @add(%x: !u16) -> ()
      guard { ac.yield %can_add }
      body {
        %old = ac.method.call %total::@read() : () -> !u16
        %sum = ac.add %old, %x : !u16
        %next_n = ac.add %n, 1 : !u8
        ac.method.call %total::@write(%sum) : (!u16) -> ()
        ac.method.call %samples::@write(%next_n) : (!u8) -> ()
        ac.end
      }

  ac.method @take() -> !u16
      guard { ac.yield %complete }
      body {
        %result = ac.method.call @value() : () -> !u16
        ac.method.call %total::@write(0) : (!u16) -> ()
        ac.method.call %samples::@write(0) : (!u8) -> ()
        ac.yield %result
      }
  ac.end
}
```

`@value()` 指当前 Accumulator 实例的方法。这里没有把 add/take 展开到调用者，也没有把状态变成物理 Q/D 引脚。Module 中 `%can_add`、`%complete` 是供 guard 使用的组合值，不是寄存器。

Controller 中 autorule 先规范化为普通 Rule 加一次显式注册；emit 的两个输出分别绑定 Queue 和 State：

```mlir
ac.module @Controller
    ins(%inp: !Q16, %enabled: !S1, %acc: !ac.module_ref<@Accumulator>)
    outs(%out: !Q16, %latest: !S16) {
  %accepted = ac.state initial(0) : !S8
  %latest_state = ac.state initial(0) : !S16
  ac.export %latest_state as %latest

  %a = ac.method.call %accepted::@read() : () -> !u8
  %next_accepted = ac.add %a, 1 : !u8

  ac.rule @collect ins() outs()
      guard {
        %en = ac.method.call %enabled::@read() : () -> !u1
        %room = ac.cmp ult, %a, 2 : !u8
        %allow = ac.and %en, %room : !u1
        ac.yield %allow
      }
      body {
        %x = ac.method.call %inp::@pop() : () -> !u16
        ac.method.call %acc::@add(%x) : (!u16) -> ()
        ac.method.call %accepted::@write(%next_accepted) : (!u8) -> ()
        ac.end
      }

  ac.rule @emit ins() outs(%dst: !Q16, %status: !S16)
      body {
        %result = ac.method.call %acc::@take() : () -> !u16
        ac.method.call %accepted::@write(0) : (!u8) -> ()
        ac.method.call %dst::@push(%result) : (!u16) -> ()
        ac.method.call %status::@write(%result) : (!u16) -> ()
        ac.end
      }

  ac.rule.call @collect ins() outs()
  %batch_done = ac.cmp eq, %a, 2 : !u8
  scf.if %batch_done {
    ac.rule.call @emit ins() outs(%out, %latest_state)
  }
  ac.end
}
```

内部 Rule 捕获所属 Module 的资源和组合值。`scf.if` 示意启用路径，实际条件保留四态与可用性检查。它不动态分配 Rule 或输出对象；因此 Python 中条件分支里的 `out` 绑定可以由 Module 导出。

Module 组合逻辑先按原表达保留。需要提前计算的条件由依赖分析确定，不要求作者把每个 if 改写成具名布尔变量。按已讨论的简化，Module 顶层组合逻辑不直接或间接调用其他 Module 的 Method；跨模块调用放在事务路径中。

## 7. fire、旧状态与原子性

编译器合并 Rule 的启用路径、自身 guard，以及实际执行路径上必要 Method/资源操作的准入条件。本例中：

```text
add_one.fire = 输入可 pop AND middle 可 push
collect.fire = enabled AND accepted < 2 AND middle 可 pop AND acc.add.guard
emit.fire    = accepted == 2 AND acc.take.guard AND 输出可 push
```

这些公式不需要作者在 Python 中重复书写。存在条件调用时，只合入选中路径的要求，不把未执行分支的 guard 也无条件 AND 进去。准入计算可以读取旧值、计算实参和候选返回值，但不能消费 Queue 或提出状态更新。

- **读旧状态**：普通状态读取看到本拍已提交值。先调用 add 再调用 value，不会隐式读到 add 的更新；需要新值时由 Method 显式计算并返回。
- **无 Queue 同拍前递**：pop 要求旧队列非空，push 要求旧队列未满；满队列不能利用本拍 pop 腾出容量，空队列不能读取本拍 push 的值。非空且未满时可同拍各执行一次。
- **统一提交**：emit 的输出 push、latest 更新、accepted 清零，以及 take 对 total/samples 的清零，要么全部提交，要么全部保持。
- **多写报错**：不能证明互斥的同拍多写冲突由编译器拒绝，不隐式仲裁。collect/emit 的条件在本例中互斥。
- **数据与可用性分开**：Method 返回值可以参与组合计算，但失败事务不能发布成功结果；作者不用额外包装 valid。X/Z 数据与结果不可用也不是同一概念。

guard 保持纯计算。assert 是执行检查，不是 guard；发生系统检查错误时，应保持全局零提交。

## 8. gfsim 与 RTL 如何使用这份 IR

新 gfsim 直接使用已链接事务级 ACIR 和公共分析结果，保留 Module 对象、private Rule 函数及 public Method 函数。一次完整逻辑周期按以下顺序组织：

```text
Begin → 所有 Module 的 CombWork → 所有 Module 的 Work
      → 全局检查 → 各资源所有者 Xfer
```

CombWork 准备可共享的组合值，避免跨模块调用 Method 时其依赖尚未计算。依赖 Method 实参的计算仍留在方法求值路径。Module Work 判断最终 fire，成立后调用 Rule；Rule 调用已有实例的 Method，由 Method 向目标实例提出更新。各资源只由唯一所有者提交，别名不增加提交次数。

RTL 将组合表达式变成电路，将 Method 参数、返回值、guard 和调用使能变成端口及连线。同一个 Rule 的 fire 控制所有相关状态和 Queue 更新。方法调用本身不增加周期；连接中的 Queue 才承担存储。独立 if 不应被后端擅自改成带优先级的 if/else。

现有 C++/RTL 后端先通过目标 lowering 尝试适配这份公共语义；遇到具体困难再调整、裁剪或重写。物理图转换不应成为新 gfsim 的必经步骤，也不能恢复旧的候选值转发来迁就旧实现。

首轮保留四态 Bits、Enum/Struct 和单时钟整周期执行；timer 暂缓。更复杂的同拍依赖和调度可以在这个基础上继续发展。本文记录语义与表示，不把后续扩展或讨论示例当成实现完成证明。
