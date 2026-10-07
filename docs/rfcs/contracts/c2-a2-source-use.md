# C2-A2：原始 next 写入与 helper 返回的源 use 载体

修订 B。状态：待新的独立设计审阅与用户精确批准。独立 architect
u02_ods_design 给出设计，PM 整理。获批 C1-C/C2-C/C3-C/
C2-N1-C 原文保持冻结；本增补尚不授权实现。目标是按既有
U02-B 工作包编译原样 Packet、Accumulator、Core 三份 Python
源，保留每次源写入和返回的来源、实际值、路径及 target，供
MLIR 在 specialization 后独立提取 C2 RequiredUse；不增加
另一套 Python frontend 或 backend 语义引擎。

## 缺口与前后行为

冻结 C2 已定义 ValueID、UseID、RequiredUse、UseTarget、
ac.value.use、ac.yield_bindings，并规定 specialization 后从
实际 source next-assignment/return-target operations 独立提取
RequiredUse；不能从合并的 final yield 猜测原目标。然而 C2
没有给通用 scalar source assignment 明确的序列化 op/属性。
现有 Rules.cpp 直接生成源 rule yield；私有 C++ assignment
列表在 .ac 序列化/重读后不存在。donor 的 next map 还可能按
最后写入覆盖，与 C1 同一路径重复 next 必须拒绝不符。

原始 Accumulator 的条件 self.total 写入与无条件返回是两次
不同 source use；Core 对 left_request 的完整 Request 写入
是第三次。变更后这三次在 source MLIR 中各有实际 SSA 操作、
独立身份和 target，特殊化/inlining 后才经已批准的 proof/
binding 流程转为 final data,enable 方程。局部变量赋值不是
持久 next use；读 self.total 始终读 committed current。

扩展现有 ac.value.use 的 target/results 会混淆原始 use
权威与派生的 final witness。新增一个 source-only operation
是本增补的全部新 op；不添加 dialect type、Python 装饰器、
CLI 选项、runtime ABI、backend route 或隐式仲裁。

## 精确 operation 与闭合 target

    ac.source.use(value:T, valid:i1, path:i1)
        -> (data:T, enabled:i1)
      id:UseID
      source:ValueID
      target:SourceUseTarget
      loc:原赋值/返回位置

无 region、无 symbol、无状态分配。必需属性均不许缺失或错型；
target 为闭合 DictionaryAttr：

    SourceUseTarget =
        {kind="next_scalar", state:StateRef}
      | {kind="helper_result", ordinal:u32}
      | {kind="helper_return", call:Occurrence, ordinal:u32}

前两者由 source producer 写入；helper_return 由验证过的
inline/expansion 在 linked 阶段生成。UseID、ValueID、
Occurrence、StateRef、u32 沿用 C2；UseID.role 对
next_scalar 为 next，对其余两个为 helper_return。这个
初次增补不声称覆盖 C2 的动态 next_selection，相关 source
carrier/提取门槛仍列入后续能力表，不可从本 op 擅自类推。

唯一值方程是 data=value、enabled=path&&valid。
enabled 仅是 next-enable 或 helper-return-valid，不证明某个
i1 是源 bool。inactive data 不可观察，后续合并仍须产生
C2 允许的类型正确 placeholder。本 op 既不写 Q、不提交、
不吞掉源错误，也不提供 winner。不得标为 Pure/可投机/
可随意丢弃；直到独立提取和受证替换完成才可删除。
这里的保留是编译器证据义务，不是新增硬件 side effect。

## 语境、类型与身份

next_scalar 只能处于已注册 rule 的计算区域及其已验证控制
子区域，state 精确解析为 enclosing module 的 owned DFFE
或 formal port 的 scalar/whole-record StateRef。T 与其
LogicalType 的实际 payload representation 相等；源
math_int 须经获批的边界转换后才成为 finite next 数据。
record field 不是独立可写 state，child 内部 state 不可写。

helper_result 只能在 source-owned constructor/value helper body，
ordinal 是 declared data result，而非 hidden valid result。
helper_return 只能在已验证 helper inlining 后、已注册 rule
内部，call 指向该真实展开 call occurrence。其 T 遵守原
ValueConstraint；数学整数/静态列表只在原 C2 限制内暂存，
不会因本 op 获得 runtime ABI。U02-B 只要求原三源的有限
logical result 子集；其他合法大能力仍须显式 capability
诊断直至后续实施。

每个原始 next assignment、每个数据 return ordinal 各产生
一个 use。UseID.origin 指真实赋值或 Return occurrence，
slot 为左到右 target/result ordinal，scalar 为零。
constructor 隐式 constructed-record return 可用 constructor
声明 occurrence 及 result ordinal，不伪造 Python statement。
不同 use 可共享数据 SSA；共用 ValueID 必须在同一未来
ProofScope 内绑定同一实际 value/valid/path 和兼容 logical
meaning，否则创建不同 ID。loc 仅用于诊断，不成为调度优先级
或身份。rule 注册调用 occurrence 和 rule 方法体 Return
occurrence 独立保留。

source ValueID 必须对应 operand(value,valid,path) 的真实
边界值；只放一个孤立 id 属性不能满足义务。verifier 对
next_scalar 从实际 rule output handle、方向、LogicalType、
StateRef 和模块状态声明重算 target，拒绝同型 handle 交换、
错 owner、未声明状态、写 child 内部/record field、错误绑定。
next_scalar 的 data/enabled results 必须沿真实 SSA 到对应的
candidate merge/rule yield pair；不能流向 current read 或
另一个 owner。helper_result 的 data 到对应 func.return 数据
operand；helper_return 的结果取代实际 caller call-result 路径，
与 call/ordinal/返回 SSA 精确匹配。修改任一边、增加一个
无关 good marker 均无效。

目标类别的完成关系不同：每个存活的 source use 在 helper 展开后
恰产生一个 RequiredUse，并在 final 恰对应一个 ac.value.use marker。
每个 next_scalar RequiredUse 在其自身 UseID 下对
ac.yield_bindings 贡献恰一次。helper_return 在其自身 UseID 下
对 ac.yield_bindings 贡献恰零次；它通过实际返回/调用 SSA 与
hidden valid 绑定，若调用者随后写 next，由另一条 next_scalar
use 提供该写入的 contribution。helper_result 在提取前必须改写
为 helper_return，不直接进入 final RequiredUse。一个 helper 的值
即使最终普通数据不被使用，只要该调用在 source path 上被求值，
其 demanded check、返回 validity 与 use marker 仍须保留到有
独立的安全删除证明；不得因为无 yield contribution 就跳过错误。

同一 Return site 的多个 flat data ordinal 共享一个 hidden valid：
该 site 的 valid 等于到达它的 live path 与全部必需 return 元素
enabled 的合取，不能每个元素独立宣称返回成功。互斥 Return site
分别计算该值，再按真实 site guard 合并；未选择 site 不参与
失败判断。helper_result ordinal 必须匹配声明 data arity，不能
把 hidden valid 当数据结果。值/路径检查先于候选 next 使能，
helper-return 用途不因不写状态而成为可无条件提前执行的纯操作。

## 路径、错误、分支和重复写

path 是 C2 的 threaded source use path，valid 是实际
边界值有效性；其 AND 不替代独立 source error path。
继续使用已批准的

    live_after = live_before && (!evaluation_path || result_valid)

按源顺序求值 actual，只执行选中 runtime 分支，失败抑制同一路
之后的表达式，未选分支的 false valid 不污染选中路径。
一个后续 next enable=false 不能抹去先前 demanded
除零/index/range 错误。

互斥分支的多个原始 use 和 UseID 均保留到提取，即使
最后合并为一个 next pair。C1 的同一路径重复目标静态拒绝、
可交叠目标 precommit 检查、不同已知 owner 独立继续生效；
本增补不引入 last-write-wins、临时表或后端仲裁。
valid=false 或 path=false 不能使原本非法的重复源写入变
合法。每个 output merge 必须由该批 op 的实际 data/
enabled 结果构成；不能从最终 yield 倒推 use。

## helper 展开与静态初始化

参与本 source-use 闭包的 source func.call 必带
ac.origin:Occurrence，标识真实调用而非诊断 loc。
callee/signature 仍由 owning header 唯一授权。verified
inlining 按真实 SSA 映射克隆 helper source use，对
UseID、ValueID、call、nested occurrence 追加一致的 C2
call-expansion frame；把 helper_result(ordinal) 改为
helper_return(expanded_call_occurrence,ordinal)。
原 helper return 数据/valid 与 caller result 的映射须先
验证再丢弃 call 边界。多条 Return 路径不合成一个伪来源；
改 callee、ordinal、frame、返回 SSA 或 caller 连线均应
失败。

精确 rebasing：令 C 是调用点已经展开到当前 caller 的
Occurrence，H 是 canonical helper，P=C.expansion，
F={kind="call",site=C.site,callee=H}。helper_return.target.call
保留 C（不附加 F）。被克隆 callee 内部的 Return、ValueID、UseID
及来源 occurrence O 均取 expansion=P ++ [F] ++ O.expansion；
callee 本地的原 expansion 保持其外到内次序。callee 内部的
nested call 先同样被克隆/重基，再在其自身 inlining 时只追加
该 nested call 的新 F；已消费的 call 边界标记为已展开，再次
展开必须拒绝，不能把 caller 的 P 或 F 加两遍。

例如 Core 在静态循环第 j 次调用 Request constructor 时，
C.expansion 末项为 iteration(j)。返回来源的 expansion 为
[iteration(j),call(C→Request),callee-local frames...]，target.call
仍是带 iteration(j) 的 C；第 k 次调用是不同 iteration(k)。
helper 中再调用另一 helper 时，第二个 call frame 出现在该 nested
call 的已重基 occurrence 后，身份不依赖 pass 遍历顺序。

StaticExpr evaluator 若遇到已验证的 helper source use，只按
真实 MLIR SSA 把 data 透传、enabled=path&&valid；false
return valid 是静态求值失败。它不能假定 constructor 参数
排列等于 record field 布局。Header/body snapshot 比较包含
该 op、ac.origin、属性、result 路径与 helper body。Owning
header 仍是 declaration authority；body snapshot 只作
已验证副本。此变更要求重新编译受影响的 Packet 等
source/header，不读取旧版本 artifact 作为兼容输入。

## 提取、替换与最终拒绝

按同一条 MLIR 路线严格排序：

1. 每源编译发布实际 source use；source verifier 校验闭合
   记录、调用/target/SSA、body/header snapshot 与 origin。
2. link 先完成 SourceOwner/header authority、namespace、
   signature/effect/helper body 的比较。
3. specialize/static 展开及 helper inline 同步改全部 occurrence、
   ValueID/UseID/target；不得先丢 branch uses。
4. specialization 后、list scalarization/numeric lowering 前，
   仅从源 use op 读 id/source/target，独立建立有序
   ac.required_uses:Array<RequiredUse>。helper_result 此时
   必须全部解析；不能从 yield、私有 sidecar 或静态推测生成。
5. 按当前保留的真实 SSA 边做 source math/数字 lowering，
   产生现有 ac.value.binding、ac.value.use、proof/check、
   candidate contributions 与 ac.yield_bindings。
6. 所有目标均一对一核对 RequiredUse、ValueID、实际 use 的
   data/enabled 与 check path。next_scalar 另核对恰一次真实
   yield contribution；helper_return 核对对应 helper Return、
   caller result/ordinal SSA 与 hidden validity，且其自身 UseID
   无 yield contribution。完成目标各自的证据后，才用普通 SSA
   方程替换并删除 ac.source.use；每次删除已留存可复查的
   C2 证据。
7. final verifier 和 cpp/verilog 两个真实 emit 入口拒绝任何
   残留 ac.source.use、helper_result 或 source math carrier。
   ac.stage 是诊断标签，改写它不能跳过义务。

RequiredUse 按 UseID 的结构顺序排序，不建立源执行优先级。
source-use 与最终无效果 proof 的责任不同：前者原始执行
路径要保留到提取，后者留在 final 供独立 verifier 检查。

## 原样 fixture 与独立 gate

原三源的三项 state-writing use 固定为：

| Source | 源使用 | 目标 |
| --- | --- | --- |
| accumulator.py 条件 self.total=total | 路径取旧 item.valid | owned total |
| accumulator.py Return total | 每成功返回路径 | formal result |
| core.py self.left_request=Request(...) | 完整 record 写入 | owned left_request |

Packet 构造器有 helper_result anchor；Core 的 runtime
Request 调用 inline 后有与实际调用 occurrence 对应的
helper_return use。Accumulator 两个 next 目标不能合并：
total 只在旧 item.valid 成功分支写，result 每次成功
返回写；Core 读取旧 left_request.valid，再提出完整新
Request。两次 Accumulator child 共用同一 SpecKey 定义，
状态仍属不同实例 OwnerRef。

### 四个源/IR 对照

以下 I_total/I_return/I_ctor/I_core 是各自完整闭合的 UseID，
V_total 等是从真实表达式建立的 ValueID；这些代号不另造文本
IR 语法。每个真正的 source op 仍带原 AST occurrence、类型、
MLIR loc 和已验证 StateRef/调用身份。

| 原源码及当前缺口 | 增补后的实际源 IR 边 |
| --- | --- |
| Accumulator 条件 self.total=total 直接折到一个 yield pair | %td,%te = ac.source.use(%total_bits,%total_valid,%item_valid)，id=I_total，source=V_total，target=next_scalar(owned total)；真实 %td/%te 进入 total candidate merge。 |
| Accumulator 的 return total 直接生成 result pair | %rd,%re = ac.source.use(%returned_bits,%returned_valid,%return_path)，id=I_return，target=next_scalar(formal result)；它与 I_total 分立并进入 result pair。 |
| Packet constructor 直接 func.return(%created,%hidden_valid) | %cd,%ce = ac.source.use(%created,%hidden_valid,%return_path)，id=I_ctor，target=helper_result(0)；func.return 使用 %cd 和完整返回 validity。 |
| Core 的 Request 调用与 left_request 完整写入直接合并 | func.call 带 ac.origin=C；inline 后 constructor use 的 target=helper_return(C,0)，保留 caller result/valid 路径；另一条 I_core next_scalar(owned left_request) 写入完整 Request。 |

helper-return use 即使返回数据最后未用于 next，也仍须保留其
实际求值路径上的 demanded 错误；不能为它伪造 state yield。

独立测试应覆盖：op parse/print/reparse、闭合 target/类型/
arity；重复/缺失 UseID、冲突 ValueID、错 StateRef/handle、
同型 data/enable 交换、脱离真实 yield 的孤立 use、错误
path/valid 绕过异常；同一路径重复写拒绝/互斥写分立；
next 后 current 仍读旧值；错误 helper callee/ordinal/
call frame/返回 SSA；提取后丢/改 target、提前删除、final
残留 op、StaticExpr 经新 helper return anchor 求值。
另需反例：缺失/重复 next contribution、未使用但仍 demanded
的 helper return、平坦 tuple 多返回共用 hidden valid、互斥
Return site、nested call 与 iteration frame 顺序，以及
CSE/DCE/canonicalization 在提取前不得抹掉 source use。
三源必须各有独立 producer，parent 隐藏依赖 Python/body
仍可编译；测试不可只比较 producer 两次输出，预期来自
C1/C2 本合同。

最终 C++ 与 Verilog 的独立执行 oracle 仍为 Reset
(7,19)，Work 后 (7,19)，三次 Xfer (1,2)/(1,4)/(2,6)，
再 Reset/rerun。U02-B source 编译成功不算这些执行证据；
U02-C link/owner 与 U03 final proof、双后端和 C3 SDK
仍需各自验收。

### 计划门槛与命令

下列测试/target 是计划交付项，尚不声称存在或通过。
PYC_A2_BUILD/PYC_A2_DRIVER 必须指向同一当前 checkout 的
LLVM/MLIR 22.1.8 构建和获批 C3 driver；执行前归档实际绝对路径、
候选 Git/文件摘要，不能使用旧 acc.py/pycc 或另一 worktree 的二进制。

| lane | 计划载体和独立观察 |
| --- | --- |
| structural | SourceUseContractsTest.cpp / ACIRSourceUseContractsTests：闭合 target、真实 StateRef/handle、UseID/ValueID、path、helper Return/ordinal、排序和同型交换变异。 |
| source/header | tests/system/test_source_use_provenance.py：原样 Packet/Accumulator/Core 三个独立 producer，仅消费已发布 header；固定 expected use/target 与 roundtrip，隐藏依赖 Python/body。 |
| link/extraction | 同一系统测试实际 C3 link：authority/snapshot、nested/iteration rebasing、RequiredUse 独立提取和 helper demand/return cardinality 的正反例。 |
| final/emit | 同一系统测试先证明未改合法 final IR 两后端可消费，再分别向合法 final 输入注入残留 source.use/错误 helper_return；对 cpp/verilog 四种组合真实 emit 均拒绝且无新输出，replace 保持旧输出不变。 |

    : "${PYC_A2_BUILD:?set current-checkout LLVM22 build}"
    : "${PYC_A2_DRIVER:?set current-checkout C3 driver}"
    cmake --build "$PYC_A2_BUILD" --target ACIRSourceUseContractsTests -j 6
    ctest --test-dir "$PYC_A2_BUILD" -N -R '^ACIRSourceUseContractsTests$'
    ctest --test-dir "$PYC_A2_BUILD" -R '^ACIRSourceUseContractsTests$' --no-tests=error --output-on-failure
    PYCIRCUIT_TEST_DRIVER="$PYC_A2_DRIVER" pytest tests/system/test_source_use_provenance.py --collect-only -q
    PYCIRCUIT_TEST_DRIVER="$PYC_A2_DRIVER" pytest tests/system/test_source_use_provenance.py -m system -q

系统 gate 按冻结 C3 形式分别对 packet.py、accumulator.py、
core.py 调用 pycircuit compile -c 与明确 -I 单元，再显式
pycircuit link 全部单元 --top demo.core.Core，最后对同一个
program.ac 执行 pycircuit emit --target cpp 和 verilog。
负例断言工具非零、准确诊断及产物未变化后，测试自身才以 0 退出。
CTest/pytest 发现数、执行数和断言非空；必需用例 skip/xfail
或零断言不得关闭义务。最终证据记录命令、退出码、候选摘要、
固定预期表、变异输入、header-only 访问审计和产物路径/hash。

## 实施边界和授权

影响 ODS/注册/verifier、Rule expression/statement 生成、
header helper return、static evaluator、specialization/inline、
RequiredUse 提取、数字 lowering、final proof/merge 和
双 emit 拒绝测试。诊断须给 source location、UseID、
target 和实际不一致的 SSA/owner/path；缺来源锚点是
incomplete source IR，不得从 final yield 回填。

| 具体调用/文件 | 必需改动与旧捷径处置 |
| --- | --- |
| compiler/acir/include/acir/Dialect/ACIR/ACIROps.td 与独立 verifier TU | 同一 ac dialect 增加 source.use；source/link/final 按阶段验真，在 final 直接拒绝。 |
| PythonImportRules.cpp 及后续独立 expressions/statements 文件 | 将直接 yield 的 next/return 改成实际 source use + 受证 candidate merge；不保留私有 assignment sidecar 或 last-write-wins。 |
| PythonImportRecords.cpp、SourceHeaderHelpers.cpp | constructor 的直接 struct.create→func.return 路径新增 helper_result anchor；helper whitelist、物理签名与头部 authority 同步校验。 |
| ACIRStaticEvaluation.cpp | 按原 MLIR SSA 计算数据/valid，再解释同一 source.use passthrough；不猜参数与字段同序。 |
| 后续 specialize/helper-inline/RequiredUse 提取及 numeric/final verifier | 消费真实源 op、重基身份、证明一对一提取/替换；未完成前明确能力拒绝。 |
| 独立 cpp/native、system 与 C3 driver gates | 固定 oracle/变异、真实 compile/link/emit 和双 backend 残留拒绝，不从实现输出生成预期。 |

失败回退边界是尚未切换的隔离迁移候选：撤回 A2 实现，保留原
批准合同与历史证据，从源代码及选择的工具链重新生产受影响
header/body。旧临时 Packet/Accumulator/Core source units 不能
直接读为 A2 输入；产品主路线仅在 M5 一次 hard break 接受
新候选。回退不加入 schema mode、旧 helper 兼容入口或第二 lowerer。

本增补没有新增 Python 语法、reset、仲裁、storage、runtime
或 backend 路线。实施时一次切换新 source/header；
无 schema-version 选择、旧 helper header 兼容、第二
assignment 语言或旧 QueueGraph/ACPy fallback。接口仍需
独立设计审阅并得到用户对精确文本的批准。
