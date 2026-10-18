# 给执行 agent 的 prompt

请继续 pyCircuit #319 与 #307 尚未闭合的已确认设计 gap。当前 17 个 examples 的迁移已完成（34/34 PASS），54 个旧包装已完整归档；不要重复迁移或恢复全部旧包装。API lit 汇总 161/169，8 个失败仍待处理；以执行状态中的最新首个分歧为准。先读本仓 AGENTS.md、当前 frontend guide/language/源单元流程，以及：

- `docs/development/issue-307-319-review-checklist.md`
- `docs/development/issue-307-319-examples.md`

按 checklist 的 W0–W7 和依赖执行，逐项记录证据。不要把 #307 的历史 class/Cell/隐式Queue赋值路线恢复；#319与用户最新要求优先。独立Signal资源及其声明/set/get设计已经取消，W4不再执行，也不列为后续待办；不要恢复其公开关系接口；保留编译器内部的依赖/冲突/有效性/EHR检查。作者public/private、跨Module独立registration的after接口、通用mixed调度及旧Queue扩展没有当前examples需求，均不列为本轮必做或阻塞项；保留已有本地after、嵌套调用及资源owner/链接边界检查。

**候选必须延续当前脏工作树** `/Users/zhoubot/.codex/worktrees/typed-module-outputs/pyCircuit`，分支 `codex/typed-module-outputs`。HEAD `412a180928d3bdf1af3a99f14ed4a21a1c1b172a` 不含现有未提交实现。先保存tracked/untracked内容哈希，使用 `git --work-tree="$PWD"` 核对根目录；不要reset、清理他人工作或从干净HEAD开始。build/install只能使用此工作树 `.pycircuit_out/toolchain/{build,install}`；一名集成者独占构建安装，不能复制另一工作树产物。

前端写成简单、串行、Pythonic的函数。Module直接调用表达层次；独立接口用typed tuple，同一消费者共同字段用Struct整体传递。持久变量在Module声明，Rule局部值是组合SSA。Rule一个功能内聚，必须共同提交的pop→compute→push和父子状态效果放在同一个外层事务；嵌套Rule不提前提交。不能为了拆短函数改变原子性或pipeline旧状态读取。

编译器负责类型/owner/effect/调用图、选中路径资源guard、输出有效性、proposal/EHR/commit和诊断。普通内部Queue不要作者重复ready/valid/full/empty检查；保留真实外部协议、算法仲裁、payload.valid、fork交付状态。Queue是普通native子Module，复用已有FIFO kernel/RTL。invalid输出不可消费，不补零或保持寄存器。

沿现有Python capture→MLIR→verified final IR→C++/Verilog修复根因；source replay与final proof均保留。禁止example名字/宽度/常量特判、shim、第二套编译/运行引擎、新计费/预算设计、提高上限、通过缩短刺激或删oracle掩盖失败、把失败改skip或旧API回退。类型范围是已有有限类型和受支持SCF，不擅自扩大成所有SCF/新浮点系统。

先关闭W0当前回归，再按依赖补已确认路径的跨层级partial写和本地ordered组合、DFF/DFFE选择、已有依赖/有效性/冲突体系及剩余 API 失败的根因。复杂新语义按仓库要求独立架构/分解审查。其他agent并行时明确文件owner，测试与实现独立，不能互相覆盖。

先读 examples/GUIDE.md、examples/STANDARD.md 和最新适用性清单。71 是历史库存，不是保留或扩展目标。推荐主线7例、设计/专项参考10例；其余按合并讲解、API回归或过时定位处理。不要把所有变体拼成一个巨型DUT，也不要为每个小例子强制维护System/C++/RTL三份脚手架。新例子必须说明独立用途；普通例子优先短System，物理/四态/Discard等独有边界才保留手写host/RTL。54 个目录已完整归档，现有显式引用已重定向；归档中的候选未全部接入 API 执行，退役不得计为PASS。长时进位及穷举仍归现有nightly，不为过时目录计数追加编译器机制。DMA/memory_banks 的新短 System 已通过；原长 bench 保留为历史回归，其旧失败不能用短场景通过抹去。

简单、定位清楚的错误直接修并验证。复杂错误或语义歧义暂停受影响packet，报告首个失败阶段、命令、最小复现、证据、影响与建议；继续独立无阻塞工作，不堆特殊路径，不反复询问常规本地操作许可。

交付：更新checklist和示例适用性/覆盖归属清单，给出完成/未完成/阻塞、改动文件、准确命令/退出码/工具与内容哈希/证据、保留风险。清单由独立测试与review确认，不能凭文档或自己判定整体完成。完成后把结果交还用户，由原审查agent独立复核。提交以用户当次明确授权为准；不要据此推定 push/merge、关闭 issue 或发外部消息的授权。
