# 程序测试标准化记录

> 以下为标准化当时的记录。后续四项审查问题均已修复，并完成 79 项程序对照；当前结果见[修复验证](fixes-2026-10-15.md)。

本轮在 Ripes5 示例内建立统一的程序测试入口，尚未迁移到编译器测试框架或 `@ac.system`。CPU、ISA helper 和编译器实现没有修改。
使用方法见[测试流程](../tests/README.md)，程序配置见 [cases.json](../tests/cases.json)。

现在以独立 `.S`／`.c` 文件或现成 ELF 为输入。源码用例经过 LLVM 编译、ELF 段加载、初始化镜像与预期答案检查；
运行器按选择执行程序检查、Ripes 对照、编译器回归。快速检查不再默认运行两层 IR 和三个阶段重载。
程序测试与性能计时使用独立入口，本轮没有重新测量性能。

本地程序共 34 份（包含独立性能循环），上游 31 个适用自检仍引用固定来源和校验和。
统一清单包含 67 个验证用例和两个单独选择的开放问题反例。IM／IMC 共用 CPU 模型和 C 算法源码，通过 ISA 配置生成不同 ELF。

历史 `acceptance`、`legacy`、`special` 和 `suite` 命令转入统一入口；`suite.prepare()` 的原 53 项清单接口保留给历史报告生成器。
`verify` 保留保存轨迹回归，`e2e` 保留比较器和历史执行入口。历史报告、原始运行目录和旧审查证据没有覆盖。

| 原审查问题 | 当前状态 | 证据 |
| --- | --- | --- |
| ELF 只抽取指定段、假定固定段起始地址 | 已修复：按 PT_LOAD 地址加载，补零 BSS，校验边界与重叠 | [独立加载器测试](../tests/test_harness.py)、[额外数据段 CPU 对照](standard/standardization-20261015/smoke/elf_extra_section/result.json) |
| store 宽度漏检、比较时修改参考数据 | 已修复：独立比较参考宽度；不修改参考轨迹 | [宽度故障注入记录](standard/standardization-20261015/audit/findings.json) |
| 合法跳转的错误路径取指越界 | 仍开放；新入口保留实际失败 | [失败记录](standard/standardization-20261015/audit/audit_wrong_path_fetch/result.json) |
| store 后立即打印字符串的观察时序 | 仍开放；新入口检查到原生输出差异 | [失败记录](standard/standardization-20261015/audit/audit_store_then_print/result.json) |

四项问题统一保存在 [known-issues.json](../tests/known-issues.json)。登记问题不会自动将失败变成通过。
此外补充故障注入，确认退出后的年轻指令副作用和错误 trap 类型能被程序检查器检出。

本轮完成的验证：

- **67 个验证用例全部完成 ELF 构建和预期结果检查。** 这一步没有运行完整 CPU。
- 原有 **62 个常规程序的新旧初始化输入完全一致**，包括机器码、ISA、初态和结束条件。[输入对照](standard/standardization-20261015/input-equivalence.json)
- 六个默认快速用例通过程序检查及原生 RV5S 逐拍对照，包含汇编算法、C 的 IM／IMC 版本、压缩跳转、历史流水线用例和 ELF 反例。[快速检查](standard/standardization-20261015/smoke/summary.json)
- 一个现成 ELF 直接加载并通过 C++ 程序检查，确认该模式没有执行 Ripes 或 IR 解释；故意设置错误预期答案时命令返回 1。[ELF 运行](standard/standardization-20261015/external-elf/summary.json)、[错误答案拒绝](standard/standardization-20261015/wrong-expectation/summary.json)
- `compressed_calls` 分别通过编译器模式和合并模式；合并模式共 21 拍，程序、原生、两层 IR 解释与三个阶段重载均通过。[合并检查](standard/standardization-20261015/all-checks/summary.json)
- 乘除极值、所有访存偏移与 `upstream_rvc` 通过 C++ 的 ASan／UBSan。前两项与原生一致，rvc 仍仅有已记录的六项参考差异，单列为参考缺陷。[边界检查](standard/standardization-20261015/boundaries/summary.json)
- 四个参考差异专门用例得到两个参考缺陷、两个参考不支持；没有计为原生对齐通过。[参考分类](standard/standardization-20261015/reference/summary.json)
- 用修复后的比较器重新核对原有 62 个保存轨迹，共 6049 拍，没有新增差异。[轨迹复核](standard/standardization-20261015/saved-trace-review.json)
- 相关 CPU／前端 9 项及测试框架当时的 6 项回归通过；随后增加两项检查器故障注入，最终测试框架 8 项均通过。[15 项回归日志](standard/standardization-20261015/regression.log)、[最终框架检查](standard/standardization-20261015/harness-final.log)
- 原来的四项问题全部复测：两个已修复、两个仍可复现。[复测记录](standard/standardization-20261015/audit/findings.json)

[本轮结果汇总](standard/standardization-20261015/verification-summary.json)、[源码校验和](standard/standardization-20261015/final-source-sha256.json)、[全部原始证据](standard/standardization-20261015/)。

当前尚未重新运行全部 67 个用例的完整 CPU 对照，也没有重跑整个编译器回归或执行真正 RTL 仿真。
本轮结果证明新测试流程和所测检查有效，尚不代表完整 ISA 验收。两个开放问题仍需后续处理。
