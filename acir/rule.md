# Rule 事务与条件关系编译契约

完整语义以 [rule 图设计](../rule-graph-spec.md) 为准，实际保存格式见 [ACIR MLIR v6](../pycircuit/acir.md)，前端写法见 [ACPy spec](../acpy/spec.md)。

Module 组织固定状态和连接，并保存选择条件、参数与共享组合计算。rule 是独立原子事务，可以跨 Module 调用 method、访问外部状态。method 的 guard、读取、消费和修改传递归入最外层 rule；Module 边界不形成原子边界。

Module 准备逻辑允许只读 method 调用及其嵌套调用和 guard，提交效果仍归属外层 Rule。变量 guard 引用声明作用域的布尔 SSA 定义，函数 guard 保留调用来源；外层选择路径和显式 guard 分别保存并共同约束事务。

Queue 输入实际读取 payload 的路径隐式 pop；Cell 输入读取旧值；Signal 参数传递资源，值表达式读取当前值；普通参数是 SSA。返回 payload 分别形成 Queue push、Cell write 或 Signal set。混合输出逐位置绑定，None 省略对应效果。Cell/Signal 目标必须已声明并初始化，Rule 不返回普通 SSA。消费按实际资源和最外层事务合并，不消费未执行分支。

IR 保存所有资源初始化、实例层次与固定接口连接、输入角色、读取视图、函数/CFG、显式 guard、效果和源码位置。选择条件和参数计算共享 SSA，不通过重新读取状态复刻闭包。

派生 `preparations` 保存计算区域身份、IR 来源、输入输出、条件、输出有效性、状态及 method 依赖和使用者；Rule 引用所需区域，`computations` 仍表示准备阶段状态访问。共享访问关联到 Rule 后保留原始身份、调用链、method 祖先及 EHR 端口，参与接口顺序传播。就绪要求只约束使用者，不作为整个 Module 的条件。区域保留分支、循环和状态阶段边界，不作为事务节点，不新增计算 operation。共享关系保存同一计算定义，不合并独立调用。

成功条件概念上包含选择、实际路径上的显式 guard、基础 ready 及准备计算自身的有效性；这一阶段保存这些关联，不生成运行时仲裁或最终执行电路。guard 只观察状态。必要 ready 随实际操作路径保留，不将所有可能分支的资源需求无条件合并。

组件契约决定顺序：Queue 为 Pipeline FIFO，read/pop/revise 要求旧状态非空；push 可利用成功 pop 释放容量，新 push 的数据下拍才可读。默认 `read SB revise SB pop SB push` 并计算闭包。empty/size 查询旧状态并与修改默认 CF；full 读取内部 EHR 的晚端口。Cell read/write 默认 CF，read 只读周期开始的值。用户 `@ac.method(After=...)` 引用本组件公开接口，和内部必要关系合并；同一事务内部 SB 不产生 rule 自环。

Signal 声明保存为 `!acir.signal<T>`，保存身份、类型和初值；赋值读写使用 `acir.read/write` 的 current 视图与 get/set 接口。跨事务 set 需整个事务获准，失败回退保留值；同事务前序临时 set 对后序 get 可见且整体提交或取消。不分配 EHR 端口，不产生消费或容量要求；Module 读及共享计算的依赖关联到实际使用者。Rule-var 结果提取、生产者有效性与 `var.validity` 已移除，普通 SSA 参数、helper/method 返回及 Module 共享计算保留。

EHR 声明保存为 `visibility="ehr"` 的 Cell 资源，沿用 `acir.read/write`。端口依据具体实例的 method 访问、调用绑定和关系闭包求解；无顺序的相关读写共用端口，不能由任意 Rule 排序增加旁路。helper 继承阶段，嵌套 method 保留自己的归属，直接 Rule/准备访问使用端口 0。同阶段与严格先后约束冲突时报告资源、method 和来源。

EHR 读取选择前序端口最后一次有效写入，没有则回退旧值；同端口写入不可见。跨事务资格包含完整事务获准，事务内部允许前序 method 临时写入并保持最终原子提交或取消。Queue full 是由旧数量初始化的内部 EHR 容量视图；pop 写 false，full 与 push guard 读晚端口。容量视图不单独提交，数量由实际 Queue 效果计算。

MLIR 的 `acir.model` 保存原始组件契约、来源和接口身份，资源、实例、调用保存绑定。完整 RuleGraph、推导契约和准备区域摘要保存在独立 JSON/DOT，不嵌入顶层分析属性；EHR 端口和 `acir.ehr` 继续保留。分析收集基础访问和嵌套接口摘要，按具体实例查表、闭包、结合路径与 guard，再提升为 rule 关系；JSON 顺序边统一为 method.order；Signal 的 `set SB get` 保留写者判断先于读者的顺序，读者不要求写者成功。关系表不含 when，ready、guard 和调用路径分别保存。

关系边代表处理结果的先后约束，不代表前者必须成功。边保存适用条件、资源/值身份、操作对和源码位置。完整 IR 保留中间计算，rule 图展示主要事务关系。常量和相同 SSA 条件可证明简单互斥；其余环区分可确认环和无法证明合法顺序。经过 Cell 旧值的跨周期反馈不形成同周期环。

`compile` 和 `analyze` 导出相同格式；analyze 只解析保存的 MLIR，重建摘要与关系，不依赖源 AST 或旧 analysis 字典。不实现运行时激活、冲突仲裁、Xfer、仿真后端或竞争写入检查。

ACIR v6 不兼容 v3/v4/v5：旧格式产物必须重新编译。arith.select、循环携带 SSA、helper 定义环境及实例层次身份均参与重载分析。
