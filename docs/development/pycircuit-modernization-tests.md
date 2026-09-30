# 单一路线 hard break 验收规范

日期：2026-09-27。规划修订：6。状态：测试设计；不表示产品测试已执行。

配套：[主计划](pycircuit-modernization-plan.md)、[治理方案](project-governance.md)。当前产品路线是 pyCircuit Python source → MLIR → 同一硬件 IR → C++/Verilog；旧路线仅保留适用的历史 oracle、回归责任和负例，不再作为可用 authoring、编译或运行入口。

## 先冻结验收依据

C1/C2/C3 必须给出能力表，每行选择“新接口保持硬件行为”“采用 donor 语义替换”“用户批准退役/延期”，并记录正反例、backend、判断标准和批准修订。未分类能力阻塞切换，不能在实现遇到困难时临时改成 unsupported。

保留行为使用独立数学/逐拍状态机 oracle，并可对照旧 revision；变化行为按用户批准的新语义测试。旧 API 正例改为新接口正例或 retired-API 负例，不能用兼容实现维持旧语法通过，也不能删除语义断言而不提供继承用例。

## 验收矩阵

| ID | 要证明的主张 | 关键证据与反例 | 阶段 |
| --- | --- | --- | --- |
| T01 | 只有一个 source lowering 入口 | 唯一 driver/API 进入新 capture/import；按实际调用跟踪验证；旧 CLI/import/JIT/builder 编译失败或不可用 | M2/M5 |
| T02 | Python 只做 capture | source 中恶意/有副作用 import、decorator、constructor 不被执行；合法同构语法产生可追踪 capture；没有 type/effect/storage/scheduler 分析回退到 Python | M2 |
| T03 | MLIR 拥有语义分析 | 名字、类型、range、方向、alias、state owner、static/runtime、effects 有合法/非法 MLIR 测试；从 carrier 篡改不能绕过 | M2/M3 |
| T04 | 同一 final hardware IR 供两个 backend | 保存共同 IR 与各 backend 输入映射；final verifier 独立重建关键义务；RTL 不从 C++/runtime 恢复语义 | M2 起 |
| T05 | donor 的实际能力准确迁入 | 来源 revision/内容/许可、已批准设计、实际可执行微型测试；`@system`、整数格式、evaluation-check 缺口必须显式补齐或批准延期 | M0/M3 |
| T06 | Pythonic 接口实现批准合同 | 普通 class/method/record/annotation/控制流示例；rename 不改变方向，构造与 rule 含义明确；拒绝动态拓扑和非法 host 执行 | M2/M4 |
| T07 | 整数与类型没有悄悄改义 | 1/边界/64 位、数学运算和 range/overflow、显式截断、signedness；u64 中间结果超宽有规定；定宽 wrap 与 donor 数学整数的差异绑定批准 | M2/M3 |
| T08 | current/next、enable、reset 正确 | rule 读 cycle-start Q；跨 rule next 不提前可见；无写 hold、非零初值、reset/rerun；无源码顺序 winner | M2/M3 |
| T09 | Queue/state 与事务区分成立 | 普通多 reader state 不变成 consuming FIFO；Queue full/empty、同拍 pop/push、阻塞不消费；批准的多 owner group 无部分发布 | M2/M3 |
| T10 | drivers/冲突与失败行为可证 | declaration/traversal 置换、disjoint/overlap、显式算法/策略；失败注入检查 Q/pending/events；Evaluate/Check/Drive 名字不代替证明 | M3 |
| T11 | 两 backend 对齐硬件观察 | 同输入独立期望下 C++/Verilator 对齐 cycle/edge、latency、value/known/Z、reset 和背压；失配报最早 rule/instance/source | M2 起 |
| T12 | 所需硬件约束仍在 MLIR | clock/reset domains、组合环、CDC、memory 读写/延迟、RTL primitive 目录验证；不支持能力在 output 发布前拒绝 | M3 |
| T13 | 逐源编译没有退化 | 每 implementation source 独立 producer/semantic ACIR/interface；parent 无 child body仍可编译；repeated child state 独立；source→AC→TU→build graph→DUT 对应 | M2/M3 |
| T14 | identity/provenance 不混淆 | 相对 source path、nominal type、参数/实例身份保持批准约束；无 absolute-path/content-hash 语义身份；diagnostics 跨 pass 可定位 | M3 |
| T15 | backend 只生成代码 | 无 unresolved capture/static/control；合法化不新造仲裁、缓冲、状态；非法 final IR 即使直接进入 emitter 也拒绝 | M2/M3 |
| T16 | hard break 在三层成立 | 源码无旧语义引擎，CMake 无旧编译/链接目标，wheel 无旧模块/CLI/第二 C++ backend；无 旧路线 mode/fallback/alias | M5 |
| T17 | 每项接口变化获用户批准 | 精确提案、独立 approval-ready、用户明确批准与真实差异一致；从首次接口变化起，包括 governance runner CLI | 全阶段 |
| T18 | GFSIM 复用是实质迁移 | 每组件直接迁移/适配/补齐有代码与证据；不能只移植命名/治理、保留三条旧 lowering 实现 | M2–M5 |
| T19 | 独立编译与 SDK 真正可用 | 并行 TU 编译链接、仅包含公开 DUT header 的 consumer、relocation、输出失败事务、toolchain 来自当前 checkout | M3/M6 |
| T20 | 性能与规模有基线 | 相同机器/输入/选项的 stage time/RSS/code size/build/sim 数据；退化定位，不恢复旧 backend 兜底 | M0/M6 |
| T21 | 门槛迁移没有丢测试责任 | 每个旧 required assertion 有新 owner/替代或批准退役；decision/docs/examples/CI/installed-smoke 一致 | M4–M7 |
| T22 | subagent 交付证据独立 | exact candidate、文件 owner、actual model、独立测试/review、PM 集成验收；旧 hashes 不能验证新字节 | M1 起 |

T11 的共同能力与四态范围在 C2 冻结。不能只证明两 backend 都接受空模型或共享一个错误 emitter 的 golden。对 C++ 仿真独有的统计/日志等 host 功能单独测试，不伪装成硬件等价。

## 唯一编译路线的结构检查

同时检查三层，任何一层缺失都不能宣布清理完成：

1. **源码与调用图：** 旧 CAS/JIT/direct builder、旧 Python `_queue_compiler`、QueueGraph planner/text lowering、PYC C++ emission 的实现及分派已退役。简单 grep 是线索，必须对照 imports/call graph、注册和间接加载。
2. **构建与链接：** 查看 CMake target/dependency、编译 source inventory、bindings 和 backend registration；不能在隐藏 target 中继续编译第二引擎。
3. **安装与动态路径：** 在干净 relocated wheel/SDK 环境编译同一 Pythonic fixture，证明只走新 importer/MLIR trunk；旧 entrypoint 不可生成模型，旧 module/共享库无法被悄悄导入作为 fallback。

旧源码字符串可出现在历史决策/日志和明确 rejection fixtures，但不得以活跃 runtime 或测试依赖保留。最终只有一个 C++ model generator，Verilator 为 RTL 对照工具，不是第二 C++ codegen 产品路线。必要的 RTL 私有低层表示允许存在，但其所有语义来自共同 final IR。

## 测试分层与首个切片

### 源与 pass 单元测试

syntax capture 验证完整 AST/literal/span 和静态 import 输入，source importer 验证类型/效果/连接/state；每个 pass 有 input invariant、output invariant、no-op case、illegal case。优先复用 donor 微型 fixture 结构与本仓独立语义反例。

合法的 source 解糖可在 capture 保留原结构；决定寄存器、调度、宽度或 owner 的转换必须有 MLIR-side verifier 与反例。`scf`/`index` 等高层形式是否允许出现在中间阶段由合同决定，final backend endpoint 不允许未闭合的高层语义。

### 首个双后端端到端切片

以下 queue、external-port、reset 和 `@system` 场景属于更广的产品能力义务，不能推断为当前 M5 标量 profile 已支持；应由对应批准工作包与 gate 关闭。

由独立测试作者写逐拍期望：普通对象式叶模块有 typed integer、非零 reset、conditional enable，父模块实例化两次；另加单 Queue 的阻塞与同时 pop/push。不同实例不共享状态，同拍读旧 Q，未允许提交不改变输出或消费输入。

从唯一 Pythonic source 经逐源产物/link/final verify 同时生成 GFSIM C++ 和 Verilog，编译运行；再篡改 captured type、owner 或 driver 元数据，验证失败发生在规定 MLIR 边界。不能手写 generated C++ 来实现缺失逻辑。

### 扩面与故障注入

按批准能力加入 records/arrays/static parameters、hierarchy、memory/CDC、Queue/state/slot/多 lane、integer range/check、四态和复杂 reset。执行 rule 顺序置换、alias/owner 冲突、缺失 source interface、重复定义、错误 layout、output publication 中断、reset 后 rerun。

保存 capture、semantic/source-unit ACIR、linked/final IR、backend source inventory 和 gate-local observations，报告首个不同点。禁止为诊断向公开 ABI 加 consumer trace/cursor/ISA 接口。

## 当前路线命令

下列检查使用现行 source-unit compile/link/emit 和 common-final runtime。测试目录中保留的旧语义 oracle 仍可作为映射后的回归证据，但旧 CLI、脚本、CMake target 和 frontend 不再是运行入口。

```sh
pre-commit run --files <changed-files>
pytest tests/unit -m unit
python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md
python3 flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md --status docs/gates/decision_status_v6.md --out .pycircuit_out/preview/decision_status.json --require-no-deferred --require-all-verified --require-concrete-evidence --require-existing-evidence
mkdocs build --strict
git diff --check

PYC_BUILD_TESTING=ON bash flows/scripts/pyc build --build-dir "$PWD/.pycircuit_out/toolchain/build" --install-prefix "$PWD/.pycircuit_out/toolchain/install"
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
export PYC_BUILD_DIR="$PWD/.pycircuit_out/toolchain/build"
bash flows/scripts/run_examples.sh
bash flows/scripts/run_sims.sh
bash flows/scripts/run_sims_nightly.sh
bash flows/scripts/run_semantic_regressions_v6.sh
python3 flows/tools/check_m5_retirement.py --install-root "$PYC_TOOLCHAIN_ROOT"
```

These commands are validation entry points, not a claim that the gates passed. Bind captured output and statuses to the exact candidate before reporting evidence. The historical `tests/mlir/agentic-circuit/` and `tests/cpp/agentic-circuit/` inventories may retain semantic obligations while they are mapped to the current implementation; they do not imply an active `agentic-circuit` compiler route.

当前本地/CI closure 的路线 gate 包括 strict decision-status、unit/API/documentation checks、`run_examples.sh`、`run_sims.sh`、`run_sims_nightly.sh`、`run_semantic_regressions_v6.sh` 和 installed-payload retirement check。`run_agentic_circuit.sh` 已从当前 gate topology 移除；保留的语义断言应由当前 tests 与 gates 承担，而不是继续调用旧 runner。

donor 定向源到生成结果测试名称已核对，包括 `frontend_capture_syntax`、`acir_object_scalar`、`acir_object_records`、`acir_fixed_collections`、`acir_integer_codegen`、`acir_verify_final_driver_checks`。需在 donor 精确源码重新构建后运行，结果只用于该组件准入；不借用其二进制验证 pyCircuit。

## 独立性、证据和回退

实现、独立测试、代码审查分开实例；架构审阅先于用户接口批准。每份证据记录 cwd、source revision/dirty overlay、实际 toolchain/配置、命令、返回码、选中测试、日志和验收范围。摘要仅作外部证据身份，不进入产品 IR/类名。

新硬件语义以批准规范与独立 oracle 为准。旧 revision 只在合同相同的范围提供差分参考；不能要求所有旧语法/行为继续工作，也不能把两个 backend 互相同意当作唯一正确性证明。

冻结候选后 review/build 可并行只读；任意修复产生新候选，按影响重审重测。切换前保留 old revision 与日志，失败回退完整候选并从恢复源码构建，不保留产品 旧路线 switch，不覆盖他人的未提交文件。

本轮文档 checks 通过只说明规划文档验证完成。产品迁移完成必须同时有唯一管线证明、双后端语义验证、批准能力闭合和旧路径删除证据。
