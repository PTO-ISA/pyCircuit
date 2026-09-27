# C3：统一 driver、产物发布与 runtime/SDK

修订：C。状态：architect（interface_design，Astra xhigh）已补齐，PM 整理，待独立审阅；尚未请求用户批准。配套 [C2](c2-mlir-contract.md) 和[已批准 C1](approvals/c1-pythonic-source.md)。本包不改变 C1 源语义，也不批准一般 clock/CDC、外部 typed DUT 或其他尚缺的硬件接口。

## 单一产品接口

| 对象 | 本包建议 |
| --- | --- |
| Python distribution | 保留 `pycircuit-hisi`，每支持平台一 wheel |
| 源 namespace | 只有 `pycircuit` 的 C1 表面；退役单独 agentic_circuit authoring 和旧 JIT/builder |
| 产品命令 | 一个 `pycircuit` driver，compile/link/emit 子命令；不保留 acc.py/acc/pycc/agentic-circuit aliases |
| MLIR 实现 | 一个 repo-local compiler library/私有 helper，Python 只 capture/orchestrate，不选择第二语义链 |
| runtime | 一个 donor-derived execution model，库名保留 `libpyc6_runtime`；不把 donor ELF、全局 guest-memory/consumer adapters 搬入 |
| CMake | `find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime 或 CompilerDev)`；Runtime target 保留 `pycircuit::pyc6_runtime` |
| 工具链 | 保持当前 LLVM/MLIR 22.1.8；runtime/生成 C++ 按当前平台 C++20 profile；不为 donor C++11 代码另开一种产品 ABI |

选择保留已有可用名字，避免与语义收敛无关的重命名。原 `AgenticCircuit` CMake package 和第二 runtime targets 在切换时删除，不提供兼容 forwarding target。Runtime 不要求 LLVM/MLIR；CompilerDev 明确要求精确 pinned 版本。

## compile

```text
pycircuit compile -c <one-source.py> --source-root <root>
  [--package-prefix <dotted-prefix>]
  [-I <interface-unit-dir>]... -o <unit-dir> [--replace]
```

-c、source-root、-o 必需；只接受一个 source。prefix 默认空。-I 是明确的已发布 unit 目录，不是 Python sys.path 或隐式源码搜索目录。compiler 根据头文件 SourceOwner/export declarations 解析 source import，不能读取 child body/python 作为 fallback。缺失或歧义 interface 报错。type/declaration-only source 用同一个命令，无假 root。

输出恰为该 stem 的 `.ac`、`.interface.ac`、`.d`，以及一个仅供目录发布管理的 `unit.json`。C2 source owner 与 body/header 元数据必须一致；receipt 不是语义或验证权威。

```text
unit.json = {
  "kind":"pycircuit-source-unit",
  "source":{"package":string,"path":string},
  "files":{"body":"<stem>.ac","interface":"<stem>.interface.ac",
           "depfile":"<stem>.d"}
}
```

未知字段/文件、绝对/越界路径、symlink输出拒绝。stem 从 source basename 得出，不由 receipt 自行指定。depfile 是 Make/CMake 可读取的目标/依赖转义格式，可含本机构建路径，不参与模型身份；source内容与实际用到的接口文件为依赖。

一个 implementation source 的三个主体文件来自一个 CMake custom command；root单独调用。CMake/Ninja 并行处理依赖已就绪的不同单元，不能从一次whole-design invocation拆文件。初始项目可显式写 source-unit DAG；自动工程生成的接口另行定义，不能为此保留旧 builder。

## link

```text
pycircuit link <unit-dir>... --top <qualified-module>
  [--parameters <bindings.json>] -o <program.ac> [--replace]
```

必须显式列出完整实现与声明单元闭包。重复 SourceOwner、重复 qualified authority、缺 body/header、snapshot mismatch、未绑定引用、illegal instance recursion、非法参数及单位阶段失败均拒绝。

bindings.json 是 ordered array，按 root static 参数声明顺序，每项恰含 `name` 和 `value`。value使用 C2 StaticValue 的JSON形状，math integer为规范有符号十进制字符串，bool为JSON bool，record symbol为qualified字符串，fields/values保持声明顺序；无隐式bool/int转换。可省略有default的项；禁止重复、未知或connection参数绑定，MLIR补齐defaults并验证范围。`--parameters`缺省等价空数组。

基础 root 是普通 portless `@module`，可带静态参数；它不是一个伪造的library-unit root，也不需要尚未实现的 `@system`。外部typed ports/完整system仍是必须交付的后续合同，不能以本限制关闭完整目标。

link完成单元/header验证、特化、whole-design与所有必要安全lowering/final verifier，才发布共同硬件program.ac。它不调用C++或Verilog emitter。输出可单独传给两种backend；手改/不合法 final IR 在 emit 重新验证时拒绝。

## emit

```text
pycircuit emit <program.ac> --target cpp|verilog
  -o <generated-dir> [--replace]
```

同一份final输入分别运行backend前验证。cpp使用一套GFSIM-derived generator；verilog使用RTL-private合法化和复用的RTL emitter，不调用旧 QueueGraph/PYC C++ compiler。

输出目录恰有该target的 `generated.json` 以及其列明文件：

```text
generated.json = {
  "kind":"pycircuit-generated",
  "target":"cpp"|"verilog",
  "entry":SpecKeyJSON,
  "entry_source":SourceOwner,
  "files":[{"path":relative-string,"role":closed-role}],
  "source_groups":[{"source":SourceOwner,"files":[relative-string,...]}]
}
```

role 为 `header/source/cmake/rtl/runtime-glue/source-map`；按path排序，无重复，所有路径安全且闭合。files 不含 receipt 本身；目录恰为列明文件加 generated.json。分组文件必须存在且只属一个 source，runtime/core/CMake glue 可不属 source group。SpecKeyJSON 使用 C2 definition 和 ordered StaticValue arguments；整数为规范十进制字符串。receipt仅用于文件清单与管理，不包含源数学语义或可执行调度模型。

C++ 每implementation source保留一个 `.hpp/.cpp` group，type/value declarations按owning source发布headers；不生成一个跨全项目types头。另有core/interface/runtime glue；不生成per-case文件或参数值命名的公开类。生成CMake把各TU独立并行编译再链接一个DUT。

`generated/dut.h` 提供规范的模型ABI include入口，不依赖编译器开发头。模型source-owned headers可以供专门的gate C++ TU检查，但不承诺跨release的generated C++类ABI。RTL source group及source map同样按source owner归属。

### 生成名字与特化组织

生成名字不参与 C2 语义身份。统一 ASCII lower-snake legalization：ASCII 大写转小写，前字符为小写/数字时插入一个下划线；源分隔符变单下划线，去除首尾重复分隔符；非 ASCII 转 Unicode codepoint token（如 u8bf7），不使用内容摘要。数字开头和目标语言关键字加固定 pyc_ 前缀。legalize 后的 scope/path 碰撞拒绝，不追加流水号、参数值或内容摘要。

C++ namespace 对应 package/import-module components，类名来自 source class，文件路径来自 SourceOwner。局部 state/instance/rule 名使用本地 source 名和必要角色名。每个 implementation source 一个 hpp/cpp group，declaration-only source 只有 header；donor `CodeGenDriver.cpp:242–262` 当前的全局 ACIRTypes.h 和按 class 分文件必须适配。

C++ 一个 module 对应一个模板族：

```cpp
template<class... StaticArguments>
class bank;
```

每个 linked SpecKey 为其 explicit specialization，声明均在该 source header，方法实现均在该 source cpp。禁止参数值命名的额外公开类或 per-case 文件。唯一小型 runtime header 提供静态身份类型：

```text
static_boolean<true|false>
static_integer<canonical_decimal_literal>
static_record<NominalTag,FieldKeys...>
static_list<ElementKeys...>
```

整数 literal 用 C++20 structural-string NTTP 表示任意精度身份，不实现 runtime bigint；bool/int、record nominal 和字段次序保留 C2 区分。这是生成 C++ 类型参数，不是新的 Python family DSL。

RTL 每 implementation source 一个按 qualified source-module path 命名的 module family；static parameters 选择互斥 generate 分支。record/list 叶按 C2 声明/ordinal 次序展开，nominal/字段顺序/list 长度由已验证声明固定，不另设可覆盖参数。整组叶必须匹配一个完整 SpecKey，不能逐叶分别属于允许集合就接受。未声明 named override 由 elaboration 拒绝。源 bool/int/nominal 检查仍在 MLIR，RTL 是固定类型接口的物理表示。

### RTL 参数接纳先于缩窄

公开参数无显式数据类型、无 packed range，保留覆写表达式自决定位宽/signedness，例如 `parameter entries = 32'sd2;`。禁止 `parameter logic [2:0] entries = 3'd2;` 之类先将 override 10 截成 2 的形式。对每个原始 integral 叶 R 和每个 admitted 数学常量 v，规则为：

```text
RW = $bits(R)
CW = max(RW + 1, signed_bits_required(v))
known = ((^R) !== 1'bx)
negative = (R < 0)
extended = {{(CW-RW){negative}}, R}
match(v) = known && (extended === exact_CW_bit_pattern(v))
```

不先缩窄 R；RW+1 也容纳 unsigned 最大值的非负 signed 表示，signed 负值符号扩展，unsigned 高位不当负号。四态 case equality 拒绝 X/Z；非 integral 参数不能参与 reduction/concatenation，必须 elaboration 失败。生成 actual literal 使用足够位宽和明确 signedness，自身不得截断。

Bool 叶仅接纳已知数学 0/1，拒绝 2、-1、X/Z；integer 与完整合法 SpecKey 对应值精确相等，拒绝越界、v+2^N 和负数截断别名。选中完整合法分支后，内部只使用已验证 SpecKey 有限常量/布局，不再从 raw 参数窄化重建。

非法 generate 分支同时包含故意未定义的保留 module 与 fatal：

```systemverilog
pycircuit_rejected_static_configuration reject_configuration();
initial $fatal(1, "unadmitted static configuration");
```

生成器、用户设计和 primitive catalog 不得提供该保留 module 定义。支持流程要求 elaboration/lint 的 unresolved-module 错误；simulation 先 elaboration，即使工具允许未解析模块也须 fatal 非零；synthesis 对完整 selected hierarchy 严格检查（例如 Yosys hierarchy -check），不得把此保留实例当黑盒。未启用严格检查的流程不属于此发布合同，不能声称参数拒绝成立。仍是一份 RTL、一个 verilog target，不另造执行路线。

工具级反例必须在 elaboration/simulation/synthesis 分别非零失败：linked entries={2,4} override10、合法值+2^旧窄化宽度、负数截断别名、bool2/-1/X/Z、record/list 逐叶各自合法但组合无对应 SpecKey、域外宽整数。只检查文本含 fatal 不够。可复用当前 VerilogEmitter.cpp 的 generate-if 结构，但必须替换有类型参数的接纳方法，不保留旧 finite-case source compiler。

## 目录事务、失败与并行

默认目标不存在；显式 --replace 只更新本 driver 合法已发布且 owner 相同的产物，不覆盖额外用户文件、损坏产物或 symlink。没有 --force。source unit、generated bundle、单 program.ac 均使用以下同一个协议。

对 parent/name 使用固定控制目录：

```text
parent/.name.pycircuit-publication/
  owner.json
  owner.json.tmp
  lock
  journal.json
  journal.json.tmp
  stage
  previous
```

owner.json 恰为 `{"kind":"pycircuit-publication-control","destination":"name"}`。控制项只能为上面七种固定名字，目录/文件类型必须相应，拒绝 symlink/reparse point。临时文件可能因中止不完整，不据此当成外来目录。lock 永不删除/替换。

bootstrap：创建或打开控制目录；未初始化时创建/打开固定普通 lock 并取独占锁。owner 不存在时只允许 lock/owner.json.tmp（或空目录），出现 journal/stage/previous 拒绝。在锁内完整写 owner.json.tmp、flush、校验 marker 后原子 rename 为 owner.json，再同步目录，随后才可创建 journal/stage。空目录、仅 lock、半写 owner tmp 均按此流程重入。正式 owner 不原地写，若已存在而不正确则拒绝；正式 owner 存在但 lock 缺失视为损坏，不重建新锁 inode。

journal 更新固定使用 journal.json.tmp→journal.json，只有正式 journal 权威：正式有效则忽略/清理 tmp，按正式 phase 恢复；无正式 journal、仅 tmp 且无 stage/previous，意图未生效，删 tmp 保持输出；无正式 journal 却有 stage/previous 则拒绝猜测；正式损坏不能以 tmp 替代。严格保证正式 preparing 已持久化才创建 stage。

Linux/macOS 对同一 lock 使用共享/独占 flock；Windows 对 byte 0 长度 1 使用 LockFileEx，共享读/独占写。退出由 OS 释放锁，不能依据 PID/时间戳抢锁。初始保证限定支持该锁及同卷 rename 的本地文件系统，不宣称网络文件系统同样可靠。

一次命令提前收集全部输入共享锁和输出独占锁，按规范绝对路径排序获取。输入输出相同或祖先/后代冲突拒绝。持锁完成快照/生成/发布，不升级锁。读者发现未提交 journal，释放当前锁集合，以该目标独占锁恢复，再重新获取原锁集合；丢弃之前部分输入。committed 状态可按下述规则直接验证读取。

### Journal 与提交点

```text
Journal={kind:"pycircuit-publication",
  artifact:"source-unit"|"generated"|"program",destination:String,
  owner:PublicationOwner,had_previous:Bool,
  phase:"preparing"|"prepared"|"rollback_restore"|"rollback_cleanup"|"committed"}
PublicationOwner={kind:"source-unit",source:SourceOwner}
  | {kind:"generated",source:SourceOwner,
     definition:QualifiedSymbol,target:"cpp"|"verilog"}
  | {kind:"program",source:SourceOwner,definition:QualifiedSymbol}
```

字段闭合且必需，stage/previous 固定路径不能由 journal 指定。静态参数值不属 publication owner，同 root 可换参数 --replace；不同 owner/definition/target 必须另用输出路径。旧 receipt/IR 与实际文件集验证通过才更新。锁/journal 不进入模型身份或 SDK。

事务严格顺序：获取锁、恢复并验证旧输出；持久化 preparing 后创建 stage；写完且验证全部文件/receipt，flush 后持久化 prepared；旧输出 rename 到 previous；stage rename 到目标；验证新目标后持久化 committed（提交点）；删除 previous，最后删 journal。

journal 更新写临时文件、flush、原子替换；POSIX 同步有关目录。Windows 同卷 rename/相应 flush，对 sharing violation 有界重试。现有 DirectoryPublication.h:19–47 仅支持目标不存在的移动，不能当作已有 replacement/recovery。

### 可重入恢复

所有恢复 mutation 持独占锁，每次 phase 更新仍用固定 tmp 原子发布。正常发布的 preparing 表示目标从未移动；恢复时先持久化 rollback_cleanup，再清理 stage。

prepared 先验证路径组合来自合法发布状态，再持久化 rollback_restore，不能先移动/删除。rollback_restore 重入同一恢复动作：

| had_previous | 路径组合 | 恢复 |
| --- | --- | --- |
| true | previous 在、目标缺失 | previous rename 回目标 |
| true | previous 在、目标在、stage 缺失 | 新目标 rename 回 stage，再恢复 previous |
| true | previous 缺失、目标在、stage 在 | 旧目标未移动或恢复已完成；保留目标 |
| true | 其他组合 | recovery error |
| false | previous 在 | recovery error |
| false | 目标在、stage 缺失 | 目标 rename 回 stage |
| false | 目标缺失、previous 缺失 | 无旧产物需恢复 |
| false | 目标和 stage 同时在 | recovery error |

恢复后 true 要求完整旧目标、previous 不存在，false 要求目标/previous 均不存在；stage 可存在。随后持久化 rollback_cleanup。

rollback_cleanup 要求目标存在性等于 had_previous，存在则验证完整性/owner，previous 必须缺失。stage 允许完整、部分删除或不存在；完成 stage/tmp 清理，最后删正式 journal。这样恢复的 rename、phase 更新、删除各处再次中止均可重入。

committed 要求完整新目标且 owner 正确、stage 缺失，previous 允许完整/部分清理/缺失，继续清理。新目标缺失/损坏或不可能的路径组合为 recovery error，保留证据，不猜测。

提交后持续清理 I/O 错误：保留 committed journal 和完整新目标，原发布命令仍成功，输出固定 warning `publication_cleanup_pending`；不能误报回滚。读者持共享锁验证并读取 committed 目标，无需等 previous 清理成功；新 writer 必须先清理，失败则本次新发布非零退出而不改新目标。提交前恢复 I/O 失败则非零退出，保留恢复资料，不认可 prepared/rollback 目标。

保证针对进程中止/可观察 I/O 故障，不承诺硬件违反 flush 时断电原子性。提交点前取消回滚、exit130；提交点后清理并按已提交成功返回（清理失败按上述 warning）。强制终止由下一次访问恢复。

### Program 读取入口

先检查控制目录，不能先判 program.ac 存在性。控制目录存在则先锁/marker/journal：prepared/rollback 先恢复，再打开目标；committed 验证并读取；控制目录损坏报 recovery error，不 fallback 到 unmanaged。

控制目录不存在时，可把 program.ac 作为外部不可变输入：检查无控制目录→打开读取完整文件并核对前后身份/大小/修改状态→再次检查控制目录不存在→完整 final verifier。途中出现控制目录则丢弃 snapshot，重走 managed；打开失败也先复查控制目录，仅仍不存在才报告真正缺文件。外部提供者不得在读期间原地改写，这不冒充 managed 的原子保证，也不强制在只读输入目录创建 lock。

### 输入快照与 CMake

compile 一次读取自身 source snapshot，只读明确 -I 单元的 receipt/header，不读 child Python/body。link 在共享锁下读完整 body/header 闭包；emit 在共享锁下读 program。内存快照建立后不重读部分内容。普通 source 非 managed：前后文件身份/大小/修改状态改变则拒绝，不拼接版本。

CMake custom command 的 OUTPUT 是 body/header/unit.json，DEPFILE 是 stem.d，DEPENDS 为本 source、实际使用 interfaces 及 receipts、compiler/helper/toolchain config。命令可以始终 --replace，首次不存在正常创建；root 独立 producer。link 依赖全部 body/header/receipts/参数文件/compiler，emit 依赖 program/backend tools；各 TU 编译依赖 emit 完成。

同一图按依赖生成/消费；emitted bundle 在 C++ 编译期间保持不变。并发独立 build graph 使用不同 generated/build 目录，不能宣称发布锁自动保护不参与协议的任意外部 compiler。

## 一个模型 ABI 与精确生命周期

保留 `simulator/gfsim/include/gfsim/model_api.h` 的 typedef、enum 数值、64/16/24 字节布局（64-bit host ABI）和 agentic_model_query_v1()，只保留一份指向新 runtime 的实现。保留布局不等于保留全部旧行为，以下行为需本包批准。

| 操作 | 合法状态 | 成功结果 |
| --- | --- | --- |
| create | 无 handle | Build 固定树一次，Created |
| configure_json | Created | 原子安装配置，Configured |
| reset | Configured/Ready/Completed/Failed | 整树恢复，时间/统计清零，Ready |
| step | Ready | 下述 RUNNING/QUIESCENT/TERMINATED/FAILED |
| statistics_json | Ready/Completed/Failed | wrapper 已提交统计快照 |
| last_error | 任意有效 handle | 保留诊断，状态不变 |
| destroy | 任意有效 handle；允许 null | 释放且不抛异常 |

参数/状态错误不推进仿真、不把健康模型变成 Failed。configure 解析失败留在 Created 可重试；reset 失败进入 Failed，仍可再次完整 reset。create 的非空输出指针先设 null，Build 失败 RUNTIME_FAILURE、不交付半初始化 handle；无 handle 不新增全局 last-error API。

所有 C 边界捕获异常；不同 model 可并行，同 model 串行且不可重入。默认日志不写共享 stdout/stderr，错误进 per-model 缓存。Buffer 只读，到下一次同 model API 调用或 destroy 失效。

### Step 与失败边界

每次接纳的 step 为全树 EvaluateNext → 全树 CheckNext → 全树 DriveNext → 全树 Xfer → 更新成功提交计数。

- source/check 失败：不 Drive，Q 不变，返回 RUNTIME_FAILURE/FAILED，进入 Failed。
- Evaluate/Check 内部异常：Q 不变，进入 Failed。
- Drive 内部异常：可能已有暂存 next，Q 尚未发布，进入 Failed。
- Xfer 内部异常：可能部分发布，不承诺 rollback；必须 Reset。
- 普通 DFFE commit 应实现为无分配/无失败；内部异常条款不替代该义务。

基础 epoch_time 是完整成功提交的 default cycles，epoch_delta=0，reserved=0，reset 后为 0；计数溢出在 Evaluate 前失败。donor SimSystem.cpp:175–190 在 Work 前加 cycles_，不能原样照搬到此 ABI。

| StepState | 精确行为 |
| --- | --- |
| RUNNING | 成功提交一周期，未达到上限 |
| QUIESCENT | 无 registered rule、无 pending work，不推进时间，保持 Ready |
| TERMINATED | 本次成功提交达到配置上限，进入 Completed |
| FAILED | 本次执行失败，进入 Failed |

Q 未变化不构成 QUIESCENT；clocked rules 无 active write 仍 RUNNING。完整 system/外部输入/资源调度的终止扩展另批。

step 的 struct_size!=24 返回 ABI_MISMATCH，不写其余结果；null 指针 INVALID_ARGUMENT。其他接纳前错误也不改 result；接纳后的失败写 FAILED 与最后完整提交时间。旧 QueueGraphGenerator.cpp:9056–9076 将失败归入 Completed、错误 size 返回 INVALID_ARGUMENT，这些行为明确替换。

## 配置、上限、统计与错误

沿用 model_input.cpp:179–249 的 canonical config parser 和形状：

```json
{"deadlock_window":null,"max_domain_cycles":{"default":100},"max_ticks":100,"schema":"agentic-model-config","version":"1"}
```

另接受 `{}`；允许末尾恰一 LF，其余字段顺序/空白按现有 canonical 格式。未知/重复字段、非法 UTF-8、负数、非整数、溢出拒绝。max_ticks 为 null 或 1..UINT64_MAX；max_domain_cycles 为空或仅 default:1..UINT64_MAX；同时指定取较小，相同时 stop_reason 优先 max_ticks。null/空不设用户上限。

foundation deadlock_window 必须 null；非空 INVALID_ARGUMENT 加 capability 诊断。合法 hold 不以无 Q 变化判死锁；资源进度协议在事务扩展完成。

### statistics_json

保留现有统计数组，每行闭合字段（QueueGraphGenerator.cpp:8905–8934）：

```text
buckets:Array<{count:u64,upper_bound:u64}>
count:u64
kind:"counter"|"gauge"|"histogram"
last_update:{delta:u32,time:u64}
maximum:u64
minimum:u64
name:String
object_path:String
sum:u64
value:u64
```

foundation 恰提供两行，按 (object_path,name) 排序：@runtime/cycles 为 counter，value 是成功提交周期数；@runtime/stop_reason 为 gauge，value 0=none、1=max_ticks、2=max_domain_cycles。buckets 空，count/sum/minimum/maximum 均零，delta 零；last_update.time 是 value 最近变化的提交时间。Reset 全零。

Failed 只读 wrapper 已提交统计，不遍历可能部分提交的对象树。额外 source statistics 后续按批准语义添加，不把模型 state 当 stats 输出。

### last_error

无诊断返回 size=0；非空为 UTF-8 canonical JSON，字段固定：

```text
{code:String,message:String,
 phase:"api"|"configure"|"reset"|"evaluate"|"check"|"drive"|"xfer"|"statistics",
 instance:String|null,source:SourceSpan|null,check_id:CheckID|null}
```

code 闭合集合：invalid_argument、abi_mismatch、invalid_state、source_check_failed、driver_conflict、runtime_failure、reset_failed、counter_overflow、statistics_failure。

成功 configure/reset/step 清除旧诊断；statistics/last_error 不清除。Failed 保留首个执行失败，非法继续 step 不覆盖；reset 失败可替换为 reset_failed。正常 TERMINATED 无错误。构造诊断失败使用预置有效最小 runtime_failure JSON，错误处理不得再抛异常。

status code 沿用 header 0–4：执行异常 RUNTIME_FAILURE，JSON/参数错 INVALID_ARGUMENT，生命周期错 INVALID_STATE，布局错 ABI_MISMATCH。

## Runtime 迁入边界

迁入 donor 固定对象树、Build/Freeze、current/next 和分阶段求值，修正 check 门控和周期计数。SimSystem.h:23–89 有容量声明/冻结机制，但该文件 127/149 行的 SimMemory/GetMemory 全局 guest-memory、ELF/ISA/workload/consumer adapters 必须拆除。旧 QueueGraph scheduler、第二 wrapper/runtime library 一并退役。

唯一库 libpyc6_runtime，唯一 target pycircuit::pyc6_runtime；内部可沿用 gfsim namespace，不生成第二 runtime target。

## SDK 与平台

Runtime component 只导出所需 runtime link/include，不要求 LLVM；CompilerDev 导出 native compiler 开发资产，精确要求 LLVM/MLIR 22.1.8。消费者只需 Runtime 和公开 DUT 入口。保持 Linux x86_64、macOS arm64、Windows x86_64 及当前 C++20 profile。

SDK manifest/version-map 顶层形状、platform 表、pycircuit-hisi distribution 和 runtime_abi="1" 保留；generator_abi 改为 "2"，反映 source-unit/driver/generated-family hard break。sdk-manifest.schema.json capabilities 精确改为：

```json
["pycircuit-pythonic-source","pycircuit-source-units","pycircuit-cpp","pycircuit-verilog","pyc6-runtime-v1"]
```

release-index、consumer-lock 必须与 platform manifest 的 capability 集合/ABI tuple 完全一致，旧 generator_abi=1 拒绝。schema-local version 保持 "1"，外部 source/product pin 和 generator ABI 选具体合同，不把 release identity 塞进 ACIR。

inventory 只安装一个公开 pycircuit 和明确私有 compiler helper；拒绝 acc.py/acc/pycc/agentic-circuit、旧 runtime 库和 forwarding targets。pycircuitConfig.cmake 增加真实 Runtime/CompilerDev branches，移除 PYCIRCUIT_PYCC_EXECUTABLE，只提供唯一 driver 路径；现有 cmake/pycircuitConfig.cmake.in:1–9 尚无该隔离，不能声称已实现。

Decision 0267 中禁止 ownership/publication manifest 的对应条款须明确 supersede，允许本文仅文件管理的 receipt/journal；其不使用内容指纹、结构化语义身份要求继续有效。不能仅改名 receipt 规避旧条款。release 版本由实际候选确定，本包不创建 tag 或外部发布。

## 验收与授权边界

必须验证：每个 journal crash point、提交点前后取消、foreign-owner replacement、reader/writer 并发、Windows sharing violation；每源 CMake producer/header-only parent；生成模板族及 RTL 未知配置拒绝；同 final IR 双 backend；全部 ABI 状态/错误码、source 失败无 Q 变化、内部 Xfer 失败后 Reset；limits 边界/QUIESCENT 反例；Runtime-only relocated consumer；SDK 负向安装扫描。

当前仅为 architect author 提案，须独立 approval-ready 后请求用户精确批准。本包不批准 memory/CDC/四态、external typed DUT、多 clock/system 的缺失扩展，也不代表完整迁移验收。
