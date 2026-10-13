# ACIR 基础设计

本文记录本轮确认的 IR 设计；已实现的原生操作、文本语法和分析 lowering 见 [ACIR v7](../pycircuit/acir.md)。

ACIR 基于 MLIR，直接表达静态结构、计算、控制流、组件调用和事务语义。编译器从这些信息推导 RuleGraph，不在高层 IR 中重复保存完整分析图。

## 1. Module、Rule 与 Method

| Primitive | 组成 | 语义 |
| --- | --- | --- |
| `acir.module` | 实例身份、端口、声明区域、Work 区域 | 保存静态结构及运行时组合逻辑，不建立事务 |
| `acir.rule` | 参数、返回类型、guard、body | 建立独立原子事务 |
| `acir.method` | 参数、返回类型、guard、body | 可组合调用，效果归入调用者的外层 Rule 事务 |
| `acir.rule.call` | 目标 Rule、实参、输出资源绑定 | 选择调用 Rule，不表示效果已经提交 |
| `acir.method.call` | 目标 Method、实参、SSA 返回值 | 在调用者的计算中调用 Method |

### Module

- 每个静态展开的实例对应一个 `acir.module`，首版不额外建立模板与实例化体系。
- 声明区域保存资源、子实例、固定连接、Rule／Method 定义及输出端口绑定。
- Work 区域保存组合计算、状态观察、只读 Method 调用、控制流和 Rule 选择，不提交状态。
- 资源和实例声明只依赖构造参数。构造期分支静态展开，运行时分支保留在 Work 中。
- Module 建立局部符号作用域；跨 Module 访问明确引用目标实例的资源或 Method。
- Work 产生的普通值通过实参传给 Rule。前端闭包对这些值的引用转换为显式参数。

### Guard 与 body

Rule 和 Method 各自具有 guard 区域与 body 区域。两个区域分别绑定同一组调用实参；guard 只观察状态并产生布尔值，body 保存实际计算和效果。

没有显式 guard 时默认为 `true`，文本可省略 guard 区域。定义中的 guard 保存自身条件，不提前复制所有被调用 Method 的 guard。

用户声明的 Method 先后关系保存在组件契约中，关联对应的 Method 符号。具体 Rule 顺序由实际调用与契约共同推导。

### 返回值

- Rule 保留返回类型和 body 中的返回值。Queue／Cell／Signal 输出 payload 由调用位置绑定到固定资源，后续 lowering 分别展开为 push／write／set，效果属于该 Rule 事务。
- Rule 返回值不建立向另一个 Rule 直接传递临时普通值的通路。
- Method 可以返回普通 SSA 值，供调用者继续计算；其效果与返回值的使用遵循外层事务及 guard 条件。
- Rule 的消费性 Queue 输入在实际读取 payload 的路径上对应 pop；普通观察和 guard 读取不消费。

## 2. 基础组件

基础组件具有自己的声明、引用类型和 Method 操作，避免由通用 read／write／query 再解释组件语义。

| 声明 | Method 操作 | 核心语义 |
| --- | --- | --- |
| `acir.queue` | `acir.queue.peek/pop/push/empty/full/size` | FIFO，读取旧数据；成功 pop 可为同拍 push 腾出空间 |
| `acir.cell` | `acir.cell.read/write` | 读取周期开始的旧值，获准写入下拍生效 |
| `acir.signal` | `acir.signal.get/set` | 本拍写入后有效，get 以 valid 为隐式 guard，set 排在 get 前 |
| `acir.ehr` | `acir.ehr.read/write` | 根据 Method 阶段观察前序有效写入 |

普通 payload SSA 值与组件引用分开。所有状态效果属于外层 Rule，并随整个事务提交或取消。Module Work 只能观察状态。

### 组件契约

基础组件定义由编译器集中维护，提供 Method 的默认 guard、状态效果、顺序及可见性。分析器通过统一接口查询契约，不将组件规则散落为特殊判断。

用户组件的契约显式保存在 MLIR 中，与基础组件使用同一套契约接口。

关系含义对齐 BSV：CF 表示无冲突；C 表示不能同时使用；SB 表示普通先后，允许同事务组合；SBR 表示严格先后，禁止在同一外层事务的实际路径上组合，包含嵌套调用。SB／SBR 推导顺序边，C 推导冲突约束；顺序不规定仲裁优先级。关系声明的前端写法与冲突选择策略尚未确定。

### Queue

- 不保留 `revise`。
- `peek` 和 `pop` 要求非空，顺序为 peek → pop → push。
- push 要求有空间，可观察本拍成功 pop 释放的容量；新 push 数据下拍可读。
- 内部 full 使用 EHR 容量视图，其阶段关系和 push guard 属于 Queue 自身契约。
- 高层 ACIR 不显式声明 Queue 内部 full 状态、EHR 端口或旁路记录，后续 lowering 按需展开。

### Signal

- Signal 采用 BSV Wire 语义，是具有固定身份的组件，不是临时 SSA 值；不声明初值，也不保存跨周期值。
- 每拍开始为 invalid，本拍有效 set 后才可 get。get 的隐式 guard 为 valid，不消费，允许多个读取者。
- set → get；跨 Rule 只读取前序获准写入。没有有效写入时 get 失败，不回退到旧值。
- set → get 是严格同拍先后关系，同一原子事务不能对同一 Signal 同时 set 和 get，包含嵌套调用。set 与 set 冲突，get 与 get 无冲突；不提供同事务临时写入旁路。
- Signal 不使用 Queue 的消费语义，也不要求高层显式 EHR 端口。
- RTL 中 data 和 valid 对应组合信号；仿真对象保存本拍求值结果，不代表跨周期寄存器。

### 用户 EHR

- 用户显式声明 EHR 及其初始值，读写操作不手填端口号。
- 端口契约对齐 BSV CReg。编译器根据访问它的 Method 顺序分配阶段与端口；后阶段只观察前阶段获准写入，多个前序写入时选择端口编号最大的写入。
- 没有顺序关系时不增加可见性端口；同端口写入不可见。没有前序有效写入时读取旧值。
- 同端口 read／read 为 CF，read → write 为 SB，write → write 为 SBR；低端口所有操作到高端口所有操作为 SBR。
- 同一外层事务不能访问同一 EHR 的不同端口，包含嵌套调用；不提供同事务跨端口临时写入旁路。Queue 内部 EHR 的 pop／push 阶段也遵循该限制。
- 获准写入中端口编号最大的写入决定下拍保存值；没有获准写入时保持旧值。

## 3. Method 返回值的条件传播

Method 返回普通 SSA 值，但带 guard 的 Method 只有在条件满足时才能求得返回值。

在 Rule 内调用时，实际调用的 Method guard 归入外层事务的执行条件。在 Module 内调用时，guard 沿返回值的数据和控制依赖传播到相关 Rule，不影响无关 Rule。

例如：

```python
value = component.read()  # Method guard 为 ready。
send(value + 1)           # send 要求 ready。
independent()            # 不受 ready 影响。
```

如果 `value` 决定分支，则两条分支都以 ready 成立为前提。调用失败不能当成条件为假而进入 else。编译器在 lowering 时将受保护计算落实为相应的执行路径，不给普通 SSA 值增加持久 valid 状态，也不默认使用旧值。

Rule 间通过 Signal 传递值时，消费者通过 get 的隐式 guard 依赖本拍有效写入；生产者失败，其 set 无效。set → get 顺序要求先确定前序生产者的事务结果。Module 中 get 的有效性条件沿返回值的数据与控制依赖传播，不将 invalid 当作 False。

Rule 的完整执行条件由 Module 选择路径、Rule 自身 guard 和实际调用的 Method guard 组成。分支中的要求只约束对应路径。

## 4. 计算、控制流与分析结果

- 普通算术复用 `arith`，普通 helper 使用 `func`。
- 运行时结构化分支和循环使用 `scf`，后续可以 lower 为 `cf`。
- 保留实际 SSA 依赖和控制流，多个 Rule 使用同一组合计算时保留共享定义。

| 高层 MLIR 直接保存 | 编译器推导 |
| --- | --- |
| 实例、端口、资源、类型、初始化和固定连接 | Rule 节点及具体资源绑定摘要 |
| Rule／Method 身份、事务边界、实际调用和返回 | 调用链与效果的事务归属 |
| 实际计算、控制流、显式 guard、组件契约 | 选择条件、完整执行条件、条件化状态访问 |
| SSA 参数及计算来源 | 准备计算、共享关系与 Rule 使用绑定 |
| Method 顺序和 EHR 声明 | Rule 顺序边、EHR 阶段与端口 |

RuleGraph 是编译器内存中的派生分析结果，可导出 JSON 与可视化。相关 IR 改变后需更新或重建分析。后续 lowering 消费图中的约束，将执行顺序、受保护计算和事务效果落实到生成代码中。

高层 MLIR 不重复嵌入完整 RuleGraph。保存静态共享关系不等于已经实现运行时共享求值或跨 tick 缓存。

## 5. 尚未收敛的表达

- 聚合数据、数组及其他辅助计算操作的后续扩展。
- EHR 作为 Rule 输出目标，以及共享值和辅助表达的后续扩展。

原生操作的 assembly format、终结操作、输出绑定及现有 export／external 形式见 [ACIR v7](../pycircuit/acir.md)。未决扩展不在本文补充。
