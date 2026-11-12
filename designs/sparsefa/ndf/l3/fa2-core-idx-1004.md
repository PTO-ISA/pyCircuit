# FA2-CORE-IDX-1004：精确索引校验器架构验收 {#FA2-CORE-IDX-1004}

<!-- ndf: kind=verification modality=must refinement=L3 status=draft owner=flash_attention2 refines=FA2-CORE-IDX-1003,FA2-CORE-IDX-1001 -->

## 被测对象与独立期望

本页只验收 P0S。被测对象为公开编译流程生成的完整 FA2TensorScheduler C++ 模型，包含真实的
Index 读取、重复检测、Q/K/V 计算和 O 写确认。host 只提供描述符、内存与
握手，不能读取或修改私有状态。C++ 和 Verilog 来自同一 verified final；
按用户要求不做 Verilog 仿真。

独立 oracle 按原始输入顺序检查 uint32 token，先检查 memory.error，再检查
range 和 duplicate；完整前缀验证通过前不得发 Q/K/V 请求。无序合法列表
保持原顺序，Index 来自外部而非校验器计算。取消必须 drain 已接受响应，
重启不得复用旧树节点或已失效的原序列表。

## 必须覆盖的结构与行为

- Count 为 0、128、512、2048；升序、降序、无序、最高位分歧、长公共前缀
  与 uint32 上界，覆盖 Patricia 分支方向和节点容量。
- 第 2 项重复首项、末项重复、越界、memory.error 及同一响应的错误优先级；
  首个错误之前的 Index 请求地址连续，错误之后无 Q/K/V 或 O 写。
- leaf、branch、parent/root 和 IndexStore 发布之间的取消；已接受响应完成
  drain 后同一实例能处理新命令，旧节点不可达。
- 正常请求的地址、原 token K/V 顺序、请求数量、反压时载荷保持、
  单 outstanding 和完成状态与独立 oracle 一致。

本 NDF 验收正确性、容量和状态所有权，不规定 Index 校验、QK 或整个算子的
完成时延。相同输入的旧/新实现周期测量、适配器方法、运行矩阵与实际结果
由 [Index 性能测试](../../tests/index_performance/README.md)保存。
