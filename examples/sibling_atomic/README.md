# 兄弟调用的原子更新

`Counter` 从 10 开始，每次加 3；`Ledger` 从 100 开始，每次加 10。
`Writer` 的一个 Rule 依次调用两个已有实例，两个更新必须共同提交。
第二个周期，`Ledger.record` 的业务条件不满足，整个 Writer 保持。

`increment`、`read` 和 `record` 都是可调用的 Method。
Writer 与 Reader 的 Rule 声明自动激活，各自从已提交状态开始。
Reader 读取当前值；Writer 的候选更新在提交后才会被下一周期看见。
Writer 被阻塞时 Reader 仍执行，无需手写转发或提交控制。

四个周期的 Work 观测如下。每行在低、高两个采样时刻各输出一次；
`atomic committed` 是独立 Rule 读取的已提交状态。

| 周期 | `atomic committed` | `atomic visible` |
| --- | --- | --- |
| 0 | 10, 100 | 10, 100 |
| 1 | 13, 110 | 13, 110 |
| 2 | 13, 110 | 13, 110 |
| 3 | 16, 120 | 16, 120 |

在仓库根目录使用已安装工具链运行：

```sh
pycircuit run examples/sibling_atomic --target cpp --workers 1 --cycles 4
pycircuit run examples/sibling_atomic --target cpp --workers 2 --cycles 4
pycircuit run examples/sibling_atomic --target verilog --cycles 4
```

可追加 `--toolchain /path/to/install`。两个 Python 文件独立编译；
System 生成 C++ 与 RTL 仿真入口，独立期望值由共享 example verifier 检查。

要自动核对上表的预期轨迹，在仓库根目录执行以下命令，并将安装路径换成
当前工具链路径。CTest 覆盖 C++ workers 1、2 和 RTL：

```sh
cmake -S examples/sibling_atomic -B .pycircuit_out/examples/sibling_atomic -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/install
cmake --build .pycircuit_out/examples/sibling_atomic --parallel 4
ctest --test-dir .pycircuit_out/examples/sibling_atomic --output-on-failure --no-tests=error
```
