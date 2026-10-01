# 五级 mini RISC-V CPU

一个流水阶段一个 Module，Rule 是普通成员函数。输入为汇编文本，汇编成真实 RV32I 指令字后由 CPU 取指、译码和执行。组件使用现有 GFSim 读取、候选、仲裁、事件及 Xfer；运行驱动只观察结果。

## 运行

在仓库根目录执行，仅依赖 Python 3.11 标准库：

```bash
python3 -m unittest discover -s gfsim/experiment -k examples.riscv -v
PYTHONPATH=gfsim/experiment python3 -m examples.riscv.run gfsim/experiment/examples/riscv/programs/sum.s
PYTHONPATH=gfsim/experiment python3 -m examples.riscv.run gfsim/experiment/examples/riscv/programs/sum.s --latency 5 --trace
PYTHONPATH=gfsim/experiment python3 -m examples.riscv.run gfsim/experiment/examples/riscv/programs/sum.s --latency 5 --html
```

`--html` 默认生成本例目录下的 `review-output/riscv.html`，用浏览器打开即可审阅；可用 `--html 路径` 覆盖输出位置。生成目录由 Git 忽略。

[sum.s](programs/sum.s) 用指令向内存写入 1～10，再加载、求和并存回 55。数据存储初始为零，没有从测试代码直接注入运行时状态。三种配置均退休 98 条指令，与顺序解释器一致：

| MEM 访存延迟 | 总周期 | x4 和地址 40 的值 |
| --- | ---: | ---: |
| 1 | 148 | 55 |
| 3 | 190 | 55 |
| 5 | 232 | 55 |

`--trace` 输出每拍开始时 ID/EX/MEM/WB 所持指令的 PC、MEM 内部 busy 请求和该拍获准 Rule。tick 从 0 开始，第一条无停顿指令在 tick 4 结束退休。RuleId 1～5 分别对应 IF～WB。

也可以调用：

```python
from examples.riscv.model import build_cpu

cpu = build_cpu('''
    addi x1, x0, 7
    sw x1, 0(x0)
    lw x2, 0(x0)
    add x3, x2, x1
    halt
''', memory_latency=3)
cpu.run(max_cycles=1000, trace=True)
assert cpu.register_values()[3] == 14
```

CLI 在退休时调用独立解释器比对；`CPU.run()` 只运行 CPU，适合外部测试自行观察结果。超过周期上限报告超时，不把未 HALT 当成成功。

## 结构与阅读顺序

```text
程序 ROM → Fetch → IF/ID → Decode → ID/EX → Execute → EX/MEM → Memory → MEM/WB → Writeback
              ↑                               │                  │                │
              └──────── redirect ──────────────┘                  │                └→ 寄存器堆
                                              │                  └→ 数据存储、busy
                                              └→ 路径代号、停止取指

只读旁路：MEM/WB → ID；EX/MEM、MEM busy、MEM/WB → EX
```

| 文件 | 阅读内容 |
| --- | --- |
| [model.py](model.py) | 实例化、Queue 连接和静态资源绑定；CPU 观察接口 |
| [fetch.py](fetch.py) | PC、顺序取指、重定向 |
| [decode.py](decode.py) | 译码、寄存器读取、WB→ID 旁路、load-use 停顿 |
| [execute.py](execute.py) | 前递优先级、ALU、分支和 HALT |
| [memory.py](memory.py) | 数据 Queue array、延迟请求和完成 |
| [writeback.py](writeback.py) | 寄存器写回、退休记录 |
| [stage.py](stage.py) | 共享的 begin/complete/abort 和显式读取辅助函数 |
| [records.py](records.py) | 有字段名的固定不可变记录，对应 C++ struct |
| [isa.py](isa.py) | 汇编器和流水 CPU 译码器 |
| [reference.py](reference.py) | 独立顺序解释器，直接解析指令字 |
| [test_model.py](test_model.py) | 完整程序、退休比对和流水时序断言 |

五个阶段共 5 条 Rule。它们通过 `Stage.Work()` 进入各自的 `work_stage()` 成员函数。除了 MEM 显式接收当前 tick 参数，其他阶段只依赖读取的 Queue 和固定配置。调度器不分析这些函数。

`observe(q)` 登记实际依赖后纯读 current；`take(q)` 在输入存在时提出 pop，空输入返回 None。两者都不提前改变状态。`work_stage()` 用布尔返回值显式报告候选是否准备完成：False 调用 abort，True 调用 complete；是否获准仍由后续仲裁决定。输入存在但操作数未就绪，是普通的 `if not ready: return False`；丢弃错误路径指令则在提出 pop 后返回 True。

这个布尔值是模拟生成代码的 Work 完成状态，不是未来 ACPy Rule 的业务返回值，也不是 CPU 的额外信号或持久状态。正常完成且没有 proposal 的路径仍然不 firing。具体调用均在阶段文件中可见。

## 指令与执行边界

支持 `ADD/ADDI/SUB/AND/OR/XOR/SLT/LUI/LW/SW/BEQ/BNE/JAL/JALR`。语义参照 [RV32I](https://docs.riscv.org/reference/isa/v20260120/unpriv/rv32.html)，采用 32 位截断、字节地址、对齐字访存和固定为零的 x0。

- `NOP` 展开为 `ADDI x0,x0,0`。
- `HALT` 使用 EBREAK 编码 `0x00100073`，本实验把它约定为停机。
- 汇编支持标签、`#` 注释、x0～x31、十进制／十六进制立即数，以及 `lw x1,4(x2)`、`jalr x0,0(x1)` 形式。
- LUI 接受无符号 20 位立即数；分支／JAL 的数字目标表示相对当前 PC 的字节偏移，标签自动解析。
- `.word` 可插入指令字，供错误路径和非法编码测试使用。
- 程序 ROM 只读；数据存储独立，默认 256 个 32 位字。所有数据初始化为零，由程序执行 store 更新。
- 实际执行非法编码、未对齐目标／访存或越界数据访问会终止仿真。错误路径中的非法编码先随流水传播，在 EX 确认路径有效后才报错。

这是明确限定指令与环境的教学模型，不是完整 RV32I 合规实现。不包含 CSR、特权级、精确异常、中断、缓存、MMIO、非对齐访问或自修改代码。

## 时序和前递

四个级间 Queue 均为容量 1。正常无冒险程序每拍退休一条指令；第一次退休在 tick 4。所有旁路读取本拍 current，不依赖 Module 的执行顺序。

ID 保存寄存器操作数，并提供同拍 WB→ID 旁路。EX 优先观察最近的 EX/MEM 生产者，然后观察 MEM busy 中的未完成 load，再观察 MEM/WB；没有匹配时使用 ID 保存的操作数。匹配到尚未完成的 load 时必须停顿，不能使用更旧的同名寄存器值。

紧邻 load-use 在 ID 插入一拍气泡；长访存还会在 EX 等待结果。测试覆盖源寄存器反复覆盖、64 种短依赖窗口、长存储背压及模块顺序反转。EX 不额外读取寄存器堆；这一取舍依赖本模型的顺序执行、WB 无外部背压及声明的缓冲布局，不据此保证任意多发射模型。

分支按顺序路径取指，在 EX 判断。taken branch 的 Rule 原子更新路径代号并产生 redirect；下一拍 IF 可消费 redirect 并取目标指令，ID/EX 丢弃旧路径指令。MEM/WB 中较老指令继续完成。默认一拍配置的定向程序验证两个错误路径槽位和两拍分支气泡。

HALT 在 EX 停止取指并使年轻指令失效，在 WB 退休。错误路径 HALT 不得停止 CPU。

MEM 延迟 1 时直接完成。延迟 L>1 时，开始事务消费 EX/MEM 请求，将指令和 `due=t+L-1` 保存到 busy Queue，并请求延迟 L-1 的事件；完成事务处理存储并写入 MEM/WB。开始事务没有容量相关 push，因此其计算 tick 就是获准 tick。完成事务后下一拍可以接收新请求。

多拍模式多一个显式请求保存槽位：busy 工作时 EX/MEM 仍可保存下一条指令；它不再等同于所有级间寄存器一起冻结的流水。等待由持久 busy 状态和已发布事件表达，见 [框架问题记录](findings.md)。

## 验收

14 个 CPU 测试方法均从汇编程序运行到 HALT 或明确错误；含 64 种依赖窗口、8 个随机种子、1/3/5 拍访存和缓存／构造顺序对照。

顺序解释器自行解析位域和执行算术，不调用 CPU 的 decoder、ALU、Rule、Queue 或调度器。每次退休比较 PC、指令、写寄存器和 store 效果，并检查全部寄存器。存储可能在 MEM 先于退休更新，因此全部内存只在程序结束时比较。

时序断言另行覆盖首条延迟、连续吞吐、load-use、分支冲刷和长访存的 busy 生命周期。缓存开启／关闭结果与退休拍数一致；背压测试还确认存在未重新 Work 而直接获准的候选。汇编器另有已知编码的端到端断言。

问题、实现修正及仍未定义的前端行为见 [findings.md](findings.md)。
