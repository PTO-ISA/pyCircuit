# C1：唯一 Pythonic 源语言接口

修订：C。状态：提案，待独立设计审阅和用户批准。适用项目：[单一路线迁移](../../development/pycircuit-modernization-plan.md)。

本提案请求批准**新的 Python 源语言合同及旧 authoring 接口的 hard break 方向**。它不授予尚未定义的 C2 IR 属性/载体或 C3 CLI/runtime ABI 的实现批准；相应产品修改仍等待那些精确提案。它也不批准删去 memory、CDC、四态、异构静态参数和外部 DUT 能力。完整项目状态见[能力矩阵](../../work-items/migration-capabilities.md)。

## 推荐决定

采用 GFSIM 已批准修订 C 的对象式设计作为本仓源语言基线，只有 `module`、`rule`、`system` 三个领域装饰器。普通 class、method、record、标准类型注解与 Python 表达式描述意图；MLIR 负责名字、类型、效果、静态参数、连接与状态分析。Python 层只捕获语法与来源，不执行用户模型。

源 import 为：

```python
from typing import Annotated
from pycircuit import module, rule, system
```

三个名字均表示编译期源声明，不能建立另一条 eager Python 仿真路线。普通 Python 执行模型不是受支持的产品执行方式。`@system` 的完整测试动作合同归后续 C1/C3 扩展；M2 不依赖它，选一个普通 module 作为 root。实现未覆盖的源构造必须给出能力诊断，不回退到旧 compiler。

来源：GFSIM `frontend/ObjectFrontendProposal.md` 和 `docs/API/proposals/signless-integer-codegen.md` 的精确修订 C，批准记录为 `docs/IR/approvals/object-frontend.md`。本仓[来源证据](../../gates/logs/20260927-migration-intake/source-inventory.json)绑定了实读内容。donor 的旧 pending 页眉不替代批准记录；donor 批准也不代替本仓用户批准。

## 模块、构造函数与连接

`@module` 无参数，装饰一个普通类。一个 implementation source 拥有一个 public module；普通记录与 private 纯 helper 可以同源。`@rule` 无参数，装饰 module 的方法。

构造函数是静态描述。可分析的常量、有限 `if/for`、子实例构造和 rule 注册合法；文件/网络 I/O、动态 import、运行时改变拓扑、descriptor/metaclass、继承与动态多态不在此合同内。

| 源形式 | 含义 |
| --- | --- |
| `self.input_value = incoming` | 将成员绑定到 constructor connection 实参，保留相同 state identity，不读 Q，不新建寄存器 |
| `self.count: Word = 0` | 声明本实例拥有的状态及 reset image；初值必须是 MLIR 可证明的静态值 |
| `self.child = Child(self.request, self.result)` | 声明独立子实例和连接，不执行 Python constructor |
| `self.result = self.compute(self.request)` | 注册一条 rule，并把返回绑定到已声明/已绑定目标的 next |
| `self.a, self.b = self.compute(...)` | 固定平坦 tuple 的逐项 next 绑定，目标数/类型/identity 必须匹配 |
| `self.update()` | 注册无返回值 rule；不能丢弃一个数据返回值 |

### rule 注册的源文法

registration 只出现在 constructor 静态展开中。允许以下返回目标：已声明的 `self.member`、固定 owned/connection list 的**静态常量 ordinal**，以及由这些目标组成的平坦 tuple。record field 不是可写注册目标，动态 index 不能选注册目标。

实参只允许已绑定 connection/owned state reference、它的只读 record field projection、固定 reference list 的静态元素、或 MLIR 可证明的静态纯值。注册时不读取 current 来计算算术或动态下标；这些计算写进 rule body。

```python
self.out = self.compute(self.input)        # 合法引用
self.out = self.compute(self.packet.value) # 合法只读字段投影
self.outputs[0] = self.compute(self.inputs[1], increment=1)
# 下列分别是独立负例：
self.out = self.compute(self.input + 1)     # runtime 表达式不是注册 reference
self.out = self.compute(self.items[self.index]) # 动态注册下标
self.out.field = self.compute(self.input)  # record field 不是 next 注册目标
```

位置参数、具名 keyword 参数和 source 定义中的静态默认值按普通 Python 绑定；缺参、多参、重复绑定和类型不匹配拒绝。constructor、rule 和纯 helper 的 `*args`、`**kwargs`、动态 keyword 集合在本包中拒绝。registration 的 rule 参数重绑定只创建局部值；不会经 field projection 写回实参。

未注册的 rule 方法不执行，也不贡献该实例的 active next drivers；其源码仍接受基本 scope/type/subset 检查。同一方法可注册多次，每个 constructor 中的静态调用 occurrence/展开 ordinal 是独立 rule instance，共用当前模块的已声明 state；它们仍遵守相同 writer conflict 规则。C2 定义其精确 identity carrier，不能用源码行号或内容摘要赋予优先级。

同一成员路径只做一次结构声明，后续不能改 owner、初值或成员种类。rule 返回绑定不是第二次结构声明。带注解的 state 初始化与不带注解的 connection alias 不混用：`self.copy: T = runtime_connection` 不是 alias，因 reset image 不是静态值而拒绝。

连接方向由 MLIR 在 module scope 按效果得出：读 current 为 R，写 next 为 W，同时读写为 R+W；R+W 的两个物理端点共享同一 state identity。参数/字段的名字与前导下划线不决定方向。子模块端口转发不增加 relay storage 或额外周期。

父模块编译时 import 子模块的 compiler-published interface，不读取子模块 Python/body AC。interface 的精确 MLIR schema、依赖求解顺序和 link 一致性检查归 C2；不能用 C1 批准绕过逐源编译。

## rule 的值、时序与错误

每条 rule 读取拍初的 committed current。局部变量赋值按普通 Python 顺序建立值依赖；持久成员赋值只提出 next。再次读取持久成员仍读 current，需要复用候选时使用局部变量。

```python
value = self.count
value = value + 1
self.count = value
old_count = self.count  # 仍为 current，不是刚提交的候选
```

一条动态执行路径对同一 next target 最多驱动一次；重复连续赋值、重复 tuple target、跨 alias 重复 target 均拒绝，不能 last-write-wins。互斥分支可合并成一个 next 与 enable；无写的路径保持 current。

多条 rule 并行定义行为，声明/遍历顺序不建立优先级。所有 driver 先规范化为 `(StateID, index, enable)`；无条件写入的 enable 为 `true`，已 scalarize 的 reference-list 写入按实际 StateID 与元素 enable 处理，和 owned list 使用同一规则。本包的分类固定如下，不能由 backend 自行选择静态拒绝、动态失败或 winner：

| 类别 | 源接纳与行为 |
| --- | --- |
| 同一路径重复 next target、重复 tuple target、结构 alias 的重复写 ordinal，或两条无条件 rule 必然同时写同一 scalar/whole-record/static element | 静态拒绝；不采用最后写入 |
| 不同已知 owner/不同静态元素，或从 current 表达式证明 guards 互斥 | 接受；保留独立 driver/proof 供 final verifier 复核 |
| 同一已知 scalar/whole-record target，或已接纳的 owned/reference fixed list 的有限已知目标，可能但不必然重叠（包括一条无条件与一条条件 writer） | 接受并生成 precommit overlap check：两个路径/写 enable 同时成立且目标 index 相等时失败，整个 source tree 不 DriveNext；不是 stall 或仲裁 |
| 动态 owner、不可静态闭合的 alias 集合、未纳入本包的资源/仲裁形式，或无法保持 path-safe 的检查 | 明确能力拒绝；不能改成隐式优先级或后端补救 |

例如两条 rule 分别在 `self.choose` 和 `not self.choose` 时写同一 `self.out`，可证明互斥并接受；分别在 `self.a` 与 `self.b` 时写则必须有 `a and b` 的动态 precommit check；两条无条件写 `self.out` 必须静态拒绝。一条无条件写 1，另一条在 `self.enabled` 时写 2，则 overlap 为 `true and enabled`：enabled=false 的拍合法发布 1，enabled=true 的拍在 DriveNext 前失败，保留原 Q，不能发布任一 winner。静态展开后的常量 enabled=true 使两条写入必然同时 active，必须编译拒绝。同一 fixed list 的两个动态写入先分别完成 path-enabled bounds 检查，再检查 `enable_a and enable_b and index_a == index_b`。任何检查必须在危险访问/驱动之前，并保持短路路径。

普通源算法可以显式表达选择；compiler 不替作者选 winner。transactional Queue/Slot 等 resource overlap 与本段普通 state driver checks 不等价，其原子性/仲裁合同另由 C2 批准。

rule 可以读取本实例合法 input/owned current、驱动合法 output/owned next；不得读写 child 内部状态或创建模块/持久状态。普通 helper 必须是纯值计算，可由 MLIR 内联，不得隐藏 persistent effects，也不得用 rule-to-rule call 暗建调度。

有返回绑定的 rule 每条正常返回路径必须有同类型/结构的值；`None` 不表示“本次没有输出”。有条件 hold 用无返回绑定 rule 的条件成员赋值表达。标量形参重绑定是局部变量，不隐式写回调用者。

source `assert` 和动态算术/index/range 错误按照 evaluation path 检查，而非乘以输出 fire。未被求值的短路分支不报其动态错误。发生源求值或 precommit 检查错误时，整个 source-owned tree 不执行 DriveNext，当前 step 进入失败状态，必须完整 Reset 后复用；不能把错误当成合法背压或吞掉继续运行。C2/C3 必须精确实现相应 IR check coverage 和 runtime failure 状态，再接纳这种执行路径。

## 普通 record

未装饰的普通类描述有序 nominal 数据记录。字段类型静态确定，constructor 必须初始化全部字段，可用有限默认值、关键字参数、嵌套 record 和纯值计算。

record 是不可空逻辑值，不是 nullable host pointer；`None`、缺字段、未定义路径、夹带 module/state/resource handle、动态属性均拒绝。构造后不通过 current alias 原地改字段；更新持久记录时构造完整新值再赋给 owner。后端可使用共享存储实现，但指针/null 不成为源或 RTL 可观察语义。

普通 `Enum`、固定 tuple/array 的完整扩展需给出对应 source/IR 合同；此提案不让它们因为当前 donor 未完全支持而被永久删除。

## 有限数学整数

采用 `Annotated[int, range(lo, hi)]`，表示 `lo <= value < hi`。允许等价简写 `range(hi)`；步长只能为 1，区间必须非空，边界为 MLIR 可求值的静态整数。`bool` 与 `range(2)` 是不同源类型。

MLIR 选择能表示全部值的最小 N>=1；非负区间按 unsigned 解释，包含负数的区间按二补码 signed 解释。可执行 scalar state/port/temporary 的初始实现边界为 1..64 位；静态常量不受该运行时限制，record 总打包宽度也不等于一个 scalar 的上限。

| 算子 | 精确合同 |
| --- | --- |
| `+ - *`、一元 `-` | 数学整数；按操作数区间推导足够中间宽度，在运算前扩展，不能由目标位宽或 C++ promotion 暗中回绕 |
| `// %` | Python floor 语义，余数符号同除数；`-7//3 == -3`，`-7%3 == 2`；除零只在实际求值路径报错 |
| `& \| ^ ~` | 无限二补码整数语义，扩展后运算；`~x == -x-1`，`-1 & 255 == 255` |
| `<< >>` | 负 shift 为路径错误；左移按数学乘法，右移按 floor 除；大 shift 明确处理，不执行未定义 C++/IR shift |
| 比较 | 先扩展到可同时表示两边的公共宽度；按源数学值比较，`255 > -1` 为真 |
| `and or not`、条件表达式 | 条件为 bool；短路且未选中分支不提前求值 |
| `int(bool)`、`int(integer)` | 0/1 转换或恒等；不是截断/位模式重解释 |
| `/`、`**`、运行时浮点、隐式 bool 算术 | 初始合同拒绝，不能部分生成代码 |

每个声明边界——字段、state、rule 参数/返回、next 赋值——检查区间。静态越界编译失败，动态越界在发布前失败；检查后才能缩窄。

```python
# x/out 为 range(256)
self.out = self.x + 1         # x=255 时为区间错误，不能变成 0
self.out = (self.x + 1) & 255 # 独立示例：x=255 时结果为 0
```

以上两行是不同模型的替代示例，不能同时驱动一个 target。完整 u64 加一需要 i65，未获证明的表达式必须拒绝；`(x+1) & ((1<<64)-1)` 可由 MLIR 等价证明改为 i64 低位方程，证明不能移除除零/index 等可观察错误。C++ signedness/format 表示归配套 C2/C3，不能在 codegen 猜测。

这明确替换旧定宽算术接口。保留旧 wrap 行为的模型必须在迁移后的普通表达式中显式表示；不新增第二种兼容 arithmetic mode。

## 固定集合与静态配置

constructor 的固定集合按结构声明分类，不能仅凭 Python list 形状把引用、状态和子实例互换：

| 声明 | 语义与规则 |
| --- | --- |
| `self.values: list[Word] = [0 for _ in range(entries)]` | 本实例 owned fixed state，所有初值静态且在元素类型范围内；不包含连接 reference |
| `self.links = [self.left, self.right]`（已有 connection reference）或 `self.links = incoming_list` | connection-reference 集合，仅保留已有 identity，不复制状态、不增加延迟；所有元素都是引用，不夹字面量临时值 |
| `self.children = [Child(...) for _ in range(count)]` | 固定子实例集合，每个 occurrence 状态独立；不能作为 record/data，也不能在 rule 内动态创建或窥视内部状态 |

owned list 的 rule read/write 可用静态或有界 runtime integer index；必须满足 `0 <= index < length`。**负 index 是越界，不是 Python 尾部索引；bool index 拒绝。** 静态活跃越界编译失败；动态越界在该 evaluation path 上 precommit 失败。本包 fixed state/connection 集合要求静态正长度；零/负长度拒绝，不生成假元素。是否扩展无存储空集合由后续源合同决定，不能隐式改变当前接纳范围。

reference/child 集合在 registration/binding 时只接受静态 ordinal；不能用 runtime index 动态创建连接或实例。rule 内可以按有界 integer index 读写已 scalarize 的固定 connection-reference 集合：先检查 bounds，再显式选择 current 值或给每个已知 StateID 生成 path/index enable；不得用运行时指针运算选择 owner。child 实例集合内部状态仍不能由父 rule 访问。重复 read reference 允许；一个 connection 可供多个只读端。alias 接纳按**每个 ordinal 的 effects**判断：R/R 可以共享 identity，R/W 也可以共享同一 current/next identity；两个都包含 W（W 或 R+W）的不同 ordinal 绑定同一 identity 则静态拒绝。不能把整个列表含有 W 扩大为所有 ordinal 都禁止 alias。单一已知 identity 上不同 rule 的 driver 重叠按上节统一 enable 规则检查，不能用重复 write ordinal 绕开。未证明方向的绑定必须等 MLIR effects 闭合，不能先假定只读。

例如某 module 对 `items[0]` 仅 R、对 `items[1]` 仅 W：绑定 `[self.value, self.value]` 合法，读者仍读 current、写者提出 next；如果两个 ordinal 均有 W，则同一绑定静态拒绝。多个 R ordinal 共享 `self.value` 不新增 owner，也不复制 reset image。

集合固定后禁止 append、delete、reshape 或成员重绑定。slicing、嵌套/ragged reference list、动态 child/reference 结构绑定的源合同留待后续提案，本包明确拒绝，不把该排除视为永久功能退役。fixed list 本身不代表 FIFO，FIFO 必须由显式状态算法或后续批准的资源接口表达。

### 普通静态参数

参数在普通 constructor 签名中声明，可以使用普通静态默认值。用于结构个数、静态循环界限、类型/形状边界或 **reset initializer** 的参数必须 static；由其派生的纯表达式也 static。static 参数可作为 rule 内常量，但不能接纳运行时连接。只因某次 actual 是常量或参数名为 width/depth，不足以把一个 connection 参数变 static。

只被 rule 作为数据/connection 使用的参数按 connection 类型与效果处理：需要有限源类型及已声明 reference，不能从数字 actual 或默认零值隐式分配 connection/state。分类冲突拒绝。static actual 允许常量、静态 source/import 名称与 MLIR 可求值的纯表达式；使用普通位置/keyword/default 绑定，禁止额外 parameter wrapper。

以下是无省略的参数化 module 和双实例 root；不同 `entries` 是同一 owning source 的不同配置，`initial` 由 reset 用途被分类为 static：

```python
# bank.py
from typing import Annotated
from pycircuit import module, rule
Word = Annotated[int, range(256)]

@module
class Bank:
    def __init__(self, incoming: Word, outgoing: Word,
                 entries: int, initial: int = 0):
        self.incoming = incoming
        self.outgoing = outgoing
        self.cells: list[Word] = [initial for _ in range(entries)]
        self.outgoing = self.exchange(self.incoming)

    @rule
    def exchange(self, value: Word) -> Word:
        previous = self.cells[0]
        self.cells[0] = value
        return previous
```

```python
# bank_core.py
from pycircuit import module
from .bank import Bank, Word

@module
class BankCore:
    def __init__(self):
        self.input_a: Word = 9
        self.input_b: Word = 13
        self.output_a: Word = 0
        self.output_b: Word = 0
        self.small = Bank(self.input_a, self.output_a, entries=2)
        self.large = Bank(self.input_b, self.output_b, entries=4, initial=5)
```

第一拍提交后输出 `(0,5)`，第二拍为 `(9,13)`；每实例 state 独立。`entries=runtime_ref`、`initial=runtime_ref`、`initial=256` 都拒绝；这里 `entries=0` 违反正长度合同，且不能使 `cells[0]` 合法，不能偷偷分配一项。

目标允许这类异构普通 static actual，由 MLIR specialization 处理。相同 typed arguments 复用实现，不共享 state；不同配置仍由同一个 owning Python source/AC/C++ source group 发布，不生成 per-case 文件或公开 parameter-value 类名。

本包批准**参数化 collection shape 和 reset**的源语义，不批准尚未具体定义的 constructor 参数依赖类型注解语法；当前 `Annotated` 的范围通过 source/import 的静态类型别名声明。跨参数 dependent port/record type、更多集合形状仍须后续源合同，与 C2 dependent interface/family realization 一起审阅，且在完整能力矩阵保持必需任务。

删除 authored `module_decl`、`finite_cases`、`case` 接口，使用生成 source interface。donor 每 symbol 一组参数可作为 M2 的显式实现能力限制；上述 `BankCore` 仍是目标合法源码，实现未接纳异构参数时只能给出明确 capability rejection，不能宣布该目标能力完成或作为永久限制。

## 首个可执行源例与独立期望

以下代码是提议的新语法，尚未实施或作为已编译示例发布。三个 source 分别编译，interface linking 由 C2/C3 定义。

```python
# packet.py：type-only source
from typing import Annotated
Word = Annotated[int, range(256)]

class Request:
    value: Word
    valid: bool
    def __init__(self, value: Word = 0, valid: bool = False):
        self.value = value
        self.valid = valid
```

```python
# accumulator.py
from pycircuit import module, rule
from .packet import Request, Word

@module
class Accumulator:
    def __init__(self, request: Request, result: Word):
        self.request = request
        self.result = result
        self.total: Word = 0
        self.result = self.accumulate(self.request)

    @rule
    def accumulate(self, item: Request) -> Word:
        total = self.total
        if item.valid:
            total = (total + item.value) & 255
            self.total = total
        return total
```

```python
# core.py：M2 普通 portless module root
from pycircuit import module, rule
from .packet import Request, Word
from .accumulator import Accumulator

@module
class Core:
    def __init__(self):
        self.left_request: Request = Request(1, True)
        self.right_request: Request = Request(2, True)
        self.left_result: Word = 7
        self.right_result: Word = 19
        self.left = Accumulator(self.left_request, self.left_result)
        self.right = Accumulator(self.right_request, self.right_result)
        self.advance()

    @rule
    def advance(self):
        self.left_request = Request(1, not self.left_request.valid)
```

| 观察 | left_result | right_result |
| --- | --- | --- |
| Reset 后 | 7 | 19 |
| 第一拍 Work 后、Xfer 前 | 7 | 19 |
| 第一拍 Xfer 后 | 1 | 2 |
| 第二拍 Xfer 后 | 1 | 4 |
| 第三拍 Xfer 后 | 2 | 6 |

Reset 后重复该序列。期望向量由独立测试维护，不由 compiler 生成。首 gate 可用 donor 已有 `Print(std::ostream&) const` 的状态输出观测 C++，RTL 用仅供 gate 的 source-state 到 Q 路径映射，在 clock edge/NBA 前后采样。它们不改变状态、不携带预期、不成为公开 model observation ABI；精确生成/采样接口随 C2/C3 批准。

M2 portless root 是启动验证的方法，不是最终输入/输出限制。外部 typed DUT 端口、真实 stimuli、多 clock stepping、source `system` 都是强制后续合同与完整验收工作。

## 受影响的旧合同与 hard break

C1 接受后登记新的待实现源合同；实际产品切换时同步 supersede Decision 0148 的 CAS/JIT/direct builder/occurrence 接口、0150 的多 frontend/ACPy 表面、以及相关旧 function-style Agentic 与 finite-case authoring 条款。0136 的 authoritative type/effect inference 迁到 MLIR。必要硬件行为由新表达和批准的扩展继承，不随源 API 删除。

本包源语义的明确替换如下；IR carrier、硬件事务执行和 ABI 的变化仍单独属于 C2/C3：

| 旧行为 | 新源行为 | 受影响决定 | 本包范围 |
| --- | --- | --- | --- |
| CAS occurrence/JIT/direct elaboration，多 source authoring | 单一对象 source，capture 不执行模型，持久 state 表达周期 | 0148、0150 | 批准 source API 退役及替代；切换时生效 |
| frontend authoritative 类型/effects 推导 | MLIR authoritative 分析 | 0136 | 源接纳责任；具体 IR carrier 另批 |
| unsigned div/rem 对零除数返回 0 | 静态活跃零除数编译失败，动态 evaluation-path 零除数 precommit 错误 | 0247 | 明确批准源可观察错误变化，不保留第二 arithmetic mode |
| 定宽 +/* 等隐式回绕 | 数学中间结果，声明边界检查，模意图显式 mask | 0246/0247及关联位算子条款 | 源算术改变；C++ 表示/IR 方程另批 |
| record 字段赋值/with_fields 规范化，同块重复字段保留最后值 | record 构造后不可原地改字段，完整重建；同路径重复 next target 拒绝 | 0236、0246、0256的 source 更新条款 | 只替换源表面/重复写接纳；0236 的事务原子性义务不因此删除 |
| source-owned `finite_cases` 枚举与 `case` 选择，调用者不得引入新 case | 普通 static constructor actual 由 MLIR 专门化，无人工 finite-case 列表 | 0275–0278 | 明确改变源码参数接纳关系；保留 source ownership、实例独立和同参共享实现；carrier/公开生成 family 另批 |

切换删除旧 exports、CLI source dispatch、Python semantic lowerers、旧接口例子/正例和 fallback；保留明确的拒绝测试和历史记录。所有动态驱动、数据类型、ABI 或 IR 变化还需 C2/C3 对应批准，不能从此 C1 提案推导无限实施权限。

## 本次批准的范围与不在本次批准的范围

建议用户批准本文件修订 C 的源语法、对象/record/current-next、数学整数、collection/static 参数方向及硬切换义务，允许据此完成相关 verifier/test 和后续具体 IR/SDK 设计。

本次不批准：新的 IR op/type/attribute 精确形式、CLI/compile/link flags、source-header binary/schema、runtime ABI、memory/CDC/四态源绑定、完整 `system`/external-DUT 端口协议。它们分别进入 C2/C3 或后续扩展，并保持完整项目的 release 阻断条件。任何实现同时触及这些接口，必须等待其批准。

独立设计 reviewer 必须检查合法/非法 source、例子的逐拍结果、范围/短路/错误语义、当前来源记录、hard-break 删除范围及本次批准边界，给出针对精确文件的 approval-ready 或 revise。PM 不能替用户批准。
