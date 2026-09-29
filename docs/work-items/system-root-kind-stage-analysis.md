# S1 分析：`ac.root_kind` 与 `ac.system` 的阶段边界

状态：分析记录，**不是提案、不是批准**。回答"为什么不 lower 到 `ac.system`，而要
保留 `ac.root_kind = "system"`"，并给出最小建议。精确变更以
[C2-SYSTEM 修订 B](../rfcs/migration/c2-system-definition-role.md) 为准；本页不改
ODS、IR 语义或实现。日期：2026-09-29。

## 1. 各阶段谁承载什么（实测，绑定当前候选）

| 阶段 | system 定义承载者 | 入口选择 | root-only 约束 | 证据 |
| --- | --- | --- | --- | --- |
| capture | Python AST 的真实 `@module` / `@system` 种类 | 无 | 无 | `_source_capture.py` |
| source body | `@system` → `ac.module` **加 `ac.root_kind="system"`** | 无 | 无 | `PythonImportModuleBody.cpp:94` |
| source header | `ac.module.import` **加同一标记** | 无 | 无 | `PythonImportModules.cpp:216` |
| linked | 同上（标记仍在 module 上） | `ModuleGraph` 用标记挑候选 system root；无标记时取唯一 zero-incoming | 标记 root 不得作 child callee | `ModuleGraph.cpp:125–126`、`:271`；`PythonImportModuleAnalysis.cpp:560` |
| final | package 上 `ac.stage=final`/`ac.entry`/`ac.instance_bindings`；body 里再插入**零 region** `ac.system(entry, domain, source_owner, origin)`；module 上的 `ac.root_kind` **残留** | `ac.entry` + `ac.system.entry` | 由 canonical 检查与实例树保证 | `FinalHardware.cpp:388–400` |
| reparse / emit | 同上 | `buildFinalProgramFromHardware` 重建 `ModuleGraph`，若标记仍在则再次用它挑 root | 同 linked | `FinalHardwareProgram.cpp`、`ModuleGraph.cpp` |

`ac.system` 现有 ODS 是 `(entry: DictionaryAttr, domain: StrAttr)`，无 symbol、无
region（`ACIROps.td:48`）：它是**入口描述符**，不是系统定义。

## 2. final 里的两个"入口决策者"是否互相独立（实测）

final 同时存在 package 的 `ac.entry` 与 `ac.system.entry`。实测两者**不是**独立
权威，`ACIRHardwareClosure.cpp:99–109` 逐项核对：`ac.system` 无 operand/result/region、
恰好 4 个属性、`systemEntry == closure.entry`、owner/origin/domain 合法，否则
`ac.system descriptor is not canonical`。我把 `ac.system.entry` 改指另一个 module、
或只改 package 的 `ac.entry`，两种改动都在 emit 前被该检查拒绝（rc=1，不产出文件）。

被保留的 `ac.root_kind` 在 final 中还起不起作用？实测：**不起独立作用**。把
final 里的 `ac.root_kind = "system"` 删掉后，emit 仍然成功（rc=0）——因为该
system root 同时是唯一 zero-incoming 定义，`ModuleGraph` 的回退规则选到同一
root。也就是说 final 的标记是**阶段残留**，它既不是第二权威，也不影响结果；
但它确实还在，且与 `ac.system` 并存，容易被误读成"两个决定入口的地方"。

## 3. 为什么今天不是"直接 lower 到 ac.system"

因为要区分两件事：

1. **阶段清理**：source/header 用 `ac.module` + 标记表达"这是 system"、final 再插
   零 region 描述符，这套表示是 M1-C:235–255 明确设计的原合同，不是执行 agent
   私自加的属性。把 final 的残留标记清掉、或让 source 直接用 `ac.system`，
   都属于改 final/source IR 形状，影响现有 verifier、snapshot 比较与两个 emitter，
   必须按精确合同走。
2. **结构重定义**：方案 2 要求 `ac.system` 本身变成带 symbol 与 body 的一等定义，
   并新增 `ac.system.import`。这**不是**把名字替换过去就行：现有零-region schema、
   `ac.module` 的 symbol/header/link/verifier 假设、以及所有
   `getParentOfType<ac::ModuleOp>` 使用点都要一起改。

## 4. 两种方案（供精确提案对照，本页不选也不实现）

- **方案 A（保持描述符）**：`ac.system` 继续是 final 入口描述符；source/header 的
  标记在 linked→final 边界转成这一唯一表示并清除残留。改动面小，但 Python 侧
  "system" 始终只是 module 上的标记，逐源 header 无法用 op 种类表达定义种类。
- **方案 B（一等定义，用户已选方向）**：`@system` → `ac.system`（Symbol +
  IsolatedFromAbove + body），header 用 `ac.system.import`，新路线各阶段禁止
  `ac.root_kind`，final 不再插入零-region 描述符。改动面覆盖 ODS、source/header、
  ModuleGraph、final 物化、reparse、verifier 与两个 emitter，即
  [C2-SYSTEM 修订 B](../rfcs/migration/c2-system-definition-role.md) §7 的替换清单。

用户已选择方案 B 的**方向**；该页 §2–7 列出的新 import op、schema、
`ac.artifact_role`、public link `--role`、manifest 字段与旧形式退役规则**仍待精确
批准**（`docs/rfcs/migration/approvals/` 下无对应记录）。

## 5. 必须保持的边界

- **system ≠ testbench**：`system` 是"系统组合"这一结构概念，design 与 testbench
  都可以有系统组合。不得再用 `root_kind` 或 `ac.system` 的名字冒充已定义的
  DUT/TB role（当前私有桥的 `@system ⟹ testbench` 规则正是方案 B §5 要求删除的
  私有规则；删除需精确批准，本轮未改）。
- 普通 portless `@module` 作为 top 仍必须保留；不生成假 system wrapper、不加 reg、
  不复制 root invocation。
- 本分析不新增 region、op、testbench ABI，也不解除 system root-only 限制。
