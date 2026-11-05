# FA2-CORE-XFER-1001：P0S 标量访存协议与 drain {#FA2-CORE-XFER-1001}

<!-- ndf: kind=mechanism modality=must refinement=L2 status=draft owner=flash_attention2 refines=FA2-CORE-XFER-0001,FA2-CORE-PROFILE-0001 -->

## 端口 ABI

源码类型定义在 `src/fa2_interfaces.py`。MemoryRequest 与 MemoryReply 各是一个有内在
关联的 Struct；MemoryRequest 和操作状态分别路由，top 返回 tuple，直接解包连接。
这些 valid/ready 是真实外部协议字段，不是手写的内部 Rule proposal/commit控制。

| 方向/字段 | 位宽 | 含义 |
| --- | ---: | --- |
| request.valid | 1 | 当前请求存在 |
| request.write | 1 | 0读、1写 |
| request.address | 64 | byte address |
| request.data | 32 | 写入低size字节；读请求忽略 |
| request.size | 3 | 2=Q/K/V/O元素；4=count/index，单位byte |
| request.response_ready | 1 | 能消费之前已接受请求的响应 |
| reply.request_ready | 1 | 接收当前请求 |
| reply.valid | 1 | 上一请求的完成/错误响应 |
| reply.data | 32 | 读值低位对齐；little-endian；写响应忽略 |
| reply.error | 1 | 当前请求失败 |

请求在 valid&request_ready 的边沿接受。响应在 reply.valid&response_ready 的边沿
接受，不得早于请求接受后的下一周期。每次读和写恰好一个响应；全系统最多一个
outstanding；不允许同周期请求/立即响应。读数据采样发生在响应接受时。

## 资源和状态

没有无界队列；`REQ→WAIT→下一阶段` 编码就是单请求占用状态。REQ 保持地址/size/write/
有效写数据，WAIT 不再发请求，只开放 response_ready。Q、Index、K、V、O 按阶段共享
端口，无仲裁饥饿问题，也无跨 descriptor 重排。读响应直接提交到对应 buffer/标量，
不需要多 beat staging；实际 SoC AXI/HBM 接口需要在外部适配标量事务和错误。

| 类别 | 发请求前拥有的状态 | 响应提交目标 |
| --- | --- | --- |
| COUNT | 当前query、count_base | count |
| INDEX | p、index_base | incoming_index |
| Q | d、q_base、dtype | queries[d]，初始化acc |
| K | token、d、query_lane | dot |
| V | token、d、alpha/beta | value_lane，后续shadow update |
| O | d、accumulator_lane、denominator | 写确认后推进d/终止 |

请求端反压时 REQ 不推进，写数据不重算为另一lane。响应延迟时 WAIT 保持所有游标。
若响应端 valid保持，数据/error应稳定直到被接收。有效读数据之外的 bus 位不解释。

## 取消与错误优先级

cancel优先于请求接受和运算。REQ尚未被接受时可撤回valid，这是本协议对“valid保持”
的明确例外；不能把该接口直接等同于要求valid直到握手的AXI子通道。

- REQ/计算阶段取消：停止新请求与新计算提交，进入取消完成。
- WAIT取消且同拍有响应：消费响应但丢弃载荷，进入取消完成。
- WAIT取消且无响应：进入DRAIN；response_ready保持，等该唯一响应后再完成。
- memory.error：消费失败响应，code5终止，不再执行后续计算或发出请求。响应边沿的
  内部暂存和Q buffer可能更新；该数据不构成有效行结果，整个操作的O无效。无其他
  outstanding须清理。
- 最后O写响应成功后才可code0完成；仅发出写请求不能代表结果可见。

DRAIN没有超时假成功。环境不返回响应时操作保持等待；外部watchdog可执行真实bus
fence/reset，再复位设备。不能仅增加generation而让无tag协议猜测哪个包属于旧操作。

数据Rule按phase和response valid接收数据；控制器同拍锁存错误码并进入终止阶段。
因此错误隔离不承诺内部寄存器或RAM逐位回滚。下一操作重新装载Q、清当前累加bank，
并只读取新count对应的完整已校验index前缀；失败响应的暂存不得进入下一操作计算。

## 复位和重用

编译器拥有clk/reset和存储控制。设备复位清生命周期，不能撤销已经到达内存的写。
环境必须在重新提交前完成bus reset/fence或drain；整个取消/失败操作的O无效。
正常completion被接受后回IDLE，下一操作不会接收上一操作的响应，因为它已经清空。
请求重复、无请求响应或擅自重放违反此物理接口的环境前提，P0S没有tag纠错机制。

## 带宽与周期

每行实际数据传输为 `Q=2D`、`KV=4*D*N_active`、`O=2D` 字节；indexed另有
`4+4*count`字节。对mask项无KV请求，不读padding。没有S/P片外写。
理想内存每请求也需REQ和WAIT两个周期，重复检查与算术阶段另计；一拍一个标量
响应不等于一拍一个token，更不代表并行FA2性能。实际cycle必须由生成DUT计数测量。

## 必验场景

分别阻塞request_ready和延迟response.valid；验证地址/数据保持、单outstanding，
所有读写错误、REQ取消、WAIT取消、取消与响应同拍、O最后ack、terminal反压及
cancel后重新提交。复位测试须包含环境fence前提，不允许注入无tag陈旧包却声称识别成功。
