# C2-L1：current 读取来源的源级载体

修订 B。状态：修订 A 独立审阅要求四项修补；待复审与用户对
精确文本的批准。设计依据是
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
LogicalType 和物理 payload。当前 operand 与其声明的 binding
不一致，或指向 next、其他 rule、未声明 handle 时拒绝；不接受
只靠 metadata 声称来源。

已通过结构和 authority 检查的 body 是 executable source-read
语义权威。若把合法 marker 的 operand 从同型 owned `%a` 改为
同型 owned `%b`，且新 body 仍满足全部已序列化约束，仅凭本 op
与不含 owned-read origins 的 connection header，无法还原原
Python 意图并一律拒绝。此类语义变化须由可信 source producer
或 C2 规定的受验证 rewrite 保证。link 独立校验结构、真实 SSA
归属、公开 effect 和 header authority，不声称从缺失的 Python
源码重演整个程序。

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

现有 `ac.math.from_bits` 来源 verifier 只接纳 rule entry
argument；本增补实施时必须同步允许沿**已验证的**
`ac.source.read` result 回溯至同一个 current argument，并核对
完整 LogicalType。record integer field 还须沿 marker result 后的
真实 `ac.struct.get`，从 authoritative nominal field declaration
取得域。通用 cast、同宽任意 SSA 或伪造透传不获批准；
`range(2)` integer、signed i1 与 source bool 仍严格区分。
helper 数据参数的既有独立来源规则不变。

## header、link、final 的义务

producer 的 generic read effects 必须从实际 marker 建立，而非
另外一张仅由 AST 汇总的私有表。每个注册 rule 的 marker 经真实
input binding 映射到 constructor connection parameter/ordinal 或
owned state。owned 读取不发布为 connection effect。每个
connection element 的 read/write flags 分别从实际 read/write
事件计算；内部暂存 R-origins、W-origins 和 child-contributions，
公开 `ElementEffect` **只有一个** `origins` 字段：

```text
origins = sorted_unique(R-origins ∪ W-origins ∪ child-contributions)
```

formal current-read marker 的 `ac.origin` 进入 R-origins。写入
来源仍由 C2/A2 的真实 next/use 载体推导，本增补不批准任何
“凭 marker 推断写入”。child effect 先按真实 `ac.instance`
实参映射到 parent parameter/ordinal；对每项 child 引入的
read/write effect，在 parent 的 child-contributions 中使用
**该 parent instance 的 child-construction call Occurrence**。
不把 child 内部 leaf origin 直接复制为 parent origin，也不
虚构父级 `source.read`。同一个 parent element 因同一个 child
同时 R+W，该 occurrence 经 sorted_unique 只出现一次。
这与当前 producer 的 child 转发来源一致；两个同定义 child
实例各用自己的 construction occurrence，注册规则也各自保留
registration scope。固定预期须覆盖 R+W 同一连接、纯 child
转发、两个 child 与两个 registration。

link 先完成 SourceOwner、N1 provider/name、owning header、body
snapshot 和实例端点 authority，再从每个真实注册 rule 的
source.read 重算 R 事件，合并独立 W 和 child 贡献。
`precision="exact"` 要求重算的 read/write flags 及上述唯一
`origins` 数组与 header 相等；`conservative` 仍只能是获批 C2
允许的上界，不能把缺失 marker 解释成 exact。保持 header
不变而删除/修改会改变公开效果的 marker、直接绕过 marker
消费 entry argument、伪造 header/snapshot origins，均拒绝。
同型 owned input 的合法重接、同一 effect 集合内 marker 的
合法改写不能无独立证据一概拒绝；有意 source/IR 变换遵守
C2 的受验证 rewrite 边界。

该操作不标为可随意 DCE/CSE 的 `Pure`。这是 source provenance
保留义务，不是新硬件副作用。只有 generic/exact effects、
snapshot 比较和实例端点验证完成，才可把 result 替换为 operand
并删除 marker。完整 final verifier、C++ 和 Verilog 两个真实
emit 入口均拒绝残留 source.read；更改 `ac.stage` 标签不能跳过
最终拒绝。source/header 的过期产物须重新编译，不增加 schema
版本兼容或旧 lowering fallback。

## 验证包与实施边界

计划改动 `ACIROps.td`、独立 source-read verifier、
`ACIRMathOps.cpp` 的已验证 marker/record-field 来源追溯、
`PythonImportRules` 的 current expression producer、source
effects 汇总、header/body snapshot 比较、U02-C link 和 final/emit
验证器。维持一个 importer、一条 MLIR route；不改公开 Python
语法、CLI、runtime ABI、C++/Verilog 数据语义或资源接口。尚未
批准时，这些代码不得实施，U02-C 只能先验 body/header/N1
及 R/W 集合，不能宣称完整 read origins 闭合。

| Gate | 固定预期与反例 |
| --- | --- |
| op/SSA | parse/print/reparse；实际 current argument、类型、owner、作用域；非法 handle/StateRef/类型/next/local/helper/cast、topology/readiness、伪造 occurrence 拒绝；合法同型 owned 重接不伪称可凭源 op 检出 |
| source/header | 同 state 两次直接读有两个 origins；局部缓存仅一次；bool/完整 record/field projection；未使用 read 保留；未注册方法无 active effect；parent 仅用 owning header 编译 |
| control/effects | runtime 两臂并集；static 选中臂；R+W 同一连接的单一 sorted union；纯 child 转发的 parent instance occurrence；同方法两次注册及两个同定义 child 使用独立 occurrence |
| math | 经已验证 read 的 scalar integer 与 record integer field 正例；signed i1/range(2)/bool 区别；错误域、cast、伪透传和未声明 field 反例 |
| link | 删除/移动导致公开 effect 改变的 marker、保留 R/W 改 origin、伪造 snapshot、移动纯诊断 loc、unit 顺序置换与 SSA alpha rename；保 effect 的合法 body 改写按受验证 rewrite 而非声称 link 一律能识别 |
| final/emit | 先以未改合法 final IR 证明两后端可消费，再分别注入残留 marker，cpp/verilog emit 均失败且不发布；replace 时旧输出保持不变 |

测试须记录选定当前 checkout 的 source/binary SHA、发现数、执行数、
零必需 skip、独立固定 expected Site 列表、变异输入及旧产物
hash。source/link 子集通过不算 final/emit 证据；尚未实现的
gate 明确标为计划，不写成通过。完整 C2 的 list/table/read,
numeric proof、A2 source use、U03 final、双后端和 SSM ELF 继续
在原迁移账本中开放。

### 计划命令与证据

下列 target/测试文件和产品 driver 均是**计划入口**，现阶段
不声称存在或通过。`PYC_L1_BUILD`、`PYC_L1_DRIVER` 必须来自
同一当前 checkout 的 LLVM/MLIR 22.1.8 构建，不能使用旧
`acc.py`/`pycc` 或另一工作树二进制。

```sh
: "${PYC_L1_BUILD:?set current-checkout LLVM22 build}"
: "${PYC_L1_DRIVER:?set current-checkout C3 driver}"
cmake --build "$PYC_L1_BUILD" --target ACIRSourceReadContractsTests -j 6
ctest --test-dir "$PYC_L1_BUILD" -N -R '^ACIRSourceReadContractsTests$'
ctest --test-dir "$PYC_L1_BUILD" -R '^ACIRSourceReadContractsTests$' \
  --no-tests=error --output-on-failure
PYCIRCUIT_TEST_DRIVER="$PYC_L1_DRIVER" \
  pytest tests/system/test_source_read_provenance.py --collect-only -q
PYCIRCUIT_TEST_DRIVER="$PYC_L1_DRIVER" \
  pytest tests/system/test_source_read_provenance.py -m system -q
```

系统 gate 先分别调用真实 `pycircuit compile -c` 发布三份源
unit，以明确 `-I` 编译 parent，再调用真实 `pycircuit link`
检查 body/header/effects；最终从一份已通过的合法 final IR
分别调用 `pycircuit emit --target cpp|verilog`。每个残留 marker
变异须断言非零退出、准确诊断、无新输出，`--replace` 保留旧
产物。日志记录命令、退出码、当前 commit/文件摘要、固定
origin 数组和负例输入；CTest/pytest 发现数非零，必需用例
不得 skip/xfail。未形成 C3 product driver 与合法 final IR 前，
这些 link/emit 行只作为计划，不充当 L1 验收证据。

### 回退边界

本变更先存在于未切换的隔离迁移候选。若撤回 L1 实施，撤回
该 op、producer、effect/link/final 接入，保留既有获批合同及
历史 gate 证据，再从所选源码和工具链重新编译所有受影响的
body/header。不得把旧无 marker 的临时产物作为 L1 输入，
不得加入 schema mode、兼容旧 helper 或第二 lowering route。

若独立设计审阅修改本文语义，PM 先修订并重新送审，再向用户
请求该**精确修订**的批准；批准前不改产品接口。
