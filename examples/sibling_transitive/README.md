# 兄弟回调的当前状态与结果可用性

Current migration status and remaining work are tracked in the
[catalog](../catalog.json) and [migration inventory](../README.md#remaining-migration-work).
The Method-caller adapter uses the transactional frontend and explicit Rule
registration. Its four-cycle history passes C++ workers 1 and 2 and Verilog.
This is partial coverage; the original Module-output boundary remains pending.

`Counter` 初值为 1，`increment` Method 每次加 2。
`bridge` 是一个未调用的只读 Method；声明本身不会激活它。
Writer 和 Reader 的独立 Rule 都从当前已提交状态开始，实例的构造顺序
不提供候选状态转发。

`read` Method 只在当前值不大于 5 时返回。Reader 显式注册的 Rule
调用 Method 并记录同周期结果；Method guard 控制整个调用者事务，
包括日志发布。第四个周期没有 `transitive visible` 日志，Writer 仍递增。
该观察端不增加 Queue、valid 字段或保持寄存器，原有日志时序保持不变。

原有接口是 `Counter.read` → Reader Rule 的标量结果 → Reader Module 的
`ac.u8` 输出 → System 消费者 Rule。该跨 Module 输出的可用性传播仍是
必需能力。当前日志由 Reader 内部 Rule 发布，只验证 Method guard 对调用者
事务的取消，不验证原有输出连接及 System 消费者的阻塞。把输出改成 State
或 Queue 会改变采样时序，因此这一接口尚未迁移，不能用当前通过的轨迹抵销。

四个周期的 Work 观测如下。每个有值的格子在低、高采样时刻各输出一次；
`transitive committed` 来自独立读取已提交状态的 Rule。

| 周期 | `transitive committed` | `transitive visible` |
| --- | --- | --- |
| 0 | 1 | 1 |
| 1 | 3 | 3 |
| 2 | 5 | 5 |
| 3 | 7 | 无日志 |

在仓库根目录使用已安装工具链运行：

```sh
pycircuit run examples/sibling_transitive --target cpp --workers 1 --cycles 4
pycircuit run examples/sibling_transitive --target cpp --workers 2 --cycles 4
pycircuit run examples/sibling_transitive --target verilog --cycles 4
```

可追加 `--toolchain /path/to/install`。两个 Python 文件独立编译；
System 生成 C++ 与 RTL 仿真入口，独立期望值由共享 example verifier 检查。

要自动核对上表的预期轨迹，在仓库根目录执行以下命令，并将安装路径换成
当前工具链路径。CTest 覆盖 C++ workers 1、2 和 RTL：

```sh
cmake -S examples/sibling_transitive -B .pycircuit_out/examples/sibling_transitive -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/install
cmake --build .pycircuit_out/examples/sibling_transitive --parallel 4
ctest --test-dir .pycircuit_out/examples/sibling_transitive --output-on-failure --no-tests=error
```
