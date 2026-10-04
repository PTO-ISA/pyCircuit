# ACPy 编译链验收报告

当前三方性能结果见 [同批固定周期报告](benchmark-report.md)。下面的七次短程序计时保留历史口径。

生成版、仅 ACIR 重载生成版、手写 C++、当前 Python、固定原生 Ripes：13 程序 × 四配置，共 52 配置逐拍通过。使用同一输入和周期边界，不移动轨迹。原生版本验证为强制门槛。

Release 与 Clang ASan/UBSan/LeakSanitizer 均通过全部 8 项 CTest，其中包含 12 项编译器测试及原有 GFSim 回归。现有 Python GFSim 的 28 项测试也在强制原生参考模式下全部通过。LeakSanitizer 在受 ptrace 限制的沙箱内无法运行，正式内存验收在获准的沙箱外执行。

编译器测试覆盖完整消息序列、分支消费、别名去重、必要读失败原子清理、背压与候选保留、共享 RuleId／输出、资源身份和普通参数缓存、整值／嵌套字段 revise、动态资源阵列、Signal 过滤与共享读取、事件、短路／提前返回、定宽运算和固定数组。临时源码删除后，保存的 ACIR 可独立生成并运行。Ripes5 两种入口生成的三个 C++ 文件逐字相同。

## 代码量

| 类别 | 物理行数 | 非空行数 |
| --- | ---: | ---: |
| 独立 Python 编译器 | 1235 | 1152 |
| 通用 C++ 值／存储支持 | 79 | 78 |
| ACPy Ripes5 | 296 | 264 |
| 生成模型头文件及实现 | 2579 | 2579 |
| 编译器测试及补充电路 | 426 | 379 |
| ACPy 消息流水示例 | 49 | 40 |
| 验收／计时／报告工具 | 240 | 215 |
| 复用的宿主 runner | 195 | 194 |

计数包含注释，生成模型不重复计入通用支持头。既有 GFSim runtime 与 Python／原生参考没有计入编译器。逐文件计数及散列保存在 [results.json](results.json)。

## 历史短程序循环耗时

下表为缓存开、Module 正序配置的七次采样中位数。两模型各四配置的全部样本、预热、轮换次序、CPU affinity 和二进制指纹保存在 [timing.json](timing.json)。数字描述完整模型，不代表单独调度器成本。

| 程序 | 周期 | 生成版 ns/周期 | 手写版 ns/周期 | 生成/手写 |
| --- | ---: | ---: | ---: | ---: |
| array_sum | 120 | 2682.9 | 2388.2 | 1.123 |
| mixed_2026 | 167 | 2640.7 | 2325.9 | 1.135 |
| memory_loop_256 | 2054 | 2511.8 | 2058.6 | 1.220 |

计时包含逐拍循环、结束 marker 检查、runtime 计数与首次 Signal 初始化；不含构造、轨迹观察及 JSON。无性能专用编译路径。

## 重现

构建、两阶段编译、验收与计时命令见 [编译器 README](../../README.md)。原始证据位于本目录 `output/Release`、`output/Debug`。随后执行：

```bash
RIPES5_REQUIRE_NATIVE=1 python3 -m unittest discover -s gfsim/experiment -v > /tmp/acpy-build/python-regression.log 2>&1
python3 pycircuit/examples/ripes5/report.py --build /tmp/acpy-build --asan-build /tmp/acpy-asan
```

语言仍限于 README 列出的首版子集；运行时资源声明、运行时循环、跨 Signal 依赖和动态 payload 字段 revise 尚不支持。
