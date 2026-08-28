# DavinciOO FlashAttention-2 trace

这是 `B=1,H=1,S=128,D=64`、分块 `Br=32,Bc=64` 的在线 softmax FlashAttention-2
PTO JSONL。调度与时延仍由
[`../davincioo-matmul/model.mlir`](../davincioo-matmul/model.mlir) 定义。

每个 Q tile 的序列为：

1. `TLOAD Q`，`TEXPANDS` 初始化 `m/l/O`
2. 对每个 KV tile：`TLOAD K/V`、`TMATMUL QK`、scale、`TCOLMAX`、在线 max、
   `TEXP`、`TCOLSUM`、在线 sum、`TMATMUL PV`、用 `alpha` 重标定 `O`
3. `TRECIP`、归一化 `O`、`TSTORE`

生成器：

```bash
python3 examples/davincioo-fa2/generate_trace.py \
  examples/davincioo-fa2/fa2-b1-h1-s128-d64.pto.trace
```

运行仿真并写出 Perfetto JSON：

```bash
./examples/davincioo-fa2/run.sh
```

默认 `TIMELINE=examples/davincioo-fa2/out/fa2.perfetto.json`。把该文件拖进
https://ui.perfetto.dev 。

**依赖箭头默认不铺满整张图**（Perfetto 为了性能）。请：

1. **单击**一条 Engine 切片，入边/出边会高亮
2. 或者框选一段时间，右侧 **Flow events → Show all**

箭头从生产者 Engine **begin** 连到消费者 Engine **begin**。切片 `args.deps`
列出最多三个生产者 `sequence_id`。
