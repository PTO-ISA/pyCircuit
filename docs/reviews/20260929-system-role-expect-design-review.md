# 一等 system、artifact role 与 expect schema 设计审阅

日期：2026-09-29，同日更新为两份修订 B 的审阅结论。用户已选择方案 2：
`ac.system` 成为拥有 symbol 与结构 body 的一等系统定义，不再是只引用 root 的
零 region descriptor。用户同时要求审阅 IR 级 design/testbench role 与 expect
字段；方向选择不等于下表字段已经批准。

**approval-ready 不等于用户批准。** 本记录把"独立设计审阅结论"与"用户批准
状态"分列，不得混写。截至本记录，两份修订都**没有**用户批准：规划
`docs/rfcs/migration/approvals/` 下只有 `c1-pythonic-source.md`、
`c2-c3-foundation.md`、`c2-n1-namespaces.md`、`c2-r1-m1-interface.md` 四份，
不含本两份修订。不要据本页补写或推断批准记录。

设计基线：规划 `b7d0dd3d`，实现 `6696eb3f`（实现侧后续含本批 R2–R4 测试与
注释修正）。本记录不修改产品 ODS、编译器语义或 runtime。

| 提案 | 修订 | 内容 SHA-256 | 独立设计审阅 | 用户批准 |
| --- | --- | --- | --- | --- |
| C2-EXPECT | B | `e8f287e76180c701648dfb2dc8aa08a877ee347da351ebc1fbab4ae24c400b3b` | approval-ready | **未批准** |
| C2-SYSTEM | B | `db81dbc85e54346a0b8f953b6162c7b88d0ed5c5368b1cdbe26a2b4ccda79448` | approval-ready | **未批准** |

两处哈希按当前工作树实测，与用户交接记录给出的修订 B 哈希一致；哈希只作
项目证据与批准绑定候选，不进入设计身份。

## C2-EXPECT 修订 B

[提案](../rfcs/migration/c2-expect-schema.md)。作者：PM。修订 B 修正了修订 A
把简单 source assert 的 direct-bool-SourceRead 限制提升为**所有** `ac.expect`
要求、从而与 range/composition/final 冲突的错误。

确认事项：

- operands 恒为 `condition:i1`、`path:i1`；核心属性为 `kind`、`location`；
  无 results、无 regions；不新增 kind、operand、region 或 Python API。
- `condition` 是该检查的 safety，`path` 是 source evaluation path 与 demanded
  operand validity 的合取；不存在跨所有阶段的 direct-SourceRead 通用限制。
- simple source assert / source range / numeric composition / final 各自按其
  来源验证：simple assert 才要求 direct bool `source.read`，range 来自
  `math.to_bits` 的 validity 或已 lowering 的 `numeric.proof` operands，
  final 阶段 `source.read` 已消除。
- ODS 字段、动态派生属性（`ac.check_id`、`ac.check_template`、
  `ac.required_checks` 等）与 MLIR `loc` 三者分开，不得用"属性恰为"删除既有
  合同内容，也不得混同 loc 与 `location`。
- 检查不产生 SSA、不直接写 Q，但不是可自由删除的 Pure/no-effect marker；
  失败按既有协议阻止当前系统本拍提交。
- helper/展开检查沿用已批准 C2 的 evaluation path、check template 与
  call/iteration expansion 合同；本增补不宣称该部分实现已完成。

非阻断提醒（须继续如实报告）：当前 final verifier 还要求 `location.path` 属于
owning module source owner。这是当前实现限制，不能被本次字段冻结表述成
"跨文件 helper 检查位置已交付"。

## C2-SYSTEM 修订 B

[提案](../rfcs/migration/c2-system-definition-role.md)。§2 明确列出这是**新 op
（`ac.system.import`）+ 已有 op 的结构重定义 + 一个 IR 属性 + 一个 public link
参数 + 一个 manifest 字段**的精确提案，不得称作"没有接口变化"。

确认事项：

- `ac.system` 变为拥有 `Symbol`/`IsolatedFromAbove`、`sym_name`/`domain` 与
  恰好一个 body region 的系统结构定义；owning header 用新的
  `ac.system.import`（无 body/operand/result），与 `ac.module`/`ac.module.import`
  的对应方式一致。
- owning `.interface.ac` 中的定义使用 `ac.declaration_role=definition`；只有
  消费/转存副本才是 `import_snapshot`，副本不成为新 authority。
- 旧 `ac.root_kind` 与新 `ac.system`/旧零 region descriptor **不允许**三条接纳
  路线并存：新路线各阶段禁止 `root_kind`，final 不再插入零 body descriptor。
- `ac.entry` 是唯一入口选择，可解析到普通 portless module 或 system；module top
  不生成假 wrapper、不分配额外 reg、不复制 root invocation。
- 新增 `ac.artifact_role` **只允许存在于最外层 linked/final package**，取值为
  `design`/`testbench`，由 link 显式提供；public `emit` 不新增重复 `--role`，
  `generated.json.artifact_role` 必须等于 final IR 中的用途。
- `generated.json.files[].role` 的既有枚举
  `header/source/cmake/rtl/runtime-glue/source-map` 完全不变，不混入
  design/testbench。
- **role 与 root 种类正交**：`system` 不等同 `testbench`，`module` 不等同
  `design`；两者都可用于两种用途，§5 要求删除"system 只能 testbench /
  module 只能 design"的规则。role 是显式声明的用途，不是编译器证明"没有
  stimulus"的证书，不能从文件名、symbol 名、assert/log/report 或端口推断。
- system 初期仍 root-only（`ac.instance` callee 只能为 module）；不借机开放
  系统嵌套、外部 typed DUT、多时钟或新调度模式。
- staging 与其配对 IR 一起验证；旧产物恢复必须自包含，允许同 owner 合法跨
  role replacement，不能把共享 validator 绑成"必须等于本次新 role"。

## 与当前私有实现的已知差异（待批准后处理，本轮不改）

当前私有 `acir-design-harness --role` 把 `@system` root **强制等同** `testbench`
（`@module` root 不得标 `testbench`）。C2-SYSTEM 修订 B §5/§7 明确要求删除这条
私有规则。该差异属于 §2 的 public link `--role`、IR `ac.artifact_role` 与
manifest 字段变更，**必须等精确批准后实施**；本轮不修改它，也不据本页把它标成
已批准或已交付。修订 B 允许私有测试 helper 暂时保留 `--role` 作为**一致性断言**，
但只能是比较相等，不得推断用途，也不得覆盖 IR。
