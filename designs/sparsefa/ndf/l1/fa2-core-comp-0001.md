# FA2-CORE-COMP-0001：注意力计算架构合同 {#FA2-CORE-COMP-0001}

<!-- ndf: kind=requirement modality=must refinement=L1 status=draft owner=flash_attention2 refines=FA2-ADR-001,FA2-ADR-002 -->

## 目的

定义硬件计算路径必须提供的功能阶段、数据依赖和数值边界。阶段可以由独立模块、共享
阵列或融合流水实现，但不能删去这些语义边界。

## 必须存在的计算阶段

```text
Index → gather K/V（indexed）或连续 K/V（dense）
Q/K tile → dot product → scale/mask → row max
       → online softmax state update → weighted V accumulate
       → normalize → output staging
```

每个阶段都必须识别输入 tile 的有效范围。indexed 模式保留原token metadata，以原 token ID
做 causal 判断；只在当前 query 的选择集合上更新 m/l/acc，不能与其他 query 的列表混用。mask 后的 score 不能参与 row max、指数、
行和或 value 累加。分块之间必须保持每个 query 行的 `m/l/acc` 数据依赖。

## 数值边界

首个 profile 的 Q/K/V 输入是 FP16 或 BF16；dot product、`m`、`l` 和 `acc` 至少以
FP32 语义定义。指数、倒数、舍入和输出 cast 可以由实现选择近似形式，但必须在 profile
中声明误差界限。任何近似不得把有限输入变成未声明的 NaN/Inf。

`l=0` 的行输出全零。该结果和其他行的状态必须独立，不能复用前一行的归一化因子。

## 资源和提交边界

计算路径至少区分 score 临时值、行统计状态和 value 累加状态。一个 tile step 的行
更新必须是全有或全无：`m`、`l` 和 `acc` 不能只提交其中一部分。normalize 只读取
已经完成所有 K/V tile 的行状态；writeback 只提交已经完成 normalize 的输出 tile。

L1 不规定阵列是 systolic、SIMD、vector 或时分复用。L2 可进一步规定资源
组织和吞吐能力，但算子时延不属于 NDF；依赖、精度和提交边界必须保持。

## L2 细化

P0S共享标量ALU、phase操作数选择、位级算术、exp、PV影子提交及normalize连接见
[FA2-CORE-COMP-1001](../l2/fa2-core-comp-1001.md)。

## P0S具体化

数值执行顺序和RTZ/FTZ/exp/cast由 [P0S profile](fa2-core-profile-0001.md) 定义；
计算模块的共享运算单元、状态寄存器和每阶段提交见L2。该实现使用binary32位编码，
不是整数attention。P0T阵列的并行度不属于当前已执行能力。

## P0T 三维计算候选

当前并行方向采用 `M×N×P_K = 16×16×16`：M/N 对应输出矩阵的行/列，P_K 对应
同一输出在归约维上的空间展开。每组有 4096 个独立乘积，按相邻 K lane 的平衡树
归约后更新 256 个输出累加器。这里的“三维”是逻辑计算并行度，不规定芯片的物理堆叠。

每次乘法、树节点加法和跨组累加分别舍入；树形归约与 P0S 逐项累加不保证逐位相同。
片上供数须同时提供两侧各 256 个 binary32 操作数，尾部和 mask 不得参与有效贡献。
K 分组号不能替代外部预生成的 KV token index。

具体接口、寄存数据路径和停顿/取消规则见
[ARRAY3D-1002](../l2/fa2-core-array3d-1002.md)，供数容量与布局见
[ARRAY3D-BUF-1004](../l2/fa2-core-array3d-buf-1004.md)。独立阵列先于完整 attention
集成验证，不能由设计并行度推断已测吞吐或更新 P0S 的执行合同。
P0T Vector 和 exp 的独立资源吞吐规格见
[VECTOR-1006](../l2/fa2-core-vector-1006.md)。
