# C2-R1/M1 联合接口批准记录

状态：approved。日期：2026-09-28。批准者：本项目用户。

用户在完成 Bluespec interface/effect 参考吸收、纯 Pythonic 边界、
W00–W12 checklist 和 V00–V49 verification matrix 后明确回复：

> 好的，按照这个计划来实现。你是本项目经理，开始启动不同的subagent按照计划来吧

批准并授权实施的精确对象：

- [C2-R1 统一寄存器修订 B](../c2-r1-unified-register.md)：
  SHA-256 `76283661bffba96aad544e31d2b4996f019135f448c4c8b685ebb02eab8edfba`；
  [独立设计审阅](../../../reviews/20260928-c2-r1-unified-register.md)为 approval-ready。
- [C2-M1 模块/System/接口修订 C](../c2-m1-module-system.md)：
  SHA-256 `84b551ea84d6aa0956ea2342f520f4aafc63f7d741cc651500446a26f49c35cd`。
- [迁移执行 checklist](../../../development/migration-agent-checklist.md)：
  SHA-256 `7e7791e3524a0a9feca11ce63afb9fb6bcf50f2a1d95dabda2b8042fed34fe44`。
- [迁移 verification matrix](../../../development/migration-verification-matrix.md)：
  SHA-256 `6f82ce4c2cc124bd4597a54809816628fc6795bded10e54c90fcf97616b216ea`。

后三项由独立 `gpt-6-astra/high` reviewer 绑定并判定
[approval-ready](../../../reviews/20260928-interface-handoff.md)。审阅涵盖
Python 无 Queue/Interface DSL、MLIR 自动 interface/effect 推导、唯一
owner、system Work/precommit/Xfer、固定事件出口、源码重排比较、
QUIESCENT，以及执行/验证防假通过约束。

## 授权范围

授权 PM 按 W00–W12 依赖顺序分解、派发、实现、测试、修补、审阅和
整合上述合同。允许对批准范围内的 ACIR op/type/attribute、Python
source contract、header/link schema、internal passes、GFSIM-derived
runtime、C++/Verilog lowering、runner、工具事件出口、构建和 gates
执行一次 hard-break 实现；不要求每个已批准的普通实现步骤重新询问。

实现必须保持：Python 只有模块函数/嵌套 rule/普通 Python 值与注解，
没有 Queue/FIFO、手写 Interface、Reg/Signal、方向/effect/调度 API；
MLIR 从实际 source reads/uses 和 child bindings 推导 interface。不能
引入 implicit producer backpressure、默认 writer priority、旧路线
fallback 或 backend 语义补丁。

## 不扩张的范围

本批准不自动批准尚未冻结的参数化/动态 collection carrier、完整
circular-buffer 库及其外部协议、外部 typed DUT ABI、多时钟/CDC、
四态 source 值、开放 RTL fault continuation 或外部发布。执行包遇到
这些边界须 fail closed，并按 checklist 交还 PM；不能自行永久退役
能力或发明接口。

批准不是实现或测试通过。每个包仍须候选绑定、独立测试、独立 review
和 PM 验收；Vxx planned tests 只有真实创建并执行后才能标 pass。
实质修改上述精确合同需要新摘要、独立复审和用户批准。
