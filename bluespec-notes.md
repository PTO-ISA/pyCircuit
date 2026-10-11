# 关于 Bluespec 的一些理解

目前可以把 Bluespec 理解为：module 封装状态和操作，method 提供接口，rule
组织自主执行的原子事务，编译器决定这些事务能否同周期发生，以及必要的执行顺序。

下面先按单时钟同步设计理解。这份笔记记录的是 BSV/Bluesim 的机制，供后续思考
Agentic Circuit 和 GFSim 的设计时参考。

## 1. module：状态与行为的组织单位

module 可以包含：

- 寄存器、FIFO 等状态组件。
- 子 module 和静态连接。
- 对外公开的 method。
- 自主运行的 rule。
- 纯函数和组合表达式。

一个 module 可以同时具有公开接口和自主逻辑，也可以只有接口，由其他 module
的 rule 调用。因此，module 边界不是原子事务边界。

module 实例化和结构连接属于展开阶段；运行时的状态变化主要由 rule 和 Action
类 method 表达。组合表达式也可以在 rule/method 之外定义。

## 2. method：带就绪条件的接口操作

method 是 module 的公开操作，也可以被本 module 的逻辑使用。它可以调用其他
module 的 method。

| 类型 | 含义 |
| --- | --- |
| Value method | 纯读取或计算，返回一个值，不修改状态 |
| Action method | 表达状态修改，不返回业务值 |
| ActionValue method | 同时返回值并表达状态修改 |

method 可以显式定义 guard。即使没有显式 guard，也可能通过底层调用继承隐式
就绪条件。例如外层 method 调用 FIFO.enq，就需要继承入队操作的容量条件。

因此，调用者不仅得到操作本身，也得到“什么时候可以调用”的约束。

一次 method 调用本身不会挂起并跨多个 tick 执行。返回值可以用于本周期计算，
状态修改遵守所使用组件的时序语义。多周期功能通常通过 request/result 等接口，
配合内部 rule 和状态组织起来。

## 3. rule：自主执行的原子事务

rule 是 module 中自主尝试执行的事务入口，可以写条件分支、进行组合计算，并
调用本 module 或其他 module 的 method。

```bsv
rule move;
    let x = inQ.first;
    outQ.enq(x);
    inQ.deq;
endrule
```

这条 rule 表达读取输入、写入输出和消费输入。这些必要操作必须一起发生，不能
只完成其中一部分。

method 的状态修改也具有原子操作语义，但被 rule 调用时，其效果并入调用者
rule，不成为独立提交的子事务。method 调用其他 method 时，调用链上的效果
继续属于外层原子动作。纯读取 method 则只是求值，没有状态更新需要提交。

rule 是自主执行的，method 是被调用的；同一个状态可以同时被内部 rule 和公开
method 访问，编译器需要处理它们的访问关系。

## 4. guard 与 fire：就绪之外还有调度

rule 的执行资格来自自己的显式条件，以及所调用 method 传播上来的隐式条件。
但这些条件满足，还不一定最终执行，因为可能与其他规则争用资源。

可以概念上区分：

```text
CAN_FIRE：条件满足，可以尝试执行
WILL_FIRE：经过冲突和优先级处理，最终执行
```

两个 rule 都写同一个普通寄存器时，它们可能都满足业务条件，但不能共同执行，
需要选择一个。

对于运行时分支，BSC 默认可以保守地汇总分支中的隐式条件。启用
`-aggressive-conditions` 后，通常可以把就绪判断与对应分支条件关联；某些不能
提前计算的条件仍可能回退到保守处理。规则拆分也会影响分析结果。

“业务条件”和“基础条件”取决于组件抽象层级。FIFO 的满空条件、TLB 的处理阶段
限制、EpochManager 的编号复用限制，都可以成为组件接口的就绪契约。但操作的
状态条件与多个调用者之间的资源竞争，仍然需要分别表达。

## 5. conflict matrix：操作能否共存及其顺序

基础组件的 method 之间具有访问关系，上层 module 根据内部调用推导自己的
接口关系。通常所说的 module conflict matrix，是该 module 导出 method 之间
的关系摘要。编译器再根据 rule 的调用，推导 rule 之间的冲突和顺序。

| 关系 | 含义 |
| --- | --- |
| CF | 可以同周期执行，先后顺序不影响结果 |
| SB | 可以同周期执行，但要求指定的先后顺序；也允许在同一 rule 中组合 |
| SBR | 要求先后顺序，并限制两者不能放在同一 rule 中 |
| C | 不能同周期执行 |

需要区分执行顺序与调度优先级：前者决定双方都发生时的语义顺序，后者决定
发生竞争时谁获准。

这些关系不是简单根据源码排列决定的，而是来自底层资源契约、调用关系、条件
分析，以及显式调度属性。

### 从 method 关系传播到 rule 顺序

假设同一个 Queue 定义了 pop 在 push 之前，rule A 调用 pop，rule B 调用 push：

```text
组件关系：q.pop → q.push
调用关系：A 调用 q.pop，B 调用 q.push
    ↓
共同执行时的顺序：A → B
```

这约束的是两条 rule 的执行顺序。如果 push 的 ready 还依赖 pop 本周期释放的
容量，就需要先判断并执行 A，再判断 B。A 没有 fire 时，B 仍然可以判断，但看不到
来自 A 的容量更新。

约束针对具体资源和具体调用者建立，也可以通过多层 method 调用传播。一条
rule 调用多个组件时，需要综合全部约束，不能只看其中一个组件的关系。

### rule 顺序图成环时怎么办？

从资源关系推导出的初始 rule 顺序图可以成环，但最终用于拓扑执行的调度图
必须无环。

例如，两个 Queue 都采用 pop → push 的顺序：

```text
A：pop q1，push q2
B：pop q2，push q1

q1 要求 A → B
q2 要求 B → A
```

这个环表示：A、B 无法在同周期按这些组件契约找到一个合法的原子规则串行顺序。
BSC 会处理某些这类环，通过增加冲突约束限制共同执行，并给出警告。对于上面的
二规则例子，可以使 A、B 互斥，每周期至多执行一个。其他无法消除的综合调度环
仍可能导致报错。

不能仅删除一条必要的顺序边，却继续允许原本无法合法排序的规则共同执行。
也不能假设 Bluesim 会在运行时反复执行规则来求解这个环。

### 有顺序环，不等于硬件无法实现

仍以上面的例子为例，如果 q1、q2 周期开始时都满：

- A 先执行，q2 满，A 无法完成。
- B 先执行，q1 满，B 无法完成。

但硬件可以读取两个旧队首，然后在同一个时钟边沿交换元素：

```text
q1：pop 旧元素，push 来自 q2 的元素
q2：pop 旧元素，push 来自 q1 的元素
两边占用数量都不变
```

这需要明确的联合判断，例如专门的交换接口或一个使用合适组件操作的联合
事务。不能简单依靠“A 获准取决于 B，B 获准取决于 A”的组合反馈。

当两个队列都满时，若只写出容量方程，可能得到：

```text
fire_A = fire_B
fire_B = fire_A
```

“都不执行”和“都执行”都可能满足方程，因此还需要明确的选择机制。

BSV 的多规则共同执行保持可以解释为某个合法逐条执行顺序的语义。限制的是
这些原子规则按现有契约共同执行的方式，不是上述交换功能在物理硬件上的可实现性。
跨周期、经过持久状态的电路反馈也不属于这里的同周期调度环。

## 6. EHR：显式的同周期状态可见性

EHR 是一个有序多端口寄存器抽象。Toooba 用 BSV 库实现了它。

```bsv
Ehr#(3, UInt#(32)) x <- mkEhr(0);
```

这里是一个持久状态和三个逻辑访问端口，不是三个独立寄存器。

| 操作 | 可以观察的值 |
| --- | --- |
| 读取端口 0 | 周期开始时的旧值 |
| 读取端口 1 | 旧值，加上端口 0 的本周期有效写入 |
| 读取端口 2 | 旧值，加上端口 0、1 的本周期有效写入 |
| 最终保存 | 最高编号、实际发生的写入；没有写入则保持旧值 |

同一端口读取看不到自身本周期的写入。同一端口同一周期最多一个有效写入，
多条 rule 写同一端口仍然需要互斥或仲裁。

EHR 可以表达数据旁路、容量传播、同周期多次更新的接续，以及不同来源更新的
有序覆盖。例如：

```text
rule A 向端口 0 写 10
    → rule B 从端口 1 读到 10，再写 11
    → 高端口读取可以看到 11
    → 最终持久状态保存为 11
```

前提是这些规则都获准执行。没有发生的写入不能成为旁路来源。

### 一个需要修正的理解

编译器不会因为发现先后关系，就自动把普通寄存器改成 EHR，并自动分配端口。
Toooba 中由设计者或组件库显式实例化 EHR，选择各操作使用的端口：

```text
设计者／组件库选择端口
    → 端口产生可见性和调度约束
    → 编译器推导调用者之间的关系
    → 安排合法执行顺序
```

先后关系本身不保证新值可见；可见性由所使用的组件和端口语义决定。

## 7. Toooba 中几个典型的 EHR 用法

### FIFO：数据与容量旁路

Pipeline FIFO 的 deq 写 full 的低端口，enq 的 guard 读取高端口。消费者获准
出队后，生产者可以看到本周期释放的容量，即使周期开始时 FIFO 已满。

Bypass FIFO 则把新入队数据和非空状态传递给后面的 first/deq，使空 FIFO 的
新数据也可以在同周期被消费。

### RenamingTable：多条提交依次更新映射

同周期多条提交按位置使用不同 EHR 端口。如果两条指令都更新同一个架构寄存器，
后面的提交可以看到前面的映射更新，正确确定需要回收的物理寄存器。

### FetchStage：多个 PC 更新的有序覆盖

取指推进、Decode 修正和后端 redirect 使用不同 PC 端口。共同发生时，后端
redirect 的高端口更新成为最终 PC，final 端口可以观察最终值。

### ROB：条目初始化与推测位清除接续

ROB 条目分配写 spec_bits 的端口 1，正确推测更新从端口 2 读取并应用 mask。
两者同周期发生时，清除操作可以作用于刚初始化的推测位。

ROB 提交读取的执行状态使用端口 0，所以不会自动看到本周期刚写入的 Executed。
EHR 允许设计者选择哪些更新同周期可见，哪些仍然观察旧值。

## 8. Bluesim：把关系编译成执行程序

Bluesim 的调度顺序在编译、链接生成阶段确定。运行时按照生成的 C++ 程序计算
条件和执行获准操作，不需要每周期重新构图和拓扑排序。

### 顺序从哪些信息推导？

method conflict matrix 是重要输入，编译器还会综合调用关系、显式优先级、
就绪信号和数据的组合路径，以及子模块内部调度约束：

```text
基础组件的 method 关系与信号路径
    → 上层接口摘要和 rule 访问关系
    → 冲突、执行顺序、判断依赖和优先级
    → 综合调度图
    → 拓扑排序
    → 固定 C++ 执行程序
```

仅知道 pop 在 push 前面，可以约束主体的执行顺序；结合 full 的更新与读取路径，
才能进一步知道 push 的 ready 判断也必须在 pop 的执行之后。

### Sched 与 Exec：分别安排判断和执行

BSC 为每条 rule 区分两类调度节点：

```text
Sched(A)：计算 A 是否获准执行
Exec(A)：如果获准，执行 A 的主体
```

综合图主要表达以下依赖：

| 依赖来源 | 图中的顺序 |
| --- | --- |
| 先判断自己，再执行 | Sched(A) → Exec(A) |
| A、B 冲突，A 优先 | Sched(A) → Sched(B) |
| 共同执行时，A 的主体必须在前 | Exec(A) → Exec(B) |
| B 的判断需要 A 本周期产生的信号 | Exec(A) → Sched(B) |

最后一项意味着判断和执行可以交错。例如 Pipeline FIFO 的容量旁路可以要求：

```text
Sched(A)
    → Exec(A)，如果 A fire，产生有效的 pop / 容量旁路
    → Sched(B)，读取实际可见的 ready
    → Exec(B)，如果 B fire，执行 push
```

示意代码如下，实际生成代码还会包含必要的组合计算和基础组件处理：

```cpp
will_A = /* A 的完整条件和调度许可 */;
if (will_A)
    rule_A();

will_B = /* 读取更新后的 ready，计算 B 的执行资格 */;
if (will_B)
    rule_B();
```

因此，不一定先算完所有 fire，再执行全部 rule。没有依赖约束的节点可以选择
任意合法顺序；运行时分支决定哪些操作实际发生，不要求每周期重新排序。

### 跨 module 的调度合并

生成 Bluesim 模型时，编译器找到父 rule 对子 method 的调用，把子 method 的
相关约束接到调用者，并合并子模块内部 rule 的约束，按时钟域形成综合调度图。

method 通常由调用者执行，其边界依赖合并到调用者，不是每个 method 都作为
独立自动任务运行。模块实例保留层次结构，不同 module 的 rule 可以交叉执行：

```text
A.rule0 → B.rule0 → A.rule1
```

部分基础组件的 C++ 方法直接修改仿真状态。普通寄存器需要的旧值读取通过
编译器安排的顺序保证；EHR 则按照端口契约提供本周期更新的可见性。

rule 的自主执行也不表示每条 rule 有独立监听线程。Bluesim 由时钟事件调用固定
调度程序，程序计算相关条件并执行获准操作。

## 当前形成的整体认识

```text
module 封装状态与接口
    ↓
method 定义操作、guard 和访问关系
    ↓
rule 组合操作，形成原子事务
    ↓
组件端口决定可见性，编译器分析冲突与顺序
    ↓
生成 fire 判断和执行程序
    ↓
每周期计算并执行获准的事务
```

最核心的三层是操作契约、事务原子性和同周期可见性。method、rule、EHR 分别
承担其中的重要部分，编译器把它们组合成合法的执行安排。

## 本地源码参考

- [method 调度关系定义](/home/lc/bsc/src/comp/SchedInfo.hs:91)
- [CAN_FIRE、WILL_FIRE 与 method RDY 的生成说明](/home/lc/bsc/src/comp/AAddScheduleDefs.hs:29)
- [顺序图成环时增加冲突约束](/home/lc/bsc/src/comp/ASchedule.hs:1043)
- [综合判断、执行、优先级及路径依赖图](/home/lc/bsc/src/comp/ASchedule.hs:2235)
- [综合调度图的环检查](/home/lc/bsc/src/comp/ASchedule.hs:2329)
- [父子模块调度关系合并](/home/lc/bsc/src/comp/SimExpand.hs:1014)
- [Bluesim 调度图排序](/home/lc/bsc/src/comp/SimExpand.hs:314)
- [Bluesim 调度节点转为仿真语句](/home/lc/bsc/src/comp/SimMakeCBlocks.hs:1003)
- [Toooba EHR 实现](/home/lc/bsc/examples/Toooba/src_Core/RISCY_OOO/procs/lib/Ehr.bsv:24)
- [Toooba Pipeline FIFO](/home/lc/bsc/examples/Toooba/src_Core/RISCY_OOO/procs/lib/Fifos.bsv:366)
- [Toooba Bypass FIFO](/home/lc/bsc/examples/Toooba/src_Core/RISCY_OOO/procs/lib/Fifos.bsv:414)
- [Toooba RenamingTable 提交更新](/home/lc/bsc/examples/Toooba/src_Core/RISCY_OOO/procs/lib/RenamingTable.bsv:234)
- [Toooba FetchStage PC 端口](/home/lc/bsc/examples/Toooba/src_Core/RISCY_OOO/procs/RV64G_OOO/FetchStage.bsv:270)
- [Toooba ROB 推测位端口](/home/lc/bsc/examples/Toooba/src_Core/RISCY_OOO/procs/lib/ReorderBuffer.bsv:183)
