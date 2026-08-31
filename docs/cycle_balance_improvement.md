# Cycle balance 设计改进（pyCircuit）

## 1. 背景与问题

在 **cycle-aware** 编译模型中，每个数据值关联一个 **逻辑周期索引（occurrence / stage cycle）**，表示该值在流水线或调度语义下“有效”的周期。当 `pyc.assign` 的左值（目标线网）与右值在该索引上不一致时，编译器需要在右值侧插入 **寄存器（DFF / `pyc.reg`）** 做 **cycle balance**，使对齐后的右值与左值处于同一周期。

**Fanout 冗余问题**：若同一右值 SSA 被多个左值引用，且各自独立做 balance，可能在每条路径上各插一条等长延迟链，导致：

- 寄存器与连线重复，面积与功耗上升；
- 行为虽可能对齐，但结构非最小。

**期望**：编译器应 **intern（复用）** 延迟结果——对同一 `(右值, 时钟上下文, 复位上下文, 延迟深度 d)` 只保留一条延迟链，所有需要 `d` 拍对齐的 `assign` 共用其输出。

## 2. 当前 pyCircuit 编译器实现（摘要）

### 2.1 驱动与前端

- Python `pycircuit` 前端通过 `Module`/`Circuit` 生成文本 **`.pyc`（MLIR）**。
- **pyCircuit 6 以 CycleAwareSignal 为主路径**：`CycleAwareDomain.next()` 推进逻辑周期，信号运算按 cycle provenance 自动对齐。`ClockHandle`、`clk.next()` 和 `m.assign(..., dst_cycle=, src_cycle=)` 是结构化库接口使用的显式 occurrence-cycle 机制。两条入口降低到同一个 `pyc-cycle-balance` 语义；规范见 `docs/v6_PyCircuit_Specification.md`。

### 2.2 `pycc` 流水线（与 cycle 相关的位置）

典型优化与合法性顺序（节选，见 `compiler/mlir/tools/pycc.cpp`）：

1. 契约与层次：`pyc-check-frontend-contract`、`inline`、规范化、CSE、SCCP
2. 结构整理：`pyc-lower-scf-to-pyc-static`
3. **周期对齐**：`pyc-cycle-balance`（按 `dst_cycle`/`src_cycle` 插入并 **复用** 共享延迟寄存器）
4. 线网：`pyc-eliminate-wires`、`pyc-eliminate-dead-state`、`pyc-comb-canonicalize`、…
5. 合法性：`pyc-check-comb-cycles`、`pyc-check-clock-domains`
6. 寄存器打包：`pyc-pack-i1-regs`
7. 组合融合：`pyc-fuse-comb`（可选）
8. 深度统计：`pyc-check-logic-depth`

组合环检查依赖 `pyc.reg` 等作为时序割点；`pyc-cycle-balance` 新增的寄存器同样参与该割集。

### 2.3 当前 PYC IR（与本文相关部分）

| 构造 | 角色 |
|------|------|
| `pyc.wire` | 组合线网占位 |
| `pyc.assign` | `dst`（须为 `wire` 结果）← `src` |
| `pyc.reg` | `clk, rst, en, next, init` → `q` |
| `pyc.comb` | 融合组合区（与 tick/transfer 后端协作） |
| `pyc.instance` | 层次实例 |

周期语义 **尚未** 作为一等类型出现在类型系统里；若引入 cycle balance，宜先用 **assign 上的可选属性** 或独立 metadata pass 输入，再逐步规范化。

## 3. 设计目标（新要求）

1. **正确性**：`dst_cycle` 与 `src_cycle` 给定且 `dst_cycle >= src_cycle` 时，插入 `dst_cycle - src_cycle` 拍延迟，使驱动 `dst` 的数据与左值周期一致（在单时钟域、与既有 `tick/transfer` 语义一致的前提下）。
2. **共享延迟**：同一 `(src, clk, rst, d)` 只构建 **一条** `d` 级寄存器链（或等价结构），多 `assign` 复用最后一级 `q`（及中间级若需要）。
3. **时钟域**：首版可要求 **单主时钟/复位**（与模块内既有 `pyc.reg` 一致）；多域需显式扩展（绑定到域 ID 或不同 `clk/rst` 对）。
4. **可观测性**：插入的寄存器可带 `pyc.name` 前缀（如 `pyc_cyclebal_`）便于波形与调试。
5. **默认无行为**：未携带周期属性的 `pyc.assign` 与今保持一致，保证现有设计零差异。

## 4. 实现方案概要

### 4.1 IR 扩展

在 `pyc.assign` 上增加 **可选** 属性：

- `dst_cycle`：`i64`，左值周期索引
- `src_cycle`：`i64`，右值周期索引

约定：二者 **同时出现或同时省略**；若出现，必须 `dst_cycle >= src_cycle`。深度 `d = dst_cycle - src_cycle`；`d == 0` 时不插入寄存器，并可剥离属性。

### 4.2 新 Pass：`pyc-cycle-balance`

- **作用域**：`func.func` 内（与多数 PYC transform 一致）。
- **算法要点**：
  - 从函数体中解析 **默认 `clk/rst`**（例如取第一个 `pyc.reg` 的时钟与复位；若存在多组不一致则报错）。
  - 对每个带周期属性的 `pyc.assign`，计算 `d`，调用 `getOrCreateDelayed(src, d, clk, rst)`：
    - 内部缓存 `map[(src,clk,rst,d)] → q`；
    - 递归构造：`delayed(src,0)=src`；`delayed(src,k)` = 一级 `pyc.reg`，`next = delayed(src,k-1)`，`en = 1`，`init = 0`。
  - 将 `assign` 的 `src` 操作数替换为延迟链输出；**移除**周期属性，避免重复执行。
- **插入位置**：在对应 `pyc.assign` **之前**（保证 `src` 支配新寄存器）。
- **流水线位置**：在 **`pyc-eliminate-wires` 之前** 运行——此时仍保留 `wire`+`assign` 形态，与 `assign` 校验一致。

### 4.3 后续可选工作

- 前端/Python 生成 `dst_cycle`/`src_cycle`。
- 与 `pyc-check-clock-domains` 对齐：显式校验 balance 寄存器与目标 assign 的域一致。
- 带 `en` 的流水线寄存（非恒 1）的精确语义与共享策略。
- 在 `pyc-fuse-comb` 之后是否再跑一遍 CSE 以合并重复别名。

## 5. 文档索引

更细的步骤、文件清单与验收标准见 **`docs/cycle_balance_improvement_detailed_plan.md`**。

## 6. 实现落点（代码）

| 组件 | 路径 |
|------|------|
| IR：`pyc.assign` 周期属性 | `compiler/mlir/include/pyc/Dialect/PYC/PYCOps.td` |
| 校验 | `compiler/mlir/lib/Dialect/PYC/PYCOps.cpp`（`AssignOp::verify`） |
| Pass | `compiler/mlir/lib/Transforms/CycleBalancePass.cpp`（`--pyc-cycle-balance`） |
| 注册与链接 | `Passes.h`、`compiler/mlir/CMakeLists.txt` |
| 流水线 | `compiler/mlir/tools/pycc.cpp`（`createCycleBalancePass` 位于 lower-scf 与 eliminate-wires 之间） |

另：`pycc.cpp` 中对 `GreedyRewriteConfig` 使用 `setMaxIterations` / `setMaxNumRewrites`，以兼容 LLVM 21 将对应字段改为私有的变更。
