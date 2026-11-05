# FA2-CORE-ARRAY3D-1003：3D 阵列结构、算术与流水验证 {#FA2-CORE-ARRAY3D-1003}

<!-- ndf: kind=verification modality=must refinement=L3 status=draft owner=flash_attention2 refines=FA2-CORE-ARRAY3D-1002 verifies=FA2-CORE-COMP-0001 -->

本页验证 [ARRAY3D-1002](../l2/fa2-core-array3d-1002.md) 的新增 16×16×16 阵列。
独立 host 模型、source compile、完整 final、双 emit 和生成执行分别记录。
当前设计合同不是完整阵列已运行的声明；旧 2D、P0S 或缩小微例的通过不能代替
当前 4096 空间乘法位置的证据。按用户要求不运行 Verilog 仿真。

## 所有权与独立性

产品新文件负责 3D 接口、层级、真实流水数据和方法；既有 numeric helper 是唯一
DUT 算术实现。`tests/array3d/` 的独立实例负责 Fraction 数值 oracle、状态模型、
输入场景和生成执行校验。架构/源码审查与产品作者分离，记录实际 agent/model/effort，
不能以角色名称代替发生过的审查。旧 P0S、L0 和 2D 历史文档保持原状。

oracle 不导入产品 source，不执行 Python 设计函数，不把期望 rows、接收次数或
周期答案写入 DUT。输入和期望分别保存；generated System/公开 adapter 只驱动
输入并读取公开输出。管理日志和 review 记录不是测试运行依赖。

## 独立算术 oracle

每个 cell 先独立编码 16 个有限乘积，每层相邻配对并分别 RTZ/FTZ/有限饱和，
四层后按组接受顺序加到 total。保留每次加法的 signed-zero 规则；不用 fused
FMA、host float、重新排序或跨层一次舍入。叶被 mask/tail 排除时为精确 +0；
整个 cell-group 无有效叶时，total 逐位保持。

加法零符号选择使用原始输入 magnitude，tie 取第一个操作数；Fraction 数值的
FTZ 不得擦掉这项原始编码信息。乘法零符号为符号异或。

至少覆盖以下独立手算锚点和向量：

| 情景 | 检查目标 |
| --- | --- |
| 不同行、列、K 操作数 | A[m,k] 与 B[k,n] 索引及右侧按列布局，全部 256 结果 |
| 相邻树与串行累加有不同舍入的向量 | 四层树顺序不可被旧线性 oracle 掩盖 |
| 非融合乘加差异、消去 | 每个乘法/树节点/累加分别舍入 |
| ±0、输入 subnormal、结果下溢、有限饱和 | FTZ 与符号、饱和边界 |
| length=1/15/16/17/31/32/33/63/64 | 组数和最后一组各 K 尾部 |
| 非矩形 cell_mask、变化 row/col/k mask | 有效叶数及数据路径，K mask 不是 per-row membership |
| 全空组/全空 tile、空 cell-group | 组仍退休；total 保持与加 +0 的区别 |
| 无效叶带无关数值或 X/Z | 已屏蔽输入不得影响有效结果；未知控制仍按到达规则处理 |

NaN/Inf 不属于数值合同，不将它们的任意输出计为通过；对有效输入的四态失败
检查与有限数值测试分开报告。host oracle 通过只验证模型自身，不是硬件通过。

## 生命周期与显式推进模型

模型保存五个 group token 槽、各 cell 的阶段数据/参与状态、total、accepted 与
retired。每次成功 advance 先处理匹配 ABORT，否则从旧 s4 开始向上游推进。
GROUP 接受时写产品；END 接受时封闭输入并注入气泡。没有调用时模型完全保持。

返回 actionAccepted 与事务提交分开统计：ACTIVE/DRAIN 中错误 tag/index、
重复 GROUP、提前 END、DRAIN GROUP 均返回 0，但原有 token 继续前进。
必须安排一个拒绝命令与最后退休同边沿，证明拒绝不会卡住 DRAIN→RESULT。
IDLE advance 全返回 0；RESULT 除匹配 ABORT 外返回 0 且保持数据。begin/take
guard 不可用则调用事务不提交，与上述协议返回 0 不同。

首批要求：

- 连续组 II=1，穿插 BUBBLE，完全不调用的 hold，以及两者的差异。
- early END、错误 tag、重复/跳号、非法 length、忙时 begin、提前 take。
- END 早于最后退休、晚于全部退休，以及未 END 时不能取结果。
- 在 product、每个 reduction stage、最后累加边沿和 RESULT 取消；没有旧结果。
- abort 后新 begin、不同 tag 的连续 tile、初始 reset 与中途 reset。
- RESULT 延迟 take 保持全部 256 words 及统计。
- 调用者下游阻塞/检查失败时整个 advance 或 take 原子保持；不能用延迟 take
  替代真实 Queue 背压。若标准 Queue 路径未获支持，报告精确阶段和 NOT RUN。
- X/Z action/tag/index/已到达 mask 的 whole-System 失败和零提交；mask 排除的
  数据 X/Z 不得冒充控制故障。分别记录 unknown 与 unavailable。

测试驱动 tick 必须独立于被拒 begin/take 的事务，避免日程停在错误请求上。
take、公开结果记录及成功 tile 计数放在同一事务，未提交的 take 不得伪造结果。
不能改产品接口暴露私有寄存器以直接注入阶段或给 DUT 填入期望结果。

## 结构与周期证据

保留独立 compile/link/emit 的 source-unit 和 final manifests。最终 verified IR
检查 16 Row、每 Row 16 Dot16、4096 逻辑乘法位置、3840 树加法位置和 256 累加
位置，以及 16/8/4/2/1/1 数据存储布局。source 中实例或 helper 调用计数仅为
审查线索；不能据此宣称 final 硬件结构存在。验证阶段寄存边界而非固定延迟计数。

同一 verified final 生成 C++ 和 Verilog，C++ workers 1/2 比较全部结果、事件
与周期；Verilog 仿真明确 NOT RUN。不得从其他 worktree 拷贝工具链或生成产物。
完整规模默认预算失败时保留失败阶段、预算种类、requested/used/limit 和闭包
identity；不提高预算、不减少 M/N/P_K、不调用 host 计算替代 DUT。

用实际成功 rising-edge commit 标记 begin、每个 advance、actionAccepted 和 take，
不能把低/高 Work sample 计算为两个组。模型应验证：

```text
group acceptance at E -> accumulation at fifth subsequent successful advance
continuous input group II = 1
begin=e0, groups=e1..eG, END=eG+1
last accumulation/RESULT=e(G+5), take=e(G+6), next begin=e(G+7)
```

“第五次后续 advance”可以跨越若干无调用 hold 周期；这与五个绝对时钟周期不同。
记录 accepted groups、rejected actions、bubble advances、no-call holds、RESULT waits、
abort 和完整 tile 数。每个结果比较完整 16×16 words、tag、groups、useful_macs。
useful_macs 是有效乘法叶数；容量 slot=4096×accepted_groups。带 mask 时树加法
次数与有效乘法数不同，不能把二者都记成 useful_macs。

G=4 理想 begin-to-take=10、tile cadence=11 只在连续必要 advance、无外部阻塞且
无取消时成立。实测逐项对照这些前提；host 秒数、MHz、DSP 数、SRAM/片外带宽
和完整 attention 加速均不由周期模型直接推导。

## 结果发布

在 ignored `docs/gates/logs/<run-id>/` 记录确切 source/tool hashes、命令、退出码、
完整 artifacts 路径、独立 review 和所有 NOT RUN。正式文档只更新可复查的状态
摘要，测试不读取本地管理日志。提案审查、源码审查、oracle 通过、编译通过、
生成执行通过逐层报告，不用“已实现”概括尚未通过的完整闭包。

首轮完整闭包的实际结果：五个 source unit compile PASS，默认预算 link FAIL
（transaction plans，requested 768、used 1048555、limit 1048576）。见
`docs/gates/logs/fa2-array3d/20261104T073610.941015Z/manifest.json`。
该轮的 final 结构检查、C++/Verilog emit、workers1/2 全部未运行；七项独立 host
oracle/checker 测试通过不能替代这些阶段。Queue 背压、中途 reset、X/Z 注入仍是
明确未执行项，RTL 仿真按用户要求排除。

首轮冻结输入为 449 条指令、42 个预期成功 tile。后续候选补入独立 length=33
用例，覆盖第三组单叶尾部，成为 459 条指令、43 个预期成功 tile；此前的执行
记录仍绑定旧输入。独立 host 检查另覆盖全部 160 个合法 length/group 尾掩码
组合，两者都不能替代生成硬件执行证据。

新增边界后的最终轮次为
`docs/gates/logs/fa2-array3d/20261104T074254.674027Z/manifest.json`：五个 source unit
compile PASS，link FAIL（requested 512、used 1048269、limit 1048576，transaction
plans）；全部 181 个输入/tool hash 前后不变。host 最终七项测试与 Ruff 通过，
记录在 `docs/gates/logs/fa2-array3d/host-final-k33/manifest.json`。该轮仍无 final、
双 emit 或 C++ workers1/2 执行；所有周期值保持设计合同身份。
