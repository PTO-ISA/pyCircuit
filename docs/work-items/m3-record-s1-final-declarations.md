# M3-E01 S1：final record 声明与来源闭合

状态：done for the bounded S1 declaration slice。日期：2026-10-01。输入候选：
`1efec35eaa19eab2315797340904def88b24a640`，派发前工作树干净。
授权：[C2-DECL-R B](../rfcs/migration/approvals/c2-decl-record-final.md)，
提案 SHA-256 `ea242da0d85de4f51c439051c80c2e7ce12c17dca5ef9a63b8743f0f280a0043`。
沿用 Decision 0283 当前 scalar profile；本切片不宣布 record 产品支持。

## 目标与边界

完成两字段 flat finite record 的 source-owned final 声明投影、canonical
constructor provenance、完整声明 inventory、shared envelope/reader 验证及
EmitReady 冻结。constructor 在 source/header 必须有经验证的函数，在 final
仅为 canonical 来源引用；final 不保留函数或伪造 stub。

S2 的 helper/value/use/required_records、S3 的 reset/state/commit 和 S4 的
source-owned record C++/RTL 仍未交付。S1 保持 record state 拒绝，并在共用
emit guard 拒绝含 record 声明的生成，避免缺失 record header 的成功产物。
不改 ODS、importer、runtime、公开 CLI、receipt、ABI 或语言支持范围。

独立分解发现：已有 source helper verifier 仅保证 constructor 的类型和
constructed return，并不证明字段直接来自参数或 valid 原样传递。
S1 在 registry 验证之后增加 link erasure-safety 校验：正文恰 create/return，
字段来自 entry 的 data arguments 且 logical constraint 与字段相等，返回
valid 恰为最后的 evaluation_path 参数。允许字段/参数顺序不同、复用参数。
不更改通用 source helper validator；较宽的 source record 编译能力保持。

## 独占文件与独立实例

- 分解：`/root/m3_s1_decomposition`，Sol high，只读。
- 实现：`/root/m3_s1_implementation`，Luna high，仅
  `ACIRFinalDeclarations.h/.cpp`、`FinalDeclarations.cpp`、`FinalHardware.cpp`、
  `ACIRHardwareClosure.cpp`、`FinalProgram.cpp`、`FinalEmit.cpp`。
  前缀分别为 `compiler/acir/lib/Dialect/ACIR/` 或 `compiler/acir/lib/Compiler/`。
- 独立测试：`/root/m3_s1_independent_tests`，不同 Luna high，仅新
  `tests/system/test_final_record_declarations.py` 和
  `tests/cpp/agentic-circuit/Dialect/ACIR/FinalRecordDeclarationsTest.cpp`。
- PM 独占 CMake source-list、文档、候选冻结、构建/安装与最终集成。
- 最终审阅：`/root/m6_resource_independent_review`，独立 Sol high，APPROVE；
  `/root/m3_m6_plan_validation`，独立 Astra high，CONFORMANT。共享 checkout 中不得撤销
  其他实例的修改；子实例不能递归组队或自行提交。

## Checklist 与验证

- [x] 正例：独立 provider、header-only scalar consumer、unused/private records、
  facade/empty units、mixed widths、完整 inventory 与确定排序。
- [x] 反例：owner/constructor/fields/origin/location/expansion/unknown attrs、
  source body/header mismatch、unsupported supplied record/helper，不靠 DCE 接纳。
- [x] final 保存后新进程原生验证，无 source/header 依赖、无 executable helper。
- [x] 合法 standalone edit 与 frozen-object 篡改的界限明确，不声明历史源认证。
- [x] S1 不接纳 record state；CPP/RTL 共用明确拒绝且不生成不完整产物。
- [x] 独立 failing-first、候选测试、scalar/source 回归、Sol review 留档。
- [x] PM 关闭本切片并更新后续 S2 依赖，完整 E01 仍为 open。

构建由当前 checkout 在 `.pycircuit_out/m3-s1-build` 完成，安装到
`.pycircuit_out/m3-s1-install`，LLVM/MLIR 22.1.8，testing/runtime/compiler dev ON。
窄目标是三个 source/design/CPP-parts harness 与 `ACIRFinalProgramTests`、
`ACIRSourceUnitTests`；baseline 两个 CTest binary 已通过。
独立 system 用明确的三个 `ACIR_*_HARNESS` 路径运行 record/scalar/source
namespace/unit fixtures；精确命令、候选文件 SHA-256 和原始结果在验收时归档。
实现开始前先运行新增 system 正例失败基线；运行候选 gate 时冻结 source/test。
任何超出 B 的接口分支上报 PM，只冻结新增范围，不阻止其余已授权工作。

## 验收与明确缺口

十文件 code/test/CMake aggregate：
`3405b937fb6ddce11a21214c6ec3722a73a15455087a33101705ea72a55e1159`。
[独立代码审阅](../reviews/20261001-m3-record-s1-review.md)与
[原始证据/复现命令](../gates/logs/20261001-m3-record-s1/README.md)绑定该候选。
10 system、4 record native、19 CTest binaries/268 native cases、119 安装工具
回归和 12 条安装 CLI smoke 命令通过；测试无 fail/error/skip/disabled。
独立 reviewer 重跑 system 10、record native 4、FinalProgram 58 通过。
最终同一测试文件在新建 pristine baseline 工具上是 6 fail/4 pass，缺失阶段和
两个新增 constructor 准入诊断均可重现。

不声称完整 20-binary 原生 lane 通过：`ACIRSourceMathContractsTests` 的三个
NumericComposition/APInt assertion 在精确 `1efec35e` fresh archive build 与
候选中相同；LLVM/MLIR 22.1.8 和构建选项逐项相等。baseline 91 cases 中
88 pass/3 fail，三项精确 filter 在两者都是 3 fail。此数值验证缺口独立保留，
不以 deselection/skip 改写为 PASS，也不在 S1 擅自修另一个能力包。
S2–S4、完整 record 执行与产品 profile 扩展仍开放。
