# WSE Calendar model and compiler reproducer

本项目发布当前基于 pyCircuit 的 WSE Calendar 控制、路由和接收模型，供编译器维护者复现大型 System 的链接、代码生成和宿主 C++ 构建问题。导入来源为 [PTO-ISA/wse-model](https://github.com/PTO-ISA/wse-model) 的迁移工作树，原始文件摘要见 [source-origin.json](reproduction/source-origin.json)，许可证见 [LICENSE](LICENSE)。

## 模型范围

- `models/calendar/`：pyCircuit DUT（被测设计）、System 测试输入、NDF（规范定义格式）条款及依赖记录。成员结果和控制器状态的 `ac.log` 均已保留，断言与自动检查也保留。
- `src/wse_model/`：独立普通 Python 语义参考的依赖闭包；它不参与 DUT 代码生成。旧 `agentic_circuit` 层不在本项目中。
- `tools/`：原有参考模型与 C++/Verilog 验证入口；`tests/unit/` 检查参考结果、输入编码和宿主比较逻辑。
- `reproduction/`：分阶段编译复现工具及历史编译器源补丁串。
- `docs/design/`：按原始字节保留的白皮书和 Calendar 方案；`docs/decisions/` 记录有限资源及溢出语义。

默认场景是 40 节点载体、32 个参与成员、8 位轮次、2 个早到槽和 56 个去重身份槽。行、列两个阶段由实际配置、接收完成和退役状态推进。基线为 523 周期；延迟例仅改变节点 31 的接收建立许可和节点 30 的片段接纳许可，预期 529 周期。容量和轮次宽度都是显式验证配置。

这是控制与通信功能模型。Cube/Vector 数值计算、载荷存储、物理链路竞争和最终硬件性能不属于本次复现范围。

## 开始使用

纯 Python 检查不需要编译器工具链：

```bash
cd designs/wse_model
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -e '.[dev]'
python3 -m pytest tests/unit -q
```

完整的分阶段命令、工具链前提和日志位置见 [REPRODUCE.md](REPRODUCE.md)。历史失败与本次发布候选的实际检查分别记录在 [VALIDATION.md](VALIDATION.md)。

本项目不注册进全仓示例 smoke 门禁；长时间编译由维护者按复现说明显式运行。
