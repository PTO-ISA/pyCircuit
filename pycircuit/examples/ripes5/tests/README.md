# Ripes5 程序测试流程

当前先在示例内验证一套简单流程：独立汇编／C 文件 → ELF → 固定运行器 → 明确的检查结果。
程序、清单和结果格式独立于 ACPy 测试前端；尚未迁移到 `@ac.system`。后续修复了 CPU 的取指错误传递和原生运行器的 ECALL 观察时序，未修改编译器。

从仓库根目录运行，编译器环境按 [pyCircuit README](../../../README.md) 设置。

```bash
# 查看用例；只生成 ELF、初始化镜像和顺序预期
python -m pycircuit.examples.ripes5.tests list
python -m pycircuit.examples.ripes5.tests build --case asm_factorial

# 默认运行 8 个快速用例，检查程序结果及原生 Ripes 逐拍对照
python -m pycircuit.examples.ripes5.tests run

# 分别运行程序检查、编译器检查，或合并全部检查
python -m pycircuit.examples.ripes5.tests run --case compressed_calls --check program
python -m pycircuit.examples.ripes5.tests run --case compressed_calls --check compiler
python -m pycircuit.examples.ripes5.tests run --case compressed_calls --check all

# 直接运行已生成的 ELF；不会重新汇编，按 ELF 段地址加载
python -m pycircuit.examples.ripes5.tests run --elf path/to/program.elf \
  --isa IMC --expect-status 42 --check ripes

# 完整用例、sanitizer 和历史缺陷复测分别显式选择
python -m pycircuit.examples.ripes5.tests run --all
python -m pycircuit.examples.ripes5.tests run --case memory_offsets --sanitize
python -m pycircuit.examples.ripes5.tests.audit.probes
```

`--output` 指定本轮结果目录；默认在 `pycircuit/builds/benchmarks/ripes5-standard/<UTC 时间>/` 新建目录。
[results/standard](../results/standard/) 是集中结果入口，不覆盖原来的验收和审查证据。

| 检查 | 执行内容 | 不执行的检查 |
| --- | --- | --- |
| `build` | 汇编／编译、ELF 加载、顺序预期与预设答案核对 | CPU 执行 |
| `run --check program` | 生成 C++，完整 CPU 执行；核对退休、最终寄存器／RAM、环境事件和输出 | Ripes、IR 解释和阶段重载 |
| `run --check ripes`（默认） | 程序检查，加原生 RV5S 逐拍和最终结果对照 | IR 解释和阶段重载 |
| `run --check compiler` | 程序检查，加两层 IR 解释、三个阶段重载及相应 C++ 对照 | Ripes |
| `run --check all` | 合并程序、Ripes、编译器检查 | 性能计时 |

这里的 RTL 友好 IR 是解释执行，不是真正 RTL 仿真。检查中记录的编译和执行过程不作为性能 benchmark。
性能仍使用独立入口 `python -m pycircuit.examples.ripes5.tests.benchmarks.measure`，对应
[循环汇编](programs/benchmarks/mixed_loop.S)和 [C++ 计时程序](benchmarks/runner.cpp)。该入口保留原来的测量设置和结果目录，本轮没有重测或扩展其性能结论。

用例集中在 [cases.json](cases.json)，本地程序在 [programs/](programs/)。上游程序仍从 `reference/ripes-reference/` 读取，
逐项检查固定源码校验和；语法／布局适配集中在 [upstream.py](upstream.py)，原始源码和许可证复制到本轮证据目录。
本地程序不再藏在 suite 的 Python 字符串里。固定种子混合程序已保存完整汇编，种子在清单中记录。

清单当前包含 79 个验证程序，原来的两个审查反例已修复并归入常规 `boundary` 回归。
`--all` 包含全部 79 项；`--group` 可选择 `upstream`、`algorithms`、`boundary`、`legacy`、`reference`。`--case` 可重复；各选择方式互斥。

最小用例配置示例：

```json
{
  "name": "asm_factorial",
  "source": "programs/algorithms/asm_factorial.S",
  "language": "asm",
  "isa": "IM",
  "group": "algorithms",
  "expected_reason": "exit",
  "expected_status": 720
}
```

可选字段包括 `expected_output`、`max_cycles`、初始寄存器和 RAM、结束 PC。默认寄存器为零、sp=0x2000。
为了保留历史轨迹，原有 9 个程序显式设置原初始寄存器／RAM，并以指定 PC 退休结束。

运行环境采用 ELF32、小端、RISC-V 可执行文件，按 **PT_LOAD** 的地址加载，补零 BSS；
拒绝文件范围错误、段重叠、越界和不同的虚拟／物理地址。文本窗口 `[0,0x1000)`，RAM `[0x1000,0x2000)`。
当前平台的退出／打印使用 a7 选择的 ECALL：1 整数、4 字符串、11 字符、10／93 退出。
每个用例默认在退出后附三个 NOP 供流水取指，清单可通过 `pad_fetch: false` 关闭；传入 ELF 时不会追加指令。
错误路径取指错误随 IF/ID、ID/EX 传到 EX，可被较老跳转冲刷；真正执行到越界地址仍报错。
打印事件在有效 ECALL 的拍初捕获参数，拍末在较老 store 提交后读取字符串并完成输出；退出仍在拍初 finalize，保留参考流水清空时序。

每个用例输出 `program.elf`、`input.json`、`case.json`、`oracle.json` 和 `result.json`。
源码编译用例另记录工具命令；上层 `provenance.json` 记录模型／脚本校验和和原生构建来源。
Ripes、C++、IR 的原始轨迹只在相应检查执行时生成。首次失败保存 `first-mismatch.json` 和复现命令，命令返回非零。

`result.json` 的 `checks` 分别记录 `input`、`program`、`ripes`、`compiler`，未执行的检查标为 `not-run`。
顶层状态为 `built`、`passed`、`failed`、`reference-defect` 或 `unsupported`。
已确认参考差异和参考不支持不计为通过；已记录差异的专门检查成功时命令可返回零，汇总仍单列这些状态。
普通新差异一律失败。`regression_issue` 只记录回归来源，不会改变通过／失败判定。

[known-issues.json](known-issues.json) 保留四项审查发现。ELF 漏段与 store 宽度漏检已修复并加入独立加载／故障注入回归；
后续 CPU 取指边界和字符串 ECALL 采样问题也已修复，四项均有复测证据。[历史审查报告](../results/review-2026-10-15.md)保留原结论与证据，
[标准化记录](../results/standardization.md)保留当时状态；当前结果见[修复验证](../results/fixes-2026-10-15.md)。

后续若迁移到编译器测试框架，保留 `.S`／`.c`／ELF、用例清单和结果约定，替换模型初始化、执行和状态观察适配部分（入口见
[runner.py](runner.py)），继续探索 `@ac.system` 写法；本轮不预先增加通用插件或框架接口。
本流程参考 [RISC-V 架构测试](https://github.com/riscv/riscv-arch-test)的 ELF、自检和平台适配惯例，当前尚未接入其官方测试框架或认证流程。

历史 `acceptance`、`legacy`、`special` 和 `suite` 命令现在转入同一运行器；新脚本以本页入口为准。保存轨迹回归和历史报告生成仍保留独立工具。

编译器字段分析的额外验收使用 `benchmarks.verify_fields`，复用同一个 `run_case(check='all')`，
运行默认模型的全部 79 项，再运行 `control.field` 参数接口副本的 10 项。各 case 在独立进程中
编译，避免序列化命名状态被并发线程干扰。`benchmarks.split_control` 对照四种接线及 IM/IMC
启用配置，检查代码规模与 64 拍轨迹；该循环只验证有限窗口，不作为退出验收或动态性能测量。
命令、结果和限制见[字段分析修复报告](../results/field-analysis-2026-10-15.md)。
