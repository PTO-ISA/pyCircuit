# WSE Model

本项目提供 WSE（晶圆级系统）的普通 Python 语义基准，以及使用 pyCircuit
编写的通信功能模型。它是独立的 SDK（软件开发工具包）消费项目，不注册到
pyCircuit 示例 smoke 门禁。架构来源为 [PTO-ISA/wse-model](https://github.com/PTO-ISA/wse-model)。

## 实现范围

| 层次 | 职责 |
| --- | --- |
| 普通 Python 语义 | 拓扑、Calendar 路由与接收规则、容量、宿主约束；计算与存储的容量、时延估算 |
| 共享 Calendar 组件 | 配置准入、轮次、路由解码、逐跳转发和有限接收账本；原组件接口保留回归适配 |
| 通用通信功能硬件 | `WseMachine` 加载运行时程序，执行节点内存读写、多播、接收完成等待与退休 |
| 通用通信时序模型 | `TimedWseMachine` 用实际数据和资源事件推进预测时延；参数与容量显式记录 |
| 通信程序 | AllGather 的分组、布局、分片、地址和期望接收量；PTO 输入经语义分析后生成同一程序接口 |
| 专项时序分析 | 显式参数下的 UB 服务、Calendar 时窗、链路和端点调度，暂定参数保留 `pending` 状态 |

AllGather（每个参与者取得全组数据）是运行在同一机器上的程序。更换程序、
地址或数据时，加载新的输入文件；硬件不识别行列阶段、固定片号或某个组大小。
功能版本采用 40 节点、每节点 64 KiB、最多 64 字节拍、单片段在途的显式配置。
其执行周期描述功能实现的步数。新增时序版本按存储、链路和 Calendar 资源事件
推进模型时间，沿用显式暂定参数；详见 [时序执行模型](docs/timed-machine.md)。矩阵、向量及完整 FFN/Attention 的数值计算尚未接入电路。

## 目录

```text
models/machine/       通用通信功能/时序模型与宿主驱动
models/calendar/      共享通信组件、组件测试与规范
src/wse_model/        Python 语义、架构分析、程序表示及 PTO 映射
src/wse_model/programs/  通用程序编码与 AllGather 程序生成
verification/fixtures/  历史 AllGather 与最小存储事务验证源
spec/                通用机器与程序的当前验收规范
ndf.yaml             项目级规范与源码追溯入口
models/pto_allgather/  独立专项时序投影及参数（analysis-only）
tools/                构建、程序加载、PTO 处理与独立比较
tests/                单元、契约、黄金向量及集成测试
examples/             Python 示例与 PyPTO AllGather 输入
docs/design/          按原始字节保留的白皮书与 Calendar 方案
```

分层和实现边界见 [architecture.md](docs/architecture.md)，运行时程序接口见
[program-interface.md](docs/program-interface.md)，配置见
[profiles.md](docs/profiles.md)，构建入口见 [RUNNING.md](RUNNING.md)。

## 普通 Python 使用

从本目录执行，使用 Python 3.11 或更新版本：

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -e '.[dev]'
make check
python -m wse_model --json topology show
python examples/ffn_allgather.py
```

普通 Python 测试与原生电路构建是独立的验证阶段。外部产物比较测试在未提供
输入、输出路径时会跳过；PyPTO 示例需要单独设置其解释器。

## 编译器与验收

`models/calendar/pycircuit.lock.json` 指定 pyCircuit 集成提交
`7942030dd623bb8db6a0a3bcb061fa11f667f43a` 和 LLVM/MLIR 22.1.8。
原生执行需要该集成 checkout 及与其对应的 SDK；本目录不携带编译器补丁。

构建与执行分别记录：`build_wse_machine.py` 完成编译、链接、后端生成和原生构建；
`verify_wse_machine.py` 与 `verify_machine_rtl.py` 比较通用程序的实际输出；
`run_pto_allgather.py` 验证真实 PTO 数据链在同一模拟器上运行不同规模的程序。
组件、通用程序和 PTO 整链各自保存回执，绑定源码、运行时输入、最终 ACIR
（电路中间表示）与模拟器的哈希。条件与命令见 [RUNNING.md](RUNNING.md)。
生成产物、原始样本和验证回执保存在调用者选择的输出目录，不随项目源码发布。

## 许可

软件使用 BSD 3-Clause，见 [LICENSE](LICENSE)。设计文档权利说明见
[NOTICE](NOTICE)。
