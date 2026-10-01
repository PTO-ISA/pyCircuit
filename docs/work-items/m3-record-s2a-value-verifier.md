# M3-E01 S2A：共享 record 值验证

状态：done for the bounded intermediate S2A packet。日期：2026-10-01。
输入候选 `4c3a4be690f30453acdc551e224e9ef258291ee1`，派发前工作树干净。
授权：[C2-DECL-R B](../rfcs/migration/approvals/c2-decl-record-final.md)，
SHA-256 `ea242da0d85de4f51c439051c80c2e7ce12c17dca5ef9a63b8743f0f280a0043`。
S1 已验收；本步骤不重新请求同一合同批准，也不扩大 Decision 0283 产品 profile。

## S2 的依赖分片

- S2A：共享 read/get/create 义务、实际 SSA/状态 handle、类型、来源与 selector 验证。
- S2B：Python capture/importer 的带参纯 helper、展开来源、source-order checks/live
  path、helper_return 和真实 source operations 的义务提取。
- S2C：record/numeric/use/check 的统一 final 保留、materialization、module selector、
  冻结与全包 fresh reconstruction。依赖 S2A/S2B，按实际状态与 S3 顺序集成。

S2A 交付一个明确有界的共享验证器，绑定现有 value.binding/use 和 final rule
验证入口。新 `ac.required_records` 是 B 已批准的唯一新属性；不新增 op、type、
flag、CLI、runtime 或 receipt。ODS 的 AnyType 放宽与严格类型验证同包交付。

## 精确准入与关闭边界

直接验证的 final rule 有一个 owned record current input 与同一 target，
恰一个 read、两个 field get、一个 create 的有序/唯一 RequiredRecord，
恰一个 next use/yield contribution，以及 B §6.4 的两个 get/zero/select/
synthetic create 组成的 n=1 selector。两字段类型和 nominal 来自独立 provider
final unit 的 canonical 声明；不在 consumer 复制 import snapshot。
实际 block argument、reg handle、StateRef、field ordinal、ordered SSA、valid/path
与 obligation 对应，全部 binding/record op/use 都必须进入闭合且不可孤立。
本切片的 read/get/create entry path 为已证明 true，next-use path 仅接纳
true/false 常量；动态 guard 的实际 threaded-path 证明随 S2B 实施，不以
任意安全 bool 字段作为 path 即宣布来源闭合。

仅本模块、empty-expansion 来源；empty checks/observations。numeric evidence、
math/scf/index、helper/expanded origin/return、check/observe 与其他未验证形状
明确 capability-reject，不能借 record 属性绕过旧 numeric/use/check 闭合。
非 record 规则仍保留原 signless-integer witness 宽度与验证责任，不把现有
宽中间数学 witness 用新 finite-field 上限误拒绝。

此阶段直接 native rule/op 验证可 PASS，整包 hardware/record reset/state/commit
验证必须仍 FAIL；两个 emitter 的 S1 guard 保持关闭。source-free reparse 使用
不自动验证的 parser，然后显式区分 rule seam 与整包检查，不以 parse 成功冒称
可执行 final。S3/S4、完整 S2/E01 与产品 record 执行未在本切片完成。

## 独占派发与验证

- 分解：`/root/m3_s2_decomposition`，独立 Sol high，只读依赖与范围核对。
- 共享 verifier：`/root/m3_s2a_record_core`，Luna high，仅新
  `ACIRFinalRecordUses.h/.cpp`；必要实质性拆分须报告，不用空 delegate 规避 600 行。
  PM 已批准 `ACIRRecordSelector.h/.cpp` 的 selector/精确 use-inventory 职责拆分。
- resolver/dispatch：`/root/m3_s2a_integration_impl`，另一个 Luna high，仅
  `ACIRFinalDeclarations.h/.cpp`、`ACIRPacketOps.cpp`、`ACIRFinalContracts.h/.cpp`、
  `ACIRNumericProofOps.cpp`、`ACIRNumericNextUse.cpp`、`ACIRDialect.cpp`。
  路径前缀均 `compiler/acir/lib/Dialect/ACIR/`。
- 独立测试：`/root/m3_s2a_independent_tests`，第三个 Luna high，仅新
  `tests/cpp/agentic-circuit/Dialect/ACIR/FinalRecordValueContractsTest.cpp`。
  PM 已批准共享 fixture `FinalRecordValueTestFixture.h` 与边界矩阵
  `FinalRecordValueEdgeContractsTest.cpp`，各新 C++/header 少于 600 行。
- PM 独占 ODS、两个 CMake source lists、文档、构建/安装、候选冻结与集成。
  source writers 不改 FinalProgram/HardwareClosure、importer、backend/runtime。
- 候选冻结后由独立 Sol code review 与 Astra 合同核对，不由作者自证。
  子实例不递归组队、不撤销其他人的文件、不自行提交。
- Fixture SSA 修复由独立 Sol high debugger `/root/m3_s2a_fixture_debug` 接手
  header；PM 只补 API/打印 location/namespace 等集成问题。移除旧 SSA 前必须
  先 drop yield/all-old-op references，不能留下悬空 OpOperand。语义断言未放宽。

- [x] 基线兼容测试通过现有 rule/op 验证入口真实 failing-first。
- [x] read/get/create schema、排序/唯一 ID、前向引用/DAG、实际 handle/SSA 闭合。
- [x] source-linked ops 与 exact-use synthetic selector 分离；零仅在合法 inactive leaf。
- [x] AnyType、缺属性、空属性、错误落点、nominal/来源/字段/控制篡改拒绝。
- [x] numeric/check/helper/observation 拒绝责任不被 record 分支降级。
- [x] source-free rule reparse、cross-unit nominal authority 与 whole-package 拒绝可复现。
- [x] S1/scalar/source/原生回归、独立审阅、raw XML/候选哈希与范围限制留档。

构建由当前 checkout 在 `.pycircuit_out/m3-s2a-build` 完成，安装到
`.pycircuit_out/m3-s2a-install`；LLVM/MLIR 22.1.8，四 jobs。测试源码与编译器
冻结后再运行 gate。已确认的三个 baseline SourceMath/APInt 失败仍单列，
不能声称完整 20-binary 原生 lane PASS。超出 B 的新合同分支上报 PM，其他
已授权工作继续。

## 验收与仍开放的工作

19 个 code/test/ODS/CMake 文件 aggregate：
`eaee80593852c78d3a5c5150c42afe47184283d669b72fb5599964914040e3ec`。
独立 Sol code review **APPROVE**，独立 Astra source conformance **CONFORMANT**；
见[审阅](../reviews/20261001-m3-record-s2a-review.md)与
[命令/原始证据](../gates/logs/20261001-m3-record-s2a/README.md)。
新 native 11 pass，reviewer 全 FinalProgram 69 pass；19 CTest binaries/279
cases 与 S1/scalar/source/CLI 129 cases 全通过，候选无 skip/disabled。
SourceMath 保留 91 中 88 pass/三个已知 APInt failure，不宣称全 20-binary PASS。

相同最终测试源码在 fresh pristine `4c3a4be6` 工具中逐例运行：0 pass、2 个
普通测试失败、9 个 SIGABRT。旧 ODS getter 将 record 强制 cast 为 IntegerType，
LLDB 栈证明失败发生在旧 ValueBinding/getter/generic verifier；这些是阶段缺口，
不能改写为普通 11 fail/no error。早期 fixture 编译/脚本/SSA 生命周期错误均修复，
不作为产品 failing-first 证据。

source 使用已支持的 `range(upper)` 生成基线包，之后在 focused native fixture
修改 final field lower=1，以验证 inactive zero 例外；不声明 Python 已支持
`range(lower,upper)` 或额外 annotation 表达式。S2B 接下来负责真实 source/helper
展开和 live-path 证明；S2C/S3/S4、完整 S2/E01 与 record 产品执行仍开放。
