# M6-03：首次发布故障矩阵扩面

状态：done, bounded macOS/POSIX first-publication coverage。日期：2026-10-01。
依据：[M3/M6 扩展计划](m3-m6-expansion-plan.md)、C3-C 精确批准、M6-01。
输入基线 `4584ad0b`；派发前重新绑定实际 HEAD/dirty 内容与工具链。
本包先写独立测试，产品实现仅在实测缺陷出现后由 PM 另给 bounded repair ownership。

## 唯一目标与非目标

M6-01 已验证四路 replacement 的六个 checkpoint，first publication 仅覆盖
source unit。补齐首次 linked final、CPP bundle、RTL bundle 的五个可达点，
复用现有协议/私有 fault hook；不新造 fault API、journal/receipt 字段或作者认证。
不捆绑 Windows/SDK 发布、性能优化、parallel simulation 或新 M3 能力。

POSIX 原生 SIGKILL/flock 是首轮范围，不能以 skip 证明 Windows。普通 invalid
input/已有产物保护保留现有 gate，不重复冒称本包新完成能力。

## 独立状态表

| Fault point | first publication 预期 | 恢复后的目标 |
| --- | --- | --- |
| after_journal_preparing | preparing；stage 可为空，destination 不在 | absent |
| after_stage_complete | preparing；完整 stage、destination 不在 | absent |
| after_journal_prepared | prepared；完整 stage、destination 不在 | absent |
| after_previous_saved | 初次发布没有 previous，hook 不可达 | N/A，不能伪造 hit/PASS |
| after_destination_installed | prepared；destination 存在、尚未 committed | absent |
| after_journal_committed | committed；合法完整 destination | 保留新 bytes；按 shared-read/writer cleanup 语义处理 |

以上“恢复”必须通过该 artifact 的真实 public reader/writer 路径，不能私下删
journal 来使下一次成功。首次目标 absent 时不可能宣称原始 lock inode 保持；
检查首次创建后同一 control/lock 在后续恢复/重试的身份与 owner 稳定。

三个 artifact × 五点是 15 个新 interruption scenario；这是矩阵条目数，不是
pytest case 数。原 source-unit 五点/四路 replacement 作为回归，独立统计。

## 文件与角色

- 独立 tests：Luna high，唯一写 `tests/system/test_m6_publication_process_recovery.py`；
  若需改 test-only launcher，范围仅 `tests/integration/agentic-circuit/m6-publication/crash_runner.py`。
  复用现有 fixtures/布局 helpers，不重写 publication 协议或放宽断言。
- PM：当前 checkout fresh build/install、精确 fault hit/path/layout/byte 结果整合。
- 产品 bug repair：不同 Luna 实例；实测定位后单独冻结 affected files。共享 validator/
  filesystem/journal 文件由一个 writer；新接口需要精确批准，测试失败不自动授予修改权。
- 独立 Sol high review：审阅三类 reader/writer 的实际状态路径与最终候选。

## Checklist / verification

- [ ] 每项 real owned child self-SIGKILL，返回 `-SIGKILL`、确实到达指定点；不是抛异常自动 rollback。
- [ ] 中断后先验 phase、stage/destination/previous、source owner/control/lock 布局；不先恢复后猜状态。
- [ ] final reader 使用 verified emit，bundle reader 使用对应 public managed replacement/read 校验；
  malformed/partial output 不能成为有效输入，非零原因/阶段必须匹配，不靠“任意失败”。
- [ ] commit 前恢复 absent，随后正常首次发布成功；commit 后保留原新 artifact，重复恢复 idempotent。
- [ ] bundle 的 public writer 在恢复后可能立即重发布；在已有 test-only pause/fault
  checkpoint（例如下一事务的 after_journal_preparing）先观察 restored absence，
  再继续重试。不能由最终成功 replacement 推断中间已正确 rollback。
- [ ] first/replace 可达点分开，shared-reader cleanup 与 writer cleanup 不强行等同。
- [ ] 恢复中断、缺失/篡改 journal、owner mismatch 和等待 reader/writer 保持已有 C3 错误语义。
- [ ] 既有 compile/link/CPP/RTL replacement、source first、reentrant recovery 和锁案例仍通过。
- [ ] 原先同 owner 与合法 stale≠corrupt 行为不被“加安全校验”误拒绝；现有数据保护不放宽。
- [ ] 新矩阵给 failing-first/coverage evidence，artifact bytes 与合法 native parse/manifest验证绑定。
- [ ] 实测无产品 bug 时只提交测试；发现 bug 修复后刷新相关 gates、独立 review 与候选 manifest。

## 精确 gate 模板

先设置本候选的 LLVM/MLIR 22.1.8，再执行 fresh build。`python3` 必须有当前
pytest/dev 环境；不要用旧 M7 输出目录冒充本包 fresh native identity。

```sh
export PYC_BUILD_DIR="$PWD/.pycircuit_out/m6-03-root"
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/m6-03-install"
PYC_BUILD_TESTING=ON bash flows/scripts/pyc build --build-dir "$PYC_BUILD_DIR" --install-prefix "$PYC_TOOLCHAIN_ROOT"
export PYCIRCUIT_NATIVE_BUILD="$PYC_BUILD_DIR"
export PYCIRCUIT_COMPILER_INSTALL="$PYC_TOOLCHAIN_ROOT"
export PYCIRCUIT_M6_PREFIX="$PYC_TOOLCHAIN_ROOT"
export ACIR_SOURCE_UNIT_HARNESS="$PYC_BUILD_DIR/bin/acir-source-unit-harness"
export ACIR_DESIGN_HARNESS="$PYC_BUILD_DIR/bin/acir-design-harness"
export ACIR_CPP_SOURCE_PARTS_HARNESS="$PYC_BUILD_DIR/bin/acir-cpp-source-parts-harness"
python3 -m pytest tests/system/test_m6_publication_process_recovery.py -q
python3 -m pytest tests/unit/test_publication.py tests/unit/test_publication_fs.py tests/unit/test_driver_commands.py -q
```

证据入 `docs/gates/logs/<m6-03-run-id>/`：artifact/fault/path state/预期/实际/
command/exit/owner/bytes/lock/recovery 次数逐项留档；failed/unreachable/skipped 分列。
任何“15/15”必须指上述真实可达条目，不能由一行 pytest summary 推导。
完成出口：三类首次发布缺口关闭，已有 protocol/范围 unchanged，无额外 schema/ABI 承诺。

## 本轮完成记录 — 2026-10-01

产品提交 `9ff015a2` 完成 M3-P01 准入核对与 M6-03 首次发布扩面。
M3 独立 Astra 核对原批准字节、局部证据和 current代码：record 的既有 C1/C2/R1
语义无需整体重批，但 scalar-only final declaration projection 需精确增补。
下一包 [M3-E01 design/oracle](m3-record-final-projection-design.md) 已独立 planning
readiness通过；不批准产品实现。SYSTEM/EXPECT B 的原审阅归档已齐、精确批准仍无；
Bank carrier不自动准入，他人文稿保持未提交且未修改。

M6：15个新 first-publish SIGKILL 场景和10次 bundle follow-up writer中断恢复
通过；原 replacement/source-first/reentrant/lock断言保留。PM系统5passed；
regression131passed、3Windows-only skips；独立 Sol 实跑5passed。没有产品
协议/接口改动。范围限macOS/POSIX，不证明Windows、networkFS或power-loss。

- [M3完整核对与证据](https://github.com/PTO-ISA/pyCircuit/blob/9ff015a2/docs/gates/logs/20261001-m3-admission/README.md)
- [M6完整测试与证据](https://github.com/PTO-ISA/pyCircuit/blob/9ff015a2/docs/gates/logs/20261001-m6-first-publication/README.md)
- [M3独立核对](https://github.com/PTO-ISA/pyCircuit/blob/9ff015a2/docs/reviews/20261001-m3-contract-admission-review.md)
- [M6独立审阅](https://github.com/PTO-ISA/pyCircuit/blob/9ff015a2/docs/reviews/20261001-m6-first-publication-review.md)

实施 checkout hooks/strictdocs通过。规划 checkout 的12条既有historical
missinglinks保持单独披露，不在此包修复或冒称planningstrictpass。
