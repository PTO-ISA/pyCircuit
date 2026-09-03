# Cycle balance 详细实施计划

本文档是 `cycle_balance_improvement.md` 的落地细化，并记录已执行项。

## 阶段 A：IR 与校验

| 步骤 | 内容 | 状态 |
|------|------|------|
| A1 | 在 `include/pyc/Dialect/PYC/PYCOps.td` 为 `PYC_AssignOp` 增加 `OptionalAttr<I64Attr>`：`dst_cycle`、`src_cycle` | 已完成 |
| A2 | 重新 TableGen（构建时自动生成） | 随构建 |
| A3 | 在 `lib/Dialect/PYC/PYCOps.cpp` 的 `AssignOp::verify` 中：若仅一侧有属性则报错；若 `dst_cycle < src_cycle` 则报错 | 已完成 |

## 阶段 B：Pass 实现

| 步骤 | 内容 | 状态 |
|------|------|------|
| B1 | 新增 `lib/Transforms/CycleBalancePass.cpp`：`OperationPass<func::FuncOp>`，参数名 `pyc-cycle-balance` | 已完成 |
| B2 | `inferClkRst`：遍历 `pyc.reg` 取第一组 `(clk,rst)` 并检查全体一致；若无 `reg` 则尝试入口块 `!pyc.clock` / `!pyc.reset` 参数 | 已完成 |
| B3 | `getOrCreateDelayed`：`std::map` 键 `(src,clk,rst,depth)`；在 `inner` 定义之后插入下一级 `pyc.reg` 以保证支配 | 已完成 |
| B4 | 遍历带双属性的 `pyc.assign`：`d = dst - src`；`d==0` 删属性；`d>0` 替换 `src` 后删属性 | 已完成 |
| B5 | 插入的 `pyc.reg` 带 `pyc.name` = `pyc_cyclebal_N` | 已完成 |

## 阶段 C：集成

| 步骤 | 内容 | 状态 |
|------|------|------|
| C1 | `include/pyc/Transforms/Passes.h` 声明 `createCycleBalancePass()` | 已完成 |
| C2 | `compiler/mlir/CMakeLists.txt` 将 `CycleBalancePass.cpp` 加入 `pyc_transforms` | 已完成 |
| C3 | `tools/pycc.cpp`：`pyc-lower-scf-to-pyc-static` → `pyc-cycle-balance` → `pyc-eliminate-wires` | 已完成 |

## 阶段 D：验收

| 步骤 | 内容 | 状态 |
|------|------|------|
| D1 | 完整链接 `pycc` | 已在 LLVM 21 上通过；`pycc.cpp` 改用 `setMaxIterations` / `setMaxNumRewrites` |
| D2 | 手写 `.pyc`：两 `assign` 共享 `src`、相同 `d`，确认仅一条深度为 `d` 的寄存器链 | 建议 |
| D3 | 无周期属性的 IR | pass 为 no-op |

## 风险与回滚

- **风险**：多组 `(clk,rst)` 的模块在存在带属性 `assign` 时会被拒绝。
- **缓解**：无带属性 `assign` 时不做 `clk/rst` 一致性扫描。
- **回滚**：从 `pycc` 移除 `createCycleBalancePass` 一行即可。

## 执行记录

- **IR**：`pyc.assign` 可选 `dst_cycle` / `src_cycle`（`i64`），须成对且 `dst_cycle >= src_cycle`。
- **共享**：缓存键含原始 `src` 的 opaque 指针、`clk`/`rst`、`depth`；多 `assign` 同 `(src,d)` 复用同一末级 `q`。
- **前端**：`Circuit.assign` / `Module.assign` 支持关键字参数 `dst_cycle`、`src_cycle`（须成对），生成带属性的 `pyc.assign`。
