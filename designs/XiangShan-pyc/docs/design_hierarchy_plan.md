# XiangShan-pyc 层次化重构计划（函数调用方式）

**版本：1.0**
**日期：2026-04-02**
**方法：** 子函数调用（Method A）— 所有层次关系通过 `build_*()` 函数调用表达

---

## 一、设计原则

### 1.1 核心思路

当前 XiangShan-pyc 的 40 个模块各自独立、扁平实现，互不调用。重构目标是：

1. **父模块通过函数调用包含子模块**：`build_xs_core()` 内部调用 `build_frontend()`、`build_backend()`、`build_memblock()`
2. **子函数的逻辑扁平展开到父函数的信号图**：共享 `(m, domain)` 上下文
3. **生成的 Verilog 保持层次化**：通过 `build_verilog.py --hierarchical` 的后处理 wrapper 机制
4. **不使用 `m.instance()` 语法**：完全依赖 Python 函数调用

### 1.2 编程约定

```python
def build_子模块(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "子模块缩写",     # 必选，用于命名隔离
    参数1: int = 默认值,
    参数2: int = 默认值,
) -> 返回值类型 | None:
    """模块说明"""
    domain.push()  # 若需要周期隔离
    # ... 模块逻辑 ...
    domain.pop()   # 配对恢复
    return 返回值   # 若父模块需要子模块的输出信号
```

### 1.3 命名规则

| 层次 | 前缀模式 | 示例 |
|------|---------|------|
| SoC 顶层 | `soc_` | `soc_axi_rdata` |
| Tile | `tile{N}_` | `tile0_l2_req` |
| Core | `core_` | `core_redirect_valid` |
| Frontend | `fe_` | `fe_fetch_pc` |
| BPU | `fe_bpu_` | `fe_bpu_s0_pc` |
| uBTB | `fe_bpu_ubtb_` | `fe_bpu_ubtb_hit` |
| Backend | `be_` | `be_rob_commit_valid` |
| MemBlock | `mem_` | `mem_ldu0_addr` |

规则：**从根到叶逐级追加缩写**，保证全局唯一。

### 1.4 层次化 Verilog 生成策略

每个 `build_*` 函数仍可**独立编译**为一个扁平 Verilog 模块（用于单元测试和独立综合）。同时，当父函数调用子函数后，`build_verilog.py --hierarchical` 生成 wrapper Verilog 文件，使最终 RTL 保持层次化：

```
xs_top.v              ← build_xs_top 独立编译
xs_top_hier.v         ← wrapper: 例化 xs_tile, plic, clint
xs_core_hier.v        ← wrapper: 例化 frontend, backend, memblock
frontend_hier.v       ← wrapper: 例化 bpu, ftq, icache, ifu, ibuffer, decode
...
```

---

## 二、完整模块层次树

```
build_xs_top                          ← SoC 顶层
├── build_xs_tile                     ← 单核 Tile（含 Core + L2）
│   ├── build_xs_core                 ← 处理器核心
│   │   ├── build_frontend            ← 前端子系统
│   │   │   ├── build_bpu             ← 分支预测单元
│   │   │   │   ├── build_ubtb        ← micro BTB
│   │   │   │   ├── build_tage        ← TAGE 预测器
│   │   │   │   ├── build_sc          ← Statistical Corrector
│   │   │   │   ├── build_ittage      ← 间接跳转 TAGE
│   │   │   │   └── build_ras         ← Return Address Stack
│   │   │   ├── build_ftq             ← Fetch Target Queue
│   │   │   ├── build_icache          ← 指令缓存
│   │   │   ├── build_ifu             ← 指令获取单元
│   │   │   ├── build_ibuffer         ← 指令缓冲区
│   │   │   └── build_decode          ← 译码器
│   │   ├── build_backend             ← 后端子系统
│   │   │   ├── build_ctrlblock       ← 控制块
│   │   │   │   ├── build_rename      ← 寄存器重命名
│   │   │   │   ├── build_dispatch    ← 分派
│   │   │   │   └── build_rob         ← 重排序缓冲区
│   │   │   ├── build_issue_queue     ← 发射队列（×N 实例）
│   │   │   ├── build_regfile         ← 物理寄存器文件
│   │   │   ├── build_alu             ← ALU
│   │   │   ├── build_bru             ← 分支执行单元
│   │   │   ├── build_mul             ← 乘法器
│   │   │   ├── build_div             ← 除法器
│   │   │   └── build_fpu             ← 浮点单元
│   │   └── build_memblock            ← 访存子系统
│   │       ├── build_load_unit       ← 加载单元（×N 实例）
│   │       ├── build_store_unit      ← 存储单元（×N 实例）
│   │       ├── build_load_queue      ← 加载队列
│   │       ├── build_store_queue     ← 存储队列
│   │       ├── build_sbuffer         ← Store Buffer
│   │       ├── build_prefetcher      ← 预取器
│   │       ├── build_dcache          ← 数据缓存
│   │       └── build_tlb             ← TLB
│   ├── build_l2_top                  ← L2 顶层壳
│   └── build_coupled_l2             ← CoupledL2 实现
├── build_plic                        ← 中断控制器
└── build_clint                       ← 定时器/软中断
```

---

## 三、逐模块重构方案

### 3.1 SoC 顶层

#### `build_xs_top` — SoC 顶层

**文件**：`top/xs_top.py`
**调用**：`build_xs_tile()` × N, `build_plic()`, `build_clint()`
**周期策略**：各子系统 `push()/pop()` 隔离

```python
def build_xs_top(m, domain, *, num_cores=2, prefix="soc", ...):
    # ── 全局输入：AXI 主端口、外部中断 ──
    axi_arready = cas(domain, m.input(f"{prefix}_axi_arready", width=1), cycle=0)
    # ...

    # ── 每核 Tile ──
    for i in range(num_cores):
        tile_prefix = f"{prefix}_tile{i}"
        domain.push()
        build_xs_tile(m, domain, prefix=tile_prefix, hart_id=i, ...)
        domain.pop()

    # ── 外设 ──
    domain.push()
    build_plic(m, domain, prefix=f"{prefix}_plic", ...)
    domain.pop()

    domain.push()
    build_clint(m, domain, prefix=f"{prefix}_clint", ...)
    domain.pop()

    # ── 顶层互连：AXI 仲裁、中断路由 ──
    # ...
```

**重构要点**：
- 当前 `build_xs_top` 内联了多 tile 仲裁和 AXI 端口逻辑，无子模块调用
- 重构后：内联仲裁逻辑保留，tile/外设改为函数调用
- 每个 tile 用不同 prefix (`soc_tile0_`, `soc_tile1_`) 隔离命名

---

#### `build_plic` — PLIC 中断控制器

**文件**：`top/peripherals.py`
**调用**：无（叶子模块）
**变更**：仅添加 `prefix` 参数

```python
def build_plic(m, domain, *, prefix="plic", num_sources=64, num_targets=4, ...):
    # 当前逻辑不变，所有 m.input/m.output 名称加 prefix
```

---

#### `build_clint` — CLINT 定时器

**文件**：`top/peripherals.py`
**调用**：无（叶子模块）
**变更**：仅添加 `prefix` 参数

---

### 3.2 Tile 层

#### `build_xs_tile` — 单核 Tile

**文件**：`top/xs_tile.py`
**调用**：`build_xs_core()`, `build_l2_top()`, `build_coupled_l2()`

```python
def build_xs_tile(m, domain, *, prefix="tile", hart_id=0, ...):
    # ── Core ──
    domain.push()
    build_xs_core(m, domain, prefix=f"{prefix}_core", ...)
    domain.pop()

    # ── L2 Cache ──
    domain.push()
    build_l2_top(m, domain, prefix=f"{prefix}_l2top", ...)
    domain.pop()

    domain.push()
    build_coupled_l2(m, domain, prefix=f"{prefix}_l2", ...)
    domain.pop()

    # ── 互连：Core ↔ L2、L2 ↔ 外部 ──
    # Core 的 ICache miss/DCache miss → L2 请求
    # L2 的 refill → Core 的 data response
```

**重构要点**：
- 当前 `build_xs_tile` 内联了 Core+L2 的简化逻辑
- 重构后：抽取 Core 和 L2 为子函数调用，本层保留互连逻辑

---

#### `build_xs_core` — 处理器核心

**文件**：`top/xs_core.py`
**调用**：`build_frontend()`, `build_backend()`, `build_memblock()`

```python
def build_xs_core(m, domain, *, prefix="core",
                  decode_width=6, commit_width=8, ...):
    # ── 外部输入 ──
    l2_refill_valid = cas(domain, m.input(f"{prefix}_l2_refill_valid", width=1), cycle=0)
    meip = cas(domain, m.input(f"{prefix}_meip", width=1), cycle=0)
    # ...

    # ── Frontend ──
    domain.push()
    fe_results = build_frontend(m, domain,
        prefix=f"{prefix}_fe",
        decode_width=decode_width, ...)
    domain.pop()

    # ── Backend ──
    domain.push()
    be_results = build_backend(m, domain,
        prefix=f"{prefix}_be",
        decode_width=decode_width, commit_width=commit_width, ...)
    domain.pop()

    # ── MemBlock ──
    domain.push()
    build_memblock(m, domain,
        prefix=f"{prefix}_mem", ...)
    domain.pop()

    # ── 跨子系统互连 ──
    # Frontend → Backend: decoded uops
    # Backend → Frontend: redirect
    # Backend → MemBlock: memory dispatch
    # MemBlock → Backend: load/store writeback
    # Frontend → L2: ICache miss
```

**重构要点**：
- 这是重构量最大的文件——当前 330 行内联逻辑需要拆分
- Frontend 的 fetch/decode pipeline（cycle 0-3）移入 `build_frontend`
- Backend 的 dispatch/issue/redirect 逻辑移入 `build_backend`
- MemBlock 的 load/store writeback 移入 `build_memblock`
- 本层保留跨子系统的互连线和全局输入/输出

---

### 3.3 Frontend 子系统

#### `build_frontend` — 前端顶层

**文件**：`frontend/frontend.py`
**调用**：`build_bpu()`, `build_ftq()`, `build_icache()`, `build_ifu()`, `build_ibuffer()`, `build_decode()`

```python
def build_frontend(m, domain, *, prefix="fe",
                   decode_width=6, pc_width=39, ...):
    """Frontend = BPU → FTQ → ICache → IFU → IBuffer → Decode"""

    # ── BPU: 分支预测 ──
    domain.push()
    bpu_pred_target, bpu_taken = build_bpu(m, domain,
        prefix=f"{prefix}_bpu", pc_width=pc_width, ...)
    domain.pop()

    # ── FTQ: 取指目标队列 ──
    domain.push()
    ftq_pc, ftq_valid = build_ftq(m, domain,
        prefix=f"{prefix}_ftq", pc_width=pc_width, ...)
    domain.pop()

    # ── ICache ──
    domain.push()
    icache_data, icache_hit = build_icache(m, domain,
        prefix=f"{prefix}_ic", pc_width=pc_width, ...)
    domain.pop()

    # ── IFU: 指令获取 ──
    domain.push()
    ifu_insts = build_ifu(m, domain,
        prefix=f"{prefix}_ifu", pc_width=pc_width, ...)
    domain.pop()

    # ── IBuffer: 指令缓冲 ──
    domain.push()
    ibuf_out = build_ibuffer(m, domain,
        prefix=f"{prefix}_ibuf", ...)
    domain.pop()

    # ── Decode ──
    domain.push()
    dec_uops = build_decode(m, domain,
        prefix=f"{prefix}_dec", decode_width=decode_width, ...)
    domain.pop()

    # ── 子模块间互连 ──
    # BPU → FTQ: prediction entry
    # FTQ → ICache + IFU: fetch request (PC, valid)
    # ICache → IFU: cache data
    # IFU → IBuffer: extracted instructions
    # IBuffer → Decode: buffered instructions
    # Backend redirect → BPU/FTQ: flush + new PC

    return dec_uops  # 传递给 Backend
```

**重构要点**：
- 当前 `build_frontend` 约 200 行内联 4 级流水线
- 拆分后每个子模块独立管理自己的流水线周期（`push()/pop()` 隔离）
- 本层负责连接 BPU→FTQ→ICache→IFU→IBuffer→Decode 的数据通路

---

#### `build_bpu` — 分支预测单元

**文件**：`frontend/bpu/bpu.py`
**调用**：`build_ubtb()`, `build_tage()`, `build_sc()`, `build_ittage()`, `build_ras()`

```python
def build_bpu(m, domain, *, prefix="bpu", pc_width=39, ...):
    """BPU 4-stage pipeline: s0(generate) → s1(check) → s2(redirect) → s3(update)"""

    domain.push()

    # ── Stage 0: 快速预测 (uBTB) ──
    ubtb_hit, ubtb_target = build_ubtb(m, domain,
        prefix=f"{prefix}_ubtb", pc_width=pc_width, ...)

    domain.next()

    # ── Stage 1: 精确预测 (TAGE + SC) ──
    tage_taken = build_tage(m, domain,
        prefix=f"{prefix}_tage", pc_width=pc_width, ...)
    sc_correction = build_sc(m, domain,
        prefix=f"{prefix}_sc", ...)

    domain.next()

    # ── Stage 2: 间接跳转 (ITTAGE) + RAS ──
    ittage_target = build_ittage(m, domain,
        prefix=f"{prefix}_itt", pc_width=pc_width, ...)
    ras_target = build_ras(m, domain,
        prefix=f"{prefix}_ras", pc_width=pc_width, ...)

    domain.next()

    # ── Stage 3: 预测合并 + 输出 ──
    # 合并 uBTB / TAGE+SC / ITTAGE / RAS 的结果
    final_target = mux(...)
    m.output(f"{prefix}_pred_target", final_target)
    m.output(f"{prefix}_pred_taken", ...)

    domain.pop()
    return final_target, ...
```

**重构要点**：
- 当前 `build_bpu` 约 620 行，内联了全部 4 级流水线和 5 个预测器
- 5 个子预测器各有独立逻辑，适合拆分为子函数
- BPU 内部使用**周期延续**模式（`domain.next()` 在子函数间推进），而非 `push()/pop()` 隔离
- 子预测器是同一周期的并行逻辑（如 s1 阶段 TAGE 和 SC 并行），在同一周期内依次调用即可

---

#### `build_ubtb` — micro BTB（叶子）

**文件**：`frontend/bpu/ubtb.py`
**调用**：无
**变更**：添加 `prefix` 参数，内部逻辑不变

---

#### `build_tage` — TAGE 预测器（叶子）

**文件**：`frontend/bpu/tage.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_sc` — Statistical Corrector（叶子）

**文件**：`frontend/bpu/sc.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_ittage` — 间接跳转 TAGE（叶子）

**文件**：`frontend/bpu/ittage.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_ras` — Return Address Stack（叶子）

**文件**：`frontend/bpu/ras.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_ftq` — Fetch Target Queue（叶子）

**文件**：`frontend/ftq/ftq.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_icache` — 指令缓存（叶子）

**文件**：`frontend/icache/icache.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_ifu` — 指令获取单元（叶子）

**文件**：`frontend/ifu/ifu.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_ibuffer` — 指令缓冲区（叶子）

**文件**：`frontend/ibuffer/ibuffer.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_decode` — 译码器（叶子）

**文件**：`frontend/decode/decode.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

### 3.4 Backend 子系统

#### `build_backend` — 后端顶层

**文件**：`backend/backend.py`
**调用**：`build_ctrlblock()`, `build_issue_queue()` ×N, `build_regfile()`, `build_alu()`, `build_bru()`, `build_mul()`, `build_div()`, `build_fpu()`

```python
def build_backend(m, domain, *, prefix="be",
                  decode_width=6, commit_width=8,
                  num_int_exu=4, num_fp_exu=2, ...):
    """Backend = CtrlBlock + IssueQueues + RegFile + ExeUnits"""

    # ── CtrlBlock: Rename + Dispatch + ROB ──
    domain.push()
    ctrl_results = build_ctrlblock(m, domain,
        prefix=f"{prefix}_ctrl",
        decode_width=decode_width, commit_width=commit_width, ...)
    domain.pop()

    # ── 发射队列（按功能单元类型分组） ──
    # Integer issue queue
    domain.push()
    build_issue_queue(m, domain, prefix=f"{prefix}_iq_int", ...)
    domain.pop()

    # FP issue queue
    domain.push()
    build_issue_queue(m, domain, prefix=f"{prefix}_iq_fp", ...)
    domain.pop()

    # Memory issue queue
    domain.push()
    build_issue_queue(m, domain, prefix=f"{prefix}_iq_mem", ...)
    domain.pop()

    # ── 物理寄存器文件 ──
    domain.push()
    build_regfile(m, domain, prefix=f"{prefix}_irf", ...)  # 整数
    domain.pop()

    domain.push()
    build_regfile(m, domain, prefix=f"{prefix}_frf", ...)  # 浮点
    domain.pop()

    # ── 执行单元 ──
    for i in range(num_int_exu):
        domain.push()
        build_alu(m, domain, prefix=f"{prefix}_alu{i}", ...)
        domain.pop()

    domain.push()
    build_bru(m, domain, prefix=f"{prefix}_bru", ...)
    domain.pop()

    domain.push()
    build_mul(m, domain, prefix=f"{prefix}_mul", ...)
    domain.pop()

    domain.push()
    build_div(m, domain, prefix=f"{prefix}_div", ...)
    domain.pop()

    for i in range(num_fp_exu):
        domain.push()
        build_fpu(m, domain, prefix=f"{prefix}_fpu{i}", ...)
        domain.pop()

    # ── 互连 ──
    # CtrlBlock → IssueQueue: dispatched uops
    # IssueQueue → RegFile: read requests
    # RegFile → ExeUnits: operand data
    # ExeUnits → ROB: writeback
    # ROB → CtrlBlock: commit / redirect
```

**重构要点**：
- 当前 `build_backend` 约 750 行内联逻辑
- CtrlBlock 本身也是组合模块，内部调用 rename/dispatch/rob
- 发射队列和执行单元有多个实例，通过循环调用并改变 prefix 来区分
- 本层的核心工作是连接 CtrlBlock → IQ → RegFile → ExeUnit → Writeback 的数据通路

---

#### `build_ctrlblock` — 控制块

**文件**：`backend/ctrlblock/ctrlblock.py`
**调用**：`build_rename()`, `build_dispatch()`, `build_rob()`

```python
def build_ctrlblock(m, domain, *, prefix="ctrl",
                    decode_width=6, commit_width=8, ...):
    """CtrlBlock = Rename → Dispatch → ROB"""

    # ── Rename: 逻辑→物理寄存器映射 ──
    domain.push()
    rename_results = build_rename(m, domain,
        prefix=f"{prefix}_ren", rename_width=decode_width, ...)
    domain.pop()

    # ── Dispatch: 分派到各发射队列 ──
    domain.push()
    build_dispatch(m, domain,
        prefix=f"{prefix}_dp", dispatch_width=decode_width, ...)
    domain.pop()

    # ── ROB: 顺序提交 ──
    domain.push()
    build_rob(m, domain,
        prefix=f"{prefix}_rob", commit_width=commit_width, ...)
    domain.pop()

    # ── 互连 ──
    # Rename → Dispatch: renamed uops (physical reg tags)
    # Dispatch → IssueQueues (via backend outputs)
    # ROB commit → Rename (free physical regs)
    # ROB redirect → Frontend (flush)
```

---

#### `build_rename` — 寄存器重命名（叶子）

**文件**：`backend/rename/rename.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_dispatch` — 分派（叶子）

**文件**：`backend/dispatch/dispatch.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_rob` — 重排序缓冲区（叶子）

**文件**：`backend/rob/rob.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_issue_queue` — 发射队列（叶子，多实例）

**文件**：`backend/issue/issue_queue.py`
**调用**：无
**变更**：添加 `prefix` 参数。通过不同 prefix（`be_iq_int`, `be_iq_fp`, `be_iq_mem`）区分多个实例

---

#### `build_regfile` — 寄存器文件（叶子，多实例）

**文件**：`backend/regfile/regfile.py`
**调用**：无
**变更**：添加 `prefix` 参数。通过 `be_irf`（整数）/ `be_frf`（浮点）区分

---

#### `build_alu` — ALU（叶子）

**文件**：`backend/fu/alu.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_bru` — 分支执行单元（叶子）

**文件**：`backend/fu/bru.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_mul` — 乘法器（叶子）

**文件**：`backend/fu/mul.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_div` — 除法器（叶子）

**文件**：`backend/fu/div.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_fpu` — 浮点单元（叶子）

**文件**：`backend/fu/fpu.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

### 3.5 MemBlock 子系统

#### `build_memblock` — 访存顶层

**文件**：`mem/memblock.py`
**调用**：`build_load_unit()` ×N, `build_store_unit()` ×N, `build_load_queue()`, `build_store_queue()`, `build_sbuffer()`, `build_prefetcher()`, `build_dcache()`, `build_tlb()`

```python
def build_memblock(m, domain, *, prefix="mem",
                   num_load=2, num_store=2, ...):
    """MemBlock = LoadUnits + StoreUnits + LSQ + SBuffer + DCache + TLB"""

    # ── TLB ──
    domain.push()
    build_tlb(m, domain, prefix=f"{prefix}_dtlb", ...)
    domain.pop()

    # ── DCache ──
    domain.push()
    build_dcache(m, domain, prefix=f"{prefix}_dc", ...)
    domain.pop()

    # ── Load Units ──
    for i in range(num_load):
        domain.push()
        build_load_unit(m, domain, prefix=f"{prefix}_ldu{i}", ...)
        domain.pop()

    # ── Store Units ──
    for i in range(num_store):
        domain.push()
        build_store_unit(m, domain, prefix=f"{prefix}_stu{i}", ...)
        domain.pop()

    # ── Load Queue ──
    domain.push()
    build_load_queue(m, domain, prefix=f"{prefix}_ldq", ...)
    domain.pop()

    # ── Store Queue ──
    domain.push()
    build_store_queue(m, domain, prefix=f"{prefix}_stq", ...)
    domain.pop()

    # ── Store Buffer ──
    domain.push()
    build_sbuffer(m, domain, prefix=f"{prefix}_sbuf", ...)
    domain.pop()

    # ── Prefetcher ──
    domain.push()
    build_prefetcher(m, domain, prefix=f"{prefix}_pf", ...)
    domain.pop()

    # ── 互连 ──
    # LoadUnit → TLB: 地址翻译请求
    # TLB → LoadUnit: 物理地址
    # LoadUnit → DCache: 缓存读请求
    # DCache → LoadUnit: 读数据
    # StoreUnit → StoreQueue: 入队
    # StoreQueue → SBuffer: 提交写
    # SBuffer → DCache: 写请求
    # Prefetcher → DCache: 预取请求
```

---

#### `build_load_unit` — 加载单元（叶子）

**文件**：`mem/pipeline/load_unit.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_store_unit` — 存储单元（叶子）

**文件**：`mem/pipeline/store_unit.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_load_queue` — 加载队列（叶子）

**文件**：`mem/lsqueue/load_queue.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_store_queue` — 存储队列（叶子）

**文件**：`mem/lsqueue/store_queue.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_sbuffer` — Store Buffer（叶子）

**文件**：`mem/sbuffer/sbuffer.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_prefetcher` — 预取器（叶子）

**文件**：`mem/prefetch/prefetcher.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_dcache` — 数据缓存（叶子）

**文件**：`cache/dcache/dcache.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_tlb` — TLB（叶子）

**文件**：`cache/mmu/tlb.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

### 3.6 L2 Cache

#### `build_l2_top` — L2 顶层壳（叶子）

**文件**：`l2/l2_top.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

#### `build_coupled_l2` — CoupledL2 实现（叶子）

**文件**：`l2/coupled_l2.py`
**调用**：无
**变更**：添加 `prefix` 参数

---

## 四、重构实施步骤

### Phase 0：添加 prefix 支持（所有叶子模块）

**目标**：为全部 40 个 `build_*` 函数添加 `prefix` 关键字参数，将所有 `m.input()`、`m.output()`、`domain.signal()`、`domain.cycle()` 的名称改为 `f"{prefix}_..."` 格式。

**工作量**：每个文件约 30 分钟，共约 20 小时
**验证**：所有 65 个 pytest 测试通过；每个模块可独立 `compile_cycle_aware` 编译

| 批次 | 模块 | 文件数 |
|------|------|--------|
| P0-A | 叶子执行单元：alu, bru, mul, div, fpu | 5 |
| P0-B | 后端结构：regfile, rename, dispatch, issue_queue, rob | 5 |
| P0-C | 前端叶子：ubtb, tage, sc, ittage, ras, ftq | 6 |
| P0-D | 前端结构：icache, ifu, ibuffer, decode | 4 |
| P0-E | 访存叶子：load_unit, store_unit, load_queue, store_queue, sbuffer, prefetcher | 6 |
| P0-F | Cache/MMU：dcache, tlb | 2 |
| P0-G | L2/外设：l2_top, coupled_l2, plic, clint | 4 |

**prefix 默认值规则**：每个函数的 `prefix` 默认值为当前模块名缩写（如 `build_alu` 的 prefix 默认为 `"alu"`），确保独立编译时行为不变。

---

### Phase 1：中间层组合（子系统顶层）

**目标**：让 `build_bpu`、`build_ctrlblock`、`build_frontend`、`build_backend`、`build_memblock` 调用各自的子模块。

| 步骤 | 文件 | 调用的子模块 | 变更说明 |
|------|------|------------|---------|
| P1-1 | `frontend/bpu/bpu.py` | ubtb, tage, sc, ittage, ras | 当前 620 行内联预测逻辑；拆为 5 个子函数调用 + 本层的预测合并逻辑 |
| P1-2 | `backend/ctrlblock/ctrlblock.py` | rename, dispatch, rob | 当前约 200 行；拆为 3 个子函数调用 + 互连 |
| P1-3 | `frontend/frontend.py` | bpu, ftq, icache, ifu, ibuffer, decode | 当前约 200 行 4 级流水线；拆为 6 个子函数调用 + 数据通路互连 |
| P1-4 | `backend/backend.py` | ctrlblock, issue_queue×3, regfile×2, alu×N, bru, mul, div, fpu×N | 当前约 750 行；拆分量最大 |
| P1-5 | `mem/memblock.py` | load_unit×N, store_unit×N, load_queue, store_queue, sbuffer, prefetcher, dcache, tlb | 当前约 400 行 |

**验证**：
- 每完成一个中间层，独立编译该模块验证 MLIR 输出
- 子模块也仍可独立编译（prefix 默认值保证兼容）

---

### Phase 2：顶层组合

| 步骤 | 文件 | 调用的子模块 |
|------|------|------------|
| P2-1 | `top/xs_core.py` | frontend, backend, memblock |
| P2-2 | `top/xs_tile.py` | xs_core, l2_top, coupled_l2 |
| P2-3 | `top/xs_top.py` | xs_tile×N, plic, clint |

**验证**：
- `build_xs_top` 独立编译输出完整 MLIR
- `build_verilog.py --hierarchical` 生成层次化 Verilog

---

### Phase 3：build 脚本升级

更新 `build_verilog.py`：

1. **双模式编译**：
   - `--flat`（默认）：每个模块独立编译为 Verilog（当前行为不变）
   - `--hierarchical`：编译顶层模块（`build_xs_top`），子函数调用展开后的完整信号图生成单个大 Verilog，同时通过 wrapper 拆分为层次化 Verilog 文件

2. **更新 `HIERARCHY_TREE`**：确保与新的函数调用层次一致

3. **增加 `--compose` 模式**（可选增强）：编译 `build_xs_core` 等中间层模块，验证子函数组合后的 MLIR 正确性

---

## 五、信号传递模式详解

### 5.1 隐式传递（通过 `m.input()`/`m.output()`）

子模块通过 `m.output(f"{prefix}_signal", wire)` 输出信号，父模块或兄弟模块通过 `m.input(f"{other_prefix}_signal", width=W)` 读取：

```python
# 在 build_frontend 内:
def build_ifu(m, domain, *, prefix="ifu", ...):
    # 从 ICache 读取响应（ICache 通过 m.output 输出）
    icache_data = cas(domain, m.input(f"{prefix}_icache_data", width=512), cycle=0)
    # ... IFU 逻辑 ...
    m.output(f"{prefix}_insts", extracted_insts)

# 在 build_frontend 互连中:
# 将 icache 的输出连接到 ifu 的输入（都在同一个 m 上）
```

**适用场景**：模块间的接口信号、需要在 Verilog 端口中可见的信号

### 5.2 显式传递（通过 Python 返回值）

子函数返回 Wire 对象，父函数直接使用：

```python
def build_bpu(m, domain, *, prefix="bpu", ...):
    # ...
    return pred_target, pred_taken  # Wire 对象

def build_frontend(m, domain, ...):
    target, taken = build_bpu(m, domain, prefix=f"{prefix}_bpu", ...)
    # 直接使用 Wire 对象，无需 m.input/m.output 中转
    ftq_entry_target = target
```

**适用场景**：紧耦合的父子信号、不需要暴露为端口的内部连线

### 5.3 混合模式（推荐）

实际中两种方式混用：

```python
def build_rename(m, domain, *, prefix="ren", ...):
    # 输入：从外部接收
    dec_valid = cas(domain, m.input(f"{prefix}_dec_valid", width=1), cycle=0)
    # ...
    # 输出方式 1：通过 m.output（对外接口）
    m.output(f"{prefix}_alloc_valid", alloc_valid)
    # 输出方式 2：通过返回值（给父函数的内部互连）
    return alloc_ptag, alloc_valid
```

---

## 六、双模式兼容设计

每个 `build_*` 函数保持**独立可编译**：

```python
# 作为叶子模块独立编译
if __name__ == "__main__":
    ir = compile_cycle_aware(build_alu, name="alu", eager=True,
                             data_width=16, prefix="alu")
    print(ir.emit_mlir())

# 作为子模块被父函数调用
def build_backend(m, domain, ...):
    build_alu(m, domain, prefix=f"{prefix}_alu0", data_width=data_width)
```

`prefix` 参数的默认值确保独立编译时信号名合理，被调用时由父函数指定更具体的前缀。

---

## 七、测试策略

### 7.1 单元测试（不变）

每个叶子模块的现有 pytest 测试不受影响（prefix 默认值保持兼容）。

### 7.2 组合测试（新增）

为每个中间层模块新增组合编译测试：

```python
# tests/test_compose.py
def test_frontend_compose():
    """验证 build_frontend 调用所有子模块后的 MLIR 编译"""
    ir = compile_cycle_aware(build_frontend, name="frontend_composed",
                             eager=True, prefix="fe", decode_width=2, ...)
    mlir = ir.emit_mlir()
    assert "fe_bpu_ubtb_" in mlir  # BPU 子模块的信号存在
    assert "fe_dec_" in mlir       # Decode 子模块的信号存在

def test_xs_core_compose():
    """验证 build_xs_core 调用 frontend + backend + memblock"""
    ir = compile_cycle_aware(build_xs_core, name="xs_core_composed",
                             eager=True, prefix="core", ...)
    mlir = ir.emit_mlir()
    assert "core_fe_" in mlir
    assert "core_be_" in mlir
    assert "core_mem_" in mlir
```

### 7.3 层次化 Verilog 测试

```bash
# 编译完整层次化 Verilog
python build_verilog.py --hierarchical --small

# 验证生成的 wrapper 文件
ls build_out_small/verilog_hier/*_hier.v
# 应包含：xs_top_hier.v, xs_tile_hier.v, xs_core_hier.v,
#          frontend_hier.v, backend_hier.v, memblock_hier.v, bpu_hier.v
```

---

## 八、模块清单总表

| # | 模块 | 层级 | 父模块 | 子模块数 | 文件 |
|---|------|------|--------|---------|------|
| 1 | `build_xs_top` | L0 | — | 3 | `top/xs_top.py` |
| 2 | `build_xs_tile` | L1 | xs_top | 3 | `top/xs_tile.py` |
| 3 | `build_xs_core` | L2 | xs_tile | 3 | `top/xs_core.py` |
| 4 | `build_frontend` | L3 | xs_core | 6 | `frontend/frontend.py` |
| 5 | `build_bpu` | L4 | frontend | 5 | `frontend/bpu/bpu.py` |
| 6 | `build_ubtb` | L5 | bpu | 0 | `frontend/bpu/ubtb.py` |
| 7 | `build_tage` | L5 | bpu | 0 | `frontend/bpu/tage.py` |
| 8 | `build_sc` | L5 | bpu | 0 | `frontend/bpu/sc.py` |
| 9 | `build_ittage` | L5 | bpu | 0 | `frontend/bpu/ittage.py` |
| 10 | `build_ras` | L5 | bpu | 0 | `frontend/bpu/ras.py` |
| 11 | `build_ftq` | L4 | frontend | 0 | `frontend/ftq/ftq.py` |
| 12 | `build_icache` | L4 | frontend | 0 | `frontend/icache/icache.py` |
| 13 | `build_ifu` | L4 | frontend | 0 | `frontend/ifu/ifu.py` |
| 14 | `build_ibuffer` | L4 | frontend | 0 | `frontend/ibuffer/ibuffer.py` |
| 15 | `build_decode` | L4 | frontend | 0 | `frontend/decode/decode.py` |
| 16 | `build_backend` | L3 | xs_core | 11+ | `backend/backend.py` |
| 17 | `build_ctrlblock` | L4 | backend | 3 | `backend/ctrlblock/ctrlblock.py` |
| 18 | `build_rename` | L5 | ctrlblock | 0 | `backend/rename/rename.py` |
| 19 | `build_dispatch` | L5 | ctrlblock | 0 | `backend/dispatch/dispatch.py` |
| 20 | `build_rob` | L5 | ctrlblock | 0 | `backend/rob/rob.py` |
| 21 | `build_issue_queue` | L4 | backend | 0 | `backend/issue/issue_queue.py` |
| 22 | `build_regfile` | L4 | backend | 0 | `backend/regfile/regfile.py` |
| 23 | `build_alu` | L4 | backend | 0 | `backend/fu/alu.py` |
| 24 | `build_bru` | L4 | backend | 0 | `backend/fu/bru.py` |
| 25 | `build_mul` | L4 | backend | 0 | `backend/fu/mul.py` |
| 26 | `build_div` | L4 | backend | 0 | `backend/fu/div.py` |
| 27 | `build_fpu` | L4 | backend | 0 | `backend/fu/fpu.py` |
| 28 | `build_memblock` | L3 | xs_core | 8+ | `mem/memblock.py` |
| 29 | `build_load_unit` | L4 | memblock | 0 | `mem/pipeline/load_unit.py` |
| 30 | `build_store_unit` | L4 | memblock | 0 | `mem/pipeline/store_unit.py` |
| 31 | `build_load_queue` | L4 | memblock | 0 | `mem/lsqueue/load_queue.py` |
| 32 | `build_store_queue` | L4 | memblock | 0 | `mem/lsqueue/store_queue.py` |
| 33 | `build_sbuffer` | L4 | memblock | 0 | `mem/sbuffer/sbuffer.py` |
| 34 | `build_prefetcher` | L4 | memblock | 0 | `mem/prefetch/prefetcher.py` |
| 35 | `build_dcache` | L4 | memblock | 0 | `cache/dcache/dcache.py` |
| 36 | `build_tlb` | L4 | memblock | 0 | `cache/mmu/tlb.py` |
| 37 | `build_l2_top` | L2 | xs_tile | 0 | `l2/l2_top.py` |
| 38 | `build_coupled_l2` | L2 | xs_tile | 0 | `l2/coupled_l2.py` |
| 39 | `build_plic` | L1 | xs_top | 0 | `top/peripherals.py` |
| 40 | `build_clint` | L1 | xs_top | 0 | `top/peripherals.py` |

**统计：**
- 总模块数：40
- 叶子模块（L4/L5，无子调用）：28
- 中间层（L2-L4，有子调用）：9（xs_core, frontend, bpu, backend, ctrlblock, memblock, xs_tile, l2_top区域, xs_top）
- 最大调用深度：6 层（xs_top → xs_tile → xs_core → frontend → bpu → ubtb）

---

## 九、HIERARCHY_TREE 更新

重构后 `build_verilog.py` 中的 `HIERARCHY_TREE` 应与函数调用层次完全一致：

```python
HIERARCHY_TREE: dict[str, list[str]] = {
    "xs_top":    ["xs_tile", "plic", "clint"],
    "xs_tile":   ["xs_core", "l2_top", "coupled_l2"],
    "xs_core":   ["frontend", "backend", "memblock"],
    "frontend":  ["bpu", "ftq", "icache", "ifu", "ibuffer", "decode"],
    "bpu":       ["ubtb", "tage", "sc", "ittage", "ras"],
    "backend":   ["ctrlblock", "issue_queue", "regfile",
                  "alu", "bru", "mul", "div", "fpu"],
    "ctrlblock": ["rename", "dispatch", "rob"],
    "memblock":  ["load_unit", "store_unit", "load_queue", "store_queue",
                  "sbuffer", "prefetcher", "dcache", "tlb"],
}
```

---

## 十、风险与缓解

| 风险 | 影响 | 缓解 |
|------|------|------|
| prefix 重构引入命名错误 | 编译失败或信号连接错误 | 每个模块改完立即运行 pytest；用 `emit_mlir()` 对比前后信号名 |
| 子函数组合后信号图过大 | `compile_cycle_aware` 编译耗时或内存不足 | 先用 `--small` 参数验证；大模块组合前做增量集成 |
| 跨模块互连逻辑复杂 | 中间层代码膨胀 | 把互连逻辑集中在父函数的明确标注区域（`# ── 互连 ──`） |
| domain.push/pop 不配对 | 运行时 RuntimeError | 代码审查 + 用 `try/finally` 包装关键路径 |
| 同一模块多实例的参数差异 | 如 ALU0 和 ALU1 配置不同时 | 通过循环 + 参数字典灵活控制 |

---

**Copyright (C) 2024–2026 PyCircuit Contributors**
