# TMU.BGF.RQ — 读队列

- 源候选项：`DAV-TMU-BGF-RQ-0001`
- 硬件层次：**H3**，位于 H1 `TMU` / H2 `BGF`
- NDF 细化：**L2 微架构**；模块级 L1 行为链接仍需评审。
- 建议处置：**状态模式**（提议，尚未获目录批准）
- 实现位置：使用 `designs/davincioo/` 下获准的包含所有者或契约文件；此目录处置不授权独立叶节点。
- 当前设计计划执行状态：**代码已实现，还没跑 gfsim 验证**。实现方式是由父层拼装，RQ 自己
  不声明独立的所有权。

RQ 就是读请求排队等着的地方：每个源类别一条队列，装的是 MAP 已经定好位置、但下游这会儿还
接不走的请求。

## 它解决什么问题

MAP 已经算出这次读该去哪个 bank 了，但那个 bank 这一拍可能正忙——别的源类别在用它。请求总得
有地方等，RQ 就是这个地方：每个源类别一条队列。

关键在于 RQ **不**做什么。它什么决策都不做：从不看手上这条记录的内容，不挑谁先谁后，也完全
不带 bank 维度。谁先过是 ARB 的事。这一点是可以查证的，不只是文档里的一句声明：
`bgf_rq_system` 降级后的 IR 里既没有 `ac.route`，也没有任何 `ac.var.get`。

隔离性就靠"每个类别一条独立队列"这件事。某个类别堵住了，占不到别的类别的位置；某个 PE 实例
堵住了，也拖不住别的实例。

RQ 也不是独立的状态所有者。它代表的是父层持有的那组读队列，队列的占用情况归包着它的那层
BGF 组装；RQ 不能变成一个独立于 BGF 组装之外的策略或存储所有者。

## 在系统里的位置

一次 cell 读在 BGF 里的顺序是固定的：客户端寻址一个 cell，MAP 解出这个 cell 在哪，RQ 把请求
存着直到下游接得走，ARB 把它扇出到各 bank 并裁决冲突，XBAR 把授权送到目标 bank，BANK 真正
搬字节。RQ 只负责其中一环——等。

| 谁 | 管什么 | 不管什么 |
| --- | --- | --- |
| CUBE / VEC / TLSU | 要读哪个 cell | 这个 cell 在哪个 bank |
| [MAP](map.md) | 把 `cell_key` 解成 `(bank, row)` | 排队和冲突裁决 |
| **RQ** | **按源类别各排一条队，并保证类别内 FIFO** | **记录的内容、任何取舍、任何 bank 维度** |
| [ARB](arb.md) | bank crossbar、冲突裁决、公平性与老化 | 存请求 |
| [XBAR](xbar.md) | 把已授权的请求送到目标 bank | 授权给谁 |
| [BANK](../trf/bank.md) | 数据字节本身 | cell 放在哪、谁先过 |

写路径是完全对称的一份：[wq.md](wq.md) 是 `CellWriteReq` 的同一套东西，而 RQ 不会去给自己的
读队首和 WQ 的写队首排先后。再往 Tile 那边看，[FRE](../trn/fre.md) 决定一个版本占哪块物理
空间，[REF](../trf/ref.md) 数还有几个人在读它——这两件事在这里都看不见。

## 一个请求怎么走完

一条 `CellReadReq` 从 MAP 过来时，`source_class`、`bank`、`row`、`out_of_range` 都已经带好了。
`source_class` 决定它进哪条队列，那条队列有 `CLASS_RESIDENCY_DEPTH` 个位置。每条类别队列的
队首都暴露给 ARB，而只有 ARB 真的取走了，队首才算被消费掉。ARB、XBAR 或响应槽位堵住时，队首
就留在原地，而且反压只往它自己这个类别传。

记录原样传过去，中间那个什么都不改的 transform 只是用来挂 `CLASS_RESIDENCY_DEPTH` 的——队列
前端没有单独的 buffer 算子，而对外的端口深度固定是 1。

`CLASS_RESIDENCY_DEPTH` 是一个类别**总共**能有多少请求在途，不是每个 bank 的额度。初始值取
`BANK_PARTITIONS`，理由是 ARB 每拍每个 bank 最多放走一个请求，所以一个类别里排着超过
`BANK_PARTITIONS` 个请求时，它们不可能全都互不冲突。这只是个起点，容量还没最终定。

## 输入

| 名称 | 载荷/类型 | 含义 | 证据状态 |
| --- | --- | --- | --- |
| cube_read / vec_read / tlsu_read | CellReadReq | 一个 PE 里某个源类别发来的读请求，带代际限定，已经由 [map.md](map.md) 打好标签、解好位置 | 已实现 |

## 输出

| 名称 | 载荷/类型 | 含义 | 证据状态 |
| --- | --- | --- | --- |
| cube_read / vec_read / tlsu_read | CellReadReq | 每个源类别一条排队中的请求流，由父层连到这个 PE 的 ARB；每个 PE 实例 3 条 | 已实现 |

提议表格中的载荷名称在字段、位宽和名义身份冻结前均为设计伪类型。Queue 传输由编译器推断，不应要求公开的 Queue 包装器。时钟/复位/时间域属于执行上下文，不得臆造为普通载荷端口。

## 所有或包含的状态

- 队列的占用情况归 BGF 父层，不归一个独立的 RQ 模块

## 参考实现

[`rq.py`](rq.py) 定义 `bgf_rq_system`；`CellReadReq` 和位置几何参数通过
[`contracts/tmu_bgf.py`](../../contracts/tmu_bgf.py) 共享。RQ 给每个客户端类别留一条深度为
`CLASS_RESIDENCY_DEPTH` 的队列，除此之外什么都不做。它只是数据在途中暂存的一种形式，真正的
状态所有者是包着它的那层 BGF 组装。

## 与卡片原设想的差异

**RQ 里没有 bank 维度。** bank 只是个调度坐标，所以整个 bank 维度归 [arb.md](arb.md)——真正
解决 bank 冲突的是它。`bank` 和 `row` 跟着请求记录走：[map.md](map.md) 解码一次，ARB 读一次，
RQ 只负责搬，不看内容。要是把 ARB 的 crossbar 拆开、把它的入口那一半放进 RQ，展开出来的
`(source_class, path, bank)` 边一模一样，隔离性不多也不少——因为不管画在哪边，队头阻塞点都在
扇出的入口。

**FlowKey 不再用来选路。** 卡片原来把一条流当成路由维度看，但现在一次 `bgf_rq_system` 展开就
是一个 PE，FlowKey 的路由作用直接被实例几何吸收了。它仍然跟着记录走，用来标身份和匹配取消——
所以下面那套映射还是得冻结，即便 RQ 自己一个字段都不读。

**队列深度挂在一个空的 transform 上。** 框架里没有可以直接实例化的公开 Queue 包装器，队列前端
也没有单独的 buffer 算子，所以能挂 `CLASS_RESIDENCY_DEPTH` 的地方只剩一个原样返回输入的
transform。反过来这也有好处：正因为函数体是空的，"RQ 不做任何决策"变成了降级后 IR 的一个性质，
而不是这张卡片里的一句话。

## 物理前提

[架构](../../ARCHITECTURE.md)中的四个 PE 控制流在物理上彼此独立：每个 PE
拥有私有的 cell 寄存器 bank 组和自己的仲裁。[bank.md](../trf/bank.md) 记载
共 32 个单端口 128 字节 bank 并采用私有分组，即**每个 PE 8 个 bank**。

所以这里描述的是**一个 PE**。PE 的身份体现在"`bgf_rq_system` 每个 PE 展开一份"这件事上，
而不是模块内部的某个路由维度。队列、游标、老化值、公平性状态全都不跨 PE 共享。四份实例由父层
组装出来。

## 提议的队列分区（需评审）

RQ 只是父层持有的队列状态，但 BGF 这套配置还是得把逻辑分区说清楚。分区键就是
`source_class`。下面这个拓扑只是拿来评审的，还没获目录批准：

```text
单个 PE 实例
CUBE / VEC / TLSU                      （只寻址 cell，不带 bank 维度）
              |
              v
             MAP                       （打标 source_class；解码 bank/row）
              |
              v
      read_q[source_class]             （RQ；只负责排队，不带 bank 维度）
              |
              v
             ARB                       （拥有 bank crossbar 与冲突裁决）
              |
              v
          XBAR/BANK                    （每 PE 一套，不跨 PE 共享）
```

- `source_class` 是覆盖 `CUBE`、`VEC`、`TLSU` 的封闭类别标签，这三者是仅有的
  会访问 cell 寄存器的单元；MAP 在进入 RQ 之前就写好了这个标签，此后必须一直保留到
  `BankGrantReq`。
- PE 身份属于实例几何，而非模块内部的分区键。由于每个 PE 单独展开，四个
  逻辑 ID 相同的 PE 流在构造上就不可能共享 FIFO 顺序。
- `bank` 是 PE 私有的 bank 索引，取值 `[0, BANK_PARTITIONS)`。由于每个 PE
  私有自己的 bank，该索引即该 PE 内的物理 bank，无需再分一层 bank group。
  在 BGF 内部它之所以放在请求记录里，正是为了让 RQ 不需要 bank 维度：MAP 写入、
  ARB 读取，两者之间没有任何东西按 bank 展开。
- bank 之间的独立性是 ARB 给的：它的 crossbar 给每条 `(source_class, path, bank)` 边单独留了
  缓冲，所以某个 bank 下游堵住时只占着自己那条边。
- 队列条目至少携带完整的 `CellReadReq` 身份：`FlowKey`、
  epoch/generation、`TileVersion`/`CellKey`、请求 ID 以及响应路由。没有"子 cell 区域"这回事：
  访问粒度就是一整个 cell，所以共享记录里没有元素范围字段，也没有字节掩码字段。MAP 追加
  `source_class`、`bank`、`row` 与 `out_of_range`；这里不会替换任何源身份。

## FlowKey 到 `CellReadReq` 的字段映射（需评审）

`FlowKey(core_id, pe_id, stid, launch_generation)` 的定义见
[架构](../../ARCHITECTURE.md)。`rq.py` 的 `CellReadReq` 尚未冻结与该四元组的
对应关系；下表为待评审的提议，不代表已批准的字段语义。FlowKey 现在不参与路由了，只用来标身份
和匹配取消。但要让取消能正常工作，这套映射还是得定下来。

| FlowKey 分量 | 候选字段 | 类型 | 说明 | 证据状态 |
| --- | --- | --- | --- | --- |
| `core_id` | 无 | — | 若 BGF 按核实例化，则由实例上下文隐含，不进请求记录；若 bank group 跨核共享，则必须新增显式字段。 | 未决 |
| `pe_id` | 路由不需要 | — | 由实例上下文隐含：一次 `bgf_rq_system` 展开即一个 PE。若父级下游的消费者需要还原是哪个 PE 发起的请求，仍需显式字段。 | 未决 |
| `stid` | `thread_id` | `u16` | 候选。参考展开假定每个 PE 只有一个 `stid`；多于一个则需在实例内部再加一层分区。 | 提议 |
| `launch_generation` | `launch_generation` | `u16` | 已在 `rq.py` 中新增；`allocation_generation` 是 Tile 分配代际，语义不同，不得复用。生产者/消费者语义尚未冻结。 | 已实现字段，语义待评审 |
| 整体句柄 | `flow_id` | `u16` | 若 `flow_id` 是父级预分配的 FlowKey 句柄，则以上分量均由它覆盖。 | 提议 |

上表存在两种互斥解释，必须择一冻结：

- **解释 A（句柄）**：`flow_id` 是 FlowKey 的完整句柄，`requester` 与
  `thread_id` 仅作派生的调试/路由信息。
- **解释 B（分量）**：FlowKey 由 `(core_id、pe_id、thread_id、
  launch_generation)` 组合而成，其中 `core_id` 与 `pe_id` 由实例上下文隐含；
  `flow_id` 另有含义或应删除。

改成按 PE 实例化之后，这个选择不再影响分区键了，剩下的影响只在取消匹配上：取消必须对上
`FlowKey` 和 epoch/generation，而且不能把更新的代际删掉。没有启动代际的话，恢复前后同名的
两条流就分不开，所以不管选哪种解释，`rq.py` 和 `wq.py` 都把 `launch_generation` 作为一个
显式的 `u16` 字段带着。

下面这些字段属于单个请求的身份，不属于流的身份，所以不进 FlowKey，但按上面的要求必须跟着
条目一起完整保留：`block_id`、`operation_id`、`tile_id`、`tile_version`、
`allocation_generation`、`request_id`、`cell_key`、`response_route`。

## 队列协议与顺序

- 仅当选定的类别队列有容量，且下游预留策略允许其最终推进时，才接受
  `read_enqueue`。
- 每个类别队首只有在成功传输到 ARB 时才被消费。ARB、XBAR 或响应槽位受阻时，
  必须保留该队首条目，且反压只传播到它自己的类别：任一类别不得拖住其他类别，
  任一 PE 也不得拖住其他 PE。某个 bank 的反压只能经由 ARB crossbar 上属于该
  类别的那条边传到该类别，因此根本传不到别的类别。
- 单个 PE 实例内，每个 `source_class` 必须保持 FIFO。跨类别的顺序不由 RQ
  提供；冲突调度和公平性由 ARB 负责。
- RQ 不得合并不同类别的请求。它只按类别各暴露一个队首，自身不做任何选择；
  round-robin 或优先级取舍属于 ARB，其固定优先级次序与老化升级记录在
  [arb.md](arb.md)。特别地，RQ 不对同一 bank 的读队首与 WQ 的写队首定序，
  该顺序由 ARB 裁决。
- 恢复时只能取消还没被下一级接收的条目。取消要匹配 `FlowKey` 和 epoch/generation，而且不能
  把更新的代际删掉。

## 需验证的能力

- 父层持有的、可参数化的 Queue 数组
- 借用子模块的 Queue 端口，而不复制一份状态
- CUBE、VEC、TLSU 的流量按类别分开排队，队列里不带 bank 维度
- 每个类别的深度、当前占用，以及入队/出队计数
- 从 MAP 接过来之后，请求身份、类别标签、位置字段都原样保留，而且 RQ 自己不去读它们
- 各个 PE 实例互相隔离，不共享队列，也不共享公平性状态

这些是待验证的要求，并不证明当前框架缺少相应能力。先测试当前修订版；确认缺口后，将其作为通用框架/原语问题加入回归，并先合入共享修复，再提交依赖设计的 PR。

## 行为验收

- 每个 `source_class` 保持 FIFO 顺序
- 映射或授权那边反压时，请求不能丢
- 每个类别具有独立容量和反压：某个类别受阻不影响其他类别，某个 PE 实例受阻
  不影响其他实例。每 bank 的独立性是 ARB 的验收义务，不是 RQ 的
- 类别内不得重排
- 自己不做仲裁，也不管 Tile 完成这件事
- 不读取任何放置字段：降级后的 IR 中不含字段访问

gfsim 执行是首个实现门槛。PYC/RTL 义务适用于获准的降级结果；拒绝临时存储的部分仍须明确后续工作。仅编译证据不能证明行为正确。

## 未决决策

- 冻结选定配置中每个源类别的 `CLASS_RESIDENCY_DEPTH`。`BANK_PARTITIONS`
  只是由 ARB 每拍退休上限推出的起始取值，不是经过尺寸论证的决策。bank 维度
  本身已确定且归属 ARB：每 PE 8 个私有 bank，四个 PE 共 32 个，依据见
  [bank.md](../trf/bank.md)。
- 在解释 A 与解释 B 之间冻结 FlowKey 到 `CellReadReq` 的字段映射，并确定
  `core_id`/`pe_id` 是否需要为父级下游的消费者保留显式字段。
- 确认"每个 PE 一个 `stid`"这一假设。若单个 PE 承载多个 `stid`，应在实例
  内部再加一层分区，而不是重新引入跨 PE 的路由维度。
- 定义每个源类别的取消匹配与过时代际处理；类别之间与 bank 之间的公平性属于
  ARB 而非 RQ。

## 贡献者闭环

- [ ] 认领候选项并确定其父级/包含状态所有者。
- [ ] 确定处置；别名和包含状态不得重复硬件。
- [ ] 将相关 NDF L0 意图和 L1 行为链接到此 L2 实现。
- [ ] 冻结端口载荷字段/位宽、生产者/消费者、父级连接面、状态/复位和时序配置。
- [ ] 冻结 CUBE/VEC/TLSU 队列维度、类别标签及每类别深度。
- [ ] 证明每类别 FIFO、独立反压及过时代际取消。
- [ ] 定义功能分支、全有或全无效果、争用以及取消/恢复生命周期。
- [ ] 为每个实际框架/原语缺口链接最小失败门槛，并先合入共享修复。
- [ ] 实现获准的所有者，并编写设计本地的预期结果测试。
- [ ] 在 gfsim 中证明反压、身份/代际、恰好一次效果及实例隔离。
- [ ] 集成到 H2/H1，并记录获准的 PYC/RTL 证据或剩余边界。

## 源证据

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:105` — RQ 被明确建议为状态模式。
- `docs/architecture/core/l3/CONTRACT_FOUNDATION.md:86` — 最近公共父级拥有连接面 Queue。
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:738`（经 [bank.md](../trf/bank.md)）— 32 个单端口 128 字节 bank；首个配置采用私有分组，即每 PE 8 个。
- [架构](../../ARCHITECTURE.md) — 四个由 `FlowKey(core_id, pe_id, stid, launch_generation)` 限定的独立 PE 控制流。

源路径使用冻结的外部仓库拼写（包括旧式 `l3/` 目录）；它们仅用于溯源，不代表新的层次结构术语。文件哈希及原始目录处置记录见 [catalog.json](../../catalog.json)。未决的全局决策参见 [架构](../../ARCHITECTURE.md)。
