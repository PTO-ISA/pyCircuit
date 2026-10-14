# pyCircuit array 设计

本设计定义首版 array 的前端语义、事务分析、MLIR 表示和执行要求，替代旧 array、vector、qarray 及其专用构造，不保留兼容路径。设计已确认，实现尚未迁移。基础组件与 Rule 的语义沿用[核心设计](core-design.md)。

## 1. 类型与构造

```python
ac.array(T, shape=(D0, D1, ...))
```

array 是静态形状、元素同型的容器，不是 Module，不自动增加状态、事务或组件操作。shape 是类型的一部分，必须是非空元组，每个维度是编译期确定的正整数。首版不接受零长度维度、运行时形状或 resize。

规则网格统一用多维 shape；不接受元素类型直接为另一个 array，也不自动合并嵌套维度。struct 可以含 array 字段。

类型声明配合嵌套列表或静态推导式构造，必须完整覆盖 shape，元素类型和位宽匹配，不隐式补齐、广播或截断。列表只表示固定构造，不引入可变长容器。

```python
Matrix = ac.array(ac.u32, shape=(2, 3))
values: Matrix = [[1, 2, 3], [4, 5, 6]]

regs: ac.array(ac.cell[ac.u32], shape=(4,)) = [
    ac.cell[ac.u32](initial=0) for i in range(4)
]

queues: ac.array(ac.queue[ac.u32], shape=(4,)) = [
    ac.queue[ac.u32](capacity=2) for i in range(4)
]

stages: ac.array(Stage, shape=(4,)) = [Stage() for i in range(4)]
```

普通 payload 的元素可以来自运行时 SSA 计算，元素数量仍静态确定。组件和 Module 只在 __init__ 中构造，状态初值必须静态确定，各组件沿用自己的初始化接口。

每次组件构造产生独立实例；填写已有引用则引用原对象，不复制硬件。组件和 Module 引用不允许运行时重新绑定。Module 数组通过静态索引或静态循环建立固定连接，不允许动态 __call__ 连接。

## 2. 数据与资源

| 类型 | 含义 |
| --- | --- |
| array(T)，T 为 payload | 组合聚合值，默认不跨拍保存 |
| Cell[array(T)] | 一个 Cell 保存完整数组 |
| array(Cell[T]) | 多个独立 Cell 的引用集合 |
| Queue[array(T)] | 每条消息携带一个完整数组 |
| array(Queue[T]) | 多个独立 Queue 的引用集合 |
| array(Module) | 固定 Module 实例引用集合 |

只有递归属于 payload 的完整数组可以作为组件存储。普通 helper 可接收选取后的 payload array，但必须保留选取域。组件或 Module 引用数组不能作为跨拍消息，不能传入普通 helper。

Queue[array(T)] 首版从空队列开始，暂不提供数组消息预装接口。后续若增加预装，外层必须明确表示消息序列，每条消息严格符合 array shape，不猜测数组是一条消息还是多条消息。

payload 数组遵循 SSA 值语义：

```python
b = a
b[0, 1] = value
```

b 更新为新值，a 保留原值。子数组整体更新必须匹配剩余 shape 和元素类型。

Cell[array] 的部分更新仍属于同一个 Cell，不自动变成独立写口。多个更新需累计时，先修改局部 payload，再提交一次 Cell 写入：

```python
value = state
value[0, 0] = first
value[0, 1] = second
state = value
```

## 3. 索引与选取

| 表达 | 语义 |
| --- | --- |
| a[i, j] | 定位二维数组元素 |
| a[i] | 定位子数组，保留剩余维度 |
| a[mask] | 多选，结果仍是 array，保留原 shape、坐标和元素身份 |
| len(a) | 最外层维度的编译时长度，不是已选元素数 |

mask 是与 shape 对齐的多选信号，每个坐标一个 bool，允许零个或多个坐标。普通下标使用整数坐标，不增加 index 类型或构造器。mask 不隐式转换为整数或 bool；mask[i, j] 读取相应坐标的 bool 选取位。

普通 array 的选取域为全选。a[mask] 不压缩或重新编号；连续选取取交集，不修改原 array。构造选取本身不读取、消费或写入组件，后续实际操作才产生效果，并受选取域约束。

空 mask 是正常结果，得到的 array 选取域为空。后续逐元素效果可以全部不使能，不自动使所属事务未就绪。要求至少一个目标的操作必须明确提供这一执行要求。

实际组件操作沿用原组件语义：Cell 读旧值，Rule 主体中的 Queue 普通值读取提出消费，peek 只观察，写入提出对应效果；Module 组合计算和 guard 中的 Queue 读取只观察。Signal 有效性和 EHR 端口可见性不因 array 改变。不隐式把组件数组整体读取为 payload 数组。

`cells[mask] = value` 向所有选中的组件写入同一 payload，效果归入所属 Rule。不同元素的数据用静态循环显式表达；不提供批量读取。

首版不增加 append、resize、容器 pop、切片、map、fold 或与下标重复的 select/update 公开接口。

## 4. match 与调用

match 是纯组合计算，接受 lambda 或编译期确定的命名 payload helper，谓词结果必须是 bool：

```python
# entries 为明确取得的 payload array。
selected = entries[entries.match(
    lambda entry: entry.valid and entry.tag == wanted_tag
)]
```

match 不隐式观察或消费组件；按状态值匹配时先通过明确操作取得 payload。类型错误在编译时报错，没有匹配项则返回全零 mask，没有独立的运行时失败机制。上游读取未就绪不能伪装成空 mask。对已有选取的 array 继续匹配时，结果限于其原选取域。

mask 是本拍临时 SSA 信息，不隐式增加寄存器，下一拍按模型重新计算。同拍复用保留选取信息、来源依赖和原状态视图，不引入跨拍缓存。

对外调用传选取后的 array，首版不提供独立 mask 参数。参数、返回值和局部合流必须保留选取域。传 array 不自动执行批量读取或复制数据；接收方只使用选取信号时，lowering 可只保留实际需要的信息。普通 helper 的 payload 限制不变。

禁止把部分选取的 array 直接跨拍保存，包括通过 struct 包装、helper、Rule 返回值或合流隐式丢弃选取域后保存。不能自动保存隐藏 mask，或把未选元素填零当有效数据。显式操作产生的完整独立 payload 值仍可按组件语义保存。

## 5. 仲裁与无效访问

模型通过显式的只读仲裁 Rule 返回普通 tuple：一维为 `(found, position)`，多维为 `(found, i, j, ...)`。found 为 bool，坐标为普通整数，不隐含有效性。

```python
selected = metadata[metadata.match(predicate)]
found, position = age.choose(selected)
if found:
    value = cells[position]
    out = value
```

无候选时返回 found=False，坐标不应使用。有效赢家须属于选取域且唯一。仲裁器可用 `selected.match(lambda item: True)` 获取本地候选 mask，再按坐标查询选取位；不增加 is_selected 接口。

found 分支只关闭其中效果。若要求无赢家时整个 Rule 不执行，明确将 found 纳入 guard；普通坐标不自动传播“未找到”。仲裁查询不修改状态；年龄和释放等更新与实际事件同事务提交，反压时全部取消。

静态下标越界编译报错。动态下标逐维判断范围；越界或实际访问未选元素向依赖该访问的事务传播未就绪，不返回零，也不能作为 False 进入 else。仅构造组件选取不读取、消费或写入资源。

Rule 未就绪表示本拍不 fire，不是异常返回。完整执行条件汇总选择路径、guard 及实际必要操作的就绪要求；未执行路径不增加要求，控制条件自身的有效性仍须满足。嵌套调用共享外层事务，所有效果统一提交或取消。

## 6. 保守顺序与竞争

首版按真实资源、静态组件契约和声明关系排序。动态访问按全部可能命中的资源收集约束，不要求证明动态互斥。所有路径的顺序边合并为节点级图，有环则拒绝编译并指出相关 Rule、资源和关系，即使运行时路径可能排除该环。

拓扑排序仅实现已有约束，不增加 EHR 端口可见性，也不构成仲裁或写入优先级。别名、重复 Queue 消费、容量与事务原子性仍须分析；不同数组位置不一定是独立资源。

mask 与普通下标两路实际写入可分别生成每个元素的使能：

```text
enable_A[k] = fire_A AND path_A AND mask[k]
enable_B[k] = fire_B AND path_B AND coordinate_in_range AND (coordinate == k)
```

不同真实 Cell 可各自更新；同一真实 Cell 同拍多路有效写入沿用未处理竞争结果不保证的约定，不承诺双写口，不自动决定覆盖优先级。仿真可报告实际竞争，诊断不改变硬件语义。

保守顺序分析不意味着保守阻塞所有候选资源：执行条件仍只要求实际选择和实际路径上的组件就绪，未选 Queue 的空满状态不能阻塞事务。

## 7. MLIR、lowering 与验证

| 阶段 | 要求 |
| --- | --- |
| 源语义 MLIR | 保留类型、shape、构造、索引、选取域、match、逐维范围检查和更新，全部由操作、SSA、属性及 region 承载 |
| 事务 MLIR | 保留 array 结构，明确真实资源候选、别名、路径、就绪、效果、固定端口和静态顺序 |
| 直接仿真 | 按 shape 解释 array 和选取信息，保留组件状态与同拍视图，不提前读取未选元素 |
| RTL 友好 MLIR | 物理展开元素状态、组合比较、mux、使能及 next-state，不遗留资源选择、就绪或调度决策 |

延后物理展开不延后资源和契约分析。共享读取保持同一 SSA 来源及固定 EHR 视图，前序写入依赖传播到所有使用者；禁止提前求值或跨视图合并。

元素布局采用最后一维变化最快的顺序，二维下标对应 i*N+j，但范围逐维判断，不能让越界坐标别名到另一行。多选传播为元素使能，单选读取生成 mux，组件内部存储按需要继续展开。不自动推断 RAM、时间复用或跨拍缓存。

验收覆盖多维构造、子数组更新、空选取、连续选取、调用传递与跨拍保存拒绝；越界访问取消与 found 分支跳过；动态别名、重复消费、Queue 反压、EHR 共享视图、保守顺序环和实际竞争；仲裁状态更新与实际提交一致。

事务与 RTL 执行器必须逐拍一致，并对照独立预期。优化开关和各阶段 MLIR 保存重载保持行为一致，轨迹保留原数组坐标。JSON 只用于报告。上述为实施验收要求，尚未运行新设计测试。

参考：[BSV Vector 文档](https://github.com/B-Lang-org/bsc/blob/main/doc/libraries_ref_guide/LibDoc/Vector.tex)、[库实现](https://github.com/B-Lang-org/bsc/blob/main/src/Libraries/Base1/Vector.bs)、[香山 AgeDetector](https://docs.xiangshan.cc/projects/design/en/kunminghu-v3/backend/Schedule_And_Issue/IssueQueue/)。
