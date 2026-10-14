# 扩展实施与验收矩阵

本文件是后续 agent 的执行入口，补充[总体规格](README.md)、[实现所有权](implementation-plan.md)
和[验收需求](acceptance.md)。状态基线为 `3078e75f6a354c6a947a7d31e1a7a930978d9a0a`；
不是移动的 main，也不是本次文档更新后的测试通过声明。

## issue #307 的取舍

issue 是设计动机，不覆盖本目录明确的语言合同。扩展优先复用当前函数式 `@module`、
typed 值、Struct/Table、source units、`@system` 和共同 IR；不迁回 issue 中的 class DSL。

| issue 中的方向 | 本规格的明确决定 | 对后续 agent 的约束 |
| --- | --- | --- |
| 模块操作嵌套调用 | Rule 调 Method，Method 调 Method，归属唯一外层事务 | 自主 Rule 不可再次嵌套调用；纯计算复用普通 `def` |
| 简单串行书写 | 局部不可变快照 + 事务内 owner 候选版本 | 源码一行不代表一个周期；无依赖计算仍可并行 |
| class 构造、隐式 self、二阶段连接 | 保留现有 typed 函数式 Module 与静态实例 | 不引入并行 authoring profile 或兼容层 |
| 返回值隐式变 Queue、赋值隐式 pop | 普通返回仍是普通 typed payload | 资源引用语法另行确定；字段名不带隐藏语义 |
| Rule 关系表、Signal/EHR | 保留为明确后续范围 | 基础 Method 不引入隐式前递、priority 或 `After` |
| Signal 与 EHR 的可见性 | Signal 无本拍获准写入则 invalid；EHR 才允许旧 Q 回退 | 不混同无状态 Wire 与有状态寄存器，不将 invalid 作为 false |
| 由编译器生成高效硬件 | MLIR 承担 effects、SSA、admission、冲突、因果及优化 | capture 不执行设计；后端不补语义；不隐式 pipeline |

ROB 对应“完成→退休→分配”的候选可见性；SSM S1 对应多个 owner 的 all-or-none；
PFL 对应 retry 返回与真实游标前进可以并存。分别用设计中立的 ordered slots、owner
bundle、retry probe 验收。具体源码依据见[总体规格](README.md)，完整处理器模型留在 SSM。

## 当前实现与剩余出口

“已有入口”只说明基线中有实现。历史测试结果须另查其精确候选证据，不能替代未来改动的
fresh verification。本表区分历史基线与当前有界实现；原生执行证据见表后说明，
未勾选项仍不构成完成声明。

| 切片 | 复用/扩展的 owner | 当前边界 | 必须补齐的退出条件 |
| --- | --- | --- | --- |
| 类型、效果与独立发布 | SourceEffects、SourceMethodAnalysis、SourceTransactionSummary、SourceMethodResolution | 有 typed 方法、闭包/摘要验证及 source-unit 入口 | 后续变化重验 body/summary 和接口-only consumer，不增加第二 authority |
| 纯 helper hard break | 现有 typed helper body 与 capture/import 路径 | 已有普通纯 `def` 路径 | 每次扩展维持纯度、快照、类型权威和旧入口拒绝 |
| state SSA 分析 | SourceTransactionStateAnalysis | whole-Bits 版本、bounded branch D/raw-intent join 和 scalar local join；partial/aggregate state 未开放 | 按[branch checklist](branch-state-ssa.md#acceptance-checklist)重验 entry inheritance、四态 intent、source history 和预算 |
| 同 owner 执行 | LowerTransactions | 同域、whole Bits、唯一实际 writer 的统一采样 lowering；bounded branch 使用 D/E slots | 保留实际执行、独立旧 Q 和失败原子性回归；conditional calls/effects 与 partial state 另行验收 |
| 跨实例执行 | 同一 LowerTransactions + 现有 Module/Instance | 原实例接线已生成完整 companion，统一采样；Collection 限制保留 | 维持[跨实例规格](cross-instance-lowering.md)全部不变量并重验公共源 |
| 动态 guard / 路径准入 | 既有 source check plan、identity、preflight、共同 lowering | 有序 validity/guard、双层证明及 C++/RTL history；bounded branch 只承载 state/local selection | 持续重验[采样准入规格](sampled-admission.md)的顺序矩阵与输出保护；conditional call/effect path 仍单独扩展 |
| 部分状态与冲突 | 既有字段/索引类型、write footprint、grant/disjointness proof | 不能由旧 behavioral 路径支持推断 Method 已支持 | whole/field/index 冲突统一，保留四态及源写意图 |
| 资源与因果 | 既有 HardwareAnalysis、primitive 端口合同 | Method 的完整 resource closure 未完成 | 合法请求准入、明确拒绝超端口与联合环；Queue 单独验收 |
| Method System 闭环 | 现有 System lowering、Runtime 与双 emitter | direct body effects 与 bounded whole-Bits branch 共用原源/final 双证明；arm 内 effects 仍拒绝 | 持续重验[body effects](sampled-body-effects.md#acceptance-checklist)及[branch](branch-state-ssa.md#acceptance-checklist) checklist，不扩大到 conditional effects/resources |
| 效果安全优化 | 既有 record-wire、纯 SSA 优化和 final verifier | 不能从能够生成代码推断效率达标 | 三项结构效率义务及明确样本的成本测量 |

建议剩余依赖顺序：保持跨实例和 bounded branch 不变量 → conditional calls/effects → partial 更新 → 资源/冲突/因果完整闭包 →
Method System 集成 → 效果安全优化。独立测试可以提前准备；基础因果和冲突验证必须在
每个切片存在，不能等资源切片才开始验证。Queue、Signal/EHR 不混入基础交付。

基线后的有界实现已接入 sampled/wiring schema、identity/prefix/物理接线证明、采样
binding/occurrence plan、GuardOnly 源分类和统一 lowering。每个实际事务均生成真实域
captures、完整 companion、有序 guards 和 validity prefixes；literal true/false、无状态、
只读事务没有旁路。外层 Rule 的普通返回值仍要求既有恒真证明，不新增 valid-result 作者接口。
detached candidate 先对照原源计划认证，再进入一次 canonical final verifier；两层验证
成功后才原子替换。C++ 与 managed RTL 使用同一采样计划及 history 生命周期。

当前 focused final-IR 和直线公共动态回归均有 native workers 1/2 与真实四态 RTL
执行证据；独立 generated Method `@system` 回归补充 direct body checks/observations。
动态回归包括独立 Rule 的局部 hold、同一
Rule 跨 owner 的原子取消、候选 guard 入口及后续覆盖、实际状态/history 零提交和恢复。
2026-10-14 有界候选的公共恒真六场景、短
API gate 与 examples gate 已重验通过；定义局部证明复用解决了原公共跨实例预算失败，
保持原限额和全部实际实例证明。结果不自动覆盖后续候选；nightly 未运行，基础 Method
checklist 仍有 conditional calls/effects、partial state、资源和效果安全优化缺口，不据此
宣称全范围完成。whole-Bits branch 候选的 fresh evidence 与独立 review 另行绑定，不能
由此前直线候选的结果推断。
当前边界见[语言参考](../../reference/language.md#method-execution-and-remaining-capabilities)。

## 动态 guard：共同 IR 合同与剩余验收

复用已有 `ac.expect`、`ac.required_checks`、CheckID、HardwareSourceCheckPlan、
Runtime Precheck/Precommit/Xfer 和 RTL commit gate。原检查点的 `HardwareResolvedSourceCheck`
只有 reset；当前实现增加独立采样索引、实例计划及双后端冻结快照/history。
DFFE 的 clock history 位于 Current/Pending；不能把 reset-gated
普通 assert 当作动态 Method guard 的完整实现。

[采样准入规格](sampled-admission.md)已将这些事实固定在**同一检查表示**中：必填源
guard span、final-only sampled/wiring 属性、expect sampling operands、共同 proof API
和 history plan。不得创建另一 runner 或后端私有事务检查器。派发依照该文的依赖与
所有权表冻结具体 C++ 类型/文件；不再重开一个平行 admission pass。下表概述行为。

| 必须表达的事实 | 强制行为 |
| --- | --- |
| 实际 domain | 绑定外层 transaction 的真实 clock/reset occurrence；含只读/无状态 Rule |
| 当前与候选 clock history | 成功采样后统一提交，失败统一丢弃；与实际 storage 的边沿判定一致 |
| guard 入口快照 | 普通 capture 为词法 SSA，owner 为该调用前的候选；body 后写不能反向改变 guard |
| 到达某检查的条件 | 来自支配路径与先前已知通过的 guard，不能用最终 fire 反向屏蔽早先错误 |
| invocation 身份与顺序 | 外层 registration + 实际调用 occurrence/frame + obligation；重复调用不混同，顺序可验证 |
| 返回有效性 | invalid 不当 false，不能借 else 使用无效值；首批 guarded Rule 仍只允许 None |
| 检查生命周期 | Work 建立完整快照，所有检查通过后才 Xfer；Discard 清除候选及检查快照 |

新增 guard knownness 检查只在所属域的有效采样边沿、reset 未激活且实际到达时触发。
原有 clock/reset 未知值诊断继续由现有物理域合同处理，不能通过上述表述把未知 clock
当作“没有边沿”。不新增算法可见寄存器；必要的采样历史属于既有生命周期语义。

必须保留下列顺序行为，不能仅生成全部 guards 的交换性 AND：

| 同一事务中到达的 guard 顺序 | 期望 |
| --- | --- |
| true → true | 候选可提交，仍受全系统检查约束 |
| true → false | 该 Rule 全部 hold，无关 Rule 可提交 |
| X/Z → false | 到达的未知控制导致全系统零提交 |
| false → 下属 X/Z | 该 Rule hold；未到达的下属新检查不激活 |
| reset 激活 → X/Z guard | reset 屏蔽新增 guard fault，按原复位合同处理 |
| held/falling → X/Z guard | 不激活本次新增 sampled-edge guard fault，保留原检查合同 |

已知 false 只禁止该 Rule 的数据更新；成功系统 epoch 仍按既有 storage 合同维护 domain
clock history，包括 held/falling 输入。仅全系统硬失败或 Discard 才丢弃候选 history，
不得把“不 fire”实现成“不采样时钟”。

`eq(p,p)` 对四态 p 不能折为 true：已知 0/1 得 1，X/Z 得 X。若用其表达 knownness，
必须有独立 C++/四态 RTL 及优化前后证据，不只依赖 emitter 的当前写法。
普通 `success/retry/accepted` 仍是数据；可选探测失败但推进游标必须保持。

## 分支、部分状态与资源的实现约束

whole-Bits 分支的精确合同和验收见[branch state SSA](branch-state-ssa.md)。分支 join
必须从每个进入路径的 candidate 生成；未写路径保持进入该分支的候选，
而非无条件退回最初 Q。字段/元素更新保留 nominal type、完整索引宽度、未触及字段、
写意图和条件 footprint。不能用整记录最后写覆盖其他事务的已证明独立字段。
未知索引的读/写遵循既有类型与四态原语合同，不由新 Method lowering 私自统一为失败。

分析必须区分 MustAlias、可证明独立和 MayOverlap；不能证明独立的跨事务重叠访问
保守拒绝。路径互斥只能使用可检查的 SSA 证明，不凭源码次序、参数名称或任意优先级。
一次事务的多次访问也必须满足真实 memory/Queue 端口与延迟，串行代码不赋予无限端口。

Queue 另按[请求/准入合同](extensions.md#queue-request-admission)构造 prospective
capacity，再物化 accepted transfer。禁止 `fire → pop → ready → fire` 自循环、
运行时固定点或隐式寄存器。资源源接口仍未开放时，内部 IR 正例不得当作 Python 通过。

## pass 的完成标准

每项职责需有可检查表示，不强制每行新建一个 pass。优先扩展现有 owner：

| 职责 | 输入 → 可检查产物 | 变换后必须重验 |
| --- | --- | --- |
| 绑定/类型/效果 | capture + 接口 → typed source operations | nominal/source kind、owner、域和来源 |
| 跨过程摘要 | typed definitions/imports → 条件 effects/调用闭包 | provider body 权威、递归、预算 |
| state SSA | 调用展开与 owner → Q/候选版本、frame、join | 快照、所有读版本来源、partial write footprint |
| admission | 版本 + 路径 + guard → value validity / reached / fire | 有序未知控制、false-local-hold、check domain |
| 资源/冲突/因果 | 请求 + footprints + 原语 → 可实现准入与依赖 | 别名、端口、全定义环、无任意 winner |
| 物化 | 已闭合证明 → 普通规则、组合图、storage D/E、checks | 一致接口、无高层残留、失败原子性、双后端通用 |
| 优化 | 已验证语义 → 更小的等价普通图 | effects、四态、检查/写意图保留、缓存失效 |

单独列出不变 IR analysis 和 mutating pass；分析计划存在不等于执行完成。
SourceLink 的真实 stage capture 继续作为物化前后证据，不用手工重新拼出的 IR 代替。

## 后续 agent 接单与交付 checklist

接单前：

- [ ] 指明实现哪一行能力，引用源语义和 `METHOD-*` 验收 ID，列出明确不在本切片的范围。
- [ ] 核对实际 revision、dirty 内容、当前实现与批准记录，不从本表推断测试已通过。
- [ ] 对新增共享 IR/API 完成真实 architect + 独立分解/设计审查；冻结输入、输出、
  schema、拒绝边界、预算、诊断责任、失效策略与准确文件所有权。
- [ ] 给出迁移前后接口及删除的旧入口/重复算法；hard break 同时修改所有保留 caller，
  不增加 alias、shim 或旧 route fallback。
- [ ] 独立测试所有者先确定数值/时序 oracle、正反例、公共源和完整 IR artifact。

提交验收时：

- [ ] 每个需求记录 `implemented` 与 `PASS/FAIL/NOT-RUN` 两个维度，不能以文件存在勾选。
- [ ] 最小 native/lit 测试通过，再运行适用既有短 API/example gate；nightly 单列未运行。
- [ ] 公共源独立 compile → 完整 closure link → 同一 final 双 emit → C++ workers 1/2
  和真实四态 RTL；对应 `@system` 集成不能用 module-only host driver 替代。
- [ ] reset/hold/Discard、old Q/快照、全系统 state + clock history 零提交有独立探针。
- [ ] 负例具有责任对象和源位置；compile/link/emit 与 stage output 失败不覆盖已有产物。
- [ ] 结构效率：不加延迟/存储、指定样本未用临时字段消除、等价纯计算共享；面积/时序
  只有实际综合才能报告，操作数下降不作为替代证据。
- [ ] `METHOD-ANALYSIS-BOUNDS`：冻结规模的公共源在现有预算内通过，极小预算反例
  原子失败；准备结果复用、准确绑定与变换后失效都有独立证据。
- [ ] 保存精确候选、工具、源与 oracle 哈希、完整命令/退出码、stage files、final 哈希和
  双目标清单；测试不读取本地日志或本规格作为行为 oracle。
- [ ] 更新当前语言文档的真实支持范围；独立 reviewer 检查代码与证据，列出剩余缺口。

证据使用[现有验收命令](acceptance.md#commands-and-gates)和 ignored
`docs/gates/logs/<run-id>/`。全部基础 checklist 关闭才可称“同域 Method 基线完成”；
Queue、Signal/EHR 和一般参数化分别验收，不能把基础完成写成整个 issue #307 完成。
