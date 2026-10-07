# C3 compile/link driver 实施批准记录

状态：approved。日期：2026-09-30。批准者：本项目用户。

批准对象是**实施已批准的 C3-C 接口**，不是新接口。C3-C 正文
（[`c3-driver-runtime.md`](../c3-driver-runtime.md)，SHA-256
`0c476ced27519cf93427a89f77b9348d388e96db57fb183790144d24118b1170`）已在
[c2-c3-foundation](c2-c3-foundation.md) 获用户批准并保持冻结；本次批准前复核该哈希
与批准记录一致，正文未被修改。

## 用户批准的内容

用户先回复"批准"，随后在四项待批准事项中选定"**下一包：link/emit 交付集成（public C3
命令）**"，并对随后的三个边界问题给出选择。据此本次批准绑定为：

- **不引入 C3 `emit` 子命令，留到 M5。** 该名字已被旧路线占用（旧 `emit` = Python 设计
  文件 → `.pyc`），而 C3-C 要求唯一 driver 的 compile/link/emit 且不保留 alias，批准的
  迁移计划又规定旧路线在 M5 一次性 hard break 才退役。因此本包既不提前替换，也不改名或
  加别名。
- **本包交付 `pycircuit compile` 与 `pycircuit link`**（批准时两个子命令名均空闲），
  命令行形状严格取 C3-C §compile / §link，不增加 C3-C 之外的选项。
- **`link --parameters` 非空即 fail closed。** native linker 目前没有任何 static
  parameter/binding 入口，本包如实声明该缺口，不猜测、不假装支持；省略 `--parameters`
  与显式空数组等价于"无绑定"。
- **私有 `--entry-owner-out` 通道获准加入。** program 的 publication owner 需要 root 源
  的 SourceOwner，而只有 native linker 知道它（`FinalProgram.h` 的 `ModuleSnapshot` 同时
  持 `owner` 与 `definition`，`runLink` 已取得 root）。用户批准按上一包 `--deps-out` 的
  同一模式，在私有 `acir-design-harness` 上增加该输出通道。它不改变 ODS/IR/公开
  CLI/runtime，也不是公开协议。
- 用户同时确认：将来批准两份修订 B 提案时，批准记录**绑定到已归档的被审字节**。

## 未被本次批准覆盖

- 一等 `ac.system`（[C2-SYSTEM 修订 B](../c2-system-definition-role.md)，`db81dbc8…`）与
  `ac.expect` 逐字段冻结（[C2-EXPECT 修订 B](../c2-expect-schema.md)，`e8f287e7…`）
  **仍未获用户批准**，本轮未实施。删除 `acir-design-harness` 中私有
  `@system ⟹ testbench` 规则属该提案范围，本轮同样未改。
- C3 `emit` 子命令、static parameter 特化、per-implementation-source `.hpp/.cpp` 分组、
  `generated.json` 生产、C3-C 要求的 native verify-only 入口均**不包含**在本包。
- 本批准不表示实现或测试通过，也不授权用旧路线 fallback 补齐新路线。实现验收仍以各 lane
  证据、独立测试与独立审阅为准，并须在账本中如实记录未实现范围。
