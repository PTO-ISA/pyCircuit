# FA2-CORE-PROFILE-0001：P0S 可执行硬件合同 {#FA2-CORE-PROFILE-0001}

<!-- ndf: kind=requirement modality=must refinement=L1 status=draft owner=flash_attention2 refines=FA2-ADR-001,FA2-ADR-002 -->

## 定位与参数

P0S 是串行功能实现；一个 tensor command 驱动一个 row engine，row engine 每次处理一个
query、一个 token 和一个维度。它保持 online softmax、有限片上存储和外部索引语义。
P0T 是独立的并行阵列候选，当前方向为
[16×16×16 空间展开](../l2/fa2-core-array3d-1002.md)；其平衡树数值顺序和寄存流水
独立定义，不能用 P0S 仿真证明 P0T 的吞吐、SRAM 或流水线。
L0 原文保持不变；本合同和对应 L2 仍为 draft，不把源码存在或一次测试通过视为冻结。

| 项目 | P0S 的具体值 |
| --- | --- |
| 计算 | 推理前向；无 dropout；`D_k=D_v=D` |
| B、H、S_q | 非零 uint16、非零 uint16、非零 uint32；逐行循环，无相应规模的片上分配 |
| D | 1..64；尾维按实际 D 执行 |
| S_k | uint32；dense 0..2048；indexed 由实际地址空间约束 |
| index_count | 0..min(2048,S_k)；完整有效前缀唯一且每项 `<S_k` |
| Index 分配 | 每行固定容量 2048×uint32；读取有效前缀，不读 padding |
| Index 校验 | 外部输入；精确Patricia树，4096×64节点；保留原顺序及错误优先级 |
| dtype | FP16 或 BF16，同一次调用的 Q/K/V/O 使用相同 dtype |
| Q/K/V 元素 | 解码并 FTZ 后有限且绝对值≤8；NaN/Inf、超限值终止操作 |
| scale | 传入 binary32 位模式，正 normal 且≤1；无硬件 sqrt |
| 数据布局 | 64-bit byte address；元素连续，d stride=2；独立 tensor/axis stride |
| 片外传输 | 单个请求通道、最多一个 outstanding；读写均须响应；2/4 字节标量事务 |
| 并发 | 一个 tensor operation；一个 query；一个 token；无跨操作重叠 |
| mask | none 或 causal；使用原 token ID，支持负 query origin 的空集合 |

零 key 长度或零有效集合产生零输出。indexed 模式下 `S_k=0` 只允许 count=0；非空
列表先由count校验拒绝。无序索引按输入顺序累加；不排序、不选 top-k、不执行 indexer。

## 对外接受与地址责任

`command.valid & command_ready` 表示描述符被捕获。非法描述符也可以被捕获，但随后
只报告错误完成，不产生访存；这与“接受执行”分开。command 被捕获后调用方可以改变
端口上的下一个 command。操作只能在成功 completion 被接受后使用 O。

硬件检查维度、对齐、内部布局约束和 64-bit 算术溢出。乘加采用 128-bit 临时值，检查
end-exclusive≤2^64 后才使用低 64-bit 地址。O 行/head/batch 必须互不重叠；只读输入
的广播支持以当前 descriptor validator 的明确规则为准，不能把 O 的零 stride 当作广播。

本接口没有 IOMMU、allocation length 或内存访问权限表。调用方须保证所有 descriptor
指定区间可读/可写、O 与所有输入不别名、输入在结束/drain 前不可变。地址不回绕不等于
地址获授权；bus.error 报告实际访问失败。Index 每行分配 8192 字节是布局约束，实际只
读取 `4*count` 字节。线性 cache 无 token→physical-address 查表。

## 精确的数值执行语义

`fa2_numeric.py` 用整数位运算实现浮点编码的加、乘、除和转换；`u32` 是 binary32
位模式，不是定点/整数 attention。所有中间结果保持 24-bit significand、8-bit exponent。

1. FP16/BF16 输入的 subnormal 转为同号零；normal 精确扩展到 binary32。
2. add/mul/div 在每次运算后向零截断（RTZ）；binary32 subnormal 结果 FTZ；溢出饱和
   至最大有限值。profile 范围内正常 attention 不应触发饱和。
3. dot 按 d 从小到大执行非融合 `add(sum,mul(q,k))`，完成后乘 scale。
4. exp 输入非正。`x≤-16` 输出零；否则令 y=x/256，计算
   `p=1+y*(1+y*(1/2+y*(1/6+y/24)))`，再连续平方八次。系数以源码中 binary32
   常量为准，每个 add/mul 独立 RTZ/FTZ。
5. online 更新顺序为 `m'=max(m,s)`、`a=exp(m-m')`、`b=exp(s-m')`、
   `l'=add(mul(l,a),b)`、`acc'[d]=add(mul(acc[d],a),mul(b,v[d]))`。
   首 token 使用 a=0，m'=s；空行不做除法语义提交，输出零。
6. normalize 使用 `div(acc[d],l)`；输出 FP16/BF16 按 RTZ 转换，目标格式
   subnormal FTZ。FP16 溢出饱和有限值；合法 attention 的输出处于 [-8,8] 附近。

上述定义是确定的执行规格，不是完整 IEEE-754：不提供异常标志、RNE、NaN 传播、
无穷运算或 fused multiply-add。不能将其标成常规 GPU FP32 等价实现。

## 精度验收边界

基本算术须相对独立精确有理数 RTZ 参考逐位匹配（signed zero 按上述定义）；转换须
覆盖零、normal 边界、FTZ、符号与溢出。exp 对 [-16,0] 的绝对误差目标为 5e-5，
`x=-16` 的被截尾真值纳入误差；这不是 1024 ULP 在任意指数下都相同的误差。

端到端设置两类判据：首先逐操作符合上述执行语义；其次与独立 binary64 softmax
比较，并分别记录 dtype、规模、绝对/相对误差和最坏输入。基本 bring-up 样例的
FP16 目标为 atol=0.02、rtol=0.01，BF16 为 atol=0.07、rtol=0.02。
这些是预先声明的测试目标，**尚非整个输入域的证明或 accepted 保证**；不能因为小样例
通过而冻结 profile。完整范围精度验收需要覆盖 D=64、count=2048、累积消去和连续
更新最大值等对抗输入。未满足时应改进数值实现或收窄并明确 profile，不增大容差掩盖失败。

单次 exp 截尾的上界不能直接作为 online recurrence 的总误差：最大值可以反复更新，
alpha 截尾、RTZ 累加和输出 cast 都有传播误差。BF16 的 RTZ 量化本身可能接近 0.0625。
这项风险由 L3 持续验证；当前不宣称全域精度收敛。

## 终止语义

completion 使用原 command tag，保持 valid/code/tag 直到 completion_ready。成功 code0；
code1 描述符、2 count、3 index range、4 duplicate、5 memory、6 cancel、7 operand。
一次操作只发布一次 completion；成功才使整个 O 有效。失败前已确认的 O 写入不能撤销。

cancel 优先于新命令/新请求/计算提交；尚未接受的 request 可撤回，已接受的请求必须
消费响应后才报告取消。无 response tag 的单 outstanding 协议要求环境对每个已接受
请求恰好返回一次，不得自主重放旧响应。复位中止操作后，环境必须 reset/fence bus，
或在恢复命令前 drain 旧事务；硬件不假装能识别无 tag 的任意迟到包。

## 对应设计

- [整机与模块连接](../l2/fa2-core-eng-0001.md)
- [命令和调度](../l2/fa2-core-ctrl-1001.md)
- [存储布局](../l2/fa2-core-buf-1001.md)
- [传输协议](../l2/fa2-core-xfer-1001.md)
- [索引校验与直接地址生成](../l2/fa2-core-idx-1001.md)
- [精确树的布局、遍历与插入](../l2/fa2-core-idx-1003.md)
- [共享计算数据通路](../l2/fa2-core-comp-1001.md)
- [实际验收入口](../l3/fa2-core-eng-1001.md)
