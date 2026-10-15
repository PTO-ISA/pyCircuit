# RV32IMC 验收报告

> 此为原用例的历史验收记录。审查提出的四项问题已在后续修复，当前结果以[修复验证](fixes-2026-10-15.md)为准；本页保留原来的统计与证据。

66 个完整 CPU 程序：61 个原生逐拍严格一致，3 个确认参考缺陷，2 个参考不支持。
严格一致程序共 5798 拍；所有程序均核对生成 C++、事务 IR 解释、RTL 友好 IR 解释和独立顺序预期。
最终完整编译器回归：76 项通过。没有生成 Verilog，也没有执行 RTL 仿真。

参考源码均未修改。Ripes / VSRTL 固定 commit：

- Ripes `5b8a616edcb6f0a2ddb07e78951348b72497f1e1`。
- VSRTL `8497dd14fe80e57efcff4c424a9a3b6363d93eb7`。
- 原生 runner SHA256：`caa8ca33ea01b39046628781cc806b4a6da1a5af6925797a5c8443f875606e01`。

## 逐拍比较

以拍初原生组合信号对照模型在拍末保存的控制决策；两边拍末比较 PC、各级原始 valid／PC、气泡、前递、enable／clear、实际下一 PC、IF/ID 展开编码、寄存器、全部 4 KiB RAM、load／store、退休和 ECALL。最终还检查退出状态与输出。
原生 stageInfo() 的退出后显示隐藏与硬件 valid 分开处理；比较使用原始寄存器，没有将显示隐藏解释为提前冲刷。
顺序预期由 LLVM 独立反汇编提供指令，用 Python 无界整数和 byte 内存解释，不调用 ACPy decode／decompress／ALU。上游自检另要求退出值 42；算法另核对 720、21、55、500、134。

## 参考差异

- `upstream_rvc`：confirmed-reference-defect。保留两次错误路径 `0x0000` 的 2／4 字节取指差异，共 6 项 PC／stage／下一 PC 观测差异；退休和体系结构结果正确。
- `jalr_low_bit`：confirmed-reference-defect。RV5S wires ALU result directly to pc_src without the JALR bit-zero mask; native runner encounters the odd PC.
- `ebreak`：confirmed-reference-defect。rv_decode.h maps every SYSTEM opcode to ECALL, including EBREAK. Native invokes syscall 0 and fails; the model emits a break event.
- `c_ebreak`：reference-unsupported。rv_uncompress.h explicitly leaves C.EBREAK unimplemented; native executes the younger store instead of emitting a break event.
- `illegal_instruction`：reference-unsupported。Ripes decode maps unknown opcodes to NOP; this model reports an illegal-instruction event and flushes younger instructions.

差异不计为原生对齐通过，也不自动新增忽略项。rvc 的每项差异均保存精确周期、字段和值；变化即失败。

## 覆盖与输入

实际执行覆盖 RV32I 40 条、M 8 条、RV32C 整数 27 条，共 75 个指令名称。HINT 另由完整 CPU 回归核对 x0、内存和唯一 C.EBREAK 事件。
31 个适用上游自检全部纳入；其余上游项逐项列明 F/D、RV64、交互／文件／堆分配等排除原因。rvc.s 保留原文件、校验和与语法／布局适配说明。
原有 9 个程序保留并重新采集。另有 5 个汇编算法、5 个 C 算法的 IM／IMC 版本、7 个边界／固定种子程序，以及 4 个参考差异程序。
文本 [0,0x1000)，RAM [0x1000,0x2000)，默认 sp=0x2000；输入包括机器码 bytes、入口、RAM、全部寄存器、ISA、结束条件、周期上限与 SHA256。所有分段及 BSS 均做边界检查；越界明确失败。

## 编译和边界检查

通用显式整数转换支持全部 8／16／32／64 位 signed／unsigned 组合；由源符号性决定扩展，同宽保留位模式，缩窄保留低位，lower 到标准 MLIR 整数操作。64 种组合均覆盖优化开关、阶段重载和 sanitizer。
M 除法使用无符号绝对值实现向零截断，并保护除零和 MIN/-1；M 极值、访存边界及参考差异程序运行 ASan／UBSan（宿主 LeakSanitizer 不可用，设置 detect_leaks=0）。
4 KiB RAM 使用一个保存固定数组的 Cell，避免逐项初始化导致大规模重复构造。跨 word store 在局部 payload 上合并后一次提交。
越界访问在进入数组读取／更新前分支报错，避免动态越界的未就绪取消掩盖断言。Fetch 退出排空只计算长度，不重复求完整压缩展开；曾发现重复展开导致 sanitizer 栈开销过大，已收敛并复验。没有添加 CPU 专用编译路径。

CSR、特权、交互输入、文件、浮点和其他扩展不在本次范围。

## 交付索引

- [指令覆盖表](instruction-coverage.md)／[原始覆盖](instruction-coverage.json)。
- [程序清单](program-manifest.json)／[全部上游纳入与排除清单](upstream-inventory.json)。
- [参考缺陷精确预期](../tests/fixtures/reference-differences.json)。
- [本地验收汇总](acceptance/summary.json)。
- [完整编译器回归日志](acceptance/compiler-regression-final.log)。
- [参考构建来源与命令](native-build/provenance.json)。
- [Ripes MIT](../tests/licenses/Ripes-MIT.txt)／[riscv-tests BSD](../tests/licenses/riscv-tests-BSD.txt)／[来源说明](../tests/licenses/riscv-tests-provenance.txt)。
