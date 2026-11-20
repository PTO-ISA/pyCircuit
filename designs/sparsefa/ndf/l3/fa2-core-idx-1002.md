# FA2-CORE-IDX-1002：外部索引与直接 gather 验收 {#FA2-CORE-IDX-1002}

<!-- ndf: kind=verification modality=must refinement=L3 status=draft owner=flash_attention2 refines=FA2-CORE-IDX-1001 verifies=FA2-CORE-IDX-0001,FA2-CORE-PROFILE-0001 -->

## 独立期望

`tests/oracle_attention.py`直接选择集合计算稳定softmax，online版本用于分块数学交叉
检查；binary64的1e-12比较只用于参考模型之间，不能移作硬件容差。
`tests/oracle_gather.py`是通用布局/gather模型。旧模型会枚举mask前的gather候选；
P0S实际在发KV前mask，DUT事件检查应依据L2有效集合独立构造，不能直接照搬旧日志。

| 输入 | 地址/结果检查 |
| --- | --- |
| 无序[2,0,1]，causal位置1 | 全3个Index被验证；token2不发KV；token0/1非零score结果 |
| K stride8、V stride14 | 分别计算base+j*stride+2*d，不共用错误stride |
| count0 | 无Index/KV读；Q装载后逐维零输出 |
| count>min(2048,S_k) | code2，count响应后终止，无Index/KV/O访问 |
| token==S_k或uint32高位越界 | code3，无该query KV/O |
| 重复token，包括0和masked项 | P0S 报 code4，完整前缀校验之前不执行KV；Shared-KV 的逐批放行另见 [IDX-STREAM-1008](fa2-core-idx-stream-1008.md) |
| padding含重复/越界垃圾 | padding不读、不验证、不生成请求 |
| 不同query/head列表 | 不合并、不跨行复用此前缀 |
| count2048 | 全前缀验证、实际比较次数、计数不截成0 |
| uint64最后可访问元素和溢出 | end-exclusive检查，失败前不发请求 |

原token保留至K/V完成，causal不使用p。完整唯一集合与dense结果在L1验收容差内比较；
不同列表顺序可产生浮点差异但不得重复/漏选。sparse支持与DeepSeek完整MLA/cache ABI
分别验收；没有字段表达的paged/quantized视图不能声称已兼容。

## 生命周期

请求延迟/反压、取消发生在COUNT/INDEX/DUP/K/V各边界；测试复位时必须配合bus
fence，P0S无tag接口不支持任意旧包重放。第二行索引错误可在第一行O已写后出现，
操作仍整体ERROR，不承诺回滚先前写入。终止握手后重新提交必须重建当前有效前缀。

## 实际证据

数学测试与 `tests/integration/bench.py`、`vectors.py` 分别记录。测试文件存在不等于
对应场景已经运行。最新compile/link/run/RTL状态及未覆盖最大容量项见
[VALIDATION.md](../../VALIDATION.md)，精确内容绑定在本地gate manifest。
