# M3-E01：已批准 record 的有界实施顺序

状态：S1 已验收；S2 ready-for-bounded-decomposition，S3/S4 尚未实施。日期：2026-10-01。
精确授权：[C2-DECL-R B 批准](../rfcs/migration/approvals/c2-decl-record-final.md)，
proposal SHA-256 `ea242da0d85de4f51c439051c80c2e7ce12c17dca5ef9a63b8743f0f280a0043`。
准入/设计/oracle已闭合；本包不是重新请求批准，也不是 record profile 已实现。

## 四个串行集成切片

| 切片 | 产品责任 | 有界验证/未交付范围 |
| --- | --- | --- |
| S1 final declarations/nominal authority | `FinalDeclarations`、shared `ACIRFinalDeclarations` 与对应 final envelope/preflight/reader：保留两字段 ac.struct、canonical constructor provenance-only、owner/origin/order/引用/排序 | source provider/header-only consumer、unused/private/facade、saved final/reparse、owner/field/constructor篡改；不冒称 stateful record 已跑通 |
| S2 source/helper/value/use closure | importer/helper inlining、value.binding/use record接纳、新 ac.required_records、actual SSA/路径/RequiredUse组合与selector/zero严格形状 | A14 helperassert-return suppression、A19/A20guard/zero/selectormutations、scalar回归、无finalscf/index；不把metadata当自证 |
| S3 one-record state/hardware view | record reset dict、whole-record reg/StateID/D/E、same verified scalar-leaf layout、唯一 no-fail Xfer 和丢弃/Reset | 单contribution/单writer、inactiveD无可观察/提交、fieldrange/root失败全树零提交；record ports/多driver仍拒绝 |
| S4 source-owned codegen/E2E acceptance | 两backend从同一final/leaf view，CPP值aggregate与SimDFFE数组载体分离、RTL位布局、源属header与TU inventory | `(3,17)→(17,4)→(4,18)`逐phase/literaloracle、hold/Reset/replay、final-onlyfreshprocessdualemit、实编实跑/无relayreg、输出保护 |

实际派发前 Sol/PM 基于当前文件边界给每片 **exclusive whitelist**、独立测试文件
和 exactselectors；表中组件定位不是任意改所有文件的许可。ODS/registry、link
pipeline、公共 runtime 头/root CMake、semantic gate/status由一个 integration owner。
不新增 proposal 没有定义的属性/primitive，不仅在一个backend修语义。

先增加 shared verifier/negative tests，再接纳/lower；partialslice不更新整个产品
supported profile。已批准接口同一范围内的普通实现/修复无需再次询问用户；发现
真正新增字段/语义超出B时，只冻结该新增分支并另提案，其余授权工作继续。

## 派发、独立性和出口

- 架构合同/oracle按已冻结B使用；Sol分解/集成、Luna有界实现、不同Luna tests、
  Sol独立review；复杂交叉scope再请原独立Astra核对合同适用性，不重复发明接口。
- 固定当前 HEAD/dirty/批准字节、所有tools/output命令，fresh build来自本checkout；
  source/test/native文件冻结后才执行候选gate，不改运行中的脚本。
- 每source一producer/owningheader/AC/filegroup；parent不能读providerbody或源fallback。
  implementation/declarations配对权威不能由CPP重建或额外Pythonsemanticcompiler替代。
- 基线 scalar/empty-static/单默认clock和已验证M6保护保持；独立记录失败阶段/diagnostic
  与已有输出bytes，不删oracle掩盖缺失能力。
- S4最终验收后才扩大当前profile到这个two-fieldrecord slice，更新语言/guide/decision/
  gate/例子与source/group/ABI依赖图；未支持features保留精确拒绝与backlog。

S1 的[声明投影/准入与验收](m3-record-s1-final-declarations.md)已经完成。
下一次从 S2 的实际 importer/helper/value/use 文件依赖核对开始，冻结独占文件
与独立 oracle；不同时发起四个共享文件 writer。S1 的 shared emit guard 仍
拒绝 record 包，只有 S4 双后端/源属声明完成后才能移除。三个已确认的基线
SourceMath/APInt 失败单列为验证缺口，不能把 S1 说成全原生 suite 通过。
本顺序只实现用户已批准record范围，不授权Bank、SYSTEM/EXPECT B、recordchildports、
record多contribution/writer、typedDUT、四态/memory/CDC或runtimeAPI变化。
