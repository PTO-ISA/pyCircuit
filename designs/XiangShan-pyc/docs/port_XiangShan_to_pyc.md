# XiangShan → PyCircuit V6 Implementation Plan

**目的：** 以香山（XiangShan）昆明湖微架构规格为蓝本，使用 PyCircuit V6 cycle-aware 语法从零实现全部 RTL 和测试，输出到 `designs/XiangShan-pyc/`。

**方法论：** 遵循 `docs/pycircuit_implementation_method.md` 十步工作流。

**输入来源：**

| 来源 | 本地路径 | 用途 |
|------|---------|------|
| XiangShan 微架构文档 | `designs/XiangShan-doc/docs/` | 前端/后端/访存/Cache 分模块设计规格 |
| XiangShan 架构框图 | `designs/XiangShan-doc/docs/figs/` | 模块关系、流水线结构、数据通路图 |
| XiangShan 参考实现 | `designs/XiangShan/` | 行为参考（用于提取端口宽度、参数默认值、功能细节） |

> **注意：** `designs/XiangShan/` 中的原始实现仅作为**规格参考**使用，用于提取端口定义、参数配置和行为细节。XiangShan-pyc 中的全部 RTL 代码和测试均使用 **PyCircuit V6** 原生语法编写，不保留、不依赖、不引用任何原始框架的代码结构。

---

## 一、项目规模评估

### 1.1 微架构模块统计

以下按 XiangShan-doc 文档结构和参考实现中的模块层次统计：

| 子系统 | 参考文档路径 | 估计模块数 | 主要子模块 |
|--------|-------------|-----------|-----------|
| **Frontend** | `XiangShan-doc/docs/frontend/` | ~30 | BPU (uBTB/TAGE/SC/ITTAGE/RAS), FTQ, IFU, ICache, IBuffer, Decode |
| **Backend** | `XiangShan-doc/docs/backend/` | ~50 | Rename (RAT/Freelist), Dispatch, Scheduler, IssueQueue, ExeUnits (Int/FP/Vec), ROB, RegFile |
| **MemBlock** | `XiangShan-doc/docs/memory/` | ~25 | LoadPipeline, StorePipeline, LSQ, SBuffer, Prefetcher, 向量访存 |
| **Cache/MMU** | `XiangShan-doc/docs/huancun/`, `memory/mmu/`, `memory/dcache/` | ~20 | DCache, TLB, PTW, WPU, L2/L3 Cache |
| **Top/SoC** | 架构框图 | ~10 | XSCore, XSTile, XSTop, 外设 (PLIC, CLINT, Debug) |
| **合计** | — | **~135** | — |

### 1.2 外部 IP / 协议组件

| 组件 | 角色 | PyCircuit V6 实现方案 |
|------|------|----------------------|
| TileLink 总线协议 | 核内/L2 互联 | `lib/tilelink.py`：用 PyCircuit V6 原生定义 A/B/C/D/E 通道端口和协议常量 |
| AXI 总线协议 | SoC 外部接口 | `lib/axi.py`：用 PyCircuit V6 原生定义读写通道端口 |
| L2 Cache (CoupledL2) | Non-inclusive L2 | 独立 PyCircuit V6 实现单元 |
| L3 Cache (HuanCun) | Inclusive L3 | 独立 PyCircuit V6 实现单元（可延后） |
| 向量运算单元 | RVV 执行 | 独立 PyCircuit V6 实现单元 |
| 中断控制器 (AIA) | 中断分发 | 低优先级，独立实现 |

### 1.3 模块层次

```text
XSTop (SoC)
├── XSTile[0..N-1]
│   ├── XSCore
│   │   ├── Frontend ─── BPU, FTQ, IFU, ICache, IBuffer, Decode
│   │   ├── Backend ──── Rename, Dispatch, Scheduler, Issue, ExeUnits (Int/FP/Vec), ROB
│   │   └── MemBlock ─── LoadPipeline, StorePipeline, LSQ, SBuffer, Prefetcher, DCache, MMU
│   └── L2Top ────────── CoupledL2
├── L3 (HuanCun / OpenLLC)
└── SoC peripherals (PLIC, CLINT, Debug, …)
```

---

## 二、实现策略：自底向上 + 分阶段

### 原则

1. **纯 PyCircuit V6：** 全部 RTL 使用 `CycleAwareCircuit` / `CycleAwareDomain` 编写，全部测试使用 PyCircuit Testbench 框架。不依赖任何外部 HDL 框架。
2. **自底向上：** 先实现叶子模块（无子模块依赖的纯逻辑单元），逐层向上组装。
3. **子系统隔离：** Frontend / Backend / MemBlock 作为三条独立实现主线，可并行推进。
4. **参数化推迟：** 先以昆明湖默认配置硬编码，待核心功能正确后再提取参数。
5. **显式端口：** 所有模块间连接使用显式端口定义，协议参数从设计文档和参考实现的默认值中提取。
6. **每个模块独立可验证：** 实现一个模块即写 PyCircuit TB + 运行 `emit_mlir()`。

### 目录布局

```text
designs/XiangShan-pyc/
├── docs/
│   ├── port_XiangShan_to_pyc.md        ← 本文件
│   ├── requirement_sources.md           ← 文档索引
│   ├── ASSUMPTIONS.md                   ← 设计假设与冲突记录
│   ├── port_list/                       ← 按子系统分文件
│   ├── feature_list/                    ← 按子系统分文件
│   ├── traceability/                    ← 追溯矩阵
│   └── step1.md … step10.md            ← 可选子步骤文档
├── lib/                                 ← 公共 PyCircuit V6 工具库
│   ├── primitives.py                    ← 常用组合逻辑原语（one-hot mux、PopCount、PriorityEncoder 等）
│   ├── tilelink.py                      ← TileLink 通道端口定义与协议常量
│   └── axi.py                           ← AXI 通道端口定义
├── frontend/
│   ├── bpu/
│   │   ├── ubtb.py                      ← micro BTB
│   │   ├── tage.py                      ← TAGE 预测器
│   │   ├── sc.py                        ← Statistical Corrector
│   │   ├── ittage.py                    ← ITTAGE 间接跳转预测
│   │   ├── ras.py                       ← Return Address Stack
│   │   ├── bpu.py                       ← BPU 顶层
│   │   └── tb_bpu.py
│   ├── ftq/
│   │   ├── ftq.py                       ← Fetch Target Queue
│   │   └── tb_ftq.py
│   ├── ifu/
│   │   ├── ifu.py                       ← Instruction Fetch Unit
│   │   └── tb_ifu.py
│   ├── icache/
│   │   ├── icache.py                    ← Instruction Cache
│   │   └── tb_icache.py
│   ├── ibuffer/
│   │   ├── ibuffer.py                   ← Instruction Buffer
│   │   └── tb_ibuffer.py
│   ├── decode/
│   │   ├── decode.py                    ← 译码器
│   │   └── tb_decode.py
│   ├── frontend.py                      ← Frontend 顶层集成
│   └── tb_frontend.py
├── backend/
│   ├── rename/
│   ├── dispatch/
│   ├── issue/
│   ├── exu/
│   ├── fu/
│   ├── rob/
│   ├── regfile/
│   ├── backend.py                       ← Backend 顶层集成
│   └── tb_backend.py
├── mem/
│   ├── pipeline/
│   ├── lsqueue/
│   ├── sbuffer/
│   ├── prefetch/
│   ├── memblock.py                      ← MemBlock 顶层集成
│   └── tb_memblock.py
├── cache/
│   ├── dcache/
│   ├── mmu/
│   └── wpu/
├── l2/                                  ← L2 Cache
├── top/
│   ├── xs_core.py                       ← XSCore (Frontend + Backend + MemBlock)
│   ├── xs_tile.py
│   ├── xs_top.py
│   └── parameters.py                   ← 昆明湖默认参数（纯 Python 常量）
├── test_xs_steps.py                     ← 十步验证 pytest suite
├── run_xs_verification.py               ← 独立验证脚本
└── README.md
```

---

## 三、十步工作流映射

### Step 1 — 阅读 PyCircuit V6 编程风格文档与示例

| 动作 | 内容 |
|------|------|
| 阅读 PyCircuit V6 编程规范 | `docs/v6_PyCircuit_Specification.md` |
| 阅读测试框架 | `docs/TESTBENCH.md` |
| 阅读 IR / 编译 | `docs/IR_SPEC.md`，`docs/PIPELINE.md`，`docs/PRIMITIVES.md` |
| 参考实现 | `designs/BypassUnit/`，`designs/IssueQueue/`，`designs/RegisterFile/` |
| 交付物 | `designs/XiangShan-pyc/README.md`（编程风格：V6 cycle-aware），`ASSUMPTIONS.md` |

### Step 2 — 阅读 XiangShan 微架构设计规格

| 动作 | 内容 |
|------|------|
| 核心架构文档 | `XiangShan-doc/docs/frontend/overview.md`、`backend/overview.md`、`memory/overview.md`、`huancun/overview.md` |
| 架构框图 | `XiangShan-doc/docs/figs/xs-arch-simple.svg`、各子系统框图 |
| 参数提取 | 从参考实现中提取端口宽度、队列深度、流水线级数等默认配置 → `top/parameters.py` |
| 行为细节 | 从参考实现中理解非文档化的边界情况和微架构细节 |
| 交付物 | `requirement_sources.md`（文档索引 + 参数清单） |

### Step 3 — 端口、总线、特性列表

按子系统分文件，端口定义和特性列表全部基于设计文档和参考行为：

| 子系统 | 端口来源 | 特性来源 |
|--------|---------|---------|
| Frontend | 架构文档 + 参考实现 IO 分析 → `port_list/frontend.md` | BPU 预测、ICache 命中/缺失、IBuffer 流控、Decode → `feature_list/frontend.md` |
| Backend | 架构文档 + 参考实现 IO 分析 → `port_list/backend.md` | 重命名、分派、发射、执行、ROB → `feature_list/backend.md` |
| MemBlock | 架构文档 + 参考实现 IO 分析 → `port_list/memblock.md` | Load/Store 流水线、LSQ、SBuffer → `feature_list/memblock.md` |
| Cache/MMU | 架构文档 + 参考实现 IO 分析 → `port_list/cache.md` | DCache、TLB、PTW → `feature_list/cache.md` |
| Top | 架构框图 → `port_list/top.md` | 核间连接、L2 接口 → `feature_list/top.md` |

### Step 4 — 顺序行为描述（伪代码）

对每个子系统写 `ALGORITHM_SEQUENTIAL.md`，使用纯 Python 伪代码描述每拍行为：

- **Frontend：** `fetch_cycle()` → BPU predict → ICache lookup → IFU → IBuffer → Decode
- **Backend：** `execute_cycle()` → Rename → Dispatch → Issue → ExeUnit → Writeback → ROB commit
- **MemBlock：** `mem_cycle()` → Load pipeline → Store pipeline → LSQ → SBuffer → DCache → Prefetch

### Step 5 — 流水线映射（`domain.next()` 规划）

关键约束：XiangShan 是**超标量、乱序、多发射**处理器，流水线级数多。

| 子系统 | 预估 `domain.next()` 段数 | 说明 |
|--------|---------------------------|------|
| BPU | 3–4 | s0(predict) / s1(check) / s2(redirect) / s3(update) |
| IFU + ICache | 3–4 | fetch req / cache access / hit-miss / respond |
| Decode + IBuffer | 2 | buffer → decode |
| Rename | 2 | 逻辑寄存器→物理寄存器映射 |
| Dispatch | 2 | 分派到各 issue queue |
| Issue Queue | 2–3 | enqueue / select / dequeue |
| ExeUnit | 1–3 | 依 FU 类型不同（ALU=1, MUL=2, DIV=多拍 FSM） |
| ROB | 2 | writeback → commit |
| Load pipeline | 3–4 | TLB / DCache / data 回写 |
| Store pipeline | 3–4 | TLB / SBuffer / DCache write |

### Step 6 — 完整 cycle-aware PyCircuit V6 实现

按子系统逐模块编写，全部使用 PyCircuit V6 原生语法：

```python
from pycircuit.v6 import CycleAwareCircuit, CycleAwareDomain, cas, mux

def build_bpu(m: CycleAwareCircuit, domain: CycleAwareDomain):
    cd = domain.clock_domain
    rst = m.pyc_reset_active(cd.rst)
    # --- Occurrence 0: generate prediction ---
    pc_in = cas(domain, m.input("pc", width=39), cycle=0)
    ...
    domain.next()
    # --- Occurrence 1: check / override ---
    ...
    domain.next()
    # --- Occurrence 2: redirect ---
    ...
```

**PyCircuit V6 编程惯用法（替代外部框架构造）：**

| 硬件构造 | PyCircuit V6 表达 |
|---------|------------------|
| 带复位寄存器 | `reg = domain.signal(width=W, reset_value=V, name="reg")`；`reg <<= next_val` 或 `reg.assign(next_val)` |
| 流水线寄存器 | `pipe_reg = domain.cycle(data, name="pipe_reg")` |
| 多路选择 | `mux(cond, a, b)` |
| One-hot 选择 | `lib/primitives.py` 中封装 `mux1h(sels, vals)` |
| 位拼接 | `m.cat(a, b)` |
| 位切片 | `a.slice(hi, lo)` 或 `a[lo:hi+1]` |
| 同步存储器 | `m.sync_mem(clk, rst, ren=..., raddr=..., wvalid=..., waddr=..., wdata=..., wstrb=..., depth=D, name="mem")` |
| 多时钟域 | 多 `CycleAwareDomain`；`domain.create_reset()` |
| 常量 | `m.const(value, width=W)` |
| 零值 | `_zero(m, W)` → `m.const(0, width=W)` |
| PopCount | `lib/primitives.py` 中封装归约树 |
| PriorityEncoder | `lib/primitives.py` 中封装 |
| Valid/Ready 握手 | 显式 `_valid`, `_ready`, `_bits` 端口三元组 |
| 模块层次 | `build_*` 函数内联调用，或 `pyc_CircuitModule` 封装 |
| 参数化 | `top/parameters.py` 中纯 Python 常量/字典 |

### Step 7 — 规格追溯

每个子系统的 `traceability/<subsystem>.md`：

- 设计文档章节 → PyCircuit V6 模块 → 特性 ID → 测试 ID
- 验证每个规格中定义的功能在 PyCircuit V6 实现中都有对应

### Step 8 — 测试计划（全部使用 PyCircuit Testbench）

| 层级 | 测试策略 |
|------|---------|
| 单元 | 每个叶子模块（ALU、MUL、BPU predictor 等）用 PyCircuit `@testbench` 独立测试 |
| 子系统 | Frontend / Backend / MemBlock 分别集成测试（`tb_frontend.py` 等） |
| 核级 | XSCore 端到端 TB：取指→执行→访存→提交 |
| 仿真波形 | 使用 Perfetto UI trace 格式（Chrome Trace Event JSON）输出仿真波形 |
| MLIR 契约 | 每个模块 `emit_mlir()` + `pyc.reg` 计数断言 |
| Verilog 生成 | `pycc --emit=verilog` 全模块编译验证 |

### Step 9 — 增量实现计划

详见下方 §四。

### Step 10 — 系统测试

- 完整核级仿真（取指-执行-访存-提交）
- 多核 + L2 + L3 集成测试
- 性能指标验证（IPC、分支预测命中率等）
- Perfetto UI 全核波形生成

---

## 四、增量实现计划（阶段划分）

### Phase 0：基础设施（预计 1–2 周）

| Inc | 内容 | 交付物 |
|-----|------|--------|
| P0-1 | 创建 `lib/primitives.py`：PyCircuit V6 常用组合逻辑原语（`mux1h`、`mux_lookup`、`popcount`、`priority_enc`、`leading_zeros` 等） | `lib/primitives.py` + `lib/tb_primitives.py` |
| P0-2 | 创建 `lib/tilelink.py`：TileLink-A/B/C/D/E 通道端口定义和协议常量（用 PyCircuit V6 原生端口） | `lib/tilelink.py` |
| P0-3 | 创建 `lib/axi.py`：AXI4 读写通道端口定义 | `lib/axi.py` |
| P0-4 | 提取 `top/parameters.py`：昆明湖默认配置（FetchWidth、DecodeWidth、IssueQueueSize 等），纯 Python 常量 | `top/parameters.py` |
| P0-5 | 项目框架：`README.md`、`ASSUMPTIONS.md`、`requirement_sources.md`、`test_xs_steps.py` 骨架 | 十步文档骨架 |

### Phase 1：Frontend（预计 3–4 周）

优先级：Frontend 是最独立的子系统，与 Backend/MemBlock 接口清晰（IBuffer 输出 → Decode → Backend）。

| Inc | 模块 | 参考文档 | 难度 | 依赖 |
|-----|------|---------|------|------|
| F-01 | **FTQ**（Fetch Target Queue） | `frontend/ftq.md` | 高 | P0 |
| F-02 | **BPU** 框架 + uBTB | `frontend/bp.md` | 高 | F-01 |
| F-03 | **TAGE** + SC + ITTAGE + RAS | `frontend/bp.md` | 高 | F-02 |
| F-04 | **ICache** | `frontend/icache.md` | 中 | P0 |
| F-05 | **IFU** | `frontend/ifu.md` | 中 | F-04, F-01 |
| F-06 | **IBuffer** | `frontend/overview.md` | 低 | F-05 |
| F-07 | **Decode** | `frontend/decode.md` | 中 | F-06 |
| F-08 | **Frontend 顶层集成** | `frontend/overview.md` | 中 | F-01..F-07 |

每个 Inc 交付：`<module>.py` + `tb_<module>.py` + `emit_mlir()` 通过 + `feature_list` 更新。

### Phase 2：Backend（预计 4–6 周）

| Inc | 模块 | 参考文档 | 难度 | 依赖 |
|-----|------|---------|------|------|
| B-01 | **Rename** (RAT, freelist) | `backend/rename.md` | 中 | P0 |
| B-02 | **ROB** | `backend/rob.md` | 高 | B-01 |
| B-03 | **Dispatch** | `backend/dispatch.md` | 中 | B-01, B-02 |
| B-04 | **Issue Queue** (wakeup, select) | `backend/issue.md`, `backend/scheduler.md` | 高 | B-03 |
| B-05 | **RegFile** + RegCache | `backend/overview.md` | 中 | B-04 |
| B-06 | **ExeUnits — Int** (ALU, MUL, DIV, BRU) | `backend/exu_int.md` | 高 | B-05 |
| B-07 | **ExeUnits — FP/Vec** | `backend/exu_fp.md` | 很高 | B-06 |
| B-08 | **CtrlBlock** + datapath | `backend/overview.md` | 高 | B-01..B-07 |
| B-09 | **Backend 顶层集成** | `backend/overview.md` | 高 | B-01..B-08 |

每个 Inc 交付：`<module>.py` + `tb_<module>.py` + `emit_mlir()` 通过。

### Phase 3：MemBlock + Cache（预计 3–4 周）

| Inc | 模块 | 参考文档 | 难度 | 依赖 |
|-----|------|---------|------|------|
| M-01 | **MMU / TLB** | `memory/mmu/*.md` | 高 | P0 |
| M-02 | **DCache** (tag/data/MSHR) | `memory/dcache/*.md` | 高 | M-01 |
| M-03 | **Load Pipeline** | `memory/overview.md`, `memory/mechanism.md` | 中 | M-01, M-02 |
| M-04 | **Store Pipeline** + SBuffer | `memory/overview.md` | 中 | M-01, M-02 |
| M-05 | **LSQ** | `memory/lsq/*.md` | 高 | M-03, M-04 |
| M-06 | **Prefetcher** | `memory/overview.md` | 中 | M-02 |
| M-07 | **MemBlock 顶层集成** | `memory/overview.md` | 高 | M-01..M-06 |

### Phase 4：核级集成（预计 2–3 周）

| Inc | 模块 | 依赖 |
|-----|------|------|
| C-01 | **XSCore** (Frontend + Backend + MemBlock 互联) | F-08, B-09, M-07 |
| C-02 | **L2Top** (L2 Cache 接口壳) | C-01 |
| C-03 | **XSTile** | C-01, C-02 |
| C-04 | **XSTop** (SoC wrapper) | C-03 |

### Phase 5：Cache 层次与 SoC（预计 3–4 周，可延后）

| Inc | 模块 | 依赖 |
|-----|------|------|
| L-01 | **CoupledL2** 完整实现 | C-02 |
| L-02 | **HuanCun** L3 | L-01 |
| L-03 | **SoC 外设** (PLIC, CLINT, Debug) | C-04 |
| L-04 | **OpenLLC** (NoC LLC) | L-02 (可选) |

---

## 五、PyCircuit V6 设计规范

### 5.1 文件命名约定

| 类型 | 命名规则 | 示例 |
|------|---------|------|
| RTL 模块 | `<module_name>.py`，内含 `build_<module_name>()` 函数 | `frontend/bpu/tage.py` → `build_tage()` |
| Testbench | `tb_<module_name>.py` | `frontend/bpu/tb_tage.py` |
| MLIR 输出 | `<module_name>_mlir.py`（含 `emit_<name>_mlir()` 函数） | 可内嵌在 RTL 模块文件末尾 |
| 参数 | `top/parameters.py` 集中管理 | — |
| 公共原语 | `lib/*.py` | — |

### 5.2 模块接口风格

所有模块间连接使用 PyCircuit V6 显式端口：

```python
def build_ifu(m: CycleAwareCircuit, domain: CycleAwareDomain):
    cd = domain.clock_domain
    rst = m.pyc_reset_active(cd.rst)

    # 输入端口（来自 FTQ）
    ftq_valid  = cas(domain, m.input("ftq_to_ifu_valid", width=1), cycle=0)
    ftq_pc     = cas(domain, m.input("ftq_to_ifu_pc", width=39), cycle=0)
    ftq_target = cas(domain, m.input("ftq_to_ifu_target", width=39), cycle=0)

    # 输入端口（来自 ICache）
    icache_data  = cas(domain, m.input("icache_resp_data", width=512), cycle=0)
    icache_valid = cas(domain, m.input("icache_resp_valid", width=1), cycle=0)

    # --- Occurrence 0: fetch request ---
    ...
    domain.next()
    # --- Occurrence 1: instruction alignment ---
    ...

    # 输出端口（到 IBuffer）
    m.output("ifu_to_ibuf_valid", ifu_out_valid)
    m.output("ifu_to_ibuf_insts", ifu_out_insts)
```

### 5.3 Valid/Ready 握手协议

对于需要反压的接口，使用三端口模式：

```python
# 发送端
m.output("req_valid", req_valid_signal)
m.output("req_bits", req_data_signal)
req_ready = cas(domain, m.input("req_ready", width=1), cycle=0)
fire = req_valid_signal & req_ready

# 接收端
req_valid = cas(domain, m.input("req_valid", width=1), cycle=0)
req_bits  = cas(domain, m.input("req_bits", width=W), cycle=0)
m.output("req_ready", ready_signal)
```

---

## 六、风险与缓解

| 风险 | 影响 | 缓解 |
|------|------|------|
| 微架构细节仅在参考实现中，未文档化 | 部分行为需从参考代码中逆向提取 | 先以文档为准实现核心功能，参考实现用于补充边界情况 |
| 向量扩展（RVV）复杂度 | 向量运算 + 向量访存拆分/合并逻辑量大 | Phase 2 先实现标量路径（B-06），向量（B-07）延后 |
| 隐式时钟/复位约定 | 参考实现中模块自带 `clock`/`reset`；PyCircuit V6 要求显式 `domain` | 统一模式：每个 `build_*` 接收 `(m, domain)` 参数 |
| 功能等价验证 | 无成熟的 ISS 对接 | 模块级 Perfetto 波形对比 + 指令级 golden 测试向量 |
| 项目规模（~135 模块） | 全量实现耗时长 | 分阶段：Phase 1–3 并行推进；先关注核心数据通路，外设延后 |

---

## 七、里程碑与验收标准

| 里程碑 | 内容 | 验收 |
|--------|------|------|
| **M0** | 基础设施 + 文档骨架 | `lib/` 通过 PyCircuit TB 单元测试；`parameters.py` 可 import |
| **M1** | Frontend `emit_mlir()` 通过 | 所有 Frontend 模块编译为 MLIR；PyCircuit TB 复位后空闲 |
| **M2** | Backend 标量路径 `emit_mlir()` 通过 | Rename→Dispatch→Issue→ALU→ROB 路径可编译 |
| **M3** | MemBlock `emit_mlir()` 通过 | Load/Store pipeline + DCache 可编译 |
| **M4** | XSCore 核级集成 | 取指→译码→执行→访存→提交 MLIR 通过 |
| **M5** | Verilog 生成 + 基础仿真 | `pycc --emit=verilog` 输出全核 Verilog；PyCircuit TB 通过 |
| **M6** | L2/L3 + SoC | 完整 XSTop Verilog + 系统级 PyCircuit TB |

---

## 八、开始执行的第一步

1. 确认 `designs/XiangShan-pyc/` 目录已创建。
2. 执行 **Phase 0**（P0-1 ~ P0-5）：
   - 编写 `lib/primitives.py`（`mux1h`、`popcount`、`priority_enc` 等）和 PyCircuit TB 单元测试。
   - 从 XiangShan-doc 文档和参考实现中提取默认配置到 `top/parameters.py`。
   - 建立 `README.md`、`ASSUMPTIONS.md`。
3. 选择 Frontend 中最简单的叶子模块（如 IBuffer）作为首个实现目标，完成端到端流程验证（规格理解 → PyCircuit V6 实现 → MLIR → Verilog）。

---

Copyright (C) 2024–2026 PyCircuit Contributors
