# TMU.BGF.XBAR — 交叉开关

- 源候选项：`DAV-TMU-BGF-XBAR-0001`
- 硬件层次：**H3**，位于 H1 `TMU` / H2 `BGF`
- NDF 细化：**L2 微架构**；模块级 L1 行为链接仍需评审。
- 建议处置：**叶节点**（提议，尚未获目录批准）
- 拟定实现路径（若接受为独立叶节点）：`designs/davincioo/tmu/bgf/xbar.py`
- 当前设计计划执行状态：**已实现静态出口生成；gfsim 行为及 PYC C++/Verilog 构建已验证；H2/H1 集成仍待完成**。

现有已注册的传输叶节点；负责路由完整的 BGFPacket 事务，仅标记目的地交付。

## 输入

| 名称 | 载荷/类型 | 含义 | 证据状态 |
| --- | --- | --- | --- |
| ingress_0 | BGFPacket | 独立请求者入口 | 已声明 |
| ingress_1 | BGFPacket | 独立请求者入口 | 已声明 |
| ingress_2 | BGFPacket | 独立请求者入口 | 已声明 |
| ingress_3 | BGFPacket | 独立请求者入口 | 已声明 |

## 输出

| 名称 | 载荷/类型 | 含义 | 证据状态 |
| --- | --- | --- | --- |
| delivered | BGFPacket | 送达选定目的 bank 后的数据包；completed 和 delivered_bank 已设置 | 已声明 |

提议表格中的载荷名称在字段、位宽和名义身份冻结前均为设计伪类型。Queue 传输由编译器推断，不应要求公开的 Queue 包装器。时钟/复位/时间域属于执行上下文，不得臆造为普通载荷端口。

## 所有或包含的状态

- 仅包含内部合并、路由、目的地延迟及完成 Queue 阶段

## 需验证的能力

- 已支持的 merge/route/apply 层次
- 生产替代方案必须连接授权与 BANK 确认，不能将试验延迟当作 bank

这些是待验证的要求，并不证明当前框架缺少相应能力。先测试当前修订版；确认缺口后，将其作为通用框架/原语问题加入回归，并先合入共享修复，再提交依赖设计的 PR。

## 行为验收

- 每个入口内保持 FIFO，并在入口之间进行轮询仲裁
- 输出反压时保留完整数据包
- 交付标记表示传输完成，绝不表示 Tile 发布或 PTO 提交

gfsim 执行是首个实现门槛。PYC/RTL 义务适用于获准的降级结果；拒绝临时存储的部分仍须明确后续工作。仅编译证据不能证明行为正确。

## 未决决策

- 决定保留现有试验版作为生产 XBAR，还是替换为由 ARB 授权驱动的传输叶节点。

## 重点实现证据

- `designs/davincioo/tmu/bgf/xbar.py` 冻结四个入口端口、轮询式入口/完成合并、目的地路由及静态延迟
  `(1, 2, 4, 7)` 来自一个索引化的 `apply` 模板。
- `pytest -q designs/davincioo/tests/fabric/test_xbar_static_generation.py`
  检查冻结的数据包模式、精确的 Queue 拓扑及全部四个目的地，
  延迟偏移、完整数据包身份、争用下每入口 FIFO，
  反压保持、恰好一次完成、飞行中复位、活动
  实例隔离，以及 PYC C++/Verilog 构建闭环。
- 静态模板依赖于 [决策 0227](../../../../docs/rfcs/pyc6-decisions.md#decision-0227-static-queue-collections-elaborate-supported-operator-families)。
  框架和设计门槛结果归档于
  `docs/gates/logs/20260908-issue63-opt06-static-queue-array/` 和
  `docs/gates/logs/20260908-issue63-opt06-davincioo-xbars/`.

## 贡献者闭环

- [x] 认领候选项并确定其父级/包含状态所有者。
- [x] 确定处置；别名和包含状态不得重复硬件。
- [ ] 将相关 NDF L0 意图和 L1 行为链接到此 L2 实现。
- [ ] 冻结端口载荷字段/位宽、生产者/消费者、父级连接面、状态/复位和时序配置。
- [x] 定义功能分支、全有或全无效果、争用以及取消/恢复生命周期。
- [x] 为每个实际框架/原语缺口链接最小失败门槛，并先合入共享修复。
- [x] 实现获准的所有者，并编写设计本地的预期结果测试。
- [x] 在 gfsim 中证明反压、身份/代际、恰好一次效果及实例隔离。
- [ ] 集成到 H2/H1，并记录获准的 PYC/RTL 证据或剩余边界。

## 源证据

- `srcs/core/tmu/bgf/xbar.py:28` — 声明四个 Queue 输入和一个 Queue 输出。
- `docs/specification/davincioo/ndf-next/interfaces/modules.md:22` — 规范入口及完成边界。
- `docs/specification/davincioo/ndf-next/tile/tmu.md:69` — BGF 传输必须保留身份，且不得报告架构完成。

源路径使用冻结的外部仓库拼写（包括旧式 `l3/` 目录）；它们仅用于溯源，不代表新的层次结构术语。文件哈希及原始目录处置记录见 [catalog.json](../../catalog.json)。未决的全局决策参见 [架构](../../ARCHITECTURE.md)。

