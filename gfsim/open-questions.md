# GFSim 待决问题

已确定的静态调度契约见 [spec](spec.md)。本文件不把历史 Python 实验中的动态机制视为当前 C++ 要求。

<a id="q02"></a>

## Q02. Module 与 Signal 的组合传播

ACPy 子 Module 调用目前表示静态实例化和连接，构造后各实例独立激活；父 Work 分支不限制子 Module 的独立执行。Module 运行时 var 输入绑定 Signal，普通 Rule 参数按值传递。Signal 现在接受 Queue／Signal 输入与固定配置，在全部 Queue Xfer 后按静态拓扑序传播，初始化同序，组合环报错。跨 Signal 组合链已实现；Module→Module 的直接 var 传播或 delta Work 仍未定义。

<a id="q03"></a>

## Q03. 独立 Cell

目前寄存器用容量 1、有初值、通过 revise 更新的 Queue 表示。独立 Cell 的原子更新、连接与后端表达尚未确定。

<a id="q06"></a>

## Q06. 已发布事件取消／覆盖

显式请求保存在 Rule proposal，整体获准后按 acceptanceTick+delay 发布，delay≥1。普通 Queue/Signal 激活不进入事件堆。已发布事件独立于候选生命周期，到期激活按 Module 去重。当前没有事件取消／覆盖；若需要，应先定义事件身份。

<a id="q07"></a>

## Q07. 外部驱动

tick 0 激活全部 Module，后续由静态资源变化或已发布事件激活。容量释放只重仲裁已有完整 pending，不执行 Work；Queue Xfer 后仍按静态连接安排下一拍 Work，包括 pure push 的 owner。来源 0 保留但没有外部 proposal、仲裁、提交协议；测试通过程序 ROM/初始 Queue 驱动。

<a id="q11"></a>

## Q11. 重启与快照恢复

tick、版本和计数溢出及执行异常使实例 failed，不能继续，不承诺回滚。时间戳用 optional 区分未发生与 tick 0，任务列表使用去重标志，不再有读取代号或任务 tick 标签。未定义重启、快照恢复、资源重新注册。容量环允许保持 pending，由宿主周期上限暴露活性问题，不专门检测。

<a id="q12"></a>

## Q12. 并行与 ABI

C++20 runtime 是单线程源码接口，可独立安装并通过 `gfsim::gfsim` 链接。不承诺跨版本二进制布局。没有并行 Work、并行 Xfer 或跨实例重入契约；若增加，需明确 proposal 所有权和各阶段屏障。

<a id="q14"></a>

## Q14. 静态连接成本（本轮已简化）

资源只保存实际声明的 Module 和 Signal 邻接表，Queue 另保存唯一 pop/push 来源。构造追加、freeze 排序去重；Signal 拓扑序只建立一次，没有动态读取登记、Rule dirty、参数缓存或资源×全部 Module 的映射。有 Signal 待更新时扫描固定拓扑序，只求值受影响节点；通知范围保守，可能增加 Work 次数。Queue 已使用固定 proposal 槽位与共享 RuleId 直接索引。完整模型的测量方法见 [skyzh OoO](../pycircuit/examples/skyzh_ooo/README.md)。

<a id="q15"></a>

## Q15. 类型与 lowering

MLIR 编译器已接通 Ripes5 和完整 Queue OoO。支持 bool、8/16/32/64 位整数、定长 payload 数组、嵌套结构、动态字段下标 revise 和一维固定 Queue 数组。前端保证静态访问范围与 Signal 纯性；runtime 不逐次检查读取声明，仍检查 proposal 上下文与必要旧目标。

任意位宽整数、bit-field、指针/可选对象路径、重叠字段 revise 的语言约束，以及多来源端口竞争仍未定义。资源须地址稳定且活到 Simulator 析构；不提供重新 attach 或模块析构通知协议。
