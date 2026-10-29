# [IR] 统一 Module/Rule/Method 前端、事务级 ACIR 与 t-gfsim/RTL 编译语义

| 项目 | 内容 |
| --- | --- |
| 变更类型 | Design proposal / RFC；涉及前端语义与编译流程重构 |
| 主责领域 | IR；协作领域为 Capture、MLIR、Link、CPP、Verilog、Runtime、System |
| 整理日期 | 2026-10-27 |
| 文档范围 | 统一前端、事务级 ACIR 与后端设计，以此前确认的语义为准 |
| 状态 | 设计提案；不是实现完成声明或测试通过记录 |

## 1. 问题与目标

Module、Rule、Method、组合逻辑和资源连接需要一套统一且容易阅读的定义，ACIR 需要保留这些概念，使前端表达、事务分析和后端生成可以逐层对应。

**本 issue 优先打通没有同拍依赖或执行顺序问题的简单场景。** 当前编译器虽已声称实现部分相关功能，其实际语义和 lowering 结果仍可能偏离设计预期，需要按本文逐项核对。首要目标是在这些简单场景下规范化 ACIR，使其人类可读、能直接对应前端语义；理想情况下，可以直接从这份规范化的 ACIR 生成事务级 t-gfsim，再逐步扩展同拍依赖与顺序处理能力。

核心范式为：**Module 管结构、持久状态和组合计算；Rule 管原子事务；Method 提供同拍调用接口。** AC 是 Architecture Concept 的缩写，ACIR 的主要表达层级是事务。

期望的编译过程：

```text
Python 薄捕获
  → 未链接 ACIR：Module、Rule、Method、资源、组合逻辑和调用
  → link：绑定、具体化，闭合静态实例与资源连接图
  → 已链接事务级 ACIR【本 issue 的目标，位于现有锚点之前】
  → 公共依赖、fire、更新与冲突分析
      ├─ 直接生成事务级 t-gfsim
      └─ 路由/降低并适配现有 Routed ACIR 锚点
          → Hardware final → 已有 C++ / RTL 后端
```

Python 只捕获语法，不执行设计函数。语义、类型、依赖和资源身份由 MLIR 处理。已链接 ACIR 仍保留事务和 Method 调用，不要求先展开为物理引脚才能生成新 t-gfsim。

**本 issue 讨论的 ACIR 是现有 Routed ACIR 锚点之前的事务级 IR。** 现有锚点已经展开 Method 调用和事务效果，主要保留归一化后的计算、逻辑状态更新及路由连线；本文希望在更靠前的位置保留 Module、Rule、Method、guard 和调用结构，规范化为人类可读的表示，并直接生成事务级 t-gfsim。旧后端可继续通过适配后的路由/降低进入现有锚点与 Hardware final；新 t-gfsim 不以经过该锚点为前提。

本文的 Python、MLIR、C++ 和 RTL 都是设计示意。它们说明目标语义，不限定最终 assembler 或 Runtime ABI，也不以当前实现的限制反向修改目标。

## 2. 统一的前端规则

### 2.1 Module：实例、资源和端口

Module 用 `@ac.module def` 定义，可包含持久状态、组合表达式、Rule/Method 定义、子模块实例和连接。

| 写法 | 含义 |
| --- | --- |
| `acc: Accumulator` | 显式创建具名实例；acc 是实例引用 |
| `middle = BiasStage(inp)` | 隐式创建实例并连接；middle 是输出资源，不兼作实例句柄 |
| `out = controller(inp, acc)` | 连接已声明的 controller 实例，不再次创建 |
| `acc: ac.module` 形参 | 接收已有实例的静态引用；具体类型及所需方法在绑定时检查 |
| `accepted: ac.u8 = 0` | Module 拥有的寄存器，初值为 0 |
| `next_accepted = accepted + 1` | 组合值，不创建寄存器 |

Module/Rule 的参数和返回注解描述载荷类型，Queue/Reg 组件类型由实际连接决定。新连接未显式指定组件时默认 Queue，深度为 **2**；允许 `ac.queue[T](depth=N)` 显式配置正整数深度。本文将寄存器资源统一称为 State。

输出也能绑定已有 State，例如 `out: ac.u16` 后再写 `out = stage(inp)`。已声明的组件类型优先，不能重新分配默认 Queue。单值保持标量；共同传输的多字段用 Struct，独立输出用有序 tuple。

新 Queue 在建立连接的父层分配。导出、转发和别名只引用同一个资源，不复制存储、不改变所有者，也不在每层 return 时追加 Queue。当前接口模型先包含 Queue 和 State，Wire 端口不在本轮表达中。

### 2.2 Rule：private 原子事务

普通 Rule 可定义在 Module 内部或源码外部；定义本身不注册执行。Module 顶层显式调用它，静态建立一个每拍参与 fire 判断的事务。

- 内部 Rule 可捕获所属 Module 的状态、资源和组合值；Rule 自身没有持久局部状态。
- 外部 Rule 是源码模板，通过显式参数连接资源；注册后是所属实例的 private 事务。
- Rule 可有资源输入输出和 guard；不能以 `instance.rule()` 作为跨模块公开接口。
- Rule 内可调用各类 Method，调用产生的全部更新归入同一个外层事务。

在事务中，`x = src` 按组件解释：Queue 是 pop，State 是 read。`x` 是取得的局部值，重复使用 x 不重复消费 Queue。写入对象时，Queue 对应 push，State 对应 write。

三种 return 的含义必须区分：

| 所在定义 | return 的含义 |
| --- | --- |
| Module | 导出已有资源或接口 |
| Rule | 向调用处绑定的输出资源提出写入 |
| Method | 返回同拍计算值 |

一个 Rule 的输入消费、输出写入、内部状态更新、跨模块 Method 更新共同提交；任何必要条件不满足，整个事务保持。不同 Rule 通过存储连接，不因此合并为一个事务。

### 2.3 autorule：只自动注册，不增加执行机制

`@ac.autorule` 是前端语法糖：转换为普通 Rule，并在所属 Module 中注册一次，然后走同一条编译路径。

当前形式位于 Module 内，无显式参数和返回值，直接捕获资源；不允许再显式调用它重复注册。需要调用处绑定输入输出时使用普通 Rule。自动注册不代表每拍一定 fire，guard 和资源准入仍然生效。

以状态为 guard 的多个 autorule 都读取旧 state，所以一拍不会连续穿过多个状态。autorule 不进入 ACIR 成为另一种调度机制。

### 2.4 Method：public 同拍接口

| 类别 | 返回同拍值 | 提出更新 |
| --- | --- | --- |
| Value | 是 | 否 |
| Action | 否 | 是 |
| ActionValue | 是 | 是 |

Method 的参数和返回值是数据值，不默认创建 Queue 或 State。Method 可有独立 guard，可嵌套调用 Method，也可通过已有实例引用跨模块调用；其效果属于调用者的外层 Rule，没有独立提交边界。

返回值的可用性由编译器管理。候选值可参与纯计算；事务未获准提交时，不能将其发布为成功调用的结果，作者无需再包装 valid。

### 2.5 Module 中的组合逻辑与控制流

组合逻辑可计算数据、状态汇总、Rule 启用条件、Method guard 和返回值，并供内部 Rule/Method 使用。

- 依赖本地旧状态、已连接资源的只读观察以及纯计算；不能在组合逻辑中 pop。
- Module 顶层不直接或间接调用其他 Module 的 Method；跨模块调用放在事务路径中。
- Method 实参到达后才能进行的计算留在该 Method 中，不提前放入 Module 的 CombWork。
- `if accepted < 4` 可以直接书写，不要求作者先声明一个条件变量。

Module 的 if/else 控制已静态注册 Rule 的启用路径，不在运行时创建或销毁实例、事务或输出 Queue。因此，分支内建立的资源输出可由 Module 导出；这不是对任意普通局部变量放宽定义规则。

**else 取决于原条件，不取决于前一个 Rule 是否成功。** 条件为真而资源阻塞时，不自动执行 else。条件不可用也不等于 False，不能误入 else。两个独立 if 不隐含优先级。

### 2.6 状态、fire 与原子性

- 所有普通状态读取观察本拍旧值，包括同一 Rule/Method 中先写再读。新值通过显式组合计算结果传递。
- 编译器合并路径条件、Rule guard、必要资源条件和实际调用路径上的 Method guard；未选中分支的 guard 不应无条件加入。
- 准入计算只读，可以计算实参或候选返回值，但不能消费 Queue 或提出更新。最终 fire 成立后才执行事务效果。
- Queue 根据旧占用判断满空：空队列不读取本拍 push，满队列不能用本拍 pop 腾出容量。非空且未满时可同拍各执行一次。
- 不能证明互斥的同拍多写冲突报错，不以源码顺序、调用顺序或 RTL 赋值顺序隐式仲裁。
- assert 是执行检查，不是 guard；必要系统检查失败时全局零提交。四态 payload 与结果不可用是不同概念。

## 3. 两个简单例子

### 3.1 基本范式：加一后输出累计和

沿用前端简化文档的例子：输入先加一，再通过平级实例的 Method 累加，每次成功提交都输出新的累计和。最多接受四个样本，之后保持。

```python
import pycircuit as ac


# 外部 Rule：通过显式输入输出连接。
@ac.rule
def add_one(src: ac.u8) -> ac.u8:
    x = src
    return x + 1


@ac.module
def BiasStage(inp: ac.u8) -> ac.u8:
    out = add_one(inp)          # 注册 Rule，输出默认 Queue
    return out                 # 导出已有输出对象


@ac.module
def Accumulator():
    total: ac.u16 = 0
    samples: ac.u8 = 0

    @ac.method(guard=lambda: samples < 4)
    def add(x: ac.u8) -> ac.u16:
        new_total = total + x
        total = new_total
        samples = samples + 1
        return new_total       # 返回同拍计算值


@ac.module
def SumStage(inp: ac.u8, acc: ac.module) -> ac.u16:
    # acc 是上层传入的实例引用，具体模块在连接时确定。
    accepted: ac.u8 = 0

    @ac.rule
    def accumulate(src: ac.u8) -> ac.u16:
        x = src
        result = acc.add(x)
        accepted = accepted + 1  # 直接操作所在 Module 的寄存器
        return result

    out = accumulate(inp)      # 显式注册内部 Rule
    return out


@ac.module
def Pipeline(inp: ac.u8) -> ac.u16:
    acc: Accumulator           # 显式创建具名实例
    summer: SumStage

    intermediate = BiasStage(inp)  # 隐式创建实例，绑定输出 Queue
    out = summer(intermediate, acc)  # 连接已有实例，传入模块引用
    return out
```

```text
Pipeline
├─ BiasStage（隐式实例）
├─ summer: SumStage
└─ acc: Accumulator

输入 Queue → BiasStage → intermediate Queue → summer → 输出 Queue
                                              │
                                              └─ acc.add(x)
```

输入 `3、5、2、7` 经加一得到 `4、6、3、8`，累计输出为 `4、10、13、21`。`acc.add` 返回显式计算的 `new_total`，不依赖写后读取新状态。

输入与最终输出 Queue 由连接 Pipeline 的外层分配；intermediate 由 Pipeline 分配；accepted 属于 SumStage，total/samples 属于 Accumulator。导出资源和传入实例引用都不复制状态。

accumulate 的输入消费、acc 的累加、accepted 更新和输出入队构成一个事务；输出已满或 Method guard 不满足时，全部保持。BiasStage 是另一个事务，通过 intermediate Queue 与之连接。

### 3.2 组合逻辑与控制流：四个样本组成一批

这是独立的补充例子，沿用组合逻辑文档：每次收集四个原始样本，再输出一次批次和。它只补充共享组合值、只读 Method 和 Module 的 if/else 控制，不与上例拼接。

```python
import pycircuit as ac


@ac.module
def BatchAccumulator():
    total: ac.u16 = 0
    samples: ac.u8 = 0

    # Module 组合逻辑：只读本地旧状态，不创建寄存器。
    slots_left = 4 - samples
    can_add = slots_left != 0
    complete = slots_left == 0

    # public Value Method：直接返回已计算的组合值。
    @ac.method
    def remaining() -> ac.u8:
        return slots_left

    # public Action Method：使用组合条件作为 guard。
    @ac.method(guard=lambda: can_add)
    def add(x: ac.u8) -> None:
        total = total + x
        samples = samples + 1

    # public ActionValue Method：返回本批结果，同时清空批次状态。
    @ac.method(guard=lambda: complete)
    def take() -> ac.u16:
        result = total
        total = 0
        samples = 0
        return result


@ac.module
def BatchController(inp: ac.u8, acc: ac.module) -> ac.u16:
    accepted: ac.u8 = 0

    # Module 组合逻辑供 Rule 和 Method 使用。
    next_accepted = accepted + 1
    slots_left = 4 - accepted

    @ac.method
    def remaining() -> ac.u8:
        return slots_left

    @ac.rule
    def collect(src: ac.u8) -> None:
        x = src
        acc.add(x)
        accepted = next_accepted

    @ac.rule
    def emit() -> ac.u16:
        result = acc.take()
        accepted = 0
        return result

    # 声明两个事务及其互斥的启用条件。
    if accepted < 4:
        collect(inp)
    else:
        out = emit()

    return out


@ac.module
def BatchPipeline(inp: ac.u8) -> ac.u16:
    acc: BatchAccumulator
    controller: BatchController

    out = controller(inp, acc)
    return out
```

输入 `3、5、2、7` 输出一个批次和 `17`。第四次 collect 提交后，下拍才选择 emit；emit 成功提交后，下拍才重新收集。两个 remaining Method 展示组合值可以供 Method 读取，不额外增加监控事务。

`else` 由旧 accepted 决定：收集阶段输入为空时，不能转去执行 emit。输出 Queue 始终存在，分支只决定是否向它写入。

以下 ACIR、t-gfsim 和 RTL 以第一个例子为主，第二个只补充控制流差异。autorule、State 输出绑定等保留第 2 节的规则，不额外塞进主例子。

## 4. 事务级 ACIR

### 4.1 保留的概念

Module/Rule 的 ins/outs 表示资源接口，Method 参数和结果表示 SSA 值。`!ac.queue<T>`、`!ac.state<T>` 是资源引用；`!ac.primitive<T>` 是未确定组件类型的引用。分配用 `ac.queue`、`ac.state` 示意，操作统一使用 `ac.method.call`。

guard 与 body 分成两个 region，省略 guard 表示 true。Rule 自身表示事务，不额外用 `ac.transaction` 包一层调用；Module body 直接保留组合表达式和控制流，不增加 `ac.work` 区域。Work 是后端组织形式。

以下是示意 MLIR，省略定位、部分类型和捕获标注，不是正式 parser 输入。bits 运算保留四态语义。

### 4.2 未链接：保留资源与方法约束

```mlir
!u8  = !ac.bits<8>
!u16 = !ac.bits<16>
!P8  = !ac.primitive<!u8>
!Q8  = !ac.queue<!u8>
!Q16 = !ac.queue<!u16>

ac.rule @add_one ins(%src: !P8) outs(%dst: !P8)
    body {
      %x = ac.method.call %src::@read() : () -> !u8
      %result = ac.add %x, 1 : !u8
      ac.method.call %dst::@write(%result) : (!u8) -> ()
      ac.end
    }

ac.module @BiasStage ins(%inp: !P8) outs(%out: !P8) {
  ac.rule.call @add_one ins(%inp) outs(%out)
  ac.end
}
```

`%out` 可在 Module body 中引用；声明输出端口不等于分配 Queue。未链接的 read 可能成为 Queue pop，不能提前视为无副作用操作并消除。

独立编译 SumStage 时，只记录 acc 必须提供 `add(u8) -> u16`。实际模块、guard 和效果在 link 时闭合；接口要求不能替代实现。

### 4.3 link：先绑定，后默认，保留所有权

link 传播实际连接、显式资源类型和模块引用，再对需要新分配且未指定类型的连接采用默认 Queue，深度为 2。

| 抽象操作 | Queue | State |
| --- | --- | --- |
| read | pop | read |
| write | push | write |

显式 push/pop/add 必须由实际对象提供。纯 guard 和组合逻辑中的只读观察不能变成消费操作。

```mlir
ac.module @Pipeline ins(%inp: !Q8) outs(%out: !Q16) {
  %intermediate = ac.queue depth(2) : !Q8
  %acc = ac.instance @Accumulator
  %bias = ac.instance @BiasStage
      ins(inp = %inp) outs(out = %intermediate)
  %summer = ac.instance @SumStage
      ins(inp = %intermediate, acc = %acc) outs(out = %out)
  ac.end
}
```

最终输出 Queue 由外层提供，Pipeline 只分配 intermediate。已有 State 的导出同样保留原所有者，不重新分配。一个模板连接不同组件时可以按绑定特化，不能修改共享定义而影响其他实例。

link 后的可执行实例不再有未绑定的资源和方法，但仍保留 Rule/Method 及调用。独立编译接口需保存载荷、资源约束、方法要求和导出关系。

### 4.4 链接后：保留 Method 和事务

Accumulator.add 仍是带 guard 的 Method，SumStage.accumulate 仍是一个事务：

```mlir
// Accumulator 内：%total、%samples 是本实例拥有的 State。
ac.method @add(%x: !u8) -> !u16
    guard {
      %n = ac.method.call %samples::@read()
      %room = ac.cmp ult, %n, 4
      ac.yield %room
    }
    body {
      %old = ac.method.call %total::@read()
      %n = ac.method.call %samples::@read()
      %wide_x = ac.zext %x : !u8 to !u16
      %new_total = ac.add %old, %wide_x
      %next_n = ac.add %n, 1
      ac.method.call %total::@write(%new_total)
      ac.method.call %samples::@write(%next_n)
      ac.yield %new_total
    }

// SumStage 内：%acc 是已绑定的实例引用，%accepted 是本地 State。
ac.rule @accumulate ins(%src: !Q8) outs(%dst: !Q16)
    body {
      %x = ac.method.call %src::@pop()
      %result = ac.method.call %acc::@add(%x)
      %n = ac.method.call %accepted::@read()
      %next_n = ac.add %n, 1
      ac.method.call %accepted::@write(%next_n)
      ac.method.call %dst::@push(%result)
      ac.end
    }
ac.rule.call @accumulate ins(%inp) outs(%out)
```

第二个例子的 Module 控制流只需额外保留启用路径；collect/emit 的定义仍是同样的 Rule：

```mlir
// BatchController 内。
%n = ac.method.call %accepted::@read()
%next_accepted = ac.add %n, 1
%slots_left = ac.sub 4, %n
%collecting = ac.cmp ult, %n, 4
scf.if %collecting {
  ac.rule.call @collect ins(%inp) outs()
} else {
  ac.rule.call @emit ins() outs(%out)
}
```

这里 scf.if 示意条件注册，实际 lowering 仍保留四态与可用性检查。组合 SSA 可供本地 Rule/Method 读取；条件不取决于另一个事务是否成功。

## 5. t-gfsim（transactional-gfsim）：先准备组合结果，再执行事务

每个 Module 保留自己的 Work/Xfer；Rule 生成 private 函数，Method 生成 public 函数。全系统每轮按以下阶段执行：

```text
所有资源所有者 Begin → 所有 Module CombWork → 所有 Module Work
                    → 全局检查成功 → 各资源所有者 Xfer
```

不能一个模块先 Work/Xfer 完，再执行下一个模块。Method 被调用前，所属模块的组合缓存必须准备好；其自身 Work 可以尚未执行。借用资源不重复清理或提交，组合缓存不参与 Xfer。

主例子的 fire 很直接：

```text
bias_fire       = 输入可 pop AND intermediate 可 push
accumulate_fire = intermediate 可 pop AND 输出可 push AND acc.add.guard
```

以下是 C++ 风格的关键函数示意；省略成员声明、构造、复位和通用检查。State.Read/Queue.Head 读旧值，Propose 只暂存更新。实现仍须保留四态，不以普通整数替代四态语义。

```cpp
// Accumulator：total/samples 是该实例的 State 成员。
bool Accumulator::AddGuard() const { return samples.Read() < 4; }

uint16_t Accumulator::AddWork(uint8_t x) {
    uint16_t new_total = total.Read() + x;
    total.Propose(new_total);
    samples.Propose(samples.Read() + 1);
    return new_total;
}

void Accumulator::Work() {}  // 没有自行注册的 Rule。

// SumStage：inp/out 是借用的 Queue，acc 是借用的实例，accepted 为自有 State。
void SumStage::Work() {
    if (inp.CanPop() && out.CanPush() && acc.AddGuard())
        AccumulateWork();
}

void SumStage::AccumulateWork() {  // private Rule
    uint8_t x = inp.Head();
    uint16_t result = acc.AddWork(x);
    inp.ProposePop();
    accepted.Propose(accepted.Read() + 1);
    out.ProposePush(result);
}
```

SumStage 只清理/提交 accepted，Accumulator 负责 total/samples，Pipeline 负责 intermediate，外层负责输入与最终输出 Queue。一次失败检查阻止所有资源提交。

第二个例子的额外部分只是组合缓存和 if/else：

```cpp
void BatchAccumulator::CombWork() {
    slots_left = 4 - samples.Read();
    can_add = slots_left != 0;
    complete = slots_left == 0;
}
uint8_t BatchAccumulator::Remaining() const { return slots_left; }

void BatchController::CombWork() {
    next_accepted = accepted.Read() + 1;
    slots_left = 4 - accepted.Read();
}
uint8_t BatchController::Remaining() const { return slots_left; }

void BatchController::Work() {
    if (accepted.Read() < 4) {
        if (inp.CanPop() && acc.AddGuard()) CollectWork();
    } else {
        if (out.CanPush() && acc.TakeGuard()) EmitWork();
    }
}
```

此处 BatchAccumulator 的 AddGuard/TakeGuard 分别读取 can_add/complete。CollectWork 调用 acc.add 并更新 accepted；EmitWork 调用 acc.take、输出结果并清零 accepted。每个函数只在自己的完整 fire 成立后提出全部更新。

一般 guard 可依赖实参或候选返回值，需要纯准入计算；不能为了判断 fire 先执行具有效果的方法再撤销。上述例子的 guard 只读旧状态。

## 6. RTL：Method 端口与共同使能

RTL 中组合表达式就是线网。Method 的参数和调用使能由调用者传入，返回值和 guard/ready 由被调用者传出。下面给出主例子的两个模块及平级连接；Queue 本体和 BiasStage 不展开。

```systemverilog
module Accumulator (
    input  logic clk, reset,
    input  logic [7:0] add_x,
    input  logic add_en,
    output logic add_ready,
    output logic [15:0] add_result
);
    logic [15:0] total;
    logic [7:0] samples;
    assign add_ready = samples < 8'd4;
    assign add_result = total + {8'd0, add_x};

    always_ff @(posedge clk) begin
        if (reset) begin
            total <= 16'd0;
            samples <= 8'd0;
        end else if (add_en) begin
            total <= add_result;
            samples <= samples + 8'd1;
        end
    end
endmodule

module SumStage (
    input  logic clk, reset,
    input  logic [7:0] in_data,
    input  logic in_valid,
    output logic in_pop,
    output logic [15:0] out_data,
    input  logic out_ready,
    output logic out_push,
    output logic [7:0] acc_add_x,
    output logic acc_add_en,
    input  logic acc_add_ready,
    input  logic [15:0] acc_add_result
);
    logic [7:0] accepted;
    logic accumulate_fire;
    assign accumulate_fire = !reset && in_valid && out_ready && acc_add_ready;
    assign acc_add_x = in_data;
    assign acc_add_en = accumulate_fire;
    assign in_pop = accumulate_fire;
    assign out_push = accumulate_fire;
    assign out_data = acc_add_result;

    always_ff @(posedge clk) begin
        if (reset) accepted <= 8'd0;
        else if (accumulate_fire) accepted <= accepted + 8'd1;
    end
endmodule
```

Pipeline 内的连接片段如下。`middle_*` 属于 intermediate Queue，`output_*` 属于外层输出 Queue；没有额外转发寄存器。

```systemverilog
logic [7:0] add_x;
logic add_en, add_ready;
logic [15:0] add_result;

SumStage summer (
    .clk(clk), .reset(reset),
    .in_data(middle_head), .in_valid(middle_valid), .in_pop(middle_pop),
    .out_data(output_data), .out_ready(output_ready), .out_push(output_push),
    .acc_add_x(add_x), .acc_add_en(add_en),
    .acc_add_ready(add_ready), .acc_add_result(add_result)
);
Accumulator acc (
    .clk(clk), .reset(reset), .add_x(add_x), .add_en(add_en),
    .add_ready(add_ready), .add_result(add_result)
);
```

同一个 accumulate_fire 控制输入 pop、输出 push、accepted 和 acc 的状态更新。add_result 可以持续出现在组合线上，但只有 fire 成立才被输出 Queue 接收。

第二个例子的 if/else 对应两个互斥启用条件：

```systemverilog
assign collecting = accepted < 8'd4;
assign collect_fire = !reset && collecting && in_valid && acc_add_ready;
assign emit_fire = !reset && !collecting && out_ready && acc_take_ready;
```

两个 Rule 可分别使用 `if (collect_fire)` 和 `if (emit_fire)` 提出更新；互斥来自前端的 if/else，不增加隐式优先级。无法证明互斥的多写应报错。

clock/reset 属于后端接口；以上片段省略系统检查。实际目标仍须保留四态控制检查和失败零提交，不能把普通 SV if 对 X 的处理当作完整语义。

## 7. 用背压检验原子性

主例子在 SumStage 处的行为如下，不表示端到端固定延迟：

| 情况 | 事务结果 |
| --- | --- |
| 输入有值、输出有空间、samples < 4 | 消费一个输入，更新 total/samples/accepted，输出新累计和 |
| 输入为空 | 不更新状态、不输出 |
| 输出已满 | 不消费输入，所有累加状态保持 |
| samples == 4 | Method guard 为假，整个累加事务保持 |

即使输出 Queue 在本轮被消费者 pop，轮初满仍然使本轮累加事务保持。第二个例子同理：批次收满后，输出阻塞时不清空累加器、不开始下一批。

这些是目标语义推演，不是编译器已运行通过的证据。

## 8. 实现拆分、兼容处理与验收

### 实现拆分

| 步骤 | 工作 | 验证重点 |
| --- | --- | --- |
| 1 | 确立资源、Module、Rule、Method、guard 与调用的 ACIR 表示 | 可读性、解析/打印、纯度、所有权与签名检查 |
| 2 | 公共实例身份、路径、fire、旧状态和多写分析 | 跨模块原子性、互斥判断、别名与不可用路径 |
| 3 | 已链接手写 ACIR → 新 t-gfsim | 函数结构、全局阶段、四态、保持与失败零提交 |
| 4 | 独立 compile/link 和资源具体化 | 分开编译、模块引用、Queue/State 绑定、导出不复制 |
| 5 | Python lowering、autorule、组合控制流与 System | 完整例子走同一条产品路径 |
| 6 | 旧 C++/RTL 后端适配和清理 | 同一公共语义、相同提交边界、旧用例状态记录 |

基础类型和四态运算、布局、独立编译和发布机制可按实际适用性复用。旧后端先尝试适配；出现具体困难后再调整、裁剪或重写，不能未经尝试整体弃用。物理 lowering 不能成为新 t-gfsim 的必经步骤。

不强行保留与新语义冲突的旧功能，也不要求所有旧测试和 example 继续通过。每个受影响的旧用例都要标记基线/修改后结果、原因、处理方式及证据；预期不兼容仍然是 FAIL，删除、跳过和未运行不能计作 PASS。

### 验收条件

- [ ] 内部/外部 Rule、显式注册、autorule 一次注册与重复注册拒绝。
- [ ] Module/Rule 的 Queue/State 输入输出、已有 State 输出绑定、别名和资源导出。
- [ ] 默认深度 2、显式深度、Queue 空/满/中间占用和无同拍前递。
- [ ] Value/Action/ActionValue、嵌套调用、平级实例引用与跨所有者原子更新。
- [ ] 纯 guard、实参相关 guard、条件调用和路径相关 fire；阻塞整体保持。
- [ ] Module 组合值供 Rule/Method 读取；callee Work 未执行时仍可使用其 CombWork 结果。
- [ ] 写后读取旧值、显式返回新 SSA 值、多写冲突拒绝及互斥写接受。
- [ ] 四态数据、Enum/Struct、状态保持、初始化/复位和全局检查失败零提交。
- [ ] 分开编译后 link 具体化；缺失/不匹配的方法、未闭合引用与错误绑定拒绝。
- [ ] Python → ACIR → 新 t-gfsim 的完整例子；成功适配的旧 C++/RTL 与独立预期一致。
- [ ] 所有旧测试/example 的失败、退役和 NOT RUN 状态可追溯。

以上是待取得的验收证据，本 issue 的代码和时序推演不等于通过记录。

### 边界与后续讨论

首轮保留四态 Bits、Enum/Struct 和单时钟整周期执行；timer 暂缓。更复杂的同拍 Work 依赖认为可以在这个框架上升级，通过依赖分析和调度逐步扩展，不提前断定哪些能力永久不做。

正式 IR 拼写、Runtime ABI 和优化方式由后续实现落实，但不得改变本文资源身份、事务边界与时序含义。若发现这些规则之间无法一致实现，应先讨论语义，不以兼容旧实现为由自行修改约定。

## 9. 相关工作

- [#319：作者接口与执行语义](https://github.com/PTO-ISA/pyCircuit/issues/319)：前端与事务语义的背景讨论。
- [#320：架构层 IR 与高层仿真能力](https://github.com/PTO-ISA/pyCircuit/issues/320)：保留高层语义与执行表示的背景。
- [#327：周期精确的高层 C++ profile](https://github.com/PTO-ISA/pyCircuit/issues/327)：相关后端工作，其历史复用约束与本方案需要分别核对。

本 issue 先在没有同拍依赖或执行顺序问题的简单场景下核对设计语义、规范化可读 ACIR，并打通事务级 t-gfsim。上述 issue 是相关背景，不表示其全部历史范围都是本次首阶段的前置任务，也不表示这些工作已由本 issue 完成或关闭。
