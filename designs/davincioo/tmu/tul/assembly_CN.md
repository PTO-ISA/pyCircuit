# TMU.TUL — H2 组装

NDF 细化：**L2 微架构**。迁移到 SPE.OOO 的那批名字。

不提议任何源文件。和 [BGF](../bgf/assembly_CN.md)、[TRF](../trf/assembly_CN.md)、
[TRN](../trn/assembly_CN.md) 不同，这个组装底下没有硬件可组：它的每个子项都是迁移别名，
真正的所有者在别处。在这里写 `assembly.py`，等于造出这些处置本来就要防的第二个所有者。

## 这个组装到底是什么

TUL 是一个**名字空间，不是一级流水线**。当标量 T/U 的所有权迁到 SPE.OOO 之后，那些旧名字
仍然会出现在数据包、issue 和评审意见里，所以每个名字都留了一张卡片，说明它的功能去了哪儿。
这就是这个 H2 的全部用途：有人拿着"重命名""冲刷""退休""重提交""延迟绑定"这些词找过来，
总得有个地方告诉他该去找谁。

[架构](../../ARCHITECTURE.md)把约束写得很直白：标量 T/U 的所有权属于 SPE.OOO，TUL 的名字
是迁移工作项，不是可以再造一个重命名或退休所有者的许可。

## 每个名字去了哪里

| 名字 | 标量侧所有者 | TMU 里对应的 Tile 侧 |
| --- | --- | --- |
| [REN](ren_CN.md) | [SPE.OOO.REN](../../spe/ooo/ren.md)、[MPQ](../../spe/ooo/mpq.md) | [RAT](../trn/rat_CN.md) 管名字到版本；[FRE](../trn/fre_CN.md) 管物理空间 |
| [LBA](lba_CN.md) | SPE.OOO 的重命名状态（[SMAP](../../spe/ooo/smap.md)、[MPQ](../../spe/ooo/mpq.md)） | [FRE](../trn/fre_CN.md) 把分配拆成预留和落定两步 |
| [RCM](rcm_CN.md) | [SPE.OOO.CMT](../../spe/ooo/cmt.md)、[BROB](../../spe/bctrl/brob.md) | [STS](../trn/sts_CN.md) 发布版本；[RAT](../trn/rat_CN.md) 发布名字 |
| [FLS](fls_CN.md) | [SPE.OOO.FLS](../../spe/ooo/fls.md) | [CHK](../trn/chk_CN.md) 与 [RAT](../trn/rat_CN.md) 回退；[FRE](../trn/fre_CN.md) 取消 |
| [RET](ret_CN.md) | SPE.OOO 的退休路径、[BROB](../../spe/bctrl/brob.md) | [REF](../trf/ref_CN.md) 释放物理读；[RAT](../trn/rat_CN.md)/[STS](../trn/sts_CN.md) 了结生命周期 |

右边一列不是左边那列换了个名字。Tile 版本和标量寄存器是两种资源、两种生命周期，两列在同一条
流里**并行**跑，不是谁实现谁。正因如此，每张卡片写的是"对应物"，而不是把人转发过去。

## 输入边界提议

没有独立端口。TUL 的边界是按处置留空的，不是漏写：每个子项指向的所有者都已经有自己的边界。
在这里加端口，只能是复制一个 SPE.OOO 或者 TMU 自己 TRN/TRF 组装里已经有的端口。

## 输出边界提议

没有独立端口，理由同上。

## 组装契约

- 父级拥有子项之间的每条连接面；子项状态只有一个所有者。这里没有连接面，因为这里没有带状态
  的子项。
- 迁移别名不得获得端口、状态或源文件。这是这个组装真正在执行的唯一一条组装规则。
- FlowKey、epoch/代际、载荷以及各不相同的完成含义，都在子项指向的那些所有者里保持。

## 检查清单

- [ ] 将实现该模块的 NDF L2 细节链接到 L1 行为和 L0 意图。
- [x] 确认子项/包含状态的处置与实例几何。每个子项都是没有实例、没有状态的别名，各自卡片
      记录了它的所有者。
- [x] 冻结对外端口以及生产者-消费者-连接面表。冻结的结论是：没有。
- [ ] 关闭各子卡片上剩余的旧 ID 迁移记录，包括 [ren_CN.md](ren_CN.md) 上的 `DAV-OQ-OOO-0001`。
- [ ] 确认在全部子项的迁移记录关闭之后，这个 H2 本身是退役，还是作为词汇保留。

这个组装不欠 gfsim 或 PYC/RTL 证据。这里没有东西可执行；证据义务属于上表里的那些所有者。

## 包含的 H3 候选项

- [DAV-TMU-TUL-FLS-0001](fls_CN.md)
- [DAV-TMU-TUL-LBA-0001](lba_CN.md)
- [DAV-TMU-TUL-RCM-0001](rcm_CN.md)
- [DAV-TMU-TUL-REN-0001](ren_CN.md)
- [DAV-TMU-TUL-RET-0001](ret_CN.md)

[架构](../../ARCHITECTURE.md) · [完整清单](../../MODULE_CHECKLIST.md)
