# 硬件 design / testbench 边界与 ACIR 授权审计

日期：2026-09-29。结论：M2 的封闭系统验证仍有效；M4 独立 design/testbench
交付尚未成立，暂停进一步公开入口与 IR 扩展。用户最新要求硬件设计产物
使用 `design_top.ac`，不能把自测试 system 当成独立 DUT。

审计对象：实现分支 `codex/gfsim-source-units` 的 `6b514f90` 与未提交的
M4 文件桥；规划分支 revision 8。本报告不是新增 IR 的批准记录。
独立审计由 native `n0_c1_review` 和 `ir_approval_audit` 完成；PM 整合。

## 1. 实际发现

1. `FinalProgram` 是编译器内部已验证链接闭包的 C++ 容器；当前没有
   `ac.program` primitive，也没有把规则变成任意软件程序的运行模型。
   寄存器、实例树、并发 rule、Work/Xfer 提交的硬件语义仍然存在。
2. 但 V41/V42 编译的 root 是 `TestIncrement` / `TestPipeline`，带有
   stimulus、phase、检查和 completed report。选它作为 root 后，这些
   内容也进入 `FinalModel`。因此它证明的是封闭测试系统，不是独立 DUT
   design。仅改文件名不能完成边界分离。
3. `ac.system` 只有所选实例树 entry/owner/origin/domain，没有独立的
   design/testbench role。不能从目前的验证结果推导出已实现这种区分。
4. `FinalEmitVerilog.cpp:336` 输出硬件 `FinalModel`，`:396` 继续输出
   simulation observation wrapper `FinalModelSim`，当前在同一文本中。
   clock loop 位于外部测试 harness，不在硬件模块里。公开 bundle 尚需
   按已批准文件角色拆分 RTL 与 simulation/runtime glue。这是产物角色
   混放；没有证据显示 wrapper 反馈或改变 FinalModel 的硬件语义。真正
   的语义混合来自选中的 root 本身含有测试 stimulus/check/report。
5. `ComposedFixture.cpp` 的模型是私有回归夹具，未进入安装/export，
   compiler library 没有内嵌 TestIncrement。未发现 consumer CPU/NPU
   设计进入本轮 compiler/runtime 的证据。generic fixture 可保留在
   tests/private harness，但不能变成标准编译入口必须知道的模型。

证据文件（实现分支）：`compiler/acir/lib/Compiler/FinalProgram.h`、
`FinalHardware.cpp:382`、`FinalEmitVerilog.cpp:336` / `:396`、
`FinalEmitCppSystem.cpp:67`、
`compiler/acir/tools/acir-backend-closure-harness/ComposedFixture.cpp:49`。

## 2. 应保持的三层边界

| 层 | 负责什么 | 不应混入什么 |
| --- | --- | --- |
| Hardware design | module/reg/rule/实例和连接、设计自身必要约束；产物 `design_top.ac` | 测试 stimulus、测试完成计数、宿主运行循环 |
| Testbench | 实例化 DUT、提供 stimulus、比较期望、控制测试完成；独立测试产物 | 修改 DUT 的寄存器拥有权或依赖 compiler 内置特定模型 |
| Framework/runtime | 通用 capture、MLIR passes/verifiers、codegen、GFSIM 调度和观察接口 | 特定 DUT 的算法、固定 fixture 名字、硬编码期望轨迹 |

设计自身的 assert/检查仍可合法存在于设计；不能把所有 `ac.expect`
一律当作 testbench 操作删除。Work/Xfer 的硬件提交约束也不会因分层而改变。

## 3. 新增 primitive 的实际数量及审批来源

范围：相对最近实现基线 `82f161ea` 到已推送 `6b514f90` 的 ACIR ODS。
新增 **9 个 op**，另有 **1 个 op/type 替换**；本轮 M4 dirty patch 没有
新增或修改 ODS op/type。`ac.*` 中既有硬件构造，也有编译期来源和证明
载体，不能都称为用户要手写的硬件 primitive。

| 新增/变更对象 | 用途 | 用户批准依据 |
| --- | --- | --- |
| `ac.reg` / `!ac.reg<T>` 替换 dffe | clock/reset/state handle，统一物理状态 | R1-B:88–149；联合批准 |
| `ac.system` | 选择最终硬件实例树 root；不包含 scheduler 脚本 | M1-C:235–255；联合批准 |
| `ac.observe` | print/log/report 的 source observation | M1-C:507–543；联合批准 |
| `ac.source.read`, `ac.source.use` | source 读取与赋值来源；后续 lowering | M1-C:407 明确纳入 A2-B/L1-B 并重基为 lexical/reg；非另一次独立批准 |
| `ac.value.binding`, `ac.value.use`, `ac.numeric.proof` | 有限 SSA 与源数学/使用的编译期证明 | C2-C:228–286；C2/C3 基础批准 |
| `ac.math.compare` | source 数学比较及 validity | C2-C:204；C2/C3 基础批准 |
| `ac.expect` | assert/安全检查及 source path | C2-C:220–224 有语义与部分字段依据；**当前完整 ODS 的逐字段批准证据不足** |
| `ac.rule`, `ac.instance` 的修改 | 单 rule body、reg targets、实例 clock/reset 前缀 | R1-B:116–149；联合批准 |

`ac.module` 在上述区间的 ODS 声明没有实质修改；其控制前缀和动态属性
合同需一起看 R1/M1/verifier，不能只按 TableGen diff 统计。

`ac.expect` 不能标成“从未讨论”：condition/path、check kind、location、
CheckID 与失败语义都有批准依据。但本次没有定位到完整
`expect(condition:i1,path:i1){kind:StringAttr,location:DictionaryAttr}`
的逐字段冻结声明。按 AGENTS，后续任何该 schema 扩展先补审批映射，
不能用子代理 PASS 或已存在实现代替用户批准。当前功能覆盖也比完整
合同窄，未实现 helper 等能力不能冒称已交付。

完整当前 ODS inventory（24 ops）：
`type_alias`, `constant`, `math.constant`, `value.binding`, `value.use`,
`system`, `numeric.proof`, `math.from_bits`, `math.compare`, `math.binary`,
`math.to_bits`, `module.import`, `reg`, `module`, `rule`, `source.read`,
`source.use`, `observe`, `expect`, `instance`, `yield`, `struct`,
`struct.create`, `struct.get`。统一前缀为 `ac.`。
当前三种 types 为 `!ac.math_int`、`!ac.struct<name>`、`!ac.reg<T>`。
这个完整清单不等于都在最近一轮新增。

## 4. 用户批准与 AGENTS 的要求

实际批准记录：

- [C2/C3 基础批准](../rfcs/migration/approvals/c2-c3-foundation.md)：
  记录用户回复“批准”，精确绑定 C2-C/C3-C。
- [R1/M1 联合批准](../rfcs/migration/approvals/c2-r1-m1-interface.md)：
  记录用户回复“好的，按照这个计划来实现……”，精确绑定 R1-B/M1-C。

本次实算正文 SHA-256 与记录一致：

- C2-C：`387cf52b129f864b87a1b2a388213a3fe36d81c330d94ced0e6696522b58a322`
- C3-C：`0c476ced27519cf93427a89f77b9348d388e96db57fb183790144d24118b1170`
- R1-B：`76283661bffba96aad544e31d2b4996f019135f448c4c8b685ebb02eab8edfba`
- M1-C：`84b551ea84d6aa0956ea2342f520f4aafc63f7d741cc651500446a26f49c35cd`

这证明存在整包批准记录，**不意味着用户曾逐 op 单独签字，也不意味着
实现每个字段必然与批准一致**。A2/L1 页眉的历史 pending 与 M1 的明确
引用纳入要区分；不能把引用纳入夸大为独立批准。

[AGENTS.md](../../AGENTS.md) Modernization project management 明确规定：
“Every Python, CLI, IR, cross-module, generated C++, runtime, schema,
diagnostic, timing, ownership, or error-contract change requires the user's
precise approval before implementation.” 同时要求独立设计/验证、实现/测试、
作者/reviewer，要求先在 MLIR verifier/pass 固化语义，并禁止 consumer
设计/testbench/专用 adapter 进入框架。PM 的排程权不等于设计批准权。

## 5. 本次处理与后续边界

- 根据用户最新明确指令，当前设计产物称为 `design_top.ac`；私有桥改名
  `acir-design-harness`，读取参数为 `--design`，不保留旧私有 alias。
- 当前封闭测试系统的回归文件使用 `closed_system_testbench.ac`，不借
  `design_top.ac` 的名字把测试刺激伪装成独立 DUT。
- 历史批准稿/哈希不篡改。`FinalProgram` 暂留内部容器名，不做机械全仓
  重命名，也不引入 `ac.program` / `ac.design` / `ac.testbench`。
- M4 只保留内部文件桥与逻辑类型重解析修复；设计/testbench 公开交付
  尚未验收。若后续要声称公开 design/testbench 交付，必须先分别给出
  独立 design、外部 testbench 的产物/依赖图和测试证据；涉及新 role、
  op、端口或 runtime 协议时另行批准。本报告不授权新的 testbench 接口。
- 对任何需要新 role 属性、op、端口或 runtime 协议的方案，先列出精确
  before/after 与现有批准映射，独立审阅后由用户批准；本报告不授权它们。

## 修订注记（同日，用户追加命名指令）

用户追加指令：「ac应该是和python的文件名一致」。本报告 §1/§2/§5 中把
`design_top.ac` 当作当前设计产物固定名的表述按以下规则订正：`.ac` 产物按来源
Python 文件名命名（`<stem>.ac`），不设保留标签；`design_top.ac` 只是
`design_top.py` 这一 root 源文件的产物，`test_increment.py` 产出
`test_increment.ac`。C3-C 文本中的 `-o <program.ac>` 需按此规则修订，公开
driver 不得硬编码旧名或保留标签。

§3 的 primitive 授权清单与 §4 的批准映射不受本注记影响；本注记不新增任何
op/role/端口/协议授权。产品侧已按该规则更新：私有桥 `--output` 仍由调用方给出、
不自行发明名称，系统测试与证据中的链接产物名改为跟随源文件名
（`test_increment.ac`）。

## 修复记录（2026-09-29 后续处理）

本记录的发现按下列状态逐条处理；本节只记状态与证据，不改写上面的原始发现与
授权边界。

| 发现 | 状态 | 处理与证据 |
| --- | --- | --- |
| §1.4 `FinalEmitVerilog` 把硬件 `FinalModel` 与 simulation wrapper `FinalModelSim` 输出在同一份文本 | **已修复** | 按 C3-C 已批准的 `rtl` / `runtime-glue` 角色拆分：`FinalVerilogEmission{rtl,runtimeGlue}` + `emitFinalVerilogParts`，`emitFinalVerilog` 保持字节不变；私有 harness `--glue-output` 把两个 role 写成两份文件，双目标先校验后创建、第二个失败回滚第一个。产品 commit `fcdb75e6`，复审 PASS + 溯源修正 `0d2e3ab6`，证据 `docs/gates/logs/20260929-m4-role-split/` |
| §1.2 / §5 把"封闭测试系统跑通"当成"独立 design 可交付"；仅改名不算分离 | **已修复（工具层强制）** | ① 加 M4-D2 边界测试：DUT-only closure 单独 link 成设计产物（无 `ac.observe`/stimulus），system testbench 为独立产物，设计 `rtl` 无 `FinalModelSim`/`AC_OBS`，证据 `docs/gates/logs/20260929-m4-artifact-boundary/`；② 私有桥新增**显式 role 契约**：`--role design\|testbench`，`@system` root（`ac.root_kind = "system"`）必须是 `testbench`、否则 link/emit 都拒绝，`@module` root 不得标 `testbench`；emit 时按产物内的 `ac.root_kind` **再校验一次**。测试 `test_system_root_must_be_declared_as_a_testbench`、`test_module_root_rejects_the_testbench_role`、`test_emit_rechecks_the_role_against_the_artifact` |
| §5 命名：设计产物名 | **已按用户指示修订** | 用户指令「ac应该是和python的文件名一致」，`.ac` 按来源 Python 文件名命名；记为用户指示修订 `docs/rfcs/migration/c3-artifact-naming-amendment.md`，C3-C 冻结正文不改 |
| §1.5 私有回归夹具不得进入安装/导出面，也不得成为标准编译入口必须知道的模型 | **已核验并加回归守卫** | 三个私有 harness 的 `CMakeLists.txt` 均无 `install(`；`compiler/acir/lib/` 不引用 `TestIncrement`/`TestPipeline`/`ComposedFixture`。新增守卫测试 `test_private_regression_fixtures_do_not_leak_into_shipped_surfaces` 固定这两条 |
| §1.3 `ac.system` 没有独立的 design/testbench role | **仍开放（需批准）** | 加 IR role 属性属新 op/role 接口，按本报告 §5 与 AGENTS 必须另行批准。当前不改 IR，改由工具层用既有 `ac.root_kind` 强制区分；若最终要在 IR/生成产物里落 role，需独立提案 |
| §3 `ac.expect` 完整 ODS 字段缺逐字段批准映射 | **已提交审批请求** | `docs/rfcs/migration/c2-expect-schema.md`：现有 op 形状、逐字段 → C2-C 条款映射、已实现不变量与 9 项反例、拟逐字冻结文本、明确不覆盖范围；待用户决定 |

本轮（工具层 role 契约 + 夹具泄漏守卫）的 lane 结果：80 Python system
selector 通过、2 项 V44 deselected，0 failed / 0 skipped；native lane 未受影响
（无编译器库源码变更，且无 native target 链接该私有工具）。
