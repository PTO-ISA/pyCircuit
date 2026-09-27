# C2 接口事实与待冻结缺口

状态：分析完成，精确提案待编写；不是 C2 批准。来源为独立 `gfsim_inventory` 的只读代码核对，donor 基线 `b852ed83fa0288d0be7406bba0ed47be4b2c0f63`。本页把已有语义与真正需要新增的接口分开，避免重新发明基本状态操作。

## 可直接采用设计的现有 donor 形状

| 对象 | 实际定义 | 原文件 |
| --- | --- | --- |
| nominal record | `ac.struct` 的 symbol 与 ordered field 字典；`!ac.struct<"Name">` 从声明取得布局 | `compiler/acir/include/gfsim/ACIR/ACIROps.td:10`、`ACIRTypes.td:17` |
| record 值 | pure `ac.struct.create/get/with`；构造/字段读取/不可变替换 | `ACIROps.td:16` |
| owned state | `ac.dff`/`ac.dffe` 返回 typed handle，`ac.initial_value` 承载初值 | `ACIROps.td:48` |
| module | isolated symbol/body、input/output 分段、`ac.input_count`；端口接 Queue/DFFE，不接 DFF | `ACIROps.td:192` |
| instance | `name`、flat symbol `callee`、typed input/output handles、prepared next results | `ACIROps.td:207` |
| rule | `name`、`input_modes`、分段 inputs/outputs；readiness/conditions/body/admission 四个 region | `ACIROps.td:216` |
| current/next | input mode `q` 将 payload current 作为 region 参数；DFF yield data，DFFE yield data+enable | `compiler/acir/test/valid/state_ports.mlir:5` |
| static parameter | `ac.param` 有名字及 integer/index result；link 用 param_names/values 绑定 | `ACIROps.td:62`、`tools/acir-link.cpp:392` |

这些是物理 IR 的真实接口，不自动等于已实现完整 C1 数学整数、range/check 或 source-unit 接口。C1 当前 source 提案仍等待用户批准。

## 真正需要 C2 冻结的接口

- **独立 module import/header**：donor 没有对应 module declaration-summary op。本仓旧 `ac.module.import` 存在，但载体绑定旧 finite-family schema。需要精确列出 source owner、qualified symbol、constructor static/connection 参数、nominal types/ranges、R/W effects、shape、child imports，以及 body/header 的 link 检查，不能原样复制两者拼成第三套。
- **逐源验证阶段**：donor topology/final verifier 要求 linked IR 和单个 portless root；不能为每个 library unit 构造假 top 来宣称逐源编译。需要 unit verifier 与 linked/final verifier 分工，parent 仅凭已发布接口编译。
- **qualified symbols 与输出所有权**：donor approval 明确排除 package naming，当前 codegen按 linked definition 输出且有共享类型头。新方案必须定义可读 qualified definition identity、source-group map、source-owned types/header，不能丢掉 Python source ownership。
- **最终硬件语义出口**：继承 donor prepare/current-next/driver checks，补齐 C1 evaluation-path 和 integer-format。现有 final verifier 仍拒绝这些未完成能力，不能只删除 guard 让代码生成继续。
- **RTL 输入**：普通 ACIR operation 名字与旧 pyCircuit 同名不证明 schema 兼容。需要 final ACIR → RTL-private PYC 的精确合法化规则；不得重新引入旧 QueueGraph 文本语义路线。
- **发布失败行为**：donor linker/translator 验证后直接写文件，尚无成组临时目录/原子发布合同；新 source-unit body/header/depfile 与 backend tree 发布规则需要 C2/C3 联合定义。

## 已核对的阶段顺序

syntax capture → `acir-import-python`（验证并移除 capture）→ link（选择单一 top/system、解析符号和参数）→ specialize → 局部 CSE/simplify → topology → Queue expansion → static loop unroll → Queue view → rule prepare → Queue next binding → next driver checks → final verify → codegen再次 final verify。

后端入口只能消费满足上述义务的产物。保留 donor 的分工，不等于保持每个工具的旧公开命令；单一 `pycircuit` 产品 driver 与 debug tools 的精确角色归 C3。

## 下一步与不被缩减的目标

PM 将这些缺口写成可批准的 C2/C3 接口表和正反例，独立 Astra 审阅后再提交用户。首先覆盖与 C1 scalar/record/root 匹配的完整 foundation；异构 static geometry、external typed DUT、memory/CDC/四态和 transaction/resource 扩展仍是完整项目必须完成的合同，不因 foundation 可编译而关闭。
