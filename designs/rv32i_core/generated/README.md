# C++ 与 RTL 生成产物

这里提供由同一份 PyCircuit CPU 源码生成的 C++ 软件模型和物理 RTL。
[中文实验报告](../EXPERIMENT.md)说明 NDF、Spec、实现与验证如何连接，并给出
可直接运行的步骤。[生成说明](artifacts.json)集中记录源码/SDK 版本、执行描述和
工具、工艺依赖。

| 路径 | 内容 |
| --- | --- |
| `cpp/` | 完整 C++ 头文件与源码、CMake 入口、端口和源码映射 |
| `rtl/` | 物理顶层、五个硬件模块、端口与源码映射 |
| `profiles/` | 功能执行描述；各产物目录保留时序执行描述 |

两种执行策略使用相同的计算与状态源码，因此产物各保存一份，执行描述分别保留。
`verification/artifact_driver` 提供编译和存储器/时钟适配，运行方式见
[直接运行生成产物](../EXPERIMENT.md#直接运行生成产物)。

可以直接浏览文件，或使用 Raw/download 和仓库 checkout 获取。最大的
[core.hpp](cpp/sources/rv32i_core/core.hpp)约 3.17 MB，适合通过 Raw 查看。
重新生成的公开命令见[从源码重新生成](../EXPERIMENT.md#从源码重新生成)。
