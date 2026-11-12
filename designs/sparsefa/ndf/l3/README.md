# L3 独立验证与证据

- [端到端和数值验收](fa2-core-eng-1001.md)
- [外部index、直接地址和稀疏集合](fa2-core-idx-1002.md)
- [模块、资源、事务与物理边界](fa2-core-arch-1002.md)
- [16×16×16 三维阵列、树形数值与流水结构](fa2-core-array3d-1003.md)
- [此前的 16×16 二维阵列验证基线](fa2-core-array-1003.md)
- [精确索引校验与完整2048项结构行为验收](fa2-core-idx-1004.md)
- [Shared-KV批处理、跨Head重用与双缓冲验收](fa2-core-skv-1006.md)
- [Shared-KV Index 双缓冲、逐批 KV 放行与跨批错误验收](fa2-core-idx-stream-1008.md)
- [本次实际通过/失败/未运行项](../../VALIDATION.md)

数学Oracle与真实生成DUT独立。compile、link、emit、构建、C++执行、RTL执行和PPA
分别记录；任何阶段NOT RUN都不能从相邻阶段推断通过。测试不依赖ignored gate日志。
