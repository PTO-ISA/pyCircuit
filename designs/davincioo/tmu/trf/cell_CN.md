# TMU.TRF.CELL — Tile 单元

- 源候选项：`DAV-TMU-TRF-CELL-0001`
- 硬件层次：**H3**，位于 H1 `TMU` / H2 `TRF`
- NDF 细化：**L2 微架构**；模块级 L1 行为链接仍需评审。
- 建议处置：**状态模式**（提议，尚未获目录批准）
- 实现位置：使用 `designs/davincioo/` 下获准的包含所有者或契约文件；此目录处置不授权独立叶节点。
- 当前设计计划执行状态：**已作为包含状态实现**，位于 [bank.py](bank.py) 展开的
  两个阵列，以及 [contracts/tmu_trf.py](../../contracts/tmu_trf.py) 中的几何常量。
  这里刻意不存在 `cell.py`：本处置不授权独立叶节点，在此建模块会为同一份字节
  造出第二个所有者。

定义一个原始 128 字节载荷单元及其代际；不是单独调度的模块，也不是描述符所有者。

## 包含所有者

| 元素 | 所在位置 | 说明 |
| --- | --- | --- |
| 128 字节数据 | [bank.py](bank.py) 里的 `cells` 数组 | **一个条目就装一整个 cell。** 元素类型是 `CellData` struct，降级后大小正好 128 字节 |
| 单元代际 | [bank.py](bank.py) 里的 `generations` 数组 | 每行一个条目，因为一个代际管的是整个 cell |
| 布局几何 | [contracts/tmu_trf.py](../../contracts/tmu_trf.py) | `ROWS_PER_BANK`、`CELL_BYTES`、`WORDS_PER_CELL`、`CELL_READ_LATENCY` |

## 输入

无独立端口：此工作项是包含状态或迁移别名。请确定其包含所有者；不要再分配其他 Queue/状态所有者。

## 输出

无独立端口：此工作项是包含状态或迁移别名。请确定其包含所有者；不要再分配其他 Queue/状态所有者。

提议表格中的载荷名称在字段、位宽和名义身份冻结前均为设计伪类型。Queue 传输由编译器推断，不应要求公开的 Queue 包装器。时钟/复位/时间域属于执行上下文，不得臆造为普通载荷端口。

## 所有或包含的状态

- 128 字节载荷
- 单元代际
- 不含分配、shape、dtype、layout、valid-region 或定义性字段

## 需验证的能力

- 用 aggregate/array 精确表示 1024 位数据 —— **能做**。cell 就是一个扁平 struct，里面 16 个
  `u64` 字段，整个存在持久索引变量的一个条目里（Decision 0151）。这 16 个字段只是一份不可
  拆分数据的写法，不是能拿来寻址的坐标
- 带掩码的子字访问 —— **不需要**。访问粒度是一整个 cell，没有子字访问这回事

这些是待验证的要求，并不证明当前框架缺少相应能力。先测试当前修订版；确认缺口后，将其作为通用框架/原语问题加入回归，并先合入共享修复，再提交依赖设计的 PR。

## 行为验收

- 这套结构嵌在 BANK 自己的数组里 —— **成立**。两个数组都是 BANK 展开的，没有别的所有者
- CellKey 不能拿来当 TileVersion/TileLease 用 —— **结构上就成立**。BANK 压根不读
  `cell_key`：寻址用 `row`，判代际用 `tile_version`，就算写错也混不到一起
- 分配了不等于数据就有意义 —— **这里还证明不了**。cell 数组初始值是零，而 BANK 又不存
  "是否已定义"这类状态，所以没有任何东西能区分"刚分配"和"确实写过零"。要区分得靠分配器
  和一个管"是否已定义"的模块，前者是 [fre.md](../trn/fre.md)（不是 ALC，ALC 不做分配），
  两者都还没实现

设计局部证据：`designs/davincioo/tests/fabric/test_bank_physical_access.py`
（`test_cell_key_never_substitutes_for_tile_version`、
`test_cell_schema_lives_in_bank_owned_arrays_only`）。

gfsim 执行是首个实现门槛。PYC/RTL 义务适用于获准的降级结果；拒绝临时存储的部分仍须明确后续工作。仅编译证据不能证明行为正确。

## 未决决策

- **这 1024 位数据在 PYC/RTL 里怎么表示，还没定。** pyCircuit 这边已经定了：一个条目、一个
  扁平 struct、不要字节掩码——因为访问粒度就是一整个 cell，没人能请求 cell 的一部分。但这种
  存储在 Decision 0151 里算临时状态，PYC/RTL 必须拒绝，所以要有一个能降级的表示，还得等
  "memory 支持聚合类型"那个框架改动。边界和复现用例见 [bank.md](bank.md)。

## 贡献者闭环

- [ ] 认领候选项并确定其父级/包含状态所有者。
- [ ] 确定处置；别名和包含状态不得重复硬件。
- [ ] 将相关 NDF L0 意图和 L1 行为链接到此 L2 实现。
- [ ] 冻结端口载荷字段/位宽、生产者/消费者、父级连接面、状态/复位和时序配置。
- [ ] 定义功能分支、全有或全无效果、争用以及取消/恢复生命周期。
- [ ] 为每个实际框架/原语缺口链接最小失败门槛，并先合入共享修复。
- [ ] 实现获准的所有者，并编写设计本地的预期结果测试。
- [ ] 在 gfsim 中证明反压、身份/代际、恰好一次效果及实例隔离。
- [ ] 集成到 H2/H1，并记录获准的 PYC/RTL 证据或剩余边界。

## 源证据

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:110` — CELL 是状态模式，不是第二个所有者。
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:729` — 原始单元和 Tile 元数据由不同所有者持有。

源路径使用冻结的外部仓库拼写（包括旧式 `l3/` 目录）；它们仅用于溯源，不代表新的层次结构术语。文件哈希及原始目录处置记录见 [catalog.json](../../catalog.json)。未决的全局决策参见 [架构](../../ARCHITECTURE.md)。

