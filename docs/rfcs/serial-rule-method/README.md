# 串行 Rule/Method 与 MLIR 事务降低扩展规格

状态：目标扩展规格，供后续实现任务使用；不是当前支持清单，也不是实现或测试通过声明。
日期：2026-10-12。关联讨论：[issue #307](https://github.com/PTO-ISA/pyCircuit/issues/307)。
交接复核：2026-10-14；main 仍为下文固定 revision。本次补充分析复用与预算验收，
区分 Signal invalid 与 EHR 旧 Q 回退，并修正历史增量与当前内部接口的描述。

本规格把简单的串行作者体验、可组合的模块操作和共同硬件 IR 联系起来。
Module 拥有状态，Rule 定义一次自主原子事务，Method 提供归入调用者事务的组件操作，
普通函数复用纯计算。MLIR 推导类型、效果、候选状态、执行条件和资源依赖，
再由同一个已验证的最终硬件产物生成 C++ 和 Verilog。

## 文档与阅读顺序

| 文档 | 负责的合同 |
| --- | --- |
| [源语言语义](source-semantics.md) | 构造、注册、Method 调用、状态顺序、guard、返回值、错误与迁移 |
| [普通纯函数](pure-helpers.md) | Rule/helper hard break、词法快照、复用 typed body、发布 ABI 与独立验收 |
| [MLIR 分析与变换](mlir-pipeline.md) | 中间表示、分析输入输出、变换前后条件、逐源发布与链接 |
| [跨实例 Method 接线](cross-instance-lowering.md) | 原实例状态、内部物理端口、根 ABI、writer 组合与独立验收 |
| [事务采样准入](sampled-admission.md) | 动态 guard 的精确 IR、双层证明、clock history、Runtime/RTL 生命周期及实施 checklist |
| [whole-Bits 状态分支与局部值合并](branch-state-ssa.md) | statement branch、D/raw-intent join、标量与 nominal local joins、D/E schema hard break、双层证明及独立验收 |
| [事务 body effects](sampled-body-effects.md) | 六字段 schema、语句时刻 SSA、效果私有路由、源认证、预算与 generated Method System 验收 |
| [条件调用与 body effects](conditional-effects.md) | statement-arm calls/effects、四态 selection、路径有效性、tagged controls hard break 与待完成验收 |
| [事务接口摘要](publication-summary.md) | 闭合字段、body checks/observations、类型与依赖作用域、body 重验及预算 |
| [观测身份与逻辑 owner](observation-identity.md) | 共同身份验证、紧凑 ReportID 证明、完整日志组和双目标 metadata hard break |
| [资源和后续扩展](extensions.md) | Queue 准入、端口、跨事务前递及明确未开放的接口 |
| [验收标准与 checklist](acceptance.md) | 独立 oracle、正反例、产物、性能证据和合入条件 |
| [实现分解与 agent 交接](implementation-plan.md) | 有界任务、依赖、独占文件、角色分离和退出条件 |
| [扩展实施与验收矩阵](extension-roadmap.md) | issue 取舍、当前边界、剩余切片、采样检查合同和接单 checklist |

实现者必须先读源语义及其任务涉及的验收条款，再修改代码。表中的各文档共同组成
本扩展；只完成语法识别、增加 verifier 或输出一份 IR，均不构成功能完成。

历史检查点 `3078e75f6a354c6a947a7d31e1a7a930978d9a0a` 包含受限的恒真跨实例
Method lowering，不证明后续候选通过。当前有界实现已加入直线与 bounded statement
branch、同域、whole Bits、
唯一实际 writer 的统一采样 lowering、有序 validity/guard、完整接线记录，以及先对照
原源计划认证、再执行 canonical final verifier 的验证。literal true/false、无状态、
只读事务同样采样；外层 Rule 的普通返回值仍要求恒真证明。C++/managed RTL 共用采样计划并实现
冻结检查与 history 的提交/丢弃。

focused final-IR 回归已有 native workers 1/2 和真实四态 RTL 执行证据。2026-10-14 的
有界候选已通过公共恒真六场景重验、短 API gate 和 examples gate。后续公共动态回归
已加入独立事务、跨 owner 原子事务和 guard 入口候选快照三种场景，以原生 workers 1/2
及真实四态 RTL 检查实际 Q、storage/sampling history、Discard/Reset 和失败零提交。
直接 body assert/log/report 已接入 state plan、统一物化、原源认证与 final proof；
focused Method `@system` 回归验证生成 runner、重复调用、完整观测身份/顺序和失败 epoch。
分析复用必须保持原预算及完整实际实例检查。
这些结果只属于对应候选，不自动覆盖后续修改，也不等于 nightly 通过。
whole-Bits statement branch 与局部值 join 的合同见
[branch state SSA](branch-state-ssa.md)。它把候选 wiring hard break 为 D/E 两个 slots，
其原有切片拒绝 conditional calls/effects。当前恢复的条件候选在此基础上接入
statement-arm Method/assert/log/report、source-path 有效性与 tagged controls：未选
guard bypass，选中 false guard hold 全部事务，reached X/Z effect selector 在 rising
非 reset 时 fault。纯分支保留四态 join；局部值现接纳同 nominal Enum 和有限无环的
Bits/Enum/Struct payload，字段及 subtree 赋值重建不可变值并保留先前别名快照。
changed Table-containing joins/reconstruction 仍拒绝；unchanged Table captures
保留既有接纳。partial/aggregate persistent state、resources、cross-domain、多 outer
writer、effectful loops 和 early return 继续拒绝。standalone conditional observations
无需 dummy Method 或 state。
条件调用与效果的验收必须覆盖默认预算下的完整公共流程；该规格、实现文件与 fixture 本身不证明
候选通过，也不证明 issue #307 完成。
实施与剩余验收入口为[扩展实施与验收矩阵](extension-roadmap.md)，精确合同见
[事务采样准入](sampled-admission.md)。本次文档不改变历史证据的候选归属，也不声明
基础 Method 或整个扩展完成。

## 适用范围与规范用语

“必须”“不得”表示目标合同的强制要求；“建议”表示不改变语义的工程选择。
代码块标为“目标源码”的内容是待实现语言的测试输入，不是当前版本的可运行教程。
“语义示意”只定义行为，不额外批准 Python 语法或原语。

本次用户要求形成规格和验收依据；此文不启动产品实现。后续 agent 在收到实现任务后，
按这里的明确范围推进。已有授权范围不重复请求确认；真正改变本规格语义的方案须先
形成具体差异并接受独立设计审查，不得在修测试时自行选择新的硬件行为。
冻结批准记录保持原字节；本文件不改写历史审批，也不把历史批准扩大到新增接口。

基线合同是 [Python language](../../reference/language.md)、
[source-unit workflow](../../development/source-unit-workflow.md) 和
[project governance](../../development/project-governance.md)。
语言参考和 authoring guide 可明确标注尚待验收的候选行为；文档描述不提前赋予其
已验收状态。历史通过结果仍仅绑定原候选。

## 设计决定

| 决定 | 目标行为 |
| --- | --- |
| 单一编译路线 | Python capture → MLIR 语义分析/变换 → verified common IR → C++/Verilog |
| 串行作者体验 | 同一事务中按语句顺序传递局部值和 owner 候选状态；每行不表示一个周期 |
| 并行硬件 | 仅真实数据、状态版本、效果及资源关系建立依赖；源码排列不串行化无关计算 |
| 事务边界 | Rule 是注册的自主入口；Method 不产生独立事务，也不独立提交 |
| 状态封装 | Method 修改其声明模块的状态，或调用另一个模块的 Method；不直接写兄弟私有状态 |
| 跨 Rule 读取 | 基础扩展默认旧 Q；源码注册顺序不提供前递或仲裁 |
| guard 与状态返回 | 必需 guard 控制整个外层事务；普通 success/retry 不自动阻止该事务 |
| 物理提交 | 所有候选都经过现有全系统检查；成功后才 Xfer，硬错误保留零提交 |
| 数据接口 | 保留普通 typed 值、Struct/Table 和现有 module 输出；不把 return 隐式解释为 Queue |
| 时序 | 不隐式插入寄存器、FIFO、pipeline、重试循环或多周期调度 |
| 有界性 | 递归、别名、展开、冲突证明与资源使用均有明确可诊断边界 |
| source-unit | 调用者只使用已发布接口；完整闭包链接验证 provider/body，不回读 provider Python |

## 实现范围分层

| 范围 | 包含 | 不包含 |
| --- | --- | --- |
| 基础 Method 扩展 | 同一物理域、固定实例、typed 方法、事务内候选状态、路径 guard、独立源单元与双后端 | 新的静态参数体系、跨域原子事务、隐式仲裁 |
| Queue 事务扩展 | 在基础扩展上实现请求/获准分离、合法容量抵扣、完整 token 原子传输 | 未定义的资源引用语法、默认策略改变、空队列旁路 |
| 前递扩展 | 明确记录 Signal/EHR 的可见性、因果和验收义务 | 在基础 Method 交付中悄悄增加跨 Rule 前递或 `After` API |
| 后续通用能力 | 独立确定一般静态参数、资源数组、动态循环和额外 memory 策略 | 从一个示例、端口数量或默认参数推导通用支持 |

后两类的未开放源接口列在[扩展决策表](extensions.md#deferred-source-interfaces)。
它们不阻塞基础 Method 扩展；基础交付也不得宣称已经实现它们。
“完整 issue #307”不是基础交付的同义词。

## 用真实设计约束语言

本规格基于 pyCircuit main
[`1e8b0208501980297b18da877d84938d3b94074d`](https://github.com/PTO-ISA/pyCircuit/commit/1e8b0208501980297b18da877d84938d3b94074d)
和 SSM
[`6272dfc6198456594dd7756e089309af0e972752`](https://github.com/LinxISA/SuperScalarModel/commit/6272dfc6198456594dd7756e089309af0e972752)
的源码需求。引用用于说明设计动机，不证明当前扩展通过验证。

- [ROB](https://github.com/PTO-ISA/pyCircuit/blob/1e8b0208501980297b18da877d84938d3b94074d/examples/rob/rob.py)：同拍完成可以使队头退休，退休可以释放分配容量。
- [流水线组合](https://github.com/PTO-ISA/pyCircuit/blob/1e8b0208501980297b18da877d84938d3b94074d/tests/fixtures/historical_examples/frontend_composition_pipeline/frontend_composition_pipeline.py)：Method 组合应减少重复握手组装。
- [S1 发布](https://github.com/LinxISA/SuperScalarModel/blob/6272dfc6198456594dd7756e089309af0e972752/model/pyc/davo/spe/ooo/s1_publish_owner_slice.py)：同一逻辑发布涉及多个独立状态 owner。
- [PFL](https://github.com/LinxISA/SuperScalarModel/blob/6272dfc6198456594dd7756e089309af0e972752/model/pyc/davo/spe/ooo/pfl.py)：返回 retry 的探测仍可能推进游标，跨拍 lease 和 generation 是真实协议状态。

SSM 的历史源 API 不成为 pyCircuit 的兼容入口。测试只提取设计中立的行为场景，
完整处理器源码、协议类型及其独立模型仍属于 SSM。编译器不得识别这些示例的名称、
宽度、常量、节点数量或源码形状来决定语义。

## 完成定义

某个范围完成，必须同时具备明确源码合同、可检查的 MLIR 阶段、独立正反例、
共同最终 IR 的 C++/RTL 执行证据、发布保护，以及对应作者文档。
未运行测试、被跳过的平台和未实现扩展必须逐项标明。文档构建通过仅证明文档质量。

实现证据按[现有证据政策](../../gates/README.md)保存于本地 ignored logs。
干净 checkout 的测试不得读取本次聊天、个人目录、历史日志、评审文本或状态计数。

条件调用与 body effects 的冻结合同见
[conditional effects](conditional-effects.md)。设计冻结不等于实现候选已验收；
其中的四态选择、路径有效性、controls hard break 和 checklist 须独立执行验证。
