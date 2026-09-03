# XiangShan-pyc 完整验证计划

**目的：** 借鉴 XiangShan 原始仓库的多层次验证方法论（DiffTest、Verilator 仿真、
riscv-tests、workload 回归、性能 Top-down 分析等），结合 PyCircuit 自身的 TB
框架能力（`@testbench`、`CycleAwareTb`、SVA、MLIR 门、Verilog/Verilator 仿真、
`pyctrace` 波形），为 XiangShan-pyc 项目制定从单元到系统的完整验证策略。

---

## 一、XiangShan 原始验证方法概览（参考基线）

| 层级 | 手段 | 工具 | 覆盖范围 |
|------|------|------|----------|
| **模块/Chisel 单元** | ScalaTest + ChiselSim (Verilator) | `mill xiangshan.test.test` | BPU 饱和计数器、WPU、Decode 等 |
| **全芯片 RTL 仿真** | Verilator (`make emu`) / VCS (`make simv`) | DiffTest + NEMU/Spike 参考模型 | 指令提交级逐拍对比 |
| **功能回归** | riscv-tests, cputest, misc-tests, rvh-tests | CI (`emu-basics.yml`) | ISA 合规、基础功能 |
| **OS/应用** | Linux, FreeRTOS, RT-Thread, coremark, microbench | CI + 手动 | 真实负载启动与运行 |
| **性能回归** | SPEC2006 checkpoint + SimPoint | nightly CI (`perf-template.yml`) | IPC 无回退 |
| **Top-down 分析** | 硬件 perf counter + `scripts/top-down/` | Python 脚本 | Frontend/Backend/Mem 瓶颈定位 |
| **调试辅助** | lightSSS (fork 快照)、ChiselDB (SQLite)、波形 VCD/FST | 按需 | 出错定位 |
| **DiffTest 加速** | Batch/Squash/NonBlock/Replay 模式 | FPGA/Palladium | 保持指令级精度、减通信开销 |
| **覆盖率** | FIRRTL cover + `coverage.py` | 后处理 | 行/功能覆盖率 |

---

## 二、XiangShan-pyc 验证分层架构

```
    ┌───────────────────────────────────────────────────┐
    │  L5: 系统测试 — 多核 SoC + OS 启动 + 真实负载     │
    ├───────────────────────────────────────────────────┤
    │  L4: 核级集成 — 取指→译码→执行→访存→提交          │
    ├───────────────────────────────────────────────────┤
    │  L3: 子系统集成 — Frontend / Backend / MemBlock   │
    ├───────────────────────────────────────────────────┤
    │  L2: 功能定向 — 模块级多周期激励/检查             │
    ├───────────────────────────────────────────────────┤
    │  L1: 编译门 — MLIR emit + Verilog 生成 + IR 合法  │
    ├───────────────────────────────────────────────────┤
    │  L0: 可导入性 — Python import + build_* 存在       │
    └───────────────────────────────────────────────────┘
```

**现状评估：** 当前 34 个 `tb_*.py` 文件全部停留在 **L0–L1** 层（import 检查 +
MLIR 冒烟 + 端口名字符串匹配），**L2–L5 层完全空白**。

---

## 三、分层验证详细计划

### L0: 可导入性门（现有 `test_xs_steps.py` 扩展）

**目标：** 确保所有模块可被 Python 导入、`build_*` 函数存在、参数可加载。

| 测试 ID | 检查项 | 工具 | 状态 |
|---------|--------|------|------|
| L0-001 | `top.parameters` 可导入，`XLEN==64` | pytest | 已有 |
| L0-002 | `lib.primitives` 所有 `build_*` 可调用 | pytest | 已有 |
| L0-003 | `lib.tilelink` 常量存在 | pytest | 已有 |
| L0-004 | `lib.axi` 端口函数存在 | pytest | 已有 |
| L0-005 | 所有 ~40 个模块 `build_*` 可导入 | pytest parametrize | 已有 |

**改进：** 增加 `phase5` marker 覆盖 L2/PLIC/CLINT。

### L1: 编译门（MLIR / Verilog / IR 合法性）

**目标：** 每个模块能编译到 MLIR，再到 Verilog，且满足 IR 合法性约束。

| 测试 ID | 检查项 | 工具 | 优先级 |
|---------|--------|------|--------|
| L1-001 | 每模块 `compile_cycle_aware(...).emit_mlir()` 成功 | pytest | P0 (已有) |
| L1-002 | MLIR 中含 `func.func @<name>` | assert 字符串 | P0 (已有) |
| L1-003 | MLIR 中含所有声明的端口名 | assert port in mlir | P0 (部分已有) |
| L1-004 | **`pyc.reg` 计数断言**：每模块的寄存器数 >= 预期下界 | 正则 count | P1 (新增) |
| L1-005 | **`pycc --emit=verilog`** 编译通过 | subprocess + returncode | P1 (新增) |
| L1-006 | **IR 合法性门**：`pyc-check-no-dynamic` 通过 | pycc pass | P2 (新增) |
| L1-007 | **组合环检测**：`pyc-check-comb-cycle` 通过 | pycc pass | P2 (新增) |
| L1-008 | **逻辑深度门**：关键路径 WNS 代理不超阈值 | pycc pass | P3 (新增) |

**实现方式：**

```python
# L1-004 示例：pyc.reg 计数断言
import re
def test_alu_reg_count():
    mlir = compile_cycle_aware(build_alu, name="alu", eager=True).emit_mlir()
    reg_count = len(re.findall(r'pyc\.reg', mlir))
    assert reg_count == 0  # ALU 是纯组合，不应有寄存器

def test_rob_reg_count():
    mlir = compile_cycle_aware(build_rob, name="rob", eager=True, rob_size=16, ...).emit_mlir()
    reg_count = len(re.findall(r'pyc\.reg', mlir))
    assert reg_count >= 16 * 5  # 至少 16 entries x 5 fields
```

### L2: 功能定向测试（**当前最大缺口**）

**目标：** 对每个模块验证核心功能行为的正确性。

#### L2-A: 叶子模块功能测试

使用 `@testbench` + `CycleAwareTb` 编写多周期激励-检查场景。

| 测试 ID | 模块 | 场景 | 检查方法 |
|---------|------|------|----------|
| L2-A-001 | ALU | ADD: 3+5=8, SUB: 10-3=7, AND/OR/XOR 位操作 | `tb.expect("result", expected)` |
| L2-A-002 | ALU | SLL/SRL/SRA 移位边界（shift 0, shift 63） | golden 值 |
| L2-A-003 | ALU | SLT/SLTU 有符号 vs 无符号比较 | golden 值 |
| L2-A-004 | MUL | 2 周期延迟验证：drive at cycle 0, expect at cycle 2 | `tb.expect` at cycle+2 |
| L2-A-005 | MUL | MULH/MULHU/MULHSU 高位乘积 | Python `(a*b)>>64` |
| L2-A-006 | DIV | FSM 延迟验证：in_valid → out_valid 经过 N 周期 | 计数器 |
| L2-A-007 | DIV | 除以零行为 | 检查输出值 |
| L2-A-008 | BRU | BEQ/BNE 所有分支类型 taken/not-taken | golden 值 |
| L2-A-009 | BRU | JAL/JALR 目标地址计算 + link_addr | golden 值 |
| L2-A-010 | BRU | mispredict 检测 | redirect_valid |
| L2-A-011 | RegFile | 写后读一致性（W→R same cycle bypass） | `tb.expect("rdata")` |
| L2-A-012 | RegFile | r0 硬连线为零 | 写 r0 后读仍为 0 |
| L2-A-013 | RegFile | 多端口同时写冲突（最后写者胜） | golden 值 |
| L2-A-014 | FPU | FADD 3 周期延迟 | cycle count |
| L2-A-015 | FPU | FDIV FSM 完整握手 | valid/ready 协议 |
| L2-A-016 | Primitives | mux1h one-hot 正确选择 | 遍历 one-hot |
| L2-A-017 | Primitives | popcount 0x00/0xFF/交替位模式 | golden 值 |
| L2-A-018 | Primitives | priority_enc 边界（全 0、仅 MSB、仅 LSB） | golden 值 |

#### L2-B: FIFO/队列模块功能测试

| 测试 ID | 模块 | 场景 | 检查方法 |
|---------|------|------|----------|
| L2-B-001 | IBuffer | 空→满→空 指针回绕 | num_valid 追踪 |
| L2-B-002 | IBuffer | 反压：满时 in_ready=0 | `tb.expect("in_ready", 0)` |
| L2-B-003 | IBuffer | flush：所有指针归零 | pointer reset |
| L2-B-004 | IBuffer | 数据完整性：enqueue N 条, dequeue 顺序一致 | inst 逐条比对 |
| L2-B-005 | FTQ | BPU 写入 → IFU 读出 一致 | target/taken 比对 |
| L2-B-006 | FTQ | redirect 回退指针正确 | ptr 值检查 |
| L2-B-007 | FTQ | commit 推进 + 空间释放 | bpu_in_ready 恢复 |
| L2-B-008 | ROB | enqueue→writeback→commit 完整流程 | commit_valid 序列 |
| L2-B-009 | ROB | 异常处理：exception 阻止后续 commit | redirect 输出 |
| L2-B-010 | ROB | flush 清空所有表项 | num_valid=0 |
| L2-B-011 | IssueQueue | enqueue→wakeup→select→issue 链路 | issue_valid + uop 比对 |
| L2-B-012 | IssueQueue | oldest-first 选择正确性 | age matrix 验证 |
| L2-B-013 | LoadQueue | enqueue→地址查找→commit 释放 | ptr 追踪 |
| L2-B-014 | StoreQueue | store-to-load 转发 | forward_data 比对 |
| L2-B-015 | SBuffer | 同 cache line 合并 | merged_valid 检查 |

#### L2-C: Cache/MMU 模块功能测试

| 测试 ID | 模块 | 场景 | 检查方法 |
|---------|------|------|----------|
| L2-C-001 | ICache | 冷启动 miss → refill → 再访问 hit | resp_hit 变化 |
| L2-C-002 | ICache | 多路替换 | 超过 n_ways 后 eviction |
| L2-C-003 | ICache | flush 后全 miss | resp_hit=0 |
| L2-C-004 | DCache | load hit/miss 路径 | load_resp_hit |
| L2-C-005 | DCache | store hit → 数据写入 → 再 load 读回 | 数据一致 |
| L2-C-006 | DCache | miss → MSHR → refill 完整链路 | miss_valid → refill → hit |
| L2-C-007 | TLB | lookup hit/miss | resp_hit/resp_miss |
| L2-C-008 | TLB | refill 新页表项 + 再查 hit | VPN→PPN 对应 |
| L2-C-009 | TLB | sfence 全局 flush | 全 miss |
| L2-C-010 | TLB | ASID 选择性 flush | 仅目标 ASID miss |

#### L2-D: 控制路径功能测试

| 测试 ID | 模块 | 场景 | 检查方法 |
|---------|------|------|----------|
| L2-D-001 | Rename | RAT 初始 identity 映射 | psrc == lsrc |
| L2-D-002 | Rename | 分配后 pdest != old_pdest | 比对 |
| L2-D-003 | Rename | WAW/WAR 依赖内组 bypass | 同组 rd==rs 的 psrc |
| L2-D-004 | Rename | redirect 快照恢复 | RAT 回到快照值 |
| L2-D-005 | Dispatch | fu_type 路由到正确 IQ | int/fp/mem 分类 |
| L2-D-006 | Dispatch | 反压传播 | stall 信号 |
| L2-D-007 | Decode | R/I/S/B/U/J 类型判别 | golden 指令集 |
| L2-D-008 | Decode | 立即数提取（含符号扩展） | golden 值 |

#### L2-E: 分支预测器功能测试

| 测试 ID | 模块 | 场景 | 检查方法 |
|---------|------|------|----------|
| L2-E-001 | uBTB | 训练后命中 | pred_valid + target 正确 |
| L2-E-002 | uBTB | 冷启动全 miss | pred_valid=0 |
| L2-E-003 | TAGE | 多表命中选最长历史 | provider_idx |
| L2-E-004 | TAGE | 计数器饱和行为 | 反复训练检查边界 |
| L2-E-005 | RAS | call push / ret pop 对称 | 栈顶值 |
| L2-E-006 | RAS | 溢出回绕 | 指针 wrap |
| L2-E-007 | RAS | redirect 恢复 | spec 栈回退到 commit |
| L2-E-008 | SC | 翻转 TAGE 预测 | 当 sum > threshold |
| L2-E-009 | ITTAGE | 间接跳转目标预测 | target 匹配 |

**L2 测试框架示例：**

```python
@testbench
def tb_alu_add(t: Tb):
    tb = CycleAwareTb(t)
    tb.clock("clock")
    tb.reset("reset")
    tb.timeout(20)

    # Cycle 3: drive ADD operation
    tb.next(); tb.next(); tb.next()
    tb.drive("src1", 0x0000_0000_0000_0003)
    tb.drive("src2", 0x0000_0000_0000_0005)
    tb.drive("alu_op", 0)  # ADD
    tb.drive("in_valid", 1)

    # Cycle 3: expect result (combinational)
    tb.expect("result", 0x0000_0000_0000_0008, phase="pre")

    tb.next()
    tb.drive("in_valid", 0)
    tb.finish()
```

### L3: 子系统集成测试

**目标：** 验证 Frontend / Backend / MemBlock 各自内部模块协同工作。

| 测试 ID | 子系统 | 场景 | 长度 |
|---------|--------|------|------|
| L3-001 | Frontend | BPU 预测 → FTQ 入队 → IFU 取指 → ICache hit → IBuffer 入队 → Decode 输出 | ~20 cycles |
| L3-002 | Frontend | ICache miss → refill → 再取指成功 | ~30 cycles |
| L3-003 | Frontend | 分支误预测 → redirect → 流水线 flush → 重新取指 | ~15 cycles |
| L3-004 | Backend | Rename → Dispatch → IssueQueue → ALU → Writeback → ROB commit | ~15 cycles |
| L3-005 | Backend | RAW 依赖：第二条指令等待第一条 writeback 后才能 issue | ~10 cycles |
| L3-006 | Backend | ROB 异常：异常指令到达 head → redirect | ~10 cycles |
| L3-007 | MemBlock | Load: addr gen → TLB hit → DCache hit → data return | ~8 cycles |
| L3-008 | MemBlock | Load: DCache miss → MSHR → refill → data return | ~20 cycles |
| L3-009 | MemBlock | Store: addr gen → TLB → StoreQueue → commit → SBuffer → DCache write | ~15 cycles |
| L3-010 | MemBlock | Store-to-Load forwarding: store 后 load 同地址获得转发数据 | ~10 cycles |

### L4: 核级端到端测试

**目标：** 在 XSCore 层面验证完整的指令执行流程。

| 测试 ID | 场景 | 指令序列 | 验证点 |
|---------|------|----------|--------|
| L4-001 | 单条 ADD | `add x1, x2, x3` | commit_valid, 结果正确 |
| L4-002 | Load-Store | `sw x1, 0(x2); lw x3, 0(x2)` | x3 == x1 |
| L4-003 | 分支跳转 | `beq x0, x0, target` | PC redirect 正确 |
| L4-004 | 函数调用 | `jal x1, func; ... ret` | RAS push/pop 对称 |
| L4-005 | RAW 依赖链 | `add x1,..; add x2,x1,..` | 流水线正确前递/停顿 |
| L4-006 | 异常 | 非法指令 → trap | redirect 到 trap handler |
| L4-007 | 多拍乘法 | `mul x1, x2, x3` | 结果在 MUL 延迟后正确提交 |
| L4-008 | Cache miss | load 到未缓存地址 | miss → refill → commit |

**实现方式：** 将指令编码为 32 位常量注入 IBuffer，模拟取指输出，跟踪到 ROB commit。

### L5: 系统级测试

**目标：** 多核 SoC 层面验证，对标 XiangShan 的 CI workload 回归。

| 测试 ID | 场景 | 对标 XiangShan | 工具 |
|---------|------|----------------|------|
| L5-001 | 单核 reset 后空闲 | M1 验收 | PyCircuit TB |
| L5-002 | 双核 tile 独立运行 | emu-basics | Verilator |
| L5-003 | 中断响应（PLIC→core） | misc-tests | Verilator |
| L5-004 | 定时器中断（CLINT） | misc-tests | Verilator |
| L5-005 | riscv-tests (RV64I 子集) | emu-basics | Verilator + golden |
| L5-006 | 简单 benchmark (dhrystone 类) | perf regression | Verilator |

---

## 四、SVA 形式化属性（借鉴 BypassUnit 模式）

利用 PyCircuit 的 `sva_assert` 为关键模块添加持续检查属性：

| 模块 | 属性 | SVA 表达式（伪码） |
|------|------|-------------------|
| IBuffer | 不会同时满和空 | `!(num_valid == SIZE && enq_ptr == deq_ptr)` |
| ROB | commit 只在 head valid 且 writebacked 时 | `commit_valid → head_valid & head_wb` |
| ROB | commit 顺序连续 | `commit[i]_valid → commit[i-1]_valid` |
| IssueQueue | issue 选中项必须 ready | `issue_valid → src0_ready & src1_ready` |
| Rename | pdest 分配不重复 | `pdest[i] != pdest[j] for i != j` |
| TLB | hit 和 miss 互斥 | `!(resp_hit & resp_miss)` |
| DCache | 不同 way 的 hit 互斥 | `popcount(way_hit) <= 1` |
| AXI | valid 不能在 ready 之前撤销 | `$rose(valid) → valid until ready` |
| FTQ | bpu_ptr 不超过 commit_ptr + SIZE | 溢出保护 |

---

## 五、DiffTest 等价验证策略

XiangShan 的核心验证手段是 DiffTest（RTL 与 NEMU 逐指令对比）。对 XiangShan-pyc
的适配路径：

### 阶段 1：Golden 向量（近期可行）

- 从 XiangShan 参考实现的仿真中提取指令级 trace（PC、寄存器写回值、内存访问）
- 编码为 Python 字典列表，作为 `tb_*.py` 的 golden reference
- 逐周期 `tb.expect` 对比

### 阶段 2：Python ISS 对接（中期）

- 用 Python 编写简化 RV64I ISS（或对接 riscvmodel / spike-dasm）
- 在 `CycleAwareTb` 中每次 commit 时比对 ISS 状态
- 类似 XiangShan DiffTest 的 commit-级对比，但在 Python TB 层完成

### 阶段 3：Verilator DiffTest（远期）

- `pycc --emit=verilog` 生成完整 Verilog
- 复用 XiangShan 的 DiffTest 框架（DPI-C + NEMU）
- 在 Verilator 仿真中实现真正的指令级 DiffTest

---

## 六、波形与调试

### PyCircuit 原生

| 方式 | 用途 | 启用方法 |
|------|------|----------|
| VCD | 标准波形查看 (GTKWave) | C++ TB: `tb.enableVcd` / SV: `$dumpvars` |
| `.pyctrace` | 二进制 trace + `dump_pyctrace.py` 解码 | `--trace-config trace.json` |
| Perfetto UI | Chrome Trace Event JSON 可视化 | 自定义 `sim_*_perfetto.py` 脚本 |

### 对标 XiangShan 调试手段

| XiangShan 工具 | PyCircuit-pyc 对等物 | 状态 |
|----------------|---------------------|------|
| ChiselDB (SQLite) | `.pyctrace` + Python 后处理 | 待开发 |
| lightSSS (fork 快照) | Verilator `$dumpvars` + VCD window | 可用 |
| xspdb 交互调试 | Python `pdb` + TB step | 可用 |
| Top-down 计数器 | 在 RTL 中添加 perf counter 输出端口 | 待开发 |

---

## 七、CI/回归策略

借鉴 XiangShan 的 `emu-basics.yml` / `perf-template.yml` 分层 CI：

### Gate 层级

| Gate | 触发 | 内容 | 阻塞 PR |
|------|------|------|---------|
| **G0** | 每次 push | Python import + `test_xs_steps.py` | 是 |
| **G1** | 每次 push | 全模块 MLIR emit + 端口名回归 | 是 |
| **G2** | 每次 push | L2 功能定向测试（叶子模块） | 是 |
| **G3** | 合并到 main | `pycc --emit=verilog` 全核编译通过 | 是 |
| **G4** | 合并到 main | L3 子系统集成测试 | 是 |
| **G5** | 每日夜间 | L4 核级端到端 + Verilator 仿真 | 否（报告） |
| **G6** | 每周 | L5 系统级 + 性能回归 | 否（报告） |

### 回归测试执行

```bash
# G0+G1: 快速门（<2 分钟）
cd integrations/xiangshan
pytest test_xs_steps.py -v
pytest lib/tb_primitives.py -v
pytest -k "emit_mlir" --ignore=top/tb_xs_top.py -v  # 全模块 MLIR

# G2: 功能测试（<10 分钟）
pytest -m "functional" -v

# G3: Verilog 编译
pycc integrations/xiangshan/top/xs_top.pyc --emit=verilog -o xs_top.v

# G5: Verilator 仿真
pycircuit build top/xs_core.py --target verilator --run-verilator
```

---

## 八、测试文件组织

```
integrations/xiangshan/
├── test_xs_steps.py               ← L0 可导入性 (pytest)
├── test_xs_functional.py          ← L2 功能定向 (pytest, 新建)
├── test_xs_integration.py         ← L3 子系统集成 (pytest, 新建)
├── test_xs_e2e.py                 ← L4 核级端到端 (pytest, 新建)
├── test_xs_system.py              ← L5 系统级 (pytest, 新建)
├── conftest.py                    ← pytest fixtures + markers
├── golden/                        ← Golden 测试向量 (新建)
│   ├── alu_vectors.json
│   ├── decode_vectors.json
│   ├── riscv_tests/               ← riscv-tests 指令序列
│   └── README.md
├── lib/tb_primitives.py           ← L1+L2 原语测试
├── frontend/
│   ├── bpu/tb_bpu.py              ← L1 冒烟 + L2 功能 (扩展)
│   ├── ftq/tb_ftq.py              ← L1 冒烟 + L2 功能 (扩展)
│   ├── icache/tb_icache.py        ← L1 冒烟 + L2 功能 (扩展)
│   ├── ifu/tb_ifu.py
│   ├── ibuffer/tb_ibuffer.py      ← L1 冒烟 + L2 功能 (扩展)
│   ├── decode/tb_decode.py        ← L1 冒烟 + L2 功能 (扩展)
│   └── tb_frontend.py             ← L3 子系统集成 (扩展)
├── backend/
│   ├── exu/tb_alu.py              ← 优先扩展为 L2 功能测试
│   ├── ...
│   └── tb_backend.py              ← L3 子系统集成
├── mem/
│   └── tb_memblock.py             ← L3 子系统集成
├── top/
│   ├── tb_xs_core.py              ← L4 核级端到端
│   └── tb_xs_top.py               ← L5 系统级
└── sva/                           ← SVA 属性集 (新建)
    ├── sva_rob.py
    ├── sva_issq.py
    ├── sva_cache.py
    └── sva_bus_protocol.py
```

---

## 九、覆盖率目标

| 层级 | 目标覆盖率 | 衡量方式 |
|------|-----------|----------|
| L0 | 100% 模块可导入 | 模块计数 |
| L1 | 100% 模块 MLIR 编译通过 | 编译成功率 |
| L2 | 每模块至少 3 个功能场景 | 场景计数 / feature_list |
| L3 | 每子系统至少 5 个集成场景 | 场景计数 |
| L4 | 至少 8 种指令类型端到端通过 | 指令类型覆盖 |
| L5 | riscv-tests RV64I 子集全通过 | 测试通过率 |

---

## 十、里程碑与验证交付物对应

| 里程碑 | 验证交付物 | Gate |
|--------|-----------|------|
| **M0** 基础设施 | L0 全通过 + L1 `lib/` MLIR 通过 | G0 |
| **M1** Frontend MLIR | L1 Frontend 全通过 + L2 IBuffer/Decode 功能 | G1+G2 |
| **M2** Backend MLIR | L1 Backend 全通过 + L2 ALU/ROB 功能 | G1+G2 |
| **M3** MemBlock MLIR | L1 MemBlock 全通过 + L2 Cache 功能 | G1+G2 |
| **M4** XSCore 集成 | L3 三子系统集成 + L4 单指令端到端 | G4+G5 |
| **M5** Verilog + 仿真 | L4 全场景 + Verilator 仿真通过 | G3+G5 |
| **M6** SoC 系统级 | L5 系统级 + 性能基线 | G6 |

---

## 十一、实施优先级

**立即行动（P0）：**
1. 为 ALU 编写第一个 L2 功能定向 TB（作为模板）
2. 为 IBuffer 编写 FIFO 功能 TB
3. 为 ROB 编写 enqueue-writeback-commit TB
4. 创建 `conftest.py` 和 pytest markers

**短期（P1, 2 周内）：**
5. 扩展所有叶子模块的 L2 功能测试
6. 添加 `pyc.reg` 计数回归
7. 添加 SVA 属性（ROB/IssueQueue/Cache）
8. 集成 `pycc --emit=verilog` 编译门

**中期（P2, 1 个月内）：**
9. L3 子系统集成测试
10. L4 核级端到端测试
11. Golden 向量生成与比对

**远期（P3）：**
12. Verilator 完整仿真
13. DiffTest 对接
14. 性能回归基线

---

**Copyright (C) 2024-2026 PyCircuit Contributors**
