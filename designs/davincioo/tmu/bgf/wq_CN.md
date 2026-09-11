# TMU.BGF.WQ — 写队列

- 源候选项：`DAV-TMU-BGF-WQ-0001`
- 硬件层次：**H3**，位于 H1 `TMU` / H2 `BGF`
- NDF 细化：**L2 微架构**；模块级 L1 行为链接仍需评审。
- 建议处置：**状态模式**（提议，尚未获目录批准）
- 实现位置：使用 `designs/davincioo/` 下获准的包含所有者或契约文件；此目录处置不授权独立叶节点。
- 当前设计计划执行状态：**代码已实现，还没跑 gfsim 验证**。实现方式是由父层拼装，WQ 自己
  不声明独立的所有权。

WQ 就是写请求排队等着的地方：每个源类别一条队列，装的是 MAP 已经定好位置、但下游这会儿还
接不走的请求。

## 它解决什么问题

MAP 已经算出这次写该去哪个 bank 了，但那个 bank 这一拍可能正忙——别的源类别在用它。请求总得
有地方等，WQ 就是这个地方：每个源类别一条队列。

关键在于 WQ **不**做什么。它什么决策都不做：从不看手上这条记录的内容，不挑谁先谁后，也完全
不带 bank 维度。谁先过是 ARB 的事。这一点是可以查证的，不只是文档里的一句声明：
`bgf_wq_system` 降级后的 IR 里既没有 `ac.route`，也没有任何 `ac.var.get`。

隔离性就靠"每个类别一条独立队列"这件事。某个类别堵住了，占不到别的类别的位置；某个 PE 实例
堵住了，也拖不住别的实例。

WQ 也不是独立的状态所有者。它代表的是父层持有的那组写队列。写请求排在队列里确实是真实的状态，
但这份状态归包着这些队列的那层 BGF 组装，不足以让 WQ 成为第二个策略所有者。

## 在系统里的位置

一次 cell 写在 BGF 里的顺序是固定的：客户端寻址一个 cell，MAP 解出这个 cell 在哪，WQ 把请求
存着直到下游接得走，ARB 把它扇出到各 bank 并裁决冲突，XBAR 把授权送到目标 bank，BANK 真正
落字节。WQ 只负责其中一环——等。

| 谁 | 管什么 | 不管什么 |
| --- | --- | --- |
| CUBE / VEC / TLSU | 要写哪个 cell，以及写什么数据 | 这个 cell 在哪个 bank |
| [MAP](map.md) | 把 `cell_key` 解成 `(bank, row)` | 排队和冲突裁决 |
| **WQ** | **按源类别各排一条队，并保证类别内 FIFO** | **记录的内容、任何取舍、任何 bank 维度** |
| [ARB](arb.md) | bank crossbar、冲突裁决、公平性与老化 | 存请求 |
| [XBAR](xbar.md) | 把已授权的请求送到目标 bank | 授权给谁 |
| [BANK](../trf/bank.md) | 数据字节本身 | cell 放在哪、谁先过 |

读路径是完全对称的一份：[rq.md](rq.md) 是 `CellReadReq` 的同一套东西，而 WQ 不会去给自己的
写队首和 RQ 的读队首排先后。再往 Tile 那边看，[FRE](../trn/fre.md) 决定一个版本占哪块物理
空间，[REF](../trf/ref.md) 数还有几个人在读它——这两件事在这里都看不见。

## 一个请求怎么走完

一条 `CellWriteReq` 从 MAP 过来时，`source_class`、`bank`、`row`、`out_of_range` 都已经带好了。
`source_class` 决定它进哪条队列，那条队列有 `CLASS_RESIDENCY_DEPTH` 个位置。每条类别队列的
队首都暴露给 ARB，而只有 ARB 真的取走了，队首才算被消费掉。ARB、XBAR 或 BANK 确认槽位堵住时，
队首就留在原地，而且反压只往它自己这个类别传。

记录连带要写的数据原样传过去，中间那个什么都不改的 transform 只是用来挂
`CLASS_RESIDENCY_DEPTH` 的——队列前端没有单独的 buffer 算子，而对外的端口深度固定是 1。

`CLASS_RESIDENCY_DEPTH` 是一个类别**总共**能有多少请求在途，不是每个 bank 的额度。初始值取
`BANK_PARTITIONS`，理由是 ARB 每拍每个 bank 最多放走一个请求，所以一个类别里排着超过
`BANK_PARTITIONS` 个请求时，它们不可能全都互不冲突。这只是个起点，容量还没最终定。

## 输入

| 名称 | 载荷/类型 | 含义 | 证据状态 |
| --- | --- | --- | --- |
| cube_write / vec_write / tlsu_write | CellWriteReq | 单个 PE 的某一源类别提供的带代际限定整 cell 写入，已由 [map.md](map.md) 打标并完成放置解码 | 已实现 |

## 输出

| 名称 | 载荷/类型 | 含义 | 证据状态 |
| --- | --- | --- | --- |
| cube_write / vec_write / tlsu_write | CellWriteReq | 每个源类别一条排队中的写请求流，由父层连到这个 PE 的 ARB；每个 PE 实例 3 条 | 已实现 |

提议表格中的载荷名称在字段、位宽和名义身份冻结前均为设计伪类型。Queue 传输由编译器推断，不应要求公开的 Queue 包装器。时钟/复位/时间域属于执行上下文，不得臆造为普通载荷端口。

## 所有或包含的状态

- Queue 占用属于 BGF 父级，而非独立的 WQ 模块

## 参考实现

[`wq.py`](wq.py) 定义 `bgf_wq_system`；`CellWriteReq` 和位置几何参数通过
[`contracts/tmu_bgf.py`](../../contracts/tmu_bgf.py) 共享。WQ 给每个客户端类别留一条深度为
`CLASS_RESIDENCY_DEPTH` 的队列，除此之外什么都不做。数据作为一条完整记录整体传输，不做拆分、
不做合并，也没有"只接受一部分"这回事。

FlowKey 映射、`launch_generation` 字段、placement 解码、`bank`/`row` 的位宽
理由以及 `CLASS_RESIDENCY_DEPTH` 的取值理由与 `CellReadReq` 共用，记录在
[rq.md](rq.md) 与 [map.md](map.md)。包含它的 BGF 组装仍是状态所有者。

## 与卡片原设想的差异

**WQ 里没有 bank 维度。** bank 只是个调度坐标，所以整个 bank 维度归 [arb.md](arb.md)——真正
解决 bank 冲突的是它。`bank` 和 `row` 跟着请求记录走：[map.md](map.md) 解码一次，ARB 读一次，
WQ 只负责搬，不看内容。要是把 ARB 的 crossbar 拆开、把它的入口那一半放进 WQ，展开出来的
`(source_class, path, bank)` 边一模一样，隔离性不多也不少——因为不管画在哪边，队头阻塞点都在
扇出的入口。

**FlowKey 不再用来选路。** 卡片原来把一条流当成路由维度看，但现在一次 `bgf_wq_system` 展开就
是一个 PE，FlowKey 的路由作用直接被实例几何吸收了。它仍然跟着记录走，用来标身份和匹配取消——
所以 [rq.md](rq.md) 里那套映射还是得冻结，即便 WQ 自己一个字段都不读。

**队列深度挂在一个空的 transform 上。** 框架里没有可以直接实例化的公开 Queue 包装器，队列前端
也没有单独的 buffer 算子，所以能挂 `CLASS_RESIDENCY_DEPTH` 的地方只剩一个原样返回输入的
transform。反过来这也有好处：正因为函数体是空的，"WQ 不做任何决策"变成了降级后 IR 的一个性质，
而不是这张卡片里的一句话。

## 物理前提

[架构](../../ARCHITECTURE.md)中的四个 PE 控制流在物理上彼此独立：每个 PE
拥有私有的 cell 寄存器 bank 组和自己的仲裁。[bank.md](../trf/bank.md) 记载
共 32 个单端口 128 字节 bank 并采用私有分组，即**每个 PE 8 个 bank**。

所以这里描述的是**一个 PE**。PE 的身份体现在"`bgf_wq_system` 每个 PE 展开一份"这件事上，
而不是模块内部的某个路由维度。队列、游标、老化值、公平性状态全都不跨 PE 共享。四份实例由父层
组装出来。

## 提议的队列分区（需评审）

WQ 是由父级拥有的队列状态模式，但写请求与读请求一样采用源类别分区
纪律：分区键就是 `source_class` 本身。以下拓扑仅供评审，尚未获目录批准：

```text
单个 PE 实例
CUBE / VEC / TLSU                      （只寻址 cell，不带 bank 维度）
              |
              v
             MAP                       （打标 source_class；解码 bank/row）
              |
              v
     write_q[source_class]             （WQ；只负责排队，不带 bank 维度）
              |
              v
             ARB                       （拥有 bank crossbar 与冲突裁决）
              |
              v
          XBAR/BANK                    （每 PE 一套，不跨 PE 共享）
```

- `source_class` 是覆盖 `CUBE`、`VEC`、`TLSU` 的封闭类别标签，这三者是仅有的
  会访问 cell 寄存器的单元；MAP 在进入 WQ 之前就写好了这个标签，写请求经过 ARB 时
  必须保留。
- PE 身份属于实例几何，而非模块内部的分区键。由于每个 PE 单独展开，不同 PE
  流的相同逻辑 ID 在构造上就不可能共享顺序或取消状态。
- `bank` 是 PE 私有的 bank 索引，取值 `[0, BANK_PARTITIONS)`，即该 PE 内的
  物理 bank。在 BGF 内部它之所以放在请求记录里，正是为了让 WQ 不需要 bank
  维度：MAP 写入、ARB 读取。
- 每 bank 的独立性由 ARB 提供：它的 crossbar 为每条
  `(source_class, path, bank)` 边单独缓冲，因此某个 bank 在下游受阻时只占住
  它自己的边。
- 队列条目携带完整的 `CellWriteReq` 身份和效果字段：`FlowKey`、
  epoch/generation、`TileVersion`/`CellKey`、请求 ID、响应路由，以及要写的数据。
  MAP 追加 `source_class`、`bank`、`row` 与 `out_of_range`；
  这里不会替换任何源身份。

## 队列协议与写入顺序

- 仅当选定的类别队列有容量，且下游写入/确认路径能够保留该条目时，才接受
  `write_enqueue`。
- 每个类别队首只有在成功传输到 ARB 时才被消费。ARB、XBAR 或 BANK 确认槽位
  受阻时，必须保留完整的写入，且反压只传播到它自己的类别：任一类别不得
  拖住其他类别，任一 PE 也不得拖住其他 PE。某个 bank 的反压只能经由 ARB
  crossbar 上属于该类别的那条边传到该类别。
- 单个 PE 实例内，每个 `source_class` 必须保持 FIFO。跨类别顺序不由 WQ
  提供；冲突调度由 ARB 负责。
- WQ 不在类别之间做任何选择。它只按类别各暴露一个队首；round-robin 或
  优先级取舍属于 ARB，其固定优先级次序与老化升级记录在 [arb.md](arb.md)。
  该次序把写排在同一源类别的读之前，但 WQ 自身不对 RQ 定序，该顺序由 ARB
  裁决。
- 写请求在入队和传输到 BANK 时都必须全有或全无。除非后续配置明确规定，不得隐式拆分、合并、
  发布或部分接受——而且目前这些根本表达不出来：共享记录里没有掩码字段，BANK 一个条目就是一整个
  cell。
- 恢复只能取消尚未被 BANK/XBAR 接受的写请求。取消匹配 `FlowKey` 和
  epoch/generation，过时代际不得修改复用的单元。

## 需验证的能力

- 父层持有的、可参数化的 Queue 数组
- 借用子模块的 Queue 端口，而不复制一份状态
- CUBE、VEC、TLSU 的流量按类别分开排队，队列里不带 bank 维度
- 每个类别的深度、占用及入队/出队计数
- 自 MAP 交接起、直至经过 ARB，保留写入身份、类别标签与放置字段，
  而且 WQ 自己不去读它们
- 各个 PE 实例互相隔离，不共享队列，也不共享公平性状态

这些是待验证的要求，并不证明当前框架缺少相应能力。先测试当前修订版；确认缺口后，将其作为通用框架/原语问题加入回归，并先合入共享修复，再提交依赖设计的 PR。

## 行为验收

- 每个 `source_class` 保持 FIFO 顺序
- 保留 TileVersion、操作代际、请求 ID 和响应路由
- 每个类别具有独立容量和反压：某个类别受阻不影响其他类别，某个 PE 实例受阻
  不影响其他实例。每 bank 的独立性是 ARB 的验收义务，不是 WQ 的
- 类别内不得重排
- 不得部分接受或隐式发布
- 不读取任何放置字段：降级后的 IR 中不含字段访问

gfsim 执行是首个实现门槛。PYC/RTL 义务适用于获准的降级结果；拒绝临时存储的部分仍须明确后续工作。仅编译证据不能证明行为正确。

## 未决决策

- 冻结选定配置中每个源类别的 `CLASS_RESIDENCY_DEPTH`；写条目还要装要写的数据，深度未必和读
  一样。bank 维度本身已确定且归属 ARB：每 PE 8 个私有
  bank，四个 PE 共 32 个，依据见 [bank.md](../trf/bank.md)。
- 确认与 [rq.md](rq.md) 共享的"每个 PE 一个 `stid`"假设。若单个 PE 承载多个
  `stid`，应在实例内部再加一层分区，而不是重新引入跨 PE 的路由维度。
- 定义取消匹配、过时代际处理以及 WQ 到 ARB/BANK 的交接。字节掩码语义不在其中：访问粒度就是
  一整个 cell，共享记录里也没有掩码。

## 贡献者闭环

- [ ] 认领候选项并确定其父级/包含状态所有者。
- [ ] 确定处置；别名和包含状态不得重复硬件。
- [ ] 将相关 NDF L0 意图和 L1 行为链接到此 L2 实现。
- [ ] 冻结端口载荷字段/位宽、生产者/消费者、父级连接面、状态/复位和时序配置。
- [ ] 冻结 CUBE/VEC/TLSU 队列维度、类别标签及每类别深度。
- [ ] 证明每类别 FIFO、独立反压、全有或全无写入及过时代际取消。
- [ ] 定义功能分支、全有或全无效果、争用以及取消/恢复生命周期。
- [ ] 为每个实际框架/原语缺口链接最小失败门槛，并先合入共享修复。
- [ ] 实现获准的所有者，并编写设计本地的预期结果测试。
- [ ] 在 gfsim 中证明反压、身份/代际、恰好一次效果及实例隔离。
- [ ] 集成到 H2/H1，并记录获准的 PYC/RTL 证据或剩余边界。

## 源证据

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:106` — WQ 被明确建议为状态模式。
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:805` — 统一的 bank 请求生命周期及 Queue 所有权。

源路径使用冻结的外部仓库拼写（包括旧式 `l3/` 目录）；它们仅用于溯源，不代表新的层次结构术语。文件哈希及原始目录处置记录见 [catalog.json](../../catalog.json)。未决的全局决策参见 [架构](../../ARCHITECTURE.md)。
