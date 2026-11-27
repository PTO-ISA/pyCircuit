# 0008. 计数溢出拒绝片段并保留此前状态

- **Status**: accepted
- **Date**: 2026-11-03
- **Design sources**: Calendar §4.5.2，接收契约 6
- **Code**: `ReceiveAccount.deliver`、迁移层 `ReceiveEngine`

## Context

Calendar 要求累计字节超过接收容量时报故障。原 Python 核心先增加计数、
记录身份，再抛出异常；迁移层先检查候选总量，发生溢出时保留已有计数和身份。
两者在故障后的可观察状态上存在差异。

## Decision

用户确认采用：拒绝造成溢出的片段，保留此前计数和去重身份，再报告接收契约
故障。Python 核心先检查候选总量，通过后同时更新计数和身份。迁移层仍保持
已有的故障锁存行为，直到重置；Python 核心仍通过异常把故障交给调用者处理。

这项决定针对累计字节溢出。有限去重表的模型容量不足仍遵循决定 0007。

## Consequences

回归固定接收容量 96 B，两个片段的地址范围均合法。先接收 64 B，再提交
另一个 64 B 片段，此时应报错且保留 64 B 与原身份集合。随后在 Python 核心
使用被拒绝的同一身份提交 32 B，验证失败没有提前消耗该身份。迁移层故障后
由锁存故障阻止继续接收，因此后端测试检查故障状态及保留的公开计数。

## References

- [Calendar 方案](../design/wse-calendar-scheme.md)
- [核心回归](../../tests/unit/test_receive_contracts.py)
- [有限资源测试模型](0007-finite-receive-test-boundaries.md)
