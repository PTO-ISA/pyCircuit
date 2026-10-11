# ACIR MLIR v6

MLIR 是唯一可独立重载的程序表示，不保存 Python AST，也不依赖 JSON 图恢复计算。注册 dialect 使用 `func`、`arith`、`cf` 保存计算、函数调用、SSA 块参数、分支和循环。v3/v4/v5 产物必须从 Python 源码重新编译，编译器拒绝继续使用旧格式语义。

`module` 的 `acir.model` 保存版本、top、配置、payload 类型、导出连接、实例层次和源组件契约表 `contracts`。契约有稳定 `id`、公开 `methods`、`relations`；每条关系保存 `before`、`after`、`relation="SB"`、`source` 和来源。关系表没有 `when`、ready、guard 或路径条件。未声明且未推导出必要顺序的接口对默认 CF。

基础契约为 Queue、Cell、Signal 与 EHR；用户组件契约身份是 `Module:<实例路径>`。实例保存 contract 和 method 到稳定 interface 的映射；资源保存 contract，固定资源数组的元素分别绑定身份。层次路径使用点分隔，名字中的下划线不会成为层次分隔符。导入的静态全局实例使用保留的 `.global.` 前缀。

| 表示 | 契约 |
| --- | --- |
| `!acir.queue<T>` / `!acir.cell<T>` / `!acir.signal<T>` / `!acir.qarray<T>` | 状态引用，与 payload SSA 分开 |
| `!acir.struct<"Name">` / `!acir.array<T,N>` / `!acir.vector<T>` / builtin `tuple<...>` | 普通 payload 或返回值结构 |
| `acir.resource` | 资源身份、组件契约、初始化和容量函数；固定数组另保存长度与元素初始化 |
| `acir.get` / `acir.config` | 固定资源身份和构造配置 |
| `acir.read` | old、ehr 或 Signal current 视图；EHR 复用 Cell 引用，Queue 容量用 `aspect="full"` 的 bool 读取；保留角色和派生端口 |
| `acir.query` | empty/size 查询旧状态，ready=true；full 通过 `acir.read` 读取容量 EHR |
| `acir.pop` / `acir.push` / `acir.revise` / `acir.write` | 当前路径的事务效果；revise 更新旧队尾 |
| `func.call` | helper/method/guard；method 保存 instance、contract、interface、method 绑定 |
| `acir.guard` | 显式 guard；求值只观察状态，控制后续行为 |
| `acir.invoke` | Module 选择具体 rule，保存 callee 和参数 SSA，无 SSA 返回值 |
| `acir.export` | 为共享普通值保留稳定 SSA 锚点，不重新读取状态 |
| `acir.external` | 引用另一实例准备函数的 SSA 定义，source/identity 对应真实 IR |

原生 printer 使用 TableGen 声明式 assembly format，按操作名、操作数、必要属性和类型打印 ACIR 操作。资源 name/kind/type、查询 kind、guard/invoke name 与 external source/identity 直接放在操作后，其余语义属性保留在属性字典中；源码位置仍输出为 `loc(...)`。例如：

```mlir
acir.resource "input" "queue" "u32" {contract = "Queue", visibility = "old"}
%q = acir.get "input" : !acir.queue<i32>
%value = acir.read %q {acir.id = "v1", role = "observe", view = "old"} : !acir.queue<i32> -> i32
%empty = acir.query %q "empty" : !acir.queue<i32> -> i1
acir.pop %q : !acir.queue<i32>
acir.push %q, %value : !acir.queue<i32>, i32
acir.guard %empty "enabled"
acir.invoke "top.consume"(%q) : (!acir.queue<i32>) -> ()
```

聚合、索引、更新和 revise 的可变操作数保留完整函数类型；`extract` 的 field、`update/revise` 的 path 都保留在属性字典。固定 bool/i32 结果或 guard/check 的 bool 输入由 ODS 类型约束表达。所有操作仍接受原有带引号的通用 MLIR 语法；Python 前端继续生成通用语法，仅原生 printer 负责简洁输出。此次只改变文本与数据布局，不改变控制流、分析阶段或程序语义。

Queue 采用 Pipeline FIFO：read/pop/revise 的 ready 为 `!empty`，读取只观察旧队头；push 的隐式 guard 读取 `!ehr.full(queue,1)`，跨事务只由本拍获准的 pop 释放容量。新 push 数据下拍才可读。payload 声明 `read SB revise SB pop`，内部容量视图给出 `pop SB query.full`，push guard 调用关系给出 `query.full SB push`，闭包包含 read→push，即使中间接口没有实际调用。Cell read/write 默认 CF，write 下拍可见。

Signal 使用 `!acir.signal<T>` 与 `visibility="current"` 声明，必须保存 payload 初值函数，不保存容量函数。`acir.read/write` 分别绑定 `interface="get"/"set"` 和 `view="current"`，不保存 EHR port。基础契约 `Signal` 声明 `set SB get`。访问的 `local_condition` 表达临时写入路径，`condition` 表达完整事务判断后的发布条件；顺序边使用判断路径，不引入 writer-fire 准入条件。

Signal get 的派生 `bypass` 保存 `selection="last_local_else_granted_set"`、候选 set、资格、事务、写入顺序、`fallback="retained"` 和 `commit="transaction"`。同事务选择最后一次有效前序 set，资格不要求事务最终获准；跨事务资格包含 `granted(transaction)`。没有有效写入时读取保留值；竞争写入的仲裁未定义，候选列表的顺序不表示跨 Rule 优先级。Module current 读取进入 preparations 并关联实际使用者，保留路径及 method guard。

Rule-var 普通返回通路与 `acir.result`、`var.validity`、生产者有效性字段已经移除。普通参数、helper/method 返回和 Module 共享 SSA 仍保留；Rule 的 Queue/Cell/Signal 返回都表达资源效果，调用只恢复静态连接。

EHR 资源仍使用 `!acir.cell<T>`，声明附带 `visibility="ehr"` 和普通 `initial_fn`。读写使用 `view="ehr"`，不新增专用 operation。Queue 资源声明 `views.full`，初始化表达式为旧数量等于容量；容量读写直接引用 Queue，以 `aspect="full"` 表示 bool 内部视图，verifier 检查类型。pop 后的 `acir.write false` 位于早端口，显式 full 和 push 隐式 guard 的 `acir.read` 位于晚端口。隐式 guard 与就绪要求单独保存，不能当成普通分支裁剪必要依赖。

结构收集忽略 guard 是否获准，保存派生的 `ehr_bindings`，确保拒绝的 method 仍保持阶段；有效访问按真实路径另行收集。派生的顶层 `acir.ehr`/JSON `ehr` 保存每个具体资源视图的 `port_count` 与 method 绑定。操作的 `ports` 保存具体资源映射；单一绑定另有 `port`。声明契约与派生结果分开，重载删除派生信息并从实际访问重新分配。无序读写共阶段，严格 SB 要求更晚阶段；合并同阶段后以阶段 DAG 的最长路径分配连续端口。纯读取间的 SB 不提高端口，同阶段与严格先后冲突时报告来源。helper 使用最内层 method 的阶段，嵌套 method 保留自己的身份，直接访问为端口 0。

访问的 `bypass` 保存前序端口候选写入、写入条件、事务身份、同一 method 内写入序列和旧值回退。跨 Rule 候选必须带 `granted(transaction)`；同 Rule 候选使用临时写入条件，整体仍提交或取消。不同事务的非法竞争写入没有隐含优先级。full 的 SSA 条件使用独立的、按事务区分临时视图的 `ehr.full(resource,port)@transaction` 身份，不与旧 full 合并，不参与旧 empty/full 排斥证明。

函数的 `acir.function` 保存角色、源定义、实例归属、参数类型、资源身份、消费角色及输出绑定。method/helper 按调用绑定生成专门函数，guard 观察角色不继承消费。已注解、没有资源参数且不捕获动态准备值的公开 method 另保存 `summary=true` 的普通函数，用于未调用接口的契约闭包；捕获动态值时使用实际调用绑定。不新增图专用 operation。helper 和 guard 使用定义所属实例的词法环境，避免调用者参数或另一实例的同名状态污染捕获。

`model.acir.mlir` 只保留真实源码的 MLIR location；临时 `.analysis-*/input.mlir` 路径、无效 `loc(unknown)` 后缀及旧 `acir.type:*` 标记在重载与打印时清理。块参数使用 MLIR 类型，函数参数的源类型仍由 `acir.function.params` 保存。额外的 `model.readable.mlir` 使用原生 printer 隐藏调试位置，便于阅读；它保留计算和语义属性，重载后的诊断不再具有操作源码位置，完整位置追溯应使用主文件。

`acir.id` 关联稳定源 SSA 定义。条件、访问与调用上下文的身份不依赖 location；匿名或重复源 ID 的操作按实际函数/CFG 中的操作位置区分，边与访问排序也使用这些身份。只移动源码位置不会改变条件与关系，旧分析图中的派生 ID 在重新分析时更新，ACIR 格式仍为 v6。read/query 与共享 export 保留保守 MemoryEffect，不能被删除、CSE 或推测到分支外。其余合法纯计算可 canonicalize/CSE。分析支持优化生成的资源 `arith.select`，保存各候选身份与选择条件；循环携带依赖通过固定点传递至 helper 返回值及共享 exports。

`acir-compile INPUT.mlir OUTPUT_DIR [--opt]` 解析和验证 MLIR，导出实际函数、操作、SSA 与 CFG 的临时 `program.json`，并规范化 MLIR。Python 分析重新传播资源身份、路径、guard、ready、Signal 可见性和共享计算来源，不读取 Python。完整 RuleGraph 保存在分析器内存，并导出独立的 `rule-graph.json` 和 `rule-graph.dot`；MLIR 不再嵌入 `acir.analysis` 或推导后的 `acir.contracts`。`acir.model.contracts` 继续保存原始组件契约，顶层 `acir.ehr` 和操作上的 `port`/`ports` 继续用于现有端口处理接口。加载已有 v6 文件时仍接受并清除旧 `acir.analysis`、`acir.contracts`，同时清除派生 EHR 标记，再依据 `acir.model` 声明和实际 SSA/CFG 重算；旧摘要不能覆盖实际操作。

分析收集基础操作及 method 调用，绑定具体实例，查表和计算 SB 闭包，结合路径与 guard，再提升为 rule 顺序。嵌套 method 的关系向父组件传递，用户声明与内部关系合并。无条件且资源绑定精确的接口摘要参与组件级闭包；有条件、带资源参数或动态资源索引的推导保留具体调用绑定，避免将一组绑定的必要顺序错误套用到另一组。

JSON 保存以下内容：

- `contracts`：默认、用户和推导关系、来源证据及接口闭包。带 `bindings` 的推导仅适用于具体调用，路径条件位于对应调用记录。
- `ehr_bindings`：独立于事务获准条件的结构访问绑定，供端口分配和操作标注。
- `ehr`：逐具体资源及 aspect 的连续端口数、method 绑定、旧值回退和提交归属。
- `method_summaries`：静态接口摘要函数及其绑定访问；未调用接口也可检查。
- `nodes[].methods`：实际 method 调用、稳定身份、组件、参数、路径、guard、基础访问身份和嵌套祖先。
- `nodes[].accesses`：transaction、function、resource、operation、condition、ready、view、role、接口契约、绑定精度、位置及调用链；EHR 另有 aspect、phase、port、local_condition 和 bypass（含写入来源及资格条件）。按事务与实际资源合并隐式消费。
- `nodes[].guards`、`requirements`：显式 guard、路径上的基础 ready 及准备计算有效性，分别保存。
- `edges`：统一的 `method.order` SB 边，包含接口、契约、证据、条件、资源或值身份、访问身份和源码位置。
- `computations`：准备计算的访问，共享 SSA 来源同时关联使用它的事务。
- `preparations`：从实际 SSA 定义导出的准备区域，保存 `id`、`function`、调用 `context`、`block`、源 `identity` 与位置、operation/dialect、`inputs`/`outputs`、执行 `condition`、`output_validity`、分支／循环／调用 `boundary`、`control_dependencies`、`state_dependencies`、`method_dependencies`、条件化 `requirements` 及 `users`。输入输出 SSA 身份在对应 function/context 中解释，完整 CFG 与块参数绑定仍在 IR 中。
- `preparation_methods`：准备阶段的具体 method 调用和祖先绑定；使用者的 `nodes[].methods` 通过 `source_method` 关联原调用。关联访问通过 `source_access`、`source_methods` 保留原访问及调用来源，视图、端口和调用链不被改写。同一次共享访问在每个使用它的 Rule 中只关联一次，`condition` 合并使用路径，`vias` 保存各处使用原因。
- `nodes[].preparations` / `preparation_bindings`：该 Rule 所需区域及条件化使用关系；区域不是事务节点，独立调用保持不同身份。基础 ready 随消费关系传播，未使用的准备读取不会约束其它 Rule。

`preparations` 与上述关联保存于独立 `rule-graph.json`，不嵌入 MLIR，不新增计算 operation，ACIR 格式保持 v6。重载从实际 IR 重建。Module 可调用只读 method，返回值保留调用 guard 的有效性；变量 guard 直接引用声明作用域的 SSA 值，保留原计算、状态视图及计算自身的有效性。分支保护的读取保留执行条件，循环计算保留边界。设计目标见 [编译阶段的设计意图](docs/compiler-intent.md)；静态共享标记不等同于运行时共享求值。

同一事务内的 SB 不生成 rule 自环，也不拆分事务。能够证明调用互斥时排除约束，其余情况保守保留。矛盾顺序的诊断可追溯到用户声明、基础默认关系、嵌套调用和位置；DOT 同样展示接口、条件和来源。

路径证明只处理常量、共享 SSA 的简单布尔互斥，以及正容量 Queue 的 empty/full 不共存。布尔展开有明确上限；超限不被认作可确认环。循环摘要对循环携带值和动态资源索引保守处理，完整 CFG 始终保留。诊断区分 confirmed-cycle 与 unproven-order。

本阶段只验证静态编译与依赖分析，不定义运行时激活、冲突仲裁、竞争写入检查、提交算法、仿真后端或 CPU 时序 benchmark。
