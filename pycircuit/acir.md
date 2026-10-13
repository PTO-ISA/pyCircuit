# ACIR v8：三个可独立重载的 MLIR 阶段

所有编译语义由 MLIR 操作、类型、SSA、region 和属性承载。JSON 是报告，不能恢复程序。
各阶段文件使用 builtin `module` 保存 `acir.model`（版本、类型布局、top）及静态配置。

## 源语义层

`acir.module` 为展开后的静态实例，`acir.instance` 保存子实例身份；`acir.contract`
保存当前 Rule 相对于同 Module 目标 Rule 的 SB／SBR／C／CF 声明。
`acir.rule` 有显式函数类型、guard region、body region；`acir.rule.call` 保存参数 SSA、
符号引用、结果及固定组件输出绑定。调用点保留源码位置。

`acir.body` 为 Module 的计算与选择逻辑。普通计算复用 arith／func／scf／cf，
聚合值使用 `acir.aggregate/extract/update`。组件声明、初始化函数和 `acir.ref`
保留资源身份。`acir.export/external` 保存跨实例共享 SSA 来源；不复制状态视图。

源语义中的 `!acir.queue/cell/ehr/signal` 是组件引用，payload 类型独立。
Queue peek 的 consume 角色只表示实际事务路径上的消费请求；Module／guard 只观察。
没有 Method／Work 操作，也不接受旧 API。组件返回的 None 表示该槽位省略效果。

## 事务层

`acir.circuit` 保存已解析的数据流和固定无环调度。`acir.state` 保存物理资源、初值和容量；
Queue 数组已按真实元素展开。`acir.read` 返回 data 与 ready，明确携带资源、操作、端口、
事务、调用上下文与路径条件。`acir.path` 保存分支条件 SSA 和分支 ID。

`acir.fire` 接收完整布尔条件，保存事务身份与后继。`acir.effect` 的 enable 是
fire 与实际路径的合取，其 payload 与相应读取共享同一 SSA。消费请求在该层合并。
普通返回值若跨独立事务传播则拒绝。源调用全部内联，guard 和读取有效性已化为组合条件。

circuit 的 schedule 是全部事务的固定拓扑顺序，edges 保存来源／目标分支、资源及契约原因。
这些是已解析的执行信息；重载不再调用 Python 图分析。C／SBR 检查实际路径重合，
EHR 端口只来自声明约束，不从任意拓扑顺序推定可见性。

## RTL 友好层

事务读取／效果替换为组合 arith、选择器及 `acir.current/next`。
Cell 由旧值和写使能选择 next-state；Queue 由固定数组、count、消费移位与入队更新组成；
EHR 由前序获准写入选择链和最终 next-state 组成。Signal 不声明状态，以获准 set
的 payload 与 enable 生成组合 data／valid。

`acir.fire` 保留为显式布尔信号和轨迹标签，调度元数据保留供查看。
RTL verifier 拒绝残留 Rule 调用、transaction read、控制流或未展开状态效果。
断言保留为带完整使能的验证效果，不代表硬件状态。没有 SystemVerilog 或 CIRCT 依赖。

## 执行与诊断

两个执行器直接解析上述 MLIR：事务层按固定顺序评估完整 fire 并原子提交，
RTL 层评估组合 SSA 与 next-state。每拍共享 SSA 求值一次，依赖的前序 fire 已确定后才读取；
下一拍重新求值。优化、分层保存和独立重载必须保持逐拍状态与 fire 一致。
构造参数、未知符号、接口类型、非法同事务关系、未声明 EHR 可见性和合并调度环均明确诊断。
