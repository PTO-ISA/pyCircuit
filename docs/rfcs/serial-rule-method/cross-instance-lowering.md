# 跨实例 Method 的共同 IR 接线规格

状态：基于 `078c6d5716fbd954828713e4c6d7376c12015bc7` 制定的扩展合同；
受限实现已在 `3078e75f6a354c6a947a7d31e1a7a930978d9a0a` 接入。
本文细化[事务降低](mlir-pipeline.md)，不改变[源语义](source-semantics.md)。
后续候选仍须重新验证本文的不变量；历史实现和文档不代替 fresh verification。
动态 guard 按[采样准入扩展](sampled-admission.md)单独验收。

## 目标与边界

必须通过现有 `ac-lower-transactions` 支持三层静态调用：
`Parent.Rule → Relay.Method → Counter.Method`。Method 的纯计算归入外层 Rule，
持久状态保留在原 provider 的原实例。作者继续使用现有 Module 句柄和 Method 调用，
不手写候选状态、内部使能或跨层次端口。

本切片限于同一实际 clock/reset 域、可证明为 true 的活动 guard/effect path、
whole-owner fixed Bits 状态和每物理 owner 唯一外层 writer。不支持的动态准入、
partial update、资源端口和跨域调用必须明确拒绝；后续扩展不得重新引入另一条 lowering。

复用现有 `SourceTransactionStateAnalysis` 的 anchors、frames、owners、bound values
和有序事件，以及现有 Method resolution、HardwareAnalysis 和 source-unit authority。
禁止移动状态到 caller、按调用克隆 Module、增加隐藏存储/周期、在 emitter 中解释 Method，
或按例子、常量、宽度决定准入。

## 物理接线与身份

通过现有 Module/Instance 的普通物理端口向上传递 old Q 和原模块的词法 SSA，
向下传递候选 D/E。中间模块只沿真实实例路径转接。每个状态仍生成一个原位置的 DFFE，
保留 clock、reset、initial、occurrence 和 SourceOwner。

```text
Parent 中唯一的外层 Rule
    ↑ Counter 的 Q、lexical capture      ↓ 最终 candidate D / E
Relay 的原 Instance 端口
    ↑                                  ↓
Counter 的原 Instance：DFFE(clk, reset, E, D, initial)
```

必须分别使用以下身份；名称仅用于诊断，不作为等同性证明：

| 对象 | 归一化身份 |
| --- | --- |
| owner | 词法 Module + 相对 Instance occurrence 链 + state declaration |
| 模块级普通值 | 词法 Module + 相对 Instance occurrence 链 + 原 SSA Value |
| 一次 Method 求值 | 外层 transaction + invocation frame + 绑定后的值 |
| writer | 自主 transaction occurrence，包括实际实例路径 |

同实例 alias 归一，同定义的两个实例不得合并。同 Method 两次调用有不同 frame，
但共享该外层事务的 owner 候选。保存的普通值不随后续写入更新。
不得继续用 declaration pointer 单独索引 writer 或最终 D，也不得仅用 `frame + Value`
索引来自不同模块实例的 lexical capture。

按 Module **定义**汇总各实际 occurrence 的接线需求，形成一个统一物理签名。
不能生成 caller 专用定义。原模块的 lexical SSA 必须从原实例导出，不能在 caller
重新执行其 initializer、结构实例化或计算；同名 caller 局部值不能替代它。
old-Q capture 始终为旧 Q，只有显式 `state.read` 查询事务候选。
Struct/Table 普通 capture 保持整体 typed payload，不按位或元素展开接口。

本切片的接线路径只包含普通静态 Instance。若待扩展签名的同一 Module 定义也被
Collection 引用，必须在物化前明确拒绝，即使该 Collection 不在实际 Method 调用路径上。
同样，若 transaction writer 摘要必须跨 Collection 组合，本切片拒绝该组合。
不得遗漏共享 callee 的列式 operands/results，也不得克隆 callee 来绕开限制。
未涉及签名扩展或事务 writer 组合的既有 Collection 保持原 admission、shape 与行为。
未来解除限制时，须另行验收 Table-column 完整接线和 occurrence/writer 组合，不能按坐标
全展开。本项限制不授予源码创建 Method owner collection 的新接口。

## 最终物理端口描述

新增 final-only 属性的闭合 schema 如下；这是编译器内部物理边界合同，不是源 API：

```mlir
ac.internal_ports = {
  source_inputs = 2 : i64,
  source_outputs = 0 : i64,
  root_inputs = [0 : i64, 0 : i64]
}
```

- `source_inputs` / `source_outputs` 为变换前物理签名的长度，包含原隐藏 clock/reset。
  值必须非负且不超过对应完整物理 arity。
- 原类型、名字、顺序保持为前缀；生成端口只追加到末尾，名字确定且不碰撞。
  至少增加一个输入或输出，否则拒绝该属性。
- `root_inputs` 精确覆盖输入后缀。此切片每项必须为 `i64 0`，对应端口必须是合法
  fixed Bits；表示按端口宽度构造的已知零。不是截断任意整数的许可。
- selected System occurrence 只对 source 前缀公开输入/输出；内部输出不进入公共 root ABI。
- 属性仅允许在最终包的真实 `ac.module` 上。source body/interface、单源发布物和
  `ac.module.import` 中均拒绝；不得从 provider 发布接口接受这项 final 权威。
- 普通 Instance 必须显式连接完整物理 arity。`root_inputs` 只约束选中的根 occurrence，
  不是普通实例省略参数时的 fallback。无外来写入的普通实例明确连接已知零 D/E。
- 无此属性的普通硬件保留原行为，全部端口都属于既有物理边界。
- 未知字段、错误 attribute 类型、负数、越界、错误 suffix 长度或非零默认值均失败关闭。

唯一共享读取/验证入口声明在 `HardwareAnalysis.h`，实现归 `ACIRModuleContracts.cpp`：

```cpp
struct ModulePortLayout {
  unsigned sourceInputCount;
  unsigned sourceOutputCount;
  mlir::ArrayAttr rootInputDefaults;
};
mlir::FailureOr<ModulePortLayout>
getModulePortLayout(mlir::Operation *declaration);
```

`verifyModuleSourceCallContract` 继续核对 `ac.parameters`、`ac.result_constraints`、
`ac.return_form` 与 `ac.domain_inputs`，但依据 source 前缀；clock/reset 原 ordinal 不变。
包括只有 domain 参数的形式也不得再假设 domain 位于完整签名最后两个位置。
完整物理类型与 Instance 接线仍由原硬件 verifier 检查。不得删除原 source metadata
以规避 arity 验证。两个 emitter 必须消费同一个已验证 layout，不各自解释属性。

### selected root 同时作为其他定义的 child

整个包可能有未实例化的 `Other`，其中的 `Root()` 被 `Other.Rule` 调用 Method，
而当前 System 直接选择 Root。必须保留一个 Root 定义及所有定义的验证：

1. Other 的 Root Instance 显式绑定自己的内部 D/E。
2. selected System Root occurrence 的内部输入按 layout 绑定已知零，外来 E=0。
3. Root 的原自主事务保持自己的语义；Other 未执行不能导致 selected Root 误写。
4. C++ 和 RTL 的公共根接口仍是 source 前缀；不能要求用户供应内部参数。

根绑定必须覆盖实际 generated root 的构造、Reset 后、每次 Work 和直接 System 使用。
仅在可选 `pyc_dut` facade 中初始化一次不足以满足合同。不得把根零绑定写入所有同定义
child 的 Work 而覆盖 caller 的显式连线。准确生成 owner 见下方实现责任。

C++ 继续复用既有 family/scalar wrapper 两层：完整物理实现 `cppFamilyClass` 保留全部
端口，普通 child Instance 使用 family；`cppClass` 是 scalar SimModule wrapper，
selected definition 的此类型被别名为 `pyc_root`。具体生成合同为：

- 以已验证 System entry 的 definition identity（既有 `rootBindings().owner`）判定
  selected root；`ac.root_kind` 只区分 system/module，不代表选中的根。
- `HardwareEmitCppDefinition.cpp` 中 selected scalar wrapper 只公开 source 前缀，
  用一个私有生成 helper 向其私有 family 绑定精确 Bits 类型的已知零。
- constructor 完成前绑定一次；每次 Work/Reset 先桥接 source inputs，再绑定内部默认值，
  最后调用 family Work/Reset。Xfer/DiscardNext 不重新采样或解释输入。
- family 本身不实施 root 默认绑定；同定义作为 child 时接受父模块真实 D/E。
- `FinalEmitCpp.cpp` 的 typed DUT Inputs/Outputs/drive/sample 和 system control 验证
  使用相同 source 前缀视图，不重复实现零绑定。RTL 在既有 root wrapper 接内部常量，
  真实 Module 保留完整物理签名。

## writer、因果与失败原子性

复用状态分析构建按定义缓存的相对 writer 摘要：本地 transaction 的 writes 归一化，
child 自主事务的摘要加上实际 instance 前缀，然后检查同 owner 的不同 writer。
同一外层事务重复写归为一个 writer；身份写、随后覆盖和无用结果仍保留写意图。

| 场景 | 本切片结果 |
| --- | --- |
| 两个 Rule 分别写 `left.count`、`right.count`，二者同定义 | 接纳，独立物理 owner |
| 父事务与 child 自主事务写同一 count | 拒绝，包括身份写 |
| child 自主事务读 count、写另一 owner | 接纳；读旧 Q，不读父事务 D |
| 同一父事务经 Relay 多次写同一 Counter | 接纳，按语句顺序传递候选 |
| 未选中的定义包含非法冲突或递归 | 仍拒绝；不能靠 root-reachable pruning 隐藏 |

内部 Q 输出本身不是时间边界。依赖分析必须沿真实图追到 DFFE Q 才切断；D、普通
capture、Method 结果仍保留组合依赖。死结果中的非法环不得被提前 DCE 隐藏。

同一 pass 必须先完整分析所有定义及预算，再在 detached clone 内依次完成：
签名与 Instance 重建、原位置 storage、路由映射、外层 Rule 候选计算、D/E 接线、
旧普通结果替换与 source-effect 删除。验证 layout、native IR、hardware dependencies、
checks 和无残留事务全部成功后才替换原包，并使旧 analyses 失效。
任一步失败均保持输入对象及内容、正式输出与已有阶段文件不变。

预算覆盖路径深度、需求并集、端口数、writer 摘要、Instance operands/results、
frame/value 映射和实际克隆量；溢出先于物化失败。普通无事务硬件继续原 fast path，
未参与接线的 Collection 不因本扩展额外展开或被施加事务预算。

## 实现责任与入口

| 责任 | 既有 owner / 允许新增文件 | 退出条件 |
| --- | --- | --- |
| 接线、writer、物化 | `LowerTransactions.cpp`；必要时私有 `TransactionRouting.h/.cpp` | 同一 pass，不增加第二状态分析或 evaluator |
| 共享 layout、类型验证 | `HardwareAnalysis.h`、`ACIRModuleContracts.cpp`、`ACIRModuleOps.cpp` | 原 source metadata 和完整物理类型均验证 |
| schema 与发布隔离 | `ACIROps.td`、`SourceUnitValidation.cpp` | final 属性无法进入 source/import authority |
| 根端口映射 | `FinalEmitCpp.cpp`、`HardwareEmitCppDefinition.cpp`、`FinalEmitVerilog.cpp` | 根与 child 分离，直接 root/System 与 facade 均满足默认绑定 |
| 独立 native 测试 | `tests/compiler/IR/TransactionLoweringTest.cpp` | owner/path/alias、错误布局、预算及晚期回滚 |
| 公共执行测试 | 既有 `serial-method-public` harness 及其 Inputs | 三层跨源真实执行、同 final 双目标、独立 oracle |

表内编译器文件相对 `compiler/lib/Transforms/`、`compiler/lib/Dialect/ACIR/`、
`compiler/lib/Compiler/` 或 `compiler/include/pycircuit/Dialect/ACIR/` 的既有 owner。
PM 派发时必须固定准确路径及排他编辑权；公共 schema/header/CMake 由 PM 整合。
新增 relay fixture 精确路径须随测试任务冻结，不因预留名而声称文件或覆盖已存在。

## 独立验收 checklist {#cross-instance-checklist}

- [ ] `METHOD-CROSS-INSTANCE-CLOSURE`：三层三源独立发布；consumer 编译仅能访问接口，
  link 使用显式完整 body 闭包；真正执行 `Rule → Method → Method`。
- [ ] `METHOD-CROSS-INSTANCE-VALUES`：两个同定义实例、同实例 alias、重复调用、跨 frame
  保存值、lexical SSA/old-Q captures、同名 caller 局部值与整体聚合传递均正确。
- [ ] `METHOD-CROSS-INSTANCE-WRITERS`：上表全部正反例，含身份写和未使用定义。
- [ ] `METHOD-INTERNAL-PORTS`：schema、source 伪造、完整实例 arity、类型、名字、顺序、
  原 domain ordinal 和 source grouping；无外来 writer 的实例 E=0 不误更新。
  同一待扩展 callee 被 Collection 引用、或需跨 Collection 组合事务 writer 时有明确
  的物化前拒绝；无关 Collection 的原 admission 和执行回归保持。
- [ ] `METHOD-ROOT-INTERNAL-BINDING`：selected Root 同时被未实例化 Other 引用；公共
  ABI 不变；构造/Reset/连续 Work、直接 root/System 与 typed DUT 均正确绑定。
  单独选择 Other 为根时，同一 Root definition 的 child family 必须接受父方非零 D/E；
  selected-root wrapper 的默认绑定不能覆盖 child 连线。
- [ ] `METHOD-ROUTING-ATOMICITY`：路径/端口/展开预算和物化晚期失败保持原 IR 与输出；
  真实组合环拒绝，不能将内部输出伪装成时序 cut。
- [ ] 结构 oracle：Module、Instance、持久 storage 数量及归属不变，无额外周期。
- [ ] 数值/时序 oracle：C++ workers 1/2 和真实四态 RTL；复位、上升/保持/下降、Discard，
  X/Z 数据通过路由；数据未知不等同于未知 guard。
- [ ] 使用同一公共 final 的两个 emit、同一次 link 的真实 before/after artifact，
  现有 `@system` 回归；命令/退出状态/哈希绑定候选。

以上检查独立于完整 Method 基线；通过后仍不得勾选动态 guard、部分更新、Queue 或
完整 Method `@system` 集成。后续依赖见[扩展实施与验收矩阵](extension-roadmap.md)。
