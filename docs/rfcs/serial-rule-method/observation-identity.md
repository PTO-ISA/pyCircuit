# 观测身份与逻辑 owner 扩展合同

状态：2026-10-14 有界实现已验收。本文件承接已有观测组提交前验证，固定 Method
检查/观测扩展的身份前置条件；不开放 Method body 的新语句，也不证明完整 Method System 支持。
复用既有 `ac.observe`、依赖分析、source units 和共同 emission plan，不增加编译路线。

## 身份 hard break

每个最终 module-level observation 必须携带以下五字段 ObservationID，缺字段、多字段
或旧三字段形态均拒绝：

```text
{
  registration: Occurrence,
  site: Occurrence,
  ordinal: nonnegative signless i64,
  invocation: [{call: Occurrence, receiver: OwnerRef}, ...],
  owner: OwnerRef
}
```

`OwnerRef` 的唯一空形式是 `{instance_path=[]}`。普通 observation 保留其原
registration、site、ordinal，新增空 invocation 与空 owner。ordinal 不要求连续，
不能因为新的输出格式重编号。完整 ID 在一个 Module 中必须唯一，所有 kind 均适用。

registration 指向物理承载 Module 的外层注册；site 与 `ac.location` 指向原逻辑
声明 Module 的语句和 SourceOwner。空 invocation 只能配空 owner；非空链的最后
receiver 必须等于 owner。每帧 call 属于前一 receiver 对应 Module，receiver 只能
保持原实例或向下进入普通子实例。最终结构验证不替代 lowering 对原调用历史的认证。

首批 owner 只允许 self 或普通 Instance 后代。不允许向上、兄弟跳转、动态选择、
跨 Collection、primitive 或未解消 import 终端。完整 Occurrence 必须唯一解析到
实际 allocation，不能用实例名字、位宽或类型相同替代身份。

## 一份共同验证权威

| 责任 | 既有 owner |
| --- | --- |
| 闭合 ObservationID | `ACIRIdentityContracts` 的 `verifyObservationID` |
| observation 结构与本地重复身份 | `HardwareSourceChecks` 的共享结构 helper，供 op/Module verifier 调用 |
| 实际 owner、bindings、调用链与来源 | `HardwareSourceChecks` 的 `resolveObservationOwner` |
| 完整闭包及 ReportID 冲突 | `HardwareAnalysisDependencies` 的现有 `Engine` |
| native 验证前预算 | `HardwareSourceCheckPreflight` |
| 分组、物理/逻辑路径、输出 metadata | `HardwareEmitCommon` 的共同 observation plan |

每个最终 `print/log/report` 都必须经过同一结构与 owner/provenance 验证，不能只检查
report。Python 当前没有因本合同新增 print 语法。首批仍不允许 source callable body
通过伪造 final ID 绕过其尚未开放的效果准入。

Module 局部用已验证的 immutable attribute handle 比较完整 ID；report 另按
`(OwnerRef, name)` 拒绝重复。不得在每对比较中重新递归解析或序列化整个身份。

## 紧凑 ReportID 证明

ReportID 是实际逻辑 owner 与 authored name 的组合，物理承载位置不能改变它。
同一个 owner 的重复 report 即使位于 false 路径或未选择定义中仍拒绝；不同实际
实例同名合法。parent 中重定位到 child 的 report 必须与 child 自身的同名 report
发生冲突，不能等 emitter 或 Runtime 才首次发现。

现有依赖 Engine 按精确 definition、完整 bindings 和分析 mode 缓存成功结果，
并检查所有 Instance/Collection，包括无返回值实例。扩展其成功结果，只保留本模块
直接 report 的实际相对 Instance 路径与名字，不平铺后代报告。

对每条本地重定位路径 P，在每个非空前缀对应的 child 中，检查其直接 report 的
剩余路径与名字是否相同。child 的递归证明覆盖更深层关系；所有证明完成后才发布
成功缓存。因为 owner 严格向下，指向同一真实 owner 的两个物理承载实例必为祖先与
后代，这些前缀检查覆盖全部冲突。

Collection 的每个绑定只验证一次 callee 定义，适用于每个独立 lane，无需枚举 lane。
普通 observation 的 self owner 在 Collection 中保持合法；owner 后缀仍不得穿过
Collection。既有正 extent、溢出和 Method routing 限制不变。不得为这个证明扩展
采样 `WiringProof` 的实例路径或另建一份实例图。

## 整组排序与双目标输出

先为每个 observation 建立一个 group，再排序，再展开其 value slots。排序键为：

```text
(registration 首次遇到 rank, 保留/展开后的 ordinal, 完整 ID 首次遇到 rank)
```

完整 ID rank 是确定的扫描顺序，不是指针数值。每个排好序的 group 分配递增 siteKey，
组内 valueIndex 递增；不得先平铺所有值再按原 site/valueIndex 跨调用排序。
普通生产者现有注册顺序和递增 ordinal 保持其原行为。手写 IR 的 ordinal 若故意与
operation 顺序相反，则按新合同的 ordinal 排序。literal-only log 保留一个 sentinel
slot，继续输出空 values 数组。

共同 plan 保留两条路径：physical path 用于读取真实快照；logicalOwnerPath 为
physical path 加已解析的普通 Instance owner 后缀，用于公开 instance/report objectPath。
Collection 前缀坐标沿用已有 occurrence 表示，不改变物理载荷寻址。
当前 descriptor geometry 按 definition 保存；同一定义在不同 bindings 下若产生不同的
Collection lane 数，且 child subtree 含 observation，emit 必须拒绝。此防守检查不能
被解释为通用参数化观测布局已经实现。

两目标只调用同一个 `observationSiteMetadataJson` formatter：

```text
registration = 原 registration 的 canonical JSON 字符串
site = {site: 原 Occurrence 对象, invocation: 已验证数组, ordinal: 已验证值}
       的 canonical JSON 字符串
instance = 逻辑 owner 的 root/instance[lane] 路径
```

Runtime 的事件 envelope 和任意用户提供的 opaque site 字符串不改变。编译器生成
metadata 是一次 hard break，所有生产者、消费者和精确 JSON oracle 同时迁移，
不提供旧字段默认值或兼容 fallback。

## 预算与失败原子性

raw preflight 在 `mlir::verify` 之前独立计数 observation 数 N、operand 数 V 和
完整属性结构成本 J。J 包含 ID、`ac.location`、spec、kind 的全部嵌套内容，包括
名字、文本和 malformed 属性；非法嵌套 observation 也计入保守上界。计数不能调用
类型化 accessor、verifier 或 owner resolver，也不混入旧 source-check metadata 成本。

本地 key 是三个固定 handles：identity、owner、reportName。令其含记录记账成本为 K，
`T=N(N−1)/2`，则新增 native reserve 为：

```text
S = J + 2V + 8N
reserve = 2S + J + N*K + 6T + 2N
```

全部算术饱和，T 先除偶数因子再乘。该上界绑定于两次借用式结构验证、既有属性 scope
walk、固定 key 存储和常数 handle pairwise 比较；新增遍历/存储必须更新并独立复核。
计数本身仍即时计费。Engine 的路径、bindings、副本、查询及缓存继续通过原 debit
callback 计费。限额不增加，失败不修改输入 IR 或已有发布物。

## 独立验收 checklist

- [x] 旧 ID、缺/多字段、错误 ordinal、非规范 owner 和所有 kind 的伪造来源在 emit 前拒绝。
- [x] 完整 ID 重复与同 owner/name 重复分别拒绝；重复 Method 多值 log 不拆组、不交错。
- [x] parent/child、多层重定位冲突拒绝；不同实例同名合法，false/未选择声明不隐藏错误。
- [x] 嵌套 Collection lane 保持独立，规模增长不导致逐 lane ReportID 证明；跨 Collection owner 拒绝。
- [x] 完整 bindings 隔离缓存；长 metadata、多 observation、极小预算安全拒绝且输入不变。
- [x] 普通 registration/site/ordinal 与事件顺序保留；两端输出新 site JSON 完全一致。
- [x] 物理 caller 的载荷与 logical receiver 的 metadata 分离，source-unit 和发布保护保持。
- [x] 保留六个恒真、三个动态 Method、观测组提交前回归和适用短 gates。

本检查点有 21 项原生身份测试、公共观测的 C++ workers 1/2 与 Icarus 执行、
九个 Method 场景、五个生成 System 场景及短 API/examples gate 证据。缓存隔离通过
实际 bindings 的共同依赖 Engine 验证；参数未绑定的 catalog 并未因此扩展准入。
旧 ID/非法 owner 在 native/common verifier 拒绝，输出保护沿用既有 gate，尚无单独的
公共 emit 身份篡改输出保护用例。nightly 未运行。后续修改须重新绑定候选证据；
本检查点不声明普通无观测 Collection 的 emitter 路径分配具有紧凑规模界。完整 Method System
还需要 source effects、publication/state plan、最终事务布局/认证和生成 runner 执行。
