# 编译器问题逐层审计（2026-10-19）

后续交付进展：C07已通过更准确的十进制文本空间上界修复，原始vector100r
200样本零mismatch；C11新增标量generic直接/导入调用和根命名Integer参数绑定，
完整15项独立测试通过；C10已有完整、区分目标的ports.json，benchmark不再猜测RTL。
C08已有显式physical导出与closed初值支持，完整已实现VerilogEval候选86/88通过，
另两题原TB首次边沿reset=X与严格控制契约冲突，等待语义决定。下文表格保留第一轮
审计状态；持续交付状态见
[`compiler-benchmark-delivery.md`](../../docs/development/compiler-benchmark-delivery.md)。

本次检查基于 `dcbd84c0905475ec07089fc9c985316633ae6ddd` 加工作树修改。
范围是 benchmark 移植实际遇到的问题及其编译器调用链，不是整个编译器的
无缺陷证明。源码、完整命令、诊断和本次验证记录位于
`docs/gates/logs/20261019-compiler-audit/`；这些本地日志不参与产品或测试。

结论：存在旧准入逻辑挡住已有能力、类型事实在调用点丢失、不同源码路径
支持不一致的问题。但 managed RTL 的拒绝也有真实的采样/撤销语义原因，
不能把所有 fail-closed 检查一概删除。没有证据证明这些问题出自某个 agent
故意加 shim；可追溯的是具体提交和代码职责。

## 问题与修复状态

| ID | 触发 | 首个失败位置 | 已有能力与判定 | 本次处理 |
| --- | --- | --- | --- | --- |
| C01 | Bits 与条件 Integer 结果做位运算 | `PythonImportRecords.cpp::expression` | 分支已有 kind/interval；二元运算仅给 AST 字面量保留固定宽度来源，属于衔接缺陷 | 运算前复用 `unsignedBoundary`，证明完整范围后转换；保持原变量/别名的类型事实 |
| C02 | `index < value`、`value > index` | `compareSourceValues` | helper 已有闭合 ordinal；Rule 则是有界 `scf.for` 的动态 induction，二者不能混为常量 | 闭合 Integer 或经验证的 loop induction 使用现有范围转换；不伪造常量事实 |
| C03 | `value << runtime_bits_count` | `expression` 的 static-count 分支 | ACIR、C++、Verilog、四态 Runtime 均有移位，实现缺口在源码准入 | 零扩展双方到最大宽度，调用已有 shift，截取原数据宽度；计数不截断 |
| C04 | helper 循环中的 shift、`//`、`%` | `preflightForCapabilities`、`describeLoopExpression` | 普通表达式已实现；旧白名单和资源形状没有跟进 | 放开四种已有运算，描述 reciprocal 除法中间宽度和操作数；保留事前预留与事后审计 |
| C05 | helper 中 `128 >> index` | `provenStatic` | 展开的当前 ordinal 已是实际闭合 SSA 常量，但静态语法检测不读取它 | 仅认可当前 binder 名称和实际 SSA 身份；重绑定/复制别名不获得静态计数权限 |
| C06 | `bits[W]` 与常量混合位运算 | `unsignedBoundary` 的精确 Integer 宽度证明 | generic constant 的实际参数绑定验证本来已有；新归一化曾提前要求具体宽度 | 保留真正 contextual `BitsConstantOp` 的延迟 fit 验证，负数/动态表达式不能进入该分支 |
| C07 | 100 次 bit reversal 循环 | `payLoopEnvelope` | 运算 lowering 已可用；循环对每个可能结果都预留最大常量/事实载荷 | **未修复**：原 shift 版到第44次、concat版到第64次达到同一1 MiB限制；未提高限额 |
| C08 | `dff8` 等自然状态程序接原 Verilog TB | Rule→transaction→sampled checks→managed RTL | 自主 DFF 与 managed 生命周期均已实现；缺少保持全部事务采样语义的普通物理接口导出 | **未修复**：保留明确拒绝，禁止固定 commit 或删 error 来伪造通过 |
| C09 | 外部工具寻找顶层模块名 | `_emit.py::_target_files` | native 已输出 `root_rtl_name`，Python 只用于 CMake，未写入 receipt | 写入公开 receipt，同步严格校验和旧 receipt 替换兼容；不更改内部名称编码 |
| C10 | 固定外部端口名及 domain 映射 | `FinalEmitVerilog`、`FinalCppNames` | 无碰撞命名和 verified layout 已有；公共完整端口映射/alias接口尚未提供 | **未实现**：后续应从既有布局输出元数据/公共 wrapper；不把 q/d/clk/reset 带回源码 |
| C11 | 标量结果的带静态参数 Module | behavioral/structural route selection | mapping-return generic provider 可用，scalar-result Module 在更早阶段拒绝静态参数 | **未修复**：属于前端路径支持不一致，不能用单字段 Struct 掩盖；已与 C06 单独记录 |

## 四态实施补充

reset=X 的缺口通过共同根策略解决：`link --reset-semantics if_one` 表示仅已知1
激活复位。原raw domain、未知clock/enable/guard错误和全根提交屏障均保留；
不是按后端或题目名称隐藏故障。VerilogEval本轮88/88已实现题通过，包含此前失败
的 `m2014_q4c` 和 `lfsr32`；两个策略在结果中分别统计。

前端增加 `four_state`、仅诊断用途的 `case_eq/is_known` 和 System有限samples；
ACIR保留fixed类型、observer来源和根reset策略。两端常量/日志保存value/known/Z，
生成式System复用原runner。审查发现仅按字面量选仿真器会漏掉运行时除零产生X，
现按选中调用闭包中的observer、潜在X运算和资源保守选择Icarus；独立除零oracle验证。
未新增Signal接口、调度器或提高分析上限。

## C01、C02：修复事实传递，而非放松类型验证

`PythonImportRecords.cpp` 中的分支合并保留 Integer 来源和区间，但此前
bitwise 结果是否为 fixed 值依赖直接 AST `Constant`。因此
`value & (1 if condition else 3)` 中间有完整信息，到了最终绑定反而没有
可用的 unsigned 来源。`PythonImportAggregates.inc::unsignedBoundary`
拒绝无来源值是正确的，修复应在二元运算构造之前。

新增共享 `normalizeFixedIntegerPeer` 调用已有 `unsignedBoundary` 和
`NumericLowering.cpp::lowerExactIntegerBoundary`，保持非负、完整范围 fit、
Boolean、Enum、隐式缩窄及保存别名的限制。结果不能仅被标记 fixed 而不转换。

比较有两种 induction：`PythonImportBehavior.inc::lowerLiteralFor` 展开的
ordinal 是实际闭合 Integer；`PythonImportTransactions.inc` 的 typed Rule
则产生 `ac.source.index` 连接 `scf.for` block argument，其 `[lower, upper)`
区间不代表整个循环只有一个值。后者仅在确认真实 induction 及常量循环边界后
复用范围 fit；没有给运行时值附加虚假的 `closedSourceConstant`。
原实现已经允许同宽、非负、未经过数学算术中间值路径的Integer carrier与Bits比较；
本次保留该行为。不同宽度的普通运行时Integer不会因为此次归一化自动获得转换权限。
独立审查纠正了最初对此边界过宽的负例预期，没有为测试新增语言禁令。

## C03、C06：复用已有运算和参数验证

`ACIRBitsOps.cpp` 已验证等宽的 `shl/lshr`，两种 definition emitter 已生成
对应操作。`include/gfsim/four_state.h` 已处理完整计数、overshift 和未知计数。
前端现在对不同宽度的 fixed operands 采用 `M=max(dataWidth,countWidth)`，
零扩展后调用相同 IR，再取低 `dataWidth` 位。静态巨大 Integer 计数仍走原路径；
Boolean 和动态数学 Integer 不被当作 fixed Bits。

symbolic literal 修复只适用于实际 `BitsConstantOp`、闭合非负 MathInt、
与目标等价的 contextual 类型且目标宽度尚未绑定。实际 W 下常量是否放得下仍由
`HardwareAnalysis::verifyResolvedOperation` 验证。没有以默认 W 代替所有实例的证明。
标量 generic 模块的早期拒绝另列 C11，不能算作此项已支持。

## C04、C05、C07：两条循环路径与过度预留

普通 helper 调用 `lowerLiteralFor`，其循环体最终仍调用普通 `statements` /
`expression`。typed Rule 循环则在 `PythonImportTransactions.inc` 产生 SCF。
原 `bounded-for` fixture 的 `minimal()` 构造的是 `@rule`，因此该 gate 通过
不能说明 helper 循环可用。本次加入独立 undecorated helper controls。

helper 白名单与最大载荷公式可追溯至 `9a5d5b4da`；工作量记账后续在
`ff5667c5e` 有调整。这是历史能力边界与后续运算支持脱节的证据，不是人员归因。
移位复用原6-operation上界；静态除法取余的 reciprocal product 宽度为
`2W + ceil(log2(divisor)) <= 3W`，remainder至多7个运算，描述预留9个。

100-bit循环失败不是Table语义或后端容量不足：`TableQueryBudget` 被整个 importer
复用，错误名称因此容易误导。`payLoopEnvelope` 对每个语句累计预留
`8*P*L + 3*(precision+2)*L` 个常量/事实字节，`L=2*operations+map_arguments`。
`auditLoopStatement` 随后测量实际生成内容，但没有将预估与实际占用分离。

这部分需要按可能产生常量/数值事实的操作收紧描述，同时覆盖所有分支、aggregate、
helper、map复制和绑定转换。单纯删收费、改系数或只按当前成功例子退款没有充分证明。
本次只修准入和运算形状，保留该复杂问题及原始失败复现。

## C08：managed RTL 拒绝的真实来源

完整调用链如下：

1. `PythonImportTransactions.inc` 将可执行 Rule 归入 transaction；无显式 guard 时生成 true。
2. `SourceTransactionStateAnalysis.cpp` 保留 `CallableGuardControl`；totality证明只影响结果可用性。
3. `LowerTransactions.cpp` 生成 `guard_known` 的实际 `ac.expect` 及经过认证的 sampled metadata。
4. `HardwareEmitCommon::managesChecks()` 判定 System 或非空检查清单；空清单本身不会强制 managed。
5. `FinalEmitVerilog.cpp` 暴露/传播 phase、commit、error；definition emitter选择managed存储。

`benchmark/runs/verilogeval-final/dff8/rtl/` 的旧产物显示 guard 已是常量true，
但 clock/reset采样仍可能失败。`include/verilog/dff.v` 同时有 managed 和自主
posedge实现；后者不实现每个Work epoch检查未知clock及可撤销history。
`docs/rfcs/serial-rule-method/sampled-admission.md` 明确包括裸guard、total和无状态事务。
`HardwareSourceChecks`、`HardwareSampledAdmission` 验证精确检查清单。

因此不能仅删 `ac.expect`、在emitter过滤检查、强制 `managesChecks=false`、
固定commit许可或套带delay的System仿真器。普通物理接口导出需明确保留哪些故障/
采样行为，在共同IR中证明检查可消除或实现原子提交，沿用已有存储和transition逻辑。
后续验收必须包含同拍多owner、一方失败全部不提交、clock/reset X/Z、discard/retry
和无额外周期的原TB时序。

## 验证范围

定位索引（本次工作树）：

| 问题 | 精确拥有者 |
| --- | --- |
| C01/C02 | `compiler/lib/Compiler/PythonImportRecords.cpp:2724`，位运算调用点3400，比较调用点3559 |
| C03 | `PythonImportRecords.cpp:2810` 的动态fixed移位；既有 `include/gfsim/four_state.h:273` |
| C04 | `compiler/lib/Compiler/PythonImportBehavior.inc:384` 的preflight，845的二元运算描述 |
| C05 | `PythonImportRecords.cpp:2932` 的静态证明，`PythonImportBehavior.inc:1929` 的实际binder记录 |
| C06 | `compiler/lib/Compiler/PythonImportAggregates.inc:1096`；`compiler/lib/Dialect/ACIR/HardwareAnalysis.cpp:739` 的实际fit验证 |
| C07 | `PythonImportBehavior.inc:1152` 的预留，1335的事后测量；`compiler/lib/Compiler/TableQueryLowering.cpp:13` 的共享限额 |
| C08 | `compiler/lib/Compiler/HardwareEmitCommon.cpp:565`；`FinalEmitVerilog.cpp:583`；`include/verilog/dff.v:63`及119 |
| C09/C10 | `python/pycircuit/_emit.py:216`；`_generated_bundle.py:120`；`compiler/lib/Compiler/FinalEmitVerilog.cpp:549`及700 |
| C11 | `compiler/lib/Compiler/PythonImportBehavior.inc:2316` 的显式拒绝 |

已执行验证：

| 检查 | 结果 | 证据 |
| --- | --- | --- |
| 本工作树 CompilerDev build/install | 通过 | `build-second.log`、`install.log` |
| API gate | 23/23通过 | `docs/gates/logs/20261019-compiler-audit-api/` |
| Examples gate | 13/13通过 | `docs/gates/logs/20261019-compiler-audit-examples-fresh/` |
| 数值回归 | 5/5通过 | `numeric-regressions.log`：fixed-widening、exact-select/shift及其四态版本 |
| bounded-for、static-divrem | 2/2通过 | `focused-third.log`；该日志中的shift失败由下行最终结果替代 |
| fixed-unsigned-shifts完整回归 | 通过，94项受保护拒绝 | `fixed-shifts-final.log`；136条命令及candidate摘要已保存 |
| Receipt与benchmark runner单元测试 | 89通过 | `unit-final.log` |
| Public emit端到端 | 4通过 | `public-emit-second.log`，包含native/RTL执行及失败输出保护 |
| 自然写法ResBench | int_sqrt、fibonacci、left_shift三题通过原TB | `benchmark/runs/compiler-audit-natural/`；C++仅生成，未仿真；非官方Vivado/PPA评分 |

Examples第一次使用历史输出目录时，旧queue产物与当前4-result schema不兼容，
在验证旧产物阶段失败；未删除旧目录或放松发布校验。新的输出目录完整构建并通过。
Public emit测试另修复了过时的实例路径断言：源文件明确命名left/right，原测试却
期待匿名`__pyc_call_0/1`；6个epoch的独立数值/统计oracle保持不变。

新增用例覆盖宽度1/5/65/130、计数更窄/等宽/更宽、
高位计数、overshift、原生workers1/2、RTL及X/Z、symbolic实际绑定拒绝、helper loop、
无效源码的原产物保护。原始ResBench测试台不修改；C++生成与C++执行分别报告。
独立Astra架构审查完成；唯一P2（未知计数覆盖不足）已通过低/高位X/Z计数、
全X无Z输出和已知计数恢复测试关闭。实现与审查的五个产品文件指纹一致，
记录在 `candidate-evidence.json` 和 `review.md`。

尚未实现的导出能力、标量generic和C07资源优化不以测试迁移代替。
未运行全量nightly、综合/PPA，也没有测量仿真提速；不能将这批通过结果推广为
全部benchmark或所有旧API失败已修复。
