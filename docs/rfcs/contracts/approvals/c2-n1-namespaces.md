# C2-N1 命名空间增补批准记录

状态：approved。日期：2026-09-28。批准者：本项目用户。

用户在本会话对“是否批准 C2-N1 修订 C”的精确请求回复：

> 批准 C2-N1 修订 C

批准对象：[C2-N1 修订 C](../c2-n1-namespaces.md)。内容绑定为
`ac7d56a403e21ac03186f17c2f75f9c8eb53a00dcb74677c5286219dc5f1e6c3`；
[独立 Astra xhigh 审阅](../../../gates/logs/20260928-c2-n1-review/revision-c-review.md)
结论为 approval-ready，验证包 R1 已关闭。

## 准许实施的范围

为五类既有声明增加 ac.exports、ac.import_bindings 两个必需 source-unit
属性；实施 named import/re-export、canonical declaration authority 与名称
authority 的分离、逐源消费快照、header/body 镜像、过期绑定拒绝，以及
final IR 前删除元数据。完整范围和验收要求以冻结的修订 C 为准。

C1-C/C2-C/C3-C 的原批准继续有效，不重复请求。本批准不扩展到 star import、
__all__、namespace value/intrinsic re-export、资源/memory/CDC/四态或其他
尚未批准的源/IR/runtime 接口，也不把它们永久退役。

批准不是实现或测试通过。N1 仍须按结构、真实逐源/header、link 和两个 emit
入口完成独立验收；当前 U01 证据不能替代。提案原文保持冻结，其送审页眉
不改写，批准状态以本记录为准。实质修改需重新独立审阅和用户批准。
