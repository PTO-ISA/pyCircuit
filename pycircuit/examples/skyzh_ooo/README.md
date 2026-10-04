# Queue 版 skyzh 乱序 CPU

完整 ACPy CPU 在 [model.py](model.py)，由 MLIR 编译链生成 GFSim C++。它使用 Queue 空满表示占用，pop/push 表示分配、发射、流水传递及反压。架构正确性以[独立 RV32I 解释器](oracle.py)为准；固定 skyzh 上游的已知错误继续记录在 [findings.md](findings.md)。

## 结构

- 12 个可用项的 ROB FIFO，10 个容量 1 的保留站 Queue（4 整数、3 Load、3 Store）。
- 单条原子分配 Rule 同时消耗前端输入、push ROB 和选中槽位，并更新 RAT。
- 寄存器重命名、结果表唤醒、各执行通路选择最老就绪指令，允许乱序发射／完成；ROB 顺序提交。
- Load 使用地址、读取、返回三阶段。所有较老 Store 提交之后才读内存，不做访存推测或 Store 转发。
- 分支在提交时恢复；epoch 标记旧路径，原消费者清理旧消息。64 项局部预测表，每项两位历史、四个两位计数器。
- 默认 256 KiB 数据内存，小端字节／半字／字访问；非对齐访问记 fault。测试通过 EBREAK 或 `0x30004` MMIO Store 停机。
- 不实现 CSR、特权态、M/A/C 扩展或缓存。

[runner.cpp](runner.cpp) 只负责装载机器码、驱动、观察和计时；译码与全部硬件行为来自 ACPy。没有专用于 CPU 的编译 pass。

## 构建与验收

先按[编译器说明](../../README.md)构建。LLVM 汇编器需要 `clang`、`ld.lld`、`llvm-objcopy`。

```bash
ctest --test-dir /tmp/acpy-mlir-build -R acpy-skyzh-queue-cpu --output-on-failure
# 同一检查可独立运行：
python3 -m pycircuit.examples.skyzh_ooo.verify \
  --compiled /tmp/acpy-mlir-build/examples/skyzh_ooo/acpy-skyzh-compiled \
  --emitted /tmp/acpy-mlir-build/examples/skyzh_ooo/acpy-skyzh-emitted \
  --no-opt /tmp/acpy-mlir-build/examples/skyzh_ooo/acpy-skyzh-noopt \
  --output /tmp/acpy-skyzh-evidence
```

5 个完整程序，每次提交核对 PC、指令、寄存器写入、Store、下一 PC 和 fault，最终核对全部架构寄存器及所有变化的内存字节。每程序再运行写回周期性阻塞变体；三个生成版本 × 缓存开关 × Module 正反序共 120 次完整运行，要求逐拍轨迹一致。

验收明确断言 ROB 填满、乱序发射与完成、槽位反复复用，以及恢复时存在在途消息。语义测试还覆盖输出前向引用、独立容量、别名消费去重、部分效果撤销、动态字段 revise、Signal 通知和同拍 pop/push。

旧表达缺口保留自然写法作为回归：

```bash
export ACPY_MLIR_COMPILER=/tmp/acpy-mlir-build/mlir/acir-compile
export ACPY_CXX=/home/lc/opt/pycircuit-dev/bin/c++
python3 -m pycircuit.examples.skyzh_ooo.diagnose
python3 -m pycircuit.examples.skyzh_ooo.check_components
```

## 原生参考与性能

参考固定于 `8989a09c357a69b68612f653380d60816f5176c2`，不修改上游核心。

```bash
git clone --branch out-of-order https://github.com/skyzh/RISCV-Simulator.git /tmp/skyzh-riscv-reference
git -C /tmp/skyzh-riscv-reference checkout 8989a09c357a69b68612f653380d60816f5176c2
python3 -m pycircuit.examples.skyzh_ooo.reference.build \
  --source /tmp/skyzh-riscv-reference --output /tmp/skyzh-mlir-reference \
  --cxx /home/lc/opt/pycircuit-dev/bin/c++
python3 -m pycircuit.examples.skyzh_ooo.verify_reference \
  --runner /tmp/skyzh-mlir-reference/skyzh-reference --source /tmp/skyzh-riscv-reference
python3 -m pycircuit.examples.skyzh_ooo.benchmark \
  --generated /tmp/acpy-mlir-build/examples/skyzh_ooo/acpy-skyzh-compiled \
  --reference /tmp/skyzh-mlir-reference/skyzh-reference \
  --output /tmp/acpy-skyzh-benchmark
```

`verify_reference` 对 AUIPC、SRAI、LB、重叠访存的已知差异报告失败，这些差异不定义新 CPU 的预期语义。benchmark 仅采用双模型都通过独立解释器的 window/branches 程序，循环扩展到 4096 次。

两边统一 Clang 22、C++20、`-O3 -DNDEBUG`，只计时预先验证的固定 N 次 `step()/tick()`，构造、装载、宿主结束检查和快照都在计时外。K=0，两边都从初态开始；新模型首次 Signal 初始化计入。每版先预热一个进程，再串行轮换七次，固定 CPU，报告周期、IPC、ns/tick、架构指令/秒和进程峰值 RSS。

上游 ROB 实际可用容量是 7，内存为 4 MiB；新模型为 12 和 256 KiB，流水结构也不同。因此结果是两个完整 CPU 模型的比较，不能据此推导 GFSim 调度器自身的加速比。验收摘要见 [results.json](results.json)，本次记录见 [MLIR 报告](../../mlir/results.md)。
