# 生成式 Ripes5 验收样例

五个普通 Module（Fetch、Decode、Execute、Memory、Writeback），各有显式 `work_*`，由 runtime 统一仲裁。PC、四组流水状态、32 个寄存器、数据表项、退休和 store 事件全部使用只允许 revise 的单元素 Queue。流水链接沿用当前 Python 的寄存器语义，不是消费式 FIFO。

`ex_result` 和 `load_use_stall` 是 runtime Signal，helper 只读取声明的 Queue。`logic.hpp` 为纯译码/计算，`stages.hpp` 为受分支保护的业务读取和 proposal，`model.hpp` 只构造状态与绑定。资源数量在构造后固定；每个数组项都有独立 QueueId。业务方法只读 current、提出 proposal。

本目录根部保留 `model.hpp`、`stages.hpp`、`logic.hpp` 三个微架构描述文件；完整 [ACPy 生成版](../../../../pycircuit/README.md) 与它独立比较。[tests/](tests/) 保存宿主 runner 和对照工具。汇编、输入验证、轨迹、终止条件与计时属于 testbench；`tests/runner.cpp` 通过编译宏选择手写或生成模型，观察逻辑共用。复用 Python 的汇编器、13 个程序和比较器；支持范围与 [Python Ripes5](../../../experiment/examples/ripes5/README.md) 相同，包括固定版本 Ripes 的 JALR 行为。

```bash
python3 gfsim/cpp/examples/ripes5/tests/verify.py \
  --cpp-runner reference/builds/gfsim-release/gfsim-ripes5
```

验收依赖固定原生参考，不提供跳过参考的正式成功模式。`--runner` 可指定其路径；`--case` 只用于定位问题，报告会标为部分验收。

默认结果保存在根目录 `reference/benchmarks/ripes5-handwritten/`：每个程序包含 JSON 输入、数值输入、C++/Python JSONL、原生原始 JSONL 和 stderr。每个输入测试 Module 正/反序，共 26 配置；Python 历史引擎使用其默认缓存模式作参考。比较周期、五阶段、前递、stall/flush、寄存器、数据、退休及 store；只移除原生 `raw` 诊断字段，不移动周期。失败文件保存第一个不同字段、相邻三拍和完整输入指令。

`runner.cpp` 只依赖标准库，从 stdin 接受十进制无符号数流：

```text
2 max_cycles end_pc data_base reverse word_count data_count
words[word_count]
registers[32]
data[data_count]
```

首个数是协议版本。两个区域各限制至 2^20 个元素，所有字值为 uint32；runner 拒绝截断、超范围、尾随内容和不合法 marker。默认输出逐拍 JSONL；`--benchmark` 只输出一条计时/计数 JSON。Python 在转换前调用共享 `validate()`。

## 固定周期三方测速

共用 runner 新增 `--benchmark-fixed K N`：从相同输入执行 K 拍预热，随后仅计时 N 次 `step()`，输出窗口退休增量和最终状态；要求 N>0 且 K+N≤输入 max_cycles。输出轨迹字段与 `--benchmark` 模式保持一致；数字输入须使用 v2。当前 ACPy 生成版／手写 GFSim／原生 Ripes 的统一构建、正确性门槛与 15 次轮换采样见 [测速说明](../../../../pycircuit/examples/ripes5/README.md)。
