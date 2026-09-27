# 迁移能力与退役矩阵

状态：design inventory，尚未获得精确接口批准。范围是完整框架，不能用 M2 小样板替代本表。目标路线见[主计划](../development/pycircuit-modernization-plan.md)，执行状态见[账本](single-route-migration.md)。

下表的“建议”不等于用户批准。已有能力不因 donor 暂未实现而自动消失；必须迁入新主干，或向用户单独提交明确的退役/延期决定。目标 source syntax 与 IR carrier 在对应批准包冻结。

| 能力 | 现行依据/资产 | donor 情况与目标建议 | 批准包与最低证据 | 状态 |
| --- | --- | --- | --- | --- |
| 唯一 Pythonic 源接口 | Decision 0148/0150；CAS/JIT、builder、Agentic | 采用普通 module/rule/system class/method/record，退役旧三路线，无兼容分派 | C1；捕获不执行模型、旧接口拒绝、installed driver 路由 | 精确接口待设计 |
| 有限整数/数学与位运算 | Decisions 0246/0247；exact-width primitives | donor 数学整数、range；当前实现仅部分 unsigned 范围；必须明确 wrap/check、signed、u64 中间值策略 | C1/C2；独立边界 oracle、overflow/shift/div/rem 负例、C++/RTL | 关键用户决定 |
| bool、nominal records、enums | Decisions 0212–0216、0255/0256 | 普通 Python class/标准类型优先；保留值不可变、身份、完整初始化/投影的硬件能力，不保留旧专用 DSL | C1/C2；type/field/equality/invalid-encoding、布局对照 | 精确映射待设计 |
| 固定数组/静态循环 | Decisions 0252–0254 | donor collection 为局部受限实现；静态形状/动态索引 guard 必须在 MLIR 补齐 | C1/C2；边界、空/非法形状、index guard、independent reduction oracle | 必需后续能力 |
| 模块层级/逐源独立编译 | Decisions 0270/0274 | donor root capture 与 per-definition C++ 需适配；每源 body/interface 独立，parent 不读 child body | C2/C3；三层、重复实例、异构端口、CMake producer/AC/TU map | 不得降级 |
| 普通静态配置/多特化 | Decisions 0275–0278 | 建议删除 explicit finite_cases Python 表面；保留普通 constructor 静态配置的异构实例，MLIR 专门化，一源一个文件组；donor 单 symbol/tuple 是待补限制 | C1/C2；同/异参数、dependent interface、同宽异 nominal type、无 per-case 文件 | 关键用户决定 |
| Reg/current-next/enable/reset | Decisions 0236/0264/0274 | donor 状态 owner 与 Work/Xfer 为主干；保持 Q 稳定、enabled hold、非零初值、跨实例独立 | C2；Work 前后 Q、edge commit、Reset/rerun | M2 首片 |
| 普通 state 与 Queue 区别 | Queue/Table/rule contracts | 普通连接允许多 reader，不自动 FIFO 消费；Queue 容量/延迟/背压需独立明确源表达 | C1/C2；full/empty、同拍 pop/push、blocked 不消费、无隐式 buffer | 关键用户决定 |
| 多 owner 原子事务/冲突 | Decisions 0236/0237/0280；LowerRules.cpp | donor driver checks/lifecycle 不等于完整 transaction；适用算法进 MLIR，不能在 emitter 发明 winner | C2；all-or-none、selected-branch stall、order permutation、失败无部分发布 | 必需后续能力 |
| Table/Slot/multilane | Decisions 0238–0241、0262/0263/0280 | donor 部分 collection/queue 不能冒充完整 Table/Slot；保留已批准硬件行为的新绑定需独立提案 | C1/C2；rank/mask/multi-select/lane/policy、原子组、profile rejection | 不可默默删除 |
| memory/SRAM | 现有 PYC memory/RTL library；Decision 0273 | donor Rev C 排除物理 SRAM/bank/ports；框架必须新增明确源/IR绑定，复用现有时序语义 | C2 extension；read/write latency、read-during-write、mask、bounds、enabled hold | 完整迁移阻断项 |
| clock/reset/CDC | PYC CheckClockDomains/CDC primitives | donor 核心对象提案不足以覆盖完整多域模型；保留合法跨域机制并提供新源绑定 | C2 extension；多时钟、reset ordering、非法跨域、同步器/异步FIFO | 完整迁移阻断项 |
| 四态与观察时点 | Decision 0273；value/known/Z gates | GFSIM 后端需真正承载并执行所需 known/Z 行为，不只更名观察字段；RTL 与共同 IR 对齐 | C2 extension；精确 masks、TICK/XFER、SRAM live window、有效性 | 完整迁移阻断项 |
| recovery/ordering 安全义务 | Decisions 0271/0272/0279/0281 | 通用 state/version/effect obligations 保留于 MLIR；消费者微架构算法不迁入框架 | C2；stale-response、kill/version、持有结果、义务篡改拒绝 | 必需后续能力 |
| system/root/testbench | 当前 system/DUT/SDK；donor @system 未完成 | M2 用批准的普通 portless root 和明确观察机制；外部 typed DUT ports/stimulus、完整 source system 仍必须交付 | C1/C3；最小 root、后续真实外部输入/输出、reset/error/stats | 不能以 portless-only 收尾 |
| C++/Verilog 共同输入 | 当前 QueueGraph/PYC 两种 C++路线 | donor 风格一套 C++；迁入 RTL emitter/原语；共同 final hardware IR，目标私有 legalization 无第二语义权威 | C2/C3；同 IR 同 stimulus 独立 oracle，删旧 graph/emitter/flags | 架构方向已定 |
| runtime/SDK/packaging | Decisions 0149/0232/0233/0265 | 一套 execution model；库名/ABI/C++标准/公开头文件待精确裁决，不携带 consumer memory/ELF模块 | C3；relocated wheel、DUT-only TU、三平台、依赖边界 | 精确接口待设计 |
| 诊断/来源/primitive catalogs | Decisions 0242/0250/0282 | reuse source spans、MLIR 检查、语义与 RTL implementation 目录分离；错误与生成状态可复现 | C2/C3；错误位置/码、篡改catalog拒绝、生成失败不发布半成品 | 必需后续能力 |

## 旧路线删除 owner 与证据

| 退役资产 | 负责 lane | 切换前必须出现的替代证据 |
| --- | --- | --- |
| CAS/JIT/direct builder、structural authoring、旧 function-style Agentic | frontend/import + integration | 新 source 对应能力和拒绝旧入口；public exports/CLI/wheel 同步清理 |
| Python _queue_compiler 类型/effect/storage 分析、旧 ACPy 并行合同 | frontend/import + MLIR | 新 importer/passes 正反例；capture-only 动态路由证明 |
| QueueGraphPlan/Generator/Pyc 语义引擎与文本 lowering | MLIR + C++/RTL | 共同硬件 IR、独立 verifier、两个 backend 完整 evidence |
| PYC C++ emitter/旧 runtime entry | integration + C++ | 一套 model generator/runtime，旧构建/安装/加载路径不再存在 |
| 旧正例/文档/gate registrations | tests + PM/integration | 每项语义断言继承或用户批准退役；不以删测试掩盖缺口 |

## 完成约束

当前没有任何一行因本表存在而成为“已批准”或“已验证”。首个批准包可明确允许 M2 的完整基础切片，其余接口单列后续批准；上述 memory/CDC/四态、异构参数、外部 DUT 与安全义务仍阻止整个迁移目标完成。
