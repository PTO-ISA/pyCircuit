# 示例

保留四个互补的阅读入口，建议按下表顺序阅读。细粒度语义和非法输入放在 tests 中验证，不再分别维护演示模型。

| 示例 | 重点 | top |
| --- | --- | --- |
| [array_arbiter.py](array_arbiter.py) | Module 数组、二维 age matrix、match 候选、普通坐标仲裁和反压原子取消 | ArrayArbiter |
| [ehr_shared.py](ehr_shared.py) | 固定 EHR 共享视图、前序写入依赖及最终状态 | EHRShared |
| [riscv_pipeline/model.py](riscv_pipeline/model.py) | 顶层 Queue 连接、五级 RV32I 子集、反压和 scoreboard | Pipeline |
| [ooo_scheduler/model.py](ooo_scheduler/model.py) | ROB 槽位数组、操作数等待、ADD／MUL 发射、广播及顺序退休 | OutOfOrder |

所有模型使用静态 __init__、显式接口和统一 @ac.rule。只提供状态与公开 Rule 的组件无需空 __call__；
流水线 Queue 和广播 Signal 由父 Module 声明，再通过主体连接输出。ROB 使用
`ac.array(RobSlot, shape=(4,))`，按 tag 下标调用各槽位的 Rule。

```bash
python -m pycircuit compile pycircuit/examples/ooo_scheduler/model.py --top OutOfOrder \
  --output pycircuit/builds/benchmarks/ooo --cycles 32
python -m pycircuit.tests.run_acceptance
```

验收脚本仅对这四个模型保存 32 拍轨迹，比较事务／RTL、优化开关及三个阶段重载。
测试另有独立预期：CPU 核对 ISA、寄存器、内存及退休周期；OoO 核对结果、等待／广播和顺序退休；
array 仲裁核对反压时年龄状态和槽位释放一起取消；EHR 核对共享读取视图与依赖边。

构建、工具路径、阶段输出及测试分类见[使用说明](../README.md)。
