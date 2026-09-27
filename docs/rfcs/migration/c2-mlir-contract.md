# C2：逐源 MLIR 与共同硬件 IR 合同

修订：A。状态：架构 author 已提出具体设计，待独立审阅与用户批准。依据[已批准的 C1-C](approvals/c1-pythonic-source.md)，本包不重新改变其源语义。C3 driver/发布/runtime 接口单列。

本设计由独立架构 author `interface_design`（Astra xhigh）补齐，PM 整理。目标是一个 MLIR 语义链，不能把 Python compiler、QueueGraph 或 backend interpreter 留作替代路线。实现可以分阶段，本文已定义的能力不能因 M2 尚未实现就永久退役。

## 单元、名称与声明权威

一个 source invocation 发布一个目录，含 `<stem>.ac`、`<stem>.interface.ac`、`<stem>.d`。两个 AC 文件的顶层都是 builtin `module`，必须有：

| 属性 | 精确类型和值 |
| --- | --- |
| `ac.source_owner` | `SourceOwner`，定义见下文 |
| `ac.unit_kind` | StringAttr：`implementation`、`interface` 或 `declarations` |
| `ac.stage` | StringAttr：此阶段为 `source`，只用于诊断，不是验证证书 |
| `ac.interfaces` | 有序 `Array<SourceOwner>`，先本单元 owner，再按结构排序的实际依赖，禁止重复 |

interface 文件为 `interface`；有一个 public module 的 body 为 `implementation`；只有类型、常量和值 helper 的源为 `declarations`。后者的 body 可以只有单元/依赖信息，不需要伪造 module/root。parent 和 selected root 分别编译，不能整系统捕获后拆分。

`SourceOwner={package:StringAttr,path:StringAttr}`。package 是配置的 dotted source-root prefix，可为空；path 是该 root 内规范化 POSIX 相对 `.py` 路径。禁止绝对路径、空/点/父目录组件、反斜杠、NUL、逃逸 root 的 symlink，以及大小写折叠后歧义的 ownership。definition 为 prefix + import-module 路径 + source 名字，如 `@"demo.bank.Bank"`；import alias 不改身份。`foo.py` 与 `foo/__init__.py` 解析成同一模块时拒绝。

所有 exported declaration 必须带 `sym_name`、`ac.source_owner`、`ac.origin:Occurrence`、MLIR `loc`、`ac.declaration_role`。role 为 `definition` 或 `import_snapshot`；snapshot 保留原 owner，不能成为新 owner。

owning interface 是 record、alias、constant、value-helper 和 module signature 的唯一 exported declaration authority；body 是 executable module 的唯一实现 authority。implementation 不再定义与自有 module 同名的 import symbol。nominal/value declaration 的必要副本标为 snapshot；link 先解析一个 owner/header authority、比较和去重，再合成普通 MLIR symbol table，不能把完全相同的副本当作多个 owner。

无内容派生的模型身份、随机 generation ID 或兼容版本分支；schema 由所选 toolchain 合同决定。source/release 版本只在外部构建/发布记录中。

## 闭合记录

除明确可选的字段外，未知/缺失字段拒绝。以下 `u32/u64` 表示非负且在相应范围的 IntegerAttr；SourceSpan 的行列从 1 开始，ordinal/index 从 0 开始。

```text
MathInt = #ac.math_int<canonical signed decimal>
SourceOwner = {package:StringAttr,path:StringAttr}
SourceSpan = {path:StringAttr,line:u64,column:u64,end_line:u64,end_column:u64}
PathComponent = {kind="field",name:StringAttr} | {kind="index",value:u64}
Site = {definition:FlatSymbolRefAttr,ast_path:Array<PathComponent>}
Occurrence = {site:Site,expansion:Array<ExpansionFrame>}
ExpansionFrame = {kind="call",site:Site,callee:FlatSymbolRefAttr}
               | {kind="iteration",site:Site,ordinal:u64,value:StaticValue}
```

MathInt 是任意精度数学整数属性，和 BoolAttr 不同；相等性按数学值，不按文本或宿主位宽。ast_path 按 source AST 字段/列表结构定位；空路径代表 declaration 本身。frames 保持外到内展开次序；来源位置不建立语义身份或调度优先级。

SourceSpan 采用一基 Unicode codepoint 列，end exclusive。CPython AST 给出 UTF-8 byte offset：按该源码行的 UTF-8 byte prefix 严格解码后计数并加一；越界或切断 codepoint 拒绝，不把 byte/display columns 当作同一件事。

```text
LogicalType = {kind="bool",storage=TypeAttr(i1)}
            | {kind="integer",storage=TypeAttr(iN),lower:MathInt,upper:MathInt,
               interpretation="unsigned"|"signed"}
            | {kind="record",symbol:FlatSymbolRefAttr}
            | {kind="list",element:Bool|Integer|Record,length:u64}
StaticType = {kind="bool"} | {kind="integer"}
           | {kind="integer",lower:MathInt,upper:MathInt}
           | {kind="record",symbol:FlatSymbolRefAttr}
           | {kind="list",element:StaticType,length:u64}
StaticValue = {kind="bool",value:BoolAttr} | {kind="integer",value:MathInt}
            | {kind="record",symbol:FlatSymbolRefAttr,fields:Array<StaticValue>}
            | {kind="list",values:Array<StaticValue>}
Default = {present=false} | {present=true,value:StaticValue}
```

整数 lower inclusive、upper exclusive；N 为 C1 定义的最小存储宽度，初始执行 profile 1..64。list 长度为正；record field/value 按声明顺序、arity/type 精确，禁止 null 或缺字段。bounds 成对出现；bool true 不等于 integer 1。LogicalType 是声明合同，不能单凭 metadata 就证明运行值符合范围。

## Header 与 helper

`ac.module.import` 无 operands/results/regions，属性为上述 declaration 公共字段，以及：

```text
ac.contract = {parameters:Array<Parameter>,connections:Array<Connection>}
Parameter = {name:StringAttr,
             binding="positional_only"|"positional_or_keyword"|"keyword_only",
             category="static"|"connection",type:StaticType|LogicalType,
             default:Default,origin:Occurrence,location:SourceSpan}
Connection = {parameter:StringAttr,elements:Array<ElementEffect>}
ElementEffect = {ordinal:UnitAttr|u64,read:BoolAttr,write:BoolAttr,
                 precision="exact"|"conservative",origins:Array<Occurrence>}
```

parameters 是 constructor 原顺序；connections 只列 connection 参数，按同一顺序；scalar/record 恰有一个 UnitAttr ordinal，list 恰有 length 个递增 ordinal。state field projection 读视作读整个 record，写遵守完整 record next。unused element 可暂有 R=false/W=false；完整接纳由 specialization 后 exact effects 及 C1 规则判定。connection default 不分配隐含 state，不能用数据 literal default 替代必需 reference。

保留 donor `ac.struct`、`!ac.struct<"qualified.name">`、pure `ac.struct.create/get/with`。source/header 的 `ac.struct.fields` 是有序 `{name,type:LogicalType,origin,location}`，增加 `constructor:FlatSymbolRefAttr`。layout 从声明取得，不由类型名猜测。

新增无 region/operand/result 的 `ac.type_alias`（必需 `target:LogicalType`）与 `ac.constant`（必需 `type:StaticType,value:StaticValue`）。二者使用 declaration 公共字段；alias 透明，record 保持 nominal。re-export 不改 owner。

record constructor 使用 source-owned `func.func`，例如 `@"demo.packet.Request.__init__"`，属性：

```text
ac.helper_kind="record_constructor"
ac.record=@demo.packet.Request
ac.parameters:Array<ValueParameter>
ac.result_type:LogicalType.Record
ac.check_templates:Array<CheckTemplate>
ValueParameter={name,binding,type:LogicalType,default:Default,origin,location}
```

value-helper signature 为 `(data storage types..., evaluation_path:i1) -> (data result,valid:i1)`；constructor result 为 record。hidden path/valid 是 compiler controls，不是源字段。helper 的参数/结果属性带 LogicalType；绑定 positional/keyword/default 后，实参按 Python source order 求值、按声明顺序传入。

helper 可以有经过验证的 arith、scf.if、source-math、record 操作、verified helper calls 和 check templates。它是 **state-effect-free，不等于可随意删除或提前执行**。禁止 state/resource handle、current/next、topology、driver ac.assert、logging/runtime call/递归；验证完整调用 DAG。defaults/reset 用同一个 MLIR 常量 evaluator 求值，失败在编译时报出。

constructor 字段初始化形成 source-order SSA，最终 ac.struct.create 按字段声明顺序；成功返回必须完整。headers 可以携带这些必要的值定义，但不能含 child module、rule 或 state 实现。link 比较 helper body 时允许 SSA alpha-renaming，保留 operation/order/operands/types/domains/defaults/check templates；忽略诊断位置，不能用未证明的语义等价替代比较。

## 静态表达式、结构控制与特化

`#ac.static_expr` 是唯一 MLIR 编译期表达式树，每节点包含 origin 和 location，没有 runtime SSA、Python callback 或源代码字符串：

```text
integer(value:MathInt) | boolean(value:BoolAttr) | parameter(name:StringAttr)
unary(operator,operand:StaticExpr)
binary(operator,lhs:StaticExpr,rhs:StaticExpr)
select(condition:StaticExpr,yes:StaticExpr,no:StaticExpr)
```

unary 集合：neg/invert/not/to_int。binary 集合：add/sub/mul/floordiv/mod/and_bits/or_bits/xor_bits/shl/shr/eq/ne/lt/le/gt/ge/and_bool/or_bool。operand/result kinds、arity 精确；Boolean/select 短路；求值路径上的零除数/负 shift 拒绝。所有整数操作用数学 APInt 语义，不能经 i64/index 提前截断。

- `ac.instance` 带必需 `ac.static_args:Array<StaticExpr>`，按 static 参数声明顺序。
- `ac.static.eval` 无 operands，属性 expression，一个 index result；用于结构 cardinality，先证明正数/可实现容量再转换。
- source-stage `ac.initial_value` 的 integer/bool leaf 可为 StaticExpr，record initializer 递归；特化后变为 payload-typed donor initializer。final 阶段拒绝 StaticExpr。
- `ac.static.if` 有 condition:StaticExpr、then/else 单 block region，以 ac.yield 终结；可携带局部 SSA/defined/valid 的 join，不产生运行时 branch。
- `ac.static.for` 有 iterable（range 的 start/stop/step StaticExpr 或有序有限 StaticExpr 序列）、induction_name、scope=statement|comprehension、origin；其 body/operands/results 按相同顺序携带局部 slot `(binding:!ac.local,defined:i1,valid:i1)`；当源 kind 已唯一时可用 typed slot 作为内部优化。

每次展开先给 loop target 赋数学 induction value，再执行 body 并 yield 所有 locals；statement-loop 最后一次 body 对 loop target 的重绑定在 loop 后可见。零次返回先前 binding，原本不存在则 defined=false，不能造零；后续可达读必须证明 defined，否则编译失败。comprehension 绑定不泄露。已接纳的 return/break/continue 保留显式控制状态，不能执行其后的禁用 body。静态 bounds 不依赖 runtime SSA，body 可以处理 runtime current；资源限制报明确 compile-limit，不能截断 iteration count。

source body 每 source 一个 generic ac.module。link 以 `(qualified definition, ordered tagged StaticValue tuple)` 索引特化，不依赖调用顺序、名字后缀或人工 finite-case 列表。`ac.specialization` 为 non-symbol 的 SymbolTable container，属性 definition、arguments、ac.source_owner；一个 region 内一个保留原 qualified name 的 concrete ac.module。instance 带 callee 与 bound arguments，由 composite-key resolver 验证解析。相同 key 复用代码，不复用实例状态；所有变体仍属于原 source group。

### 静态选择后的局部类型

新增临时 `!ac.local`，仅承载由 static control 决定 kind 的 source SSA binding，没有 runtime tag/union/storage/ABI。可包含 source math_int、source bool 或已接纳 nominal/value type，禁止 compiler control、state/resource handle 和 pointer。

```text
ac.local.pack(value:T) -> !ac.local
ac.local.unbound() -> !ac.local
ac.local.require(binding,path:i1,defined:i1,valid:i1) -> (T,i1)
  expected: LocalKind, origin:Occurrence
ac.local.to_int(binding,path:i1,defined:i1,valid:i1) -> (math_int,i1)
ac.local.field(binding,path:i1,defined:i1,valid:i1) -> (local,i1)
  field:StringAttr, origin:Occurrence
LocalKind={kind="integer"}|{kind="bool"}|{kind="record",symbol:FlatSymbolRefAttr}
```

全部 local ops 带 origin/loc。require 是类型义务，不是转换；to_int 只实现源显式 `int(...)`，bool→0/1，integer恒等，其他kind拒绝；field 在静态选择后按 nominal layout 解析并保留逻辑domain。

`if static_flag: x=True; else: x=3; y=int(x)` 的 generic branch 可以返回不同 packed kind；specialize 后 to_int 变为 from_bool 或 integer identity，不产生 runtime union。loop每次可有不同静态kind，最后binding遵守 C1/Python locals规则。

carrier若遇到存活runtime control，把使用下推到已有分支中逐臂校验/求值，再合并普通typed结果；不能引入runtime type tag或隐式bool算术。可达非法kind仍编译失败。reachable unbound不能用valid=false掩盖。unit verifier记录deferred type义务，specialization全部关闭；final hardware中任何 local type/op均拒绝。

## 数学值到有界硬件

新增临时 `!ac.math_int`，只允许 source/linked semantic 阶段，无运行时表示或 ABI。state/module/record 的有限存储形式沿用 donor；math values 只在 rule/helper 计算中出现。

| Op | operands → results；必需属性 |
| --- | --- |
| ac.math.constant | () → math；value:MathInt |
| ac.math.static | () → math；expression:StaticExpr，必须求得整数 |
| ac.math.from_bits | iN → math；domain:LogicalType.Integer，源 domain 必须由 state/interface/check 证明 |
| ac.math.from_bool | source bool i1 → math；精确 0/1，不能误用 compiler control |
| ac.math.unary | path,value,valid → math,valid；operator=neg/invert |
| ac.math.binary | path,lhs,lhs_valid,rhs,rhs_valid → math,valid；上述 C1 数学/位移二元算子 |
| ac.math.compare | 与 binary 同输入 → bool,valid；predicate=eq/ne/lt/le/gt/ge |
| ac.math.to_bits | path,value,valid → iN,valid；domain:LogicalType.Integer |
| ac.math.to_index | path,value,valid → index,valid；extent:StaticExpr，必须为正 |

所有 op 带 origin/loc；可能失败者带 check templates，不标成可投机 Pure。valid 等于 demanded path、operand validity 和 local safety 的合取；inactive/invalid 只产生不可观察 placeholder。signed i1 的位 1 提升为 -1，source bool true 经 from_bool 提升为 +1。

特化/静态展开/值 helper inlining 后，才重算 APInt 区间并选硬件位宽。常量区间 `[c,c+1)`；join 取保守包络，只用可独立证明的 dominating predicates/checks 细化。运算前扩展，to_bits 先范围检查再缩窄；runtime 值及实际 runtime 中间量最终须在 1..64 profile 内。静态计算没有该上限。

危险操作只在 `scf.if(path && demanded_valid && safety)` 内执行；错误路径为 `path && demanded_valid && !safety`，不能换成 rule fire/next enable。保留 source short-circuit、Python floor correction、大右移 sign fill、先 bounds 后 index conversion。final legalization 以显式 unsigned width 消除 index。

必须分别验证：完整 u64+1 未证明 i65 则 capability rejection；已证明 x<=MAX-1 可 i64；显式 `(x+1)&((1<<64)-1)` 可用低位等价证明；赋给 u64 本身不批准截断。mask 不得移除 demanded 子树的除零/index 错误。

参数参与 runtime 算术经 math.static 插入，不再以“未设计”搁置；实现 coverage 可分阶段，但完整 C1 验收必须关闭它。

## 错误、值与数值证明义务

helper 中 ac.expect 必须有 evaluation_path，携带 `ac.check_template={leaf:Site,kind,obligation:u64,location}`；kind 为 assert/division/shift/index/range。函数的 ac.check_templates 恰好枚举这些模板。inline 后追加 call/iteration frame，形成结构化 `CheckID={registration:Occurrence,check:Occurrence,obligation:u64}`，作为 ac.check_id 的闭合 DictionaryAttr。registration 是注册调用 occurrence，check 是展开后的检查 occurrence；obligation 是该 occurrence 优化前固定义务 slot，在所有 check kinds 间统一分配，而不是每 kind 从零编号。合并 index bounds 是一个 slot；调用边界 range checks 使用参数声明 ordinal。相等与排序按结构，不按打印文本。

owning rule 的 `ac.required_checks:Array<RequiredCheck>` 中每项恰有 `{id:CheckID,kind,location:SourceSpan}`；ID唯一且恰好对应一个 final ac.expect。kind与模板/numeric binding一致。没有 rule owner 的 runtime check 拒绝；source assert 不需要 NumericNode owner。

runtime ac.expect 的 condition 是 safety，path 是 source path 与 demanded operand validity；unsafe producer 受 path&&safety 支配。每个 rule/helper 按 source order 维护 SSA live evaluation path：表达式在 P 求值、返回 V 后，`live_after=live_before && (!P || V)`。operands 与 positional/keyword actual 从左到右求值；失败抑制该源路径后续参数、表达式和语句，即使它们没有数据依赖。assert以条件成功更新live；短路/条件只合并被选择的路径，未求值臂的false valid不能污染另一臂。helper失败传回caller。独立rule仍可安全收集检查，但全树失败禁止全部DriveNext。

失败值不能进入 observable next 或 log；numeric witness 必须匹配这一 threaded path，而非只匹配 operand-valid 或 fire。静态调用中求值失败编译拒绝。

新增无结果、无运行效果的 `ac.value.binding(value,valid,path)`，属性 `id:ValueID,domain:LogicalType`，把 source value 绑定到实际有限 SSA。ValueID 为 `{origin:Occurrence,slot:u32}`；同一个 ID 只有一个绑定，经合法 CSE 的不同 ID 可绑定同一 SSA。

新增无结果 witness：

```text
ac.numeric.proof(path,input_values...,input_valids...,actual_result,actual_valid,
                 check_conditions...,check_paths...)
attrs: mode="exact"|"low_bits", input_ids:Array<ValueID>,
       input_domains:Array<LogicalType>, result_id:ValueID,
       obligations:Array<NumericNode>, checks:Array<CheckBinding>,
       operand_segment_sizes:DenseI32ArrayAttr, origin:Occurrence
low_bits additionally: width:u32 in 1..64; exact forbids width
NumericNode={id:ValueID,operator:ClosedNumericOperator,
             operands:Array<NumericRef>,target:NumericTarget}
NumericRef={kind="input",index:u32}|{kind="node",index:u32}
          |{kind="constant",value:MathInt}
CheckBinding={id:CheckID,owner:ValueID,kind="division"|"shift"|"index"|"range",
              operand_ordinal:u32}
```

`NumericTarget` 为闭合 variant：`{kind="none"}`、`{kind="integer_boundary",domain:LogicalType.Integer}`、`{kind="index",extent:MathInt,storage:TypeAttr(iN)}`。to_bits 必须 integer_boundary，to_index 必须 index，其余 op 必须 none。index extent>0，`N=max(1,ceil(log2(extent)))` 且N<=64，先bounds后转换。比较的 result 为source bool，boundary/index的result精确匹配storage。

operand_segment_sizes 必须 `[1,n,n,1,1,c,c]`；input_ids/input_domains 长度n，checks长度c；所有controls是对应role的i1。每个input的值/valid/domain匹配其binding。check operand ordinal是0..c-1的排列，对应成对condition/path；只能列本witness节点拥有的checks，外部leaf的checks单独留在rule义务中。

obligations非空；node refs只能引用更早节点，input ref<n，无duplicate ID；每node贡献给末node或明确保留的demanded-error义务，末node ID=result_id。ClosedNumericOperator 恰为 constant/from_bits/from_bool/neg/invert/add/sub/mul/floordiv/mod/and_bits/or_bits/xor_bits/shl/shr/eq/ne/lt/le/gt/ge/to_bits/to_index；constant 使用一个 MathInt constant ref，from_bool 只接受经证明的 source bool input。static specialization 后的 math.static 已替换成 constant。

source math lowering 前捕获 rule.ac.required_numeric 的 ordered node 列表；每个 required node 恰好由一个 witness 覆盖，消除的中间节点仍在 obligation tree。

exact verifier 独立重算 domain/宽度，识别实际 SSA 的正确扩展、算子/谓词、安全 guard、转换和 valid 方程；不能调用 emitter 生成答案再比较。low_bits 只识别非负 leaves 的 add/sub/mul 树及明确的 `2^width-1` mask，将实际 graph 与该模运算树和 APInt 常量约减匹配。危险外部 leaves 仍保留自身 check/valid dependencies。

只容许 SSA rename、相同安全 producer 的 CSE、constant folding、明确可交换算子的 operand exchange。其他优化须有已验证 rewrite 或更新 witness；不识别不能跳过校验。CheckBinding 对应的 condition/path 必须就是相同 ac.check_id 的 ac.expect operands，并与 recipe 独立重算结果一致。proof/binding 保留到最后共同 IR 验证，emission 只忽略这些无运行效果的证据，绝不运行 bigint interpreter。

## 元数据与独立验证

- module parameters：ordered category/binding/type/default/origin/loc。
- materialized ports：LogicalType、原 parameter/ordinal、current|next role、owner、origin/loc。
- state/table：LogicalType、owner、declaration occurrence、initial value、domain；scalarized elements 额外携带原 element ordinal。
- rule data block arguments/yields：每个 data position 恰一项 LogicalType 与 originating state/connection 或 next target；controls 在使用位置单独标 evaluation_path/valid/enable；不能仅凭control用途就宣称source bool事实。一个已经证明为source bool的SSA可合法用于control，同一物理SSA的多种用途不强制复制值。
- helper args/results：相应 function argument/result attributes 带 LogicalType；constructor source symbol 在 final 删除，construction provenance 保留其 Site。

producer 从 registered rules、children 和 verified headers 重算 effects。只剔除 StaticExpr 求值证明不可达的路径，不依赖优化强弱；保留参数/字段声明顺序，origins 结构排序去重。未知 static choice 或 child uncertainty 导致 conservative union，不能假定只读。

link 在特化前自底向上重算 generic summaries 并比较所有 import snapshots；特化后重算 exact effects、materialize physical endpoints，再关闭 alias/driver 义务。exact 必须相等；conservative 是上界，不能替代最终许可。R/R 与 R/W 可别名；两个 W-capable ordinal 的同 identity 在 exact closure 时拒绝。

unit verifier 不要求 root/link，不伪造零端口顶层。linked/final verifier 才要求唯一 selected entry、完整 owner/instance closure；一个错误 stage 字符串不能绕过它们。

## 共同硬件出口

保留 donor concrete module/state/rule/record 形式及必要 guarded scf.if。所有参数绑定、接口导入、静态控制、local carrier、数学值、helper call/body、constructor reference 都已消除。DFFE next 是 declaration order 的 data+enable；读 current，不读另一 rule 的候选。

每个 state 带 `ac.domain`，foundation 值只能是 `default`：rising edge、active-high synchronous reset。声明的 reset image 与数学/driver/check 义务明确。两个 backend 都对同一 IR 运行 final verifier；RTL-private legalization 将 guards 变为等价有界方程再消除高层控制，不能自己设计仲裁/状态。

source precommit 失败在全树 DriveNext 前被报告，Step failed 后必须 whole-tree Reset；这不是多资源事务原子性的替代证明。Queue/Slot、多 lane、一般 memory/CDC、四态、多 clock/reset、dependent port types、external DUT 与完整 system 的扩展仍必须逐项完成，不能据此 foundation 宣称框架迁移完成。

## 准入、删除与验证

本包拟 hard break 旧 source/interface micro-schema、Python 语义 lowering、QueueGraph planner/text conversion 与旧 PYC C++ 路线。现有适用 MLIR 分析和 RTL assets 可迁入新链。旧 source-unit 架构保留，旧 op 同名不等于 schema 兼容。

必需证据包括：parent 缺 child body 时编译；Request() defaults/kwargs 仅凭 header；Bank 2/4；static/runtime math 与 loop locals；header range/effect/default/body tampering；重复 nominal owner/W ordinal；错 stage；遗漏 source check；低位 proof 算子/输入/mask篡改；source→AC→TU 所有权；同 final IR 的 C++/Verilog独立行为。范围和完整回归见迁移验收规范。

本提案尚未批准或实现；接口审阅必须解决所有未闭合字段，再请求用户批准，不能用“之后再完善”跳过 exact contract。
