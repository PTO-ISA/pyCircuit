# C2-EXPECT：`ac.expect` ODS schema 逐字段审批映射

状态：**approval request**（待用户精确批准）。本页**不改变任何现有 schema 或
行为**，只把已实现的 op 形状、C2-C 条款与拟冻结文本逐条对齐，供用户确认或修改。
来源：[边界与 primitive 授权审计](../../reviews/20260929-design-testbench-ir-authority.md)
§3 记录 `ac.expect` 的完整 ODS 逐字段批准证据不足；批准前的实现、测试与其他
工作不受本页影响。

## 1. 现有已实现 schema（事实）

`compiler/acir/include/acir/Dialect/ACIR/ACIROps.td:150-156`

```tablegen
def ACIR_SourceExpectOp : ACIR_Op<"expect"> {
  let summary = "Record one source assertion check without runtime side effects";
  let arguments = (ins I1:$condition, I1:$path, StrAttr:$kind,
      DictionaryAttr:$location);
  let hasVerifier = 1;
}
```

无结果、无运行效果。下列为**下游派生属性**，不是本 op 的 ODS 字段：
`ac.check_id`（每个 expect 的闭合 CheckID）、owning rule 上的
`ac.required_checks:Array<RequiredCheck>`。

## 2. 逐字段 → C2-C 条款映射

| 字段 | 语义 | C2-C 依据 |
| --- | --- | --- |
| `condition: i1` | 该检查的 safety；必须是指向 direct bool SourceRead 的精确条件，不得由 path 重定向 | 第 224 行「runtime ac.expect 的 condition 是 safety」及 SSA live 求值路径 |
| `path: i1` | source path ∧ demanded operand validity；unsafe producer 受 `path && safety` 支配 | 第 224 行同上 |
| `kind: StringAttr` | `assert/division/shift/index/range`；与 check template 和 numeric binding 一致 | 第 220 行 kind 枚举；第 222 行「kind 与模板/numeric binding 一致」 |
| `location: DictionaryAttr` | 该检查的源位置，必须归属其 module source owner | 第 220 行 `check_template` 的 `location`；第 222 行 RequiredCheck 的 `location:SourceSpan` |
| （派生）`ac.check_id` | `{registration:Occurrence, check:Occurrence, obligation:u64}`，obligation 跨 kind 统一分配 | 第 220 行 CheckID 定义 |
| （派生）`ac.required_checks` | owning rule 上每项恰有 `{id,kind,location}`；ID 唯一且恰对应一个 final expect | 第 222 行 |

（行号指 `docs/rfcs/migration/c2-mlir-contract.md` 当前冻结修订 C。）

## 3. 已实现并被测试锁定的不变量

| 不变量 | 实现位置 |
| --- | --- |
| `condition` 必须是指向 direct bool SourceRead 的精确条件 | `compiler/acir/lib/Dialect/ACIR/ACIRModuleOps.cpp:808` |
| `path` 不能重定向 condition | 同文件 `:447` |
| 检查身份必须在所属 rule scope 内 | 同文件 `:789` |
| range check 恰需一个 source `to_bits` 或 lowered proof | 同文件 `:547` |
| `location` 必须归属其 module source owner | 同文件 `:799` |
| rule 的 `required_checks` 必须与实际 source checks 匹配 | `ACIRFinalContracts.cpp:381`、`:471` |

对应的独立反例位于 `tests/cpp/agentic-circuit/Dialect/ACIR/CheckContractsTest.cpp`
（9 项，含 condition/path 重定向、scope、range、location 与 required 漂移）。

## 4. 拟冻结文本（C2-C 增补，逐字）

建议在 C2-C「错误、值与数值证明义务」一节追加以下段落；不改动该节已有文字：

> `ac.expect` 无结果、无运行效果，operands 恰为 `condition:i1`（该检查的
> safety，必须是指向 direct bool SourceRead 的精确条件，不得由 `path` 重定向）
> 与 `path:i1`（source path ∧ demanded operand validity）；属性恰为
> `kind:StringAttr`（`assert/division/shift/index/range`，与 check template 及
> numeric binding 一致）与 `location:DictionaryAttr`（归属其 module source
> owner 的 SourceSpan）。下游派生的 `ac.check_id` 与 owning rule 的
> `ac.required_checks` 按本节前述定义；本 op 不新增其他 operand 或属性。

## 5. 本批准**不**覆盖

- 不新增 kind、operand、属性或 helper 模板能力；
- 不批准 helper 内 `ac.expect` 的 `evaluation_path` / `ac.check_template` 完整实现；
- 不批准 conditional observe、numeric proof materialization 或 sink I/O；
- 不修改 C2-C/C3-C 已冻结正文，只在批准后追加第 4 节逐字段段落；
- 不改变 `ac.expect` 的任何运行时语义或两个 backend 的行为。

## 6. 待用户决定

1. **按第 4 节逐字冻结（推荐）**：把现有形状升级为逐字段批准，后续扩展有明确基线。
2. 先修改字段或语义，再批准。
3. 暂不冻结：保持现状——已实现且被测试锁定，但不构成逐字段批准，
   后续任何 `ac.expect` schema 扩展须先补齐本映射。

在得到决定前，不实施任何 `ac.expect` schema 扩展。
