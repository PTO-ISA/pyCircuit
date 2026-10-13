# 普通纯函数的扩展合同

状态：目标合同；实现状态和测试结果另由候选证据记录。本页细化
[PURE-HELPER](source-semantics.md#pure-helper)，不建立另一套前端。

## 作者接口与角色

普通 typed `def` 负责可复用的纯计算；`@rule` 负责自主事务的声明与注册；
`@method` 负责归入调用者事务的模块操作。不得根据是否返回值、是否写状态或调用
次数自动改换角色。移除旧纯 scalar `@rule` 调用路径后，不保留兼容分支。

| 项目 | 必须支持或拒绝的边界 |
| --- | --- |
| 定义位置 | 源文件顶层，或直接位于 Module/System 内；helper/Rule/Method 内再定义 helper 拒绝 |
| 签名 | 显式 typed 必需普通位置参数，零或一个 typed 结果；`-> None` 表示无值 |
| 参数形式 | defaults、keywords、variadic、positional-only、keyword-only 拒绝 |
| body | 已准入表达式、局部 Assign/AnnAssign、不可变 Struct 重建、`if/else`、`pass`、末尾 return |
| 控制流 | helper 内循环、分支早退、递归、异常和一般 Python 执行拒绝 |
| 效果 | allocation、隐式 owner 读取、Rule/Method 调用、assert/观测和 host I/O 拒绝，包括未使用或死路径 |
| 调用点 | 普通表达式、Rule/Method body、纯 guard、现有 bounded-loop caller 和 named Table callback 共用实现 |
| Table callback | 保留现有 fixed-Bits 输入/结果及无 captures 限制；expression lambda 中调用 helper 不因此开放 |

每个 actual 按源码顺序求值并完成类型边界绑定一次，然后才处理下一个 actual。
物理位宽相同不能取代 Integer、Boolean、fixed Bits 和 nominal 类型权威。
显式 fixed 目标复用既有 unsigned boundary：已证明适配的 Integer 常量或有限区间
可以转换，不能沿用旧 scalar-Rule helper 的一律 exact-kind 限制。Boolean/Integer
逻辑 formal 仍要求对应 source kind；named Table callback 仍要求 exact fixed 类型。
helper 内的运算、Table 范围证明及构造规则复用原有合同，不另设更宽松的纯函数规则。

Rule 必须作为 Module/System 中完整的静态 statement call、assignment value 或 return
value 注册，每个声明在每个实例中恰好一次。Rule 不可嵌入表达式、Table callback 或
另一 callable。多个真正独立的事务使用不同声明身份；不得将 stateful Rule 改名为
helper 来绕过重复注册检查。

## 词法绑定与快照

嵌套 helper 在声明位置捕获普通 SSA 值。后续同名 rebinding、调用者局部变量或同名
formal 不得改变这个快照。普通 module-result wire 可以捕获；owner、instance handle、
clock/reset 不可隐式捕获。分类依据实际绑定身份，不能仅看名字或 producer opcode。
需要 owner payload 时，作者通过普通显式参数传递其值。

同一词法 Module/System 内，helper body 可以引用稍后声明的 helper；实际直接静态
调用时，其完整 callee/capture 声明闭包必须已经可用。例如依次声明 `first`、`second`
后调用 `first` 可以解析 body 中的 `second`；在 `second` 声明前直接调用则须诊断。
Rule/Method 的定义和注册不执行 Python body；其调用仍须满足 helper 声明闭包可用
的边界，不能执行 Python 来补全尚不可用的快照。每个 callee 使用自己的声明快照，
调用图整体无环。传递 captures 按原始 SSA 身份绑定，不在调用者环境按名字重新查找；
不为 helper 增加任意前向 SSA 求值机制。

用户 formal/local 与 Rule/helper 同名时按实际词法绑定解析。内部额外的 capture
transport 不得覆盖用户绑定，不得因用户拼出内部前缀而丢失、改值或改变语义；跨
Rule/Method region 的快照传输遵守同一要求。顶层 helper 无运行时 captures；嵌套
helper 私有，不作为 namespace export。

## 复用现有 MLIR 与 source-unit

使用现有 `func.func` 和 `ac.helper_kind="value"` 表达纯 body，不制造虚假的 Module、
Rule 或 Instance 作为 helper 容器。物理签名为 `(explicit inputs..., captures...) ->
() | ordinary result`。旧 value-helper 的 evaluation-path 参数、i1 valid 结果、
`logical` 参数合同和 `ac.check_templates` 必须删除，不能兼容解码。
`record_constructor` 保持其已有独立静态构造职责，不按 value-helper ABI 解释。

| 元数据 | 要求 |
| --- | --- |
| `ac.source_owner`、`ac.origin` | 复用现有源所有权和完整定义 occurrence |
| `ac.declaration_role` | `definition` 或 `import_snapshot`，由既有发布合同校验 |
| `ac.parameters` | 复用现有六字段 hardware 参数约束，仅描述显式参数 |
| `ac.result_constraints`、`ac.return_form` | 零或一个结果约束，`none` 或 `single` |
| `ac.capture_types` | TypeAttr 数组，与物理签名末尾 captures 一一对应 |
| 嵌套私有声明 | `sym_visibility="private"`、`ac.lexical_owner` 指向所属 Module/System；符号包含完整 owner 前缀 |

复用现有表达式、语句、branch environment、immutable field update 和 binding boundary
建立真实 typed body。源端调用、导入 body、callback 和 loop caller 共用 resolver、body
验证及 typed IR inliner。进入 helper body 时清除 Rule/Method/state/instance 上下文，
只保留显式参数、词法普通 SSA 和合法纯调用。

native verifier 必须检查原始未优化 body：ABI、逻辑类型、region/signature 形状、SSA
支配/隔离、operation 纯度、private owner、完整 callee 闭包及所有路径。只准入既有
共同硬件运算及其合法 Table regions，不通过 whitelist 外的任意 dialect 绕过硬件类型。
格式错误的 region、function type、callee 或 result 必须诊断，不能 assert/crash。

递归、闭包深度、验证工作量和展开规模均有饱和计数边界。先按 typed DAG 计算并
支付展开成本，再 clone；计入 operands/results、captures、aggregate、region 和重复
call。复用 TableQueryBudget 与既有 loop budget，不能在指数展开后才报告超限。

provider 独立发布 typed helper body；consumer 仅读已发布接口。link 校验实际 owning
body 与快照配对，不能以 consumer snapshot 补足缺失的 provider body。私有 helper
不能跨词法 owner 调用。通过权威验证且完成内联后，删除 source-stage value helpers；
最终可执行共同 IR 不得残留 helper `func.func`/`func.call`。两个 backend 不增加 helper
解释器或源语义补丁。

## 独立验收 checklist

- [ ] 普通 def 的 locals、annotations、if/else、Struct、None 及 acyclic calls 有正例；不合法 grammar/effects 有负例。
- [ ] 快照在后续 rebinding、跨 callable region、前向 helper 声明和传递 capture 中保持原值。
- [ ] Rule/helper 同名的 formal/local 及内部 capture 前缀碰撞有独立数值 oracle。
- [ ] ordinary module-result wire capture 接纳；直接 owner/instance capture 拒绝。
- [ ] Rule/Method/guard/loop/named Table callback 使用同一 helper 路径；effectful guard 仍拒绝。
- [ ] 旧 pure-Rule helper 位置拒绝；纯自主 Rule 可合法注册；重复/循环/嵌套注册拒绝。
- [ ] 65-bit、X/Z、actual 顺序、类型权威和展开预算保留原有 oracle，不用 prepare-only 代替执行。
- [ ] helper-only provider 独立 compile；删除 provider Python 后 consumer compile、完整闭包 link、双目标 emit 成功。
- [ ] body/type/capture/owner tamper、缺失 provider、非法 private export 和 malformed IR 均拒绝；失败保护已有输出。
- [ ] 共同最终 IR 无 helper residue；保留 source-import、transformed、final 及双目标完整产物和候选绑定。
- [ ] array_combinators、array_scans、BF16 的原行为与时序 oracle 通过；未运行 nightly 明确记录。
- [ ] 删掉旧 scalar AST 语义与 fallback；更新作者文档，由独立 reviewer 检查 hard break 和剩余调用者。

通过本清单不表示 Method state SSA、admission、resource/conflict/causality 或最终事务
lowering 完成。这些职责继续按[实现分解](implementation-plan.md)分别验收。
