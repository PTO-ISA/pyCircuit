# FA2-CORE-COMP-1001：共享标量计算通路与位级浮点 {#FA2-CORE-COMP-1001}

<!-- ndf: kind=mechanism modality=must refinement=L2 status=draft owner=flash_attention2 refines=FA2-CORE-COMP-0001,FA2-CORE-PROFILE-0001 -->

## 资源结构

FA2Engine主数据通路有一个显式fp_mul、一个fp_add、一个fp_exp_neg、一个fp_div、
一组decode/encode。exp内部还有其多项式/平方需要的组合add/mul实例，因此“共享乘法”
不表示整个网表只有一个乘法器。P0S不含16×16阵列或FMA流水；频率和面积待综合。

| 硬件职责 | 状态输入 | 组合结果 | 提交时机 |
| --- | --- | --- | --- |
| Decode/domain check | memory.data、bf16 | binary32、abs≤8 | Q/K/V有效响应 |
| QK MAC | query_lane、K、dot | add(dot,mul(Q,K)) | K响应；最后维进入SCALE |
| Scale | dot、scale | mul(dot,scale) | SCALE_SCORE |
| Max | old_m、score、l | max；l=0选score | MAX |
| Alpha/Beta | old_m/score、candidate_m | exp(add(x,-candidate_m)) | ALPHA/BETA，各一阶段 |
| Denominator | old_l、alpha、beta | add(mul(old_l,alpha),beta) | DENOM，只写shadow scalar |
| PV rescale | active_acc[d]、alpha | mul(active_acc,alpha) | RESCALE_VECTOR |
| PV contribution | beta、V[d]、scaled_acc | add(scaled_acc,mul(beta,V)) | ACCUM_VECTOR |
| Normalize | active_acc[d]、l | div；empty行选择0 | O_REQ直到写ack |
| Encode | normalized、bf16 | 目标16-bit编码 | O写数据 |

对同一row采用online recurrence，不缓存完整score/probability。所有数据来自真实
内存响应。主ALU操作数由phase选择；状态寄存器把相关阶段隔开，无隐含多周期函数调用。

## 位级算术实现

浮点载荷以u32传输；符号bit31、exponent[30:23]、fraction[22:0]。normal恢复隐含1，
再按以下运算生成编码。源码由pyCircuit编译为硬件，不由Python执行算术函数。

| 模块 | 结构 | 舍入/异常 |
| --- | --- | --- |
| fp_add | 按绝对值排序、64-bit扩展尾数、指数差右移+sticky、加/减、CLZ归一化 | 保留24-bit向零截断，FTZ、有限饱和 |
| fp_mul | 24×24-bit尾数乘积、48-bit归一化、指数加减偏置 | RTZ/FTZ/饱和 |
| fp_div | 比较尾数、移位分子、无符号商、指数归一化 | 取商尾数RTZ；normal非零分母 |
| decode16 | 分离符号/指数/尾数、偏置调整；BF16位扩展 | 源subnormal FTZ；非法数由engine拒绝 |
| encode16 | 调整偏置、截尾、目标format FTZ | FP16有限饱和；profile内不会溢出 |
| fp_exp_neg | x/256，四次多项式，8次平方，-16截尾 | 每个中间运算按同一RTZ/FTZ |

动态移位、除法、CLZ的实际生成能力须经过compile/link/emit/run；源码算式存在不等于
时序收敛。fp_div是组合无符号除法网络候选，未实现迭代流水除法器或声明one-cycle频率。

特别是异号小量相减必须保留sticky：`1 + (-2^-80)`的RTZ结果为1之前的相邻binary32，
不能因对齐移位丢掉小量而返回1。零符号、极小输入、相消和转换边界由独立测试定义。

## Online 更新和原子性

初始l=0，acc全零；第一个有效token的new_m=score、alpha=0、beta=1。
随后每token计算new_m、alpha、beta、new_l，按d逐个把候选acc写shadow。仅最后一维
成功提交时更新bank、m、l与p。任何中间取消/坏V都不得把未完成shadow当active。

无有效token时l保持0，O写入正零。负score、相同score、max下降（保持old_m）和max
上升均使用原数值比较，而不是无符号编码大小直接比较。masked token不更新任何row统计。

## 时间和复用约束

每个有效token需要D次K事务、scale/max/alpha/beta/denominator阶段、D次V事务和
每维rescale/accumulate阶段；最后还有D次输出load/write/ack。外部等待可以任意插入。
exp是组合子网络，ALPHA/BETA各在一个提交边沿采样结果；这只是功能周期，不是GHz
时钟承诺。流水化需要新增latency/tag/flush合同和重新验证跨阶段可见性。

## 数值验证

遵守[L1 P0S profile](../l1/fa2-core-profile-0001.md)。独立测试须分开记录基本算术、
转换、指数近似和端到端attention；硬件binary32编码不能拿Python整数加乘作期望。
指数采样证明采样点，不证明整个连续区间；端到端小样例不能证明D64/count2048全域。
