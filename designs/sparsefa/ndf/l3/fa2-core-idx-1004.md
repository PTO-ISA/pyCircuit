# FA2-CORE-IDX-1004：索引校验性能与行为验收 {#FA2-CORE-IDX-1004}

<!-- ndf: kind=verification modality=must refinement=L3 status=draft owner=flash_attention2 refines=FA2-CORE-IDX-1003,FA2-CORE-IDX-1001 -->

## 被测对象与独立期望

被测对象为公开编译流程生成的完整FA2TensorScheduler C++模型，包含真正的索引读取、
重复检测、Q/K/V计算和O写确认。host仅提供描述符、内存与握手，禁止读取/修改私有状态。
适配器和输入位于[tests/index_performance](../../tests/index_performance/README.md)。
工具链必须由当前checkout构建；C++和Verilog来自同一verified final，按用户要求不做RTL仿真。

性能负载为单query、D=1、FP16、Q=K=0、V=1、scale=1。非空集合的精确输出为1，
空集合为0，独立于树的实现。该负载隔离前缀校验开销；既有完整Systems继续承担D64、
BF16、causal、多B/H/query和数值路径检查，不能把D1性能外推为D64实测。

## 周期计量

每个逻辑周期先在低时钟样本观察公开ready/valid，再以相同输入提交下一rising edge。
命令、请求、响应和完成只在成功边沿且握手成立时计数。请求在R接受后，最快R+1响应；
延迟4时最早R+4响应，request_ready还在绝对周期mod5=2时反压。始终检查单outstanding。

| 指标 | 精确定义 |
| --- | --- |
| first_q_latency | 首个Q请求接受边沿减command接受边沿 |
| first_kv_latency | 首个K/V请求接受边沿减command接受边沿 |
| done_latency | completion接受边沿减command接受边沿；completion_ready恒1 |
| cancel_latency | 被取消命令的完成延迟；其余指标对应随后重启的新命令 |
| host_ms | 模型执行的墙钟时间，独立于硬件周期指标 |

旧/新DUT须使用相同adapter、prefix字节、内存数据、响应时序、编译配置和workers设置。
source/prefix的SHA256必须一致，生成DUT的hash允许不同；每次运行输入与安装文件前后
一致，命令、退出码、partial failure和结果保存在独立receipt。workers1/2周期与输出须一致。

## 输入与协议覆盖

- 完整有效N128/512/2048降序；短升序和无序排列；空列表。
- 最高位分歧、接近uint32上界、相同低位、长公共前缀、覆盖32个分歧位的两种顺序。
- 第2项重复首项，以及末项重复；禁止提前读取较晚index、Q、K、V或写O。
- range、memory、同一响应memory+range：code3、code5、code5；取消保持code6优先。
- 外部响应挂起时取消并drain；第二个index响应接受后第5/6/7/8边沿取消，覆盖叶写、
  分支写、链接发布、原序列表发布边界。每种取消后在同一DUT成功重启，检查旧RAM隔离。
- 对所有正常请求检查index地址顺序、原token K/V顺序、访问数量、O数据和反压payload稳定。

大规模升序/无序在输入catalog中保留，但不属于默认运行矩阵。未执行的选择必须明确标记，
不得因存在向量而宣称覆盖。原controller21四态以及六个完整System仍需通过。

## 验收与性能含义

记录相同输入基线与候选的实际拍数和比值；N512/2048需要确认首Q与完成周期确实下降。
仅有O(32N)复杂度、136拍上界、源码审查或静态库构建都不构成性能验收。
32KiB新增节点容量必须与收益一起报告，物理SRAM映射、频率、面积、带宽和功耗单独验收。
当前运行结果与精确候选身份见[VALIDATION](../../VALIDATION.md)，失败历史不得覆盖。
