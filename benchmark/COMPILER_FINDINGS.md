# pyCircuit 移植发现与优化机会

以下只记录实际证据；源码改写、当前设计边界、编译器问题和数据问题分开。
最初移植只记录失败；后续编译器修复与逐层定位见
[2026-10-19 编译器审计](findings/compiler-audit-20261019.md)。
未提高分析上限、缩短oracle或增加按题目名称识别的路径。
具体复现和源码/工具/测试台哈希见 [VerilogEval](findings/verilogeval.md) 与
[ResBench](findings/resbench.md)；各运行目录的 `results.jsonl` 保留完整阶段命令。

| 类别 | 已观察事实 | 通用改进方向 / 当前处理 |
| --- | --- | --- |
| 作者写法 | 初版有单字段Struct、寄存器引脚式写法 | 已改为标量返回、Module变量和顺序Rule；不是compiler bug。全局Rule和嵌套Rule均已编译验证。 |
| 前端类型上下文 | 条件Integer与Bits位运算丢失来源，循环ordinal比较未使用已知事实 | 已在已有二元运算入口归一化并复用完整范围证明；不要求额外Struct或typed local。 |
| 前端运算准入 | 旧static-count检查挡住已有后端动态shift能力 | 已支持固定Bits运行时计数，保留完整计数宽度与X/Z规则；动态数学Integer仍不支持。 |
| 前端SCF准入 | Rule循环已支持的部分运算，在普通helper循环仍被旧白名单拒绝 | 已补齐helper的shift/div/rem准入与运算形状，继续复用普通expression lowering；两条路径分别测试。 |
| MLIR分析/表示 | vector100r 的concat等价循环在第64次迭代触及Table-query常量空间限制 | 需要基于实际IR和重复常量/类型来源工作的最小复现分析；只记录瓶颈，未以增大限制或删证明“修复”。 |
| RTL导出/生命周期 | 17个普通隐式状态程序成功compile/link/dual-emit，但根接口仍含phase、commit许可和完整性/错误通道 | 这是当前普通event-driven RTL接入缺口；不能仅连接clk/reset或固定commit=1。需独立设计通用硬件导出/调用协议，保留原时序和全系统原子检查。 |
| codegen外部命名 | 根名固定 `pyc_root`；native根名称曾未进入Python receipt；内部编码保证无碰撞 | 已补回receipt的 `root_rtl_name`。完整端口/domain映射及别名接口尚未实现；保持内部编码。 |
| generic路径差异 | mapping-return静态参数可用，scalar-result行为Module显式拒绝静态参数 | 保留为独立未修复项，不用单字段Struct或旧mapping写法掩盖前端缺口。 |
| 验证适配 | 初版只支持Struct返回，并假设ResBench顶层叫module_tb | 已支持直接标量，并按原测试台确定顶层；width/order/private-controls检查及负对照已覆盖。与compiler错误分开。 |
| 数据质量 | bugs_mux2描述/公开oracle的选择方向不一致；ResBench部分header缺终止符 | 明确标为oracle-informed修订/声明规范化；上游测试台字节不变，不宣称无辅助模型成绩。 |
| 算法语义 | mod_exp有符号余数、AQI Q0.8系数和最初假设不同 | 已修正通用算法并复跑原测试；不是后端wrong-code证据。 |

## 证据纪律

每项新增问题至少记录触发题目、source hash、命令、首个失败阶段和完整诊断。
若能定位代码，则分别写观察与推断；不能仅凭“compile失败”就归因MLIR或codegen。
记录改写前后的算法、接口和时序是否一致。性能优化只列机会；没有同输入、同工具、
同worker配置的对照测量，不宣称速度提升。

优先保持可读的前端，把类型/调用图、状态、guard、proposal/commit及输出可用性
留给ACIR。无法保持原语义的接口明确unsupported；不在作者代码中模拟后端生命周期。
