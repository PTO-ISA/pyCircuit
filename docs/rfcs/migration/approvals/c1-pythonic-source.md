# C1 源语言接口批准记录

状态：approved。日期：2026-09-27。批准者：本项目用户。

用户在本会话针对 PM 提交的 C1 修订 C 精确批准请求回复：

> 批准

批准对象：[C1 源语言接口修订 C](../c1-pythonic-source.md)，内容 SHA-256 为 `5768e1571e56eb1a5963ff5d40dff1087de38ee997e9c52b5ef91f520da90dfc`。原文保持冻结；原提案页眉的“待批准”描述的是送审时点，当前状态以本记录为准。

该修订已经独立 Astra xhigh 判定 approval-ready，见[精确审阅及冻结文本](../../../gates/logs/20260927-c1-source-review/revision-c-review.md)。用户请求之前 PM 已说明：本次只批准 C1，C2/C3 的 IR、CLI、headers、runtime ABI 及 memory/CDC/四态扩展仍另行提交。

## 批准范围

批准 C1 修订 C 的单一对象式源语言、registration/record/current-next、数学整数与范围/错误语义、fixed collection/static 参数规则，以及列明的旧源接口 hard-break 义务。准许按此完成源语义实现准备与测试设计；任何同时改变 C2/C3 接口的实现必须等待对应精确批准。

不扩大为未写明 IR op/type/attribute、CLI flag、header/schema、生成 API、runtime ABI 或扩展硬件绑定的批准。不批准把 M2 portless 首片作为完整框架收尾，不豁免逐源编译、双后端、能力矩阵或最终删除/gate 验收。

当前产品尚未切换；旧决定与文档在 M5 按批准提案同步更新，不伪称当前实现已满足新源合同。实质修订 C1 后需要重新独立审阅和用户批准；本记录不改写 donor 审批历史。
