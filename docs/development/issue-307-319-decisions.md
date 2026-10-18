# #307 / #319 执行决定与能力边界

日期：2026-10-17。本文件冻结本轮执行需要明确的合同差异，供
[总 checklist](issue-307-319-review-checklist.md) 的 W1/W7 使用。它不是新的
语义批准，也不把目标写成已实现。

## 设计点处置

| 设计点 | 处置 | 本轮合同 |
| --- | --- | --- |
| Module 层次 | retained | 普通 `@module` 函数静态直接调用；一次调用一个子实例；class/init/allocate-then-bind 不恢复。 |
| 独立输出 / 共同接口 | retained | 独立消费者用异构 typed tuple；同一消费者共同字段用具名 Struct 整体传递；不强制统一 wrapper。 |
| Module 持久状态 | retained + implemented-with-limits | 声明产生编译器拥有的 DFF 或 DFFE 子实例。本轮按实际物化 commit enable 选择：单一 local Rule 的直接 enable 可静态证明恒 1 时，标量状态用 DFF；复合 ordered/unordered writer、层次 incoming、不能证明恒真的 guard/资源/分支条件及 Table Collection 保留 DFFE/Collection。全 System `pyc_root_commit_ok` 保护在两个 leaf 中都保留。 |
| Rule 局部值 | retained | Rule 参数、局部注解、保存值和返回值是组合 SSA，不是存储。 |
| Rule 原子性 | retained | 一个 outer registration 一个事务；嵌套 Rule/Method 共享边界；pop→compute→push、父子状态效果必须在同一外层事务。 |
| 同事务读 | retained | 顺序 candidate 可见；已保存普通 local 保持 SSA 快照；独立外层事务读 committed Q。 |
| `after=` | implemented-with-limits | 只接受同一 Module 的唯一 local outer registration；可提供同拍 predecessor candidate 可见性。现有 examples 的排序均引用本 Module Rule。跨 Module 独立 registration 的新排序接口不列入本轮设计或必做项。 |
| 跨层级 whole-owner 写 | retained | 已有实际 owner/互斥证明路径保留。 |
| 跨层级 field/index partial 写 | implemented-with-limits（本轮 W2a） | 只接受可证明 footprint 不相交的无序 writer：共同 Struct 前缀后的不同 field 直接不相交；同一 field 的 Table 写必须证明地址分离；其余必须证明写路径互斥。祖先 partial lane 在离开其 Module 前已用自己的 domain masks 与 captured Q 组合，再作为完整 lane 路由；masks 不跨端口。接收方 closure 仍按 whole-owner 处理该完整 lane。每个 pair 仍经 source grant proof 与 final write-domain proof 独立重算。 |
| 通用跨 scope ordered/mixed 扩展 | **移除新增设计任务** | 没有现有 example 必须依赖跨 Module 独立 registration 排序或通用 mixed 调度。保留同一父 Rule 内嵌套调用的原子顺序，以及同一父 scope 的本地 after Rule 调用子模块；不将它们误列为未定的新接口。未支持的 mixed groups 继续明确拒绝。 |
| 输出有效性 | retained | 编译器产生/传播 availability；失败不补零、不保持寄存器、不把 invalid 当 else。payload.valid 与 Queue 的 raw valid 是作者协议，不替代 availability。 |
| Queue | retained | `ac.queue[T]` native 子 Module，显式 `push/pop/peek`，复用现有 FIFO kernel/RTL。隐式赋值 push/pop 不恢复；普通内部 Queue 不要求作者重复 ready/valid/full/empty 检查；真实外部握手、算法仲裁、fork 交付状态保留。 |
| Table | retained + limits | 固定元素存储；本轮标量状态可选择 DFF，Table 仍是 Collection（每元素 DFFE）。既有 shape/index/初始化限制不变。 |
| SCF | retained + limits | 只在既有有限 `if/for/while/execute_region` 和既有类型上闭环；不扩大为任意 SCF/循环/新浮点系统。 |
| Signal | **已取消设计** | 2026-10-18 用户明确要求去除独立 Signal 设计。删除 Signal 声明/set/get 资源模型及其公开 SBR 接口要求，不作为延期功能或后续待办。连接使用 typed Rule/Module 参数与返回值，编译器传播 availability；跨拍数据使用状态或 Queue。 |
| CF / C / SB / SBR | **不因 Signal 扩展公开 API** | 保留编译器内部必要的依赖、冲突、顺序与环检查；不要求作者声明关系表，不将 #307 的 Signal SBR 规则施加到普通 Rule 返回值或状态候选。 |
| 作者 public/private 修饰符 | **移除新增设计任务** | 现有 examples 未使用或提出此项需求；不新增装饰器选项，不作为迁移阻塞。已有接口导出、owner边界、链接闭包和私有编译器端口检查保留。 |
| 自动调度 | retained | 复用现有依赖分析、work partition、System commit；不增加第二调度器、动态 activation worklist 或自动仲裁。 |

## 按当前 examples 收敛设计（2026-10-18）

对 catalog 的71个目录、177个Python文件进行了AST核对，无解析失败。Rule/Method装饰器参数只有13处 `after` 和12处 `guard`；14个after引用均为本地Name。Queue构造使用现有depth/latency/ready_policy和边界连接参数；查询调用只有fork中的2处peek。完整静态审查与这些语法证据一起用于范围判断，不能仅凭某个关键字未出现就删除现有语言能力。

| 没有当前用例依据的新增设计 | 处置 | 当前保留的表达/检查 |
| --- | --- | --- |
| 独立Signal及公开CF/C/SB/SBR关系表 | 取消，不列后续待办 | typed参数/返回值，编译器availability、依赖/冲突/环检查 |
| 跨Module独立Rule注册的公开after身份、通用ordered/unordered混合调度 | 从本轮必做项移除，不等待新API决策 | 本Module after、嵌套跨Module调用、已支持的部分写组合与EHR |
| 作者public/private修饰符 | 从本轮必做项移除 | 已有Module接口、资源owner和独立编译/链接边界 |
| #307旧Queue的隐式赋值push/pop、重复pop自动合并、旧初始内容/default API与新增empty/full/size要求 | 不恢复、不作为迁移前提 | 当前显式push/pop/peek；depth/latency、背压与真实边界握手 |
| 自动仲裁、动态activation worklist、第二调度器 | 不设计、不实现 | 现有静态依赖分析/work partition；作者表达算法优先级 |
| 为迁移增加自动多时钟System或要求支持所有SCF操作 | 不扩展本轮范围 | 两个多时钟例的原始独立域验证；有限SCF与已确认的类型范围 |

仍须完成的是真实编译失败、现有类型在已确认路径上的一致性、合法primitive调用、普通Rule/Module接口、资源guard/输出有效性、状态与EHR/原子提交、原API及71例完整验收。不得用范围收敛删除已有oracle、缩短刺激、隐藏能力回归，或要求作者绕过编译器缺口。

## 跨层级 partial 写的精确边界

接受：

1. 同一实际 state owner（declaration + 实际 instance path；alias 共享 owner，不同实例不共享）。
2. 两个无序 writer 的 **write footprint** 可证明不相交：
   - 共同 selector 前缀上不同 Struct field；
   - 同一 Struct field 下固定一维 Table 的两个已证不同地址；
   - 否则两条写路径的 enable 互斥。
3. 祖先 writer 的 lane 在其 Module 内已经由 `materializeDomainCommit` 用自己的
   selectors/masks 与 captured Q 组合；它路由到后代时是一个完整 candidate
   payload + owner enable。后代再用自己的 domains 组合该 lane 与本地 candidate。
4. source replay 与 final proof 各自从原始 SSA/最终 SSA 重算 base、domain
   coverage、enable 和 old-Q，不共享执行结果。

继续拒绝：

- 同一 field/address 且不能证明互斥的 writer；
- whole-owner 与 field/index 写重叠且不能证明互斥；
- 动态 Table 地址无法证明分离；
- 任一侧参与跨 scope `after=`；
- ordered 与 unordered mixed group；
- 超出既有“一层静态 field path + 一个顺序 Table index”形状的 partial 写；
- 与 Collection 共享定义的 transaction routing。

## 已取消独立 Signal 设计（2026-10-18）

按用户明确指令，撤销 #307 中独立 Signal 资源的设计：不提供 Signal 声明、固定资源身份或 set/get 接口，也不引入该接口专属的公开 SBR 约束。W4 取消，不作为延期功能、待确认问题或迁移完成条件。

Module/Rule 使用 typed 参数和返回值连接，编译器生成并传播输出有效性。跨拍保存使用 Module 状态，排队使用 Queue。编译器继续负责必要的数据依赖、冲突、因果环、EHR 顺序和原子提交检查。冻结审批和执行日志仅保留历史记录，不构成恢复该设计的依据。

## DFF/DFFE 选择合同（W3）

- 仅单一 local Rule 的直接候选可选择 DFF；ordered/unordered 或 incoming 复合候选保留 DFFE。该限制修复了原优化误把末 writer enable 当完整 enable 的回归，不改变 Rule 行为。
- 只从**实际物化**的 commit enable 证明：该 Bits1 SSA 必须是可折叠的常量 1
  （constant、and/or、静态 compare、常量 select）；未知、多 bit 或不可折叠即保留 DFFE。
- 选择发生在 transaction lowering；leaf 仍由编译器生成，不增加作者开关或寄存器 API。
- DFF 无 en 端口；其数据、init、clock、reset 类型/连接与 DFFE 相同。
- DFF 的更新仍受全 System `pyc_root_commit_ok` 保护；失败/Discard 不改 Q 或 clock history。
- Table Collection 不参与标量 DFF 选择。
- 独立 final proof 重算 commit enable 为常量 1；source replay 验证 leaf arity/callee/domain/initial。
