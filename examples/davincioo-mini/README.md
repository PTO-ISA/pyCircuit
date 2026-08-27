# DavinciOO Mini：嵌套 ACIR → gfsim

这是一个可由当前 `acir-build` 直接生成 C++ 并运行的简化乱序核。它不复制
DavinciOO 参考源码，而是保留相同的拓扑形状：`Core` 拥有互连 FIFO，各流水级
是独立子 `ac.module`，四类执行引擎通过 wakeup 和 ROB 形成完成、唤醒、按序退休
闭环。

运行：

```bash
./examples/davincioo-mini/run.sh
```

成功结果包含：

```text
"classification":"completed"
"diagnostic":"retired=6"
```

## 模型结构

- `TraceSource`：唯一 trace 入口，注入 6 条硬编码指令。
- `ROB`：分配 1–8 的序号、记录完成位并按序退休。
- `Rename`：恒等 rename。
- `Dispatch`：按 packed instruction 的 engine 字段分发。
- `ReadyTable`：把 engine completion 同时转发到 ROB 和四个 IQ。
- `IssueQueue{S,V,C,T}`：本地 ready 位图和确定性单发选择；每类声明 4 个槽，
  当前 bounded trace 每类只激活头槽。
- `Engine{S,V,C,T}`：独立 busy/remain/current 状态和可变延迟倒计时。
- `Core`：拥有所有跨 stage FIFO、共享 `retired` 寄存器和静态实例图。

队列载荷为 packed `i32`：

```text
[7:0] opcode | [9:8] engine | [13:10] dst | [17:14] src0
| [21:18] src1 | [25:22] latency | [31:26] rob_id
```

## 对照生成契约的 8 条不变量

1. **静态对象图**：保留。实例、FIFO 容量、四类 engine 和延迟字段在构造后固定。
2. **单一 trace owner**：保留。只有 `TraceSource` 写入 `rob_in`。
3. **rename/readiness 提交边界**：保留为一 tick FIFO 边界；rename 本身为恒等映射。
4. **确定性 oldest-ready**：简化。当前每类单发、头槽选择；4 槽完整年龄比较留待后续。
5. **四类 engine 并发与有限容量**：保留。四个 engine 独立倒计时，互连容量有限。
6. **完成唤醒、按序退休**：保留。completion 更新 ready 位，ROB 只退休连续 head。
7. **Work/Xfer 分离**：保留。FIFO 使用 propose/commit，跨 stage 数据下一 tick 可见。
8. **确定性身份和事件顺序**：保留。稳定层级路径、密集 object ID 和确定性 dispatch 顺序
   均由 ACSim/C++ 生成器产生，不依赖指针值。

## v0.1 明确简化

- 单发宽、ROB 深度 8、每类一个 IQ 和一个 engine。
- 硬编码 bounded trace，不解析 PTO JSON。
- packed `i32` 代替 `PTOInstRef`/packet。
- 恒等 rename，不实现 SMAP、free-list、flush、replay。
- IQ 的 4 槽完整 oldest-ready 年龄仲裁尚未实现；当前验收 trace 使用确定性头槽。
- 不生成 DavinciOO CLI、trace exporter 或参考工程文件布局。
