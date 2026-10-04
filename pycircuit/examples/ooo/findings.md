# ACPy 表达记录

本轮只添加示例、宿主工具、验收和构建入口。没有修改编译器、GFSim、生成 C++ 或调度语义。

## F1：循环内首次受条件控制的构造常量被错误复用

想表达：同一 Signal 扫描固定槽位，根据构造参数选择整数或访存候选。
自然写法是 `ready and memory_op(entry.ins.op) == is_memory`，其中
`is_memory` 在实例化 Signal 时传入 `True` 或 `False`。

[最小复现](repro/guarded_constant.py) 将窗口缩成两个 Queue：

```python
@ac.signal
def natural(enabled, values, wanted) -> ac.u32:
    selected = ac.u32(0)
    for i in range(2):
        if enabled[i].value and values[i].value == wanted:
            selected = i + 1
    return selected
```

`enabled=[False, True]`、`values=[False, True]`、`wanted=True` 时应返回 2，
当前编译链返回 0。生成代码在第一次受 guard 保护的位置初始化 `wanted` 的
局部常量，之后展开的迭代复用它；第一次 guard 为假时，该值仍为默认 `false`。
在最初的 CPU 轨迹里，访存选择器因此错误选择了已经走整数路的 ADDI。
仅比较最终架构状态不足以捕获这个错误；“每条只发射一次”和“最老就绪且通路匹配”
的逐拍检查能直接捕获。

可用现有语法表达相同硬件：在循环前写 `lane = ac.u32(is_memory)`，
循环内比较 `ac.u32(memory_op(...)) == lane`。该转换使构造参数在无条件区域
物化，未增加 Queue、拍延迟或调度约束。可读性代价是一行显式转换。
[Issue](issue.py) 使用此写法；完整模型不被阻塞，编译器缺陷仍然存在。

复现命令（从仓库根目录运行，先完成 README 的构建）：

```bash
python3 -m pycircuit compile pycircuit/examples/ooo/repro/guarded_constant.py \
  --top Probe --output /tmp/acpy-ooo-repro
c++ -std=c++20 -O2 -I gfsim/cpp/include -I /tmp/acpy-ooo-repro \
  /tmp/acpy-ooo-repro/model.cpp pycircuit/examples/ooo/repro/runner.cpp \
  /tmp/acpy-ooo-build/gfsim/libgfsim.a -o /tmp/acpy-ooo-repro/run
/tmp/acpy-ooo-repro/run
```

本次输出：`natural=0 hoisted=2 expected=2`。宿主只读取两个 Signal。
复现不会把错误输出定义为应当长期保持的语义；退出码只检查替代写法。

## F2：动态 payload 字段 revise 的已知边界

自然的集中状态写法是 `rob.value[index].done = True`；当前前端文档明确不支持
动态下标的 Queue payload 字段 revise。本模型用固定的独立 Queue 阵列，并将
ROB 元数据、操作数、发射标记和两路完成结果拆开。动态选择
`entries[index].value` 是现有受支持接口，示例和语言回归已覆盖。
边界复现见 [dynamic_payload.py](repro/dynamic_payload.py)：
`rows.value[index.value].done = True` 编译失败，诊断为
`local field update requires a named struct`（第 12 行）。

```bash
python3 -m pycircuit compile pycircuit/examples/ooo/repro/dynamic_payload.py \
  --top Probe --output /tmp/acpy-ooo-payload-repro
```

影响：顶层连接数量增加，但每份状态只有一个写入 Rule，完成和唤醒可以独立推进；
无需改变编译器或时序。非阻塞。

## 结论与剩余边界

计划内能力均已在当前接口上实现。精确错误以提交事件的 fault 编码和 stopped 状态
报告，不提供 trap/CSR。64 位代号与序号的溢出不在有界测试范围内。访存采用保守顺序，
没有 Store 转发、缓存或访存推测。F1 应由后续编译器工作单独修复。
