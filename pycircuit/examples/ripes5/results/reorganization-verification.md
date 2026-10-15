# 目录整理验证

本次只调整文件位置、导入、路径和文档链接，没有修改 CPU／ISA helper／Demo 的实现；三个模型源码的 SHA256 与搬迁前相同。I、IM、IMC 仍共用一个参数化模型。

- 34 个搬迁文件全部存在；103 个本地文档链接检查通过。
- Python 编译检查和新 CLI 入口检查通过；原生 runner 从新位置重新构建成功，参考 CPU 源码未修改。
- 2 项回归通过：Demo 对照保存的原生轨迹；混合长度取指、跨 word 访存、乘除边界和 ECALL 排空，包含生成 C++ 的 ASan／UBSan。
- `compressed_calls` 重新运行 21 拍：原生 RV5S、生成 C++、事务 IR、RTL 友好 IR 与独立顺序预期一致，并通过三个阶段重载。
- 默认 Demo 产物从新源码位置重新生成；25 拍与保存的原生轨迹一致，两层 IR 一致。
- 报告生成器已从新位置运行，保留原有 66 个程序、75 个指令名称的统计。这是读取既有证据重新生成报告，没有重跑完整验收或性能 benchmark。

本次验证用于确认目录搬迁没有破坏入口，不代表解决了[审查报告](review-2026-10-15.md)中的问题，也没有执行真正 RTL 仿真。

原始验证证据：[结构与校验和](reorganization/layout-checks.json)、[回归日志](reorganization/regression.log)、[原生构建日志](reorganization/native-build.log)、[21 拍端到端结果](reorganization/live/compressed_calls/verification.json)、[报告生成记录](reorganization/report-generation.log)。
