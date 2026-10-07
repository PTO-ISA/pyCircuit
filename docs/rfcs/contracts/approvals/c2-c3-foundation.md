# C2/C3 基础合同批准记录

状态：approved。日期：2026-09-27。批准者：本项目用户。

用户在本会话针对 PM 的“请批准 C2-C 和 C3-C 两个精确修订”请求回复：

> 批准

批准对象及内容绑定：

- [C2 修订 C](../c2-mlir-contract.md)：SHA-256 `387cf52b129f864b87a1b2a388213a3fe36d81c330d94ced0e6696522b58a322`；[独立 Astra xhigh 审阅](../../../gates/logs/20260927-c2-review/revision-c-review.md)为 approval-ready。
- [C3 修订 C](../c3-driver-runtime.md)：SHA-256 `0c476ced27519cf93427a89f77b9348d388e96db57fb183790144d24118b1170`；[独立 Astra xhigh 审阅](../../../gates/logs/20260927-c3-review/revision-c-review.md)为 approval-ready。

原文保持冻结；提案页眉记录送审时点，当前批准状态以本记录为准。[C1-C 的既有批准](c1-pythonic-source.md)继续有效，无需重新请求。

## 准许实施的范围

C2-C 的逐源 MLIR/header、静态参数特化、普通 DFFE/current-next、数学整数和源求值检查、值/写入目标证明，以及 C++/Verilog 消费同一共同硬件 IR。C3-C 的唯一 compile/link/emit driver、生成文件与特化组织、发布和恢复协议、模型 ABI 行为、单 runtime 与 SDK 合同同时获准实施。

内部实现可按有独立测试和审查的工作包逐步完成；实际产品切换仍按 M5 一次 hard break 候选同步删除旧路线、更新决定/文档/安装资产。批准不代表实现或测试通过，也不授权用旧路线 fallback 补齐新路线。

本次不增加两份提案外的源/IR/CLI/runtime 接口。memory/CDC/四态、事务资源、外部 typed DUT、完整 system 和依赖参数的端口/record 类型等扩展继续单独设计、审阅和请求批准；其完整迁移验收义务没有删除。

实质修改批准文本需重新独立审阅与用户批准。整体目标仍是完整框架成熟和唯一编译路线，不以 foundation 或治理完成收尾。
