---
doc_id: DOC-DAV-SPE-OOO-ROB-ALLOCATE
status: draft
authority: normative
owner: spe-ooo
---

# ROB `allocate` 规则契约

本文是 NDF format 0.2 的规则级示范，细化层为 DavinciOO **L2 微架构**。
文件对应一个规则定义；单 ROB、双 ROB 以及不同 flow 的实例共享这些条款。
条款 ID 为本示范提出的稳定身份，生命周期为 `draft`，尚未完成项目级登记和评审。

实现位置：[rob.py 中的 allocate 规则](../../rob.py)。
模块总览、其他规则及已有设计决议见 [ROB 模块卡](../../rob.md)。
本文整理已确认的局部实现契约，不据此宣称满足完整 OOO 或 PTO 架构行为。

## 规则边界 {#DAV-SPE-OOO-ROB-ALLOC-0001}
<!-- ndf: kind=definition refinement=L2 domain=spe.ooo status=draft -->

`allocate` 接收 `allocate_request` Queue 中的一条 `RobEvent`，成功时向
`allocated` Queue 发布一条 `RobEvent`。绑定的 flow 由结构参数
`core_id`、`pe_id`、`stid`、`launch_generation` 共同确定。

| 类别 | 内容 |
| --- | --- |
| 读取状态 | `tail`、`count`、`epoch`、`recovering`、`entries[tail]` |
| 修改状态 | `entries[tail]`、`tail`、`count` |
| 不在本规则内完成的工作 | 执行结果接收、向 CMT 交接、ack 释放、flush 状态转换 |

`allocated` 是分配身份回复，不是执行完成消息，也不是架构提交事件。

## 分配资格 {#DAV-SPE-OOO-ROB-ALLOC-0002}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=spe.ooo status=draft -->

一次成功分配 MUST 同时满足以下条件：

1. `count < 16`，且 `recovering == False`。
2. 请求的 `valid == True`，`kind == ALLOCATE`。
3. 请求的 `epoch.flow`、`inst.flow`、`block.flow`、`rob.flow` 均等于实例绑定的 flow。
4. 请求的 `epoch.recovery_epoch` 等于当前 `epoch`。
5. 本规则能够在原子提交边界发布 `allocated` 回复并提交全部状态更新。

正常运行中，已有事务等待 CMT ack（`pending == True`）本身 MUST NOT 禁止
新分配；是否可以分配仍由上述条件决定。等待 ack 的事务继续计入 `count`。

## 槽位与身份 {#DAV-SPE-OOO-ROB-ALLOC-0003}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=spe.ooo status=draft depends-on=DAV-SPE-OOO-ROB-ALLOC-0002 -->

成功分配 MUST 使用提交前的 `tail` 作为槽位，生成：

- `RobKey.flow`：实例绑定的 flow；
- `RobKey.slot`：提交前的 `tail`；
- `RobKey.generation`：该槽位保存的旧 generation 加一，按 16 位无符号数回绕。

槽位和 generation MUST 由 ROB 生成，MUST NOT 直接沿用请求提供的对应值。
其余指令、block 和 epoch 身份保持请求值。复位后槽位 generation 为零，
所以该槽位第一次分配得到 generation 一。

## Entry 初始化 {#DAV-SPE-OOO-ROB-ALLOC-0004}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=spe.ooo status=draft -->

成功分配 MUST 以请求为基础构造 entry，并替换以下字段：

| 字段 | 分配后的值 |
| --- | --- |
| `kind` | `ALLOCATED` |
| `rob` | [[DAV-SPE-OOO-ROB-ALLOC-0003]] 定义的新身份 |
| `result`、`fault_code`、`fault_arg0` | `0` |
| `result_valid`、`fault_valid`、`fault_bi` | `False` |
| `done`、`handoff_pending` | `False` |
| `status` | `TerminalStatus.VALUE` |

其他字段保持请求值，包括目标标识、handoff owner mask 和 history 数量。
`status=VALUE` 在这里是初始化值；结果是否有效仍由 `result_valid` 表达。

## 原子提交与阻塞 {#DAV-SPE-OOO-ROB-ALLOC-0005}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=spe.ooo status=draft depends-on=DAV-SPE-OOO-ROB-ALLOC-0003,DAV-SPE-OOO-ROB-ALLOC-0004 -->

以下动作 MUST 在同一条规则的原子提交边界全部发生，或全部不发生：

1. 消费当前分配请求。
2. 将新 entry 写入提交前的 `entries[tail]`。
3. 将 `tail` 加一并按 16 个槽位回绕，将 `count` 加一。
4. 将与所安装 entry 完全一致的回复发布到 `allocated` Queue。

容量不足、恢复等待、请求身份不合法或输出背压时，本规则 MUST 保留输入请求，
MUST NOT 修改槽位、generation、`tail` 或 `count`，也 MUST NOT 发布分配回复。
这不限制其他规则独立更新它们拥有或共享的状态。

解除暂时阻塞后，仍合法的请求 MUST 保持可被重新调度和分配的资格；本文不规定
固定的完成周期数，也不将仿真器一次 `step()` 等同于一个硬件周期。

## 恢复与有限标签的环境约束 {#DAV-SPE-OOO-ROB-ALLOC-0006}
<!-- ndf: kind=constraint modality=must refinement=L2 domain=spe.ooo status=draft -->

当有效 flush 与分配竞争共享状态时，模块调度 MUST 优先处理 flush。
`allocate` MUST NOT 独立推进恢复 epoch，也 MUST NOT 释放已交接的事务。
具体 flush/ack 状态转换由 ROB 模块契约定义。

上游 MUST 处理恢复前遗留在分配输入中的旧 epoch 请求，否则它们可能持续阻塞
后续请求；本规则不提供丢弃非法分配请求的通道。

环境 MUST 保证旧响应不会跨越同一槽位 65536 次复用或 65536 次恢复仍存活。
复位前响应 MUST 在复位边界被隔离或排空；复位不保留 generation/epoch 历史。

## 容量、复用及回复背压验证 {#DAV-SPE-OOO-ROB-ALLOC-VER-0001}
<!-- ndf: kind=verification modality=must refinement=L2 domain=spe.ooo status=draft verifies=DAV-SPE-OOO-ROB-ALLOC-0002,DAV-SPE-OOO-ROB-ALLOC-0003,DAV-SPE-OOO-ROB-ALLOC-0004,DAV-SPE-OOO-ROB-ALLOC-0005 -->

验收 MUST 验证：填满 16 项后第 17 个请求保持未分配；可靠 ack 释放容量后能够
继续分配；复用槽位的 generation 增加；分配回复与独立构造的预期 entry 完全
一致；输出背压期间没有提前分配，解除背压后没有请求丢失或重复分配。

当前证据：[单 ROB 测试](../../../../tests/spe/ooo/test_rob_single.py) 的
`capacity`、`backpressure` 场景，具体输入与期望值位于
[共享驱动](../../../../tests/spe/ooo/rob_driver.cpp)。这些是已有证据入口，
尚未建立项目级 NDF test/evidence 图节点。

## 恢复期间分配验证 {#DAV-SPE-OOO-ROB-ALLOC-VER-0002}
<!-- ndf: kind=verification modality=must refinement=L2 domain=spe.ooo status=draft verifies=DAV-SPE-OOO-ROB-ALLOC-0002,DAV-SPE-OOO-ROB-ALLOC-0006 -->

验收 MUST 验证：flush 保留旧交接且等待 ack 时，新 epoch 的合法分配请求保持
等待；该 ack 完成后恢复分配，并使用新的 epoch 身份。

当前证据：单 ROB 测试的 `recovery` 场景。非法分配输入的全部字段组合以及
16 位标签完整回绕尚无本示范对应的穷举证据，不以已有场景宣称全部覆盖。

## 上层精化关系待审 {#DAV-SPE-OOO-ROB-ALLOC-Q-0001}
<!-- ndf: kind=question refinement=L2 domain=spe.ooo status=open -->

需要核对原始 NDF 中的 L0/L1 条款，确定本规则分别承担哪些有序执行、恢复和
架构状态保持责任，以及哪些责任必须由 ROB/CMT/BROB 组合完成。
完成评审之前，不填写未经核实的 `refines` 或跨项目 `conforms-to` 关系。

项目级 `ndf.yaml`、`ndf.lock`、ID 登记以及源码/测试关系标注不在本示范中建立。
本文不能单独替代完整项目的 NDF build/check/coverage 验证。
