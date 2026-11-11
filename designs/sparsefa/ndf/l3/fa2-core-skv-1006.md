# FA2-CORE-SKV-1006：共享 KV 批处理与多缓冲验收 {#FA2-CORE-SKV-1006}

<!-- ndf: kind=verification modality=must refinement=L3 status=draft owner=sparsefa refines=FA2-CORE-SKV-1005 verifies=FA2-CORE-SKV-0002 -->

## 独立 Golden 与观察面

数学 golden 从原始 Q、单份 KV、外部 Index/Count 和配置计算，不能从 DUT 内部
score、weight 或输出倒推。与任意独立 K/V 的旧 fixture 区分：本 profile 的 key
和 value **逐元素是同一份 KV**，但各 head 的 Q、softmax 行状态和 O 独立。
若不同 head 共用 Index，结果仍可不同，因为 Q 不同。独立 profile oracle
用有理数中间值和显式 binary32 编码实现
[SKV-1005](../l2/fa2-core-skv-1005.md) 的 16-lane 相邻树、组间累加、
batch 最大值、重标定、RTZ/FTZ 与输出编码；公开 O 的编码位须逐位一致。
另用高精度稳定 softmax 作数学交叉检查，其容差须在实现测试前按 FP16/BF16、
scale 和 count 明确设定并记录，不能将 P0S 标量逐位结果当作本目标 oracle。

测试适配器从公开端口观察请求 tag、地址、长度、握手、返回、slot/批次诊断事件、
阵列输入组、O 写和 completion。独立内存模型只提供输入 KV，不提供 expected O
给 DUT。性能结果以接受边沿计算，不把 host 执行耗时、System horizon 或设计公式
当作已测硬件周期。

## 必验矩阵

| 类别 | 最小输入 | 必须断言 |
| --- | --- | --- |
| 配置/尾部 | H=1,16,17,64；D=1,16,17,64 | 配置在命令接受时快照，尾 head/维 mask，无截断和越界读写 |
| Index 批次 | count=0,1,63,64,65,127,128,2048 | 64 项为上限；最后批只读有效项；padding 不被读或当成 token |
| 校验屏障 | 第 2/32 批末尾重复、越界、bus.error | 完整前缀错误优先级；本行第一个 KV 请求尚未发生 |
| 集合语义 | 无序 Index、causal 原 token、空集合 | 只读有效 token；score/分母/PV 仅包含允许位置 |
| 共享 KV | 16 个不同 Q head 共用同一 Index；K=V 非常量 | 一份 KV tile 广播；各 head 输出与独立 golden 一致 |
| 不同 Index | 两个 head 的列表不同或次序不同；count/index 仅一者 head stride 为零 | `list_identity,head_id,batch` 绑定；单 head 行计算，绝不错误广播或复用 |
| 跨批数值 | 两批最大值上升/下降、含全 mask 中间批、H=17/D=17、尾 token | 每批 QK→softmax→PV 后 `m/l/acc` 原子提交；无效批保持；profile oracle 逐位一致 |
| 阵列组计数 | 满批 64 token、D=64、16 个有效 head | 每 wave 的 QK 4 token tile × 4 维组 = 16 GROUP，PV 4 输出维 tile × 4 token 组 = 16 GROUP；64 个 score 和输出维均覆盖 |
| DMA 乱序 | 8 credit、同一 256-bit 字的不同 byte fragment 反序返回、跨字乱序、request_ready 停顿 | 装配位图完整后只整字写 bank；重叠/重复/越界报错，末字补零，未 READY 不消费 |
| bank/缓冲交叠 | 同 bank 多响应、跨 slot 同拍读写、读写等待；装载快于/等于/慢于计算；两/三 slot | 冲突反压，独立 slot 并行，至少一拍同步读，tag 对齐；慢装载产生真实停顿，无覆盖/漏算 |
| 取消与复位 | FILLING、READY、IN_USE 时取消和 bus.error | 旧响应 drain/隔离，O 无效，无重复 completion，重启无陈旧 tile |
| 写回 | O 最后一次写 ack 延迟/错误 | ack 前无成功 completion；错误后整次 O 无效 |

独立地址 oracle 检查每个已选 token 的 `kv_base+b*kv_batch_stride+
token*kv_row_stride+2*d`，并检查 Q/O 的 head stride。共享列表的理想
KV 数据量下界为每 `(b,i)` 的 `2*D*N_active`；测试须记录实际请求字节数、
命中/重读次数、slot 等待周期、DMA credit 占用、每 batch 的装载/计算周期和
head wave 数。若声称传输被双缓冲隐藏，须用同一输入、时钟和内存延迟对照
单 slot 与双 slot 的 C++ 执行 trace；三 slot 另作对照。只看源码里的 slot 数
或公式不能宣称加速。

数值 trace 逐 batch 记录每 head 的有效 score、`bmax,m_new,alpha,b_l,
b_acc,l_new,acc_new` 和提交边沿；候选未完成或取消时断言旧 `m/l/acc`
完整保持。用能区分相邻树与线性求和、先乘后加与 FMA 的输入验证顺序，
并分别覆盖 FP16、BF16、正负零和 FTZ；数学 golden 只作算法检查，
profile oracle 才判定目标编码。共享列表检查每个 slot 被全部 head wave
消费后才释放；head-specific 列表检查不同 head 的同 batch 编号不会互借
slot，且 batch `t+1` 只预取当前 head 的列表。

## 当前状态

截至本 NDF 草案，`SharedKVCommand`、64 项 DMA、带 tag 的 gather、KV slot
manager、转置 feeder、跨 head softmax 和整机 System **尚未实现**。
因此上述矩阵均为 **NOT RUN**；原 P0S 的 C++ 结果及独立三维阵列的 host golden
不能替代。实现后从同一 verified final 生成 C++ 与 Verilog；按用户要求，
执行 C++ 仿真而不运行 Verilog 仿真。编译、link、双目标 emit、C++ workers 1/2
和物理 PPA 分别记录，任何失败或未运行阶段都不能写为 PASS。
