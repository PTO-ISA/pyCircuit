# C2-DECL-R：flat record final 投影与有限值闭合

修订：B。状态：精确提案，待独立设计审阅和用户批准；未实施、未验证。
日期：2026-10-01。产品基线：`9ff015a2acff537338f76b8bbc44c5229ccf7ae9`。
设计作者：`/root/m3_contract_admission_author`，Astra/high。
独立 oracle 与独立设计审阅由不同实例完成，不由作者自证。
修订 B 修正 helper-return 的独立 threaded path，并闭合单 contribution 的
aggregate selector/zero-placeholder；不扩大 record ports 或多 writer profile。
任务入口：[M3-E01](../../work-items/m3-record-final-projection-design.md)。

## 1. 批准对象与明确边界

本提案交付一个用例：普通 portless function module 拥有一个 immutable record，
字段声明顺序为 `(lo, hi)`，reset `(3,17)`；rule 用旧 Q 构造 `(hi,lo+1)`，
两次成功 Xfer 后为 `(17,4)`、`(4,18)`。复用 default clock、empty SpecKey、
现有全树 Work/precommit/Xfer、reset/rerun 和 scalar observation。

C1-C/C2-C/R1-B 已批准 nominal record、完整构造、纯 helper、字段范围、
source-order 求值、旧 Q、整 record D/E/唯一 StateID。M1-C 已批准函数模块/
嵌套 rule。本文不重批这些行为。本文冻结 C2-DECL-A 尚未定义的 final record
声明/值载体，并列明当前 scalar-only reader 必须改变的接纳形状。

批准依据的原文 SHA-256：

| 合同 | SHA-256 |
| --- | --- |
| C1-C | `5768e1571e56eb1a5963ff5d40dff1087de38ee997e9c52b5ef91f520da90dfc` |
| C2-C | `387cf52b129f864b87a1b2a388213a3fe36d81c330d94ced0e6696522b58a322` |
| C3-C | `0c476ced27519cf93427a89f77b9348d388e96db57fb183790144d24118b1170` |
| R1-B | `76283661bffba96aad544e31d2b4996f019135f448c4c8b685ebb02eab8edfba` |
| M1-C | `84b551ea84d6aa0956ea2342f520f4aafc63f7d741cc651500446a26f49c35cd` |
| C2-DECL-A | `38dd31d13cff150cf7b778e9c3df469f9ab1e0b55c8b2311f7c8d05d49b736b8` |
| N1-C | `ac7d56a403e21ac03186f17c2f75f9c8eb53a00dcb74677c5286219dc5f1e6c3` |

本包接纳 **恰两个字段的 flat record**。每个字段是现有 LogicalType.Bool 或
LogicalType.Integer，integer 使用现有最小规范宽度 `1..64`、区间与解释；
不要求两个字段同型或同宽。record 总宽可到 128，不受单个 scalar 的 64 位上限。
字段名可以不同于 lo/hi。class 无装饰、继承、动态属性；constructor 仅按当前
record 源形式把已绑定的有限 scalar 参数直接赋给全部字段，允许静态 defaults、
positional/keyword binding 与不同的参数/字段顺序，不执行任意 Python。

有限 helper 子集：普通函数，参数/单返回为上述 record 或现有 finite scalar，
且至少一个参数或返回是本包 record；不借此交付一般 scalar-only helper。
正文由纯局部赋值、字段读取、scalar 表达式、source assert 和一个末尾 return
组成；return 可构造 record。helper 不声明状态、执行 observation 或调用 rule。
本包普通 value helper 不再调用另一个 value helper；可调用本包 constructor。
rule 可以多次调用同一个 helper，保留各 call occurrence；helper single-return
仅在本包含 record 的闭合路径中接纳，不以 source-only U01 helper成功替代final验证。数学表达式只接纳
现有 shared numeric verifier 已能闭合的有限算子/路径，不能因为此包增加
一个 scalar 算子；不受支持的表达式明确 capability-reject。静态 defaults/reset
仍由同一 MLIR evaluator 求值，不能执行 Python callback。

结构范围是一个 portless module 的本地 scalar/record state；可与既有 scalar
state/check 混合。每个 record target 在一个 rule 中至多一个 source next use，
在 owning module 中至多一个 rule writer；即使 guards 互斥，多 contribution/
多 writer record 也先 capability-reject。本包不新建 runtime overlap-check carrier；
这是能力限制，C2 的 OR(Ei)/verified merge/无 priority 合同仍保持，见 §6.4。
record 只能整值赋 next，不能写 `state.lo`。record data ports、
嵌套 record、record/list alias 或 constant 的 final 投影、集合、非空 static、
系统嵌套、外部 typed DUT、memory/CDC/四态均不在本包。unsupported **supplied**
声明/helper 即使 unused 仍拒绝，不借 DCE 扩大接纳；已支持 pure facade/空声明源保留。
两字段限制是此包 final 能力边界，不是永久源语言规则。已有较宽的 source/header
声明测试继续有效；nested/更多字段等声明若已能独立 compile，不提前改为
source 拒绝，而是在本包 link/final projection 明确 capability-reject。

不新增 op 名称、dialect type、runtime 类型/方法、公共 Python 名字、CLI、
receipt/journal/source-map/模型 ABI 字段。**有一个新 IR 属性和若干既有 IR
字段/operand 的明确扩展**，见 §3；不得把它称作“完全无 schema 变化”。
SYSTEM B、EXPECT B、artifact role 与整项目其他扩展没有在此获批。
Decision 0283 的现行 scalar profile 在本包验收前不扩大。

## 2. 精确源形式与 before/after

以下是拟接纳的完整逐源 fixture；record class/constructor 拼写已经存在，
带参 helper 算术和 stateful final 路径是本包拟实现部分，**不是当前编译 PASS**。
`self` 仅为普通数据 class 的 constructor receiver，不是模块作者接口。

`pair.py`：

```python
from typing import Annotated

Word = Annotated[int, range(256)]

class Pair:
    lo: Word
    hi: Word

    def __init__(self, hi: Word = 17, lo: Word = 3):
        self.lo = lo
        self.hi = hi
```

`advance.py`：

```python
from .pair import Pair

def advance(current: Pair) -> Pair:
    return Pair(hi=current.lo + 1, lo=current.hi)
```

`top.py`：

```python
from pycircuit import module, rule, report
from .pair import Pair
from .advance import advance

@module
def Top():
    state: Pair = Pair()

    @rule
    def tick():
        nonlocal state
        candidate = advance(state)
        state = candidate
        report("q_lo", state.lo)
        report("q_hi", state.hi)
        report("d_lo", candidate.lo)
        report("d_hi", candidate.hi)

    tick()
```

`state = candidate` 后再次读 state 仍是旧 Q；candidate 是局部不可变值。
report 每次仅传入一个现有 scalar 值，不定义 record event/ReportStat schema。
观察的语义值在 Work 求出，只有成功拍按既有协议发布；它不表示 Work 提前改 Q。
第一拍 q 为 `(3,17)`、d 为 `(17,4)`；第二拍 q 为 `(17,4)`、d 为 `(4,18)`。
无限运行最终会因 Word range 失败；独立测试使用有限拍数，不把溢出变成回绕。

before：source/header 可描述 record/constructor，final preflight 拒绝任何
supplied `ac.struct`/`func.func`，以及非 integer reg payload；C2-DECL 只投影
scalar alias/constant。after：三个源各自 compile，parent 仅消费 headers；
link 验证 authority、展开 helper/constructor、闭合值证据，再投影 Pair 声明，
删除全部 executable helper definitions，保存一个无需 Python/header 的 final。
CPP/RTL 均重新验证该同一 final；没有后端自行执行 helper 的路线。

## 3. 完整接口差异清单

下表以基线真实 reader/ODS 为 before。除逐项列出的字段/接纳变化外，不增添
另一个替代形状、版本选择或 fallback。实现内部算法/函数拆分不构成新公开 API。

| 表面 | before | after / 精确变化 |
| --- | --- | --- |
| `ac.struct` properties | source/header 有 `sym_name:StringAttr, fields:ArrayAttr, constructor:FlatSymbolRefAttr`，final 不接纳 | **字段不变**，final 接纳 §4 的 canonical definition；constructor 留作非可执行 provenance，§4 定义其精确校验，不再 lookup 一个 final func |
| final declaration unit | 仅 scalar `ac.type_alias/ac.constant` | 加入本包两字段 `ac.struct`；envelope 和 scalar 声明形状不变；无 helper definition、import_snapshot |
| `ac.value.binding` 第一个 operand | ODS `AnySignlessInteger` | ODS 放宽为 `AnyType` 后必须立刻由共享 verifier 限定为现有 scalar 或本包 `!ac.struct<...>`；其余 operands 仍 `valid:i1,path:i1`，属性仍恰 `id,domain`，无结果 |
| `ac.value.use` 第一个 operand | ODS `AnySignlessInteger` | 同上；其余 operands 仍 `valid:i1,path:i1`，属性仍恰 `id,source`，无结果 |
| final `ac.reg.ac.initial_value` | scalar payload-typed IntegerAttr/BoolAttr | scalar 不变；record 新增唯一闭合 DictionaryAttr `{type:TypeAttr(!ac.struct<symbol>),fields:Array<IntegerAttr>}`，见 §5。它不是 MLIR `TypedAttr` 子类，旧 unconditional TypedAttr cast 必须分支验证 |
| rule `ac.required_records` | 不存在 | **唯一新增属性名**，`Array<RequiredRecord>`，恰 §6 明列的 read/get/create 三种 variant；保存 read/get/create 源义务，不可执行、不是第二调度图 |
| `ac.struct.create/get` final 接纳 | current source/helper op；final carrier 未闭合 | 原 operand/result/property signature 不变；source-linked final create/get 要求 `ac.origin:Occurrence`；§6.4 的 synthetic get/create 恰无普通属性且不伪造源义务；get.field 仍 StringAttr，create.values 按声明顺序 |
| aggregate merge 的既有 scalar `arith.select`/struct ops | 无 record final merge 接纳 | §6.4 以两个synthetic get、两个同一E的scalar select、一个synthetic create构成唯一normalized shape；无新增signature/property/attr/op，actual SSA必须闭合匹配。final不残留scf/index |
| final rule 值/数字/use闭合 | scalar 输入/next、无通用 helper_return、无 record SSA | 对含本包 record 的 rule 运行 §6 **统一闭合**；scalar NumericNode/Proof/CheckBinding 字段与算子不变，record 不进入 arithmetic witness；`RequiredUse` 增加实现已批准的 helper_return variant，schema 本身不变 |
| final check/use/source origin | current reader 强制 leaf definition/path 属于 enclosing module、expansion 空 | 对本包展开的 helper/constructor 按 §7 验证 caller→callee chain 与 leaf owner/path；CheckID/RequiredCheck/Occurrence/SourceSpan 形状不变。不是 EXPECT B 的总体实施 |
| compiler-derived hardware view | StateCarrierSnapshot 只有 scalar width/initializer，表达式 reader 只读标量 | 以 §8 的确定递归解构生成两 scalar leaf view，同时保留一个 record reg/StateID/D/E；这是同一 final 的派生内存视图，无新增序列化图或公共字段 |
| source-owned C++ header | scalar alias/constant 与 module family | 增加 §9 两成员 plain aggregate；没有 Python constructor/default/helper 方法或 C++ arithmetic operator；已有命名碰撞与 files/source_groups 规则覆盖它 |
| CLI/runtime/receipts | 当前 C3 与 M1/Sink clarification | 全部不变；unit.json/generated.json/source-map 不加 record 字段，runtime `SimDFFE<T>` API 不变，模型 ABI 不变 |

## 4. Final 声明、constructor provenance 与 authority

先完成全部 supplied body/header、N1 exports/import_bindings、helper body 和
snapshot 校验。由 owning interface 选择本 owner 的 `definition`：既有全部
scalar declarations，加全部支持的 flat record，含 unused/private record。
不存在“只复制被第一个实例用到的 record”或把 facade snapshot 升为 owner。
声明类型引用需闭合于显式 supplied unit 集，不读取额外源文件。

每个 source-owned final builtin module 的属性仍恰为：
`ac.stage="final"`、`ac.unit_kind="implementation"|"declarations"`、
`ac.source_owner:SourceOwner`。implementation 含排序的声明再恰一个 ac.module；
declarations 只含排序的声明。源单元按 SourceOwner 结构顺序，声明按 canonical
sym_name UTF-8 字节序且唯一。helper-only 源保留一个空 declarations unit/header。
不新增一个同 owner unit，不改变普通 module entry/现有 system descriptor。

Final `ac.struct` 无 operand/result/region，inherent properties **恰为**：

| property | 约束 |
| --- | --- |
| `sym_name` | 原 canonical nominal name，属于 enclosing SourceOwner |
| `fields` | 两项、原声明顺序；每项恰 `{name:StringAttr,type:LogicalType,origin:Occurrence,location:SourceSpan}`；不同非空字段名；type 只能本包 finite scalar |
| `constructor` | 原 FlatSymbolRefAttr，精确等于 `sym_name + ".__init__"` |

普通 attributes **恰为** `ac.source_owner`、`ac.origin`、
`ac.declaration_role="definition"`；保留原 MLIR loc。record origin 的
site.definition 等于 sym_name；field origin 的 site.definition 也是 record
sym_name，原 declaration-relative AST path/expansion/SourceSpan 原样保留。
顶层 record/field declaration 的 expansion 为空；span.path 等于该 SourceOwner.path。
两个 field AST path 必须不同，不以同型字段重排或重新合成来源代替复制。

Source/header 的 constructor 仍必须解析到经验证的 source-owned func，且其
`ac.record`、single return、完整字段初始化和 defaults 匹配。**Final 的同一
constructor property 只是一条 canonical source provenance 引用，不是 SymbolUser
调用或待满足的 executable symbol reference**。Final 不含此 func，也不 lookup
原 header；禁止以一个 dummy func、constructor stub 或复制 helper body 满足它。
将 constructor 改成另一个 record 的 __init__、缺省/UnitAttr、未知字段均拒绝。
保存/reparse 后只需 record 声明即可验证 nominal identity 与构造/字段 SSA。

示意（字段 type/origin/location 用缩写，不是可运行 fixture）：

```text
source owning header:
  ac.struct @demo.pair.Pair fields=[lo:Word, hi:Word]
            constructor=@demo.pair.Pair.__init__  role=definition
  func.func @demo.pair.Pair.__init__(hi,lo,path)->(Pair,valid)
            ac.helper_kind=record_constructor
            ac.return_form=single ac.result_constraints=[logical Pair]

final owner unit demo/pair.py:
  ac.struct @demo.pair.Pair fields=[same lo, same hi]
            constructor=@demo.pair.Pair.__init__  role=definition
  // no func.func; no constructor call remains
```

不增加全项目 types 头、nominal hash 或 producer“已验证”旗标。final nominal
resolution 通过完整 package 的 source-owned canonical declaration 索引，验证
StructType 名字与 record sym_name 精确相等；同形但不同 nominal 不互换。
一 source owner 和其符号/大小写/import-module identity 规则沿用 C2-DECL。

## 5. 状态与 reset image 的唯一形状

Source R1 形状不变：一个 record declaration 产生一个
`!ac.reg<!ac.struct<"demo.pair.Pair">>`，`ac.shape=[]`、`ac.element=[]`，
`ac.logical_element={kind="record",symbol=@demo.pair.Pair}`，scalar InitialSpec
引用 verified constructor。以 `(empty SpecKey,declaration)` 恰求值一次完整
reset；验证 nominal、两个字段完整且各自范围正确后 materialize。

Final 同一个 ac.reg 的 logical type 为 `{kind="record",symbol=...}`；
`ac.logical_element/ac.shape/StaticExpr/InitialSpec` 消除。新 `ac.initial_value`
恰为以下 DictionaryAttr（本文示例字段都是 i8）：

```text
{type = !ac.struct<"demo.pair.Pair">, fields = [3 : i8, 17 : i8]}
```

`type` 是 TypeAttr，精确等于 reg payload type；fields 恰两个 IntegerAttr，
按声明序，各自 type 等于 field physical storage。bool 是 i1 的 0/1；integer
按其 signed/unsigned interpretation 解释 canonical bits 后检查 lower≤v<upper。
不接受 untyped 数字、MathInt、StaticValue 的 kind/symbol 包装、source expression、
null、缺项、额外项、record TypeAttr 不同、按 constructor 参数顺序排列的字段。
这个精确容器将 R1 的完整 payload reset 具体化，不声称 DictionaryAttr 是 TypedAttr。

StateID 仍 `{owner,declaration,element=[]}`；field 没有独立 StateID、element
ordinal、clock/reset 或写许可。one-record next 是一个 aggregate D 和一个 E，
rule/module yield 仍一组交错 `(D,E)`，无两个 field output slots。所有叶同一
reset/enable；无部分字段写、额外 latency、Q 副本或隐藏持续状态。

无 writer/无 active contribution 时，最终 D 必须是合法物理类型的 zero-bit
placeholder，而不是未经选择的 source candidate。§6.4 对 rule 和 module 两层
分别冻结其 synthetic create/selector SSA、使用限制及 E 的关系。这个物理
placeholder 的零可以在字段 logical range 外，因为它不能被观察或提交；它
不是合法 source value/default/reset，也不产生 value.binding/RequiredRecord。
reset image 始终必须在完整 logical range 内，不能使用 placeholder 例外。

## 6. Record 值义务与 actual SSA 的共同验证

### 6.1 为什么需要一个新属性

仅放宽 value.binding/use 的 operand type 不足：两个同型 struct.create 输入
交换、struct.get 改为同型的另一个字段，仍可具有相同 nominal/type/ValueID。
现有 ac.input_bindings 只把一个 rule current argument 绑定到 StateRef，
ac.source.read 只保留一次实际读取并在 final 消除；两者没有保存 create 的有序
field ValueID 或 get 的原 field ordinal。基线源码与已装入的当前合同没有
`ac.reads`/`ac.required_reads` 字段；不得把旧草案中同名建议当作已批准的
installed carrier。ac.required_uses 只保存最终值与 next/helper-return 目标，
不描述 record 内部组装。numeric proof 的算子集只有 scalar math，没有 record
construct/project；扩大其算子/result domain 会混合数学与nominal语义。
单独保留 StructCreate operands 是最终实现，无法独立对照交换前的 source
字段要求。新属性因此只填补这三个源关系，复用现有 ValueID/StateRef，不复制
numeric DAG、definition表、effects或运行调度；不做常量/算术/控制解释器。
因此在 verified helper inline 后、record/source-read/numeric lowering 之前，
从真实 source reads/create/get 抽取下列不可执行义务；不能由最终 yield、生成
C++ 或 renderer 倒推。它不替代 C2 数字、checks 或 target/use 证明。

唯一新增 `ac.required_records:Array<RequiredRecord>` 在已展开 linked rule 与
final rule 上使用。无 record 行为的旧 scalar rule 必须 **没有此属性**；
有 record binding 或 source-linked struct.create/get 的 rule 必须有非空、规范排序数组；
§6.4 的 synthetic placeholders/selectors 本身不创建源义务。
存在空数组、错误落点、未知字段/variant、重复 ID 拒绝。不加 mode/profile flag。

三种闭合 variant：

```text
{kind="read", id:ValueID, record:FlatSymbolRefAttr, state:StateRef}
{kind="get", id:ValueID, record:FlatSymbolRefAttr, base:ValueID, field:u32}
{kind="create", id:ValueID, record:FlatSymbolRefAttr, fields:Array<ValueID>}
```

record 总是 canonical symbol。field 只能 0/1；fields 恰两个 ValueID。
数组按现有 ValueID/Occurrence 结构比较排序，不按 SSA 打印名或调度顺序。
引用图可前向引用，但必须无环；全部 ValueID 在本 rule 的 empty-SpecKey
ProofScope 中解析到唯一 value.binding。也必须检查结构排序与引用图的完整性，
不能通过跳过未使用项逃避验证。此属性不进入 source interface 或 receipt。

### 6.2 逐项 actual SSA 条件

Final rule 内 **source-linked** create 的 inherent properties 为空，operands 恰为两个声明序
field values，result 恰为一个对应 StructType；get 的 inherent properties
恰为 `field:StringAttr`，一个对应 StructType operand、一个所选scalar result。
二者无region，普通attributes恰为 `ac.origin`；多余属性/结果/region拒绝。
rule/module 内 synthetic zero/selection create、get 与 scalar selector 只按 §6.4 接纳；module内
其他record计算拒绝。不得仅凭没有ac.origin就把任意create当作placeholder。

- **read**：id binding 的 domain 是本 record；实际 value 必须是该 rule 对
  state 的精确 current block argument，沿 ac.input_bindings、实际 reg handle
  和 owner 重建。不接受另一个同型 reg 的 Q、其他 rule proposal 或 child 私态。
  其 validity 为 true；path 是源求值入口的现有 threaded path。
- **get**：base binding 是该 record 类型；id binding.value 是
  `ac.struct.get(base.value)` 的实际 result，get.field 精确等于 declaration
  `fields[field].name`；结果 domain/type 等于此字段，valid 等于 base.valid。
  不允许只验证字段名合法却不核对 ordinal、base SSA 和 nominal。
- **create**：id binding.value 是此 nominal 的 ac.struct.create；两个实际
  operands 按 field order 分别等于两个 field ValueID binding.value；每个
  binding.domain/type 等于该字段，field value 的 validity/range 由 scalar
  constant、get 或现有 numeric proof 证明。aggregate valid 为两个 field valid
  的合取，使用已验证的源 live path；不靠“types 相同”跳过字段对应。

helper parameter/local candidate 重绑定只引用已有 ValueID，不制造 state read。
helper single return 的 `RequiredUse={id,value,target={kind="helper_return",
call:Occurrence,ordinal=0:u32}}` 与一个 value.use 一一对应。**只有实际 value、
valid 分别等于被返回 binding.value、binding.valid；return use.path 不要求等于
binding.path。** return use.path 必须是通过该 helper 中实际 source-order
assert/range/operand 检查图验证的出口 live path `P_exit`。返回一个先前读取的
record 不会重置该路径，也不得伪造新的 state read 来让两种 path 相等。

令 `P_call` 为关联 call occurrence 的 caller 已验证求值入口 path，`V_return`
为被返回 binding.valid；从实际 return marker 得到 `R=P_exit && V_return`。
此单 return 子集的 call-expression 求值 validity 为
`V_call = (!P_call) || R`，caller 的下一 live path 必须是
`L_after = L_before && ((!P_call) || V_call)`；P_exit 必须受 P_call 支配。
未调用时不引入失败；调用后 assert 失败时 P_exit=false，因此 caller 后续
actual/表达式/observation 被抑制，即使原 record 的 binding.valid 仍为 true。

P_call、P_exit、R/V_call/L_after 都必须从 retained actual boolean SSA、关联
call/return occurrence 与现有 check/threaded-path 义务重建验证；不能以一个
新metadata“成功”值、返回data的原binding.path或output E代替。允许现有证明
规则下的等价boolean简化，但必须验证同一方程。这里没有新增IR字段。
caller 后续使用同一已返回值和其已有ValueID，由新的caller live path控制
其使用；不能回读current代替，也不把 V_call 偷写成原 binding.valid。
下一层 constructor 返回同样保留其独立 helper_return（不同 call occurrence）。
不存在新的 alias opcode、record return wrapper 或 `ac.results` helper 属性。

rule 中每个 record-typed binding 必须由 read/create 义务解释；每个 struct.get
产生的 scalar binding 必须有 get 义务；同一 producer 的重复 use 复用 binding。
此首包不做跨不同 source occurrence 的 record create/get CSE，也不删除仍用于
proof/helper-return/check/observation 的 producer。scalar 优化仍限于原证明允许集。
对纯 dead locals 可在义务提取前消除无 demanded error 的整个表达式；提取后
不能留下孤立义务或以 DCE 删除 demanded check。

所有 surviving **source-linked** record operations 恰与相应义务一一对应，`ac.origin` 等于其
id.origin；reads 的 source op 在 final 消除，由 read 义务绑定到 block argument。
synthetic zero/selection create、get、select只受 §6.4 的exact-use closure约束，不能冒充source binding。
每个义务必须到达 next、helper_return、scalar numeric/check 或 scalar observation，
不容许孤立“证明”绕过实际 dataflow。每个 scalar binding 也必须由既有 scalar
证据或 get 义务解释，不能通过加入 record 属性免验 scalar computation。

### 6.3 与 numeric、target 和控制的组合

对于含 required_records 的 rule，统一 verifier 核验完整 record、numeric、
check、use、observation DAG；不能选择旧 generic verifier 后跳过 numeric，
也不能因存在 required_numeric 跳过 aggregate use。旧 scalar-only validators
保持原有闭合和拒绝责任。模式由真实义务/operation 集决定，不新增公共 flag。

NumericProof 的 inputs/result 仍只能 scalar；get 结果可作为 input，必须匹配
被验证的 get binding。`current.lo+1` 先从 lo 的 domain 计算数学范围，扩展到
足够中间位宽，再生成现有 range check/to_bits；不能先以 i8 加法回绕。
NumericNode/CheckBinding/RequiredCheck 的字段、op 集和真实 operand 验证不变。

`UseTarget.kind="next_scalar"` 沿 C2 包含一个 whole-record state；它不是
“integer only”的新标签。target 仍精确对应原 StateRef，source ValueID 是
aggregate binding。YieldBinding 保持 data_operand=2j、enable_operand=2j+1，
selection_ordinal=unit。helper_return 不占据 next/yield slot。
每项 source contribution 的数据 `D_i` 等于 source binding.value，
`E_i = value.use.path && value.use.valid`；**最终 yield data 不是无条件等于
D_i**，而是 §6.4 的verified aggregate merge，yield enable 等于 `OR(E_i)`。
两 field 的部分 validity 不产生两个独立写 enable。guard=false 不掩盖之前已
被求值的 helper/range/assert 错误；error 使全树本拍无任何 DriveNext。

源 source-order 与 field order 分开：按 Python 左到右评估 actuals，包括 kwargs；
在绑定后才按 parameter order传递，再按声明 field order组装。每个有可能失败
的表达式均沿 C2 `live_after=live_before && (!P || V)` 传递 live path。
construct 的最终 path 是实参求值后可进入 constructor 的 path；当 path/valid
为 false，typed placeholder 可以传播但不能观察或提交。verifier 必须核对其
实际 boolean SSA 方程，不能把 expected path 只存进新表即视为证明。

### 6.4 Rule/module aggregate selector 与无 active contribution

C2 的一般合同保持：对同一target，每个已保留next use贡献`D_i/E_i`，最终
`E=OR(E_i)`，D由相同E_i选择对应D_i；没有active项时D为全零物理placeholder。
多个可能同时active项必须有静态互斥证明或既有precommit overlap check，
没有证明/检查不能按selector顺序选winner。本包不交付record多contribution/
多writer：rule的record YieldBinding.contributions恰一项，module每个record
StateID至多一个rule driver，超过此数不论是否可互斥都capability-reject。
一般C2义务没有取消；以后扩大该子集须另闭合其实际merge/overlap carrier。

**Rule单项情况（n=1）：** 从唯一RequiredUse和ValueUse取得D1/V1/P1，common
verifier要求`E1=arith.andi(P1,V1)`（现有等价AND operand交换可接受），
`E=OR(E1)=E1`，不要求多造OR-with-false。rule yield的enable必须是这个E，
而data必须是以下normalized scalar-leaf selectors与aggregate create的result，
即使当前E能常量证明true，也不在本包优化中改成direct-D而绕过这一路验证：

```text
%f0 = ac.struct.get %D1 field=declaration.fields[0].name : record -> T0
%f1 = ac.struct.get %D1 field=declaration.fields[1].name : record -> T1
%z0 = arith.constant 0 : T0
%z1 = arith.constant 0 : T1
%s0 = arith.select %E, %f0, %z0 : T0
%s1 = arith.select %E, %f1, %z1 : T1
%selected = ac.struct.create(%s0, %s1) : (T0,T1) -> record
ac.yield ..., %selected, %E, ...
```

T0/T1恰是declaration两个field的physical signless integer type（bool也为i1）。
上面是SSA关系notation，不新增属性名`field=declaration...`；真实get仍只有
原`field:StringAttr` property，其值必须等于指定ordinal的字段名。final不含
scf.*或index；source/helper里的临时scf必须先归一化成上述已验证shape。
不提出final scf profile例外，不让backend各自lower一个未知aggregate selector。

每个synthetic get有一个D1 record operand、一个Ti result，inherent properties
恰为field，普通attributes为空；每个arith.select有相同的一个E:i1 condition、
对应fi true-value、zi false-value、一个Ti result，无其它属性/region。fi的
唯一使用是本ordinal select的true operand，zi唯一使用是同一select的false
operand，si唯一使用是selected create的第i个operand。selected create无
inherent properties/普通attributes/region，result为target nominal record，
其唯一使用是本rule yield的精确data_operand。E同时是两个select condition和
相应yield enable；不能交换字段、true/false operands或借另一个target的D/E。
既有yield_bindings仍指向源use，不改成synthetic ValueID或重复output target。

**Module单driver情况：** 从实际record target的唯一rule结果取得(D_rule,E_rule)，
先验证该rule内部上述closure；module重建相同的两个get/select/create shape，
get bases恰为D_rule、两个select conditions和commit-yield enable恰为E_rule。
不得跳过rule而直接拿source binding/value，或借另一个target/rule的同型result。
module无writer（n=0）则E为false i1 constant、D为两个zero-bit scalar constants
直接组成的synthetic ac.struct.create，无get/select。没有source next use的rule
不虚构output slot。module-level create结果只用于该StateID最终commit pair；
无record child-port来源或隐藏driver。

两层zero leaf constants的类型恰Ti、bits全零、唯一普通attribute为value。
对n=1只作为相应scalar select的false operand；对n=0只作为本target zero
create的对应operand。n=0 create也无inherent properties/普通attributes/region，
result精确是target record type，唯一使用是E=false的module commit data。
这些synthetic零值、get、select、create不含ac.origin，不加RequiredRecord/
ValueID：它们是原RequiredUse或no-writer closure授权的commit-selection实现，
不声称用户构造/读取了它们。保留compiler可用的MLIR诊断loc，不制造source
Occurrence。source-linked create/get的origin/RequiredRecord规则继续适用。

禁止把synthetic节点用于source get/create、value.binding/use、numeric proof、
helper_return、observation、别的target、reset或上述exact-use集合之外的任何
使用；选出的aggregate只沿规定yield/实际rule-result到module commit继续流动。
common verifier从每个target的retained YieldBinding/actual rule outputs向后
重建整个shape，再独立清点全部record ops，将其归类为source义务或synthetic
闭合；orphan、额外use、错误placement均拒绝。不能仅凭缺ac.origin就接纳节点。
此首包不对synthetic nodes做跨target或与source producers的CSE。两层由同一
MLIR/final pass构建、fresh reader核验；backend只消费同一已验证normalized图。

某字段的logical domain可能不含0；synthetic zero不用source range证明，因为
它只在E_i=false时被scalar select选入D，两个leaf共享同一E且commit enable
必false，无observable use。若改坏E/selector使零可能提交，或将节点接到
observation，common final verify必须拒绝；不能等runtime执行后修补。reset
仍必须检查完整logical range。全树其它source/check失败在任何DriveNext前
阻止所有commit并discard scratch，不通过修改本target E隐式吞掉错误。

## 7. Inline 来源、检查与 fresh-final 信任边界

保留每个 ValueID、UseID、NumericNode、RequiredCheck 的原 leaf Site 与按外到内
顺序排列的 call ExpansionFrame。首次 frame.site.definition 是 owning module；
后续 frame.site.definition 等于前一 frame.callee；最后 callee 等于 leaf
site.definition。无 expansion 的值/check 仍属于本 module。此首包不接纳
iteration frame；集合/静态循环保留现有缺口，不通过 helper 扩展打开。

每个 leaf/frame definition 必须属于显式 supplied final units 中唯一 SourceOwner
的 canonical source-module namespace：普通 helper 的剩余后缀是一个 Python
identifier；constructor 的剩余后缀必须是已投影 record 的 `name.__init__`。
module/record 已有定义按精确符号查找。若 owner 歧义或不存在，拒绝；不得通过
任意字符串前缀、文件名、当前工作目录或读取 Python 重新猜 owner。
SourceSpan.path 以及原 FileLineColLoc 中的 leaf path 必须匹配所解析 leaf owner，
而不强制等于调用 module 的 path。span 与 loc 不是同一字段；仍分别校验。

helper bodies 在 final 不存在，因此此查验只能证明已保存的 canonical owner/
call-chain/obligation/actual SSA 自洽，不能重新认证原 helper 的 AST 或代码。
producer 在删除 helper 前独立校验签名、body、constructor完整性和捕获清单；
FinalProgram 的 frozen snapshots 同时覆盖全部投影声明、reset image、record
义务、operations/SSA/controls、numeric/use/check evidence，修改即失效。
独立 fresh reader 只读 final，并执行同一 schema、owner、DAG 和 SSA 验证。

保持 C2 的全部 RequiredChecks；特别是 helper 中生成的 range/assert 不能因
没有输出、E=false、inline/DCE 或两个 backend 共同遗漏而消失。失败停止本次
step、无普通成功 observation 发布、全树保持 Q，须完整 Reset 后复用。
这里扩展 current reader 的 helper provenance 接纳，不改变 expect 的两个 i1
operand、kind/location、无 result/region，也不批准 SYSTEM/EXPECT B 的其余内容。

不新增认证哈希。篡改一个 SSA field、type、use 或路径而保留冻结义务必须拒绝；
一致重写全部语义、义务、来源且形成另一合法 final 不保证被检测。
对 fresh final 删除一个完全未使用 record/空声明源同样没有原源码完备性凭证；
这由 producer inventory/frozen-object 检查保证，不能声称 fresh reader能认证原项目。

## 8. 从同一 final 派生共同硬件值视图

共同 MLIR/final verifier 先完成 §4–7，才建立不可独立编辑的派生视图。
它不是额外文件或 public schema，不能从 backend metadata 恢复 authority。
对 scalar 延用当前 leaf；对 flat record 由 declaration 的两个字段得到：

```text
leaf ordinal 0: field[0], logical scalar type, width N0, bit offset 0
leaf ordinal 1: field[1], logical scalar type, width N1, bit offset N0
aggregate bit width: N0 + N1
```

offset/width 由类型唯一推导，不增加可覆盖 layout attribute。每个叶携带确切
scalar SSA expression、valid/path、reset bits 和已有数值 proof 关联；整 record
继续携带唯一 StateID/owner/aggregate D/E。read 映射同一 Q 的两个字段；get
选择已验证 ordinal；create 按 fields 组装两个已验证值；helper/local值不分配存储。
布局由 verifier的 declaration/type/SSA重建，CPP/RTL 使用同一个 frozen view。

一条 record reg 的 storage inventory 为：**1 个 architectural/owned reg，
1 个 StateID、1 个 D/E/Reset/Xfer 权限、N0+N1 个数据位、2 个 scalar payload叶**。
不能拿叶数量宣称多出两个 reg owner，也不能把 aggregate 算成一个 scalar bit。
DFFE/history 的字段不获得新的 domain/clock，也不使用 collection element=[0/1]。

C++：复用既有 `::gfsim::SimDFFE<::std::array<::std::uint64_t, 2>>`，恰一个
实例；D/proposal scratch 是同类型普通 array，无持久共享副本。每叶保存规范
零扩展 N-bit bit pattern；signedness 由现有 numeric lowering 显式处理，
不能对 array 元素套用 unsigned native arithmetic 代替数学语义。整体一次
Write(D,E)/Xfer，Reset 和 DiscardNext 沿现有 runtime；不修改 SimDFFE API
或增加 runtime wrapper。必要 `<array>` 是标准库，不是新依赖。

RTL：同一 record state 使用宽 N0+N1 的 payload，field0 为低 N0 位，field1
为后续 N1 位；一个同步高有效 reset 和一个 E 控制整 payload。reset 常量、
get/concat/select 的 offsets 只能来自同一 verified view，不能用 Verilog
struct默认布局推断顺序。bit slicing 是已有后端私有合法化，不新增公开 IR op。
两端都不执行 helper、不从 header回读语义、不引入 BigInt runtime。
§6.4的两个scalar arith.select按同一个condition选择record叶；不得给各叶
推导不同enable或把zero fallback改为旧Q。最终commit E=false才使Q hold。

模型 ABI、runner、统计/event schema 保持；record不能作为新的 C ABI参数。
仅用现有 scalar projections观测。原标量 StateCarrierSnapshot::width-only 消费者、
initializer TypedAttr cast、expression emitters、source parts 和 snapshot比较
必须一起适配这个派生视图；不能只让一个 backend 特判 ac.struct。

## 9. Source-owned C++ record declaration

根据 final owning record生成原 SourceOwner的 header；例如 `demo/pair.py`
对应 `sources/demo/pair.hpp`，沿 C3 legalization生成：

```cpp
namespace demo::pair {
struct pair {
  ::std::uint64_t lo;
  ::std::uint64_t hi;
};
}
```

每个成员按原 field order。bool 映射 bool；unsigned integer（含 integer i1）
映射 `::std::uint64_t`；signed integer 映射 `::std::int64_t`，与 C2-DECL scalar
值载体规则一致。直接使用显式 scalar type，不依赖跨源 alias include。
不生成 constructor、默认成员值、方法、继承、operator、hidden fields或
range-checked wrapper。`pair{}` 的 C++零初始化不是 Python `Pair()` 的 defaults。
record不同 canonical symbol生成不同 C++ struct，不能去重为structural typedef。

这是可 include 的普通**值声明**，不是模型 I/O ABI、hardware packed layout、
Python constructor执行器或 runtime state类型。不要承诺 sizeof/offsetof/padding
跨平台相同，不用 memcpy 将它作为 RTL数据；§8的 internal array bit carrier
独立于 native signed成员/填充。C++调用者普通运算不成为 source数学语义权威。

record/field名复用现有legalization，统一检查namespace/alias/constant/module/
record名、header路径和字段scope碰撞。两个合法Python字段合法化后同名必须
C++ target拒绝；成员合法化后与struct自身名字相同也拒绝（C++成员名冲突），
不加后缀或hash。fieldscope不因另一record的同名field冲突；不把全局std/gfsim
禁用规则错误施加到普通成员。依赖引用一律 `::std::`/`::gfsim::`，保留现有
namespace遮蔽正例；全局support/glue占用规则同C2-DECL。

record-only源只有hpp，无假cpp/object/module；implementation-owned record
进入该源已有hpp。helper-only/facade空header仍归本源；facade不复制provider
record到自己的namespace。跨源使用internal hardware record并不强迫包含整个
consumer/source闭包；仅导出的实际C++引用决定include依赖。两TU单独include/link
证明ODR与自包含。所有C++ renderer共用结构化声明输出，不从字符串切分whole design。
C3 generated.json原files/source_groups登记文件，无新类别/字段/额外receipt。

## 10. 调用者、拒绝、删除与回退

必须迁移的内部责任（路径是范围索引，不是当前修改授权）：

| 责任 | 基线路径与变化 |
| --- | --- |
| capture/import/header | PythonImportRecords/Helpers/RuleExpressions/ModuleAnalysis 与 SourceHeaderRegistry；接纳本包带参helper与record rule，不执行Python，不放宽不相关syntax |
| shared ODS/verifier | ACIROps.td仅两value operands；ACIRPacketOps/SourceContracts/FinalDeclarations/FinalContracts/FinalUses及numeric closure；完整新reset/required_records/placement检查 |
| link/materialize/reparse | FinalDeclarations/FinalProgram/FinalHardware/FinalHardwareProgram/FinalNumeric/FinalUses；从真实source提取、inline、投影、闭合、冻结与fresh reconstruction |
| dual emit/source groups | FinalEmitCpp/Verilog、expression emitters、FinalCppDeclarations/Emission/SourceParts；同一payload view、名字/文件清单、无两套语义 |
| observability/source map | scalar get绑定进入现有ObservationGraph；C3-SM保留record op/ac.origin，与两backend一致；不新增逐行映射承诺 |
| docs/profile/gates | 验收后PM更新language/M5范围与Decision 0283的扩展状态；实现者不得先改supported声明 |

替换当前无条件record/helper preflight，成为严格本包subset preflight；支持的
helper必须inline完且检查义务完整才删除，其余拒绝。删除对应已批准范围的
scalar-only假设，不删除原scalar负例、source-only元数据清理、header authority
或bad-output保护。旧type/op/path没有compatibility模式，也不恢复module class、
CAS、QueueGraph、old compiler、手写Interface或任何SYSTEM B表示。

拒绝阶段必须可区分：source绑定/类型/不支持语法，link authority/unsupported
projection，common final malformed reset/nominal/义务/SSA，C++名字或能力问题。
沿用非零退出与现有结构化诊断通道，包含source owner/symbol和可得的源位置；
不新增public错误码/忽略flag。所有emit先完整verify；cpp-only命名问题不会让RTL
解析同样合法的final时失败。任何失败不发布新partial output，replace保持旧bytes。

回退边界是同一实现候选的ODS/verifiers/import/link/reparse/两backend/测试整包，
不能只去掉拒绝或留下一个backend。原有已接受scalar artifacts继续满足旧形状；
这不是新增schema版本兼容分支。新record artifacts只能由匹配本合同的toolchain
读取，不增加IR版本字段或自动降级器。

## 11. 独立oracle与可执行验收矩阵

以下全为planned。独立测试作者按合同/手算结果写oracle，不能从生成器、receipt
或两backend相互比较生成expected。文档approval-ready不等于产品PASS。

| ID | 必须执行的正/反例与独立观测 |
| --- | --- |
| R01 source/header | 三个源独立compile；pair body/Python移走后advance与top仅header编译；恢复完整已发布units再link。defaults、反序kwargs、不同constructor/field顺序、missing/duplicate/unknown/extra参数；相同shape不同nominal拒绝 |
| R02 final projection | final保留全部owning records（含unused/private/implementation-owned），helper-only/facade/空源一个unit；constructor provenance精确且无func/call/snapshot残留；整个声明/owner inventory独立比较 |
| R03 state oracle | reset `(3,17)`；Work不改Q，proposal `(17,4)`；Xfer得 `(17,4)`；第二拍proposal/commit `(4,18)`；Reset清scratch/replay相同。logical reg=1、StateID=1、payload bits=16、无field owners；每端分别匹配literal值 |
| R04 current/local | 同拍state赋值后state.lo仍3，candidate.lo为17；下一拍分别17/4；共享helper两次调用不串identity；仅local record不增加storage |
| R05 controls | guard false时检查rule与module selector的D均为zero-bit placeholder、E=false且Q hold；guard true时D为同一source candidate、E=true；下一拍无disabled proposal复活。字段domain不含0的case仍只在inactive synthetic分支允许zero。Reset优先；mixed scalar/record另一rule失败时全树零提交；复位重跑 |
| R06 failure paths | 必测helper field range越界与assert。新增assert-then-return原record参数，binding.path早于assert而return use.path在assert后；caller后续独立range-unsafe actual/observation在失败时不得求值/发布，next不得提交。guard false则helper不求值；检查retained实际path方程。除零只在派发前有当前scalar verifier与现有执行test ID支撑时加测，否则标不适用，不强迫扩scalar算子 |
| R07 reset/type | field交换、wrong type/arity/nominal、signed负值、bool vs integer i1、窄位边界/u64、aggregate>64位正例；有类型但超range reset拒绝；全zero placeholder只在未提交路径被接纳 |
| R08 proof tamper | 保存合法final，分别交换同型create operands、get.field、record read base/state、ValueID、RequiredRecord field/ID/variant、helper_return call/exit path、yield target/data/enable、numeric input/check path；另改selector condition/true-false operands/foreign-target D、增添region或残留scf/index、zero叶bits或type、加placeholder observation/get/binding/use、给synthetic zero伪造origin、删应有selector，三个入口均拒绝并保留输出 |
| R09 origins | 跨文件helper range/assert保留leaf path/call frames；破坏first/inner/last frame、foreign/ambiguous owner、span path、来源落点拒绝；scalar module-local旧检查仍通过 |
| R10 fresh-only | 保存final后使Python/body/header/dep不可用，新进程两个emit均成功并真实build/run；缺record声明/constructor provenance篡改、放错unit/region、额外字段、helper残留两端拒绝；frozen mutation检查覆盖所有投影/reset/SSA/义务 |
| R11 C++ headers | record-only无cpp，2 TU include/link；字段顺序、成员scalar type、nominal不同型、aggregate初始化值独立断言；不要断言padding为RTL布局；record/field/namespace/模块/Unicode/legalization碰撞与Std/Gfsim遮蔽、旧输出保持 |
| R12 bounded/hard break | nested/非两字段record、record/list alias/constant、record data ports、helper recursion/value-helper互调、stateful helper、动态属性/field-next、nonempty static及record多contribution/多rule writer均明确拒绝。两个本地record值alias不复制storage，不同owned declarations独立；record child-port forwarding留待后包且本包拒绝，不扩大范围迁就oracle。保留Bank2/4 UNRUN及child-static拒绝，SYSTEM/EXPECT B不实施 |
| R13 packaging/regression | source→ac/header→final→source-owned TU/CMake和maps清单一致；CPP/RTL分别从同一final；现有scalar/namespace/publication/runner gates无退化；不是全平台/SDK/release认证 |

R07换字段reset若新值仍在合法范围，fresh verifier不能知道原Python值；必须
在source→final projection comparison或frozen-object mutation上拒绝。fresh
negative用错类型/超range/名义错误；不要求fresh reader检测自洽的新合法设计。
R08的swap只改actual SSA而保留原obligation；同时改全部自洽义务不在认证承诺内。

## 12. 实施分片与门槛命令

批准前只做设计和独立oracle文档。批准后PM绑定checkout/HEAD/dirty overlay、
批准hash、文件owner、工具路径与输出目录，按以下依赖序派发：

- [ ] D1 shared schema/verifiers：先覆盖声明、reset、required_records、finite-value
  signature的结构正反例；共享ODS/CMake/公共header只有一个integration owner。
- [ ] D2 source/header与MLIR展开：完整helper签名/绑定、record field/current义务，
  source-order/path/check提取；不从backend补语义。独立测试实例先给R01/R06反例。
- [ ] D3 final投影/重解析：authority/inventory、constructor provenance、record/reset/
  use/数字统一闭合、冻结与fresh-only reader；D1–D3未闭合不允许emitter接纳。
- [ ] D4 共同payload view和两backend：从同一个verified final逐leaf映射，runtime
  保持，source-owned C++声明/名字检查/文件发布一起做。
- [ ] D5 独立tests跑R01–R13，Sol review绑定最终bytes，PM验收后才更新supported
  profile。D1声明测试通过不能冒称stateful E01已交付。

计划新增文件：`tests/system/test_final_record_declarations.py` 与
`tests/system/test_record_value_backends.py`；native聚焦cases可加入现有
`ACIRFinalProgramTests`，不必新建测试target。这些文件目前不是已有PASS证据。
以下命令由测试包在批准后展开到**本checkout重建**的候选工具；不复用历史
`.pycircuit_out/w10-pm`或其他worktree prefix来制造PASS。

```sh
: "${PYC_E01_BUILD:?candidate build directory}"
: "${PYC_E01_INSTALL:?candidate install prefix}"
: "${LLVM_DIR:?LLVM 22.1.8 config directory}"
: "${MLIR_DIR:?MLIR 22.1.8 config directory}"
cmake -S . -B "$PYC_E01_BUILD" -G Ninja \
  -DPYC_BUILD_COMPILER_DEV=ON -DPYC_BUILD_TESTING=ON \
  -DPYC_BUILD_RUNTIME_LIB=ON -DLLVM_DIR="$LLVM_DIR" -DMLIR_DIR="$MLIR_DIR" \
  -DCMAKE_INSTALL_PREFIX="$PYC_E01_INSTALL"
cmake --build "$PYC_E01_BUILD" -j 4
cmake --install "$PYC_E01_BUILD"
export ACIR_SOURCE_UNIT_HARNESS="$PYC_E01_INSTALL/bin/acir-source-unit-harness"
export ACIR_DESIGN_HARNESS="$PYC_E01_INSTALL/bin/acir-design-harness"
export ACIR_CPP_SOURCE_PARTS_HARNESS="$PYC_E01_INSTALL/bin/acir-cpp-source-parts-harness"
export ACIR_BACKEND_CLOSURE_HARNESS="$PYC_E01_BUILD/bin/acir-backend-closure-harness"
export PYCIRCUIT_TEST_DRIVER="$PYC_E01_INSTALL/bin/pycircuit"
ctest --test-dir "$PYC_E01_BUILD" -N \
  -R '^(ACIRSourceUnitTests|ACIRRegContractsTests|ACIRFinalProgramTests|ACIRExecutableBackendClosureTests)$'
ctest --test-dir "$PYC_E01_BUILD" --no-tests=error --output-on-failure \
  -R '^(ACIRSourceUnitTests|ACIRRegContractsTests|ACIRFinalProgramTests|ACIRExecutableBackendClosureTests)$'
python -m pytest tests/system/test_final_record_declarations.py \
  tests/system/test_record_value_backends.py --collect-only -q
python -m pytest tests/system/test_final_record_declarations.py \
  tests/system/test_record_value_backends.py tests/system/test_source_unit_packet.py \
  tests/system/test_source_namespace_bindings.py tests/system/test_final_scalar_declarations.py \
  tests/system/test_generic_multi_assignment.py tests/system/test_generic_assignment_roundtrip.py \
  tests/system/test_driver_compile_link.py -q --junitxml="$PYC_E01_BUILD/e01.xml"
```

独立tests必须实际运行安装driver的compile/link/two emits、生成CPP/RTL的CMake
构建与有限runner，记录CMake/Ninja命令、退出码与非空assertion/oracle结果；仅
CTest discovery、pytest collection、generated text grep、mock emitter不能关门。
测试私有Work/Proposal分阶段检查可用既有native gate seam，不新增public ABI。
需要发布故障/源map现有回归时PM按触及代码扩测，不把全M6/M7变成本包前置条件。

完成标准：精确proposal通过独立Astra审阅并获用户批准后，D1–D5与R01–R13对
最终候选全部有独立执行证据；任一必要skip/xfail/未建立oracle明确保留open。
当前文档仅达到可送独立审阅的设计候选，不声称实现、build或上述任何gate通过。
