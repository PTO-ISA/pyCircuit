# ACIR → C++ 仿真器生成：现状与实施计划

| 字段 | 值 |
| --- | --- |
| 负责范围 | `ac-lower-to-acsim` 的进程/队列降级 + `acsim-emit-cxx` 全部 |
| 契约纪元 | `0.1` |
| 规范依据 | [acsim-gfsim-lowering-v0.1](../specs/acsim-gfsim-lowering-v0.1.md)、[gfsim-runtime-abi-v0.1](../specs/gfsim-runtime-abi-v0.1.md) |
| 状态 | P0–P5 已落地并可验收；窄切片（同步 tick、FIFO、整数 datapath）能 freeze → lower → emit → 编译运行。设备正规化与 live/captures 仍有残留 |

适用模型：同步 tick、FIFO 握手、整数 datapath、少量 register/regfile/resource/stat。不是通用 HDL 或事务级后端。

## 1. 负责边界

完整链路分六段，本文档只负责后三段：

```text
Python/手写 ACIR
  → ac-freeze-topology            （不负责）
  → ac-resolve-gfsim-bindings     （不负责，已内联进降级 pass）
  → ac-lower-to-acsim             ← 负责进程体与队列降级部分
  → acsim-verify                  ← 负责与发射器相关的不变量
  → acsim-emit-cxx                ← 全部负责
  → acsim-check-cxx-contract      ← 全部负责
```

驱动：

- `acir-opt --ac-lower-to-acsim --acsim-output-dir=DIR`：降级并发射。
- 同上再加 `--acsim-check-cxx-contract`：发射后比对嵌入指纹与 manifest。
- 仅 `--acsim-output-dir=DIR --acsim-check-cxx-contract`（无 `--ac-lower-to-acsim`）：只校验已有 `DIR`，不对 frozen ACIR 再 emit。
- `acir-build`：freeze → lower → emit → host `c++` → 原子发布 `sim`。失败不替换上一版产物。

关键源文件：

| 文件 | 职责 |
| --- | --- |
| `lib/Conversion/ACIRToACSim/ACIRToACSim.cpp` | ACIR 进程体 → ACSim 状态机、队列/设备 intern |
| `lib/CodeGen/EmitCxx.cpp` | ACSim → C++20 源码 + build manifest + 指纹校验 |
| `lib/Dialect/ACSim/ACSimOps.cpp` | 发射前的 ACSim 验证器 |
| `include/gfsim/` | 运行时（不生成，只被生成代码引用） |
| `tools/acir-build/` | 编译闭环与原子发布 |
| `include/gfsim/register.h` | `Register<T>`、`RegFile<T,N>`（index 0 恒 0） |

不变量（改降级/发射时不得破坏）：

- `yield_sim` → `suspend @entry` + next tick，不是 terminate。
- send 与 recv 之间至少隔 1 tick，xfer 才能看见队列。
- 发射器禁止按组件名 `pc` / `rf` / `busy` 分派；`rg 'acir\.(pc|rf|busy)' lib/` 必须为空。
- complete 前缀来自 `ac.assert` 的 message（须是 identifier）。
- JSON `build_fingerprint` 是替换嵌入占位符**之前**的 composite。

## 2. 现状：已实现的功能

### 2.1 产物与驱动

`acir-opt --ac-lower-to-acsim --acsim-output-dir=DIR` 在 `DIR` 下产出：

| 路径 | 内容 |
| --- | --- |
| `include/generated/model.h` | Owner/Process 类声明、dispatch 表、`GeneratedModel`、`kBuildFingerprint` |
| `src/generated/model.cpp` | 构造函数、`work`/`xfer`/`reset`/`validate`、thunk、调度表装配 |
| `src/generated/main.cpp` | JSON 入口；`--max-ticks` / `--max-events` |
| `build-manifest.json` | schema `agentic-circuit-build-manifest`，含源文件 SHA-256、`build_fingerprint`、providers / specialization_inputs / instrumentation_layers |

`acsim-check-cxx-contract`（pass 名 `acsim-verify-cxx-fingerprint`，避免与 CLI `cl::opt` 撞名）回读头文件嵌入指纹，与 manifest 精确相等；改坏则 `ACSIM-CHECK-CXX-CONTRACT`。发射失败走 staging，不写最终目录。

`examples/{adder,riscv-mini,yield-only}/run.sh` 调用 `acir-build`。handshake / queue-i64 / mul-latency 有 `model.mlir` 与 gtest，无独立 `run.sh`。

### 2.2 结构生成

- 每个 `acsim.module` 生成一个 `Owner` struct，命名空间 `acsim_generated::<Mod>::s<64hex>`。
- 每个 `acsim.process` 生成一个 `Process` struct，命名空间追加 `::<proc>::p<64hex>`。
- `acsim.instance` → 按值成员；`acsim.array` → `std::array<T, volume>`（多维按体积展平）。
- FIFO 队列 → `gfsim::SimQueue<T>`，`T` 由载荷宽度决定（`uint8/16/32/64_t`），构造传入 entry capacity；`byte_capacity` 只进入 intern 符号，不改变运行时容量。
- `watermarks.kind = register|regfile` → `gfsim::Register<T>` / `RegFile<T,32>` 成员（降级仍识别遗留队列名 `pc`/`busy`→register、`rf`→regfile，发射器不按这些名字分派）。
- `ac.resource` → `gfsim::Resource`；`ac.stat` → Owner 上的 `uint64_t` 计数器。
- `std::array<gfsim::DispatchThunk, N> dispatch`，按 object ID 索引。
- 激活邻接按 `activation_offsets[N+1]` / `activation_targets[E]` 压缩数组发射；边来自 bind / captures / invoke 操作数，**没有**全对象自边。
- Owner `commitQueues(epoch)` 统一 `doXfer`（队列）与 `doArbitrate`+`doXfer`（资源）；进程 `xfer()` 调 `owner->commitQueues`，挂起时 `scheduleWork`。
- 模块体未知算子 → `ACSIM-EMIT: unsupported module operation`（`test/CodeGen/emit-unsupported-bind.mlir`）。
- `GeneratedModel` 构造函数完成 `bind` + `setDispatchTable` + `setActivationGraph`。

### 2.3 进程状态机

- `enum class Pc : std::uint8_t`（多 PC 时为 `{entry, s1, …}`），`work()` 是 `while (steps < fairness_cap_ && !suspended_ && !terminated_)` 包住的 `switch`。
- 单基本块直接顺序发射；多基本块时预声明全部 SSA 值，再用 `goto <pc>_blkN`（标签带 PC 名，避免多 PC 重复 `entry_blk*`），每块包在 `{}` 内避免跨初始化跳转。
- `wait_until` / `wait_for` / `await_event` 在顶层各产生新 PC，用 `acsim.continue` / `acsim.suspend` 连接。
- `yield_sim` → suspend `@entry` + `impl_wake_next_tick`。
- `xfer()` 提交 `pc_`、live slot，并调用 Owner `commitQueues`；挂起时 `scheduleWork(id, proposedWake_)`。
- 公平上限来自 `ProcessStatePlan`，datapath 进程额外抬到 `sourceOps * 3 + 16`。
- `validate()`：已知 PC 返回 true，否则 false。`SimSystem` 在 `validated` profile 下调用 thunk `validate`，失败 → `Failed` / `validate_failed`。
- t=0 调度所有 `work != nullptr` 的 dispatch 行。

### 2.4 已覆盖的算子

进程体内：`arith.constant`（整数、index、`i1`、宽度 > 32、浮点字面量）、`addi`/`subi`/`muli`/`andi`/`ori`/`xori`/`shli`/`shrui`/`shrsi`、`cmpi`（10 个谓词，有符号谓词带有符号转换）、`select`、`scf.if`、`cf.br`/`cf.cond_br`、`acsim.live.load`/`live.store`（发射器已接；降级侧不产出 live slot）、`acsim.invoke`/`inline`、`acsim.continue`/`suspend`/`terminate`。

跨区域 SSA：降级 `mapValue` 把常量再物化到当前 region，其它缺值填 0。

内建 invoke（发射器按 `acsim.type` 的 **cpp 名** 特判，不是组件名）：

| cpp 名 | 生成代码 |
| --- | --- |
| `acir.queue.push` | `owner->q.proposePush(v)` |
| `acir.queue.pop` | `proposePop()` + `value_or(0)` / `has_value()` |
| `acir.register.load` / `acir.register.store` | `gfsim::Register<T>::load/store` |
| `acir.regfile.read` / `acir.regfile.write` | `gfsim::RegFile<T,N>`，index 0 恒 0 |
| `acir.complete[.<prefix>]` | `requestTerminate(Completed, "<prefix>=" + value)` |
| `acir.fail` | 条件为假时 `requestTerminate(Failed, "failure")` |
| `acir.resource.acquire` / `release` | `gfsim::Resource::proposeReserve/proposeRelease` |
| `acir.stat.add` | Owner 计数器 + `recordStat` |
| `acir.schedule` | `SimSystem::scheduleEvent` |
| `acir.probe` | 按 intern 符号发射 |
| `acir::generated::impl_wake_next_delta_*` | `epoch.nextDelta()` |
| `acir::generated::impl_wake_next_tick` | `{epoch.time + 1, 0}` |

`ac.assert` 且能找到整数 recv 时 intern `acir.complete.{message}`；`ac.require` / `ac.ensure` 走 `acir.fail`。无 recv 的 assert 不在 `scf.if`+recv 上会走 fail。

队列 intern：`acir.queue.{Mod}.{name}.i{width}[.bytesN].cap{n}`。非 fifo、非 i8/i16/i32/i64、`delay_ticks != 1` → `ACLOWER-UNSUPPORTED-CONSTRUCT`（`delay_ticks != 1` 在 ACIR 方言也会先拒绝）。`ac.time_domain` 拒绝；`ac.event_queue` / `ac.instrumentation` 在模块体跳过。

### 2.5 确定性

`acsim.type` 与队列成员按符号排序，SSA 名按遍历序 `v0..vN`，激活边排序去重，源文件与 manifest 用 RFC 8785 + SHA-256 指纹。相同输入重复生成字节一致。指纹：源里先放 64 个 `0` 占位，finalize 后再替换；JSON `build_fingerprint` 是替换前的 composite。

### 2.6 已验证的端到端案例

| 例子 | 结果 | 测试 |
| --- | --- | --- |
| `examples/yield-only` | `max_deltas_exceeded`（预期） | `EmitCxxTest.GeneratedSimulatorCompilesAndRuns` |
| `examples/adder` | `completed / sum=5 / time=3` | `test/Conversion/adder.mlir`、`test/CodeGen/emit-adder.mlir`、`EmitAdder` |
| `examples/queue-i64` | `completed / sum=5` | `test/Conversion/queue-i64.mlir`、`EmitQueueI64` |
| `examples/handshake` | `completed / token=7`，PC `entry`→`s1` | `test/Conversion/handshake.mlir`、`test/CodeGen/emit-handshake.mlir`、`EmitHandshake` |
| `examples/riscv-mini` | `completed / x3=5 / time=16` | `test/Conversion/riscv-mini.mlir`、`test/CodeGen/emit-riscv-mini.mlir`、`EmitRiscvMini` |
| `examples/mul-latency` | `completed / product=21`，含 resource + `stat.add` | `test/Conversion/mul-latency.mlir`、`EmitMulLatency` |

负向：`test/Conversion/queue-unsupported.mlir`（`per_key`、非 8/16/32/64 宽度）、`test/CodeGen/emit-unsupported-bind.mlir`、`test/CodeGen/check-cxx-contract.mlir`（指纹 mismatch）。

带 `Emit*` 的 gtest 会调用 host 编译器编译并执行生成的仿真器。回归：`ninja -C build/local check-acir`（lit）、`CodeGenTests` / `GfsimTests` / `ACSimOpsTests`。

## 3. P0–P5 落地对照

原计划五期均已按验收标准落地。下表写**实际做到哪一步**，不是「尚未开始」。

| 期 | 计划目标 | 落地情况 | 残留 |
| --- | --- | --- | --- |
| P0.1 | 未知模块算子报错且不产文件 | `ACSIM-EMIT` + staging；`emit-unsupported-bind.mlir` | — |
| P0.2 | 队列提交归 Owner | `Owner::commitQueues` | 队列仍无独立 dispatch 行 |
| P0.3 | 去掉全对象自激活 | 无 self-`acsim.activate`；sparsity gtest | 进程靠 `scheduleWork` 自唤醒 |
| P0.4 | `validated` 调 `validate` | 未知 PC → false；失败 `validate_failed` | `validate()` 只检查 PC 是否合法 |
| P1 | 去掉 `acir.pc/rf/busy` 发射 | `lib/` 无这些符号；`Register`/`RegFile` | ACIR 仍是带 `watermarks.kind` 的队列，不是 `ac.module.extern @Register`；降级仍有 `pc`/`rf`/`busy` 名字回退 |
| P1 D2 | complete 结构化 payload | **未做** | 前缀仍来自 assert message |
| P2 | i64 FIFO + 负向 lit | `queue-i64`；`per_key` / i4 宽度拒绝 | `ac.packet` 不降级；`byte_capacity` 只进 intern 名；`delay_ticks≠1` 方言层已禁止 |
| P3 | 多 PC 握手 | handshake `pcs [@entry, @s1]` + gtest | 降级不产 live slot；captures 要求空；`scf.for` 不降级；riscv-mini 仍用 `busy` 串行 |
| P4 | 资源 / stat / schedule | mul-latency；`Resource` + JSON stats | `ac.event_queue` 跳过；assert 仍借道 complete |
| P5 | 指纹闭环 + `acir-build` | check-cxx-contract + 原子发布 `sim` | 无逐特化 C++20 concept 断言；handshake/queue-i64/mul-latency 无 `run.sh` |

## 4. 剩余技术债与功能缺口

### 4.1 规格残留

- **设备仍是队列伪装。** 规范要 `ac.instance of @Register` + binding；当前是 `watermarks.kind`，降级对遗留名 `pc`/`busy`/`rf` 仍有回退。发射器按 cpp 名（`acir.register.*` 等）特判，未做到「新组件只加 schema + binding + 模板、编译器零分支」。
- **complete 诊断字符串仍是 ABI**（`x3` → `"x3=5"`）。
- **跨挂起状态。** 发射器支持 live slot，降级从不生成；跨 region 靠再物化常量或填 0。
- **模块体跳过。** `ac.event_queue` / `ac.instrumentation` 静默 continue；`ac.stat` 在模块体跳过、从进程 `stat.add` intern。与「不支持则明确拒绝」不完全一致。

### 4.2 功能缺口

| 领域 | 现状 | 缺什么 |
| --- | --- | --- |
| 队列载荷 | signless i8/i16/i32/i64 + fifo | `ac.packet`、非 fifo 序 |
| 队列属性 | entry capacity；kind watermark；byte 只进符号 | 真正的 byte 容量、多拍 `delay_ticks`（方言也只允许 1） |
| 进程状态 | 多 PC 已接 wait_* | live slot 降级、captures |
| 结构化控制流 | `scf.if` | `scf.for` 展开、`scf.while` |
| 事件 | `wait_*`、`ac.schedule` | `ac.event_queue`、`ac.time_domain`（后者已拒绝） |
| 契约 | assert→complete；require/ensure→fail | 结构化 complete payload |
| Trace / 记录 | `ac.trace.open/next/decode/eof/position` + PTO JSONL i64 handle provider | `ac.record.*`、`ac.packet.*`、多 source CLI 映射 |
| 多结果 invoke | pop / regfile.read 特判 | 通用多结果 invoke |
| 流水线 | riscv-mini 四级但 `busy` 串行 | 去掉 busy 的重叠流水 |
| 编译闭环 | 指纹 + `acir-build` | 逐特化 concept 断言 |

## 5. 建议后续（优先级）

1. 用 `ac.module.extern @Register` / `@RegFile` 替换队列伪装，删掉降级里的名字回退。
2. 降级产出 `acsim.live.store` / `live.load`，captures 映射到真实操作数。
3. `acir.complete` 改为结构化 payload，不再解析 assert 文本。
4. 不支持的模块体算子一律 `ACLOWER-UNSUPPORTED-CONSTRUCT`，停止跳过 `event_queue`。
5. riscv-mini 去掉 `busy` 串行化，作为重叠流水验收。

## 6. 每期通用要求（后续改动仍适用）

- 正向 lit（`test/Conversion/` + `test/CodeGen/`）与负向 lit 成对提交。
- 至少一个 gtest 端到端真编译真运行，断言 `classification` 与 `diagnostic`。
- `scripts/check-ir-coverage.py --write-ledger` 重新生成 `spec-coverage.md`，覆盖率不得回退。
- 不得为单个组件在编译器里加**名字**分支；需要新能力时加 schema + binding + C++ 模板。cpp 名特判是当前过渡，不是目标形态。

## 7. 明确不做的事（v0.1 内）

- 不做 C++ 协程、宿主线程、动态续体帧或解释执行。
- 不做运行时组件注册表、插件、反射或 `dynamic_cast`。
- 不做跨编译器 ABI；同工具链源码契约足够。
- 不在 `@process` 之外生成任何组件行为代码。
