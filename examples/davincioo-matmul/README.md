# DavinciOO Matmul trace 模型

这是一个独立编写的、由 PTO JSONL trace 驱动的乱序时序模型。它保留
`Core` 持有互连 FIFO、流水级作为子 `ac.module` 的结构，并包含：

- `TraceSource`：用本地 `i64` cursor register，经 `arith.index_cast` 调用
  `ac.trace.next`；只有 `rob_in` 接受 handle 后才保存新 cursor，EOF 时记录
  `trace_total`。
- 深度 8 的 `ROB`：使用单调 `head`/`tail`，槽号为
  `(sequence_id & 7) + 1`；满时不从输入取数，完成后严格按序退休。
- `Dispatch`：解码 descriptor 的 engine 字段，路由至 S/V/C/T 四个 IQ。
- 四个 8 槽 IQ：每个 IQ 有四个本地 `i64` ready bitset，可检查 descriptor
  中最多三个依赖，并在所有 ready 槽中选择 sequence 最小者。
- 四个独立 engine：各自保存 handle 和剩余延迟，完成后广播 handle。
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
seq[7:0] engine[9:8] latency[19:10]
dep0[27:20] dep1[35:28] dep2[43:36]
dep-valid[46:44] opcode[49:47]
```

engine `0/1/2/3` 分别是 S/V/C/T；opcode `0..5` 分别是
TASSIGN/TLOAD/TEXTRACT/TMATMUL/TMATMUL_ACC/TSTORE。

`synthetic.pto.trace` 是本示例单独创作的小型合法 JSONL，覆盖全部 6 种
opcode，并包含 0、1、2、3 依赖的调度情况。
