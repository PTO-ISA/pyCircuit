# C2-EXPECT：ac.expect 逐字段冻结提案

修订：B，2026-09-29。状态：修正旧稿后待独立复审、待用户精确批准。
用户要求审查此审批请求，不等于已经批准本修订。冻结的 C2-C 正文和历史
批准哈希保持不变；本页是补充合同候选，不修改产品实现。

修订 A 的问题：把简单 source assert 的 direct-bool-SourceRead 限制提升为
所有 ac.expect 的要求，与 range/composition/final 检查冲突；“无运行效果”
也容易被误读为允许删除检查。修订 B 明确字段、阶段和提交语义三者的区别。

## 1. 不变的操作形状

```text
ac.expect(condition: i1, path: i1)
  core attributes:
    kind: StringAttr
    location: DictionaryAttr<SourceSpan>
  results: none
  regions: none
```

以上是 ODS 核心字段，不是全部阶段元数据的排他清单。操作仍名为
`ac.expect`，不新增 operand/result/region、kind 或属性名，不新增 Python API。

| 字段 | 拟冻结含义 |
| --- | --- |
| condition | 该检查的 safety 条件；由对应阶段的真实 SSA 推导与核对，不能靠同类型任意值替代 |
| path | C2 的 source evaluation path 与 demanded operand validity 的合取，不能用 condition 自我屏蔽失败 |
| kind | C2 既有闭合枚举 assert/division/shift/index/range；既有枚举不代表本候选已实现所有 profile |
| location | C2 SourceSpan 的闭合 DictionaryAttr；保存真实检查来源，按当前阶段的 owner/origin/展开关系验证 |

MLIR `loc` 和 `location` 属性承担不同的工具定位/合同来源责任，不得混同或
仅凭二者打印文本一致就认定 source identity 有效。

## 2. 检查语义

`ac.expect` 不产生 SSA 结果，不直接驱动任何寄存器 proposal，也不直接写入 Q。
但它是必须保留的检查义务，不能被当作无用的 Pure/no-effect marker 删除。

| path | condition | 该检查结果 |
| --- | --- | --- |
| 0 | 任意合法 i1 | 本次不触发失败 |
| 1 | 1 | 本次检查通过 |
| 1 | 0 | 按既有 C2/R1/M1 失败协议记录检查失败，阻止当前系统本拍提交 |

unsafe producer 的求值仍受 source path/safety 保护；不能通过在错误操作
已经执行之后添加 expect 来实现安全性。求值顺序、validity 传播、错误身份排序、
Work/precommit/Xfer/reset 继续遵循已批准合同，本增补不重新定义它们。
多个 rule 的检查可以独立收集，但失败不能只屏蔽一个模块而让其余模块半提交。
观察/日志关闭、普通 DCE、codegen 文件角色拆分均不得删除仍需保留的检查。
设计本身与 testbench 都可包含 expect；op 的存在不能用于推断 design/testbench role。

## 3. 分阶段来源与证明

| 阶段/当前 profile | condition/path 的验证来源 | 不得误推的通用规则 |
| --- | --- | --- |
| source 简单非 composition assert | 当前实现核对 direct bool source.read 与允许的 source path | 这是当前 profile 的实现约束，不是所有 expect 必须 source.read |
| source range | 对应 math.to_bits 的 validity/path，或已 lowering 的 numeric.proof check operands | 不要求来自 bool 寄存器读取 |
| numeric composition | 对应已批准 RequiredCheck/数值义务与实际 SSA 的 composition closure | 不能套用简单 assert 的语法形状 |
| final 简单 assert | 已验证有限 bool SSA 与路径；本阶段 source.read 已消除 | 禁止为满足旧稿强行保留 source.read |
| final range | 对应 numeric.proof 的确切 condition/path operands 与 CheckID | 不能用另一个等类型 SSA 替代 |
| helper/展开检查 | 沿用已批准 C2 的 evaluation path、check template 与 call/iteration expansion 合同 | 本增补不宣称这部分实现已完成，也不撤销其原有批准 |

当前代码证据：`ACIRModuleOps.cpp:768–811` 分流 final/composition/range/assert；
该文件的 verifyRangeSourceExpect 核对 source to_bits 或 lowered proof；
`ACIRFinalContracts.cpp:507–535` 与 `ACIRNumericComposition.cpp` 验证 final/composition。
行号仅帮助定位，验收必须绑定实际候选，不把行号当合同。

## 4. 既有阶段元数据继续有效

本节明确已有 C2 内容，不新造 identity 或 metadata：

- helper 模板阶段：操作的 `ac.check_template`、函数的 `ac.check_templates`
  及 C2 已定义的 evaluation-path 表示继续依原合同使用。
- 已注册 rule scope：`ac.check_id` 是闭合 CheckID，包含 registration、check
  occurrence 与跨 check kinds 统一的 obligation slot；不得靠源行号或打印字符串造 identity。
- owning rule 的 `ac.required_checks` 恰覆盖当前阶段要求保留的检查。
  id/kind/location 与实际 expect 对应；检查不能丢失、重复或借用其他 rule 的证明。
- numeric proof 的 checks 只覆盖该 witness 所拥有的检查，operands 与对应
  expect 的 condition/path 逐项绑定。

SourceSpan 必须符合 C2 来源规则。当前直接 source rule 的实现要求 location.path
等于 owning source path；不能将这个当前限制扩展成“所有跨文件 helper 的检查
位置必须改写为 caller 文件”。helper 展开仍需保留已批准的实际来源和展开身份。

未知或当前阶段不合法的字段按既有闭合 schema/verifier 拒绝；本提案不是任意
新增 `ac.*` 属性的通行证。具体必需/可选元数据以对应已批准阶段合同为准。

## 5. 建议批准的精确内容

批准对象为修订 B 的 §1–4：冻结两 operand、两核心属性及无 results/regions，
明确 safety/path 与失败不提交语义，保留 C2 既有阶段元数据和分阶段证明。
不冻结旧稿的通用 direct-SourceRead 限制；不把 op 定义成可自由删除的纯观察。

该批准不增加新 kind、新 Python 接口、新 runtime 状态/错误码、新时钟域或
外部 DUT ABI；不批准 emitter 绕开共同 IR；不把未支持的 helper/数值形状标为完成。
首次实现一个已批准 profile 仍需独立测试与 review，但不为同一语义重复制造批准。

## 6. 验证与实施边界

本次文档修订不授权改 ODS 或 verifier。批准后先做当前实现与本文的 conformance
核对；若只需纠正文档，不为“冻结”而改代码。确有差异则按问题单修复。

必要正例：简单 bool assert、source range、composition assert、final range、
final bool assert；分别证明 path=0、path=1/condition=1 和失败不提交。
必要反例：condition/path 重定向、跨 rule CheckID、丢失/重复检查、错误
kind/location、final 残留 source.read、numeric check operand 不匹配。
两后端应验证同一 IR 的正常行为与既有失败协议；不得仅检查 emitted 文本含 marker。

复用 `ACIRCheckContractsTests`、`ACIRFinalProgramTests`、numeric composition
suite 和 V41/V42/V43 相关已存在 selector；只有覆盖缺口才新增测试，不强制跑
全平台/完整发布矩阵。候选、命令、原始结果、未执行 profile 均需记录。

## 7. 权威与审阅

依据：C2-C 的错误/数值证明/使用义务章节，R1/M1 的失败不提交与 source-use
重基；批准记录见 `approvals/c2-c3-foundation.md` 与
`approvals/c2-r1-m1-interface.md`。本页在用户批准前仍是候选，独立 reviewer
只能判 approval-ready/revise，不能代替用户批准。
