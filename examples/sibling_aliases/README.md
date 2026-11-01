# 引用别名与局部快照

Current migration status and remaining work are tracked in the
[catalog](../catalog.json) and [migration inventory](../README.md#remaining-migration-work).
This example uses the transactional frontend with explicit Rule registration.
Its four-cycle independent history passes generated C++ with workers 1 and 2
and Verilog simulation. Catalog integration is tracked separately.

System 创建两个同类型的 `Counter`，初值都是 2。
`Relay(shared)` 将已有实例继续传给 `Stepper`；两层引用传递和别名赋值
都不创建 Counter，也不复制状态。

`step_twice` 显式注册，在一个事务中通过别名调用 `increment_twice` Method。
Method 用普通局部变量计算 `first = count + 1` 和 `second = first + 1`，
然后只写一次 `count = second`。两个递增属于同一个原子操作，
不依赖候选状态转发，也不产生对同一 State 的重叠写。
通过原名调用 `read` 仍返回已提交的旧值。另一个独立 Counter 始终保持 2。

第三个日志字段从候选值改为 old Q，期望值依据当前读取语义推导。
四个周期的 Work 观测如下，每行在低、高采样时刻各输出一次：

| 周期 | `alias snapshots`：first, second, 已提交旧值 | `independent count` |
| --- | --- | --- |
| 0 | 3, 4, 2 | 2 |
| 1 | 5, 6, 4 | 2 |
| 2 | 7, 8, 6 | 2 |
| 3 | 9, 10, 8 | 2 |

在仓库根目录使用已安装工具链运行：

```sh
pycircuit run examples/sibling_aliases --target cpp --workers 1 --cycles 4
pycircuit run examples/sibling_aliases --target cpp --workers 2 --cycles 4
pycircuit run examples/sibling_aliases --target verilog --cycles 4
```

可追加 `--toolchain /path/to/install`。两个 Python 文件独立编译；
System 生成 C++ 与 RTL 仿真入口，独立期望值由共享 example verifier 检查。

要自动核对上表的预期轨迹，在仓库根目录执行以下命令，并将安装路径换成
当前工具链路径。CTest 覆盖 C++ workers 1、2 和 RTL：

```sh
cmake -S examples/sibling_aliases -B .pycircuit_out/examples/sibling_aliases -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/install
cmake --build .pycircuit_out/examples/sibling_aliases --parallel 4
ctest --test-dir .pycircuit_out/examples/sibling_aliases --output-on-failure --no-tests=error
```
