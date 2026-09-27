# C2-L1：current 读取来源的源级载体

修订 A。状态：待独立设计审阅与用户对精确文本的批准。设计依据是
u02c_link_design 的只读架构分析；PM 将其整理为候选合同。
C1-C、C2-C、C2-N1-C、C3-C 及另行待批的 C2-A2 原文均不由本文
改写。本文尚不授权实现。

## 缺口与范围

C2 的 `ElementEffect` 含按结构排序的 `origins`，要求 link 从真实
body 重算 generic effects 再比较 owning header 和消费 snapshot。
现有 `ac.rule` 的 `ac.input_bindings` 和 `ac.input_types` 足以重算
读写的 state 集合，却只保存每个 current 输入一次，不含方法中
各次直接读取的 `Site`。`ac.math.from_bits` 不覆盖 bool 直传和
完整 record，`ac.struct.get` 也没有 source occurrence；MLIR `loc`
仅用于诊断，不能恢复 AST path。A2 的 `ac.source.use` 是 next 写入/
helper 返回，不能扩义成 current 读。

因此，未增加载体前，完整 header/snapshot 比较加独立 R/W 重算
只能作为阶段性结果，不能称为已经从 body 独立重建完整
`ElementEffect.origins`。本文仅补普通 scalar/whole-record rule 的
current-read 来源。`ac.table.get` 已有读取 occurrence，固定列表
按 C2 自身的 scalarization 规则处理；memory/resource 不由本文扩展。

## 唯一新增 operation

```text
%read = ac.source.read %current
    {ac.origin = Occurrence}
    : T -> T
```

同一 `ac` dialect；一个 operand、一个同型 result、无 region、无
symbol、无附加 target/index/owner 属性。`T` 是被读取 rule 输入
的有限逻辑 payload：`iN` 的 bool/integer 或完整 nominal record。
不得是 DFFE/resource handle、`!ac.math_int`、`!ac.local`、index、
source validity/path/control 或未批准的聚合载体。唯一值方程为
`result = current`。该操作不读取 next、不写 Q、不分配状态、
不改变拍次、不失败；也不为后端新增可观察操作。

`ac.origin` 必须是 C2 的闭合 `Occurrence`；未展开时
`Site.definition` 等于 enclosing module 的 canonical definition，
`ast_path` 相对该 module class 定义。后续静态展开按 C2 追加
iteration frames，rule 注册 occurrence 与读取 occurrence 分立。
MLIR `loc` 保留源诊断位置，不参与 identity 或调度顺序。

operation 只出现在 source/linked semantic 阶段已注册
`ac.rule` 的 computation region，或该 region 内经验证的结构
控制子区域；readiness、conditions、admission、helper、constructor、
module topology、builtin module 顶层均拒绝。不得只凭外层
`ac.stage` 字符串认定语境合法。若 linked 容器/owner 尚未完成
独立验证，本 op 的局部通过不代表整份 linked program 通过。

`%current` 必须是所属 rule computation entry block 的第 `i`
个 current-payload argument，或者合法控制子区域对该精确
argument 的结构化透传；不能来自 `arith`、`ac.struct.get`、
next data、helper call、cast、local merge 或另一个 rule。
从真实 `rule.inputs[i]` 和对应 `ac.input_bindings[i]`、
`ac.input_types[i]` 解析 state/formal handle，核对 enclosing
module 的实际 owned DFFE 或 formal current port、StateRef、方向、
LogicalType 和物理 payload。禁止把同宽另一个 input 调包；
不接受只靠 metadata 声称来源。

## 精确读取事件

每次在 source evaluation path 上直接读取持久成员，或读取仍
对应注册输入的 rule formal，产生一个 marker。formal 被局部
重绑定后是普通局部值，其后使用不再制造 current read。
局部变量缓存后的复用也不重复读取状态：

```python
x = self.count   # current read A
y = x + x       # no new state read
z = self.count   # current read B
```

对 `self.packet.value` 这类直接 field projection，以**完整访问
表达式**的 AST path 作为一次 read origin，marker 的输入/输出仍是
完整 record，然后才用 `ac.struct.get` 取字段。链式 field projection
也只为该完整源访问记一次；`self.packet.value + self.packet.valid`
有两次完整访问。先绑定 `p = self.packet` 后的 `p.value` 是局部
projection，不额外读取 current。constructor 中的 connection alias、
child port 转发以及单纯引用已有 state handle 的静态 registration
不是 runtime current read。

source compiler 按源表达式顺序发 marker，所有由该读取产生的
数据计算都用其 result，不能保留一条 marker 同时绕过它直接
消费 entry argument。静态分析与 IR verifier 禁止其他操作在
calculation 中直接消费 rule current entry argument；合法传入
控制子区域的透传必须最终先经 marker。直接读取被求值但结果
未使用时，marker 仍留到 effect closure，不能被普通 DCE 删除。
相同 state 的两次读取可以共享原 current value，却保留两个
不同 occurrence；同一源 site 因两次 rule registration 展开可在
不同 registration scope 合法出现。

runtime 分支两臂的 marker origins 作保守并集；仅经 C2
StaticExpr 证明不活跃的结构分支可在 specialization 时剔除。
不能用普通 DCE、source 行号或动态路径猜测删除读取来源。
current 读取本身不需要新的 path/valid operands；在路径上是否
被求值由已批准的结构控制和后续 effect analysis 决定。本文不
改变 C2 source error、short-circuit 或 next enable 规则。

## header、link、final 的义务

producer 的 generic read effects 必须从实际 marker 建立，而非
另外一张仅由 AST 汇总的私有表。每个注册 rule 的 marker 经真实
input binding 映射到 constructor connection parameter/ordinal 或
owned state。owned 读取不发布为 connection effect；formal 的
`origin` 加入其 ElementEffect read-origin 集合，排序去重。child
effect 通过实际 `ac.instance` 实参向父级映射，child 转发不
虚构一次父级 current read；写入 origins 仍由 C2/A2 对应真实
next 载体独立推导，本增补只补读取来源。

link 先完成 SourceOwner、N1 provider/name、owning header、body
snapshot 和实例端点 authority，再从每个真实注册 rule 的
source.read 重算 read-origin 集合。`precision="exact"` 要求
read flag 与 origins 均相等；`conservative` 仍只能是获批 C2
允许的上界，不能把缺失 marker 解释成 exact。保持 header
不变而删/改 marker、绕过 marker、改接同型另一输入、改
origin、伪造 header/snapshot origins，均必须拒绝。

该操作不标为可随意 DCE/CSE 的 `Pure`。这是 source provenance
保留义务，不是新硬件副作用。只有 generic/exact effects、
snapshot 比较和实例端点验证完成，才可把 result 替换为 operand
并删除 marker。完整 final verifier、C++ 和 Verilog 两个真实
emit 入口均拒绝残留 source.read；更改 `ac.stage` 标签不能跳过
最终拒绝。source/header 的过期产物须重新编译，不增加 schema
版本兼容或旧 lowering fallback。

## 验证包与实施边界

计划改动 `ACIROps.td`、独立 source-read verifier、
`PythonImportRules` 的 current expression producer、source
effects 汇总、header/body snapshot 比较、U02-C link 和 final/emit
验证器。维持一个 importer、一条 MLIR route；不改公开 Python
语法、CLI、runtime ABI、C++/Verilog 数据语义或资源接口。尚未
批准时，这些代码不得实施，U02-C 只能先验 body/header/N1
及 R/W 集合，不能宣称完整 read origins 闭合。

| Gate | 固定预期与反例 |
| --- | --- |
| op/SSA | parse/print/reparse；实际 current argument、类型、owner、作用域；同宽另一 input、next/local/helper/cast、topology/readiness、伪造 occurrence 拒绝 |
| source/header | 同 state 两次直接读有两个 origins；局部缓存仅一次；bool/完整 record/field projection；未使用 read 保留；未注册方法无 active effect；parent 仅用 owning header 编译 |
| control | runtime 两臂并集；static 选中臂；同方法两次注册分立；两个 child 同定义、不同 OwnerRef 不混淆 |
| link | 删除或移动 marker 但 header 不变、保留 R/W 改 origin、同型 input 交换、伪造 snapshot、移动纯诊断 loc、unit 顺序置换与 SSA alpha rename |
| final/emit | 先以未改合法 final IR 证明两后端可消费，再分别注入残留 marker，cpp/verilog emit 均失败且不发布；replace 时旧输出保持不变 |

测试须记录选定当前 checkout 的 source/binary SHA、发现数、执行数、
零必需 skip、独立固定 expected Site 列表、变异输入及旧产物
hash。source/link 子集通过不算 final/emit 证据；尚未实现的
gate 明确标为计划，不写成通过。完整 C2 的 list/table/read,
numeric proof、A2 source use、U03 final、双后端和 SSM ELF 继续
在原迁移账本中开放。

若独立设计审阅修改本文语义，PM 先修订并重新送审，再向用户
请求该**精确修订**的批准；批准前不改产品接口。
