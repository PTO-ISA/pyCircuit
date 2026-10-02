# Ripes RV32_5S 逐拍对照

本目录独立表达固定版本 Ripes 的五级流水。原有 `../riscv/` 弹性 CPU 和通用调度器均未修改。参考端是原版 `vsrtl::core::RV5S<uint32_t>`，不是 Python 参考流水线。

当前已完成生成式写法重构，保持寄存器表达的逐拍行为。四个级间 Queue 仍通过 `revise` 更新，不消费、不产生容量背压；本次验收不代表消费型级间队列验证完成。后续需要独立解决输入读取形成 pop、背压及 load-use 停顿在编译契约下的表达。

## 构建与运行

版本由 [reference/version.json](reference/version.json) 固定：Ripes `5b8a616edcb6f0a2ddb07e78951348b72497f1e1`，VSRTL `8497dd14fe80e57efcff4c424a9a3b6363d93eb7`。同时记录全部 FetchContent 依赖 SHA。

本机 aarch64 / Python 3.11 / CMake 3.31 / GCC 14.4，Qt 6.8.3。初次配置确认系统缺少 Qt；通过 conda-forge 在 `/tmp` 安装 Qt，补齐 OpenGL 开发包。无需安装系统软件。Qt Svg 已包含于 `qt6-main`，不存在独立的 `qt6-svg=6.8` 包。可复用完整的 [Qt 环境锁文件](reference/qt-linux-aarch64.lock)：

```bash
/home/lc/opt/miniforge3/bin/conda create -y -p /tmp/gfsim-ripes-qt \
  --file gfsim/experiment/examples/ripes5/reference/qt-linux-aarch64.lock
python3 gfsim/experiment/examples/ripes5/reference/build.py
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.reference.smoke
```

构建脚本首先递归下载源码（默认本目录的 `reference/upstream/Ripes/`），固定提交并校验已有源码没有修改，再配置和构建原版 `Ripes` 及 `ripes5-reference`。已有非指定版本会报错，不会重置用户 checkout。`--source / --build / --qt-prefix / --cxx / -j / --fresh` 可覆盖本机路径。依赖下载需要网络。构建产物默认本目录的 `reference/build/`。原版源码、子模块、FetchContent 下载的依赖和二进制均在此 example 下，其中 `reference/upstream/`、`reference/build/` 由 Git 忽略；Qt SDK 仍由 `/tmp/gfsim-ripes-qt` 提供。版本锁和适配器代码由主仓保存。

目录位置：

```text
ripes5/
  reference/
    upstream/Ripes/         # 固定原版源码，含 external/VSRTL 子模块
    build/                  # Ripes、ripes5-reference 及 CMake 构建依赖
    runner.cpp              # 本仓的逐拍观察适配器
    build.py                # 构建入口
    version.json            # 原版与子模块版本锁
```

原版模型入口为 [rv5s.h](reference/upstream/Ripes/src/processors/RISC-V/rv5s/rv5s.h)，前递和冒险单元也在同一目录。

迁移已有 CMake 构建树后运行 `build.py --fresh`，重新生成绝对路径，并复用已校验的 `_deps/*-src` checkout。

通过 `CMAKE_PROJECT_Ripes_INCLUDE` 添加本目录的 C++ 观察器目标，原版源码没有补丁。观察器只装载初始状态、读取公开端口、调用 `clockUnguarded()`，不替换译码、前递、冒险、存储或状态提交。

单命令比较全部程序（原版轨迹每个程序只生成一次）：

```bash
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.run
```

每个程序运行缓存开／关、Module 正序／反序四种配置。`--case array_sum --html` 生成通用 ReviewTrace 页面；`--input input.json` 接受同一输入格式；`--runner` 指定参考二进制。非默认 Qt 路径使用 `RIPES_QT_PREFIX`。默认输出在本目录被 Git 忽略的 `review-output/`：

- 每个程序的 `input.json` 和未经改写的 `ripes.raw.jsonl`、stderr。
- 四份 GFSim JSONL；不匹配时的 `*.mismatch.json` 包含首个差异字段、前后三拍、完整指令字和汇编。
- `summary.json` 保存版本、二进制 SHA256、周期与退休计数。
- 可选的通用 HTML 页面展示 Queue 和 Rule；这不是性能计时路径。

验收和耗时入口：

```bash
RIPES5_REQUIRE_NATIVE=1 python3 -m unittest discover -s gfsim/experiment -v
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.bench
```

原版未构建时常规 unittest 明确 skip 原版测试；`RIPES5_REQUIRE_NATIVE=1` 将其变为失败。`run`、`bench` 均要求真实参考存在且版本匹配，绝不自动替换参考。

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

`model.py` 显式构造每个资源、五个阶段实例及五条 `RuleEntry`，列出空的 `pops/pushes` 和唯一 `revises` 来源。PC、四个流水寄存器、32 个寄存器、每个数据字、退休和存储事件计数都用容量为 1 的非空 Queue。valid 为 `Slot` 字段，不将流水气泡建成空 Queue。每阶段一个 Module、一个普通成员 Rule；共享 `logic.control()` 和 `logic.execute()` 只接受不可变值。规则不能看到其他规则的 proposal。

Fetch 和 Decode 各自登记四个流水 current 并调用一次 `control()`；两者各调用一次 `execute()`，Execute Rule 再独立调用一次，共每拍三次 EX 组合计算。没有隐藏缓存。每个 Rule 显式请求下一拍事件，使稳定自循环和重复值也有时钟。调度器契约未改变；由此带来的 Queue 通知、重复计算和事件成本均属于本模型表达，未试图优化为 RTL 门级求值。

## 生成式代码的阅读顺序

[stages.py](stages.py) 的 Fetch、Decode、Execute、Memory、Writeback 是五个独立普通类，不继承 example 公共基类。构造参数明确传入 Queue 和固定配置；阶段不持有整个 CPU，不通过回调访问资源。`Work()` 直接调用相应 `work_<阶段>()`，`arbitrate_<阶段>()` 直接调用核心仲裁入口。

每条 Rule 自行展示 `begin_rule → record_read/try_peek → propose_revise → request_wakeup → complete_rule`。必要读取为空时显式 `abort_rule`；普通业务分支不被改写成撤销。依赖登记只位于实际读取路径上；同一 Rule 内已经读取的值可用局部变量复用。revise 目标依赖由核心 `propose_revise` 自行登记。没有 `observe/put/controls` 等额外调度接口。

例如，下面是 Writeback 的实际 Rule；读取、寄存器修改和退休事件归属同一 Rule，直到仲裁获准才一起提交：

```python
def work_writeback(self):
    e, rid = self.engine, self.rid
    if not e.begin_rule(rid):
        return

    e.record_read(self.mid, self.mem_wb.qid, rid)
    wb = self.mem_wb.try_peek()
    if wb is None:
        e.abort_rule(rid)
        return
    rd = writer(wb)
    if rd:
        self.registers[rd].propose_revise(rid, wb.value)
    if wb.valid and wb.pc % 4 == 0 and 0 <= wb.pc < self.code_size:
        e.record_read(self.mid, self.retirement.qid, rid)
        old = self.retirement.try_peek()
        if old is None:
            e.abort_rule(rid)
            return
        self.retirement.propose_revise(rid, Event(old.sequence + 1, wb.pc, wb.word,
                                                 rd, wb.value if rd else 0))

    e.request_wakeup(rid, self.mid, 1)
    e.complete_rule(rid)
```

`logic.py` 中的 payload／事件记录对应固定数据结构；译码、ALU、前递和控制函数只计算值，不访问 Queue 或调度器。这是手写的生成式代码样例，尚非编译器自动生成产物。已有核心 API 和 `construction.assemble` 足以支持本轮写法，无需增加新接口。

本次重构证据保存在 `review-output/generated-style/`：`before/` 保存旧源码、SHA256 和 48 组旧轨迹，`after/` 保存相同输入的新轨迹与原生参考原始输出，`before-after.json` 记录逐字段比较，`regression.log` 保存全量测试输出，`visualization/array_sum/` 保存四配置 HTML。

## 输入与观察接口

输入 JSON 包含 `words`（从 PC 0 开始的 32 位机器字）、`registers`（32 个 uint32，x0=0）、`data_base`、`data`（uint32 字数组）、`end_pc`、`max_cycles`；`name/source` 仅供工具报告。两端加载同一 JSON，不分别重新汇编。`programs.make_case()` 添加唯一的 `ADDI x0,x0,2047` 结束标记，之后是 `JAL x0,0` 无副作用循环及两个 NOP。HALT、ECALL 不在接口内。

JSONL 第 0 行是初始化并完成组合传播后的状态。第 n 行是第 n 个沿之后的状态，同时也是下一沿的拍前状态；`retire/store` 表示刚发生的沿。没有按程序移动周期：

- `stages` 是 IF/ID/EX/MEM/WB 的 valid 和有效 PC，无效 PC 统一 null；valid 同时要求 PC 位于可执行代码范围。
- `fetch_pc` 为实际 PC current；`next_fetch` 是 PC mux 输出，`next_pc` 考虑 enable 后将提交的 PC。停顿时二者可以不同。
- `control` 包含 stall、两处 flush、enable、EX/MEM clear、重定向目标、两个前递选择（0=ID，1=MEM，2=WB）。`stalled` 是各阶段停顿气泡标记。
- `registers/data` 是沿后的全部建筑状态；`retired` 是累计数，`retire` 含 PC、机器字及可选寄存器写，`store` 为可选 `[地址, 值]`，即使写回相同值也记录事件。
- 原版 `raw` 额外保存所有阶段残留 PC、raw valid、ALU 和 WB 值。比较器只移除这个明确的诊断字段，不比较无效槽的残留 payload。

代码区与数据区分离；首轮仅 aligned RV32I 子集 ADD/ADDI/SUB/AND/OR/XOR/SLT/LUI/LW/SW/BEQ/BNE/JAL/JALR，不测自修改代码、压缩指令、陷阱、异常或非对齐访存／跳转。包括非零初始寄存器和内存、溢出及负数。

## 程序与结论

`programs.py` 保存可读汇编、三个有界固定种子混合程序和统一机器字生成入口。程序覆盖顺序吞吐、连续覆盖前递优先级、x0、WB→ID、store 数据前递、数组填充求和、load-use、分支循环、跳转链接、load 后分支及错误路径写入。

验收结果与成本限制见 [findings.md](findings.md)。后续 C++ GFSim CPU 和编译器版本应复用本 JSON 输入及逐拍接口；本轮没有实现这两项，也没有做正式调度性能比较。
