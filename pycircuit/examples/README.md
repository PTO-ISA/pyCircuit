# 示例

保留五个互补的阅读入口，建议按下表顺序阅读。细粒度语义和非法输入放在 tests 中验证，不再分别维护演示模型。

| 示例 | 重点 | top |
| --- | --- | --- |
| [array_arbiter.py](array_arbiter.py) | Module 数组、二维 age matrix、match 候选、普通坐标仲裁和反压原子取消 | ArrayArbiter |
| [ehr_shared.py](ehr_shared.py) | 固定 EHR 共享视图、前序写入依赖及最终状态 | EHRShared |
| [riscv_pipeline/model.py](riscv_pipeline/model.py) | 顶层 Queue 连接、五级 RV32I 子集、反压和 scoreboard | Pipeline |
| [ripes5/model/model.py](ripes5/model/model.py) | 对齐 Ripes RV5S 的固定流水线寄存器、前递、load-use 和冲刷 | [Demo](ripes5/model/demo.py) |
| [ooo_scheduler/model.py](ooo_scheduler/model.py) | ROB 槽位数组、操作数等待、ADD／MUL 发射、广播及顺序退休 | OutOfOrder |

所有模型使用静态 __init__、显式接口和统一 @ac.rule。只提供状态与公开 Rule 的组件无需空 __call__；
流水线 Queue 和广播 Signal 由父 Module 声明，再通过主体连接输出。ROB 使用
`ac.array(RobSlot, shape=(4,))`，按 tag 下标调用各槽位的 Rule。
CPU 示例的寄存器堆及相应的 scoreboard／重命名表使用 `ac.array(ac.cell[T], shape=(...))`，
每个表项都是独立 Cell，动态下标只读取或修改选中的 Cell。轨迹以 `top.rf.registers.0` 等路径显示各表项。

```bash
python -m pycircuit compile pycircuit/examples/ooo_scheduler/model.py --top OutOfOrder \
  --output pycircuit/builds/benchmarks/ooo --cycles 32
python -m pycircuit.tests.run_acceptance
```

验收脚本对这四个模型保存 32 拍轨迹，比较 C++／事务／RTL、优化开关及三个阶段重载。
测试另有独立预期：CPU 核对 ISA、寄存器、内存及退休周期；OoO 核对结果、等待／广播和顺序退休；
array 仲裁核对反压时年龄状态和槽位释放一起取消；EHR 核对共享读取视图与依赖边。
Ripes5 的统一入口 `python -m pycircuit.examples.ripes5.tests run` 核对原生 RV5S、C++ 和程序预期；
`--check all` 另启用两层 IR 解释与阶段重载，
运行与表达问题见 [Ripes5 说明](ripes5/README.md)。

构建、工具路径、阶段输出及测试分类见[使用说明](../README.md)。

生成 C++ 保留 Module、Rule、payload 字段及 Cell 数组索引。事务与 RTL 友好层轨迹均为 **IR 解释执行**，
另与独立编译的 C++ 程序逐拍对照；代码审阅与测量见 [C++ lowering 审阅](../docs/cpp-lowering-review.md)。
