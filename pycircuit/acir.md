# ACIR v9：三个可独立重载的 MLIR 阶段

所有编译语义由 MLIR 操作、类型、SSA、region 和属性承载。JSON 是报告，不能恢复程序。
各阶段文件使用 builtin `module` 保存 `acir.model`（版本、类型布局、top）及静态配置。

## 源语义层

`acir.module` 为展开后的静态实例，`acir.instance` 保存子实例身份；`acir.contract`
保存当前 Rule 相对于同 Module 目标 Rule 的 SB／SBR／C／CF 声明。
`acir.rule` 有显式函数类型、guard region、body region；`acir.rule.call` 保存参数 SSA、
符号引用、结果及固定组件输出绑定。调用点保留源码位置。
Module 返回接口在前端绑定到调用者明确声明的组件，源层 Rule call 的 output_binding(s)
直接引用这些目的地；不保留 `.outputs`，不自动生成输出组件，不增加额外转发事务。

`acir.body` 为 Module 的计算与选择逻辑；没有 `__call__` 的实例不生成 body，
其被调用 Rule 和状态仍归属于该实例。源层前端直接生成 scf.if，静态循环在构造时展开。
事务提取直接处理结构分支，按条件选择 payload／ready，不先展平为 CFG；独立输入的无环 CFG 仍可转换。
`acir.module.call` 保留调用位置，递归传播父路径与有效性；不产生 Module 级事务。普通计算复用 arith／func，
聚合值使用 `acir.aggregate/extract/update`。组件声明、初始化函数和 `acir.ref`
保留资源身份。`acir.bind` 是同类型的纯 SSA 绑定，名称和作用域只存在于 NameLoc；
参数在各自区域的块参数位置保存名称。helper 展开用 CallSiteLoc 保留调用上下文。
`acir.export/external` 保存跨实例共享 SSA 来源；不复制状态视图。

`acir-source-opt` 是后端无关的源层归一化：保留首个稳定绑定的 NameLoc，消除纯别名，
在 helper／Rule 内折叠常量与结构分支。多结果分支保留各结果名称，复用的结构值保留为整体。
Module 主体中的静态调用点不因条件恒假而删除，因此优化前后事务编号、调度和 fire 轨迹模式不变。
互斥分支沿既有 CFG 工作表的访问次序编号，切换到结构化提取不改变旧程序的事务标签。
源层不包含 C++ 语句、缓存或 RTL 物理资源展开；保存文件可单独重载并进入两个后端。

源语义中的 `!acir.queue/cell/ehr/signal` 是组件引用，payload 类型独立。
Queue peek 的 consume 角色只表示实际事务路径上的消费请求；Module／guard 只观察。
没有 Method／Work 操作，也不接受旧 API。组件返回的 None 表示该槽位省略效果。

## 事务层

`acir.circuit` 保存已解析的数据流和固定无环调度。`acir.state` 保存物理资源、初值和容量；
组件各有固定真实身份，`acir.resource.array` 保留 shape、坐标到资源的映射和别名。
结构化 payload、mask 及 selected array 保留到本层；动态索引记录实际选择的就绪条件。`acir.read` 返回 data 与 ready，明确携带资源、操作、端口、
事务、调用上下文与路径条件。逻辑单选读取的 operands 为 `(path, index[, domain])`，
逻辑单选效果为 `(enable, payload, index[, domain])`（pop 省略 payload）。
`resources` 是候选资源列表，`array` 为逻辑数组名，`ports` 为对应固定端口；domain 是扁平 mask。
调度分析临时枚举候选资源并保存已解析的图；事务层不展开运行时单选访问。
`acir-expand-resources` 是可独立调用的物理展开 pass，RTL pass 自动在克隆副本上调用它。
`acir.path` 保存分支条件 SSA 和分支 ID。

`acir.fire` 接收完整布尔条件，保存事务身份与后继；可选的 `checks(...)` 是判断 fire 前
必须求值的验证依赖。每项检查保留原来的选择／短路使能；断言成功后为 true，失败直接报错。
条件归一化可将这些依赖从重复的就绪表达式中提取出来，不能因后续容量条件失败而漏掉原本必需的检查。
`acir.effect` 的 enable 是
fire 与实际路径的合取，其 payload 与相应读取共享同一 SSA。消费请求按真实资源合并；动态别名也由执行器／RTL 合并一次消费。
普通返回值若跨独立事务传播则拒绝。源调用全部内联，guard 和读取有效性已化为组合条件。

circuit 的 schedule 是全部事务的固定拓扑顺序，edges 保存来源／目标分支、资源及契约原因。
这些是已解析的执行信息；重载不再调用 Python 图分析。C／SBR 检查实际路径重合，
EHR 端口只来自声明约束，不从任意拓扑顺序推定可见性。

## RTL 友好层

事务读取／效果替换为组合 arith、选择器及 `acir.current/next`。
Cell 的聚合 payload 和 Queue 的固定容量存储在此物理展开为标量状态，Queue 同时保存 count；
数组选择、索引和更新化为比较、mux 与逐元素使能，不残留 acir.array.* 或 acir.update。
EHR 由前序获准写入选择链和最终 next-state 组成。Signal 不声明状态，以获准 set
的 payload 与 enable 生成组合 data／valid。

`acir.fire` 保留为显式布尔信号、验证依赖和轨迹标签，调度元数据保留供查看。
RTL verifier 拒绝残留 Rule 调用、transaction read、控制流、array 操作或未展开状态效果。
`logical_states` 属性保存逻辑状态与标量存储的映射，用于恢复原 shape 的逐拍轨迹。
断言保留为带完整使能的验证效果，不代表硬件状态。没有 SystemVerilog 或 CIRCT 依赖。

## 执行与诊断

数组类型写为 `!acir.array<T, [D0, D1, ...]>`；mask 为 `!acir.mask<[...]>`，内部选取值为 `!acir.selected<array-type>`。
源层通过 aggregate、array.index/select/domain、update 和谓词的有限组合计算表达数组；
match 谓词在前端按静态 shape 展开为 SCF／SSA，未引入运行时回调。
Module 数组由 `acir.instance.array` 保留静态实例表，动态 Rule 调用展开为有限候选的条件化调用。
形状、聚合字段、选取类型、更新路径和状态初值受原生 verifier 校验，独立重载同样受约束。

两个执行器直接解析上述 MLIR：事务层按固定顺序评估完整 fire 并原子提交，
RTL 层评估组合 SSA 与 next-state。每拍共享 SSA 求值一次，依赖的前序 fire 已确定后才读取；
下一拍重新求值。优化、分层保存和独立重载必须保持逐拍状态与 fire 一致。
构造参数、未知符号、接口类型、非法同事务关系、未声明 EHR 可见性和合并调度环均明确诊断。
