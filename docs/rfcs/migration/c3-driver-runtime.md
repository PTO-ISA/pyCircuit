# C3：统一 driver、产物发布与 runtime/SDK

修订：A。状态：PM 草案，须由 architect 补齐并经独立审阅，尚未请求用户批准。配套 [C2](c2-mlir-contract.md) 和[已批准 C1](approvals/c1-pythonic-source.md)。本包不改变 C1 源语义，也不批准一般 clock/CDC、外部 typed DUT 或其他尚缺的硬件接口。

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
  "entry":qualified-definition-string,
  "files":[{"path":relative-string,"role":closed-role}],
  "source_groups":[{"source":SourceOwner,"files":[relative-string,...]}]
}
```

role 为 `header/source/cmake/rtl/runtime-glue/source-map`；按path排序，无重复，所有路径安全且闭合。receipt仅用于文件清单与管理，不包含源数学语义或可执行调度模型。

C++ 每implementation source保留一个 `.hpp/.cpp` group，type/value declarations按owning source发布headers；不生成一个跨全项目types头。另有core/interface/runtime glue；不生成per-case文件或参数值命名的公开类。生成CMake把各TU独立并行编译再链接一个DUT。

`generated/dut.h` 提供规范的模型ABI include入口，不依赖编译器开发头。模型source-owned headers可以供专门的gate C++ TU检查，但不承诺跨release的generated C++类ABI。RTL source group及source map同样按source owner归属。

具体 qualified source → C++/RTL 名字映射及异构特化的内部class组织是本草案要求 architect 补齐的内容，未闭合前不得请求本包批准。

## 目录事务、失败与并行

默认目标必须不存在；已有目标只有显式 `--replace` 且属于本driver的有效已发布产物才可更新。禁止覆盖未知目录、额外用户文件或symlink。source unit与generated bundle以**完整目录**为事务，不能先发布body再发布header。

建议协议：同父目录 staging 完成全部输出、结构验证和receipt后，取得对应输出的exclusive lock；读者取得shared lock。replacement检查receipt/实际owner与闭合文件集，通过备份目录与可恢复journal安装新目录；读者在lock期间看不到中间状态。单program.ac用临时文件和原子替换。

锁/journal是临时构建协调，不进入ACIR、模型身份或SDK。多个input locks按规范路径一致排序，避免死锁。构建时输入source与unit须冻结，不能与另一个writer共享输出目录。任意失败不能留下可被认可的半套产物；已有完整产物在可恢复情况下保留。

**待 architect 明确的事务细节：**锁的跨平台实现/范围、journal闭合字段与每个crash点的恢复决定、replace owner不同是否允许、CMake增量更新依赖及取消行为。这些不能留给实现者猜测。

## 保留一个模型 ABI，替换其实现

建议保留 `simulator/gfsim/include/gfsim/model_api.h` 当前 C ABI 的精确typedef、enum数值、64/16/24字节布局和 `agentic_model_query_v1(void)` signature，只有一份实现指向新的runtime。这个历史拼写是唯一规范ABI，不是旧模拟器的兼容桥；旧runner、第二模型API和旧codegen路径删除。

七项操作仍为 create/destroy/configure_json/reset/step/statistics_json/last_error。所有C边界捕获异常，不能让C++异常穿越。buffer归model所有，到下一次同model API调用或destroy失效；调用者复制所需内容。不同model可并行，同model调用须串行。

建议状态：Created → Configured → Ready → Completed 或 Failed。create分配并Build固定拓扑一次；configure只能Created。reset可从Configured/Ready/Completed/Failed进入Ready并恢复整树状态/临时量和时间；Build失败的create不返回可用model。失败step之后不能继续step，必须完整reset。

每次基础step推进一个default域的硬件周期：先全树EvaluateNext，再全树CheckNext，全部成功后全树DriveNext和Xfer。Source evaluation/precommit失败不Drive、不改变Q；不能通过部分先提交来避免检查。runtime内部/host异常与source失败的边界必须具体记录，不虚构全局rollback。

基础时间 `epoch_time` 是成功提交的default cycles，`epoch_delta=0`。reset后0。外部多clock/time模型在后续扩展精确定义，不通过隐藏delta scheduler保留旧执行路线。

configure_json/statistics_json 的旧形状、运行上限、QUIESCENT/TERMINATED/FAILED的细节及last_error诊断格式需由architect在批准前闭合。不能因签名未变就声称这些行为无需审阅。

## SDK 与平台

Runtime component只导出一套所需runtime link/include；CompilerDev导出native compiler开发资产和精确LLVM依赖。生成模型消费者只需Runtime及公开DUT入口，不能必须安装LLVM。wheel只安装 `pycircuit` 产品命令与其私有compiler helper；旧命令的存在不是过渡兼容要求。

保持当前Linux x86_64、macOS arm64、Windows x86_64平台目标和C++20宿主ABI配置。release版本号由实际候选确定，本包不创建tag或发布。SDK inventory/version-map的旧capability列表、工具列表与generator合同必须同步更新为单路线；具体schema字段调整在独立审阅前补齐。

## 验收与授权边界

必需证明：CMake每源独立producer；header-only parent；same-final-IR双target；helper/binary不来自另一worktree；unit与bundle失败/取消/并发发布；relocated SDK/Runtime-only TU；错误与reset/step状态；无旧CLI、frontend、runtime或第二C++backend安装资产。

本草案的待补内容是设计任务，不是让实现者自行选择的批准。architect补齐、独立review approval-ready、用户精确批准后才实施相应接口。
