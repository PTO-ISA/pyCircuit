# C2-SYSTEM：一等 system 定义与 design/testbench artifact role

修订：B，2026-09-29。状态：用户已选择方案 2 的结构方向；本页的精确
op/header/role/CLI/manifest 增补仍待独立审阅和用户批准。不得将方向选择
记录成下表所有字段已批准。本页不修改实现，不改变冻结 M1-C/C3-C 正文或哈希。

## 1. 目标与两个正交维度

- `ac.module`：可复用硬件模块定义。
- `ac.system`：一等系统结构定义，拥有 symbol 和 body，表达选定系统的
  reg、rule、子模块实例与连接；不再是无 body 的 root descriptor。
- `design|testbench`：一次链接产物的声明用途。系统组合本身不等于 testbench，
  普通 module 也不能仅凭 op 种类证明“里面没有测试逻辑”。

Python 继续普通函数 + 嵌套 rule；不新增装饰器参数、self、Queue/Interface DSL。
调用方在工具层显式声明产物用途，MLIR 保存并验证闭合枚举及传播一致性。

表示示意（不是新增自定义打印语法）：

```text
source: ac.system @Top { source-owned body }
header: ac.system.import @Top { contract/owner/snapshot }
final package: ac.entry=SpecKey(@Top), ac.artifact_role="design"
               source-owned unit containing ac.system @Top { final body }
```

**role 是明确声明的用途，不是编译器证明“没有 stimulus”的证书。**
不能从文件名、symbol 名、assert/log/report、变量 phase 或有没有端口推断用途。
若用户把一段包含 stimulus 的设计自行标成 design，本提案没有自动识别算法；
相应依赖边界仍由项目源码组织、产物选择及独立测试证明。不得宣称仅加 role
即可检查任意源程序的 design/testbench 内容纯度。

## 2. 现状与准确变更清单

现状：source `@system` 是带 root_kind 的 ac.module；header 是同样带标记的
ac.module.import；final 又生成零 region 的 ac.system(entry,domain)。
新私有工具进一步把 system root 强制等同 testbench，这种等同不进入本合同。

| 对象 | before | after（待批准） |
| --- | --- | --- |
| ac.system | 零 region 的入口描述，entry/domain | 带 Symbol/IsolatedFromAbove 的系统定义，sym_name/domain，一个 body region |
| ac.system.import | 无 | 新增独立 header declaration op，sym_name/domain，无 body/operand/result |
| ac.root_kind | source/header 的字符串身份标记，final 可残留 | 新路线各阶段禁止，以 system op/import op 的种类表达 |
| builtin linked/final package ac.entry | SpecKey，指向 module root | 保持同一 SpecKey 形状，允许解析到 module 或 system 定义 |
| builtin linked/final package ac.artifact_role | 无 | 新增必需 StringAttr：design 或 testbench |
| public link --role | C3-C 无该参数 | 新增必需 --role design\|testbench；不是从source种类推断 |
| public emit --role | C3-C 无该参数 | 不新增；从已验证 final package 读取用途 |
| generated.json.artifact_role | 无 | 新增必需同名字符串，等于 final package 中的用途 |
| generated.json.files[].role | header/source/cmake/rtl/runtime-glue/source-map | 完全不变，不混入 design/testbench |

这是一个新 op（ac.system.import）、一个已有 op 的结构重定义、一个 IR 属性、
一个 public link 参数、一个 manifest 字段的精确提案，不得称作“没有接口变化”。
不新增 ac.design/ac.testbench op，不改变任何 reg/rule/proposal 的时序或 type。
ac.specialization 的定义内容从 concrete module 扩展为 concrete module 或 system；
SpecKey、StateID、Occurrence 不增加 role 字段，role 不改变硬件身份。

选择独立 import op 是为了使逐源 header 明确表达系统定义种类，与
ac.module/ac.module.import 的对应方式一致；不让空 body、任意属性或文件名
再暗示“其实这是 system”。它需要本次单独批准，不由方案 2 自动授权。

## 3. 精确 system/header 形状

```text
ac.system
  traits: Symbol, IsolatedFromAbove
  operands/results: none
  core attributes: sym_name:StringAttr, domain:StringAttr
  regions: exactly one isolated body, exactly one block in an implementation

ac.system.import
  traits: Symbol
  operands/results/regions: none
  core attributes: sym_name:StringAttr, domain:StringAttr
```

两者必需 domain="default"，沿用 M1 的单域首片。body argument 0/1 是 i1
clock/reset，与模块现有控制前缀相同。本首片 system 无数据形式端口；普通
module 的数据端口仍可在父结构中绑定。作为 root 的普通 module 仍须 portless。
多域、外部 typed DUT、系统嵌套和未闭合 static profile 不借此开放。

必需沿用的 source identity/header metadata：ac.source_owner、ac.origin、
ac.declaration_role，以及相应阶段的现有 ac.contract/control_ports/ports、
static/snapshot/effect 记录。除表中变更外不增加新的 identity 或调度描述。
impl 的 ac.declaration_role=definition。**owning `.interface.ac` 中的
ac.system.import 也使用 ac.declaration_role=definition**，是该系统声明的唯一
header authority；只有其他单元消费/转存的副本使用 import_snapshot。副本保留
原 SourceOwner/origin，必须解析并匹配显式提供的 owning header，不能成为新 authority。
未知字段及不支持 profile 继续按闭合合同拒绝。

N1 的 hardware-definition canonical target 类别扩展为 owning ac.module.import
或 ac.system.import；ExportBinding/ImportBindingUse 的记录字段不变，类别从
已验证声明取得，不增 kind 字段。声明 op 种类纳入 snapshot 比较，同 symbol 的
module/system 替换必须失配。这一类别扩展不允许 system 成为 instance callee。

system body 使用与 module 相同的合法 reg/rule/instance/值计算/检查/观察和
ac.yield 规则；source/final yield 的形式各自复用对应阶段模块合同。
body 内的规则读 Q、产 proposal；system op 本身不执行宿主脚本、不直接提交两次。

本地函数名沿用既有 SourceOwner/symbol 来源规则，不另加与 sym_name 重复的
name 属性。ac.instance/ac.yield 和规则/检查/观察的 enclosing owner 校验需接受
module 或 system，但实例 callee 仍只能为 module。

实现应提取共用的 ModuleLike 查询/验证辅助，不复制一套 system typechecker、
lowering 或 backend。该内部接口不是新 Python/IR primitive。

## 4. 分阶段表示与链接

| 阶段 | 表示及检查 |
| --- | --- |
| capture | @module/@system 的真实 AST 种类保留，Python 不自行推导硬件调度 |
| source body | @module -> ac.module；@system -> ac.system body；两者都 source-owned |
| source header | module -> ac.module.import；system -> ac.system.import；消费方只读 header |
| link | 校验 body/header 的 op 种类、owner、SpecKey、snapshot/effects，构建一棵实例树，固定 ac.entry 与 ac.artifact_role |
| final | 保留一等 system 定义及有用的 module 定义；不再额外插入零-body system 描述符；final verifier 统一检查 module-like 结构 |
| reparse/emit | 从 final module/system entry 重建同一 verified closure；用途缺失/非法拒绝，backend 不自行猜或重写用途 |

source/final envelope 及 declaration 放置规则：

| 容器 | 必需沿用/新增的 envelope | 允许的 system 声明及角色 |
| --- | --- | --- |
| source implementation unit | C2/N1 source unit_kind、stage、source_owner、exports/import_bindings 与既有接口记录 | 本源 system body=definition；依赖声明副本=import_snapshot，不能建立 authority |
| owning source interface unit | C2/N1 interface unit_kind、source stage/source_owner、exports/import_bindings | owning ac.system.import=definition；消费其他源的副本才是import_snapshot |
| 最外层 linked package | 既有 entry/链接闭包；新增 ac.artifact_role | 只在此 envelope 保存本次链接用途；不通过source/header预设用途 |
| 最外层 final package | ac.stage=final、ac.entry、ac.instance_bindings；新增ac.artifact_role | 唯一role权威与entry解析；不再有零region selector |
| final source-owned implementation unit | 既有final unit_kind/stage/source_owner envelope | ac.system/ac.module具体definition；禁止ac.system.import、ac.module.import及import_snapshot残留 |

ac.artifact_role **只能存在于最外层 linked/final package**。在 source/header、
嵌套 implementation unit、system/module definition 上设置同名属性一律拒绝，
不能补足外层缺失值，也不能覆盖外层值。linked semantic 是已有内部阶段概念，
本增补不新增可绕过验证的 stage 字符串或序列化兼容模式。

首片沿用 system root-only 限制：ac.instance 只能指向 module，不能把 system
当普通 child；import 声明不放宽这个限制。因而 testbench 可实例化共享的 DUT
modules，但本包不承诺“另一个 design system 本身”可直接作为 DUT child。
若需要嵌套系统或外部 typed DUT，应另列精确提案。

普通 portless module 为 root 时：ac.entry 直接引用该 module，不为了统一
形状创建假 ac.system wrapper，不分配额外 reg，不复制 root invocation。
其 GFSIM system glue 由共同实例树生成，与 root 采用哪种定义 op 无关。

## 5. role 的来源、传播及边界

source body/header 不记录某一次编译产物的 role。它们是可复用定义；同一个
module 可以独立交付为 design，也可以出现在 testbench 的 child closure。

link 必须显式提供 role，验证枚举后写入 package ac.artifact_role。两种 root
定义均可用于两种用途；**删除“system只能testbench/module只能design”的私有规则**。
这改变的是用途表达与接口接纳规则，需要本提案批准，不是纯命名重排。

emit 以 final package 为 role 来源，写入 generated.json.artifact_role；不能用
另一个 CLI 值覆盖它。若私有测试 helper 暂时保留 --role 作为一致性断言，只能
比较相等，不得覆盖 IR。公开 emit 不引入重复的 --role 参数。

验证的是字段合法性、单一入口和 manifest/IR 一致性；合法编辑 IR 的用途声明
不是密码学伪造，也不会被误称为编译器已验证源码意图。不得通过 role 改写
状态、错误提交、reset、算术宽度或 source checks。

## 6. 产物与两后端

`.ac` 文件随 root 所属 Python 源文件命名：design_top.py -> design_top.ac；
test_design.py -> test_design.ac。文件名不编码第二份 role 权威。

生成文件仍按源 SourceOwner 分组；普通 design 与 testbench 分别选择各自
闭包生成产物，不用同一个带刺激的 top 重贴标签声称获得独立 DUT。

- C++：复用同一 module-like generator/GFSIM runtime；Work 只 rule 求值，
  Xfer 提交一次。testbench 可以由另一个源定义的 system 组合 DUT modules，
  runtime/executor 不硬编码模型名或期望值。
- RTL：硬件定义在 rtl 角色文件，仿真 observation wrapper 在 runtime-glue。
  testbench 可以生成用于仿真的结构，但不得被宣称为独立 DUT RTL。
- 两者均从同一个带 role 的 verified final closure 生成；role 是产物用途，
  不是不同硬件解释模式。design 中合法 assert/range checks 不能被删掉。
- link 仍不调用 emitter；file-role 拆分与 artifact-role 是不同维度。

unit.json、journal、PublicationOwner、runtime config/ABI、SDK schema 不新增
role 字段。artifact_role 是产物配置，不是新的 source/发布 owner；同 owner
的 --replace 可更新角色，但必须重建完整产物并遵守现有事务/foreign-owner
规则，不能原地改 receipt 掩盖旧 IR。

role 一致性验证分两层，不能混用：

- **本次 emit 新 staging**：将拟发布 generated.json.artifact_role 与本次已验证
  final IR 的 ac.artifact_role 比较，失配时在发布前拒绝并保留旧产物。
- **既有产物读取/恢复**：只校验其自身的闭合 schema、合法 role 枚举、owner
  和文件清单；没有原 final IR 时，不承诺识别合法 design/testbench 值被手改。
  有原 final IR 的显式配对验证可以报告二者不一致。

共享 publication validator 会被用于旧产物和新 staging，因此不能把它绑定
为“所有待验证产物都必须等于本次新 role”。新 staging 的 IR 配对验证由 emit
构建阶段完成；公共旧产物/恢复 validator 保持自包含。这样同 owner 从 design
到 testbench 的合法 replacement 不会在校验旧产物时被错误拒绝。

manifest 新字段只在新合同切换后生效；旧清单/旧 final package 缺失新字段
时明确拒绝，不默认为 design，不通过旧代码路径补齐。历史证据文件保持原样。

## 7. 错误与删除范围

以下输入必须在发布前失败：body/header 定义种类不匹配、仅snapshot而无owning header、module/system
同symbol种类替换、重复/未解析 entry、
system child/递归、伪造 owner、clock/reset 控制不合法、重复物理状态、未知/缺失
artifact_role、非法 link role、新 emit staging 或显式配对验证时 manifest 与
IR role 不一致、source/linked/final
残留 root_kind、旧零-region ac.system 描述。

错误继续使用既有 C3 usage/compile/validation 退出与诊断机制，不新增 runtime
错误码。私有入口回归需断言具体 verifier 原因以及既有输出不变，不能任意非零算通过。

替换清单：PythonImportModules/ModuleBody 的 marker 写入；source header kind；
ModuleGraph root/child 判定；所有 getParentOfType<ac::ModuleOp> 假设；source/final
verifiers；FinalHardware 的 descriptor 构造；FinalHardwareProgram 重建；两 emitters
及 source-group/manifest/driver；相关 snapshot、测试和当前文档。逐源边界不变。
不得保留 root_kind、新system、旧descriptor 三条接纳路线的兼容 fallback。

## 8. 有界实施与独立验证

阶段一：批准后先修改 ODS/header/common module-like verification 与 source
capture 映射，锁定旧 schema 拒绝和 header-only 编译；不额外增加硬件能力。
阶段二：ModuleGraph/final materialization/reparse/两 emitter 一次贯通，source
identity、寄存器个数和逐拍 oracle 不变。
阶段三：role 的 link/IR/manifest 连通及两份源闭包正例；接入已批准范围的
M4 使用流程。完整 SDK、并行、typed 外部 DUT 均不属于本包验收。

必要验证：module与system body/header roundtrip；header mismatch拒绝；
module top 不产生假wrapper/reg；system top 每实例一次Work/Xfer；旧root_kind/
零region system拒绝；design和testbench用途传播；相同硬件root的role改变不改变
周期/算术/提交结果；manifest非法字段或提供原IR时配对失配拒绝；
无source文件时header消费/link仍成立；
V41/V42原逐拍值/4与5物理reg、失败不提交、reset重跑同一候选双后端通过。

另需验证：role 非法落点不能覆盖/补足外层；仅snapshot不能提供authority；
显式owning header缺失拒绝；N1绑定中的module/system种类替换拒绝；同owner
跨role replacement正常成功；staged manifest/IR角色失配拒绝且旧产物不变；
独立旧bundle恢复不依赖本次新role，也不声称能检测无原IR时合法枚举的手改。

role 测试必须验证不从名字/观察/根定义种类推断；也必须明确它不能证明任意
源代码没有stimulus。被打上design标签的closed-system不能自动成为独立DUT证据。

## 9. 批准边界

用户已选择“一等 ac.system 定义”方向。需要精确批准的是 §2–7 中明确列出的
新 import op、system schema、artifact_role、public link --role、manifest字段及
旧形式退役规则。独立review可以判断approval-ready，不替用户批准。
本提案不增加frontend decorator DSL、外部typed DUT、system nesting、调度/时序
模式或消费者设计；任何这类需求另行提出，不因本页泛称“system”而自动开放。
