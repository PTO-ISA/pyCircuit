# Ripes5 参考模型与原生适配器

本目录提供 Ripes5 验收使用的 Python 独立模型、汇编器、13 个程序、轨迹比较器和原生观察适配器。Python 引擎保留历史实现，仅用于轨迹对照；当前编译器示例和性能入口在 [ACPy Ripes5](../../../../pycircuit/examples/ripes5/README.md)，GFSim 运行时契约见 [spec](../../../spec.md)。

根目录的 `model.py`、`stages.py`、`logic.py`、`records.py` 和 `isa.py` 描述微架构、状态与译码。测试程序、汇编器、运行观察和测试放在 [tests/](tests/)；宿主输入协议检查放在 [tools/](tools/)；原生源码的构建、版本锁与观察适配器放在 [reference/](reference/)。

## 原生参考构建

[reference/version.json](reference/version.json) 固定 Ripes `5b8a616edcb6f0a2ddb07e78951348b72497f1e1`、VSRTL `8497dd14fe80e57efcff4c424a9a3b6363d93eb7` 和 FetchContent 依赖。源码默认位于根目录 `reference/ripes-reference/`，构建位于 `reference/builds/ripes-reference/`；适配器、版本锁和构建脚本由本仓库保存。

当前 aarch64 环境使用 GCC 14、Qt 6.8.3。Qt 依赖可通过锁文件安装，已有环境时跳过第一条：

```bash
/home/lc/opt/miniforge3/bin/conda create -y -p /tmp/gfsim-ripes-qt \
  --file gfsim/experiment/examples/ripes5/reference/qt-linux-aarch64.lock
python3 gfsim/experiment/examples/ripes5/reference/build.py
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.reference.smoke
```

构建脚本校验已有源码和子模块的 commit 与干净状态。`--source`、`--build`、`--qt-prefix`、`--cxx`、`-j` 可覆盖默认设置；`--reference-only` 只编译观察器，迁移过的 CMake 构建使用 `--fresh` 重新配置。首次下载依赖需要网络。Qt 默认为 `/tmp/gfsim-ripes-qt`，运行时也可设置 `RIPES_QT_PREFIX`。

适配器经 `CMAKE_PROJECT_Ripes_INCLUDE` 注入构建，原生源码保持不变。它装载初态、读取公开端口并调用 `clockUnguarded()`。默认关闭端口与时钟观察通知，反向历史为 0；`INPUT.json --observe` 恢复通知进行轨迹检查；`INPUT.json --benchmark-fixed K N` 仅计时预热后 N 次时钟调用。

## 验收

```bash
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.tests.run
RIPES5_REQUIRE_NATIVE=1 python3 -m unittest discover -s gfsim/experiment -v
```

`run` 比较缓存开/关及 Module 正/反序四种历史 Python 配置，均与同一个原生轨迹逐拍比较。`--case`、`--input`、`--runner`、`--output` 可指定输入和路径，`--html` 保存观察页面。默认结果在 `reference/benchmarks/ripes5-python/`。正式 C++/ACPy 验收始终要求原生参考存在且版本匹配。

## 状态、控制与时钟边界

| 已登记 current | 组合控制（固定版本源码） | 同一拍末更新 |
| --- | --- | --- |
| PC、IF/ID、ID/EX、EX/MEM、MEM/WB | `HazardUnit` 的 EX load 目的寄存器与 ID 源寄存器比较；x0 不触发冒险 | load-use 时保持 PC、IF/ID，清 ID/EX；旧 EX 仍进入 MEM |
| ID/EX、EX/MEM、MEM/WB | EX 前递先 MEM ALU，再 WB 选择值；源 x0 禁止前递 | ALU 结果、前递后的 store 数据进入 EX/MEM |
| ID/EX 与前递值 | EX BEQ/BNE/JAL/JALR 产生重定向，`controlflow_or` 选择 PC mux | 更新 PC，同时清 IF/ID 和 ID/EX；旧 EX/MEM/WB 继续前进 |
| IF/ID、寄存器 Queue、MEM/WB | `RegisterFile<32,true>` 的 WB→ID 旁路 | 解码值进入 ID/EX，WB 的寄存器写在同一个沿提交 |
| EX/MEM、数据 Queue | 同拍组合读一拍存储；ALU/PC+4/读数据选 WB 值 | store 在 MEM 沿写数据；结果进入 MEM/WB |
| MEM/WB | 寄存器写控制与 valid 分别观察 | WB 沿写寄存器并计退休；marker 的此沿为结束边界 |

对应上游文件位于 `src/processors/RISC-V/rv5s/rv5s.h`、`rv5s_hazardunit.h`、`rv5s_forwardingunit.h`、`../rv_registerfile.h` 及 VSRTL `vsrtl_register.h`。`RegisterClEn` 先判 enable，再判 clear；本范围没有 ECALL，ID/EX 总使能，EX/MEM 不清零，MEM/WB 总前进。ID/EX 的 stalled 独立于 clear 保存，然后逐级传递。

`model.py` 显式构造每个资源、五个阶段实例及五条 `RuleEntry`，列出空的 `pops/pushes` 和唯一 `revises` 来源。PC、四个流水寄存器、32 个寄存器、每个数据字、退休和存储事件计数都用容量为 1 的非空 Queue。valid 为 `Slot` 字段，气泡不清空 Queue。规则不能看到其他规则的 proposal。

## 输入与观察接口

输入 JSON 包含 `words`（从 PC 0 开始的 32 位机器字）、`registers`（32 个 uint32，x0=0）、`data_base`、`data`（uint32 字数组）、`end_pc`、`max_cycles`；`name/source` 仅供工具报告。两端加载同一 JSON，不分别重新汇编。`programs.make_case()` 添加唯一的 `ADDI x0,x0,2047` 结束标记，之后是 `JAL x0,0` 无副作用循环及两个 NOP。HALT、ECALL 不在接口内。

JSONL 第 0 行是初始化并完成组合传播后的状态。第 n 行是第 n 个沿之后的状态，同时也是下一沿的拍前状态；`retire/store` 表示刚发生的沿。没有按程序移动周期：

- `stages` 是 IF/ID/EX/MEM/WB 的 valid 和有效 PC，无效 PC 统一 null；valid 同时要求 PC 位于可执行代码范围。
- `fetch_pc` 为实际 PC current；`next_fetch` 是 PC mux 输出，`next_pc` 考虑 enable 后将提交的 PC。停顿时二者可以不同。
- `control` 包含 stall、两处 flush、enable、EX/MEM clear、重定向目标、两个前递选择（0=ID，1=MEM，2=WB）。`stalled` 是各阶段停顿气泡标记。
- `registers/data` 是沿后的全部建筑状态；`retired` 是累计数，`retire` 含 PC、机器字及可选寄存器写，`store` 为可选 `[地址, 值]`，即使写回相同值也记录事件。
- 原版 `raw` 额外保存所有阶段残留 PC、raw valid、ALU 和 WB 值。比较器只移除这个明确的诊断字段，不比较无效槽的残留 payload。

代码区与数据区分离；首轮仅 aligned RV32I 子集 ADD/ADDI/SUB/AND/OR/XOR/SLT/LUI/LW/SW/BEQ/BNE/JAL/JALR，不测自修改代码、压缩指令、陷阱、异常或非对齐访存／跳转。包括非零初始寄存器和内存、溢出及负数。

13 个程序覆盖前递优先级、x0、WB→ID、store 数据前递、数组求和、load-use、分支、跳转和错误路径写入。固定版本 Ripes 对 JALR 保留 ADD 目标地址行为；本模型按该版本对齐。原生适配及输入协议也由手写 C++、ACPy 生成版和 MLIR 重载版复用。
