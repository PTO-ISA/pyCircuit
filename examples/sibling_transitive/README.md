# 兄弟回调的当前状态与结果可用性

`Counter` 初值为 1，`increment` Method 每次加 2。
`bridge` 是一个未调用的只读 Method；声明本身不会激活它。
Writer 和 Reader 的独立 Rule 都从当前已提交状态开始，实例的构造顺序
不提供候选状态转发。

`read` Method 只在当前值不大于 5 时返回。Reader 的自动 Rule 以裸名称
连接到 Module 的 `ac.u8` 输出，System 再记录该子模块结果；编译器传播
结果可用性。第四个周期没有 `transitive visible` 日志，Writer 仍递增，
无需额外 valid 字段或保持寄存器。

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
