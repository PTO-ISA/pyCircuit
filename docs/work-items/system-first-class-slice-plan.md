# 一等 `ac.system` 实施包：切片准备（未批准，仅准备）

状态：**准备文档，不是批准，也不是实现**。日期：2026-09-29。

精确内容以 [C2-SYSTEM 修订 B](../rfcs/migration/c2-system-definition-role.md)
（SHA-256 `db81dbc85e54346a0b8f953b6162c7b88d0ed5c5368b1cdbe26a2b4ccda79448`）
为准。该修订已获独立设计审阅 approval-ready，但
`docs/rfcs/migration/approvals/` 下**没有**对应批准记录，因此本包
**不得实施**新的 IR、CLI 或 manifest schema。本页只固定范围、依赖、文件归属、
拒绝用例与验收命令，供批准后按序执行。

## 0. 批准前置（阻断项）

| 项 | 状态 |
| --- | --- |
| 方向（一等 `ac.system` 定义） | 用户已选 |
| C2-SYSTEM 修订 B 精确字段 | 独立审阅 approval-ready；**用户未批准** |
| C2-EXPECT 修订 B 精确字段 | 独立审阅 approval-ready；**用户未批准** |

批准必须绑定上面的内容哈希。批准前允许做：分析、准备、以及不触及新 schema 的
测试/文档修正。批准前禁止做：改 ODS、改 source/header 映射、加
`ac.artifact_role`、改 public `link --role`、改 manifest。

**已知冲突（批准后必须一起处理）**：当前私有 `acir-design-harness --role` 强制
`@system` root ⟹ `testbench`。C2-SYSTEM 修订 B §5/§7 要求删除该私有规则（role 与
root 种类正交）。它属 §2 的 public link `--role`/IR role 变更，**本轮未改**。

## 1. 切片一：ODS、header authority、N1 与共用 module-like 验证

目标：让 `ac.system` 成为带 symbol/body 的定义、`ac.system.import` 成为 header
declaration，并让 source 阶段不再写 `ac.root_kind`；不新增硬件能力。

| 归属（单一 writer） | 文件 |
| --- | --- |
| 集成者（唯一改 ODS/注册/CMake） | `compiler/acir/include/acir/Dialect/ACIR/ACIROps.td`（`ac.system` 重构 + 新 `ac.system.import`）、dialect 注册与 `compiler/acir/lib/Dialect/ACIR/CMakeLists.txt` |
| dialect 实现者 | `compiler/acir/lib/Dialect/ACIR/ACIRSystemOps.cpp`（system verifier）、新 `ACIRSystemImportOps.cpp`；`ACIRModuleOps.cpp`、`ACIRModuleContracts.cpp` 中 `ac.root_kind` 校验的移除 |
| frontend 实现者 | `compiler/acir/lib/Compiler/PythonImportModuleBody.cpp`、`PythonImportModules.cpp`、`PythonImportModuleAnalysis.cpp`（`@system` → `ac.system` body / `ac.system.import` header；停止写 marker） |
| header authority 实现者 | `compiler/acir/lib/Compiler/SourceHeaderRegistry.cpp`、`SourceBodySnapshots.cpp`（`declaration_role=definition` vs `import_snapshot`；声明 op 种类纳入 snapshot 比较） |
| N1 实现者 | canonical hardware-definition target 类别扩展为 `ac.module.import` 或 `ac.system.import`；不新增 kind 字段 |
| 共用辅助（内部，非 primitive） | 新 `compiler/acir/lib/Compiler/ModuleLike.{h,cpp}`：统一的 module-like 查询/验证，不复制第二套 system typechecker |

独立测试作者（不同实例）：新增
`tests/cpp/agentic-circuit/Dialect/ACIR/SystemDefinitionContractsTest.cpp`；
扩展 `ACIRModuleGraphTests`、`ACIRSourceContractsTests`、`ACIRNamespaceContractsTests`。

拒绝用例（每条断言具体诊断 + 不产出/不改既有产物）：

1. source/header/linked/final 残留 `ac.root_kind`；
2. 旧零-region `ac.system` 描述符仍被接受；
3. 同 symbol 的 module/system 定义替换（snapshot 必须失配）；
4. 只有 `import_snapshot` 而无 owning header；
5. owning header 缺失、或 body/header 定义种类不匹配；
6. `ac.instance` callee 指向 system（root-only 仍成立）；
7. system body 内出现 `ac.system.import`/`ac.module.import` 残留。

验收命令（批准后）：

```bash
cmake --build "$PYC_BUILD" --target ACIRModuleGraphTests ACIRSourceContractsTests \
  ACIRNamespaceContractsTests ACIRSystemDefinitionContractsTests -j 4
for t in ACIRModuleGraphTests ACIRSourceContractsTests ACIRNamespaceContractsTests \
         ACIRSystemDefinitionContractsTests; do "$PYC_BUILD/bin/$t" --gtest_output="xml:$OUT/$t.xml"; done
```

依赖：无（首片）。不新增硬件能力，不改 reg/rule/proposal 时序。

## 2. 切片二：source/link/final/reparse 贯通，清理旧表示

目标：一条链上 `ac.entry` 唯一、final 不再插零-region 描述符、旧 marker 全清，
且 source identity、寄存器个数与逐拍 oracle 不变。

实测工作量（当前候选）：

- **约 20 处 `getParentOfType<ac::ModuleOp>` 假设**必须接受 module 或 system：
  `CheckGraph.cpp:76`、`FinalComposedNumeric.cpp:52`、`FinalNumeric.cpp:313`、
  `FinalProgram.cpp:670/1443/1450/1457/1505`、`FinalUses.cpp:39`、
  `ModuleGraph.cpp:246`、`ObservationGraph.cpp:107`、`ProposalGraph.cpp:511/624`、
  `ScalarNumericLoweringAnalysis.cpp:123`、`ScalarNumericMaskLowering.cpp:325`、
  `ScalarNumericNextUseLowering.cpp:76`、`ScalarNumericRangeLowering.cpp:359`、
  `Passes/InferRuleEffects.cpp:35`；
- 容器判定：`FinalProgram.cpp:71/73/698/720`、`ModuleGraph.cpp:101`、
  `SourceHeaderRegistry.cpp:141`、`InferRuleEffects.cpp:115`；
- 定义种类分支：`SourceBodySnapshots.cpp:206`、`Driver.cpp:519/541`；
- final 物化与重建：`FinalHardware.cpp`（移除 descriptor 构造，改为保留一等
  system 定义）、`FinalHardwareProgram.cpp`（从 system/module entry 重建）、
  `ACIRHardwareClosure.cpp`（canonical 检查改为按定义种类解析）；
- `ModuleGraph.cpp:125–139/271`（root/child 规则改为按 op 种类，不再按 marker）。

独立测试作者：扩展 `ACIRFinalProgramTests`、`ACIRModuleGraphTests`、
`ACIRSourceLinkAdmissionTests`；新增 roundtrip 反例（module/system body+header）。

拒绝用例：重复/未解析 entry；final 残留 zero-region system 或 `ac.root_kind`；
system 递归或作 child；跨种类 symbol 替换；伪造 owner；clock/reset 控制不合法。

必须保持的既有 oracle：V41 `[0,2,5,5,5]`/4 物理 reg、V42 `[0,0,3,6,6]`/5 物理
reg、失败不提交、reset 重跑、两后端一致。

依赖：切片一。

## 3. 切片三：双后端、role 传播与产物清单整合

目标：`ac.artifact_role` 只在最外层 linked/final package，emit 从 final 读 role
并写入 `generated.json.artifact_role`；`files[].role` 既有枚举不变。

| 归属 | 文件 |
| --- | --- |
| 集成者 | `compiler/acir/lib/Compiler/FinalEmit.cpp`（role 一致性校验）、public driver/manifest 相关（批准后） |
| C++ 后端 | `FinalEmitCpp.cpp`、`FinalEmitCppSystem.cpp` |
| RTL 后端 | `FinalEmitVerilog.cpp` |
| 私有桥（一致性断言，只比较不覆盖） | `compiler/acir/tools/acir-design-harness/acir-design-harness.cpp` |
| 发布校验 | 现有 publication validator：保持自包含，**不**绑成"必须等于本次新 role" |

拒绝用例：`ac.artifact_role` 落在 source/header/嵌套 unit/definition；缺失或非法
枚举；无原 final IR 时旧产物被要求匹配本次新 role；新 staging 的
`generated.json.artifact_role` 与本次 final IR 失配；同 owner 合法跨 role
replacement 被错误拒绝。

必须验证：同一硬件 root 改变 role 不改变周期/算术/提交结果；testbench 可实例化
共享 DUT module；被打 design 标签的 closed system **不能**自动成为独立 DUT 证据；
role 不从名称/观察/root 种类推断，也不能证明源码没有 stimulus。

依赖：切片二。

## 4. 明确不在本包内

外部 typed DUT、system 嵌套、多时钟/CDC、四态、FIFO/memory 库、公开 SDK/wheel、
真实并行执行、完整 `--events` 投递（其 report 归属另有待澄清项）、消费者设计。
