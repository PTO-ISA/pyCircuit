# FA2-CORE-ENG-1001：P0S 端到端验收与证据层次 {#FA2-CORE-ENG-1001}

<!-- ndf: kind=verification modality=must refinement=L3 status=draft owner=flash_attention2 refines=FA2-CORE-ENG-0001 verifies=FA2-CORE-0001,FA2-CORE-PROFILE-0001 -->

## 固定合同

验收参数引用 [P0S profile](../l1/fa2-core-profile-0001.md)，不在L3重新定义算法或
容差。必须区分数学模型、真实生成C++、同一final生成RTL和物理实现四类证据。
源文件数量、字符串测试、compile或静态库构建均不等于attention行为通过。

## 当前测试入口

| 目录/入口 | 被测对象 | 期望来源 |
| --- | --- | --- |
| tests/test_oracle_attention.py | dense/indexed数学与分块递推 | 手算、独立稳定softmax |
| tests/test_oracle_gather.py | uint64地址/布局模型 | 手算地址、边界构造 |
| tests/numeric/run.py | 真实fp_add/mul/div、转换、exp | host binary32/64及预先声明界限 |
| tests/numeric_exact/ | 基本浮点RTZ逐位验收 | 独立有理数计算，非DUT位运算复制 |
| tests/integration/run.py | 行引擎sparse/dense、错误/取消、D64/BF16 | 手算地址、binary64 softmax |
| tests/scheduler/run.py | 完整tensor调度、布局、恢复 | 显式B/H/query地址集合和写ack计数 |
| tests/scheduler/run_guard_order.py | 原始分支与嵌套guard Rule的控制语义 | 独立0/1/X/Z真值表、状态保持与全局失败原子性 |
| tests/scheduler/run_controller_error.py | 原始ControllerTransition的错误响应优先级 | COUNT/INDEX的error=1与data=X/Z；合法/越界、无响应和cancel对照 |

bench是pyCircuit System；DUT不读取Oracle结果。编译器生成System runner和顶层
clock/reset；不得手写旁路来规避System不支持的语义。测试的expected载荷不能连到
DUT计算输入上代替Q/K/V读取。

## 端到端矩阵

| 场景 | 必须验证 | 对应实现/补充 |
| --- | --- | --- |
| 非零score sparse | softmax只归一化所选且未mask集合 | integration |
| BF16多维 | 非融合dot、符号/不同V维、输出cast | integration vectors |
| D64 singleton | 全64维Q/K/PV/写回；O等于所选V | integration vectors |
| B2/H2/Sq2 | 每行地址/完成数量，最后写ack先于全局完成 | scheduler tensor |
| 负causal origin | 早期query全零，不发生KV访问 | scheduler tensor |
| Index空/错/重复/越界 | 正确错误码，KV/O请求数量，下一操作隔离 | integration |
| REQUEST/RESPONSE背压 | 请求稳定、最多一个outstanding、恰好一次写 | integration/scheduler |
| cancel后恢复 | 先drain后完成，O整体无效，后续操作不受旧响应污染 | integration/scheduler recovery |
| 六类访存错误 | COUNT/INDEX/Q/K/V/O各响应code5；O失败允许部分已发写，不能code0 | integration faults |
| 非法Q/K/V | 非有限或超域元素code7；V第二维错误不发布部分shadow；随后命令可重启 | integration faults |
| completion背压 | tag/code/valid保持，无重复终止 | integration |
| 最大规模与对抗数值 | D64/count2048、消去、反复max更新、全索引置换 | 全域验收，按实际执行记录 |
| reset/X/Z | 标准reset和bus fence；未知控制不非法提交 | C++四态控制用例与整机用例分别记录；本次不执行RTL仿真 |

最后两行不能从小样例推断通过。随机seed和样例数必须记录，空跑或assert从未被触发
均失败。仿真包含超时终态断言，确保不是等待到固定周期后悄悄结束。

握手Rule重组的专项用例枚举外层/内层guard各取0、1、X、Z的16种组合，分别运行
原始分支与嵌套Rule。外层X/Z即使内层为0也必须失败；外层0必须跳过内层X/Z；
只有两个guard都为1时更新状态。失败还必须阻止无关计数器提交和仿真epoch推进。
这项检查不替代完整attention数据路径或整机reset覆盖。

控制器专项用例直接链接原始ControllerTransition，C++适配器只提供输入和独立期望。
COUNT/INDEX等待阶段收到有效错误响应时，data为X/Z仍须输出phase16/code5；无错误但
载荷未知的对照必须保留未知结果，取消则优先输出code6。该纯组合模块检查不替代
行引擎的状态提交、drain或整机错误恢复验收。

## 执行和记录

独立source-unit编译、完整closure link，再从同一final进行双emit。记录每个source、
工具链、final和输入hash；C++ workers1/2比较；本次按用户要求不执行Verilog仿真，单独标记NOT RUN，不作为交付门槛。
命令/退出码/原始日志保存在忽略目录 `docs/gates/logs/fa2-p0-completion/`；测试本身
不依赖该目录。源码和测试只依赖正式compiler/Runtime接口。

当前实际结果汇总在设计根目录 [VALIDATION.md](../../VALIDATION.md)。旧integer
prototype的历史compile/link不能更新当前P0S的状态。L1/L2保持draft，直到全部必要
行为与全域数值验收完成（本次用户已排除RTL仿真）；文档完整不代表产品已经达到性能/流片条件。
