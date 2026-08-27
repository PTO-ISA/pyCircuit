# Agentic Circuit 详解与实践指南

> **适用分支**：`feature/acir-acsim`　**契约纪元（contract epoch）**：`0.1`
> **验证环境**：LLVM/MLIR 22.1.8 + GCC 16.1（micromamba 环境 `acir`）
> 本文中所有 MLIR 片段、命令行和输出，都在该环境下实际执行验证过，不是示意代码。

---

## 目录

1. [这个项目在解决什么问题](#1-这个项目在解决什么问题)
2. [完整编译流水线](#2-完整编译流水线)
3. [ACIR 层：架构意图的表达](#3-acir-层架构意图的表达)
4. [ACSim 层：C++ 实现结构的表达](#4-acsim-层c-实现结构的表达)
5. [用例一：pipeline.mlir 逐行精讲](#5-用例一pipelinemlir-逐行精讲)
6. [用例二：cluster.mlir 三阶段逐行精讲](#6-用例二clustermlir-三阶段逐行精讲)
7. [当前完成度盘点](#7-当前完成度盘点)
8. [框架还缺什么](#8-框架还缺什么)
9. [上手实操](#9-上手实操)
10. [附录：属性合法取值速查表](#10-附录属性合法取值速查表)

---

## 1. 这个项目在解决什么问题

### 1.1 目标

Agentic Circuit 想做的事情，一句话说就是：**让你用 Python 写清楚一个芯片架构的意图，然后自动编译出一个专门为这个架构定制的、纯 C++20 的事件驱动仿真器**。

这跟常见的两类做法都不同：

| 做法 | 特点 | 问题 |
|---|---|---|
| 通用仿真框架（SystemC、gem5） | 写 C++ 类，继承框架基类 | 架构意图淹没在实现细节里，改架构 = 改代码 |
| 高层建模语言（各种 DSL） | 描述简洁 | 通常解释执行，性能差；或者生成的代码不可控 |
| **Agentic Circuit** | Python 描述意图 → 编译期全部静态化 → 生成专用 C++ | 架构意图和实现分离，但运行时没有任何动态开销 |

关键在于**编译期把所有决策做完**：拓扑在编译期冻结、组件绑定在编译期锁定、进程状态机在编译期展开成显式 PC。运行时不查表、不做虚分派决策、不解析配置。

### 1.2 三条贯穿全局的设计原则

理解这三条，就能理解为什么 IR 里有那么多看起来"啰嗦"的字段。

**原则一：确定性（Determinism）**

同一个模型，任何时候、任何机器上编译，产出的 IR 必须逐字节相同，仿真结果也必须逐位相同。这不是"尽量"，而是硬性约束，并且有测试守卫（`test/Conversion/bindings.mlir` 里连续跑两次降级然后 `diff` 比对）。

这条原则直接导致：

- 所有集合都要**排序**（构造顺序、类型声明、绑定记录都按名字排序）
- 所有对象都要有**稳定标识**（`stable_id` 和 `path`）
- 随机种子必须是**固定值**（`seed {kind = "fixed", value = N}`，目前只支持 `"fixed"` 这一种策略）
- 全局时间用**精确整数**表示，不用浮点

**原则二：静态化（Static Elaboration）**

一切能在编译期确定的，都必须在编译期确定。ACIR 里没有"运行时创建对象"的概念，没有动态数组，没有条件实例化。`ac.array shape [2]` 就是编译期展开成两个具名对象 `cells[0]` 和 `cells[1]`。

**原则三：契约锁步（Contract Epoch Lockstep）**

整条工具链上每个环节、每个持久化产物，都带一个 `contract_epoch` 字段，并且必须**精确相等**。当前是 `"0.1"`。你会在 MLIR 模块属性里看到它：

```mlir
builtin.module attributes {ac.contract_epoch = "0.1"} { ... }
```

也会在 JSON schema、`pyproject.toml`、绑定注册表里看到它。少一个地方对不上，整条链就拒绝工作。这是为了避免"版本漂移"——某个环节升级了而其他环节没升级，产生看似能跑但语义已经错了的结果。

---

## 2. 完整编译流水线

### 2.1 全景图

```
┌─────────────────────────────────────────────────────────────┐
│  Python 源码                                                 │
│  @system / @module / @protocol / @packet 装饰器              │
└────────────────────────┬────────────────────────────────────┘
                         │  受限 AST 捕获 + 归一化
                         │  【规范：python-to-acir-lowering-v0.1】
                         ▼
                  ┌──────────────┐
                  │  acpy 语义 IR │   ← 尚未实现
                  └──────┬───────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│  ACIR —— 架构意图层（Architecture Intent）                    │
│  模块层级 / 协议 / 队列 / 资源 / 进程 / 地址空间 / 时间域        │
│  【规范：acir-core-v0.1】                                     │
└────────────────────────┬────────────────────────────────────┘
     │ ac-verify-model        全模型语义校验
     │ ac-canonicalize-model  规范化（排序、归一化属性）
     │ ac-freeze-topology     拓扑冻结，计算 topology_digest
     │ ac-lower-process-state 规划进程状态机（PC、活跃槽、公平度）
     ▼
┌─────────────────────────────────────────────────────────────┐
│  冻结 ACIR —— 带 ac.topology_frozen / ac.frozen_owners 等属性  │
└────────────────────────┬────────────────────────────────────┘
     │ ac-resolve-gfsim-bindings  绑定解析（查注册表，锁定 C++ 实现）
     │ ac-lower-to-acsim          降级
     ▼
┌─────────────────────────────────────────────────────────────┐
│  ACSim —— C++ 实现结构层（Simulator Structure）               │
│  构造/析构顺序 / 类型指纹 / 绑定记录 / 派发表 / 状态机          │
│  【规范：acsim-gfsim-lowering-v0.1】                          │
└────────────────────────┬────────────────────────────────────┘
                         │  代码生成           ← 只有骨架，未接通
                         ▼
              ┌──────────────────────┐
              │  C++20 源码 + manifest │
              └──────────┬───────────┘
                         │  编译链接 gfsim 运行时  ← header 骨架
                         ▼
              ┌──────────────────────┐
              │  专用仿真器可执行文件   │
              └──────────┬───────────┘
                         │  输入 PTO trace（JSON）  ← 未实现
                         ▼
                  结果 JSON（result_schema）
```

**当前可用的部分**：从手写 ACIR 开始，一直到产出 ACSim IR，这一段是通的、有测试的、可以实际跑的。图中标注"未实现/骨架"的部分，见[第 8 章](#8-框架还缺什么)。

### 2.2 为什么要分成 ACIR 和 ACSim 两层

这是整个设计里最值得理解的一点。两层的关注点完全不同：

| | ACIR | ACSim |
|---|---|---|
| **回答的问题** | 这个架构**是什么** | 这个仿真器**怎么建出来** |
| **典型内容** | "有一个容量 4 的 FIFO 队列，遵循 ready_valid 协议" | "构造第 3 个对象，类型是 `gfsim::Queue<i32,4>`，work 函数是 `gfsim::q_work`" |
| **抽象层次** | 与实现无关，可以映射到不同后端 | 与 C++ 强绑定，几乎是代码的 IR 形式 |
| **符号引用** | 逻辑名（`@stage`、`@ready_valid`） | 展平路径 + 数值 ID（`"Top.cluster.cells[0]"`、`object 3`） |
| **是否含指纹** | 否（冻结后有 topology_digest） | 是，六类指纹全覆盖 |

分层的好处是**替换后端不用改前端**。同一份 ACIR，配不同的绑定注册表（binding registry），可以降级出面向不同 C++ 实现的 ACSim——比如 `fast` profile 用无检查的快速实现，`validated` profile 用带断言的实现，两者的 ACIR 完全一样。

---

## 3. ACIR 层：架构意图的表达

### 3.1 算子分类总览

ACIR 一共约 50 个算子，可以分成五组：

**（1）类型声明组** —— 定义数据长什么样

```
ac.type_scope     类型作用域（带 DataLayout）
ac.type_alias     类型别名
ac.struct         结构体
ac.enum           枚举
ac.union          联合体（带判别字段 discriminator）
ac.packet         包（可序列化，用于跨模块传输）
ac.transaction    事务（PTO trace 的基本单位）
```

配套的值算子：`ac.record.create` / `ac.record.get` / `ac.record.with`（函数式更新）/ `ac.packet.serialize` / `ac.packet.deserialize`。

**（2）协议组** —— 定义模块之间怎么通信

```
ac.interface      接口容器
ac.protocol       协议容器
  ac.role         角色（producer/consumer），带对偶（dual）和基数（cardinality）
  ac.state        协议状态机的状态
  ac.event        状态机的事件（带 payload 类型和 action）
  ac.transition   状态转移（带 guard 区域、transfer/retain 语义）
  ac.guarantee    协议保证（ordering / backpressure / delivery / ...）
ac.port           端口
```

协议不是装饰性的注释，它有**可验证的语义**：比如队列声明的 `ordering` 不允许弱于协议声明的 `ordering` guarantee，`per_key` 队列必须要求协议声明了 `correlation`。

**（3）结构组** —— 定义有哪些东西、怎么组装

```
ac.system            仿真系统的根，指定根模块、tick 单位、种子、结果 schema
ac.module            模块定义（图区域，不是 SSA 顺序区域）
ac.module.extern     外部模块声明，由 C++ 实现提供
ac.module.generated  生成器模块声明
ac.instance          实例化一个模块
ac.array             实例化一个模块数组（编译期展开）
ac.instances         异构批量实例化
ac.view              对数组的视图（切片、拼接）
```

**（4）状态与行为组** —— 定义运行时会发生什么

```
ac.queue          队列（有 payload 类型、容量、排序、协议、水位线）
ac.event_queue    事件队列（挂在时间域上）
ac.resource       资源（容量、发射宽度、发起间隔、延迟模型、生命周期）
ac.address_space  地址空间
ac.address_map    地址映射
ac.time_domain    时间域（period / phase / scale）
ac.process        进程（control / workload / monitor 三种）
  ac.try_send / ac.try_recv    非阻塞收发
  ac.schedule                  延迟投递
  ac.wait_until / ac.wait_for  等待条件 / 等待资源
  ac.await_event               等待事件队列
  ac.yield_sim                 让出（进程体的终结符）
  ac.trace.open/next/decode/eof/position   trace 游标操作
```

**（5）观测与契约组** —— 定义怎么检查、怎么统计

```
ac.require / ac.ensure / ac.assert   前置/后置/一般断言
ac.probe          探针（读队列深度、资源占用等）
ac.stat           统计量声明（counter / gauge / histogram / event_log）
ac.stat.add       累加统计量
ac.instrumentation 仪表化容器
```

### 3.2 类型系统

ACIR 有自己的类型，也允许直接用 MLIR 内建类型（`i32`、`i64` 等）。自定义类型分两类：

**具名类型**（引用一个符号声明）：

```mlir
!ac.struct<@MyStruct>          结构体
!ac.packet<@MyPacket>          包
!ac.transaction<@MyTxn>        事务
!ac.enum<@MyEnum>              枚举
!ac.union<@MyUnion>            联合体
!ac.address<@MySpace>          地址（属于某个地址空间）
!ac.resource_token<@MyRes>     资源令牌
```

**参数化类型**：

```mlir
!ac.optional<i32>                    可选值
!ac.list<i32>                        变长列表
!ac.vector<4 x i32>                  定长向量
!ac.flow<i32, @proto>                数据流（带协议）
!ac.channel<i32, @proto>             通道（带协议）
!ac.endpoint<@iface, @role>          接口端点
!ac.resource_ref<@resType, @role>    资源引用
!ac.duration<ns>                     时长（带单位）
!ac.rate<bit, s>                     速率（分子单位/分母单位）
!ac.event<i32>                       事件
```

### 3.3 三个关键机制

理解这三个机制，看 IR 就不会觉得字段冗余。

#### 机制一：稳定标识（stable_id + path）

每个结构性对象都必须同时提供 `stable_id` 和 `path`：

```mlir
ac.instance @accel of @Leaf() static {width = 8 : i64}
    id "accel" path "accel" : () -> i32
//  ^^^^^^^^^^ ^^^^^^^^^^^^
```

- `path`：人类可读的层级路径，用 `.` 分隔，冻结后会补全成 `root.accel`
- `stable_id`：用于指纹计算的稳定标识，冻结后变成 `root/accel`

为什么要两个？因为 `path` 是给人看和给诊断信息用的，`stable_id` 参与哈希计算，需要保证跨版本稳定。分开之后，将来改变路径显示格式不会导致指纹变化。

#### 机制二：一 tick 状态边界（delay_ticks）

`ac.queue`、`ac.event_queue`、`ac.resource` 都有一个 `delay_ticks` 属性，默认值 1，而且**验证器强制它必须恰好等于 1**：

```
error: 'ac.queue' op stateful declaration delay_ticks must be exactly one positive tick
```

这条规则的含义是：**任何有状态的东西，写进去的值不能在同一个 tick 内被读出来**。规范里的原话是"只有纯的、无状态、无副作用的操作才能有零延迟"。

这直接决定了仿真的三相结构（Work → Arbitrate → Xfer）：进程在 Work 相里提出提案（proposal），仲裁在 Arbitrate 相里决定谁赢，状态在 Xfer 相里才真正改变。这样同一 tick 内所有进程看到的都是同一个快照，消除了执行顺序对结果的影响——这是确定性的基础。

#### 机制三：拓扑冻结（topology freeze）

`ac-freeze-topology` 这个 pass 做的事情是：遍历整个层级，把每个对象的完整路径和稳定 ID 计算出来，写成模块级属性，然后对整体算一个 SHA-256 摘要。

冻结之后模块上会多出这些属性：

```mlir
ac.topology_frozen = true
ac.topology_digest = "7bdf586a8f38efaf..."
ac.frozen_owners = [{kind = "ac.system_root", owner = @Top, path = "root", stable_id = "root"}, ...]
ac.frozen_primary_workload = {path = "root.tick", reference = @Top::@tick, stable_id = "root/tick"}
ac.frozen_system = @soc
ac.freeze_epoch = "0.1"
```

之后任何修改拓扑的操作都会被检测到——`acir-opt` 在流水线末尾还会再跑一次 `VerifyModelPass`，摘要对不上就报错。降级到 ACSim 也强制要求输入必须是冻结过的：

```
error: ACLOWER-EPOCH-MISMATCH: ac-lower-to-acsim requires a topology-frozen v0.1 model; run ac-freeze-topology first
```

---

## 4. ACSim 层：C++ 实现结构的表达

ACSim 只有 20 个左右的算子，因为它的任务很聚焦：描述"怎么建出这个 C++ 对象图"。

### 4.1 算子总览

```
acsim.model       顶层容器：根模块、构造顺序、析构顺序、六类指纹
acsim.type        一个 C++ 类型的声明（cpp_name + kind + fingerprint）
acsim.binding     一条绑定记录（ACIR 抽象组件 → 具体 C++ 实现的完整契约）
acsim.module      一个 C++ 模块类
  acsim.instance    成员实例
  acsim.array       成员数组
  acsim.element     取数组元素
  acsim.port        取端口访问器
  acsim.resource    取资源访问器
  acsim.bind        连接两个对象
  acsim.inline      内联调用（纯函数）
  acsim.invoke      调用（有副作用）
  acsim.export      导出成员给外部
  acsim.process     进程状态机（多个 region，每个是一个 PC 状态）
    acsim.live.load / acsim.live.store   跨状态活跃变量的存取
    acsim.continue    跳到另一个 PC（同一激活内）
    acsim.suspend     挂起，等待 wake 后回到某个 PC
    acsim.terminate   终止
  acsim.return
acsim.dispatch    派发表的一行：对象 ID + 四个函数指针（work/xfer/reset/validate）
acsim.activate    激活边：某个激活源可以唤醒某个对象
```

### 4.2 六类指纹

`acsim.model` 上挂着六个 SHA-256 指纹，它们共同构成一条**信任链**：

| 指纹 | 覆盖内容 | 作用 |
|---|---|---|
| `frozen_acir` | 冻结后的 ACIR 全文 | 输入没被篡改 |
| `binding_lock` | 所有绑定记录 | 绑定选择是确定的 |
| `provider` | 提供方标识集合 | C++ 实现来源固定 |
| `schema_set` | 组件 schema 集合 | 接口定义没变 |
| `profile` | 构建 profile（fast/validated） | 编译配置固定 |
| `toolchain` | 目标三元组 | 目标平台固定 |

任何一环变了，指纹就变，下游能立刻发现。这也是增量构建缓存键的来源。

### 4.3 进程状态机的表达

ACIR 里的进程是一段"看起来像顺序代码"的东西，但仿真器不能真的用线程去跑它（那样会有调度不确定性）。ACSim 把它编译成**显式的 PC 状态机**：

```mlir
acsim.process @tick captures() names [] entry @entry pcs [@entry] live [] fairness 2 ... {
state @entry {
  %2 = acsim.invoke @acir_impl_wake_next_delta_63cacba5...() : () -> !acsim.wake<@acir_wake_next_delta>
  acsim.suspend @entry on %2 : !acsim.wake<@acir_wake_next_delta>
}
}
```

- `pcs [...]`：所有 PC 状态的列表，每个对应一个 region
- `entry @entry`：入口 PC
- `live [...]`：跨状态活跃的变量槽（这个例子是空的，因为没有跨状态变量）
- `fairness 2`：公平度上限，一次激活内最多执行的工作步数

这样运行时就是一个 `switch (pc_)`，没有协程、没有线程、没有栈切换。

---

## 5. 用例一：pipeline.mlir 逐行精讲

这个例子展示了**一个完整的架构描述能写成什么样**：生产者 → 队列 → 消费者，带协议、资源、统计。它能通过 `ac-verify-model` 全模型校验，但**不能降级到 ACSim**（原因见 5.5）。

完整代码：

```mlir
builtin.module attributes {ac.contract_epoch = "0.1"} {
  ac.protocol @ready_valid {
    ac.role @producer dual @consumer cardinality "exclusive"
    ac.role @consumer dual @producer cardinality "exclusive"
    ac.state @idle initial true terminal false
    ac.state @transferred initial false terminal true
    ac.event @offer from @producer to @consumer payload i32 action "offer"
    ac.transition from @idle to @transferred on @offer transfer true retain false guard {}
    ac.guarantee "ordering" = "fifo"
    ac.guarantee "backpressure" = "none"
  }

  ac.module @Pipeline() parameters {} graph {
    ac.time_domain @core period 1 phase 0 scale 1

    ac.queue @stage payload i32 entries 4 ordering "fifo" protocol @ready_valid
        ownership "exclusive" id "stage" path "stage"

    ac.resource @alu capacity 2 issue_width 1 ii 1
        latency {kind = "fixed", ticks = 3 : i64}
        lifecycle {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}
        ownership "exclusive" classes [] id "alu" path "alu"

    ac.stat @produced kind "counter"
    ac.stat @consumed kind "counter"

    ac.process @source kind "workload" {
      %payload = arith.constant 7 : i32
      %one = arith.constant 1 : i64
      %accepted = ac.try_send @stage %payload : i32
      scf.if %accepted {
        ac.stat.add @produced %one : i64
      }
      ac.yield_sim
    }

    ac.process @drain kind "control" {
      %one = arith.constant 1 : i64
      %value, %received = ac.try_recv @stage : i32
      scf.if %received {
        ac.wait_for @alu
        ac.stat.add @consumed %one : i64
      }
      ac.yield_sim
    }

    ac.return
  }

  ac.system @soc root @Pipeline as "root" tick 0 "cycle"
      workload @Pipeline::@source seed {kind = "fixed", value = 42 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
}
```

### 5.1 模块头

```mlir
builtin.module attributes {ac.contract_epoch = "0.1"} {
```

顶层是标准的 MLIR `builtin.module`，ACIR 不自己定义顶层容器。属性 `ac.contract_epoch = "0.1"` 是**必需的**——这是契约锁步机制的入口，所有 pass 都会检查它。去掉它，降级会报 `ACLOWER-EPOCH-MISMATCH`。

### 5.2 协议定义：`ac.protocol @ready_valid`

协议是一个符号表容器，内部是**图区域**（graph region），意味着里面的算子没有顺序依赖，可以互相前向引用。

```mlir
ac.role @producer dual @consumer cardinality "exclusive"
ac.role @consumer dual @producer cardinality "exclusive"
```

声明两个角色，互为对偶（dual）。验证器强制：

1. `dual` 必须指向本协议内存在的另一个角色
2. **对偶关系必须对称**——`@producer` 的 dual 是 `@consumer`，那 `@consumer` 的 dual 必须是 `@producer`
3. **两侧的 `cardinality` 必须一致**。实测把其中一行改成 `"shared"`：

```
error: 'ac.role' op dual roles must have matching cardinality
```

`cardinality` 只有两个合法值：

- `"exclusive"`：该角色至多有一个结构性使用者（点对点连接）
- `"shared"`：可以有多个使用者，但必须存在一个确定的仲裁所有者

改成别的值直接拒绝：

```
error: 'ac.role' op unsupported role cardinality 'many'
```

---

```mlir
ac.state @idle initial true terminal false
ac.state @transferred initial false terminal true
```

协议状态机的两个状态。`initial` 标记起始状态，`terminal` 标记终止状态。这里描述的是"一次传输"的生命周期：从 `idle` 开始，传输完成后进入 `transferred`（终态）。

注意这描述的是**单次事务的状态**，不是队列的状态。协议状态机用于验证交互序列的合法性。

---

```mlir
ac.event @offer from @producer to @consumer payload i32 action "offer"
```

一个事件：由 `@producer` 发往 `@consumer`，携带 `i32` 类型的数据，动作类别是 `"offer"`。

`action` 是硬编码枚举，共 7 个合法值：

| action | 含义 |
|---|---|
| `offer` | 提议投递（携带 payload，涉及所有权转移） |
| `accept` | 接受 |
| `reject` | 拒绝 |
| `cancel` | 取消 |
| `retry` | 重试（必须配 `retain`，不能配 `transfer`） |
| `response` | 响应（携带 payload） |
| `notify` | 通知（携带 payload） |

其中 `offer`、`response`、`notify` 被称为**载体动作（carrier action）**——只有它们能真正承载 payload。这一点很重要：后面 `ac.queue` 声明 `payload i32 protocol @ready_valid` 时，验证器会去协议里找**是否存在一个载体动作事件，其 payload 类型匹配 i32**。找不到就报错。所以协议里必须至少有一个 `offer`/`response`/`notify` 事件的 payload 是 `i32`。

---

```mlir
ac.transition from @idle to @transferred on @offer transfer true retain false guard {}
```

状态转移：在 `@offer` 事件发生时，从 `@idle` 转到 `@transferred`。

- `transfer true`：这次转移**转移了 payload 的所有权**
- `retain false`：不保留 payload（不用于重试）
- `guard {}`：守卫条件区域，这里是空的（无条件转移）。非空时里面放计算布尔值的代码。

`transfer` 和 `retain` 的组合有严格约束，因为规范要求"一个 offered packet 必须被恰好一次转移、显式取消、拒绝、或为重试而保留"——不能悄悄丢失也不能重复。验证器强制：

- `offer` 事件如果没有 `transfer`，需要有对应的处理路径
- `retry` 事件必须 `retain`，且不能 `transfer`
- `retain` 只能用在 `offer` 和 `retry` 上

这也是为什么我把 `action "offer"` 改成 `"push"` 时，报的不是"未知 action"，而是先撞上了这条转移检查：

```
error: 'ac.transition' op ownership transfer requires a pending offer
```

---

```mlir
ac.guarantee "ordering" = "fifo"
ac.guarantee "backpressure" = "none"
```

协议保证。语法是 `ac.guarantee <kind> = <value>`（注意不是 `kind "x" value "y"`，这是很容易写错的地方）。

合法的 kind 及其取值：

| kind | 合法 value |
|---|---|
| `ordering` | `"fifo"` / `"per_key"` / `"unordered"` |
| `backpressure` | `"none"` / `"accept"` / `"credit"` / `"capacity"` / `"custom"` |
| `delivery` | `"exactly_once"` / `"at_most_once"` / `"best_effort"` |
| `completion` | `"on_accept"` / `"on_response"` / `"on_terminal_phase"` |
| `stable_pending` | 布尔值 |
| `max_inflight` | 任意（验证器不检查） |
| `correlation` | 非空字符串（字段名） |
| `custom_backpressure` | 非空字符串（声明式契约） |

未知 kind 直接拒绝：

```
error: 'ac.guarantee' op unknown mandatory protocol guarantee 'speed'
```

### 5.3 模块与状态声明

```mlir
ac.module @Pipeline() parameters {} graph {
```

模块定义。`()` 是函数类型的参数列表（这里没有端口），`parameters {}` 是静态参数字典（编译期常量，用于特化），`graph` 关键字标识后面是图区域。

图区域意味着：**里面的算子不按书写顺序执行**，它们只是声明。真正的执行顺序由 `ac.process` 内部和仿真器调度决定。

---

```mlir
ac.time_domain @core period 1 phase 0 scale 1
```

时间域。三个整数字段：

- `period 1`：周期，单位是全局 tick，必须为正。域的第 `n` 个周期发生在全局 tick `phase + n * period`
- `phase 0`：相位，从全局 epoch tick 0 起算，必须非负
- `scale 1`：tick 刻度因子，正整数，有实现上限

实测三个都有约束：

```
period 0  → error: 'ac.time_domain' op period must be positive global ticks
phase -1  → error: 'ac.time_domain' op phase must be non-negative global ticks
```

注意：时间域**不等于 RTL 时钟信号**，它只是一个整数时间基准。`period 1 phase 0 scale 1` 就是"跟全局 tick 一对一"。

---

```mlir
ac.queue @stage payload i32 entries 4 ordering "fifo" protocol @ready_valid
    ownership "exclusive" id "stage" path "stage"
```

队列声明，逐字段拆解：

| 字段 | 值 | 说明 |
|---|---|---|
| `payload` | `i32` | 元素类型，必须是规范的 ACIR 值类型 |
| `entries` | `4` | 条目容量，必须为正（`entries 0` → `entry capacity must be positive`） |
| `bytes` | （省略） | 可选的字节容量，若给出必须为正 |
| `ordering` | `"fifo"` | 只能是 `"fifo"` 或 `"per_key"`（**注意：`"unordered"` 在队列上不合法**） |
| `protocol` | `@ready_valid` | 关联协议，用于一致性检查 |
| `ownership` | `"exclusive"` | **只能是 `"exclusive"`**，别无选择 |
| `id` / `path` | `"stage"` | 稳定标识和路径 |
| `watermarks` | （省略） | 可选，见下 |

三条实测的跨字段检查：

```
ordering "unordered"  → error: ordering must be 'fifo' or 'per_key'
ownership "shared"    → error: queue ownership must be exactly 'exclusive'
entries 0             → error: entry capacity must be positive
```

**队列排序不能弱于协议排序**。排序强度是 `fifo(2) > per_key(1) > unordered(0)`。协议声明了 `ordering = "fifo"`，队列就必须也是 `"fifo"`。另外 `per_key` 队列要求协议必须声明了 `correlation` guarantee。

`watermarks` 是可选字典，如果写，必须恰好包含 `low` 和 `high` 两个键，且满足 `0 <= low < high <= entries`。实测：

```mlir
watermarks {low = 1 : i64, high = 3 : i64}   // 合法（entries=4）
watermarks {low = 3 : i64, high = 2 : i64}   // error: watermarks require 0 <= low < high <= entry capacity
watermarks {low = 1 : i64, high = 9 : i64}   // 同上（9 > 4）
```

---

```mlir
ac.resource @alu capacity 2 issue_width 1 ii 1
    latency {kind = "fixed", ticks = 3 : i64}
    lifecycle {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}
    ownership "exclusive" classes [] id "alu" path "alu"
```

资源声明。这是字段最多的一个算子，但每个字段都有明确的建模意义：

**三个吞吐维度**（很容易混淆，重点区分）：

| 字段 | 含义 | 约束 | 本例 |
|---|---|---|---|
| `capacity` | 并发容量 / lane 数，能同时容纳多少个在途请求 | 必须为正 | 2 条 lane |
| `issue_width` | 发射宽度，一个 tick 内最多接纳几个新请求 | 必须在 `[1, capacity]` | 每 tick 最多发射 1 个 |
| `ii` | 发起间隔（initiation interval），两次连续发起之间的最小 tick 数 | 至少 1 | 每 tick 都能发起 |

实测边界：

```
capacity 0             → error: resource capacity must be positive
issue_width 5 (cap=2)  → error: issue width must be in [1, capacity]
ii 0                   → error: initiation interval must be at least one global tick
```

所以这个 ALU 的模型是：**每周期最多接受 1 个新操作，最多 2 个操作同时在流水线里，每个操作耗时 3 个 tick**。

**延迟模型** `latency {kind = "fixed", ticks = 3}`：

只支持两种 kind：

- `"fixed"`：键集必须精确是 `{kind, ticks}`，`ticks` 必须为正
- `"symbol"`：键集必须精确是 `{kind, ref}`，`ref` 指向一个模块，由该模块计算延迟

实测：

```
kind = "table"   → error: latency model kind must be 'fixed' or 'symbol'
ticks = 0        → error: fixed latency ticks must be positive
```

**生命周期** `lifecycle {...}`：

这三个键的值是**完全固定的**，没有选择余地：

```mlir
lifecycle {reservation = "propose_commit", release = "balanced", cancellation = "explicit"}
```

- `reservation = "propose_commit"`：预留必须走"先提议后提交"两阶段
- `release = "balanced"`：释放必须与预留配平
- `cancellation = "explicit"`：取消必须走显式声明的路径

改任何一个值都是同一个错误：

```
release = "eager"  → error: lifecycle requires exact reservation/release/cancellation schema
```

看起来这三个字段没有信息量（既然只有一个合法值，为什么还要写）。设计意图是**强制显式声明**：写代码的人必须知道这个资源遵循两阶段预留、配平释放、显式取消的语义，而不是默认假设。未来扩展新语义时，这些字段就有了区分度。

`ownership "exclusive"`：资源的所有权有三个合法值 `exclusive` / `shared` / `contested`。非 `exclusive` 时**必须**声明一个 `arbiter`（仲裁所有者），`exclusive` 时**不能**声明。

`classes []`：事务类别列表，这里为空。

---

```mlir
ac.stat @produced kind "counter"
ac.stat @consumed kind "counter"
```

统计量声明。`kind` 有四个合法值：`counter`（单调计数）/ `gauge`（可增减的瞬时值）/ `histogram`（分布）/ `event_log`（事件日志）。

```
kind "average"  → error: kind must be 'counter', 'gauge', 'histogram', or 'event_log'
```

### 5.4 进程：真正的行为描述

```mlir
ac.process @source kind "workload" {
  %payload = arith.constant 7 : i32
  %one = arith.constant 1 : i64
  %accepted = ac.try_send @stage %payload : i32
  scf.if %accepted {
    ac.stat.add @produced %one : i64
  }
  ac.yield_sim
}
```

**进程种类** `kind "workload"`，三个合法值：

| kind | 用途 | 特殊约束 |
|---|---|---|
| `control` | 本地调度与编排 | 无 |
| `workload` | trace 或工作负载注入 | 可被 `ac.system` 的 `workload` 字段引用 |
| `monitor` | 非功能性观测与检查 | **不能有功能性副作用** |

`monitor` 的限制是硬性的。把 `@drain` 的 kind 改成 `"monitor"`，立刻报错：

```
error: 'ac.try_recv' op monitor process cannot perform functional state effects
```

具体禁止的是 `ac.try_send` / `ac.try_recv` / `ac.schedule` / `ac.wait_for` 四个算子。这保证了监控代码不会改变被监控系统的行为——观测者效应在设计上就被排除了。

**进程体逐行**：

```mlir
%payload = arith.constant 7 : i32
%one = arith.constant 1 : i64
```

直接复用 MLIR 内建的 `arith` 方言。ACIR 不重新发明常量、算术、控制流，这些直接用标准方言。

```mlir
%accepted = ac.try_send @stage %payload : i32
```

**非阻塞发送**。往队列 `@stage` 里塞一个 `i32`，返回 `i1` 表示是否被接受。注意这里的语义是"提案"——按照三相模型，这个操作在 Work 相里只是提出一个提案，真正入队发生在 Xfer 相。

为什么没有阻塞版本的 `send`？因为阻塞需要挂起进程，而挂起点必须在编译期显式化。ACIR 的设计是：你想阻塞，就自己写 `try_send` + 失败后 `yield_sim`，下个 tick 再试。这样所有挂起点都是可见的。

```mlir
scf.if %accepted {
  ac.stat.add @produced %one : i64
}
```

用标准 `scf.if` 做条件。发送成功才计数。

```mlir
ac.yield_sim
```

**进程体的终结符**。含义是"这一轮激活结束，让出控制权"。每个进程体都必须以它结尾（它有 `Terminator` trait）。

---

```mlir
ac.process @drain kind "control" {
  %one = arith.constant 1 : i64
  %value, %received = ac.try_recv @stage : i32
  scf.if %received {
    ac.wait_for @alu
    ac.stat.add @consumed %one : i64
  }
  ac.yield_sim
}
```

消费者进程。`ac.try_recv @stage : i32` 返回**两个值**：取到的数据 `%value` 和是否取到 `%received`。这是标准的非阻塞出队模式。

`ac.wait_for @alu` 是**等待资源**——请求 ALU 资源，拿不到就挂起。结合前面的资源声明（capacity 2, issue_width 1, latency 3），这里表达的是"消费一个元素需要占用一次 ALU，耗时 3 tick"。

### 5.5 系统声明

```mlir
ac.system @soc root @Pipeline as "root" tick 0 "cycle"
    workload @Pipeline::@source seed {kind = "fixed", value = 42 : i64}
    instrumentation [] results {id = "default", format = "json"} selected true
```

逐字段：

| 字段 | 值 | 约束 |
|---|---|---|
| `root` | `@Pipeline` | 根模块，必须是具体的 `ac.module`（不能是 extern） |
| `as` | `"root"` | 根模块的实例名，所有路径都从这里开始 |
| `tick` | `0` | 全局 tick 纪元，**必须恰好是 0** |
| （tick 单位） | `"cycle"` | 合法值：`cycle` / `ps` / `ns` / `us` / `ms` / `s` |
| `workload` | `@Pipeline::@source` | 主工作负载，必须引用一个 `kind = "workload"` 的进程 |
| `seed` | `{kind = "fixed", value = 42}` | 键集必须精确是这两个，kind **只能是 `"fixed"`**，value 必须是非负 i64 |
| `instrumentation` | `[]` | 仪表化层列表 |
| `results` | `{id = "default", format = "json"}` | 键集必须精确，`format` **只能是 `"json"`** |
| `selected` | `true` | 标记这是被选中要编译的系统 |

实测的约束：

```
tick 3                    → error: global tick epoch must be exactly 0
tick 0 "fortnight"        → error: unsupported exact global tick unit 'fortnight'
seed value = -1           → error: fixed seed value must be a non-negative signless i64
results format = "yaml"   → error: result schema requires exact {id = ..., format = "json"}
kind "driver" (在进程上)   → error: primary workload '@Pipeline::@source' must reference a workload process
```

`@Pipeline::@source` 是**嵌套符号引用**语法：模块 `@Pipeline` 里的 `@source`。

### 5.6 这个例子能做什么、不能做什么

**能**：通过全模型语义校验。

```bash
$ acir-opt pipeline.mlir --ac-verify-model -o /dev/null
# 无输出 = 通过
```

**不能**：降级到 ACSim。

```bash
$ acir-opt pipeline.mlir --ac-verify-model --ac-canonicalize-model \
    --ac-freeze-topology --ac-lower-process-state --ac-lower-to-acsim \
    --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu -o /dev/null
error: yield-only process-state fixture requires exactly one ac.yield_sim per process and no other operations
```

这不是 bug，是 v0.1 降级阶段明确的能力边界。**当前的 `ac-lower-to-acsim` 只支持"仅 yield"形式的进程**：进程体必须是单个基本块、单条指令、且那条指令是 `ac.yield_sim`、且没有捕获变量。

同时被显式拒绝的还有：`ac.queue`、`ac.resource`、`ac.address_map`、`ac.time_domain`、`ac.view`、`ac.instrumentation`。源码里的注释写得很直白——**宁可报错也绝不静默丢弃**：

```
error: ACLOWER-UNSUPPORTED-CONSTRUCT: operation 'ac.queue' has no ACSim realization in the v0.1 lowering stage
```

所以现在的状态是：**ACIR 的表达能力（前端）远远超前于降级能力（后端）**。你可以完整地描述一个架构并通过校验，但只有其中结构性的那部分能走到 ACSim。

---

## 6. 用例二：cluster.mlir 三阶段逐行精讲

这个例子专门设计成**能完整走通 ACIR → 冻结 ACIR → ACSim** 的全流程，用来展示降级到底做了什么。它包含层级嵌套、模块数组、外部模块绑定。

### 6.1 阶段一：源 ACIR

```mlir
builtin.module attributes {ac.contract_epoch = "0.1"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@tick seed {kind = "fixed", value = 1 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true

  ac.module.extern @Leaf : () -> i32 parameters {width = 8 : i64}
      implementation {registry = "cpp", name = "Leaf"}

  ac.module @Cell() parameters {} graph {
    ac.return
  }

  ac.module @Cluster() parameters {} graph {
    ac.array @cells of @Cell shape [2]() static [{}, {}]
        id "cells" path "cells" : () -> ()
    ac.return
  }

  ac.module @Top() parameters {} graph {
    ac.instance @cluster of @Cluster() static {}
        id "cluster" path "cluster" : () -> ()
    %leaf = ac.instance @accel of @Leaf() static {width = 8 : i64}
        id "accel" path "accel" : () -> i32
    ac.process @tick kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}
```

**外部模块声明**：

```mlir
ac.module.extern @Leaf : () -> i32 parameters {width = 8 : i64}
    implementation {registry = "cpp", name = "Leaf"}
```

这声明了一个**由 C++ 实现提供的黑盒模块**。ACIR 只知道它的接口（无输入、输出一个 i32）和静态参数（`width = 8`）。`implementation {registry = "cpp", name = "Leaf"}` 告诉工具链去哪儿找实现。

这里有个**实际的可用性问题**：外部模块的名字必须预先注册到 `StructuralProviderRegistry` 里，而这个注册表**只能通过 C++ API 填充，没有任何命令行或配置文件入口**。公共的 `acir-opt` 会直接拒绝：

```
error: 'ac.module.extern' op structural provider 'cpp:Leaf' is not registered
```

只有内部测试驱动 `acir-opt-internal` 预注册了 `{"A", "B", "Empty", "Ext", "Leaf", "Top"}` 这几个硬编码名字，才能跑通。所以本节的命令用的是 `acir-opt-internal`。这是一个真实的缺口，见 [8.6](#86-可用性缺口)。

**模块数组**：

```mlir
ac.array @cells of @Cell shape [2]() static [{}, {}]
    id "cells" path "cells" : () -> ()
```

实例化 2 个 `@Cell`。`shape [2]` 是编译期形状，`static [{}, {}]` 是每个元素的静态参数（各自一个空字典）。注意静态参数列表的长度必须等于元素总数——**每个元素可以有不同的参数**，但当前降级要求它们特化后必须一致，否则报 `ACLOWER-ARRAY`。

**实例化顺序不重要**：

```mlir
ac.instance @cluster of @Cluster() static {} ...
%leaf = ac.instance @accel of @Leaf() static {width = 8 : i64} ...
```

我在源码里先写 `@cluster` 后写 `@accel`，但因为这是图区域，书写顺序无意义。规范化 pass 会按符号名排序——后面会看到 `@accel` 排到了 `@cluster` 前面。

### 6.2 阶段二：冻结后的 ACIR

运行前四个 pass：

```bash
acir-opt-internal cluster.mlir \
  --ac-verify-model --ac-canonicalize-model \
  --ac-freeze-topology --ac-lower-process-state \
  -o cluster.frozen.mlir
```

模块头变成了这样（为可读性做了换行）：

```mlir
module attributes {
  ac.contract_epoch = "0.1",
  ac.freeze_epoch = "0.1",
  ac.frozen_instrumentation = [],
  ac.frozen_owners = [
    {kind = "ac.system_root", owner = @Top,            path = "root",                    stable_id = "root"},
    {kind = "ac.instance",    owner = @Top::@accel,    path = "root.accel",              stable_id = "root/accel"},
    {kind = "ac.instance",    owner = @Top::@cluster,  path = "root.cluster",            stable_id = "root/cluster"},
    {kind = "ac.array",       owner = @Cluster::@cells, path = "root.cluster.cells",     stable_id = "root/cluster/cells"},
    {kind = "ac.array",       owner = @Cluster::@cells, path = "root.cluster.cells[0]",  stable_id = "root/cluster/cells[0]"},
    {kind = "ac.array",       owner = @Cluster::@cells, path = "root.cluster.cells[1]",  stable_id = "root/cluster/cells[1]"},
    {kind = "ac.process",     owner = @Top::@tick,     path = "root.tick",               stable_id = "root/tick"}
  ],
  ac.frozen_primary_workload = {path = "root.tick", reference = @Top::@tick, stable_id = "root/tick"},
  ac.frozen_system = @soc,
  ac.topology_digest = "7bdf586a8f38efaf593a259611d622555519915c28319415fd9cbaede5e88628",
  ac.topology_frozen = true
} {
```

这就是冻结做的事情：**把整个层级展平成一张完整的对象清单**。

关键观察：

1. **路径被补全成绝对路径**。源码里 `path "accel"` 变成了 `root.accel`——`root` 来自 `ac.system` 的 `as "root"`。
2. **数组被展开**。`ac.array @cells shape [2]` 产生了 3 条记录：数组本身 `root.cluster.cells`，加上两个元素 `cells[0]`、`cells[1]`。
3. **`stable_id` 用 `/` 分隔，`path` 用 `.` 分隔**。这就是 3.3 节说的双标识——一个给人看，一个进哈希。
4. **清单是排序的**。`accel` 在 `cluster` 前面（按名字），`cluster` 的子对象紧跟其后（深度优先）。这个顺序直接决定了后面的构造顺序。
5. **`topology_digest`** 是对整个清单算的 SHA-256。之后任何拓扑改动都会被这个摘要抓到。

模块体内也发生了变化：

```mlir
%0 = ac.instance @accel of @Leaf() static {width = 8 : i64} id "accel" path "accel"
    {ac.frozen_owners = [{kind = "ac.instance", owner = @Top::@accel, path = "root.accel", stable_id = "root/accel"}]}
    : () -> i32
ac.instance @cluster of @Cluster() static {} id "cluster" path "cluster"
    {ac.frozen_owners = [...]} : () -> ()
```

- **顺序被规范化了**：`@accel` 现在排在 `@cluster` 前面
- SSA 值名字从 `%leaf` 变成了 `%0`（规范化会重命名，因为名字不参与语义）
- 每个算子都挂上了自己那份 `ac.frozen_owners` 副本（局部可查，不用回溯到模块头）

进程也被处理了：

```mlir
ac.process @tick kind "workload" {
  ac.yield_sim
} {
  ac.frozen_owners = [{kind = "ac.process", owner = @Top::@tick, path = "root.tick", stable_id = "root/tick"}],
  ac.frozen_process_skeleton = ["process/r0/b0/o0 ac.yield_sim{}props=<<NULL ATTRIBUTE>> operands= results= regions="]
}
```

`ac.frozen_process_skeleton` 是 `ac-lower-process-state` 产生的**进程骨架签名**：`r0/b0/o0` 表示 region 0、block 0、operation 0，后面是该算子的完整结构描述。这个签名用于验证进程结构在后续 pass 中没有被改动。

### 6.3 阶段三：降级到 ACSim

```bash
acir-opt-internal cluster.frozen.mlir --ac-lower-to-acsim \
  --ac-binding-registry=test/Conversion/Inputs/stateful-fast.json \
  --ac-binding-profile=fast \
  --ac-binding-target=arm64-apple-darwin
```

`--ac-binding-profile` 和 `--ac-binding-target` 是**无条件必需的**，少一个就报：

```
error: ACLOWER-BINDING-OPTIONS: --ac-lower-to-acsim requires --ac-binding-profile
```

`--ac-binding-registry` 只在模型里**有外部模块需要解析**时才需要。像 [9.6](#96-写第一个模型的最小骨架) 那样不含 `ac.module.extern` 的模型，不传注册表也能降级成功。

#### 6.3.1 模型头

```mlir
acsim.model @soc epoch "0.1" root @Top
  construction ["Top.accel", "Top.cluster", "Top.cluster.cells[0]", "Top.cluster.cells[1]", "Top.tick"]
  destruction  ["Top.tick", "Top.cluster.cells[1]", "Top.cluster.cells[0]", "Top.cluster", "Top.accel"]
  fingerprints {
    binding_lock = "sha256:3501c277a3e1e0dc...",
    frozen_acir  = "sha256:83ed0b97d6d6ed9c...",
    profile      = "sha256:079c9d12005aad81...",
    provider     = "sha256:fd2999ce2673a3f9...",
    schema_set   = "sha256:b8700691cf07dd73...",
    toolchain    = "sha256:1db0fba21ec705a3..."
  } {
```

**构造顺序**是这个 IR 里最重要的信息之一。它直接对应生成的 C++ 里成员初始化的顺序：

```
Top.accel → Top.cluster → Top.cluster.cells[0] → Top.cluster.cells[1] → Top.tick
```

注意路径前缀从 `root.` 变成了 `Top.`——ACSim 用的是**类型层级路径**（根模块的符号名），而 ACIR 冻结时用的是**实例层级路径**（`ac.system` 指定的实例名 `root`）。

**析构顺序严格是构造顺序的逆序**。这不是巧合，是 C++ 对象生命周期的要求，也被 verifier 强制。

六个指纹前面 4.2 节讲过。这里可以验证确定性：连跑两次降级，输出的每一个指纹都完全一样。

#### 6.3.2 类型声明

```mlir
acsim.type @ac_std_Leaf cpp "ac.std.Leaf" kind "schema"
    fingerprint "sha256:1111111111111111..."
acsim.type @acir_impl_wake_next_delta_63cacba5... cpp "acir::generated::impl_wake_next_delta_63cacba5..." kind "implementation"
    fingerprint "sha256:63cacba5c3eb8297..."
acsim.type @acir_wake_next_delta cpp "acir::generated::wake_next_delta" kind "wake"
    fingerprint "sha256:8cf214054e3ad1f4..."
acsim.type @cpp_i32 cpp "cpp_i32" kind "value"
    fingerprint "sha256:36c7898add8aafb1..."
acsim.type @gfsim cpp "gfsim" kind "provider"
    fingerprint "sha256:e9df42d11b8581ed..."
acsim.type @gfsim_Leaf cpp "gfsim.Leaf" kind "implementation"
    fingerprint "sha256:2222222222222222..."
```

每个 `acsim.type` 是一个"C++ 世界里的名字"。`kind` 区分它扮演的角色：

| kind | 含义 | 本例 |
|---|---|---|
| `schema` | 组件 schema（抽象接口定义） | `ac.std.Leaf` |
| `implementation` | 具体实现 | `gfsim.Leaf`、生成的 wake 实现 |
| `provider` | 实现提供方 | `gfsim` |
| `value` | 值类型 | `cpp_i32`（对应 ACIR 的 `i32`） |
| `wake` | 唤醒类型 | `acir::generated::wake_next_delta` |

**这个列表是按符号名排序的**（`ac_std_Leaf` < `acir_impl...` < `acir_wake...` < `cpp_i32` < `gfsim` < `gfsim_Leaf`），又一次体现确定性原则。

那两个 `wake` 相关的类型是**降级过程生成的**，源 ACIR 里没有。它们来自 `@tick` 进程里那句 `ac.yield_sim`——"让出"需要一个"什么时候醒来"的机制，降级把它实现成"下一个 delta 唤醒"。类型名里嵌的 `63cacba5...` 就是它自己的指纹，保证同样的语义总是生成同样的名字。

#### 6.3.3 绑定记录

```mlir
acsim.binding @Leaf record {
  activation_sources = [],
  availability = "available",
  binding = "Leaf",
  binding_schema = "acsim-binding-0.1",
  component_schema = @ac_std_Leaf,
  component_schema_fingerprint = "sha256:1111...",
  construction = {arguments = [8], kind = "constructor"},
  contract_epoch = "0.1",
  cpp = {
    concept = "gfsim::StatefulModel",
    entry_points = {
      pure = "",
      reset = "gfsim::leaf_reset",
      validate = "gfsim::leaf_validate",
      work = "gfsim::leaf_work",
      xfer = "gfsim::leaf_xfer"
    },
    header = "gfsim/leaf.hpp",
    symbol = "gfsim::Leaf",
    target = "gfsim"
  },
  cpp_type = @cpp_i32,
  effect = "stateful",
  fingerprint = "sha256:c8e227f956974726...",
  implementation = @gfsim_Leaf,
  ownership = {kind = "unique", placement = "member_or_array"},
  parameters = [{
    acir_type = "i64", cpp_type = "std::int64_t",
    mapping = "constructor_constant", name = "width",
    ordinal = 0 : i64, value = 8 : i64
  }],
  ports = [],
  provider = @gfsim,
  provider_implementation_fingerprint = "sha256:2222...",
  resources = [],
  results = [{cpp_type = @cpp_i32, name = "result"}]
}
```

这条记录是整个降级过程的核心产出：**把 ACIR 里抽象的 `@Leaf` 完全钉死到一个具体的 C++ 实现上**。它是从绑定注册表 JSON 里查出来的，查询的键是"组件 schema + profile + target"。

逐块解读：

- **`construction = {arguments = [8], kind = "constructor"}`**：怎么构造这个对象——调构造函数，传一个参数 `8`。这个 `8` 从哪来？来自 ACIR 里 `ac.instance @accel of @Leaf() static {width = 8 : i64}` 的静态参数。
- **`parameters = [...]`**：静态参数的完整映射链条——ACIR 里叫 `width`、类型是 `i64`、值是 `8`；到了 C++ 里类型是 `std::int64_t`、以 `constructor_constant` 方式传入、位置是第 0 个。`ordinal` 保证了参数顺序确定。
- **`cpp.entry_points`**：四个函数指针，对应仿真三相 + 两个辅助操作。`pure = ""` 表示这不是纯函数组件（因为 `effect = "stateful"`）。
- **`cpp.concept = "gfsim::StatefulModel"`**：C++20 concept，生成的代码会用它做静态约束检查。
- **`ownership = {kind = "unique", placement = "member_or_array"}`**：所有权语义——独占持有，可以放在成员位置或数组里。这决定了生成的 C++ 里用什么持有方式（直接成员而不是 `unique_ptr`）。
- **`effect = "stateful"`**：有状态。有状态组件才需要 `reset` 和三相回调。

对照一下注册表 JSON 里对应的 candidate 条目，会发现绑定记录几乎是原样搬过来的，只是把字符串换成了符号引用（`component_schema = @ac_std_Leaf` 而不是 `"ac.std.Leaf"`）。这样 MLIR 的符号机制就能做引用完整性检查。

#### 6.3.4 模块定义

```mlir
acsim.module @Cell interface {ports = [], resources = [], results = []}
    static [] specialization "sha256:5233a79e95bc6a2e..." exports [] {
  acsim.return
}
```

空模块。`specialization` 指纹标识这个"模块 + 静态参数组合"的唯一特化版本。

```mlir
acsim.module @Cluster interface {...} static []
    specialization "sha256:5a0bb8a0e61e3ca6..." exports [] {
  %0 = acsim.array @cells target @Cell args []
      specialization "sha256:5233a79e95bc6a2e..." shape [2]
      : !acsim.array<[2], !acsim.owner<@Cell>>
  acsim.return
}
```

数组成员。类型 `!acsim.array<[2], !acsim.owner<@Cell>>` 精确表达了 C++ 里的 `std::array<Cell, 2>`（`owner` 表示值语义持有，不是指针）。

注意 `acsim.array` 引用的 `specialization "sha256:5233a79e..."` **和 `@Cell` 模块自己的 specialization 指纹相同**——因为两个元素用的都是同一个特化版本。如果数组元素参数不同、特化不同，v0.1 降级会拒绝（`ACLOWER-ARRAY`）。

```mlir
acsim.module @Top interface {...} static []
    specialization "sha256:a6560ab447fa7281..." exports [] {
  %0 = acsim.instance @accel target @Leaf args [8]
      specialization "sha256:d4d9a91ee3469310..." : !acsim.owner<@Leaf>
  %1 = acsim.instance @cluster target @Cluster args []
      specialization "sha256:5a0bb8a0e61e3ca6..." : !acsim.owner<@Cluster>
  ...
}
```

两个实例成员。`args [8]` 就是构造参数——从绑定记录的 `construction.arguments` 来的。到这里，`width = 8` 这个信息已经从"ACIR 的静态参数字典"变成了"C++ 构造函数的第 0 个实参"。

**关键观察**：`ac.module.extern @Leaf` 在 ACSim 里已经完全消失了，取而代之的是 `acsim.binding @Leaf` + `acsim.instance ... target @Leaf`。抽象声明被替换成了具体绑定。这正是 `test/Conversion/bindings.mlir` 里 `// CHECK-NOT: ac.module.extern` 这一行在守卫的东西。

#### 6.3.5 进程状态机

```mlir
acsim.process @tick captures() names [] entry @entry pcs [@entry] live []
    fairness 2 specialization "sha256:e2819e9651e58ea1..." {
state @entry {
  %2 = acsim.invoke @acir_impl_wake_next_delta_63cacba5...()
      : () -> !acsim.wake<@acir_wake_next_delta>
  acsim.suspend @entry on %2 : !acsim.wake<@acir_wake_next_delta>
}
}
```

源 ACIR 里这个进程只有一句 `ac.yield_sim`。降级把它变成了一个**单状态的状态机**：

| 字段 | 值 | 含义 |
|---|---|---|
| `captures()` / `names []` | 空 | 没有捕获外部变量（v0.1 也不支持捕获） |
| `entry @entry` | | 入口 PC 是 `@entry` |
| `pcs [@entry]` | | 一共只有一个 PC |
| `live []` | 空 | 没有跨状态活跃变量，所以不需要活跃槽 |
| `fairness 2` | | 公平度上限 |

`fairness` 的计算规则是 `max(计划的工作步数, 2)`。这里状态体里有 2 条指令（invoke + suspend），所以是 2。ACSim 的 verifier 会检查"最长执行路径 ≤ fairness_cap"，防止一次激活里执行无界的工作量——这是**保证仿真推进的机制**：每个进程每次激活的工作量有上界，就不会出现某个进程饿死其他进程。

状态体两句话：

1. `acsim.invoke @acir_impl_wake_next_delta_...()` —— 调用生成的 helper，得到一个"下一个 delta 唤醒"的 wake 对象
2. `acsim.suspend @entry on %2` —— 挂起，等这个 wake 触发后**回到 `@entry`**（也就是自己）

翻译成人话：`ac.yield_sim` 的语义是"我这轮没事干了，下一个时间步再叫我，从头开始"。生成的 C++ 大概是：

```cpp
void doWork(Epoch epoch) {
  switch (pc_) {
    case Pc::entry:
      scheduleWake(nextDelta());
      pc_ = Pc::entry;   // suspend 回到 entry
      return;
  }
}
```

#### 6.3.6 派发表与激活图

```mlir
%object, %activation = acsim.dispatch @Top::@accel path "Top.accel" indices []
    object 0 activation 0
    work     "gfsim::leaf_work"
    xfer     "gfsim::leaf_xfer"
    reset    "gfsim::leaf_reset"
    validate "gfsim::leaf_validate"
    : !acsim.object_id, !acsim.activation_id
```

派发表的一行。这就是"静态派发"的物理形式：

- `object 0`：这个对象在运行时的数值 ID（**编译期分配**，不是运行时查表）
- `activation 0`：它的激活槽 ID
- 四个字符串是**四个 C++ 函数的完全限定名**，直接来自绑定记录的 `cpp.entry_points`

`@accel` 是外部绑定组件，所以四个 thunk 是注册表里给的 `gfsim::leaf_*`。

```mlir
%object_0, %activation_1 = acsim.dispatch @Top::@tick path "Top.tick" indices []
    object 1 activation 1
    work     "acsim_generated::Top::sa6560ab447fa7281.../tick::pe2819e9651e58ea1.../work"
    xfer     "..." reset "..." validate "..."
```

（原文是连续的 `::` 分隔，这里为可读性做了简化。）

`@tick` 是 ACIR 里定义的进程，没有现成的 C++ 实现，所以四个 thunk 指向**将要生成的代码**，命名空间是 `acsim_generated`。名字的结构是：

```
acsim_generated :: <模块名> :: s<模块特化指纹> :: <进程名> :: p<进程特化指纹> :: <相位>
```

把指纹嵌进符号名，是为了**让不同特化版本的同名进程在链接期天然不冲突**。同一个模块用不同静态参数实例化两次，生成两套函数，名字自动区分。

```mlir
acsim.activate %activation to %object : !acsim.activation_id to !acsim.object_id
acsim.activate %activation_1 to %object_0 : !acsim.activation_id to !acsim.object_id
```

激活边：`activation 0` 可以唤醒 `object 0`，`activation 1` 可以唤醒 `object 1`。这里都是自激活（对象唤醒自己）。

激活图的用途是**剪枝**：仿真器只需要遍历"可能被唤醒的对象"，而不是每个 tick 扫描全部对象。在大模型上这是关键的性能优化——不过如 [8.2](#82-gfsim-运行时phase-2有骨架未接通) 所述，运行时目前还没有用上这张图。

### 6.4 三个阶段的信息量对比

| | 源 ACIR | 冻结 ACIR | ACSim |
|---|---|---|---|
| 文件大小 | 935 B | 2727 B | 6003 B |
| 对象是否有数值 ID | 无 | 无 | 有（object 0/1） |
| 数组是否展开 | 否（`shape [2]`） | 是（清单里有 `cells[0]`/`cells[1]`） | 是（构造顺序里） |
| 外部模块 | 抽象声明 | 抽象声明 | 完整绑定记录 |
| 进程 | 一句 `ac.yield_sim` | 加了骨架签名 | 显式 PC 状态机 |
| 函数入口 | 无 | 无 | 四个具名 thunk |
| 指纹 | 无 | topology_digest | 六类 + 每个特化 |

每一步都在**丢弃抽象、增加确定的实现细节**。这就是编译。

---

## 7. 当前完成度盘点

### 7.1 按路线图阶段

路线图见 `docs/superpowers/plans/2026-08-04-agentic-circuit-roadmap.md`。

| 阶段 | 内容 | 状态 |
|---|---|---|
| **Phase 0** | 开源基线（治理、依赖锁、CI、CMake preset） | ✅ 完成 |
| **Phase 1** | ACIR/ACSim 方言 + 降级 pass | ✅ 完成（但降级能力面窄，见 8.5） |
| **Phase 2** | gfsim 运行时 + ACIR 标准库 | ⚠️ header 骨架，未接通 |
| **Phase 3** | 绑定解析 + 结构化 C++ 生成 | ⚠️ 绑定解析已完成；C++ 生成只有骨架 |
| **Phase 4** | ACPy / Python 前端 / CLI | ❌ 完全没有 |
| **Phase 5** | 端到端模型 + PTO trace + 可视化 | ❌ 完全没有 |
| **Phase 6** | 发布审计 | ❌ 未开始 |

### 7.2 已经能实际使用的能力

这些是**验证过、有测试守卫、可以现在就用**的：

1. **完整的 ACIR 语义校验**。约 50 个算子、20 多种类型，验证器覆盖了跨算子的一致性检查（协议 ↔ 队列、资源 ↔ 仲裁、系统 ↔ 工作负载）。你可以用它当一个"架构描述的类型检查器"。
2. **拓扑冻结与摘要**。层级展平、路径分配、SHA-256 摘要、篡改检测。
3. **进程状态规划**。`ac-lower-process-state` 能分析进程结构，规划 PC、活跃槽、公平度。
4. **绑定解析**。从 JSON 注册表按 profile + target 查找候选，检查可用性、歧义、指纹一致性，产出锁定记录。
5. **ACIR → ACSim 降级**（结构性子集）。模块层级、实例、数组、外部绑定、yield-only 进程、派发表、激活图。
6. **确定性保证**。同一输入连跑两次，输出逐字节相同（有测试守卫）。

测试规模：67 个 lit 测试 + 10 个单元测试可执行文件（约 267 个 gtest 用例），全部通过。

---

## 8. 框架还缺什么

这一章按"缺失的严重程度和影响面"组织，每条都注明了对应的规范文档和实际状态。

### 8.1 Python 前端与 CLI（Phase 4）：完全没有

**这是当前最大的缺口。** 项目的核心卖点是"用 Python 描述架构"，但这部分**一行产品代码都没有**。

规范里定义得很详细：

- `docs/specs/python-to-acir-lowering-v0.1.md` 定义了受限 Python AST 的捕获、校验、归一化，以及语义中间形式 `acpy`
- `docs/specs/agentic-python-cli-v0.1.md` 规定了必须暴露的公共 API 名字：`@system`、`@module`、`@extern_module`、`@generated_module`、`@struct`、`@packet`、`@transaction`、`@protocol`
- `schemas/acpy.schema.json` 是 `acpy` 的机器可读定义

实际情况：

- 全仓库只有 9 个 `.py` 文件，**全部是测试脚本或构建脚本**，没有任何包、模块、装饰器、AST 处理代码
  - `tests/contracts/test_contracts.py`、`tests/contracts/test_ir_coverage.py`：契约与覆盖率校验
  - `tests/test_build_config.py`：CMake 配置回归测试
  - `scripts/check-contracts.py`、`scripts/check-ir-coverage.py`、`scripts/audit-op-coverage.py`：CI 检查脚本
  - `test/lit.cfg.py`：lit 配置
- `pyproject.toml` 里 `dependencies = []`，没有源码包声明
- `schemas/acpy.schema.json` 存在，但**没有任何代码读取或生成它**

**CLI 同样是零**。规范定义了 10 个命令：

```
init  schema  check  elaborate  compile  build  run  inspect  explain  doctor
```

一个都没实现。`pyproject.toml` 里没有 `[project.scripts]`，没有 entry point，没有 `agentic-circuit` 或 `acirc` 可执行入口，没有任何命令分发代码。

**影响**：现在只能手写 MLIR 文本，用 `acir-opt` 驱动。对于"让架构师用 Python 描述架构"这个目标来说，等于起点还没建好。

### 8.2 gfsim 运行时（Phase 2）：有骨架，未接通

`include/gfsim/` 基本是 header-only 的骨架，`lib/gfsim/` 只有一个 `system.cpp`。**唯一的消费者是单元测试**——没有任何生成的代码或 CLI 在驱动它。

**已经有的**：

| 机制 | 状态 | 位置 |
|---|---|---|
| 精确全局时间 `(tick, delta)` | ✅ 实现 | `include/gfsim/core.h` 的 `Epoch` |
| Work → Arbitrate → Xfer 三相屏障 | ⚠️ 骨架 | `lib/gfsim/system.cpp` 的 `step()` |
| 事件调度器 | ⚠️ 骨架 | `SimSystem::step` + `EventQueue` |
| 队列 / 事件队列 | ⚠️ 骨架 | `include/gfsim/queue.h` |
| 资源 + 仲裁 | ⚠️ 骨架 | `include/gfsim/resource.h` |
| 协议状态（credit / in-flight） | ⚠️ 简化骨架 | `components.h` 的 `ProtocolState` |
| 终止结果 | ⚠️ 部分 | `TerminationResult`，能产出 Completed/Incomplete/Failed |

**完全没有的**：

| 缺失项 | 说明 |
|---|---|
| **静态派发表** | ACSim 辛辛苦苦生成了 `acsim.dispatch` 表，但运行时的 `SimSystem::step` 用的是 `lookup(id)->doWork()` **虚函数动态派发**。这跟"静态派发"的设计目标是相悖的 |
| **激活邻接图** | `run()` 目前把所有 Process/TraceSource **无条件全部入队**，没有用 `acsim.activate` 产生的激活图做剪枝 |
| **Scheduler 组件** | 规范要求的 7 个组件（TraceSource/Queue/Scheduler/Compute/Link/Memory/Sink）里，**唯独 Scheduler 没有对应的类**，`ObjectKind` 枚举里也没有它 |
| **真实的 Packet 序列化** | `PacketTraits` 里 `serializedSize = 0`、`schema = nullptr`，只是个 concept 骨架 |
| **统计汇总与导出** | 只有各组件内部的计数器，没有汇总、没有按 `result_schema` 导出 JSON |
| **无进展诊断** | `NoProgressReport` 结构体定义了但**从来没有被填充过** |
| **PTO trace 读取** | 见 8.3 |

还有一个共性问题：**所有基础组件都用 `uint64_t` 当占位数据类型**，而不是规范化的 Packet。也就是说组件模板存在，但还没有和真实的类型系统对接。

### 8.3 PTO trace：完全没有实现

`docs/specs/pto-trace-schema-v0.1.md` 和 `schemas/pto-trace.schema.json` 定义了生成的仿真器要吃的 JSON trace 格式：信封字段 `schema` / `version` / `contract_epoch` / `metadata` / `records`，每条 record 是一个根事务且必须带 `sequence_id`。规范还特别要求 JSON 解析要隔离在 trace 子系统里，组件只消费解码后的强类型事务。

实际：**没有 JSON 解析器，没有 trace 子系统，没有 record 解码器**。`TraceSource` 组件里只有一个 `cursor_` 计数器，`hasRecord()` 恒返回 false，`peekRecord()` 恒返回 0——它不读文件也不解析任何东西。

ACIR 层的 `ac.trace.open` / `ac.trace.next` / `ac.trace.decode` / `ac.trace.eof` / `ac.trace.position` 这五个算子已经定义好了，但没有运行时支撑。

### 8.4 ACIR 标准库 `ac.std`：完全没有实现

`docs/specs/acir-stdlib-v0.1.md` 规定了 `ac.std` 命名空间下的可分析组件族及其 C++20 绑定：compute、scheduler、link、bus、crossbar、router、DMA、storage、cache、protocol adapter。

实际：全仓库搜 `ac.std`，命中的**全部是测试夹具 JSON 和规范文档**，把 `ac.std.Leaf` 之类当字符串字面量用。**没有任何标准库组件的 schema 定义文件，也没有 C++ 实现**。

影响：现在每个外部组件都得自己手写绑定注册表 JSON。标准库的意义就是提供开箱即用的常见组件，这部分等于零。

### 8.5 ACIR → ACSim 降级的能力缺口

Phase 1 的核心通路是通的，但**能覆盖的 ACIR 子集很窄**。所有拒绝都带 `ACLOWER-*` 错误码，源码在 `lib/Conversion/ACIRToACSim/ACIRToACSim.cpp`。

**被显式拒绝的 ACIR 构造**（错误码 `ACLOWER-UNSUPPORTED-CONSTRUCT`）：

```
ac.queue            队列
ac.resource         资源
ac.address_map      地址映射
ac.time_domain      时间域
ac.view             数组视图
ac.instrumentation  仪表化
ac.module.generated 生成器模块
```

源码注释明确写着这些构造**宁可报错也绝不静默丢弃**——这是个好的工程决策，但也意味着[用例一](#5-用例一pipelinemlir-逐行精讲)那样的模型现在走不到 ACSim。

**进程降级只支持一种形式**（错误码 `ACLOWER-PROCESS-STATE`）：

进程体必须是单个基本块、单条指令、那条指令是 `ac.yield_sim`、且没有捕获变量。也就是说：

```mlir
ac.process @tick kind "workload" { ac.yield_sim }        // ✅ 能降级
ac.process @src  kind "workload" {                        // ❌ 不能
  %x = arith.constant 1 : i32
  ac.yield_sim
}
```

所有真正有行为的进程都不能降级。

**其他错误码一览**：

| 错误码 | 拒绝的情况 |
|---|---|
| `ACLOWER-ARRAY` | 数组静态参数不是具体字典；数组元素特化不一致（异构数组） |
| `ACLOWER-TYPE-MISMATCH` | 模块有返回值（`ac.return` 带操作数）；类型不匹配 |
| `ACLOWER-OWNERSHIP` | 不是恰好一个 `selected` 的 `ac.system`；根不是具体 `ac.module` |
| `ACLOWER-BINDING-MISSING` / `-AMBIGUOUS` | 绑定查不到 / 有歧义 |
| `ACLOWER-EPOCH-MISMATCH` | 纪元不是 "0.1"，或拓扑没冻结 |
| `ACLOWER-PROFILE` | 缺少 build profile 或 toolchain target |
| `ACLOWER-FINGERPRINT` | 指纹不一致 |
| `ACLOWER-PARAM-PHASE` | 不支持的静态属性类型 |
| `ACLOWER-BINDING-OPTIONS` | 缺少必需的命令行绑定选项 |

### 8.6 C++ 代码生成（Phase 3）：有骨架，没接通

`lib/CodeGen/` 有两个文件：

**`Emitter.cpp`** 实现了一个**通用的 C++ 文本发射器** `CppEmitter`（namespace / class / method / enum / switch 的字符串拼接），加上四个生成函数：`generateProcessHeader`、`generateProcessSource`、`generateModuleHeader`、`generateModuleSource`。

问题是：**这些函数不消费任何 ACSim IR**。它们的入参是 `std::vector<std::string>`，产出的是占位骨架。比如进程的状态机体只写注释：

```cpp
e.emitCase("Pc::" + pcNames[i]);
e.emitStatement("// state machine logic for " + pcNames[i]);
e.emitBreak();
```

模块的 `build()` 也把所有子对象硬编码成 `ObjectKind::Compute`。

**`Manifest.cpp`** 实现了 SHA-256 指纹工具、`BuildManifest::finalize`、`CacheKey`、原子写出。

问题是：**产出的 manifest 不符合规范 schema**。`schemas/build-manifest.schema.json` 要求 19 个必填字段（`project`、`system`、`source_files`、`normalized_acir_sha256`、`compiler`、`pass_pipeline`、`providers`、`component_specializations`、`protocol_identities`、`artifacts`、`validation_gates`、`build_profile`、...），而实际只写出 5 个（`contract_epoch`、`schema`、`input_fingerprint`、`output_fingerprint`、`sources`），字段名也对不上。

**最关键的是：没有任何地方把 ACSim IR 喂给代码生成器。** 唯一的调用方是单元测试 `unittests/CodeGen/CodeGenTest.cpp`。`acir-opt` 里没有注册任何 ACSim → C++ 的 pass，`ACIRCodeGen` 库甚至没有被 `acir-opt` 链接。

所以 ACSim 目前是**流水线的终点，而不是中间产物**。

### 8.7 可用性缺口

这些不是"功能没做"，而是"做了但用不了"：

**外部模块无法在公共工具里使用**。`ac.module.extern` 引用的 provider 名字必须预先注册到 `StructuralProviderRegistry`，而这个注册表**只能通过 C++ API 填充**（`getStructuralProviderRegistry(context).registerExternal(name)`），没有命令行选项、没有配置文件、没有环境变量。

结果是公共的 `acir-opt` 对任何 extern 模块都会拒绝：

```
error: 'ac.module.extern' op structural provider 'cpp:Leaf' is not registered
```

只有内部测试驱动 `acir-opt-internal` 硬编码注册了 6 个名字（`A`、`B`、`Empty`、`Ext`、`Leaf`、`Top`）。这意味着**外部用户目前根本无法使用外部模块绑定这个功能**，只能用这 6 个魔法名字之一。

### 8.8 工程与验证缺口

**Sanitizer 构建配置齐全但 CI 从不运行**。`CMakePresets.json` 里定义了 `asan-llvm22` 和 `ubsan-llvm22` 两个 preset 及对应的 build preset，但 `.github/workflows/ci.yml` 只跑 `dev-llvm22` 和 `release-llvm22`。路线图 Phase 2 和 Phase 6 都明确要求 sanitizer 构建。

这个缺口不是理论上的。我在这次分析过程中，通过手动构建 ASAN 版本发现了一个真实的内存 bug：`ACIRToACSimPass::expandModule` 里 `llvm::StringRef pathPrefix` 指向 `constructionOrder` 这个 vector 的元素，而递归展开会往同一个 vector 里 `push_back`，触发重分配后引用就悬空了。表现是构造顺序里出现乱码：

```
construction_order = ["Top.mid", "Top.mid.left", "\00op.mid.right", ...]
                                                  ^^^^^^ 本该是 "Top.mid.right"
```

CI 的常规构建抓不到这个（小字符串的 SSO 让它有时候恰好不崩），跑了 ASAN 就立刻暴露。**如果 CI 跑了 sanitizer preset，这个 bug 早就该被发现。**

这个 bug 已经修复——在 `expandModule` 开头把 `pathPrefix` 拷贝成局部 `std::string` 再递归（`lib/Conversion/ACIRToACSim/ACIRToACSim.cpp:1034`）。修复后 67 个 lit 测试和全部单元测试通过。**注意这个修复目前还在工作区，没有提交。** 另外，"引用 vector 元素的同时又往这个 vector 里 push"这个模式值得在 `lib/Analysis/` 等处再扫一遍。

**没有 benchmark、性能测试、端到端示例**。仓库里没有 `examples/`、没有 `benchmark/`。路线图 Phase 5 要求的小型 golden 示例（producer/queue/consumer、背压流水线、request/response 内存路径、嵌套数组、多时域桥、挂起进程）和 NPU showcase，**一个都没有**。

**接口演进一致性检查只实现了最外层**。`docs/specs/interface-evolution-v0.1.md` 要求全工具链单一纪元锁步、每个持久化产物暴露纪元信封、精确相等比较。目前只实现了：`scripts/check-contracts.py` 校验各 schema 的 `contract_epoch` 常量和 `pyproject.toml` 的 `contract-epoch` 都等于 `"0.1"`，以及降级 pass 里对 `ac.contract_epoch` 的运行时检查。跨产物的纪元信封统一校验、build/run/replay manifest 的一致性核对、provider 指纹的锁步一致性检查都还没有——不过这些依赖的是尚不存在的 Phase 3/4 产物。

### 8.9 缺失清单速览

| 缺失项 | 阶段 | 严重度 | 备注 |
|---|---|---|---|
| Python 前端 / ACPy | 4 | 🔴 阻断 | 零产品代码 |
| CLI（10 个命令） | 4 | 🔴 阻断 | 零实现，无 entry point |
| ACSim → C++ 生成桥 | 3 | 🔴 阻断 | 发射器存在但不消费 IR |
| PTO trace 读取 | 2/5 | 🔴 阻断 | 无 JSON 解析器 |
| 静态派发表（运行时） | 2 | 🟠 高 | 当前是虚函数动态派发 |
| 激活邻接图剪枝 | 2 | 🟠 高 | 当前全量扫描 |
| 队列/资源等构造的降级 | 1 | 🟠 高 | 6 类构造被拒绝 |
| 非 yield-only 进程降级 | 1 | 🟠 高 | 有行为的进程都不行 |
| ACIR 标准库 `ac.std` | 2 | 🟠 高 | 只有规范 |
| build manifest 符合 schema | 3 | 🟡 中 | 5/19 字段 |
| Scheduler 组件 | 2 | 🟡 中 | 7 个组件里唯一缺的 |
| Packet 序列化 | 2 | 🟡 中 | 只有 concept 骨架 |
| 统计汇总与结果导出 | 2 | 🟡 中 | 有计数器无汇总 |
| provider 注册的公开入口 | 3 | 🟡 中 | extern 模块实际不可用 |
| CI 跑 sanitizer | 0/6 | 🟡 中 | preset 有，CI 没跑 |
| 端到端示例 / benchmark | 5 | 🟡 中 | 完全没有 |
| 无进展诊断填充 | 2 | 🟢 低 | 结构体定义了没用 |

---

## 9. 上手实操

### 9.1 环境搭建

系统自带的 GCC 如果低于 10，不支持 C++20，需要自建环境。用 micromamba 可以不要 sudo 权限：

```bash
# 安装 micromamba（如果还没有）
curl -Ls https://micro.mamba.pm/api/micromamba/linux-64/latest | tar -xvj bin/micromamba

# 创建环境：MLIR 22.1.8 + GCC 16.1 + 构建工具
micromamba create -y -n acir -c conda-forge \
  mlir=22.1.8 llvmdev=22.1.8 gxx_linux-64 \
  cmake ninja python=3.11 lit zlib libxml2 gtest gmock
```

几个容易踩的坑：

1. **缺 zlib / libxml2** 会导致 CMake 报 `LLVMSupport` 的链接接口找不到 `ZLIB::ZLIB`
2. **`FileCheck`、`not`、`split-file`、`count`** 这些 lit 需要的工具在 conda 环境里被放在 `libexec/llvm/` 下，不在 `bin/`，需要建软链接：

```bash
ENV=$HOME/micromamba/envs/acir
for t in FileCheck not split-file count; do
  ln -sf $ENV/libexec/llvm/$t $ENV/bin/$t
done
```

3. **WSL 环境下 CMake 可能误抓到 Windows 侧的 gtest**（`/mnt/d/Anaconda/...` 里的 `.lib` 文件），需要显式排除

### 9.2 构建

```bash
ENV=$HOME/micromamba/envs/acir
cmake -S . -B build/local -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DMLIR_DIR=$ENV/lib/cmake/mlir \
  -DLLVM_DIR=$ENV/lib/cmake/llvm \
  -DGTest_DIR=$ENV/lib/cmake/GTest \
  -DCMAKE_IGNORE_PREFIX_PATH="/mnt/d/Anaconda;/mnt/d/Anaconda/Library"

cmake --build build/local
```

产出两个驱动：

- `build/local/bin/acir-opt` —— 公共驱动
- `build/local/bin/acir-opt-internal` —— 内部测试驱动，预注册了 6 个 structural provider（见 8.7）

### 9.3 跑测试

```bash
# lit 测试（67 个）
cmake --build build/local --target check-acir

# 单元测试（10 个可执行文件，约 267 个 gtest 用例）
ctest --test-dir build/local --output-on-failure
```

### 9.4 常用命令模板

**只做语义校验**：

```bash
acir-opt model.mlir --ac-verify-model -o /dev/null
```

**看规范化之后长什么样**：

```bash
acir-opt model.mlir --ac-canonicalize-model
```

**冻结拓扑**（这一步会打印出完整的对象清单和摘要）：

```bash
acir-opt model.mlir \
  --ac-verify-model --ac-canonicalize-model --ac-freeze-topology \
  -o model.frozen.mlir
```

**降级到 ACSim**（模型里没有 `ac.module.extern`，用公共驱动即可）：

```bash
acir-opt model.frozen.mlir --ac-lower-to-acsim \
  --ac-binding-profile=fast \
  --ac-binding-target=x86_64-linux-gnu
```

**降级到 ACSim**（模型里有外部模块，需要注册表 + 内部驱动，原因见 8.7）：

```bash
acir-opt-internal model.frozen.mlir --ac-lower-to-acsim \
  --ac-binding-registry=path/to/registry.json \
  --ac-binding-profile=fast \
  --ac-binding-target=arm64-apple-darwin \
  --ac-binding-lock-output=model.lock.json
```

注意 `--ac-binding-target` 必须和注册表里 candidate 条目的 `target` 字段精确匹配，否则查不到绑定。

**验证确定性**（连跑两次比对）：

```bash
OPTS="--ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu"
acir-opt model.frozen.mlir --ac-lower-to-acsim $OPTS -o out1.mlir
acir-opt model.frozen.mlir --ac-lower-to-acsim $OPTS -o out2.mlir
diff out1.mlir out2.mlir && echo "确定性 OK"
```

### 9.5 acir-opt 的专属选项

```
--ac-verify-model            全模型语义校验
--ac-canonicalize-model      规范化拓扑
--ac-freeze-topology         冻结拓扑，计算摘要
--ac-lower-process-state     规划进程状态机
--ac-resolve-gfsim-bindings  绑定解析
--ac-lower-to-acsim          降级到 ACSim（原子操作）

--ac-binding-registry=<file>     绑定候选/请求注册表 JSON（可重复）
--ac-binding-profile=<profile>   构建 profile 标识（如 fast / validated）
--ac-binding-target=<target>     工具链目标标识（如 x86_64-linux-gnu）
--ac-binding-lock-output=<file>  绑定锁文件的输出路径
```

注意 `--ac-lower-to-acsim` 是**原子的整模型降级**，它自己会跑绑定解析，不需要单独加 `--ac-resolve-gfsim-bindings`。

### 9.6 写第一个模型的最小骨架

能通过校验、也能降级到 ACSim 的最小模型：

```mlir
builtin.module attributes {ac.contract_epoch = "0.1"} {
  ac.system @soc root @Top as "root" tick 0 "cycle"
      workload @Top::@tick seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true

  ac.module @Top() parameters {} graph {
    ac.process @tick kind "workload" { ac.yield_sim }
    ac.return
  }
}
```

四个必须有的东西：

1. 模块属性 `ac.contract_epoch = "0.1"`
2. 恰好一个 `selected true` 的 `ac.system`
3. 根模块必须是具体的 `ac.module`（不能是 extern）
4. `workload` 必须指向一个 `kind = "workload"` 的进程

因为没有外部模块，这个骨架用**公共驱动**就能完整跑通全流程：

```bash
acir-opt minimal.mlir --ac-verify-model --ac-canonicalize-model \
  --ac-freeze-topology --ac-lower-process-state -o minimal.frozen.mlir

acir-opt minimal.frozen.mlir --ac-lower-to-acsim \
  --ac-binding-profile=fast --ac-binding-target=x86_64-linux-gnu
```

产出的 ACSim 是这样（省略了指纹的中间部分）：

```mlir
module attributes {ac.contract_epoch = "0.1"} {
  acsim.model @soc epoch "0.1" root @Top
      construction ["Top.tick"] destruction ["Top.tick"]
      fingerprints {binding_lock = "sha256:4f53cda1...", frozen_acir = "sha256:53292e0d...",
                    profile = "sha256:079c9d12...", provider = "sha256:4f53cda1...",
                    schema_set = "sha256:4f53cda1...", toolchain = "sha256:bd7deae3..."} {
    acsim.type @acir_impl_wake_next_delta_63cacba5... kind "implementation" ...
    acsim.type @acir_wake_next_delta cpp "acir::generated::wake_next_delta" kind "wake" ...
    acsim.module @Top interface {ports = [], resources = [], results = []} static []
        specialization "sha256:a6560ab4..." exports [] {
      acsim.process @tick captures() names [] entry @entry pcs [@entry] live []
          fairness 2 specialization "sha256:e2819e96..." {
      state @entry {
        %0 = acsim.invoke @acir_impl_wake_next_delta_63cacba5...() : () -> !acsim.wake<@acir_wake_next_delta>
        acsim.suspend @entry on %0 : !acsim.wake<@acir_wake_next_delta>
      }
      }
      acsim.return
    }
    %object, %activation = acsim.dispatch @Top::@tick path "Top.tick" indices []
        object 0 activation 0 work "acsim_generated::Top::sa6560ab4...::tick::pe2819e96...::work" ...
    acsim.activate %activation to %object : !acsim.activation_id to !acsim.object_id
  }
}
```

对比一下 [6.3](#63-阶段三降级到-acsim) 的 cluster 例子会发现：这里只有 2 个 `acsim.type`（都是生成的 wake 类型），没有 `schema` / `provider` / `value` 类型——因为没有任何外部绑定。`binding_lock`、`provider`、`schema_set` 三个指纹都是同一个值 `sha256:4f53cda1...`，那是**空集合的 SHA-256**。

从这个骨架出发，可以往里加实例、数组、队列、协议——加到什么程度取决于你要走到哪一步：**只要过校验**，可以用全部 ACIR 特性；**要降级到 ACSim**，就得守着 8.5 列的那些限制。

---

## 10. 附录：属性合法取值速查表

以下取值全部来自验证器源码的硬编码枚举，并且经过实际测试确认。

### 协议相关

| 算子.属性 | 合法值 |
|---|---|
| `ac.role.cardinality` | `exclusive` / `shared`（对偶双方必须一致） |
| `ac.event.action` | `offer` / `accept` / `cancel` / `reject` / `retry` / `response` / `notify` |
| `ac.guarantee` kind = `ordering` | `fifo` / `per_key` / `unordered` |
| `ac.guarantee` kind = `backpressure` | `none` / `accept` / `credit` / `capacity` / `custom` |
| `ac.guarantee` kind = `delivery` | `exactly_once` / `at_most_once` / `best_effort` |
| `ac.guarantee` kind = `completion` | `on_accept` / `on_response` / `on_terminal_phase` |
| `ac.guarantee` kind = `stable_pending` | 布尔值 |
| `ac.guarantee` kind = `max_inflight` | 不校验 |
| `ac.guarantee` kind = `correlation` | 非空字符串 |
| `ac.guarantee` kind = `custom_backpressure` | 非空字符串 |

载体动作（能携带 payload）：`offer`、`response`、`notify`。

### 状态与资源

| 算子.属性 | 合法值 / 约束 |
|---|---|
| `ac.queue.ordering` | `fifo` / `per_key`（**不能是 `unordered`**） |
| `ac.queue.ownership` | 只能是 `exclusive` |
| `ac.queue.entries` | 正整数 |
| `ac.queue.bytes` | 正整数（可选字段） |
| `ac.queue.watermarks` | 精确 `{low, high}`，`0 <= low < high <= entries` |
| `ac.queue.delay_ticks` | 必须恰好为 1（默认值） |
| `ac.resource.capacity` | 正整数 |
| `ac.resource.issue_width` | `[1, capacity]` |
| `ac.resource.ii` | `>= 1` |
| `ac.resource.latency` kind | `fixed`（键集 `{kind, ticks}`，ticks 为正）/ `symbol`（键集 `{kind, ref}`，ref 指向模块） |
| `ac.resource.lifecycle` | 必须精确是 `{reservation="propose_commit", release="balanced", cancellation="explicit"}` |
| `ac.resource.ownership` | `exclusive`（不能有 arbiter）/ `shared` / `contested`（必须有 arbiter） |
| `ac.time_domain.period` | 正整数（全局 tick） |
| `ac.time_domain.phase` | 非负整数（全局 tick） |
| `ac.time_domain.scale` | 正整数，有实现上限 |

### 行为与观测

| 算子.属性 | 合法值 |
|---|---|
| `ac.process.kind` | `control` / `workload` / `monitor`（monitor 不能用 try_send/try_recv/schedule/wait_for） |
| `ac.stat.kind` | `counter` / `gauge` / `histogram` / `event_log` |
| `ac.probe.kind` | `queue` / `resource` / `module` / `storage` / `protocol` / `trace` / `event_queue` / `external_io` / `statistics` |

探针 kind 到目标算子的映射：`queue`→`ac.queue`，`resource`→`ac.resource`，`module`/`trace`/`external_io`→`ac.process`，`storage`→`ac.address_space`，`protocol`→`ac.queue`，`event_queue`→`ac.event_queue`，`statistics`→`ac.stat`。

### 系统

| 算子.属性 | 合法值 |
|---|---|
| `ac.system.tick_epoch` | 必须恰好是 0 |
| `ac.system` tick 单位 | `cycle` / `ps` / `ns` / `us` / `ms` / `s` |
| `ac.system.seed_policy` | 精确 `{kind = "fixed", value = <非负 i64>}` |
| `ac.system.result_schema` | 精确 `{id = <非空字符串>, format = "json"}` |
| `ac.system.primary_workload` | 必须引用 `kind = "workload"` 的进程 |

### ACSim

| 算子.属性 | 说明 |
|---|---|
| `acsim.type.kind` | `schema` / `implementation` / `provider` / `value` / `wake` |
| `acsim.model.fingerprints` | 六项：`frozen_acir` / `binding_lock` / `provider` / `schema_set` / `profile` / `toolchain` |
| `acsim.process.fairness` | `max(计划工作步数, 2)`，且必须 ≥ 最长执行路径 |
| `acsim.binding.effect` | `stateful` / `pure` |
| `acsim.dispatch` thunk | 四个：`work` / `xfer` / `reset` / `validate` |

---

## 参考

规范文档（`docs/specs/`）：

| 文件 | 内容 | 实现状态 |
|---|---|---|
| `acir-core-v0.1.md` | ACIR 核心语义 | ✅ 已实现 |
| `acir-process-state-plan-v0.1.md` | 进程状态规划 | ✅ 已实现 |
| `acsim-gfsim-lowering-v0.1.md` | ACSim 降级 | ⚠️ 部分实现 |
| `gfsim-runtime-abi-v0.1.md` | 运行时 ABI | ⚠️ 骨架 |
| `acir-stdlib-v0.1.md` | 标准库 | ❌ 未实现 |
| `python-to-acir-lowering-v0.1.md` | Python 降级 | ❌ 未实现 |
| `agentic-python-cli-v0.1.md` | Python API 与 CLI | ❌ 未实现 |
| `pto-trace-schema-v0.1.md` | PTO trace 格式 | ❌ 未实现 |
| `interface-evolution-v0.1.md` | 接口演进 | ⚠️ 仅纪元校验 |

其他：

- 路线图：`docs/superpowers/plans/2026-08-04-agentic-circuit-roadmap.md`
- 规范覆盖审计：`docs/implementation/spec-coverage.md`
- Phase 1 审计：`docs/implementation/phase-1-audit.md`
- 算子定义：`include/acir/Dialect/ACIR/ACIROps.td`、`include/acir/Dialect/ACSim/ACSimOps.td`
- 验证器实现：`lib/Dialect/ACIR/ACIROps.cpp`、`lib/Dialect/ACSim/ACSimOps.cpp`
- 降级实现：`lib/Conversion/ACIRToACSim/ACIRToACSim.cpp`
