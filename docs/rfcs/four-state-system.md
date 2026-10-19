# 四态值、复位控制与 System 设计补充

状态：已实施并通过四态定向验收；完整资源矩阵仍保留为后续验证。设计基线为 `d5f57426`。
本提案补齐前端、共同 IR、C++/Verilog codegen 和生成式 System 的契约。
它不追认两道失败 benchmark 为通过，也不直接修改已冻结的 sampled-admission 记录。

## 1. 问题与设计结论

当前 Bits 的仿真载荷已经能保存 0/1/X/Z；问题是作者缺少显式四态常量、
System 缺少四态域刺激，而 reset=X 被采样规则直接判为故障。
四态表示、未知控制的解释、事务可用性和仿真错误是四件不同的事。

采用以下最小补充：

1. 继续使用 `bits[W]` / `uN`，增加 `ac.four_state("10xz")`；不增加 Signal 类。
2. 复用现有 `case_eq` 运算提供精确四态比较；提供由已有运算构成的 knownness 检查。
3. 增加一种明确的四态常量 IR 操作；不破坏已知整数常量的证明前提。
4. reset 解释由链接后的一个根策略决定：默认 `strict`，显式选择 `if_one` 时
   仅已知 1 激活 reset。两种策略在全部后端保持一致。
5. System 可声明有限的域采样序列，仍由现有生成式驱动器执行。
   DUT 内不出现 clock/reset 参数、驱动调用或提交控制。

`if_one` 不是完整的“Verilog 模式”。它不改变 enable、guard、分支、clock、
错误恢复或数据运算的语义。暂不增加悲观 reset 合并策略。

## 2. 四态值和事务状态

每个 W 位载荷沿用 value / known / Z 三个平面：

| 逻辑符号 | known 位 | Z 位 | value 位 |
| --- | --- | --- | --- |
| 0 | 1 | 0 | 0 |
| 1 | 1 | 0 | 1 |
| X | 0 | 0 | 规范常量为 0 |
| Z | 0 | 1 | 规范常量为 0 |

常量必须满足 known 与 Z 不重叠，所有位都在 W 范围内，未知位下的 value 规范为 0。
其他运行时生产者可能带有未知位下的无意义 value；精确比较不能受这些位影响。
Z 是可运输的逻辑符号；本提案不增加多驱动解析、三态总线或电气阻抗模型。

四态载荷不等于不可用值。一个已提交、可消费的结果可以全为 X；一个未提交的
proposal 不能因为填了 X 就变成合法输入。已有 availability 分析仍先于消费。
`case_eq`、`is_known` 不能绕过 unavailable-result 检查。

## 3. 前端

### 3.1 DUT 保持普通串行写法

```python
import pycircuit as ac

@ac.module
def Register(incoming: ac.u8) -> ac.u8:
    saved: ac.u8 = 0

    @ac.rule
    def update() -> None:
        saved = incoming

    update()
    return saved
```

`saved` 是持久状态，Rule 局部变量仍是组合值。clock/reset、DFF/DFFE、写使能和
proposal/commit 由编译器生成。Rule 可以在 Module 内或外定义，定义位置不改变
上述四态或状态语义。

### 3.2 显式常量

接口：

```python
pattern = ac.four_state("10xz")  # bits[4]，从高位到低位
```

- 一个非空源码字符串字面量，只允许 ASCII `0/1/x/X/z/Z`，规范为小写。
- 字符数是精确宽度；不接受空白、分隔符、前缀、`?`、运行时字符串或隐式截断。
- 结果具有 fixed Bits 来源。即使宽度为 1，也不自动成为数学 Integer/Boolean 常量。
- 赋值到更宽 Bits 时只允许已有的无损零扩展，保留低位 known/Z；缩窄须显式切片。
- 普通 `0`、`1` 等字面量语义不变。不新增未知 MathInt、参数值或 Enum 成员。
- Struct/Table 的 Bits 字段、声明初值和数据参数复用已有构造、连接、闭合初值验证。
  逻辑类型和 Enum 仍通过现有合法边界处理，不能用宽度相同替代类型/成员证明。

Python capture 只记录语法和来源；禁止执行设计函数或通过 Python 对象模拟四态。

### 3.3 检查与断言

```python
assert ac.case_eq(actual, ac.four_state("10xz"))
assert not ac.is_known(actual)
```

`case_eq(a, b)` 要求等宽 Bits，逐位比较 0/1/X/Z，返回始终已知的 Boolean。
`is_known(value)` 接受 Bits，返回所有位是否为 0/1；Z 也属于未知。
结果“运行时已知”不等于“编译期常量”，不能作为 static 参数/循环界限证明。
普通 `==` 保留原有四态传播，不偷偷改成精确比较。

这两个前端观察接口用于 System 验证和 assert/log 诊断。首轮不把 X 检测作为
DUT 的功能性输出、state write 或资源/Rule admission 条件：source/SSA 使用分析
必须区分验证用途与功能用途，包含别名和 helper 转发，不能只检查直接父 AST。
已有编译器内部的 `case_eq` 不受此作者接口限制影响。验证来源不能通过 helper
返回值伪装成普通硬件谓词；共享依赖/effect 分析负责诊断，无需新增硬件数据类型。

具体使用规则在现有 source analysis 中执行：helper 展开/参数绑定之后、作者来源
标记删除及通用折叠之前，从作者 case_eq/is_known 结果做受限依赖遍历。
传播包括别名、标量运算、aggregate/projection、helper 返回值和**控制依赖**。
允许的终点只有 assert 条件、log 载荷及对已知 Boolean 的数值 report。
诊断分支选择可以使用这些结果，但分支内的所有效果必须也是上述诊断效果。
流入 Module 输出、状态/初值、资源操作数、功能性索引/选择器、Rule/Method guard
或硬件调用实参均拒绝；同一值兼有诊断和功能用途也拒绝。
source-unit 发布/导入必须保留可由实际 provider body 重建的来源，不能仅相信
header 声明的 verification-only 标签。遍历复用现有 source-analysis 工作限制。

显式 X/Z 字面量在仿真中是确定的逻辑符号，不能降级为 0 或随机值。
功能性综合对 X/Z 常量及 X 检测的支持不是本提案的验收声明；不得将 RTL 仿真
通过视为这些表达式已具备可综合的电路含义。

## 4. 控制语义

### 4.1 只增加根级 reset 策略

| 已知上升沿的 raw reset | `strict`（默认） | `if_one`（显式选择） |
| --- | --- | --- |
| 0 | 正常计算/更新 | 正常计算/更新 |
| 1 | 按声明初值同步复位 | 按声明初值同步复位 |
| X 或 Z | reset 故障，整个根零提交 | 进入正常路径，不激活 reset |

`if_one` 精确定义为 `effective_reset = case_eq(raw_reset, 1)`。
不得只用 value 位判断，否则带隐藏 value=1 的 X/Z 会被错误当成复位。
strict 保持 raw reset；未知复位错误在采样边界报告，不改变其载荷。

公开入口为 `pycircuit link ... --reset-semantics strict|if_one`。
缺省显式序列化为 strict；source units 不携带
各自的策略，因而可用于不同根。`emit`、Runtime 配置和某个 leaf 不允许覆盖策略。
`--rtl-mode managed|physical` 只选择执行适配，不能替代 reset 策略。

选择 if_one 的设计，native、managed RTL、physical RTL 都使用 if_one。
若比较 strict 与 if_one，需要由同一 source closure 链接出两个不同 final 产物；
不能用同一个 final 文件在不同后端得到不同解释。

### 4.2 其他控制不随之放宽

| 情况 | 语义 |
| --- | --- |
| clock=X/Z | 采样失败，保留 committed clock history；if_one 不豁免 |
| 已知低电平、保持或下降样本 | 无状态上升沿更新；不新增 reset/enable 的边沿故障 |
| reset=1 | 复位优先，沿用现有 sampled guard/enable 屏蔽规则 |
| strict 且 reset=X/Z | reset 故障先于正常更新路径的 guard/effect 检查 |
| if_one 且 reset=X/Z | 正常路径；enable/已到达 guard/effect selector 的 X/Z 仍可导致故障 |
| guard 为已知 false | 该最外层事务不执行；不是全局故障 |
| 已到达 guard 为 X/Z | 故障；后面的 false guard 不能抹掉它 |
| 未到达的 guard 为 X/Z | 不凭空增加故障；保留真实 reached/ordered 路径 |
| 纯分支 selector 为 X/Z | 复用现有逐位合并，保留两臂共同的已知位/共同 Z |
| 含效果的分支 selector 为 X/Z | 保留已有 known-selector 义务，不任意选择一臂 |
| 合并后的有效 write intent 为 X/Z | 存储检查仍失败；不能把它当成 false |

普通 body assertion 的 Work 语义、reset mask 依其既有合同处理，不笼统改成
“只在上升沿检查”。所有 reset mask 必须使用同一策略的解释。
真实故障仍使整个根的 pending 状态/时钟历史丢弃，不发布该 epoch 的观察结果。
物理 reset 与 host Reset 是不同操作；物理 reset 不能清除 sticky failure。

悲观 reset 合并暂不纳入：现有 pure select 只能解决值合并，尚不能决定复位/正常
两条路径的错误优先级、资源操作和观察效果。不能用“全写 X”替代完整语义。

## 5. 共同 IR 与验证

### 5.1 四态常量

新增一个窄的纯操作，名为 `ac.bits.constant4`：规范 pattern 字符串，结果为 Bits。
`ac.bits.constant` 继续表示已知数学值；不能给旧操作添加隐含未知 mask，破坏
现有常量折叠、static evaluator、区间和 knownness 证明。

验证要求：属性闭合、仅小写合法符号、宽度为正且与 pattern 长度严格一致，
使用现有宽度/分配限制。无数据依赖；在闭合初值 DAG 中属于允许的字面量叶。
全已知 pattern 可以优化为旧 constant，但 fixed Bits 作者类型不能因此变成 Integer。
含 X/Z 的值不能产生 MathInt `closedSourceConstant`、静态计数或参数证据。

`case_eq` 复用 `ac.bits.compare` 的已有 predicate。
`is_known(x)` 可用 `case_eq(eq(x, x), true)` 组合已有操作实现，无需新 Runtime 运算。
不得把普通 `eq(x,x)` 直接折叠为 true；X/Z 输入下它应为 X。

所有显式枚举 constant 的拥有者必须同步检查：类型/来源推导、依赖、helper/loop
准入、初值验证、lowering、常量优化、source-unit round trip 和双后端。
loop 对新 pattern 的存储也要计入原有账本，不能通过新操作绕过资源验证。

### 5.2 根策略和 System 序列

在已有 `ac.system` 选择操作上增加闭合的根策略属性，例如：

```mlir
ac.execution_policy = {reset = "strict"}
```

不修改现有 `entry={callee, parameters, type_arguments}` 的含义。
新链接产物显式记录策略；旧的无属性产物仅能解释为 strict，不能由环境变量猜测。
规范化写入发生在新 link 候选中；只读 verifier 不改写已保存产物的字节。
未知值、错误 placement、子实例覆盖或冲突元数据拒绝。

共同执行/check plan 提供只读的 validated policy。每个实际采样/存储/资源 owner
使用该根值；保留 raw reset SSA、物理域 ordinal、实例路径和 source-check 身份。
不靠随意插入 reset 表达式改变域连线而绕过现有身份证明。

System 采样序列作为根声明的独立、闭合属性保存，包含 rows 和源码 occurrence/span。
source body/interface 必须一致；link 只提取实际选中 System 的序列到 final 计划。
源文件不能伪造 final-only 权威，隐藏 provider body 后导入也不能丢掉序列。

## 6. Codegen 与 Runtime

复用 `FourState`、`wire`、标准存储和 SimSystem 的现有全局提交屏障。

| 拥有者 | 修改要求 |
| --- | --- |
| 已有 C++ definition emitter | constant4 → `FourState::fromMasks`，不进入 `known()` 路径 |
| 已有 RTL definition emitter | 输出带明确宽度的二进制 0/1/x/z 字面量 |
| 闭合物理初值 renderer | 同一 constant4 语义，保留类型、字段/Collection 位序和首拍前初值 |
| `sampled_clock.h` 邻近的共享 helper | 解释 raw reset，返回 effective reset/活动性/采样故障；不依赖 LLVM |
| DFF/DFFE/FIFO/各 memory kernel | 每个实际 reset 使用同一解释；不能只改寄存器而漏掉资源 |
| native source-check 和 RTL sampled-check emitter | 以同一解释计算 reset mask，保留 raw 值用于诊断 |
| physical wrapper/leaf | 共同策略决定 reset 错误和转换；保持 pre-edge 全根许可、NBA 更新和 sticky failure |

可以通过生成代码的私有参数传递策略，但其唯一来源必须是验证过的根计划。
独立使用的 Runtime primitive 默认 strict；不得新增作者可随意设置的逐寄存器开关。
实现必须清点现有重复检查，特别是 FIFO、sync_mem、byte_mem 内部的 unknown-reset
判断。一次解释要同时覆盖状态转换和检查，不能只屏蔽某个错误提示。

生成的 manifest/结果记录 `reset_semantics`，两种后端相同。缓存、生成清单和
publication 身份校验必须考虑 final 内容及策略；失败替换保持先前产物完整。
knownness 错误仍使用现有结构化失败路径；physical fault marker 仍不能被 TB 的
成功行覆盖。合法的 if_one reset=X 不应再产生该 marker。

## 7. System：源码声明刺激，编译器执行

前端示例：

```python
@ac.system(samples=((0, 0), (1, ac.four_state("x"))))
def ResetScenario():
    observed = Register(ac.four_state("10xz0011"))

    @ac.rule
    def check() -> None:
        assert ac.case_eq(observed, ac.four_state("00000000"))

    check()
```

这里 samples 是测试根的有限 `(clock, reset)` 原始采样序列，不是 DUT 引脚声明。
在 if_one 下两行 Work 都读取寄存器旧值 0；第二行成功后才提交 `10xz0011`。
在 strict 下第二行是预期的 reset 故障，不能声称断言运行通过。若追加后续样本，
其 oracle 必须相应期待已更新的四态数据。

- 每行对应一次现有 Work/Step epoch，不是一整拍。0/1 为已知一位值，X/Z 通过
  一位 four_state 常量表达。拒绝其他宽度、空/非静态序列、任意 Python 回调。
- 没有 samples 时保留原来的自动 low/high 驱动与 `--cycles` 用法。
- 有 samples 时按有限序列执行且耗尽即结束；任何显式 `--cycles` 或 RTL `+cycles`
  都拒绝，包括恰好等于某个推测周期数的值。奇数行、保持电平或重复电平不能用
  行数/上升沿数猜测用户周期数。CLI 必须区分“省略”与“显式指定”，并验证重复/
  非法选项。序列长度和存储限制必须在构造 rows 或生成查找存储之前验证。
- 仍先执行已有 host Reset 建立声明初值和已知 clock history，再从第 0 行开始。
  样本里的 reset=1 是同步物理复位，不是 host Reset 命令。
- 数据刺激复用普通 System 状态/连接与 constant4；不引入 ac.drive/ac.step 到 Rule。
- 每行执行：驱动 raw 域值 → RTL settle → Work/PREPARE 并冻结输入/检查 →
  全根 precheck/precommit → 一次 commit 或完整 discard。
- Rule 的 assert/log 观察该 Work 的输入和旧状态；成功观察在提交后发布。
  不得因发布时点在 Xfer 之后而把它误写成“断言已更新的状态”。
- 故障行保留错误终态并停止普通执行；序列后续行不能当成隐式恢复。预期故障由
  现有测试/gate读取 runner 的错误结果判定，不把执行失败改成产品成功。
- 同一序列、同一 final 和同一 reset 策略在 native workers1/2 与真正四态 RTL 下
  比较。含 X/Z 的 System 不能交给只做二态模拟的工具后宣称四态通过。

现有 SystemRunner initialize/drive/sample 回调和生成式 RTL step 足够承载此序列。
不另建解释器、事件调度器或第二种硬件测试语言。System 的 settle 属于测试调度，
不能移入 physical DUT 制造延迟或额外周期。

## 8. 观察输出与兼容性

`ObservationSlot::SlotValue` 当前仅保存 Bool/Signed/Unsigned 的 64 位值；
它不是任意四态/宽位向量容器。不能只在前端允许 log(X) 后丢掉 mask。

设计上在现有观察通道增加带宽度的 Bits4 载荷，使用既有槽位/epoch staging 和
受限配置期存储，不在每拍临时拼装一套新事件系统。建议 JSON tagged value：

```json
{"kind":"bits4","width":4,"value":"8","known":"c","z":"1"}
```

以上是 `10xz`；十六进制字符串无前缀，固定为 ceil(width/4) 位，高处 padding 为 0，
未知位下的 value 规范化。known/Z 不重叠；X 由其余位确定。
已支持的 fully-known Bool/小整数输出保持原格式；未知载荷或超出原小整数宽度的
Bits 使用 bits4，不冒充整数 0，不静默省略。数值 `report` 聚合仍要求其数值域，
不能对 X/Z 做 sum/min；四态诊断使用 log，或 report 一个已知的检查结果。

解析器、GFSIM、RTL JSON emitter 和独立 oracle 必须一起扩展该 tagged union。
旧读取器遇到未知 kind 应明确报错。不得声称新增内部 C++ 存储布局仍有二进制
兼容性：生成模型与 Runtime 按现有打包合同共同重建；不为此发明另一套公共 API。
此项可独立交付，但没有完成它时只能声称精确断言可用，不能声称四态日志完备。
在完整 Bits4 staging 和两端 serializer 落地前，无法用现有格式无损表达的观察
载荷必须由现有 observation 验证边界明确拒绝；不得接受后只渲染 value、变成 0
或成功运行但漏掉事件。新 literal 支持不能旁路旧观察边界。

## 9. 复用位置与实施清单

| 切片 | 现有拥有者 | 完成条件 |
| --- | --- | --- |
| A 四态常量/精确检查 | `__init__.py`、绑定注册、`PythonImportExpressions.inc`、`ACIROps.td`、`ACIRBitsOps.cpp`、两种 definition emitter | 语法、source facts、纯度/依赖、同类型传输、初值和两端执行一起通过 |
| B 根 reset 策略 | `SourceLink`、`ACIRSystemOps.cpp`、`HardwareSourceChecks`、Runtime kernels、check emitter、physical wrapper | 同一 final 策略贯穿全部实际 owner；无逐后端/逐实例分叉 |
| C System 序列 | System signature/capture、source-unit校验、`FinalEmitCpp`、`FinalEmitVerilog`、`SystemRunner` | 独立 provider、生成式双后端、有限终止、故障与旧状态时点一致 |
| D 观察载荷 | `ObservationSlot`、`system_runner.cpp`、既有 observation staging/RTL emitter/oracles | X/Z和宽位日志无损；失败epoch零发布；旧已知格式不变 |
| E 集成/benchmark | 现有 API/Examples/System/benchmark入口 | 显式记录策略，原TB/刺激/成功判据不变，负对照有效 |

- [ ] 为 A–D 分别补正例、反例、序列化/篡改和失败替换测试，再修改各自拥有者。
- [ ] 复核所有 constant 枚举、knownness/totality 静态证明和资源计数，不把 X 当已知。
- [ ] 清点每个 reset 消费者，含无状态采样、Queue、各 memory，不只做 DFF。
- [ ] 保留 strict 默认的全部已知/未知控制测试；新增 if_one 的同构矩阵。
- [ ] 新增生成式 System 四态用例；不以手写 host/TB 替代前端 System 验收。
- [ ] 四态日志独立验收；最小阶段如尚未实现，清楚记录该缺口。
- [ ] 更新 active reference/authoring/skills；冻结合同只通过此补充说明覆盖范围。
- [ ] 策略/接口实施后重跑原始 benchmark，更新 PR 验证和机器可读结果。

## 10. 必须覆盖的语义用例

| 编号 | 独立验收 |
| --- | --- |
| F01 | 0/1/X/Z literal及非法字串；宽度1/5/65/130、精确宽度/扩展/显式缩窄 |
| F02 | Struct/Table传输和初值；X与Z区分；未知位下value噪声不影响case_eq |
| F03 | 普通eq与case_eq不同；is_known结果已知但不错误成为static参数事实 |
| F04 | available X载荷合法；unavailable结果即使只查knownness也不得消费 |
| F05 | 检查结果经别名、helper aggregate、分支控制泄漏到功能输出/状态/索引/guard时拒绝；纯诊断分支允许 |
| R01 | reset的0/1/X/Z × 两策略 × rising/held/falling；clockX/Z始终独立故障 |
| R02 | if_one resetX/Z且enable1正常写；enableX/Z仍故障；reset1屏蔽边沿enable故障 |
| R03 | 先到达未知guard后false应失败；false后未到达未知guard不失败；effect selector同理 |
| R04 | 两个owner中一方故障全部零提交；clock history、discard/retry、sticky失败保留 |
| R05 | 无状态事务、DFF/DFFE、FIFO、各memory采用同一root策略与reset mask |
| S01 | 同一有限samples下native workers1/2和四态RTL逐epoch结果一致 |
| S02 | 初始化、Work旧值、提交后发布、结束/截断、故障行与恢复边界分别验证 |
| S03 | 别名、导入provider、body/interface篡改不能丢掉/伪造策略或序列 |
| O01 | 四态日志与宽位值往返；旧known日志不变；故障epoch无新记录 |
| B01 | 两道resetX benchmark在显式if_one产物下运行；strict仍按原合同报错 |

不得把两个策略的结果混成一个分数。机器可读结果必须能定位 source、final、
policy、backend、stimulus和原TB哈希。仅提供此设计不代表任何新增测试已通过。
新增 literal/比较的验收还必须有独立的符号或 mask oracle，直接检查 value/known/Z；
不能只让 DUT 和 expected 同时经过同一个新 lowering 后互相比对，否则共同错误
可能使 System 自检假通过。生成式 System 验收与这些底层独立 oracle 都要保留。
