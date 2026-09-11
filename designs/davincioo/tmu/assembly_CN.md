# TMU — H1 组装

NDF 细化：**L2 微架构**。Tile 描述符、版本化所有权及原始 bank。

拟定源文件：`designs/davincioo/tmu/assembly.py`。不宣称已有实现。

## 输入边界提议

| 名称 | 载荷/类型 | 含义 | 证据状态 |
| --- | --- | --- | --- |
| rename_request | TBD 类型化事务 | 组装边界提议；需核对确切的生产者和所有者。 | 提议 |
| bank_access | TBD 类型化事务 | 组装边界提议；需核对确切的生产者和所有者。 | 提议 |
| publish_request | TBD 类型化事务 | 组装边界提议；需核对确切的生产者和所有者。 | 提议 |
| reclaim_request | TBD 类型化事务 | 组装边界提议；需核对确切的生产者和所有者。 | 提议 |

## 输出边界提议

| 名称 | 载荷/类型 | 含义 | 证据状态 |
| --- | --- | --- | --- |
| rename_response | TBD 类型化事务 | 组装边界提议；需冻结完成语义和反压行为。 | 提议 |
| bank_response | TBD 类型化事务 | 组装边界提议；需冻结完成语义和反压行为。 | 提议 |
| publish_ack | TBD 类型化事务 | 组装边界提议；需冻结完成语义和反压行为。 | 提议 |
| reclaim_ack | TBD 类型化事务 | 组装边界提议；需冻结完成语义和反压行为。 | 提议 |

## 组合契约

- 父级拥有每个子到子连接面；子状态只有一个所有者。
- 保留 FlowKey、epoch/generation、载荷及相互区分的完成含义。
- 冻结共享资源仲裁及请求/响应/取消事务。
- 组合不是整核单一原子规则；必须显式保留多周期协议状态。
- 确切的载荷字段、位宽、数量、深度和时序在评审前均待定。

## 检查清单

- [ ] 将实现的 NDF L2 细节链接到 L1 行为和 L0 意图。
- [ ] 接受子项/包含状态的处置及实例几何参数。
- [ ] 冻结外部端口及生产者-消费者-连接面表。
- [ ] 实现参数传播和特化复用。
- [ ] 运行独立反压、取消、隔离和静默状态的组合测试。
- [ ] 记录 gfsim 结果及获准的 PYC/RTL 边界。

## 包含的 H3 候选项

- [DAV-TMU-BGF-ARB-0001](bgf/arb.md)
- [DAV-TMU-BGF-MAP-0001](bgf/map.md)
- [DAV-TMU-BGF-RQ-0001](bgf/rq.md)
- [DAV-TMU-BGF-WQ-0001](bgf/wq.md)
- [DAV-TMU-BGF-XBAR-0001](bgf/xbar.md)
- [DAV-TMU-TRF-ALC-0001](trf/alc.md)
- [DAV-TMU-TRF-BANK-0001](trf/bank.md)
- [DAV-TMU-TRF-CELL-0001](trf/cell.md)
- [DAV-TMU-TRF-RDY-0001](trf/rdy.md)
- [DAV-TMU-TRF-REF-0001](trf/ref.md)
- [DAV-TMU-TRN-CHK-0001](trn/chk.md)
- [DAV-TMU-TRN-FRE-0001](trn/fre.md)
- [DAV-TMU-TRN-LRM-0001](trn/lrm.md)
- [DAV-TMU-TRN-LTR-0001](trn/ltr.md)
- [DAV-TMU-TRN-RAT-0001](trn/rat.md)
- [DAV-TMU-TRN-STS-0001](trn/sts.md)
- [DAV-TMU-TUL-FLS-0001](tul/fls.md)
- [DAV-TMU-TUL-LBA-0001](tul/lba.md)
- [DAV-TMU-TUL-RCM-0001](tul/rcm.md)
- [DAV-TMU-TUL-REN-0001](tul/ren.md)
- [DAV-TMU-TUL-RET-0001](tul/ret.md)

[架构](../ARCHITECTURE.md) · [完整目录](../MODULE_CHECKLIST.md)
