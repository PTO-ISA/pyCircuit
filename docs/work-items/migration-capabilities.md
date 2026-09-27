# 迁移能力与退役矩阵

状态：能力清单与验收责任；不是产品完成声明。[C1-C](../rfcs/migration/approvals/c1-pythonic-source.md) 与 [C2-C/C3-C](../rfcs/migration/approvals/c2-c3-foundation.md) 已获用户精确批准。三份冻结合同覆盖基础能力，尚未覆盖的扩展单独审阅批准。完整范围见[主计划](../development/pycircuit-modernization-plan.md)，实施状态见[账本](single-route-migration.md)。

合同批准、实现、执行证据分别记录。当前已独立验证私有单文件捕获和 C2-F01/F02 基础 MLIR 属性、类型、静态值和记录匹配；尚无新编译主干的完整执行闭环。现有旧路线的基线不证明新路线已保留该能力。下表每行都必须在完整迁移验收前关闭，未批准扩展不能因为 donor 缺失就默认退役或延期。

## 能力与证据责任

| 能力 | 现行依据与迁移处置 | 合同/批准状态 | 实现/证据状态 | 验收与 owner |
| --- | --- | --- | --- | --- |
| 单一 Pythonic module/rule 前端 | 0148/0150；用普通对象源替换 CAS/JIT/builder/function-style Agentic 接口，保留所需硬件能力 | C1-C 源合同已批准；system 不在本行 | 私有单文件捕获已验证，public route 未切换 | frontend/import；[36 focused、253 unit 与独立 review](../gates/logs/20260927-c1-capture/review.md)，仍需 importer、旧入口拒绝与 installed route |
| 数学整数/范围/位运算 | 0246/0247；从隐式定宽回绕改为数学中间值、边界检查与显式 mask，属于已批准语义替换 | C1-C/C2-C 已批准 | [F01](../gates/logs/20260927-c2-f01/integration/results.md) MathInt 与 [F02](../gates/logs/20260927-c2-f02/integration/results.md) 最小类型/静态值匹配通过；运算/proof/双后端未实现 | MLIR + independent tests；signed/floor div/rem、短路错误、shift、u64 超宽及低位 proof |
| bool 与普通 nominal record | 0212–0215、0255/0256 的类型/值能力保留；不可空、完整初始化、不可原地更新 | C1-C/C2-C 已批准 | F02 结构与注入 resolver 匹配通过；constructor/header/record lowering 未验证 | frontend/MLIR；默认值/kwargs、身份/字段/投影/范围、打包布局和双后端 |
| Enum 与完整 tuple/value array | 0252–0256 相关值类型能力不能随旧 DSL 消失 | 扩展未批准；C1 明确未定义完整绑定 | 完整迁移阻断项 | architect + MLIR/tests；编码/非法值、不可变聚合、布局、两 backend 独立 oracle |
| fixed owned/reference list 与静态循环 | 0252–0254 的固定集合硬件能力采用 C1 新表达；负 index 拒绝、声明正长度、逐 ordinal effects | C1-C/C2-C 已批准；不等于全部 tuple/array 扩展 | 未实现新 source→MLIR 路径 | frontend/MLIR；零次循环局部值、bounds、alias、非均匀 reset image、2/4 特化 |
| 普通 constructor 静态配置/多特化 | 0275–0278 的 typed identity/实例独立性保留；人工 finite_cases/case 源接口及旧 carrier 退役 | C1-C/C2-C/C3-C 已批准 | Bank 2/4 独立 oracle 已定义，backend UNRUN | MLIR/backend；同参数复用代码、异参数同时实例化、同源一个文件组，无 per-case 文件 |
| 依赖参数的端口/record 类型 | 0275–0278 中 family/interface 能力需新绑定；不把普通 static 参数批准扩张至 dependent types | 扩展未批准 | 完整迁移阻断项 | architect + MLIR；dependent shape/type、同宽不同 nominal type、header-only parent |
| 模块层级/逐源独立编译 | 0270/0274；保留逐源 body/interface/AC/TU ownership，适配 donor whole-project capture 与全局类型头 | C2-C/C3-C 已批准 | 尚无新路线 producer/AC/TU 执行证据 | frontend/integration；三层、重复实例、parent 不读 child body、CMake producer 和并行链接图 |
| 普通 DFFE/current-next/reset | 0236/0264/0274 中普通状态硬件能力保留，统一 data/enable；不包含资源事务 | C1-C/C2-C/C3-C 已批准 | C1 逐拍 oracle 存在，执行 UNRUN | MLIR/runtime/backend；Q 稳定、enabled hold、非零初值、Xfer、Reset/rerun、实例独立 |
| 普通 state 多 driver 与求值失败 | 普通 next 冲突按 C1 分类静态拒绝/互斥证明/precommit overlap check；不隐式仲裁 | C1-C/C2-C/C3-C 已批准 | 未实现新共同 IR/check lifecycle | MLIR/tests；rule 顺序置换、alias、同 enable 输出交换、全树失败不 DriveNext、失败后 Reset |
| Queue/Slot/Table/multilane 资源 | 0216、0238–0241、0262/0263/0280；保留容量/延迟/背压/资源效果，旧 QueueProgram carrier 退役 | 源/IR 资源扩展未批准；普通 register-list 不等于 Table 资源 | 完整迁移阻断项；M2 Queue 用例仍依赖此合同 | architect + MLIR/runtime；full/empty、同拍 pop/push、blocked 不消费、rank/mask/multi-select/lane/profile |
| 多 owner 原子事务 | 0236/0237/0280、LowerRules；all-or-none 硬件义务保留，donor driver checks 不自动证明事务原子性 | 扩展未批准 | 完整迁移阻断项 | MLIR + both backends；selected-branch stall、多输出、order permutation、无部分消费/发布 |
| memory/SRAM | 0114/0122 的 latency/RDW，加 0273 的相关义务；复用时序算法，补齐 donor 排除的 SRAM/bank/ports | 源/IR/runtime 扩展未批准 | 完整迁移阻断项 | architect + MLIR/backend；读写延迟、read-during-write、mask、bounds、enabled hold、双后端 |
| clock/reset/CDC | 0126 和现行 reset/domain、CDC 合同；保留合法跨域和 reset ordering；foundation 只含 default rising/synchronous 域 | 多域源/IR/runtime 扩展未批准 | 完整迁移阻断项 | architect + MLIR/runtime；多时钟、复位顺序、非法跨域、同步器/异步 FIFO |
| 四态与未初始化状态 | 0121/0273、value/known/Z 及未初始化状态义务保留；不能只更名观察字段 | 源/IR/runtime 扩展未批准 | 完整迁移阻断项 | MLIR + C++/RTL；精确 value/known/Z、TICK/XFER、SRAM live window、未初始化值传播 |
| 通用 recovery/ordering 义务 | 0271/0272/0279/0281；保留 state/version/effect 检查，consumer 微架构/ISA 算法不迁入 | 新 source/IR 绑定未批准；不是 C2 普通 numeric proof 的同义项 | 必需完整目标工作 | architect + MLIR/tests；stale response、kill/version、持有结果、义务篡改拒绝；consumer oracle 在其仓库 |
| 基础 portless root | 普通 module root 替代伪造 library top，带静态参数；只是首片 | C1-C/C2-C/C3-C 已批准 | 源 fixture 已捕获，完整编译/执行 UNRUN | frontend/integration；root 独立 producer、错误/reset/stats 与独立逐拍 oracle |
| 外部 typed DUT/system/testbench | 保留完整 source system、真实 stimuli、输入输出、多 clock stepping；不能 portless-only 收尾 | 源/IR/runtime 扩展未批准 | 完整迁移阻断项 | architect + frontend/runtime/tests；真实外部输入、typed 端口、通用 testbench、reset/error/multi-clock |
| 共同 final IR 与两个 backend | 旧 QueueGraph/PYC 两种 C++产品路线退役，采用 donor 一套 C++和 RTL 私有合法化 | C2-C/C3-C 基础合同已批准；扩展按各自行 | 未实现新完整闭环 | MLIR/C++/RTL；相同 final IR、相同 stimulus、独立 oracle；不能仅互相一致 |
| runtime/SDK/发布与打包 | 0149/0232/0233/0265；0267/0268 的发布/身份条款按 C3 显式更新；一个库、ABI、driver | C3-C 已批准；consumer memory/ELF 不迁入 | 尚未实施新 runtime/driver/SDK | integration/runtime；完整状态机、crash/recovery、RTL raw 参数拒绝、relocation、Runtime-only TU、三平台 |
| 源位置与基础诊断 | 0242/0250；保留定位责任，采用 C2/C3 结构化来源/错误；不以位置作为模型身份 | C1-C/C2-C/C3-C 已批准 | 私有 capture span 与 F01 闭合来源记录校验通过；跨 pass/实例/ABI 诊断未验证 | frontend/MLIR/runtime；encoding/codepoint、expanded occurrences、精确 check target、错误不发布半成品 |
| RTL primitive catalogs | 0282；保留 semantic primitive 与 RTL implementation 目录分离及目录验证，不能成为另一个语义引擎 | 迁入所需新绑定/接口尚未冻结 | 必需完整目标工作 | MLIR/RTL/integration；目录篡改拒绝、选择合法性、两 backend 相同语义 |
| DFX/probes/trace 与观察配置 | 0140/0145/0121；通用观测能力必须迁移，consumer trace/schema 不迁入 | 新源/IR/runtime 配置合同未批准；现有能力不得默认删除 | 完整迁移阻断项 | architect + runtime/tests；选择/过滤/启停、稳定 source 路径、观察时点与 known/Z、无状态副作用 |
| 组合环与实例感知时序/逻辑深度 | 主计划保留现行 CheckClockDomains、组合环和 depth 分析；适用算法迁入 MLIR | 现有硬件合法性责任保留，新 IR pass 映射待架构闭合 | 必需完整目标工作 | architect + MLIR/tests；跨实例环、非法 feedback、层级深度与时序检查，不能只分析单 module |
| 增量编译/稳定产物/规模性能 | 0141/0147 与 M6；保留增量失效、可复现产物和扩展性责任，不保留旧 JIT/compiler 实现 | C2/C3 来源/发布基础已批准；性能基线/阈值尚待测定，不豁免验收 | 必需完整目标工作 | integration/performance/tests；header/body 变动失效、并行 TU、重复实例、多特化、stage time/RSS/code size/build/sim |

## 旧路线退役责任

hard-break 方向和 C1/C2/C3 列明接口替换已批准，实际删除在 M5 完整候选执行。下表每一行同时需要源码引用、构建/链接图和安装/动态路径证据；不能只用 grep 零命中或删测试证明完成。

| 退役资产 | owner | 必须具备的替代/删除证据 |
| --- | --- | --- |
| CAS/JIT/direct builder、structural、旧 function-style Agentic | frontend/import + integration | 所需语义由新源覆盖，旧 exports/dispatch/import 拒绝，installed driver 唯一路由 |
| Python _queue_compiler 类型/effects/storage/scheduler、ACPy 并行合同 | frontend/import + MLIR | MLIR 正反例承担全部分析；Python 实际仅 capture/orchestration，无 hidden fallback |
| QueueGraphPlan/Generator/Pyc、QueueProgram carrier 和文本 lowering | MLIR + C++/RTL | 共同 final IR、独立 verifier、两个 backend 完整证据；旧注册/调用/编译 target 消失 |
| PYC C++ emitter 与其他模型 C++生成入口 | C++ + integration | 仅一套 donor-led model generator；没有隐藏 emitter target 或 installed 二进制 |
| 旧 source-unit/interface/finite-family/link/package carriers | MLIR + integration | C2 独立 header/body、typed SpecKey、source-owned groups 和一致性反例；旧 schema 正例/序列化 dispatch 退役 |
| 根 CMake producers、registry、Python/native bindings、工具 aliases/flags/modes/forwarding targets | integration | 新每源 producer、唯一 registry/driver/export graph；无 acc.py/acc/pycc/agentic-circuit 编译入口或旧模式 fallback |
| 旧 public Python namespaces、distribution 中的旧模块与安装资产 | frontend + packaging | 保留 C3 的唯一 pycircuit-hisi distribution；清理其旧 authoring namespaces/载荷，relocated wheel import/路由与清单验证 |
| 重复 runtime 库、旧 model wrapper、donor consumer adapters | runtime + integration | 唯一 libpyc6_runtime、C ABI 实现与 package target；ELF/ISA/guest-memory/consumer adapter 不随 donor runtime 迁入 |
| SDK schemas/manifests、旧 generator ABI 身份、generated bundle 格式/package exports | packaging + integration | C3 generator ABI 2/capability 一致性，拒绝旧 tuple，Runtime/CompilerDev 隔离，文件组/receipt 与 installed inventory 闭合 |
| 旧例子、活跃文档、gate/workflow/test registrations | tests + PM/integration | 每个所需语义断言有新 owner/替代或用户明确退役决定；同候选同步 decision/AGENTS/docs，不以删门槛掩盖缺口 |

## 行为保留与 carrier 退役

Decision 0216 的 QueueProgram 是旧 carrier，列在退役行；其所承载的硬件行为由对应资源/事务行验收。0275–0278 中人工 case/family 源与 carrier 条款被 C1/C2/C3 精确替换，typed identity、同参代码复用和不同实例独立 state 继续保留。0267 的 receipt 禁令按 C3 允许仅文件管理的 receipt/journal，结构化模型身份约束不因此撤销。

M5 必须逐条更新相关决定的 supersession 与 gate 责任，不能在本清单中提前改写现行产品事实。memory 的 0114/0122、四态的 0121、多域的 0126 及当前 reset/domain 条款是行为义务来源，不因为复用旧 pass 困难而自动退役。

## 完成约束

foundation 的批准不批准表中扩展；能力清单通过审计也不证明实现完成。逐行具有新路线正反例、独立双后端或相应 host 证据、接口批准和必要退役审计之后，才可关闭完整框架目标。首个 scalar/record 闭环、治理落地或基础类型测试均不足以替代本表。
