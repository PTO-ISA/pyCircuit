# 构建与运行

从 `designs/wse_model` 执行以下命令，每次使用新的输出目录。脚本保留各阶段
命令、日志和 JSON 回执。源码分层后的完整机器验收仍在进行；这里给出复现
入口，具体通过状态以相同源码、程序和产物哈希的回执为准。

## 依赖与环境

| 用途 | 依赖 |
| --- | --- |
| 普通 Python 语义及测试 | Python 3.11+，`pip install -e '.[dev]'` |
| pyCircuit 原生模型 | `models/calendar/pycircuit.lock.json` 所指定的集成 checkout 与 SDK，LLVM/MLIR 22.1.8，CMake、Ninja、C++20 编译器 |
| Verilog 执行 | Icarus Verilog 和 VVP |
| PTO 输入生成 | PyPTO `9339b96dd6d42e69b1dee8f43316d407887e2d43` 及独立解释器 |
| PTO 中间表示处理 | PTOAS v0.72，及与其 MLIR Python 扩展匹配的解释器 |

集成 SDK 与源端版本必须匹配。本机集成提交包含编译器修复，普通上游 `main`
不自动具备同样能力；本项目不会安装这些外部工具。

```sh
export PYCIRCUIT_ROOT=/absolute/path/to/integrated-pycircuit
export PYC_TOOLCHAIN_ROOT=/absolute/path/to/installed-sdk
export PATH="$PYC_TOOLCHAIN_ROOT/bin:$PATH"
export PYTHONPATH="$PWD/src:$PYCIRCUIT_ROOT/python${PYTHONPATH:+:$PYTHONPATH}"
```

同时设置实际 `CC`、`CXX` 和系统 SDK。PyPTO 与 PTOAS 使用各自匹配的解释器，
分别设置 `AG_PYPTO_PY`、`AG_PTOAS_PY`、`AG_PTOAS_ROOT`，不混装解释器环境。

## 普通 Python

```sh
make check
make integration
python -m wse_model --json run ffn-allgather
python -m wse_model --json report matmul --m 1 --k 4096 --n 4096 --precision fp16
```

外部载荷比较需要实际输入、输出路径；未提供时跳过该项。PyPTO 示例检查在
设置 `AG_PYPTO_PY` 后执行输入编译。普通 Python 测试不运行 pyCircuit 电路。

## 共享组件回归

先执行接收接口的同一 12 周期场景，C++ 与 Verilog 使用相同最终电路：

```sh
python tools/verify_calendar.py \
  --pycircuit-root "$PYCIRCUIT_ROOT" --toolchain "$PYC_TOOLCHAIN_ROOT" \
  --suite receive --target both --output "$PWD/.wse_model_out/receive"
```

同一入口还保留 `route`、`forward`、`control`、`node`、`multinode`、
`capacity-positive` 和 `capacity-exhausted`。各 suite 的条件、周期与独立
检查记录在回执中。容量耗尽是预期失败案例，须比较指定的已发布观测前缀。

旧 `allgather`、`allgather-control`、`allgather-delayed` 原生入口已撤下。
对应源和规范保留在 `verification/fixtures/calendar/`，仅作历史场景和静态
reference 回归。新 AllGather 功能验证统一使用通用机器。

## 规范追溯

项目根 `ndf.yaml`、`ndf.lock` 固定 NDF 0.2 工具依赖，并将共享组件规范和
`spec/programmed-machine.md` 纳入同一个项目。按锁文件取得、构建外部工具，
放在 `.pycircuit_out/ndf`，再运行：

```sh
pycircuit spec-check . --ndf .pycircuit_out/ndf/target/release/ndf \
  --output "$PWD/.wse_model_out/project-spec"
```

`models/calendar` 的独立追溯入口只检查组件。它自己的依赖位置为
`models/calendar/.pycircuit_out/ndf`，可以指向同一锁定 checkout；准备好该路径后
可单独对 `models/calendar` 执行 spec-check。组件检查通过不替代项目根检查。

当前条款全部为 draft。追溯 PASS 说明规范身份与实际 Python 定义绑定正确；
行为验证与批准状态另在回执中记录。源码更新后，应重新执行对应追溯检查。

## 构建一次通用机器

默认单任务构建 C++ runner，优化为 O1；添加 `--emit-verilog` 从同一个
final ACIR 生成 RTL（寄存器传输级表示）：

```sh
python tools/build_wse_machine.py \
  --emit-verilog --output "$PWD/.wse_model_out/machine-build"
```

构建回执记录共享组件、机器和driver的源码哈希、工具链及各阶段退出状态。
编译、链接、后端生成、宿主构建和执行分别判断；生成 RTL 本身不等于已运行。
程序、稀疏内存初始化和读回范围都是运行时文件，格式见
[program-interface.md](docs/program-interface.md)。

驱动回执的 `timing.phases` 分别记录复位、内存加载、程序加载、启动、执行、
读回及账户检查所用的宿主秒数、周期数和 Step 次数。每个宿主周期仍执行低、
高两个半拍，检查条件保持一致。`execution_cycles` 表示模型执行周期，宿主
秒数表示模拟器运行开销；比较性能时须固定程序、输入、硬件配置、编译优化
等级和构建并行度。`driver_seconds_before_receipt` 还包含输入解析与输出写入，
计时止于回执 `timing` 字段生成处，包含此前回执前缀的准备与写入。

## 同硬件程序验证

C++ 测试入口加载独立指定的程序，并比较字节、接收账户和许可变化：

```sh
python tools/verify_wse_machine.py \
  --simulator "$PWD/.wse_model_out/machine-build/build/wse_runner" \
  --output "$PWD/.wse_model_out/machine-cpp"
```

树形场景覆盖分支、只转发节点和节点 33；许可控制固定程序与数据，只撤去一个
远端目的在 offset=8 时的三拍许可，比较保持状态、输出及周期增量。重复身份
场景检查目标数据和计数保持。每个案例使用同一个已构建runner。

RTL 适配器从同一build目录读取final ACIR、Verilog及端口信息，以运行时输入
驱动生成硬件；独立预期仍在host：

```sh
python tools/verify_machine_rtl.py \
  --build "$PWD/.wse_model_out/machine-build" \
  --toolchain "$PYC_TOOLCHAIN_ROOT" \
  --output "$PWD/.wse_model_out/machine-rtl"
```

源码变化后，旧build的来源检查会失效，应重新构建，不能替换回执中的hash来
复用旧结果。

## 实际 PTO 数据链

设置独立工具环境后执行：

```sh
python tools/run_pto_allgather.py \
  --pypto-python "$AG_PYPTO_PY" --ptoas-python "$AG_PTOAS_PY" \
  --ptoas-root "$AG_PTOAS_ROOT" \
  --simulator "$PWD/.wse_model_out/machine-build/build/wse_runner" \
  --output "$PWD/.wse_model_out/pto-machine-run"
```

该入口重新生成 PyPTO 输入、处理 PTOAS 中间表示、验证传输与同步、生成通用
程序，再运行真实字节读写。输出逐字节与原始输入及算法定义比较。PTO基线的
512个COPY与包含配置、接收、等待和退休的完整指令数分开记录。

## 专项时序分析

`models/pto_allgather/timing.py`、`timing_driver.cpp`、`timing_profile.json`
保留原路径，作为 analysis-only 的行列专项投影。相关入口为
`build_pto_allgather_timing.py`、`analyze_pto_allgather_timing.py` 和
`verify_pto_allgather_timing.py`。它们按对应工作负载和receipt版本单独验证，
不提供通用机器的实际字节输出。完成 PTO 数据链后，可独立执行：

```sh
python tools/build_pto_allgather_timing.py \
  --output "$PWD/.wse_model_out/timing-build"
python tools/verify_pto_allgather_timing.py \
  --compiler-dir "$PWD/.wse_model_out/pto-machine-run/baseline-32/compiler" \
  --functional-verification "$PWD/.wse_model_out/pto-machine-run/verification.json" \
  --simulator "$PWD/.wse_model_out/timing-build/build/timing_runner" \
  --output "$PWD/.wse_model_out/timing-verification"
```

暂定参数保持 `pending`。通用机器串行步数、时序投影的片上网络周期与真实
硬件测量分别报告；复用功能证据前还须校验相同程序和产物身份。

## 512 位版本的单次 32 成员计时

使用上述工具环境、构建入口及同一组 32 成员输入，运行基准种子 1：

```sh
python tools/run_pto_allgather.py \
  --pypto-python "$AG_PYPTO_PY" --ptoas-python "$AG_PTOAS_PY" \
  --ptoas-root "$AG_PTOAS_ROOT" \
  --simulator "$PWD/.wse_model_out/machine-build/build/wse_runner" \
  --output "$PWD/.wse_model_out/pto-wide-baseline" \
  --profile baseline-32 --baseline-only
```

输出目录必须尚不存在。省略这两个筛选选项可运行两个 profile 的基准、换数据
和暂停案例。`baseline-32/baseline/simulation.json` 分别记录加载、执行、读回
耗时和功能周期；`verification.json` 记录完整字节校验及构建来源。计时不含
硬件编译和 PTO 程序编译，仿真器使用一个线程。

## 集成时序模型

时序构建使用 PR #424 的 SDK（提交见 `models/machine/timed-pycircuit.lock.json`）。
激活对应的 Python、源码编译器、链接器及发射器后执行下列命令。构建器从共享
账本生成固定布局的源码，保存在构建输出的 `fixed-source/` 目录。配置为 40 节点、
每节点 2 个早到槽、56 个去重槽，身份行宽 1232 位（56 × 22）。

时序构建单独链接时序顶层和所需组件。上述配置的全部源码单元已编译通过。
当前整机链接在事务计划分析阶段触发 `transaction state analysis work budget exceeded`。
后两条验收命令需要整机构建生成 `wse_runner` 后才能运行。

```sh
python tools/build_wse_machine.py --model timed \
  --output "$PWD/.wse_model_out/timed-build" --optimization O1
python tools/run_timed_wse_machine.py --suite smoke \
  --simulator "$PWD/.wse_model_out/timed-build/build/wse_runner" \
  --output "$PWD/.wse_model_out/timed-smoke"
python tools/run_timed_wse_machine.py --suite allgather \
  --pypto-python "$AG_PYPTO_PY" --ptoas-python "$AG_PTOAS_PY" \
  --ptoas-root "$AG_PTOAS_ROOT" \
  --simulator "$PWD/.wse_model_out/timed-build/build/wse_runner" \
  --output "$PWD/.wse_model_out/timed-allgather"
```

`--profile` 可指定显式时序配置。模型事件时间、电脑耗时和输出字节验证分别
保存在 `simulation.json`、`events.jsonl` 和 `verification.json`。配置文件和
所有产物均随回执记录哈希；以回执中实际完成的阶段为准。
