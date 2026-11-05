# FA2-CORE-XFER-0001：片上/片外数据传输架构合同 {#FA2-CORE-XFER-0001}

<!-- ndf: kind=requirement modality=must refinement=L1 status=draft owner=flash_attention2 refines=FA2-ADR-001,FA2-ADR-002 -->

## 目的

定义逻辑张量、片上 tile store 和计算阶段之间的事务边界。传输架构必须支持有限工作
集、尾块、反压、取消和陈旧响应隔离；它不规定具体 DMA IP 或总线协议。

## 事务类型

传输边界至少区分以下事务：

| 事务 | 方向 | 载荷 | 完成含义 |
| --- | --- | --- | --- |
| `Q_READ` | 外部存储 → Q tile | Q tile 描述符和数据 beat | 指定 Q slot 可供 score 读取 |
| `COUNT_READ` / `INDEX_READ` | 外部存储 → Index store | count 与有效索引前缀 | 索引装载完成，待校验 |
| `K_READ` | 外部存储 → K tile | K tile 描述符和数据 beat | 指定 K slot 可供 score 读取 |
| `V_READ` | 外部存储 → V tile | V tile 描述符和数据 beat | 指定 V slot 可供 value 累加读取 |
| `O_WRITE` | output staging → 外部存储 | 输出 tile 和逻辑索引 | 指定输出 tile 已提交 |
| `ERROR/CANCEL` | 控制边界 | operation、generation、原因 | 终止或隔离当前事务 |

传输owner必须能确定当前事务的operation、batch/head、元素坐标、stride、dtype和
有效范围。多outstanding协议需要tag/generation和重排匹配。P0S单outstanding使用
当前阶段和快照保存identity，响应无tag且不能乱序；在旧请求完全drain前不复用上下文。
同一slot不得被其他事务覆盖。

## 地址与布局语义

逻辑地址由 tensor base、`(b,h,row,d)` 索引和声明 stride 形成。传输层可以合并连续
beat、添加对齐 padding 或改变 burst 长度，但必须保持：

- 每个传输 descriptor 的有效元素完整交付且只提交一次；跨 tile/query 允许重读 KV；
- indexed 模式先读取并验证 index，再以原 token ID 生成 K/V 地址，见
  [索引合同](fa2-core-idx-0001.md)；未选中的 token 不发起 KV 请求；
- 尾部无效元素不被当作有效数据；
- `O_WRITE` 只写入该 operation 声明的输出索引；
- 不允许通过物理地址顺序推断 causal 或 mask 语义。

## 顺序、反压与错误

传输 owner 保留目标 slot 和返回容量后才发出外部事务；内部 Queue/Rule 可用性由
编译器传播，不要求源码重复编写相同的 ready/full 判断。下游反压时，
请求和有效载荷保持稳定（P0S允许cancel撤回尚未接受的请求）；已经接受的 beat 不能被丢弃或重复发送。读响应未完成时，计算阶段
不能读取未定义的 tile 数据。

取消使当前operation失效；已接受请求的响应只能drain，不得写入当前slot。
P0S不支持无tag旧包任意重放；复位后须由外部bus fence保证隔离。已发写请求遵守调用合同的 drain 规则，不能撤销已发生的写入。存储故障、地址错误和不支持的对齐必须返回可区分错误，不得伪装成零数据继续计算。

## L2 细化

P0S通道字段、2/4字节标量事务、单outstanding所有权、请求保持、写ack和drain时序见
[FA2-CORE-XFER-1001](../l2/fa2-core-xfer-1001.md)。
