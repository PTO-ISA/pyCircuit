# M3-E01 S1 accepted: final record declarations

状态：done for bounded S1，2026-10-01。产品提交 `4c3a4be6`，base `1efec35e`。
按已批准 C2-DECL-R B 完成 final 两字段 record 声明投影、owning-header authority、
constructor provenance/erasure safety、完整 inventory 与冻结/重解析。
独立 Luna 实现与测试，Sol APPROVE，Astra CONFORMANT。

- [完整任务和验收](https://github.com/PTO-ISA/pyCircuit/blob/4c3a4be6/docs/work-items/m3-record-s1-final-declarations.md)
- [候选绑定审阅](https://github.com/PTO-ISA/pyCircuit/blob/4c3a4be6/docs/reviews/20261001-m3-record-s1-review.md)
- [命令与原始证据](https://github.com/PTO-ISA/pyCircuit/blob/4c3a4be6/docs/gates/logs/20261001-m3-record-s1/README.md)

Code/test/CMake aggregate：
`3405b937fb6ddce11a21214c6ec3722a73a15455087a33101705ea72a55e1159`。
10 system、4 record native、19 CTest binaries/268 cases、119 安装工具回归通过，
无 fail/error/skip/disabled；12 条实际安装 CLI smoke、严格文档/hook 通过。
同一最终 system 测试在 fresh pristine baseline 工具上 6 fail/4 pass。

不声称完整 20-binary native lane 通过：三个 SourceMath/APInt 数值变异测试在
同一构建选项的 pristine baseline 与候选中均失败，原始证据保留，不改写为 skip。
S1 含 record 的 CPP/RTL/runner/parts emission 仍共同拒绝，防止生成不完整产物。
下一步 S2 helper/value/use/required_records；S3 state/reset/commit 与 S4
双后端仍未实施，record 执行、整个 E01 与完整 M3 没有在本包完成。
