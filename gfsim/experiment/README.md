# Ripes5 的 Python 参考实现

本目录为 C++/ACPy Ripes5 验收提供独立 Python 模型、汇编器、程序和轨迹比较工具。Python 引擎保留历史动态调度实现；当前 GFSim runtime 在 [cpp/](../cpp/README.md)，语义以 [spec](../spec.md) 为准。

| 目录 | 用途 |
| --- | --- |
| [examples/ripes5/](examples/ripes5/README.md) | Python CPU、固定原生参考的构建和逐拍观察适配器 |
| [engine.py](engine.py)、[construction.py](construction.py) | 参考模型使用的历史引擎和静态构造 |
| [review/](review/recorder.py) | 参考模型的轨迹观察和 HTML 工具 |
| `test_engine.py`、`test_signal.py` | 保证参考引擎行为稳定的回归 |

从仓库根目录运行：

```bash
RIPES5_REQUIRE_NATIVE=1 python3 -m unittest discover -s gfsim/experiment -v
PYTHONPATH=gfsim/experiment python3 -m examples.ripes5.run --case array_sum --html
```

原生参考准备见 [Ripes5 说明](examples/ripes5/README.md)。没有原生二进制时，普通 Python unittest 会明确跳过对应检查；上面的环境变量将缺失变为失败。C++/ACPy 正式验收一直要求真实原生参考存在。

输出默认写入 `reference/benchmarks/ripes5-python/`。当前三方固定周期测速入口统一在 [ACPy Ripes5](../../pycircuit/examples/ripes5/README.md)；旧 Python 迁移测速脚本及结果已删除。
