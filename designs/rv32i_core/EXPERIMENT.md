# 从设计需求到软件模型与硬件：PyCircuit 五级 RISC-V CPU 实验

一个硬件项目从想法走向实现，需要回答几类相互关联的问题：它应该完成什么功能，
遇到依赖、等待和异常时如何工作，软件模型与最终电路能否保持一致，以及生成的
电路需要多大规模。本实验以一颗真正运行 C 程序的五级 RISC-V CPU 为载体，展示
PyCircuit 如何把这些问题放进一条连贯的开发主线：
**NDF → Spec → PyCircuit → C++/RTL → 验证与综合**。

我们先用 NDF 组织设计意图和需求，逐层细化成设计规格 Spec，再依据规格编写
PyCircuit 源码。编译器从这份源码生成 C++ 软件模型和 RTL 硬件描述，随后用同一
程序检查架构行为，并将 RTL 映射到开源标准单元库。项目同时提供生成源码、映射
网表和复现入口，读者既可以直接运行已有产物，也可以从设计源码重新生成。

## 实验对象与程序

这颗处理器采用 32 位 RV32I 基础整数指令集，具有 32 个通用寄存器，x0 始终为零，程序计数器（PC）记录指令地址。
指令沿取指、译码、执行、访存、写回五个阶段推进，多条指令可以同时在流水线中
执行。结果转发让后面的指令及时获得前面指令的结果；当数据尚未就绪或存储器
暂时无法完成请求时，流水线保持相关指令与操作数。分支和跳转会重定向取指，
并清除更年轻的错误路径指令。

处理器通过独立的指令和数据接口连接外部存储器。实验环境采用小端字节序，提供
从 `0x80000000` 开始的 64 KiB 存储空间，并约定复位、访问故障和异常终止行为。
寄存器更新与内存写入按程序顺序提交；遇到异常时，已经完成的较老指令保留其效果，
较年轻指令不能继续产生架构可见的写入。

验证用的[数组 C 程序](programs/c-array.c)先生成 32 个无符号整数，再计算总和与
移位、异或混合结果，最后将四个签名值写入内存并执行 EBREAK。签名用于快速辨认
程序结果，完整验证还会检查每条已完成指令及最终机器状态。下文的“正常完成
指令数”统计按顺序完成架构可见效果的指令，trap 则作为独立的异常终止事件计数；
“模拟时钟周期”对应模型推进的时钟周期数。预期签名为：

```text
[10208, 424583933, 309, 329]
```

这个程序同时使用循环、整数计算、加载、存储和跳转，能够展示流水线协作执行
真实编译程序的过程。专项停顿依赖用例进一步检查流水线等待时的数据保持行为。

## NDF、Spec 与源码如何连接

[NDF 设计输入](spec/design-input.md)首先记录项目目标与外部可观察要求：支持哪些
指令和状态，存储器如何完成请求，异常怎样影响提交，以及不同执行模型需要对齐
哪些结果。需求使用稳定的条款编号，后续设计和测试可以引用同一项要求。

[Core Spec](spec/core-spec.md)在这些需求上继续细化，规定五个阶段的职责、状态
归属、接口与时序、转发与停顿、重定向、状态更新和内存写入的提交规则，以及
复位行为。Spec 是 NDF 条款逐层细化形成的设计规格；它把“要做什么”进一步
落实为实现必须满足的行为约束。
[验收条款](spec/acceptance.md)再明确如何用独立预期检查这些行为。

PyCircuit 实现沿着这份规格展开。`types.py` 中的 `Machine` 集中保存寄存器文件、
程序计数器和流水状态；`core.py` 中的 `Core` 连接各阶段的计算，`Core.step` 原子
提交完整的下一状态。定义旁的 `implements` 引用对应 Spec 条款，验证入口的
`verifies` 指向验收要求。这样可以从需求找到实现，也可以从实现找到负责检查它的
测试。条款关联的具体操作见 [NDF 工作流](spec/README.md)。

本实验的生成源码与 SDK 固定在 main 提交 `3ee29b0`，完整版本和依赖集中记录于
[生成说明](generated/artifacts.json)。该版本修复了流水线停顿期间的数据保持问题：当较老的访存
指令阻塞执行阶段，而产生结果的较老指令已完成写回时，执行阶段会保存转发得到的
操作数，直到依赖它的指令可以继续推进。

## 从一份 PyCircuit 生成两类产物

源码经过捕获、独立源码单元编译与链接，形成共同的中间表示，再分别生成 C++
和 RTL。中间表示承载同一套计算、状态与提交关系；C++ 便于软件执行和检查，
RTL 则以时钟、复位和硬件信号表达电路，可用于仿真与综合。

功能执行与周期时序执行是另一条选择轴。时序执行逐周期推进；功能执行可以折叠
已证明不会改变可见状态与事件的纯等待。它们与 C++、RTL 两种生成目标组合，
形成四种运行组合。本项目的两种执行策略共用同一份生成计算代码，差别保留在
执行描述中，因此仓库只保存一份 C++ 和一份物理 RTL。

本次 C 程序使用快速存储器配置，没有省略等待周期，四种运行组合均推进 567 个模拟时钟
周期。直接产物复验入口逐周期观察输出，方便对齐状态和事件；需要使用 SDK 的
功能执行优化时，可以运行后文的源码生成入口。

## 环境与复现

实验在 Linux x86-64 上完成。C++ 模型使用 C++20 和 Release 配置编译；固件使用
`-march=rv32i -mabi=ilp32`，采用独立链接布局。主要环境如下，详细安装说明见
[工具与工艺库文档](verification/TOOLS.md)。

| 环节 | 实测版本 |
| --- | --- |
| PyCircuit 源码与 SDK | `3ee29b0` |
| 编译器依赖、固件 clang/lld | LLVM/MLIR 22.1.8 |
| C++ 模型编译器 | GCC 13.3.0 |
| Python 与专项测试 | Python 3.12.3；停顿依赖检查使用 Python 3.14.6、pytest 9.1.1 |
| RTL 仿真 | Icarus Verilog/vvp 12.0 |
| RTL 转换与综合 | sv2v 0.0.12；Yosys 0.33 与 ABC |
| 独立指令参考 | Spike `4c6a9bc` |
| 标准单元库 | Volare sky130A `c6d73a35…`，SKY130 HD、典型工艺、25°C、1.8 V |

从仓库根目录开始，先选择项目和输出位置，再使用项目固定的公开版本安装 SDK。
`LLVM_ROOT` 指向已有 LLVM/MLIR 22.1.8 安装；编译已有 C++ 文件时，CMake 使用
SDK 的 Runtime 组件。

```sh
export PROJECT="$PWD/designs/rv32i_core"
export OUT="$PWD/.pycircuit_out/rv32i-experiment"
python3 "$PROJECT/verification/setup_sdk.py" \
  --llvm-root "$LLVM_ROOT" --output "$OUT/sdk"
export PYC_TOOLCHAIN_ROOT="$OUT/sdk/install"
```

将 `RV32_CLANG` 和 `SPIKE` 设置为固定版本的可执行文件。Spike 需要 `dtc`；采用
本地依赖前缀时，把其 `bin` 目录加入 PATH。随后编译 C 程序并生成独立参考结果：

```sh
python3 "$PROJECT/verification/prepare.py" \
  --clang "$RV32_CLANG" --spike "$SPIKE" \
  --case c-array --output "$OUT/programs"
```

准备工具会在输出目录保留 ELF、反汇编、初始内存和 Spike 轨迹。Spike 运行同一份
ELF，提供指令完成序列和最终寄存器预期；C 算法还给出独立的内存签名预期。

### 直接运行生成产物

```sh
python3 "$PROJECT/verification/run_generated.py" \
  --prefix "$PYC_TOOLCHAIN_ROOT" --program "$OUT/programs/c-array" \
  --output "$OUT/compiled-artifacts"
```

这个入口编译仓库中的 C++，并用 Icarus 编译、运行仓库中的 RTL。测试适配器驱动
时钟和复位、响应存储器请求，处理器自身计算指令结果并更新架构状态。检查器将
每条已完成指令的 PC、指令编码、寄存器更新、访存和异常逐项与独立参考对齐，检查全部最终寄存器、
PC、控制状态、内存和签名，并核对内存写入的次数、地址、字节掩码与数据。

停顿依赖检查复用已经编译的产物。此入口需要安装 pytest：

```sh
python3 "$PROJECT/verification/check_generated_held.py" \
  --compiled "$OUT/compiled-artifacts" --output "$OUT/held-operands"
```

### 从源码重新生成

也可以使用正常的 PyCircuit 公开入口重新编译并运行模型。为不同策略、目标选择
独立输出目录即可：

```sh
"$PYC_TOOLCHAIN_ROOT/bin/pycircuit" run "$PROJECT" \
  --model rv32i_core.core.Core --execution timing --target cpp \
  --memory "$OUT/programs/c-array/simulation.json" --build-dir "$OUT/from-source"
```

将执行策略改为 `functional`、生成目标改为 `verilog`，可以得到其它组合。
[产物指南](generated/README.md)提供各生成文件的阅读入口。

## 软件模型与 RTL 的实验结果

四种组合均完成了同一 C 程序，并通过独立指令执行记录、完整状态和内存检查：

| 执行方式 | 生成目标 | 正常完成指令数 | 预期 trap | 模拟时钟周期 |
| --- | --- | ---: | ---: | ---: |
| 周期时序 | C++ | 434 | 1 | 567 |
| 功能 | C++ | 434 | 1 | 567 |
| 周期时序 | 物理 RTL / Icarus | 434 | 1 | 567 |
| 功能 | 物理 RTL / Icarus | 434 | 1 | 567 |

程序按预期执行 EBREAK 后终止。运行中五个阶段可以同时处理指令；后继指令
等待加载结果时产生了两次停顿，并发生 64 次取指重定向。所有组合的最终完整
状态一致，C++ 与 RTL 的逐周期输出也一致。

停顿依赖专项检查将依赖结果的指令留在执行阶段，同时让较老的加载等待两个周期，
使产生操作数的指令先完成写回。这能直接检验转发值是否在流水线停顿期间保留，覆盖普通算术、存储指令写入数据和存储地址三种依赖。

| 停顿依赖场景 | 功能/时序 × C++/RTL |
| --- | --- |
| 算术操作数 | 4/4 通过，ALU 场景 x4 为 5 |
| 存储指令写入数据 | 4/4 通过 |
| 存储地址 | 4/4 通过 |

这 12 个组合使用现有独立编码的程序与预期，检查完整寄存器、内存、指令完成顺序、
内存写入效果和实际等待周期。[结果记录](results/reproduction.json)与
[运行日志](results/validation)保留了详细结果。

## 综合与面积

物理 RTL 通过 sv2v 转换，再由 Yosys 和 ABC 映射到真实 SKY130 标准单元。
综合过程检查层次、驱动关系与组合环，完成时序单元和组合逻辑映射，并检查归一化
前后的单元、参数及逻辑端口连接。当前结果为 **13,860 个单元**，其中 2,076 个
时序单元、11,784 个组合单元；Liberty 单元面积合计 **109,284.8128 µm²**。

面积包含寄存器文件、流水控制和架构观察逻辑，外部指令/数据存储器与测试平台
不计入。所用单元面积已与 LEF 几何核对。这是标准单元映射面积；布局布线和静态
时序属于后续物理实现评估，因此本报告不据此推算芯片面积或最高频率。

工艺库使用固定的[Volare 发行版](https://github.com/chipfoundry/volare/releases/tag/sky130-c6d73a35f524070e85faff4a6a9eef49553ebc2b)，
单元模型采用 Apache-2.0 许可。可复用已有安装，也可只安装 HD 库：

```sh
volare enable --pdk-root "$PDK_ROOT" --pdk sky130 -l sky130_fd_sc_hd \
  c6d73a35f524070e85faff4a6a9eef49553ebc2b
export HD="$PDK_ROOT/sky130A/libs.ref/sky130_fd_sc_hd"
export SKY130_HD_LIBERTY="$HD/lib/sky130_fd_sc_hd__tt_025C_1v80.lib"
export SKY130_HD_LEF="$HD/lef/sky130_fd_sc_hd.lef"
export SKY130_HD_VERILOG="$HD/verilog/sky130_fd_sc_hd.v"
export SKY130_HD_PRIMITIVES="$HD/verilog/primitives.v"
```

重新映射仓库中的物理 RTL：

```sh
python3 "$PROJECT/verification/synthesize.py" \
  --generated "$PROJECT/generated/rtl" --toolchain "$PYC_TOOLCHAIN_ROOT" \
  --sv2v "$SV2V" --yosys "$YOSYS" --liberty "$SKY130_HD_LIBERTY" \
  --lef "$SKY130_HD_LEF" --output "$OUT/remapped"
```

[综合目录](results/synthesis/README.md)中提供完整映射网表、转换输入、综合脚本、
日志和面积统计。下面的入口可以直接读取已提供的网表，重算单元数和面积，并用
已通过独立参考检查的 C 程序轨迹运行映射后仿真：

```sh
python3 "$PROJECT/verification/check_mapped_artifact.py" \
  --liberty "$SKY130_HD_LIBERTY" --cells "$SKY130_HD_VERILOG" \
  --primitives "$SKY130_HD_PRIMITIVES" --yosys "$YOSYS" \
  --trace "$OUT/compiled-artifacts/timing-cpp" --output "$OUT/mapped-check"
```

映射网表也在 567 个模拟时钟周期内正常完成 434 条指令，并通过每周期输出和
最终完整状态比较。单元仿真使用功能模型与宽松测试时钟，结果说明映射后电路
保持了本程序的逻辑行为。

## 这套 demo 展示了什么

这次实验把需求组织、规格细化、源码实现、模型生成和硬件验证连接起来：NDF
提供清楚的需求与关联，Spec 约束流水线与可见效果，PyCircuit 将计算、状态和
提交表达在同一份源码中，C++ 与 RTL 则让设计分别进入软件验证和硬件实现流程。
C 程序与停顿依赖检查给出具体行为证据，开源工艺映射给出电路规模。

本报告汇集生成产物、实测结果和可执行入口，便于读者沿同一路径继续实验。
