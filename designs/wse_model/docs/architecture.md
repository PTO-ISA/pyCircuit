# 当前模型结构

架构输入为[白皮书](design/wse-system-architecture-whitepaper.md)和
[Calendar 方案](design/wse-calendar-scheme.md)。未确定的设计值由
`src/wse_model/open_items.py` 记录。当前通信功能模型分为共享硬件、运行时程序
和验证三层。

## 共享硬件

`models/machine/machine.py` 的 `WseMachine` 是运行时程序的数据执行入口。
它拥有程序计数器、命令状态、存储服务和逐跳路由遍历状态，按程序执行配置、
推进轮次、建立接收、复制、多播、等待和退休。
功能顶层和时延顶层各自编译，共用 `models/machine/machine_core.py` 中的宿主接口、指令解码和节点内存。

`models/calendar/` 保存共享组件。`ReceiveLedger` 按节点保存接收账户、早到
元数据和去重身份；`EpochLedger`、`CalRegLedger` 保存轮次与配置状态；
`ForwardHop` 决定单跳输出。原 `ReceiveEngine`、`EpochBank` 和
`CalRegAdmission` 接口通过同一核心保留组件回归。账本只有一套范围、身份和
计数判定，原接口与机器不各自维护一份接收算法。

去重身份按节点保存为一行位向量，每个槽位占 22 位：6 位源节点编号和 16 位
序号。有效位另存为每节点一张位图。接收时只取出目标节点的身份行，退休时
清除该节点的有效位图；残留身份内容随槽位再次使用而覆盖。这样保留原有槽位
容量与去重规则，并减少生成的仿真代码对整张身份表的反复处理。行更新使用
选择和拼接，保留四态仿真中的 X（未知）与 Z（高阻）位。

功能配置包含 40 个逻辑私有节点 bank，每个 64 KiB。内部使用一份字节存储，
按节点划分不重叠区域；公开接口仍是“节点编号＋节点内地址”。跨节点复制由
COPY 命令和逐跳转发结果决定。这里采用单片段、串行存储服务，描述的是功能
载体的存储表示和执行顺序，不赋予物理集中 RAM 或共享端口的性能含义。

接收范围、轮次、身份及所有远端落点在写入前预检。每个写拍（64 字节宽拍或 8 字节窄拍）检查完整
固定远端落点集合的动态许可；缺许可时保持当前进度。目标末拍写入、去重身份
插入和接收计数在同一事务提交。普通协议故障锁存并允许诊断读回；模型容量
断言沿用后端终止语义。详细接口以 [program-interface.md](program-interface.md)
为准。

## 运行时程序

`src/wse_model/programs/abi.py` 定义 64 字节通用指令、内存初始化和读回范围。
ABI 是程序与模拟器的数据接口。程序的实际条数、字节内容与初始内存均从宿主
加载，不进入硬件编译身份。

`src/wse_model/programs/allgather.py` 决定 AllGather 的成员、rank 顺序、分片、
地址、期望接收量与阶段依赖。`examples/pto_allgather.py` 生成真实 PyPTO 输入，
PTOAS 处理后由 `tools/lower_pto_allgather.py` 分析中间表示中的传输和同步语义。
`src/wse_model/compiler/allgather.py` 将已证明的传输映射为路由与布局片段，再
生成通用程序。硬件不直接解释任意 PTO 指令。

基线程序仍使用 32 成员、行列两阶段和 512 个 COPY 片段；这些数值属于该程序。
树形多播等其他程序通过同一接口加载，用来验证逐跳分支、只转发节点和不同
地址长度。程序复用应比较同一最终 ACIR 的哈希；ACIR 是双方后端共用的电路
中间表示。

## 验证与保留资料

项目根 `ndf.yaml` 同时读取共享组件规范与 `spec/programmed-machine.md`，
从项目根扫描实际 machine、组件和验证工具的源码绑定。`models/calendar/`
保留仅覆盖组件的独立配置；该子目录门禁不覆盖兄弟目录中的机器。NDF
（规范文档格式）追溯检查核对条款身份与源码定义绑定，行为正确性另由执行
测试验证；当前条款仍为 draft。

`tools/` 和 `tests/` 保存独立预期。输出比较从原始输入与算法定义求答案，
不参与硬件构建或数据写回。组件门禁验证共享语义，通用机器测试验证程序执行，
PTO 整链测试验证实际编译输入到输出字节的连接；三者分别记录结果。

旧 `allgather.py`、`allgather_control.py` 及对应 bench 已移入
`verification/fixtures/calendar/`。它们保留历史元数据场景和源码参考，静态
路由/metadata 检查仍可使用；`verify_calendar.py` 不再注册这些旧硬件入口。
新的 AllGather 功能验收使用 `WseMachine` 加载程序。

`models/pto_allgather/timing.py` 与对应驱动、配置保留为 analysis-only 的
行列专项时序投影。`src/wse_model/analysis/allgather_schedule.py` 按显式服务
参数推导该投影，暂定项继续标为 `pending`。这条路径与通用机器的实际字节执行
分别验证、分别统计周期。矩阵与向量的 Python 模型仍提供容量、利用率和时延
估算。

验收回执分别绑定源码、程序、最终 ACIR 和模拟器的哈希。构建、执行与
逐字节比较分别记录；同硬件复用检查固定模拟器身份，只改变运行时输入。
