# class Module 示例与逐拍验收

所有模型使用静态 __init__、显式接口和统一 @ac.rule；有主体的模块使用固定 __call__ 连接。
公开 Rule 可以组合到调用者的原子事务；独立事务通过组件传值。
只提供状态和 Rule 的组件可省略 __call__：CPU 中 RegisterFile／DataMemory、DualChannel 的 Observer
均直接使用公开 Rule，无需空主体或额外调用。逐拍轨迹直接记录 RF 和内存的内部状态。
CPU 的 if_id／id_ex／ex_mem／mem_wb Queue 全部在 Pipeline.__init__ 中声明，
主体写 if_id = fetch()、id_ex = decode_stage(if_id, rf, mem_wb) 等赋值连接。
OoO 的发射／完成 Queue 和广播 Signal 同样由顶层声明；所有示例已移除 .outputs。

| 模型 | 验证内容 | top／配置 |
| --- | --- | --- |
| array_arbiter.py | 三槽位、二维 age matrix、match 候选、普通坐标仲裁与反压原子取消 | ArrayArbiter |
| dual_channel.py | 只读观察与共享表达式，两个独立调用点 | DualChannel |
| shared_state.py | 共享实例、独立实例与选择 | SharedState，choose_left=0／1 |
| mixed_ports.py | Queue／Cell／Signal 混合可选输出 | MixedPorts，enabled=0／1 |
| signal_link.py | 本拍 Signal、生产者取消及有效性传播 | SignalLink，enabled=0／1 |
| dependency_cycles.py | Cell 旧值反馈；Queue 节点合并环诊断 | CellFeedback、ExclusiveRing（choose）、QueueRing |
| ehr_shared.py | 固定共享 EHR 读取、前序依赖传播与最终状态 | EHRShared |
| ehr_slots.py | 保留旧阶段链的非法跨端口原子访问诊断 | EHRSlots，release_enabled=0／1 |
| ooo_scheduler/model.py | 两个保留站、ADD／MUL、等待／唤醒／广播／顺序退休 | OutOfOrder |
| riscv_pipeline/model.py | 指令驱动五级 RV32I 子集、反压与 scoreboard | Pipeline |

`ehr_slots.py` 的 available 读取和 allocate 写入处于同事务不同端口，违反核心设计；
它作为诊断例子保留。合法 release → allocate（guard 和写使用同一端口）及固定共享读取，
由 test_pipeline.py 中独立模型验证，不添加临时旁路。

CPU 执行 addi／add／sub／sw／lw，独立 ISA 解释器验证寄存器及内存；定向退休周期
为 4、5、9、13、14、18。OoO 的 4 个微操作最终产生 x1=42、x4=48、x5=13、x6=61，
以 tag 1、2、3、4 顺序退休，并检查等待、发射和广播均实际发生。

测试还覆盖 Queue 满时同拍腾空间、失败时取消全部修改、嵌套控制、读取失败不进入 else、
无关事务独立、重复消费、Queue 数组、Cell 聚合赋值、未声明 EHR 可见性和 C／SBR 契约诊断。
两层执行器逐拍一致；CPU／OoO 及代表性组件模型核对优化开关与三个阶段的独立重载。

建议先看 array_arbiter.py，理解 array、mask 与仲裁；再看 ehr_shared.py 的固定共享视图；
最后看 riscv_pipeline/model.py 的顶层流水线连接，以及 ooo_scheduler/model.py 的完整调度。
OoO RegisterFile 通过构造函数绑定 ROB，无空 __call__ 或额外 rf()。
ROB 使用 `ac.array(RobSlot, shape=(4,))` 构造槽位，按 tag 下标调用分配、查询、完成和退休 Rule，
每个槽位保留独立状态及 guard，无需逐槽位书写分支。

编译／执行示例：

```bash
python -m pycircuit compile pycircuit/examples/ooo_scheduler/model.py --top OutOfOrder \
  --output pycircuit/builds/benchmarks/ooo --cycles 32
```

构建、工具路径、阶段输出和绘图见 [使用说明](../README.md)。
