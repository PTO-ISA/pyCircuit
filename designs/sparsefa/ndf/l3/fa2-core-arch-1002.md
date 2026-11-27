# FA2-CORE-ARCH-1002：硬件模块、生命周期和物理边界验收 {#FA2-CORE-ARCH-1002}

<!-- ndf: kind=verification modality=must refinement=L3 status=draft owner=flash_attention2 refines=FA2-CORE-BUF-1001,FA2-CORE-XFER-1001,FA2-CORE-COMP-1001,FA2-CORE-CTRL-1001 verifies=FA2-CORE-BUF-0001,FA2-CORE-XFER-0001,FA2-CORE-COMP-0001,FA2-CORE-CTRL-0001 -->

## P0S 检查点

| 模块 | 必须证明的性质 | 观测 |
| --- | --- | --- |
| Descriptor validator | 对齐、宽地址、zero stride广播、O无重叠 | layout System及非法命令无请求 |
| Tensor scheduler | b/h/query严格覆盖；最后ack后完成 | 期望地址集合、写计数、terminal |
| Index memory | 只读取已验证前缀、0/2048边界 | 请求序列、error及容量场景 |
| Q store | D尾部和新行初始化 | D1/D2/D64结果、相邻行不同Q |
| Shadow accumulator | 未完成token不切换m/l/bank | V中途错误/取消及恢复；必要时内部诊断 |
| Scalar ALU | RTZ/FTZ、符号、normal范围 | 独立逐位算术测试 |
| Exp | 截尾、多项式误差 | 固定样本+边界+连续域分析 |
| Bus owner | 单outstanding、稳定写载荷、每事务一次响应 | bus model断言 |
| Drain | cancel不得提前释放有响应责任的事务 | 延迟response+短cancel脉冲 |
| Completion | 保持tag/code、一次握手、循环回IDLE | 终止背压及后续command |
| Reset | 控制无旧有效性，外部fence前不复用 | 生成System reset/RTL四态 |

源码Memory采用既有抽象，若映射到同步SRAM需补读延迟流水和端口冲突测试。
[三维阵列供数候选](../l2/fa2-core-array3d-buf-1004.md)的每侧16-bank宽读和双缓冲
不属于P0S验收，尚待实现；乱序beat/tag、多并行credit也不能算作已通过。

## 失败与证据完整性

compile、link、双emit、C++构建、C++执行、RTL执行各有独立退出码。任何前置失败
导致后续NOT RUN，不能发布整套PASS。编译器分析预算限制属于编译器问题，必须在
拥有该语义的分析层通用修复；不得提高测试专用限额、改Oracle、用host补算DUT。

compiler更改需要对应native回归和独立评审。独立review绑定具体source hash，旧报告
保留历史身份，不能直接替换hash伪造新候选已review。未运行四态/最大容量/全域精度
测试明确列出。实际状态集中在 [VALIDATION.md](../../VALIDATION.md)。

## 物理实现仍需的独立验收

生成RTL不等于综合/布线。必须另外记录SRAM宏、目标频率、关键路径、DSP/logic面积、
功耗、外部带宽和延迟、稳态吞吐、取消延迟。没有目标平台时不编造PPA数值。串行P0S
用于闭环功能和后续架构比较，不能宣称已获得FA2并行阵列的速度提升。
