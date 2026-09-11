# pyCircuit C++ 仿真引擎架构

## 1. 概述

本文只描述 PYC 生成的 `libpyc6_runtime` 周期模型。Agentic Circuit 的
`gfsim` 是另一套架构级运行时：它以 `(time, causal_delta)` 为全局 epoch，
用有限 delta 推进同一 tick 内的因果连续执行。两者共享验证边界，但不共享内部
调度器。

`gfsim::TimeDomainRuntime` 的 `period`、`phase` 与 `tickScale` 不是孤立的
C++ 字段。它们分别来自已验证的 `ac.time_domain` 和 `acsim.type`
`period`/`phase`/`tick_scale` 元数据；ACIR/ACSim ODS 与
`schemas/agentic-circuit/contracts/acsim.yaml` 共同定义公开 inventory。

pyCircuit 的 C++ 仿真引擎采用 **静态编译-直接执行 (Compiled-Code Simulation)** 模型，
而非传统 Verilog/VHDL 仿真器常用的 **事件驱动 (Event-Driven Simulation)** 模型。

整个 RTL 设计被编译为一个 **单一 C++ 结构体**，内含所有信号（`Wire<N>`）、
寄存器实例（`pyc_reg`）以及组合逻辑求值函数（`eval()`/`tick()`）。
仿真通过反复调用这些方法来推进时钟周期，在主机 CPU 上直接执行原生 C++ 代码。

```text
┌─────────────────────────────────────────────────────────┐
│  Python 测试驱动 (ctypes)                                │
│    设置输入 → 调用 C API → 读取输出                      │
├─────────────────────────────────────────────────────────┤
│  C API 封装层 (*_capi.cpp)                               │
│    rf_create / rf_reset / rf_tick / rf_get_rdata / ...   │
├─────────────────────────────────────────────────────────┤
│  Testbench<Dut> (pyc_tb.hpp)                             │
│    时钟管理 / reset 协议 / VCD 波形 / 二进制 Trace        │
├─────────────────────────────────────────────────────────┤
│  生成的 DUT 结构体 (*_gen.hpp)                           │
│    Wire<N> 信号成员 / eval() / tick()                    │
├─────────────────────────────────────────────────────────┤
│  运行时库 (pyc_bits.hpp, pyc_primitives.hpp, ...)        │
│    Wire<N> 位向量 / pyc_reg / pyc_fifo / pyc_sync_mem   │
└─────────────────────────────────────────────────────────┘
```

## 2. 核心数据结构

### 2.1 `Wire<N>` (pyc_bits.hpp)

所有信号（无论组合还是寄存器输出）都用 `Wire<N>` 表示，它是固定宽度的无符号位向量。

```cpp
template <unsigned Width>
class Bits {
    static constexpr unsigned kWords = (Width + 63) / 64;
    std::array<uint64_t, kWords> words_{};
};
template <unsigned Width>
using Wire = Bits<Width>;
```

- 存储以 64-bit word 为单元，小端序（word[0] = bits[63:0]）
- 所有运算符（`+`, `-`, `*`, `&`, `|`, `^`, `~`, 比较等）直接在 word 数组上操作
- 宽度 ≤ 64 bit 的信号仅占 1 个 word，零额外开销
- 宽度 > 64 bit 的信号（如 RegisterFile 的 640-bit rdata_bus）自动扩展为多 word

### 2.2 `pyc_reg<Width>` (pyc_primitives.hpp)

寄存器原语，实现两阶段更新协议：

```cpp
template <unsigned Width>
class pyc_reg {
    Wire<1> &clk, &rst, &en;
    Wire<Width> &d, &init, &q;
    Wire<Width> qNext{};
    bool pending = false;

    void tick_compute();  // 阶段1: 检测上升沿，计算 qNext
    void tick_commit();   // 阶段2: 原子提交 q = qNext
};
```

## 3. 单周期执行流程

每个仿真步（half-cycle step）按固定顺序执行，**没有事件队列**：

```text
┌───────────────────────────────────────────────────────────────┐
│  Testbench::step()  [pyc_tb.hpp:130]                          │
│                                                               │
│  1. eval()           — 组合逻辑前向求值（输入→输出）           │
│  2. clock toggle     — 翻转时钟信号                           │
│  3. tick()           — 时序逻辑更新                           │
│     3a. tick_compute() × 所有寄存器  — 计算下一状态            │
│     3b. tick_commit()  × 所有寄存器  — 原子写入                │
│  4. eval()           — 组合逻辑重新稳定（反映新寄存器值）      │
│  5. VCD dump (可选)                                           │
└───────────────────────────────────────────────────────────────┘
```

快速路径 `runPosedgeCyclesFast()` 对单时钟设计做了优化，
将上升沿和下降沿合并处理，每个完整周期执行：

```text
comb → clk=1 → tick_posedge → transfer → comb → clk=0 → tick_negedge → transfer
```

快速路径支持 SFINAE 检测 DUT 的 `tick_posedge()` / `tick_negedge()` 方法。
如果 DUT 提供了分离的时钟边沿方法，下降沿仅执行轻量级 `clkPrev` 更新，
避免对所有寄存器执行完整的 `tick_compute()` 检查。

### 3.1 eval() 组合逻辑求值

`eval()` 是编译器生成的纯函数，按 **拓扑排序** 展开所有组合逻辑节点。
编译器在 MLIR 层已完成数据流分析和调度，将组合逻辑分割为多个
`eval_comb_N()` 内联函数，顺序调用：

```cpp
void eval() {
    eval_comb_11();          // 解码 / 地址匹配
    rf_bank0_0 = pyc_reg_271; // 寄存器输出赋值
    rf_bank0_1 = pyc_reg_272;
    ...
    eval_comb_12();          // 写使能 / MUX 选择
    eval_comb_13();
    ...
    rdata_bus = pyc_comb_8234; // 最终输出
}
```

**关键特性**: 默认模式下，每个周期对所有组合节点做完整求值。
通过可选的 **信号变化检测 (Change Detection)** 机制，可以在输入未变化时
跳过 `eval()` 调用，形成混合 compiled/event 模型（参见 §5.6）。

### 3.2 tick() 时序更新

`tick()` 采用经典的 **两阶段更新协议**（compute-then-commit），
确保寄存器间无顺序依赖：

```cpp
void tick() {
    // Phase 1: 所有寄存器并行计算下一状态
    pyc_reg_271_inst.tick_compute();
    pyc_reg_272_inst.tick_compute();
    ...  // × 256 个寄存器
    // Phase 2: 所有寄存器原子提交
    pyc_reg_271_inst.tick_commit();
    pyc_reg_272_inst.tick_commit();
    ...  // × 256 个寄存器
}
```

## 4. 与事件驱动仿真的对比

| 特性 | pyc6 C++ (Compiled-Code) | 事件驱动 (如 Verilator/iverilog) |
|---|---|---|
| **调度模型** | 无事件队列；支持可选变化检测 | 全局事件队列 + 敏感列表 |
| **Delta 周期** | PYC 模型无 delta；拓扑排序保证单遍收敛 | 需要 delta 迭代直到稳定 |
| **信号变化检测** | 可选 InputFingerprint 跳过 eval | 仅重新评估受影响的进程 |
| **时间模型** | 周期精确 (cycle-accurate) | 支持精细时间步 (time-step) |
| **代码生成** | 单一 C++ 结构体 + 内联函数 | 多线程调度器 + 进程模型 |
| **延迟建模** | 不支持门级延迟 | 支持 inertial/transport delay |
| **适用场景** | RTL 功能验证、高吞吐仿真 | 门级仿真、精确时序分析 |

**PYC 生成的 pyc6 模型没有采用全局事件队列。** 它的核心是一个确定性的
"对所有组合逻辑做一次完整拓扑排序求值 → 两阶段寄存器更新"循环。
这种设计使得每个周期的执行路径完全确定，指令缓存友好，分支预测友好。

`gfsim` 不适用上述“无 delta”描述。其 causal delta 有固定上限，调度事件、
进程 continuation 和跨 time-domain 激活必须在 `Epoch{time, delta}` 上保持确定
顺序；超过上限以 `max_deltas_exceeded` 失败，而不是无限迭代。

## 5. RegisterFile RTL 仿真基准测试

正确性设计位于 `tests/integration/pycircuit/fixtures/regfile/`，性能驱动位于
`benchmarks/pycircuit/register_file/`。生成模型、共享库和 PGO profile 统一写入
`.pycircuit_out/benchmarks/register_file/`。

### 5.1 设计规格

| 参数 | 值 |
|---|---|
| 条目数 (ptag_count) | 256 |
| 常量 ROM 条目 (const_count) | 128 |
| 读端口 (nr) | 10 |
| 写端口 (nw) | 5 |
| 数据宽度 | 64 bit |
| 存储组织 | 2 bank × 128 entry × 32 bit |

### 5.2 生成代码统计

| 指标 | 值 |
|---|---|
| 生成 C++ 行数 | 33,113 |
| Wire<N> 信号成员 | ~17,590 |
| 寄存器实例 (pyc_reg) | 256 |
| 组合逻辑函数 (eval_comb) | 131 |
| tick_compute/commit 调用 | 各 256 次 |

### 5.3 性能数据

测试环境：Apple M1 (arm64)，macOS (darwin 25.2.0)，Apple Clang 17。
工作负载：每周期混合随机 10-路读 + 5-路写流量，100K cycles，取 5 次最优。

| 配置 | __TEXT 大小 | 耗时 | 吞吐量 | 加速比 |
|---|---|---|---|---|
| `-O2` baseline | 278 KB | 3.21 s | 31.2 Kcps | 1.00x |
| `-Os` (size-opt) | 246 KB | 2.46 s | 40.7 Kcps | 1.31x |
| `-Os` + SIMD + reg-opt | 262 KB | 2.58 s | 38.7 Kcps | 1.24x |
| `-O3 -flto` | 278 KB | 3.62 s | 27.7 Kcps | 0.89x |
| **PGO + `-O2` + SIMD** | **213 KB** | **1.69 s** | **59.1 Kcps** | **1.90x** |

最佳配置（PGO + O2 + SIMD + pyc_reg 优化）实现了 **1.90x 加速**。

### 5.3.1 优化前后实测对比

| 指标 | 优化前 (`-O2`) | 优化后 (PGO+SIMD) | 提升 |
|---|---|---|---|
| 100K cycles 耗时 | 3.21 s | **1.69 s** | -47% |
| 吞吐量 | 31.2 Kcycles/s | **59.1 Kcycles/s** | +90% |
| 单周期耗时 | 32.10 μs | **16.93 μs** | -47% |
| __TEXT 代码大小 | 278 KB | **213 KB** | -23% |

### 5.4 性能瓶颈分析与优化

**瓶颈诊断**: 生成代码的 `__TEXT` 段为 278 KB，远超 Apple M1 的 L1
I-cache (192 KB/core)。`eval()` 函数体包含 131 个 eval_comb 子函数，
执行约 17,000 个信号赋值/MUX/位操作。这导致：

1. **L1 I-cache thrashing**: eval() 代码无法完全放入 I-cache
2. **分支预测失效**: 大量 MUX 三元操作（`sel ? a : b`）创建不可预测分支
3. **D-cache 压力**: ~17,590 个 Wire 成员 + 256 个 pyc_reg 实例，总计 > 100 KB

**已实施的优化**:

#### (1) NEON SIMD 向量化 (`pyc_bits.hpp`)

为 `Wire<N>` 的多 word（kWords ≥ 2，即宽度 > 64 bit）操作添加了
ARM NEON 加速路径。每次处理 128 bit（2 × uint64_t）：

```cpp
// AND/OR/XOR: vld1q_u64 → vandq_u64/vorrq_u64/veorq_u64 → vst1q_u64
// EQ compare: vceqq_u64 → lane reduce
// MUX select: vbslq_u64 (bitwise select, branch-free)
```

适用信号：`raddr_bus`(80b), `wdata_bus`(320b), `rdata_bus`(640b)。
对此设计影响有限（多数操作在 ≤64b 信号上），但对宽数据路径设计显著有效。

#### (2) pyc_reg 优化 (`pyc_primitives.hpp`)

- 使用 `__builtin_expect` 标注分支概率（negedge 远多于 posedge）
- 减少 `tick_compute` 中的分支数量
- `tick_commit` 仅在 `pending` 时执行写入

#### (3) Profile-Guided Optimization (PGO)

PGO 是最大的单一优化因素。流程：

```bash
# 1. 带插桩编译
c++ -Os -fprofile-instr-generate ... -o lib_instr.dylib

# 2. 运行训练负载（50K cycles）
LLVM_PROFILE_FILE=regfile.profraw python benchmark.py

# 3. 合并 profile 数据
xcrun llvm-profdata merge -output=regfile.profdata regfile.profraw

# 4. 使用 profile 重新编译
c++ -O2 -fprofile-instr-use=regfile.profdata ... -o lib_pgo.dylib
```

PGO 的效果：

- 编译器将冷路径（从未执行的 MUX 分支）优化为 size
- 热路径保持高度优化，布局紧凑
- `__TEXT` 从 278 KB 降至 213 KB（-23%）
- 分支预测准确率大幅提升

#### (4) `-Os` 代码大小优化

`-O3` 反而比 `-O2` 慢（-11%），因为激进内联增大了 I-cache 压力。
`-Os` 减少 `__TEXT` 至 246 KB 即获得 31% 加速，证实瓶颈是 I-cache。

### 5.5 优化因素分解

| 因素 | 单独贡献 | 说明 |
|---|---|---|
| PGO | ~1.86x | 解决 I-cache + 分支预测 |
| `-Os` 编译 | ~1.31x | 减少代码体积 |
| NEON SIMD | ~1.01x | 窄信号设计受益有限 |
| pyc_reg 优化 | ~1.01x | tick 仅占周期 <10% |

**结论**: 对大型生成代码（> L1 I-cache），PGO 和代码大小优化比
SIMD 向量化更有效。SIMD 的价值体现在宽数据路径密集的设计中。

### 5.6 信号变化检测 (Change Detection)

**已实现。** 在 `pyc_change_detect.hpp` 中引入了混合 compiled/event 模型基础设施。

#### 核心组件

**`InputFingerprint<Widths...>`** — 跟踪一组输入信号的变化状态。
使用 XOR-fold 哈希做快速拒绝，memcmp 做精确比较：

```cpp
InputFingerprint<80, 5, 40, 320> fp(dut.raddr_bus, dut.wen_bus,
                                     dut.waddr_bus, dut.wdata_bus);
// 每周期:
if (fp.check_and_capture()) {
    dut.eval();   // 输入变化，必须重新求值
} else {
    // 输入未变化，跳过 eval() — 节省 ~17K 操作
}
```

**`ChangeDetector<Width>`** — 跟踪单个 Wire 的变化（轻量级快照对比）。

**`EvalGuard<Fn, Widths...>`** — 包装 eval_comb 函数调用，仅在输入
变化时执行（为编译器后端自动生成 guard 做准备）。

**`pyc_reg::posedge_tick_compute()` / `negedge_update()`** — 分离的
时钟边沿方法。posedge 路径跳过 clkPrev 检查（调用者保证上升沿），
negedge 路径仅更新 clkPrev 标记，避免 256 次无效的 tick_compute 调用。

#### RegisterFile 变化检测实测数据

工作负载：100K cycles，按活动率混合随机/空闲周期。

| 活动率 | 100% active (baseline) | 50% active | 25% active | 10% active | 1% active |
|---|---|---|---|---|---|
| 耗时 (s) | 1.72 | 1.35 | 1.17 | 1.05 | 0.99 |
| 吞吐量 (Kcps) | 58.0 | 73.8 | 85.6 | 94.8 | 101.0 |
| 相对加速 | 1.00x | 1.27x | 1.48x | 1.63x | 1.74x |

**结论**: 对活动率 50% 的设计（典型 CPU 流水线 stall 场景），
变化检测可提升 27%。对活动率 10% 的设计（外设/总线控制器），
可提升 63%。100% 活动时无额外开销（fingerprint 检查被内联后极轻量）。

### 5.7 自动化 PGO 构建 (pycircuit pgo-build)

**已实现。** PGO 流程已集成到 `pycircuit.cli` 工具链，一条命令完成全流程。

#### 使用方式

```bash
# 基本用法（自动生成训练负载）
pycircuit pgo-build regfile_capi.cpp -o libregfile_sim.dylib -I include

# 自定义训练命令 + 训练周期数
pycircuit pgo-build regfile_capi.cpp -o libregfile_sim.dylib -I include \
  --train-cycles 50000 \
  --train-command "python3 my_benchmark.py"

# 保留中间产物用于调试
pycircuit pgo-build regfile_capi.cpp -o libregfile_sim.dylib -I include \
  --prof-dir ./pgo_profiles --keep-profiles

# 指定编译器和优化标志
pycircuit pgo-build regfile_capi.cpp -o libregfile_sim.dylib -I include \
  --cxx clang++ --opt-flags "-Os" --extra-flags "-march=native"
```

#### 自动化流程

```text
┌──────────────────────────────────────────────────────────────┐
│  pycircuit pgo-build                                          │
│                                                               │
│  Step 1: 插桩编译    c++ -fprofile-generate → libinstr.dylib  │
│  Step 2: 训练运行    python3 _pgo_train.py (或自定义命令)     │
│  Step 3: Profile 合并 llvm-profdata merge → merged.profdata   │
│  Step 4: PGO 编译    c++ -fprofile-use → output.dylib         │
└──────────────────────────────────────────────────────────────┘
```

#### CLI 参数

| 参数 | 默认值 | 说明 |
|---|---|---|
| `capi_source` | (必需) | C++ CAPI 封装源文件 |
| `-o, --output` | (必需) | 输出 .dylib / .so 路径 |
| `-I, --include-dir` | 自动检测 | 额外头文件目录 (可重复) |
| `--cxx` | `$CXX` 或 `c++` | C++ 编译器 |
| `--opt-flags` | `-O2` | 优化标志 |
| `--extra-flags` | (空) | 额外编译标志 |
| `--train-command` | 自动生成 | 自定义训练 shell 命令 |
| `--train-cycles` | 10000 | 自动训练的周期数 |
| `--prof-dir` | 临时目录 | Profile 数据存放目录 |
| `--keep-profiles` | false | 保留中间产物 |

## 6. 多线程仿真可行性分析

### 6.1 当前架构的约束

当前仿真引擎是 **严格单线程** 的：

1. **周期间串行依赖**: 周期 N+1 的 `eval()` 依赖周期 N 的 `tick_commit()` 结果，
   无法跨周期并行
2. **周期内数据依赖**: `eval()` 内的 eval_comb 函数按拓扑排序调用，
   后序函数依赖前序函数的输出
3. **共享状态**: 所有 Wire 信号是同一结构体的成员变量，没有内存隔离

### 6.2 可行的多线程改造方向

#### 方向 A: eval() 内部并行化（周期内并行）

```text
eval_comb_0 ──┐
eval_comb_1 ──┼── 独立子图 → Thread 0
eval_comb_2 ──┘
eval_comb_3 ──┐
eval_comb_4 ──┼── 独立子图 → Thread 1
eval_comb_5 ──┘
              └── barrier ──→ 依赖汇合
eval_comb_6 ──── 需要两个子图的结果 → 单线程
```

**可行性**: 中等。需要编译器在 MLIR 层做数据流分析，
识别不相互依赖的 eval_comb 子图，插入 barrier 同步点。

**挑战**:

- 线程同步开销（barrier、原子操作）每周期至少数百纳秒，
  而当前单周期仅 ~32 μs，同步开销占比可达 1-5%
- 对于像 RegisterFile 这样高度交叉的 MUX 网络，
  独立子图较少，可并行度有限
- 需要保证 Wire 成员的缓存行对齐（避免 false sharing）

**预期收益**: 对大型设计（eval 耗时 > 100 μs/cycle）可能有 1.5-3× 加速。
对 RegisterFile 规模的设计，预期收益有限。

#### 方向 B: tick() 内部并行化（寄存器更新并行）

```text
Thread 0: tick_compute() for reg[0..127]
Thread 1: tick_compute() for reg[128..255]
──── barrier ────
Thread 0: tick_commit() for reg[0..127]
Thread 1: tick_commit() for reg[128..255]
```

**可行性**: 高。寄存器的 tick_compute 互相独立（只读共享状态，
写入各自的 qNext），天然适合数据并行。

**挑战**:

- tick() 通常只占每周期执行时间的一小部分（< 10%），
  大部分时间在 eval()
- 256 个寄存器的 tick_compute 每个仅几十纳秒，
  线程池调度开销可能 > 实际计算

**预期收益**: 微乎其微（< 5%）。除非寄存器数量极大（> 10K）。

#### 方向 C: 模块级并行化（多模块 SoC 设计）

```text
┌──────────┐    ┌──────────┐    ┌──────────┐
│  CPU Core │    │  RegFile  │    │   Cache  │
│ Thread 0  │    │ Thread 1  │    │ Thread 2 │
└─────┬─────┘    └─────┬─────┘    └────┬─────┘
      │                │               │
      └────── interface sync ──────────┘
```

**可行性**: 低-中。需要 `pyc.instance` 保留层次边界，
各模块独立求值，接口处插入同步。

**挑战**:

- 当前 `pyc-compile` 会内联所有子模块（不支持 `pyc.instance`）
- 模块间组合路径（如 bypass 网络）跨越边界，需要迭代稳定
- 需要重新设计编译器后端以保留层次结构

**预期收益**: 对大型 SoC（数十个模块）可能有 2-8× 加速，
但需要大量编译器和运行时工程。

#### 方向 D: SIMD 向量化（已实现）

已在 `pyc_bits.hpp` 中为 ARM NEON 添加了加速路径：

```cpp
// kWords >= 2 时自动使用 NEON (128-bit = 2×uint64)
// AND: vandq_u64, OR: vorrq_u64, XOR: veorq_u64, NOT: vmvnq_u8
// EQ: vceqq_u64 + lane reduce
// MUX: vbslq_u64 (bitwise select, branch-free)
```

**实测结果**: 对以窄信号（≤64b）为主的 RegisterFile 设计，
SIMD 贡献约 1.01x。对宽数据路径密集的设计（如 512-bit AXI 总线），
预期 1.5-2x 加速。

#### 方向 E: Profile-Guided Optimization（已实现，效果最佳）

PGO 让编译器基于实际运行 profile 优化代码布局：

- 将冷路径压缩（-Os），热路径保持优化
- 改善分支预测准确率
- `__TEXT` 从 278 KB 降至 213 KB（-23%）

**实测结果**: 单独贡献 **1.86x 加速**，是目前最有效的单一优化手段。

### 6.3 总结与建议

| 方向 | 可行性 | 改造成本 | 实测/预期加速 | 适用规模 |
|---|---|---|---|---|
| **E: PGO** | **高** | **低 (CLI 已自动化)** | **1.86x (实测)** | **所有大型设计** |
| **F: 变化检测** | **高** | **低 (已实现)** | **1.27-1.74x (实测)** | **活动率 < 100%** |
| D: SIMD 向量化 | 高 | 中 (运行时) | 1.01x (窄) / ~2x (宽) | 宽数据路径 |
| `-Os` 编译 | 高 | 无 | 1.31x (实测) | __TEXT > L1 I$ |
| A: eval 内部并行 | 中 | 高 (编译器) | 1.5-3× (预期) | > 100 μs/cycle |
| B: tick 并行 | 高 | 低 (运行时) | < 1.1× (预期) | > 10K 寄存器 |
| C: 模块级并行 | 低-中 | 很高 (全栈) | 2-8× (预期) | SoC 级 |

**已完成优化** (总加速 1.90x; 变化检测对低活动率设计可达 1.74x):

1. **PGO 构建流程**: `fprofile-instr-generate` → 训练 → `fprofile-instr-use`
2. **NEON SIMD**: `Wire<N>` 多 word 位操作向量化
3. **pyc_reg 优化**: `__builtin_expect` 分支提示 + posedge/negedge 分离
4. **`-Os` 编译标志**: 作为非 PGO 场景的推荐默认
5. ✅ **信号变化检测**: `InputFingerprint` / `ChangeDetector` / `EvalGuard`
   基础设施，跳过输入未变化周期的 `eval()` 调用。
   实测：10% 活动率时 +63%，50% 活动率时 +27%
6. ✅ **自动化 PGO 构建**: `pycircuit pgo-build` CLI 子命令，
   一条命令完成 instrumented build → training → profile merge → PGO build

**短期建议**:
7. 编译器后端自动生成 **per-eval_comb guard**，
   利用 `EvalGuard` 实现细粒度变化检测（当前为 DUT 级粗粒度）
8. 为大型设计启用 **编译期常量折叠**，消除 const ROM 的运行时求值

**中期建议**:
9. 在编译器中实现 **eval 子图分区**，为方向 A 做准备
10. 编译器后端自动生成 `tick_posedge()` / `tick_negedge()` 方法

**长期建议**:
11. 实现模块级并行（方向 C），需要重新设计编译后端的实例化策略
12. 探索 **GPU 加速仿真**：将宽位操作和 MUX 树映射到 GPU compute shader，
    适合极大规模（> 1M gate）的全芯片仿真
