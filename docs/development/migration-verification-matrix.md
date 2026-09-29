# 迁移独立 Verification Matrix

日期：2026-09-29。配套：[M1 C](../rfcs/migration/c2-m1-module-system.md)、
[执行 checklist](migration-agent-checklist.md)、[总体验收责任](pycircuit-modernization-tests.md)。
以下 Vxx 是路线图中的测试责任，不是已运行结果，也不是 M2 的全量前置。每项由独立测试作者
从合同构造；实现者不能把自己 emitter 的输出作为唯一 expected。

## 三种证据不能混用

| 标记 | 现在能运行什么 | 能证明什么 |
| --- | --- | --- |
| NOW-DOC | pre-commit、API hygiene、strict MkDocs、示例语法和普通数学 oracle | 设计文件一致；不证明新 compiler/runtime |
| EXISTING-BASELINE | 候选已有 source/header/type tests；必须重新构建并记录适用合同 | 原有有界能力，没有自动升级为 R1/M1 B |
| PLANNED-PRODUCT | 下面 V02–V49 的新 source/IR/runtime/backend 门槛 | 创建并真实执行后，才能证明对应新合同 |

W01 为当前选定范围创建 tests、selectors、负例 oracle 和必要报告。
该范围内缺文件/缺 target/未实现必须明确失败；未来范围可登记 backlog，
不要求预先创建所有测试。不能写 importorskip、xfail 或“命令不存在则跳过”
来制造通过。V00/V01 验证的是证据设施，不能替代产品行为。

## Case 矩阵

正例必须保存实际产物/观察；反例必须证明输入在前置阶段有效，并在
指定边界被拒绝。表内阶段是门槛分类，不擅自新增公开 diagnostic ABI。
W01 独立作者冻结实际 selector、诊断片段/既有 code、位置及输入摘要；
实现者不能因真实失败出现在错误阶段而修改期望。

| ID | 正例 / 独立 oracle | 必需反例或扰动 | 判定边界与主要产物 |
| --- | --- | --- | --- |
| V00 | cwd/HEAD/内容 manifest、工具与 build source 一致 | worktree core.worktree 误指主仓、混入别的 build binary | 构建前拒绝，保存 realpath/源树/工具绑定 |
| V01 | 一个真实通过、一个故意失败的设施 fixture 被完整报告 | 零匹配、missing target、skip、xfail、只有收集无执行 | test launcher 必须非零拒绝假绿灯；inventory 与 raw results |
| V02 | module/system 函数＋嵌套 rule 从真实 Python 到 source IR | 旧 class/method/return-registration 形式 | P1 接纳/拒绝位置正确，保存 source/body/header |
| V03 | 外层 reg read、nonlocal next、普通 local SSA | 未声明 nonlocal 的 local-unbound、写 static/child、hidden mutable capture | P1 scope/type 失败，不能猜测为寄存器写 |
| V04 | 带副作用的合法 capture 输入只生成 AST | import/decorator/module 内 sentinel 文件写入或 host 调用被执行 | capture 不执行，必要语义拒绝在 MLIR；sentinel 不存在 |
| V05 | 普通名字 queue/head/ready/push 不影响语义 | canonical/alias import 的框架 Queue/FIFO/Interface/Reg/ready API | 无公开 queue/interface DSL；按符号解析拒绝，不能封禁普通名字 |
| V06 | 固定 list 和显式 head/tail 计算保持原 reg 方程 | 按名字/形状私自改成 ac.queue、SimQueue 或新增 storage | P2/final 路由与 state inventory 无协议推断 |
| V07 | input/output 名字对调、下划线变化仍按实际 R/W | 按形参名字或调用时样例猜方向/位宽 | 按明确重命名映射比较 type/RW/alias；名称、origins、span 可以变化 |
| V08 | q:=q+1 的 handle→Q→proposal 无 SSA 环 | ac.dff/ac.dffe、旧 rule regions、错误 arity/target order | ODS/unit/final 拒绝旧 schema；合法 case 真正 parse/verify |
| V09 | bool、range(2)、signed i1 保持各自逻辑域 | 同宽 metadata 重标域、无 owning declaration 的 from_bits | P2/math verifier 拒绝；独立 0/1/-1 expected |
| V10 | init=3，D=99/E=false，空下一拍，D=7/E=true；Q 为3,3,3,7 | disabled D 在下一次 enable 重放、Write/Work 改 Q | 原语/双后端观察 pre/post Xfer；pending 清零 |
| V11 | reset 优先于 E，带 pending Reset 后初值恢复 | Reset 在 Xfer 外直接改 Q；无驱动路径误选 SimDFF | runtime verifier/trace；DFF 特化需完整 presence 证明 |
| V12 | 一次 physical Q slot、多次 source read origins；局部缓存不增加 read | 同名/alias 去重丢 source occurrence、Store base 被算作 Load | P3/L1 原始事件、实际 SSA 与 effects |
| V13 | nonlocal next/helper-return 分别有原始 use 和目标 | 先合并 yield 后反猜 target、漏 helper call/ordinal | A2/P3/linked 拒绝；RequiredUse 与 actual SSA |
| V14 | 未选短路危险分支不报错，已求值且 E=false 的错误仍报 | 用 proposal enable/fire 代替 source demand path | source-check path/数值 proof；Q 不变，正确失败阶段 |
| V15 | 未注册 rule 不贡献 active writers；静态死分支精确处理 | DCE 猜未读、未分析 child 就假设只读、重复注册丢 occurrence | P3 generic effects，不以保守摘要冒充 exact |
| V16 | concrete list [3,7,11] 恰三 reg，Reset 逐项恢复 | element 重复/缺失、换 initializer 副本、重排 materialized image | R1 source/link/materialization；同一权威 initializer |
| V17 | scalar/已接纳 concrete shape/control 正常 | 未闭合 N 猜自 caller、负/bool index、伪造 clk/reset | 规定 capability/type/control 边界；不新增假元素 |
| V18 | 相同 Q 读写不见 D；互斥 writes 合并、不同 targets 并行 | 必然重复写、可能冲突时 winner/priority/stall | P6 编译拒绝或 precommit fatal；无部分提交 |
| V19 | compiler-published interface 包含正确类型/effects/controls | 手写 flags/facts、直接复制 header 自称 body 重算 | P4/P5 比较真实 sources/SSA 与摘要 |
| V20 | parent 仅打开显式 child receipt/header | 隐式读 child Python/body、whole-core capture 后拆分 | 访问审计；隐藏 child source/body 后 parent 仍编译 |
| V21 | 正常 header/facade/import snapshots 可链接 | stale facade、错 nominal/range/default、漏 origins/child effects | P5 在 codegen 前拒绝；不只是 hash/文本相同 |
| V22 | source/header/depfile 成组发布、正常 header-only view | 半发布、恢复未完就降级读header、多输入跨 epoch | C3 publication/admission；旧输出保持、错误明确 |
| V23 | generic conservative 覆盖、specialized exact 重算 | 用 conservative 授权 writable alias、把同型 R/W ports 交换 | P5 exact closure/actual handle；无摘要自证 |
| V24 | 同 SpecKey 两个 OwnerRef 各有独立 hidden reg | 按 definition 合并实例、同实例两个 parents、递归实例 | link InstanceView/物理状态数与有界 trace |
| V25 | sibling/parent port/inner alias 只有一个 StateID | 无 owner、双 owner、端口自动分配副本、child 私有 state 外泄 | link reject；TestIncrement=4 reg、TestPipeline=5 reg |
| V26 | 每实例 Build/Work/Xfer/Reset/ReportStat 恰一次 | parent 递归＋system flat、system漏调由parent补调 | 动态计数＋NoCallChildren 静态调用检查同时通过 |
| V27 | 相同 reg R+W 两端、实参/controls 一致 | R+W 指向不同identity、漏/重 port、借用 reg 被 child reset/commit | link/ownership verifier；actual operands vs绑定 |
| V28 | 子 proposal 完成后 precommit 后序回传 | parent Work 偷读未完成 child proposal、merge 重算 index/D | Work barrier/依赖检查；源求值次数不变 |
| V29 | 孙模块失败，父与 sibling 有合法写但全部保持 | 漏 child error、permit 取反/断线、漏一个 state E 门控 | 共同 check/commit verifier＋RTL-private 投影 verifier |
| V30 | error 从原始 path/E 归约，无额外周期 | 用已经门控 E 计算 conflict/error，形成组合自依赖 | final/RTL-private rejects；实际网络检查 |
| V31 | 两 emitter 接受同一合法 final IR | residual source/queue/static、伪造 stage/proof、交换同型 target | 两 emit 重新 final verify，均在发布前失败 |
| V32 | all Work 只读旧 Q，all Xfer 只提交 frozen pairs | immediate Write 更新 current、Xfer 查询半更新其他 Q | 原语＋system cycle trace；遍历置换结果一致 |
| V33 | rule workers 写独占 proposal/check/event slots | 并发写同一 primitive pending、直接共享 Logger/Reporter | 真并行 fixture＋可用平台 TSAN；结构审计也必需 |
| V34 | 正常 Xfer 无分配/可失败用户回调，成功后才计周期 | commit 中做范围/冲突检查、半提交当合法失败 | 故障注入/分配审计；正常路径与灾难性 host 故障区分 |
| V35 | source failure 无 Xfer；Reset 后重跑相同 | Failed 仍接受 Step、旧 proposal/event 留到 reset 后 | runtime state machine；clock open-DUT continuation不在本包冒称通过 |
| V36 | assert/log/report 使用真实 value/path/site | observe 的同型值互换、漏/重 site、孤立正确 marker | observation/RequiredCheck verifier，真实 SSA 绑定 |
| V37 | 成功后从 --events 按稳定 key 交付 Work 时的值 | worker 直接输出、线程完成顺序当顺序、失败还发布普通 events | JSONL 的 IDs/spec/value/epochs 与独立期望对齐 |
| V38 | log关闭不改计算；report是非负gauge、静态唯一 | 参数除零被log关闭吞掉、report暗增计数、重复名、越界值 | source safety/report domain；明确错误出口 |
| V39 | statistics/Print/ReportStat 只观察稳定状态；--events 缺省不写stdout | report触发Step/递归children、sink失败回滚Q或改rule调度、已有/symlink输出被覆盖 | postcommit/Failed缓存；file/stdout选择与host I/O失败按固定合同 |
| V40 | synthesis去观察sink后硬件与source checks不变 | 把用于observe参数的危险求值检查一同DCE | 优化/投影前后 check闭包与硬件独立oracle |
| V41 | 单模块五周期 0,2,5,5,5；reg=4 | output端口重复reg、额外输入准备transfer、周期计数前移 | 同program C++/RTL真实运行＋独立期望 |
| V42 | 两级组合五周期 0,0,3,6,6；reg=5 | hidden relay reg、直接读 sibling D、模型搬进runner | 同program C++/RTL真实运行＋逐拍层次定位 |
| V43 | 两系统 commit=5、TERMINATED、completed=1、Reset/rerun一致；合法零rule系统一次Step→QUIESCENT/epoch0 | max_ticks=3/4仅无assert失败就PASS、空系统busy-loop/伪造终止、JSONL截断/缺或重复Result | 按case核对完成tuple、Step调用数和恰一个末尾Result；成功末拍已检查 |
| V44 | 固定源码改变Work/Xfer/线程调度时 Q/error/事件身份及顺序严格相同；源码重排按显式语义身份映射比较 | 调度改变winner/Q/事件；源码重排却拿旧AST identity/字节序列作expected | 两类oracle分开；源码重排后各自遵守新结构排序，映射后值/epoch对应 |
| V45 | 每源一producer/body/header，source groups与独立TU图一致 | 先整系统编译再拆、复制其他树产物、单体C++ fallback | CMake command graph＋AC/source/TU inventory |
| V46 | 自动runner与C ABI同一执行器；零rule系统QUIESCENT但模型保持Ready；有clocked rule无写仍推进到max_ticks | 额外driver、双root invocation、ac.dut再调root、把缺root当空系统、优化后错误QUIESCENT | 标准runner真实运行，root/规则活动闭包、调用图/访问计数 |
| V47 | Runtime-only relocated consumer可链接并执行 | 依赖LLVM开发headers、donor checkout、consumer设计或旧compiler | 干净安装/relocation，无源码树环境泄漏 |
| V48 | invalid final/发布失败保留旧输出 | 任意非零都算正确拒绝、emit先写半产物、跨平台skip算pass | stage/code/location＋文件内容；平台分别记录 |
| V49 | 同一候选新路线/退役manifest与能力矩阵闭合 | 仅grep零命中、删除oracle、安装包藏旧route/SimQueue | M5源码＋构建＋安装＋动态路由；范围外库保留后续责任与明确拒绝，不能用旧路线兜底 |

## 分阶段最小验证与扩展责任

下表拆分同一 Vxx 的交付范围，不改变上表 oracle。只有全项通过才能
声称该 Vxx 完整通过；子集验收必须列出实际 selectors。

| 验证责任 | 首次需要的最小范围 | 可后续补充 |
| --- | --- | --- |
| V32–V35 | M2：串行 Work 读旧 Q、Xfer 提交冻结 proposal、正常提交不失败、失败不提交、reset 重跑；冲突不可暗选优先级 | M6：实际多 worker/TSAN、调度置换及扩展故障注入 |
| V36–V40 | M2：当前用例的真实观察值/site、检查闭包、日志开关不影响计算；M4：已交付 events/report/sink 的正常与必要错误路径 | M6：更广的 host I/O 故障与平台矩阵 |
| V41/V42 | M2：两个完整源程序，同 final IR 的 C++/RTL 逐拍值、物理 reg 数及关键反例 | 更广用例由 M3 按需增加 |
| V43 | M2：基本 reset/失败不提交；M4：所交付 runner 的终止 Result、计数与重跑合同 | 未交付包装接口；当前路径已知错误不可延期 |
| V44 | M2/M4 仅声明串行执行，保留 conflict/no-priority 拒绝 | M6：实际并行、置换与源码重排完整矩阵 |
| V45/V46 | M4：所选使用流程的真实逐源生成/TU/CMake 与 runner；C ABI 若交付则验证同一执行器 | 尚未交付的包装/分发入口 |
| V47 | M4/M5：安装入口交付或切换前，在源码树外完成当前平台干净安装/import/run smoke | M6：扩展 relocation、平台和完整 SDK 矩阵 |
| V48 | emit/发布入口首次交付时：普通 invalid-final 拒绝与已有输出保护 | M6：崩溃、文件系统异常及跨平台故障矩阵 |
| V49 | M5：声明范围的新路线、退役清单和源码/构建/安装/动态路由一致 | FIFO 等范围外能力保留 owner、后续阶段和拒绝证据，不要求此时实现 |

## W01 固定的测试归属

以下全部是 PLANNED-PRODUCT，目录沿用现有仓库布局。新增文件必须
由独立测试作者拥有，测试 CMake 注册由集成者负责。目标缺失时不
尝试一个“差不多”的旧测试来替代。

| 测试文件 / CTest target | 覆盖 |
| --- | --- |
| `tests/cpp/agentic-circuit/Dialect/ACIR/RegContractsTest.cpp` → `ACIRRegContractsTests` | V08–V11、V16–V17 |
| `.../Dialect/ACIR/SourceFactsTest.cpp` → `ACIRSourceFactsTests` | V09、V12–V15、V18 |
| `.../Dialect/ACIR/InterfaceContractsTest.cpp` → `ACIRInterfaceContractsTests` | V19–V23 |
| `.../Dialect/ACIR/ModuleGraphTest.cpp` → `ACIRModuleGraphTests` | V24–V28 |
| `.../Dialect/ACIR/ProposalContractsTest.cpp` → `ACIRProposalContractsTests` | V14、V18、V28–V31 |
| `tests/cpp/agentic-circuit/Runtime/RegRuntimeTest.cpp` → `ACIRRegRuntimeTests` | V10–V11、V32–V35 |
| `.../Runtime/SystemLifecycleTest.cpp` → `ACIRSystemLifecycleTests` | V26、V28–V35、V39 |
| `.../Runtime/ObservationTest.cpp` → `ACIRObservationTests` | V36–V40 |
| `tests/system/test_lexical_module_units.py` | V02–V07、V12–V15、V20 |
| `tests/system/test_source_interface_semantics.py` | V19–V25、V27 |
| `tests/system/test_unified_register_backends.py` | V10–V11、V18、V29–V32、V36–V40 |
| `tests/system/test_system_composition_backends.py` | V24–V30、V41–V46 |
| `tests/system/test_migration_publication_sdk.py` | V22、V45–V49 |

表中的 `...` 只为排版，分别指相邻完整路径的
`tests/cpp/agentic-circuit`。dispatch card 必须展开成完整路径，不能把
省略号或 glob 当写权限。Runtime 新测试目录由 W01 明确建立；现有
候选的 old source/header targets 只作已映射回归。

W01 给每个 case 固定唯一 Vxx 映射和完整 selector；一个 Vxx 通常有
多个正反测试。native GTest 与 pytest 的 raw testcase inventory 都
必须能映射回它，不能仅记录一个 CTest executable 的“1/1”结果。

## Gate 命令和防假通过

PM 在 dispatch 中提供实际变量值：`MIG_CHECKOUT`、`MIG_BUILD`、
`MIG_EVIDENCE`、`MIG_PYTHON`、`MIG_CTEST_REGEX`。它们不得指向其他
worktree。目录创建、flags 与平台 profile 在 candidate manifest 固定。
下列命令只有 W01/Wxx 创建目标后才可用：

```bash
cmake --build "$MIG_BUILD" --target ACIRRegContractsTests ACIRSourceFactsTests
ctest --test-dir "$MIG_BUILD" --show-only=json-v1 -R "$MIG_CTEST_REGEX" > "$MIG_EVIDENCE/ctest-inventory.json"
ctest --test-dir "$MIG_BUILD" --no-tests=error --output-on-failure --output-junit "$MIG_EVIDENCE/ctest.xml" -R "$MIG_CTEST_REGEX"
"$MIG_PYTHON" -m pytest --collect-only -q tests/system/test_lexical_module_units.py
"$MIG_PYTHON" -m pytest -q -o xfail_strict=true tests/system/test_lexical_module_units.py --junitxml="$MIG_EVIDENCE/pytest.xml"
```

每包命令只选择它所需 targets/cases；上面两个 target 不是全包验收。
W01 注册包装还必须保存每个 GTest binary 的 `--gtest_list_tests` 与
`--gtest_output=xml:<unique-file>`；CTest 外层 passed 不会自动发现内部
GTEST_SKIP。不得共享多个 binary 的 XML 输出文件。

验收者逐项核对：

1. collected case 名单等于冻结 expected inventory，数量大于零。
2. 冻结范围内每个必需 selector 实际运行；raw XML 中 errors/failures/
   skipped 为零；pytest xfail/xpass 都不作为该责任通过。Vxx 的其余
   子例保持单独状态，子集通过不能写成整个 Vxx 通过。
3. process exit=0 与上述计数同时成立；collection-only 不算执行。
4. 负例 process 非零，但 crash、超时、工具缺失、未知 flag、解析器先
   拒绝不合法 fixture 不能满足预期的 semantic rejection。
5. C++/RTL lane 有可执行 DUT 的完成记录、逐拍结果与独立 oracle；
   保存同一份 final program 的内容摘要和两个 backend 输入证明。

M4 标准观察交付 lane 使用 `pycircuit_system --config <file> --events <new-file>`
及对应 RTL simulation runner 的同一出口。读完整 JSONL，核对所有
Event 的固定字段/顺序、epoch 和 values，且末尾恰一条 Result。
不得从 stdout 的随意 debug 文本猜事件，也不新增测试专用 public
model API；如果 sink 故障导致缺记录，测试失败而非跳过。

如果门槛需要的平台/工具缺失，报告 BLOCKED(platform/tool)，不要
改成 skip/pass。跨平台任务可以按批准 scope 分开验收，但 full matrix
仍保留未执行项。TSAN 可在有支持的平台承担 V33；不以当前平台不
支持为由声称真实并行已验证。

## 最小证据包

```text
candidate.json        # checkout/HEAD/完整必要overlay与文件hash、合同hash
toolchain.json        # compiler/runtime路径、源码绑定、版本、CMake配置
commands.json         # cwd/env选项/命令/exit/开始结束，不含凭据
inventory.json        # assigned Vxx -> exact collected testcase列表
ctest.xml + gtest-*.xml + pytest.xml
negative-results.json # case、pass/阶段、code/消息片段、source/site、exit
ir/                   # source/header/linked/final 与被篡改输入
observations/         # Q前后、epoch、事件、完成tuple和独立expected
build-inventory.json  # Python -> producer -> AC -> source group -> TU -> DUT
review.md             # 最终相同candidate的独立review与处置
```

证据摘要是工程身份，不能进入 ACIR、generated class 名或模型 ABI。
文件可按作用分开存放，但 candidate/toolchain/inventory 绑定不可省略。
出现修复后，相关证据作废并在新 candidate 重跑；保留旧失败以说明
问题，不把旧 PASS 复制到新目录。

## 首片与后续验证的边界（2026-09-29 修订）

用户要求有界交付、允许后续补齐。旧“首片完成 V02–V48 全部适用正反例”
要求取消；阶段归属见[checklist](migration-agent-checklist.md)。

M2 只绑定当前支持 profile 的 source/interface/link/寄存器/提交关键
验证，以及 V41/V42 两个实际系统的双后端证据。已有基础 reset、失败
不提交、无双重 reg 和必要无效 IR 拒绝仍必须验证。

V43 完整 runner/Result、V45/V46 需要的标准入口安排在 M4；V47 的
当前平台基本安装检查随入口交付。V44、扩展 V47 及 V48 故障矩阵安排在 M6。暴露某个入口时，其正常路径、必要错误
处理及已有输出保护仍须同步验证。不能延期当前执行路径上的实际
错误，也不能靠删测试、修改 oracle 或 skip/xfail 将失败计为通过。

每轮给出明确 selector 清单并记录 PASS / FAIL / DEFERRED / NOT RUN。
延期项保留原 oracle 和所属阶段，但不混入当前通过率。历史失败继续
保留；新范围验收不等于旧全量门槛通过。

M3 能力表、FIFO、外部 ABI、多域/四态和 V49 等仍是后续责任。完整
路线图完成和当前有界版本可交付是两种不同声明。
