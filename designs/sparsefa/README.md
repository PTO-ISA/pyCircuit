# SparseFA / FlashAttention2 indexed sparse accelerator

本目录包含L0–L3 NDF、P0S串行硬件源码、P0T独立阵列源码和独立测试。Index由外部提前产生：设备先读
count和index，按原token直接读取线性KV cache，再执行online softmax和PV；不计算
indexer/top-k，不宣称兼容完整DeepSeek MLA/paged/量化cache ABI。

新增的 [Shared-KV多Head目标](ndf/l1/fa2-core-skv-0002.md) 将 K 和 V 定义为
所有head共享的同一份数据，运行时配置head数和KV维度；
[64项Index/Gather与双缓冲设计](ndf/l2/fa2-core-skv-1005.md) 及
[新泳道图](ndf/l2/fa2-core-skv-swimlane-1005.md) 描述传输/计算重叠。
这些是待实现架构，下面的P0S性能和验证结果不适用于新路径。

## 当前实现

```text
TensorCommand → B/H/query scheduler → IndexCount/Index完整前缀验证
              → Q装载 → 原token causal筛选 → 直接K/V地址
              → QK / online softmax / PV影子bank → normalize → O写确认
```

P0S支持D1..64、FP16/BF16、外部uint32索引列表最多2048项、dense/indexed、正负
causal origin、独立六类tensor的byte stride、单outstanding访存和cancel drain。
数值模块使用真实位级binary32 RTZ/FTZ运算，不以host Oracle生成DUT结果。
完整tensor调度、行引擎和计算模块均已编码；实际compile/link/C++运行结果见
[VALIDATION.md](VALIDATION.md)，不能由源码存在推断仿真或性能已经通过。

线性索引校验基线的六个完整System已在默认编译预算下通过C++ workers=1/2，C++与Verilog由各自
同一份verified final生成。Verilog按用户要求只生成、不仿真；通用编译器gate仍有
一项Python/Unicode版本环境限制，详见验证记录。

当前P0S索引优化候选增加32KiB精确Patricia树，主要数据存储合计40.75KiB。每次
树遍历最多32层，外部列表原顺序和错误优先级不变。整机已通过默认预算链接、同一final
双目标生成及C++双worker共46项索引检查。D1、2048项降序有效列表的命令完成周期由
4,227,083降至88,051，约48.0倍；小列表及深路径存在变慢用例。五个完整System仍在
链接时超过证明预算，详见[当前优化记录](VALIDATION.md)。逐token/逐维计算和单outstanding
总线仍保留，上述结果不是D64、阵列吞吐或物理频率验收。

[P0T 当前三维阵列](ndf/l2/fa2-core-array3d-1002.md)按 `M×N×K_lane=16×16×16`
展开：256 个输出 cell，每个 cell 有 16 个独立乘积寄存子模块，共 4096 路空间乘法。
乘积经过四级平衡加法树，再进入跨组累加器；六个寄存边界都由实际数据状态描述。
支持 K 尾部、row/column/K/cell mask、组号校验、排空、整体保持和取消。
全部五个 source unit 编译通过；完整 System 链接仍超过默认事务分析预算，
尚无生成 C++ 的阵列执行结果。阶段记录见 [VALIDATION](VALIDATION.md)；
独立参考模型通过不等于生成硬件执行通过。

无停顿的设计合同为组 II=1；K=64 时接收 4 组，从 begin 到 take 为 10 拍，下一 tile
的 begin 间隔为 11 拍。每组片上输入需要 2 KiB，算术数据寄存容量为 32 KiB。
这些是设计值，尚无物理频率或已测阵列吞吐。
[供数 buffer 方案](ndf/l2/fa2-core-array3d-buf-1004.md)另列，SRAM/gather/softmax
和完整 attention 调度仍待集成。外部 index 的生成责任不变。

[此前的二维阵列](ndf/l2/fa2-core-array-1002.md)及其测试保留为独立基线。
三维平衡树改变了浮点加法顺序，使用[新的独立测试](tests/array3d/README.md)，
不能沿用二维逐项累加的逐位期望。
冻结的L0基线保持原文，L1/L2与参数仍为draft；尚无综合频率、面积或加速比。

## 设计入口

- [NDF总索引和实现映射](ndf/README.md)
- [P0S接口与数值合同](ndf/l1/fa2-core-profile-0001.md)
- [整机模块连接](ndf/l2/fa2-core-eng-0001.md)
- [P0S 数据流泳道图](ndf/l2/fa2-core-dataflow-swimlane-1001.md)
- [Buffer规格/布局/端口](ndf/l2/fa2-core-buf-1001.md)
- [IndexReader、校验和直接AGU](ndf/l2/fa2-core-idx-1001.md)
- [精确Patricia树与逐拍插入](ndf/l2/fa2-core-idx-1003.md)
- [标量传输协议、取消与drain](ndf/l2/fa2-core-xfer-1001.md)
- [共享算术微架构](ndf/l2/fa2-core-comp-1001.md)
- [16×16×16 三维阵列](ndf/l2/fa2-core-array3d-1002.md)、[供数布局](ndf/l2/fa2-core-array3d-buf-1004.md)和[逐位/周期验证](ndf/l3/fa2-core-array3d-1003.md)
- [描述符、tensor/row FSM与错误码](ndf/l2/fa2-core-ctrl-1001.md)
- [源码构建说明](src/README.md)和[验收矩阵](ndf/l3/fa2-core-eng-1001.md)

## 本地检查

独立数学/地址模型，无compiler依赖：

```bash
python3 -m unittest discover -s designs/sparsefa/tests -t . -p 'test_*.py' -v
```

生成硬件的各组System使用当前checkout构建的toolchain：

```bash
python3 designs/sparsefa/tests/numeric/run.py
python3 designs/sparsefa/tests/numeric_exact/run.py
python3 designs/sparsefa/tests/integration/run.py
python3 designs/sparsefa/tests/scheduler/run.py
python3 designs/sparsefa/tests/array/test_oracle.py
python3 designs/sparsefa/tests/array/run.py
python3 -m unittest designs.sparsefa.tests.array3d.test_oracle -v
python3 designs/sparsefa/tests/array3d/run.py
```

每组经compile/link/同一final双emit/C++运行；按用户要求，本次不执行Verilog仿真。完整命令、退出码、
hash与未运行项存放在忽略的gate目录；测试不依赖本地管理日志。历史16-entry/map
原型和只做字符串检查的测试已移至 `historical/integer_prototype/`，不属于当前硬件。
