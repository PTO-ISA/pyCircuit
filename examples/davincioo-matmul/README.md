# DavinciOO Matmul trace 模型

这是一个独立编写的、由 PTO JSONL trace 驱动的乱序时序模型。它保留
`Core` 持有互连 FIFO、流水级作为子 `ac.module` 的结构，并包含：

- `TraceSource`：用本地 `i64` cursor register，经 `arith.index_cast` 调用
  `ac.trace.next`；只有 `rob_in` 接受 handle 后才保存新 cursor，EOF 时记录
  `trace_total`。
- 深度 8 的 `ROB`：使用单调 `head`/`tail`，槽号为
  `(sequence_id & 7) + 1`；满时不从输入取数，完成后严格按序退休。
- `Dispatch`：在 ACIR 中根据 opcode 定义 S/V/C/T engine 路由。
- 四个 8 槽 IQ：每个 IQ 有四个本地 `i64` ready bitset，可检查 descriptor
  中最多三个依赖，并在所有 ready 槽中选择 sequence 最小者。
- 四个独立 engine：在 ACIR 中根据原始 workload 和架构吞吐率计算时延，保存
  handle 和剩余周期，完成后广播 handle。
- ROB 统计：总退休数、6 类 opcode 退休数和 4 类 engine 退休数。

所有可能在下游阻塞前已经取数的级间 FIFO 都按 trace 上限设置为 136 项；
ROB 和 IQ 则先检查本地容量再取输入。模型支持最多 136 条、连续且从 0 开始的
`sequence_id`（这是当前 PTO trace reader 的约束）。

## 运行

`PTO_TRACE` 是必需参数：

```bash
PTO_TRACE="$PWD/examples/davincioo-matmul/synthetic.pto.trace" \
  ./examples/davincioo-matmul/run.sh
```

也可指定构建工具：

```bash
ACIR_BUILD=/path/to/acir-build PTO_TRACE=/path/to/input.pto.trace \
  ./examples/davincioo-matmul/run.sh
```

脚本先调用 `acir-build`，再执行生成的 `sim --trace "$PTO_TRACE"`。正常结束时
JSON 结果包含 `classification: "completed"` 和 `diagnostic: "retired=<条数>"`。

## descriptor

模型按以下布局解码 `i64` descriptor：

```text
seq[7:0] opcode[10:8]
dep0[18:11] dep1[26:19] dep2[34:27]
dep-valid[37:35] raw-workload[63:38]
```

opcode `0..5` 分别是
TASSIGN/TLOAD/TEXTRACT/TMATMUL/TMATMUL_ACC/TSTORE。

PTO provider 只从 trace 提取事实：opcode、依赖和原始工作量。传输类操作的
`raw-workload` 单位是 byte，矩阵乘操作的单位是标量 MAC。它不包含 engine
映射、吞吐率或周期数。架构性能假设唯一地定义在 `model.mlir`：

- Scalar：TASSIGN 固定 1 cycle；
- Vector：`ceil(bytes / 512) + 4` cycles；
- Cube：`ceil(MACs / 4096)` cycles；
- TMA：`ceil(bytes / 512)` cycles。

当前单 `i64` descriptor 的 raw workload 上限为 `2^26 - 1`。provider 对超限
trace 明确报错，不会静默截断；若未来需要支持更大 tile，应将 workload 扩展为
独立的第二个 trace metadata 字。

`synthetic.pto.trace` 是本示例单独创作的小型合法 JSONL，覆盖全部 6 种
opcode，并包含 0、1、2、3 依赖的调度情况。
