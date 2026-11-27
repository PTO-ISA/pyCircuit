# FA2-CORE-CTRL-0001：控制、同步和资源可见语义 {#FA2-CORE-CTRL-0001}

<!-- ndf: kind=requirement modality=must refinement=L1 status=draft owner=flash_attention2 refines=FA2-ADR-001,FA2-ADR-002 -->

## 目的

定义硬件架构中的调用上下文、阶段同步、资源准入和终止语义。该合同约束可观察的接受、
暂停、完成、取消、错误和复位行为，不固定具体 FSM 编码。

## 上下文和阶段

每个接受的调用都有独立生命周期和调用者提供的tag。多outstanding实现需要可失效的
`generation`；P0S通过单上下文且drain后重用实现隔离，不另设无用的generation字段。上下文至少保存：逻辑
形状、tensor base/stride、scale、mask 配置、当前 query/key tile 坐标、buffer slot、
有效范围、错误状态和完成状态。indexed 模式增加 selection_mode、Index/IndexCount
描述符、K_max、当前 query、有效 count、列表游标和索引 slot identity。

逻辑阶段至少包括 `ACCEPT`、`LOAD_Q`、`LOAD_KV`、`COMPUTE`、`NORMALIZE`、`WRITEBACK`、
`COMPLETE`、`CANCEL` 和 `ERROR`。indexed 模式在 LOAD_KV 前增加
`LOAD_COUNT`、`LOAD_INDEX`、`VALIDATE_INDEX` 和 gather 地址生成边界。阶段名称可以合并，但阶段之间的资源占用、提交边界和
身份匹配必须保持。

## 准入与同步

准入同时检查上下文、Q/K/V slot、索引存储/原token metadata、Row state、Accumulator、传输 credit 和输出写回资源。
任意一项不足都只能拒绝或等待，不能生成半个调用。阶段完成由 typed completion 结果
通知控制器；控制器不能通过读取兄弟模块内部状态猜测完成。

P0S把这些资源静态分配给唯一行引擎：tensor调度器保存完整描述符及b/h/query游标，
行引擎保存当前行地址、索引游标和数值状态，PhaseController保存phase/code。空闲时
资源全部属于新调用，忙时command_ready为0；不设置动态ContextTable、credit池或
同步token网络。控制器和数据Rule读取同一已提交phase，通过阶段互斥避免Memory端口
竞争；对外访存仍使用明确的请求/响应握手。

同一 query 行的 tile 更新按 key tile 顺序提交；indexed 模式按列表位置分块顺序提交。不同 query 行可以并行，但必须使用
不同的状态 entry 或被证明为等价的串行化访问。多个 operation 并发属于可选扩展；基线
实现可以限制为一个 active operation，忙时command_ready为0，不静默排队。

## 终止语义

完成、取消、错误和复位是互斥的终止结果，每个 operation 只能产生一次。取消阻断当前
计算提交；错误阻断后续计算阶段，两者均停止新请求，已发请求 drain 后才终止。错误
检出边沿可以更新内部响应暂存，但不得发布有效行结果或向下一操作泄漏这些值。
复位使上下文和占用失效，
外部未清空请求仍需隔离；P0S无tag协议必须先由bus reset/fence或drain清空，再重启。

## L2 细化

完整描述符ABI、两级FSM、状态所有权、阶段提交条件和错误优先级见
[FA2-CORE-CTRL-1001](../l2/fa2-core-ctrl-1001.md)。
