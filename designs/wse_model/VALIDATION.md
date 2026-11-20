# 验证状态与历史失败

## 本次发布候选

本目录从 wse-model 的迁移工作树导入，带成员结果和控制器状态日志。DUT 算法、状态、输入与断言按来源保留。Python 源导入摘要见 `reproduction/source-origin.json`；DUT 的 AST 保持一致；个别文件按本项目 Black 配置调整排版。原始设计文档保持字节一致，其目录内仅对原文的块引用/标题格式设置局部 Markdownlint 例外。本次发布检查结果如下。

- PASS：`python -m pytest designs/wse_model/tests/unit -q`，67/67 独立参考、接收输入和宿主比较测试。
- PASS：`prepare_compiler.py` 在新 worktree 应用 5 个提交，生成 tree 与历史源树完全一致；独立 reviewer 还通过临时 Git index 验证了补丁结果。
- PASS：使用历史已安装工具链 `505edbc7`，独立 CMake 项目配置及 `units/bench_allgather_control/unit.json` 目标成功，覆盖选定 System 的 8 个源单元；没有启动最终链接。
- PASS：全部 Calendar Python 文件与导入原版的 AST 相同，两处 `ac.log` 存在；函数规模读取器与历史顶层 Work 数据一致。
- PASS：按 CI 的 hook 版本执行改动文件 pre-commit（跳过该 CI 步骤也跳过的 `pyc-api-hygiene`），包括 Ruff、Black、Markdownlint；`mkdocs build --strict` 和 `git diff --check` 通过。
- 发布时 CI 状态单独见 PR；上述本地检查不代替 CI 或维护者批准。
- 未执行：本次发布候选的完整 ACIR 链接、C++ 生成及仿真、Verilog 仿真、全仓 API/example 原生门禁。复现用途的完整生成链路为长运行，下面历史证据不计为当前候选通过。

## 历史基线：带完整日志

历史工具链：pyCircuit `505edbc7dc2429640a41a31385ce3ff951c02aad`、LLVM/MLIR 22.1.8、Python 3.14.7、macOS ARM64、SDK 26.5、LLD。控制场景使用 40 节点载体、32 成员、8 位轮次、2 早到槽和 56 去重槽；行阶段 10752 B、列阶段 36864 B，每阶段按实际完成/退役状态切换。

源编译、最终链接与 C++ 生成成功。最终 ACIR 为 615316661 B，SHA256：

```text
e6fed27bf2a11495d6baa6248fb6d7423c575892185037588c14da48089935d3
```

顶层 `pyc_family_AllGatherControlSystem::Work()` 的完整源码为 785269 行、153110716 B，子任务分派为 0。完整函数包含签名及结束括号；此数值不等于机器码大小。

宿主以 `-O0 -DNDEBUG` 编译完成所有对象，在链接时失败，诊断摘录：

```text
ld64.lld: error: finalize: FIXME: thunk range overrun. Consider increasing the slop-scale with `--slop-scale=<unsigned_int>`.
clang++: error: linker command failed with exit code 1
```

按该版本 LLD 的隐藏选项把 `--slop_scale` 从默认 256 调到 512、1024，两次仅重试链接，均报同样错误。之后恢复原选项。目标文件的 Work 符号到下一已定义符号间距为 144888644 B（约 138.18 MiB）；这是相邻符号地址差，未据此认定精确函数尺寸或唯一失败重定位。

随后 O1 尝试在 21600 秒宿主构建时限内未完成，外层超时。用户要求停止后，遗留 clang 子进程被终止。调用栈曾从 SROA 变量提升转到 InstCombine，又进入后续调用图优化中的 SROA，说明优化阶段有推进；该尝试没有仿真结果。

## 历史对照：关闭子模块日志

这个实验版本不作为本目录默认 DUT 提交。它只从 `allgather_control.py` 删除两个 `ac.log` 表达式，其他 AST 节点、断言、状态、输入和工具链相同。构建采用 O0、单编译任务。

| 顶层完整 Work 指标 | 带日志 | 关闭日志 |
| --- | ---: | ---: |
| 行数 | 785269 | 764493 |
| 字节数 | 153110716 | 149355902 |
| 独立子 Work 分派数 | 0 | 0 |

关闭日志只减少约 2.45% 源码字节。正式捕获、链接和生成成功；9 个对象文件编译成功，最终 LLD 链接仍报 `thunk range overrun`，未生成可执行文件，523 周期运行未执行。

当前分派规则同时检查日志观察 `SourceObserveOp` 与检查操作 `SourceExpectOp`。手写断言和编译器生成的 `guard_known` 都可保留为后者；因此关闭日志不足以使带检查的子树恢复独立调用。输入准备的依赖限制也是另一道门槛，本次没有逐实例隔离评估其影响。

## 证据边界

以上历史数据来自本会话原迁移目录的原始日志、回执、生成物和独立 agent 复核。为保持 PR 为源码项目，未提交数百 MiB 的生成 C++/ACIR、对象文件、工具链及临时原始日志；本文提供可访问的诊断、条件、摘要与重建命令，便于维护者自行生成完整证据。关闭日志实验的逐成员观察比对不适用，不能据编译或无错误退出认定完整 WSE 功能验收。
