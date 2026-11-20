# FA2-CORE-0001：单次前向调用的可观察行为 {#FA2-CORE-0001}

<!-- ndf: kind=requirement modality=must refinement=L1 status=draft owner=flash_attention2 refines=FA2-ADR-001,FA2-ADR-002 -->

## 目的与范围

本条款把 [FA2-ADR-001](../l0/fa2-adr-001.md) 转换成一次 FlashAttention2 前向调用的
外部行为。它定义逻辑张量、接受与完成、mask、数值和失败语义；不规定 tile 尺寸、
PE 数、片上存储 bank、流水级或请求仲裁算法。

从硬件架构角度，本条款把一次调用拆成五个必须存在的职责边界：`Context` 管理调用
身份，`TileStore` 管理有限片上工作集，`Transfer` 管理逻辑张量与 tile 之间的搬运，
`Compute` 完成 score/softmax/value 累加，`Writeback` 提交输出。它们可以合并为更少的
物理模块，但不能因此产生重复的状态 owner 或绕过 operation/generation 隔离。

本条款的首个候选 profile 是推理前向、无 dropout、`D_k = D_v = D`、有限输入、
FP16/BF16 输入和 FP32 中间累加。该 profile 仍属于 `draft`；如果实现选择其他精度、
训练语义或独立的 key/value 维度，必须同步更新本条款和 L3 Oracle。

下表保留现有 P0S 的独立 K/V 逻辑视图。新的
[Shared-KV 多 Head 目标 profile](fa2-core-skv-0002.md) 明确采用单份
`KV[B,Sk,D]` 同时作为 K 与 V，所有 head 共享它，并按 64 项处理外部 index。
新 profile 不表示现有 P0S 已经具备批量 DMA 或双缓冲。

按 [ADR-002](../l0/fa2-adr-002.md) 增加外部 index 输入和 KV cache gather。dense 与
indexed 为独立选择模式；索引架构由同层 [FA2-CORE-IDX-0001](fa2-core-idx-0001.md)
定义。首版 indexed 候选逐 query 读取其列表并执行，复用计算路径。

## 逻辑接口

一次调用包含以下逻辑对象。它们可以由不同的物理端口、DMA 描述符或片上连接承载，
但物理布局不能改变这些逻辑含义。

| 对象 | 逻辑形状/字段 | 含义 |
| --- | --- | --- |
| `Q` | `[B, H, S_q, D]` | query 向量 |
| `K` | `[B, H, S_k, D]` | key 向量 |
| `V` | `[B, H, S_k, D]` | value 向量 |
| `O` | `[B, H, S_q, D]` | 输出向量 |
| 配置 | `scale`、`mask_mode`、`causal_offset`、`selection_mode` | 缩放、mask 和 KV 选择 |
| `Index` | uint32 `[B,H,S_q,K_max]` | indexed 模式的 token 列表 |
| `IndexCount` | uint32 `[B,H,S_q]` | 各行有效前缀长度 |

`B`、`H`、`S_q`、`D` 必须为正整数；`S_k` 为非负整数，均满足 profile 上限。长度不是内部 tile 尺寸
整数倍时仍属于合法输入；尾部元素由实现处理，不能被静默丢弃。`scale` 必须是有限
标量。首个 profile 的默认值建议为 `1/sqrt(D)`，但调用者仍应传入解析后的值，避免
硬件自行推断未声明的数值模式。

## 接受、完成与结果

调用者在提交前提供完整的逻辑描述符、输入可读性和 O 可写区间保证。O 的有效元素
必须映射到互不重叠的字节区间，且不与输入重叠；拒绝写广播和地址算术溢出。加速器只有在能够为该调用保留
必要的上下文和资源时才报告接受；未接受的调用不能产生部分输出或改变先前调用的状态。

接受后，系统为调用分配一个 `operation_id`。每个输出元素属于该 ID，并带有逻辑索引
`(b, h, i, d)`；物理写回可以按任意内部顺序完成，调用者不能依赖未声明的流顺序。调用
完成表示所有 `O[b,h,i,:]` 均已写回且没有未处理的错误。完成事件只出现一次。

在所有 key/value 块处理完之前，不要求中间 score 或概率矩阵对调用者可见；实现不得把
这些中间矩阵作为必须由调用者管理的接口对象。

## 数学行为

对每个 `(b, h, i)`，定义允许的 key 集合 `A(i)`。对 `j ∈ A(i)`：

```text
score[i,j] = scale * dot(Q[b,h,i,:], K[b,h,j,:])
p[i,j]     = exp(score[i,j] - max(score[i,A(i)])) /
             sum(k∈A(i)) exp(score[i,k] - max(score[i,A(i)]))
O[b,h,i,:] = sum(j∈A(i)) p[i,j] * V[b,h,j,:]
```

dense 模式的候选集合为全部 `0 <= j < S_k`；indexed 模式为本行 Index 有效前缀
对应的集合。`mask_mode=none` 不再过滤；`mask_mode=causal` 再与
`j <= i + causal_offset` 相交，得到 A(i)。`j` 必须是原始 token 编号，不能使用列表
位置或 gather 后的局部行号。被 mask 或未选中的位置不参与最大值、归一化或 value 累加。

若某一行 `A(i)` 为空，首个 profile 定义该行输出为全零，行归一化因子为零，并继续
完成其他行；不能产生 NaN，也不能把上一行的累加状态带入该行。这个约定仍是草案，
若上游模型需要其他全 mask 语义，必须在 L1 和独立 Oracle 中同时修改。

## 数值合同

输入、`scale` 和中间结果均为 profile 允许的有限值。硬件可以使用近似指数、向量归约
或其他数值实现，但最终结果必须满足 profile 声明的 `atol`/`rtol` 或 ULP 界限。没有
声明容差的 profile 不能进入 `accepted`，也不能用“与某次 DUT 输出一致”作为证据。

当前可执行候选的具体尺寸、FP16/BF16转换、binary32 RTZ/FTZ运算、指数近似、
溢出及精度验收边界由同层 [P0S profile](fa2-core-profile-0001.md) 完整定义。
L3只引用和验证这些定义；全域误差尚未验收，因此L1/L2保持draft。
当前binary64数学Oracle不构成FP32硬件数值合同。

## 反压、取消与复位

- 输入未准备好或输出接收端施加反压时，调用可以暂停；暂停期间已接受的上下文、行状态
  和已产生的结果不能丢失、重复或被其他 `operation_id` 覆盖。
- 取消阻止当前计算提交和新请求；错误响应使操作失效并停止后续计算阶段与新请求。
  检出错误的同拍可以更新内部响应暂存或 buffer，但这些值不能作为有效行结果发布，
  也不能被后续操作读取。已发出的写请求可能继续修改 O，必须 drain
  到写响应完成或总线 fence/reset 确认终止后才能释放资源。调用者仅在正常 COMPLETE 后
  使用 O；取消/错误使整个 O 无效，不能承诺撤销已发生的部分写回。
- 复位失效未完成调用、score/行状态的有效性和缓冲区占用；RAM 无须物理清零。外部
  旧请求需 drain 或由总线 reset/fence 清空后才可重用 tag/输出地址；新调用不可读取旧值。
- 取消或复位产生的操作不会发送正常完成事件；若接口提供错误/取消事件，则该事件只能
  发送一次并带原始 `operation_id`。

## 错误与进展

描述符非法或维度超限时必须拒绝执行并给出错误原因，不能产生访存或部分 `O`。
P0S可以先捕获非法描述符，再通过code1 completion报告拒绝；command握手不代表
校验成功。输入不可读由bus.error报告；资源忙时command_ready为0。接受后的存储错误或内部故障必须终止该操作、报告
错误并隔离其状态，不能把陈旧响应提交到后续操作。

在输入保持可读、输出最终解除反压、没有故障且取消未发生的条件下，所有已接受的合法
调用必须在有限时间内完成。该进展保证不固定具体周期或吞吐率。

## 观察场景

至少需要覆盖以下场景：

1. `D`、`S_q` 或 `S_k` 不是实现 tile 尺寸整数倍；
2. causal 的首行、末行和空允许集合；
3. score 接近相同、极大、极小但仍有限的输入；
4. 输入暂停、输出反压、恢复后继续完成；
5. 连续调用、取消后重新提交和复位后重新提交；
6. 不同分块边界产生相同逻辑结果。

## 直接细化

以下 L1 条款是同层职责分解，分别直接细化 L0，并依赖本调用合同：

- [FA2-CORE-BUF-0001：片上存储架构合同](fa2-core-buf-0001.md)
- [FA2-CORE-XFER-0001：片上/片外数据传输架构合同](fa2-core-xfer-0001.md)
- [FA2-CORE-COMP-0001：注意力计算架构合同](fa2-core-comp-0001.md)
- [FA2-CORE-CTRL-0001：控制、同步和资源可见语义](fa2-core-ctrl-0001.md)
- [FA2-CORE-IDX-0001：索引输入与稀疏 KV 访问合同](fa2-core-idx-0001.md)
- [FA2-CORE-SKV-0002：多 Head 共享 KV 与 64-Index 批处理合同](fa2-core-skv-0002.md)

L2 再将这些架构职责具体化为 buffer 规格、通道和请求协议、计算阵列以及控制器状态。
L3 必须使用独立参考实现，分别验证逻辑结果、事件数量、错误隔离、布局映射和无越界写回。
