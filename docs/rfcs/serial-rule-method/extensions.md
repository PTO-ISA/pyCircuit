# 资源合同与后续扩展边界

本文件记录[基础 Method 规格](source-semantics.md)之外的扩展责任。
内部 IR 语义、Python 源接口和后端执行是不同证据层。本文的 Queue 算法约束可以用于
内部分析与测试；未开放的源拼写不能因此被宣称已经确定或实现。

## Queue 请求与获准 {#queue-request-admission}

**QUEUE-REQUEST-ADMISSION：**继续复用现有完整 token `ac.queue`、容量、latency、
reset、四态和 no-empty-bypass 合同。普通公开调用
`ac.queue[T](valid, data, take, depth=N, ready_policy=...)` 保留含义及默认策略。
新增事务能力不得把所有 Queue 默认改成允许 downstream pop 的策略。

对单个 Queue，定义：旧 occupancy 为 `n`，容量为 `d`，旧可用头部为 `v`。
在事务 R 的真实路径上，`p_R` 和 `s_R` 分别表示请求一次 pop 和一次 push。
它们是请求，不是已经提交的传输。首批每个 Queue 每个事务最多一项逻辑 pop 和一项
逻辑 push；多次观察同一 payload 是否合并消费必须由未来源访问合同明确。
不能因 Python 对象名相同就猜测多次 pop 是观察还是消费。

在 `local_occupancy` 策略下，push 需要 `n < d`；不抵扣同拍 pop。
在 `downstream_pop` 策略下，容量可使用合法同拍 pop 的释放量。对同一事务自己的
pop，先证明其请求路径和旧头部可用，再将这一条件性释放用于该事务的容量判断。
不能让自己的容量判断依赖自己已经获准。

单事务、已知控制的语义示意：

```text
pop_ok     = (not p_R) or v
own_credit = p_R and v                 # 仅 downstream_pop 可用于容量
push_ok    = (not s_R) or (n < d) or own_credit
fire_R     = selected_R and other_required_R and pop_ok and push_ok
actual_pop = fire_R and p_R
actual_push = fire_R and s_R
```

此公式不是四态 Boolean 化简许可。有效未知控制仍服从 managed failure 合同。
请求路径不得依赖本事务的新 push 所提供的 payload；新 push 只在其声明的后续采样边界
可用。满队列同事务 pop+push 消费旧头部并保留 occupancy；空队列不旁路。

多个事务访问同一 Queue 时，必须先证明其接受组合符合一个 Queue 的总端口能力。
跨事务容量抵扣只能使用在无环因果关系中已确定获准的 pop，不能抵扣仅仅请求的 pop。
同一释放量不能被多个并发 push 重复使用。竞争需显式协议/grant 或互斥证明，不能自动
选择 winner。整个请求/有效性/容量图闭环时拒绝，不运行动态固定点算法。

Queue 的初始化内容、`size/full` 观察、混合资源返回值及资源数组都不是当前三个
ready/valid/data 输出自动具备的能力。扩展这些能力必须修改原有 Queue owner 的共同
合同、验证和双后端；不得旁边放一份可能漂移的 shadow occupancy 或另一个 FIFO 实现。

## 同拍资源与物理端口

**RESOURCE-PORTS：**每个 primitive 的已声明合同决定允许的读写组合。
同一事务连续修改同一个普通寄存器候选可以合并为最终 D/E；两次不同地址的单端口 RAM
写入不能仅因语法串行就都在同拍完成。普通 Table 的实现能力也不能直接套到 memory。

必须区分效果冲突与硬件端口限制：两次写不同地址可能语义不冲突，但仍超过端口数。
只有已证明等价且满足资源合同的写合并可以减少实际访问。增加端口、banking、复制 memory
或多周期调度均不由本扩展默认授权。

## Signal 与 EHR 的后续语义义务

基础 Method 不提供跨 Rule 前递。后续扩展若开放 Signal/EHR，必须保留下表的区别：

| 读取场景 | 可见值义务 |
| --- | --- |
| 普通变量跨 Rule | 旧 Q，不因注册顺序前递 |
| 同事务有序候选操作 | 按本资源合同读取合法的此前候选；最终仍整体提交/取消 |
| 跨事务 Signal 后序读取 | 只有本拍此前已获准 set 才使 payload 有效；无有效写入时 invalid，依赖 get 的事务不能 fire |
| EHR 阶段读取 | 仅观察更低端口的获准写入，取最近的有效端口；同端口不增加写后读可见性 |
| EHR 无合法前序写入 | 读取旧 Q；不得读取未获准临时候选 |

Signal 对应 issue #307 的 Wire：每拍重新 invalid，没有可回退的持久 payload Q，
invalid 不作为零或 false 进入分支。set→get 是跨事务严格顺序，同一外层事务（包含
嵌套 Method）同时 set/get 必须拒绝。一个 Method 返回普通值不因此变成 Signal。
EHR 保存跨拍状态；同一外层事务跨该 EHR 不同端口或重复写同端口均拒绝。
更高端口的获准写决定最终保存值，无获准写时保持旧 Q。上述语义不开放源拼写。

前递建立的是同拍因果边，不是新的存储边界。必须联合检查数据、guard、容量与顺序。
例如 A 写 Signal 并向满 Queue push，B 读该 Signal 后决定 pop，可能同时要求 A→B
和 B→A；存在底层寄存器不使这个同拍环合法。

后续合同必须明确：多 writer 是互斥、显式优先级还是合法分阶段；同阶段冲突；多个
Method 的阶段身份与调用约束传播；未执行路径；reset、
四态、时钟域及失败回滚。不得由 C++ 函数调用顺序填补这些决定。

实现上优先复用候选 SSA、select、现有存储与共同依赖分析。是否需要新公共 IR 操作由
这些可验证语义决定，不预设 Signal/EHR 必须各自增加一个 Runtime 类。

## 尚未开放的源接口 {#deferred-source-interfaces}

以下是明确的设计检查点。后续 agent 可以在相应设计任务中形成具体补充规格；没有该
补充规格时不得自行启用，也不得把拒绝这些形式报告为基础 Method 验收失败。

| 接口 | 需要先固定的决定 | 必需验收 |
| --- | --- | --- |
| Queue 句柄、资源参数和隐式消费 | 构造/引用类型、观察与消费、别名重复访问、固定输出绑定 | 未执行分支不消费、消费合并、完整 token 守恒、容量/周期、跨源引用身份 |
| Signal/EHR 与 `After` | 声明/访问拼写、排序范围、阶段、冲突与因果规则 | Signal 无写 invalid、EHR 旧 Q 回退、同事务禁用组合、阶段可见性、环拒绝、两后端等价 |
| guarded Rule 的显式有效性结果 | payload 无效时的类型/使用规则、输出边界、无隐式持久状态 | guard=false 不泄漏 payload，不能用错误 else 或旧值掩盖 |
| module Work 调用只读 Method | 值有效性如何穿过普通组合、控制依赖和输出 | 不相关 Rule 不继承条件，相关分支都继承，跨源摘要可验证 |
| 混合普通端口与 Method 的模块 | 句柄/结果访问、绑定、公开签名和实例身份 | 既有 typed-result 语义不变，独立构造与 fanout 正确 |
| 一般静态参数与资源数组 | 允许的 elaboration、形状、共享/独立 owner、动态索引 | 多组尺寸、别名/范围/端口证明、独立 unit 与预算 |
| 跨域事务、自动 pipeline | 时钟关系、采样、CDC 或明确新增 latency 的用户合同 | 域间原子性、reset、失败与周期；不能用单域例子代替 |

## 真实协议状态不能被语法糖删除

SSM 的 reserve、consume、cancel 和 release 可能分属不同采样周期。lease、generation、
epoch、恢复身份和保留容量是硬件协议状态，不能因为 Method 原子调用就删除。
同拍准入只决定该拍的效果；不建立跨多个周期的隐式软件事务。

背压、合法 retry、过期响应和非法输入也必须区别处理。不能把所有拒绝条件都变成
required guard，否则一个本应丢弃的 token 或需要推进的探测可能永久阻塞。
组件可以通过普通返回值、明确丢弃操作或既有检查表达这些结果，框架不推测业务策略。

## 扩展检查清单

- [ ] 新源拼写及其静态/运行时边界已经写成补充合同。
- [ ] owner、别名、调用次数、端口和周期含义明确。
- [ ] guard、不发射、普通失败结果、硬错误和 reset 分别定义。
- [ ] 与既有 Queue/存储策略的差异逐项列出，没有默认策略漂移。
- [ ] 所有数据、容量、前递、guard 和顺序边统一检查，没有隐藏固定点。
- [ ] source-unit 发布/链接验证扩展完整，不需要 provider Python。
- [ ] 独立正反例及 managed C++/RTL 执行覆盖新增语义。
- [ ] 基础范围通过不被写成该扩展通过；内部 IR 测试不被写成源 API 通过。
