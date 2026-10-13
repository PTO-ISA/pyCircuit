# ACIR MLIR v7

本文记录当前 v7 实现。Signal 的目标设计已修订为 BSV Wire：本拍有效写入后可读，get 以 valid 为隐式 guard，不保存跨周期值；set → get 为严格先后，同一事务不可对同一 Signal 同时读写。下文 Signal 初始化、旧值回退及读取契约属于尚未迁移的旧实现，不是新的设计规范。

关系表的目标语义已对齐 CF／C／SB／SBR，EHR 端口契约已对齐 BSV CReg，不允许同一外层事务跨端口访问。当前 v7 实现尚未迁移这些关系与限制；下文记录现有表示。

保存的主 MLIR 直接表达静态结构、组件操作、guard、计算和返回值。它是独立重载的程序表示，不包含 Python AST 或完整 RuleGraph。v6 及更早产物需要从源码重新编译。

```text
ACPy 静态展开 → 原生 ACIR → C++ 分析 lowering（副本）
                                  → 实际 SSA/CFG → Python RuleGraph
```

## 原生结构与符号

外层 builtin `module` 的 `acir.model` 只保存版本、top、构造配置、payload 类型和导出连接。每个静态实例对应一个 `acir.module`，建立一个符号作用域。实例的构造来源、父实例、公开成员、输入和返回连接保存在 `acir.instance` 属性；子实例句柄用 `acir.instance` 操作引用已展开的 Module，引用共享实例不复制资源。

| 操作 | 内容 |
| --- | --- |
| `acir.module` | 单个声明区域，保存资源、实例句柄、接口、定义和 Work |
| `acir.instance` | 符号化的子实例／共享实例连接 |
| `acir.interface` | 公开接口、具体 Method 定义和 `orders` 中的符号化 After 关系 |
| `acir.rule` | 函数类型、输出槽位引用类型、独立 guard 和 body |
| `acir.method` | 函数类型、接口身份、独立 guard 和 body |
| `acir.work` | Module 组合计算和 Rule 选择；不建立事务 |
| `acir.rule.call` | 目标符号、实参和 `output_bindings`；没有 SSA 返回值 |
| `acir.method.call` | 目标符号、实参和普通 SSA 返回值 |

跨实例引用写为 `@instance::@symbol`。实例路径中的点属于符号名；需要时使用 MLIR 的引号符号语法。普通 helper 和初始化函数使用 `func.func`；各 Module 中的 private 声明导入共享 helper，分析 lowering 将定义保留一次。按调用参数和捕获环境静态展开的 Method 定义通过 `interface_ref` 关联同一个公开接口。已注解的未使用接口也保留定义；只有普通参数、静态捕获的接口参与无条件摘要。

Rule／Method 的两个区域具有相同的入口参数类型，验证器检查它们与函数签名一致。省略 guard 表示 true；显式 guard 以 `acir.guard.yield` 产生 bool。body 以 `acir.return` 结束，Work 以 `acir.work.end` 结束。

Rule 的 `output_types` 保存 Queue／Cell／Signal 槽位类型；`acir.return` 的 `slots` 将实际 payload 对应到槽位。None 对应省略该槽位，不产生无效 payload，也不取消其它效果。调用的输出绑定必须引用匹配的固定资源；同一 Rule 的多个互斥选择位置必须使用相同绑定。Method 的 return 则与普通 SSA 返回签名一致。

```mlir
acir.rule @top.exchange : (!acir.queue<i32>) -> ()
    {output_types = [!acir.queue<i32>]} body {
^entry(%input: !acir.queue<i32>):
  %value = acir.queue.peek %input {role = "consume"} : !acir.queue<i32> -> i32
  acir.return %value : i32 {slots = [0 : i64]}
}
```

实际产物另有 `acir.function` 来源属性，保留参数角色、源定义和接口绑定，供诊断和调用追踪使用。输出资源由 call 的绑定决定；高层 return 不提前变成 push／write／set。

## 组件引用与操作

声明包含符号、payload 类型、源类型名称和初始化函数；Queue 另有容量函数。固定一维 Queue 数组用 `acir.queue.array`，保存长度计算和元素初始化函数。`acir.ref` 用符号引用声明，并产生与声明匹配的引用类型。

| 声明／引用 | 原生操作 |
| --- | --- |
| `acir.queue` / `!acir.queue<T>` | `queue.peek/pop/push/empty/full/size` |
| `acir.cell` / `!acir.cell<T>` | `cell.read/write` |
| `acir.signal` / `!acir.signal<T>` | `signal.get/set` |
| `acir.ehr` / `!acir.ehr<T>` | `ehr.read/write` |
| `acir.queue.array` / `!acir.qarray<T>` | 索引后操作对应 Queue 引用 |

不再注册通用 resource/read/write/query/invoke 或 Queue revise。Cell、Signal 和 EHR 必须初始化且没有容量。引用、payload、输出绑定和区域返回类型由原生验证器检查；用户不手填 view、aspect 或 EHR 端口。

基础组件契约集中在 [Components.cpp](mlir/Components.cpp)。同一接口同时提供分析操作、默认 readiness、可见性和顺序，并向 Python 导出契约。Queue 的顺序为 peek → pop → query.full → push；容量视图的阶段来自该契约。payload 读取观察旧队头，新 push 数据下拍可读；获准 pop 可以释放同拍容量。Cell 始终读旧值。Signal 保留 set → get 顺序，没有有效写入时读取保留值，不传播生产者成功条件。用户 EHR 按实际 Method 顺序分配阶段，无序访问不凭任意 Rule 排序增加可见性。

## 计算、共享与控制流

普通计算复用 `arith`，普通 helper 使用 `func`。单出口结构化分支、短路和循环保存为 `scf.if`／`scf.while`；存在多个 return 的函数保留 CFG，以精确表达每个路径提供的输出槽位。聚合计算继续使用 `acir.aggregate/extract/update/length` 等辅助操作，固定数组和结构体类型保持原有范围。

Work 的动态捕获作为 Rule 参数传入，资源捕获使用明确引用。共享普通值使用 `acir.export` 保留 SSA 锚点，`acir.external` 关联另一实例中的实际定义。它们不表示运行时缓存。状态读取和共享 export 保留保守 MemoryEffect，优化不能将状态读取推测到未执行路径。

## 分析 lowering 与独立重载

```bash
pycircuit/builds/compiler/mlir/acir-opt model.acir.mlir --acir-lower-analysis
```

[Lowering.cpp](mlir/Lowering.cpp) 将 Work、Rule、Method 转换为分析函数，保留原始身份和调用边界。显式 guard 转成条件检查，输出 payload 在实际 return 路径展开为绑定资源效果，消费性 peek 产生 pop 请求。相同块中同一引用的重复消费合并，并保留贡献读取的位置；跨调用、别名和路径的消费请求按外层事务与实际资源合并为一个条件化消费。Queue full 和 push 的容量检查及 pop 的容量释放只在分析副本中展开。最后使用标准 SCF→CF 转换。

`acir-compile INPUT.mlir OUTPUT_DIR [--opt]` 验证、规范化主 MLIR，在副本上运行 lowering，再导出真实操作、SSA、CFG 和组件契约到临时 `program.json`。Python 只使用这份实际 IR 导出进行分析，不读源码或旧图。`--opt` 控制主 IR 的 canonicalize/CSE。

Method guard 在 Rule 内归入外层事务；在 Work 内沿返回值的数据和控制依赖约束相关 Rule。guard 失败不会作为条件 false 进入依赖该值的 else。普通 SSA 不增加持久 valid 状态。

主文件 `model.acir.mlir` 保留真实源码位置。临时 MLIR 输入位置与无意义的 UnknownLoc 后缀被清理；`model.readable.mlir` 隐藏位置。`acir.id` 保留源操作身份，匿名或重复 ID 根据实际操作位置区分。移动源码位置不会改变条件或顺序关系。

完整图、端口和旁路只保存在 RuleGraph；没有分析后的二次编译，也不向主 MLIR 注入端口或分析缓存。修改实际操作后可重新分析，删除 Python 源码后也可重建相同图。

## RuleGraph

JSON 的 `format_version` 为 7，保留以下派生结果：

- `contracts`、`method_summaries`：基础、用户和推导契约，公开接口及必要顺序的来源证据。
- `nodes`：事务身份、选择路径、guard、具体 Method 调用、条件化资源访问和 readiness。消费请求按实际资源合并。
- `edges`：条件化 `method.order` SB 关系、接口、资源、来源及源码位置。事务内顺序不生成自环。
- `ehr`、`ehr_bindings`：结构阶段、端口、初始化回退与访问绑定；这些字段不会注入高层 IR。
- 访问的 `bypass`：前序临时／获准写入、资格条件和旧值／保留值回退。没有跨 Rule 竞争写入的隐含优先级。
- `computations`、`preparations`、`preparation_methods`：共享 SSA、分支／循环／调用边界、条件化依赖、有效性和使用者；准备区域不是事务。
- `diagnostics`：可证明依赖环和无法证明顺序的诊断，附带来源证据。

路径证明保留现有有界布尔推导与 Queue 旧 empty/full 排斥约束；容量 EHR 不与旧 full 混用。完整循环仍在 IR 中，分析对循环携带值和动态资源索引采用保守摘要。

本轮不实现执行后端、运行时缓存、竞争仲裁或 CPU 时序验证。EHR 输出目标和新的辅助表达继续留作未决事项。
