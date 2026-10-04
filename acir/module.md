# ACPy → ACIR → GFSim 设计草稿

以新项目设计和当前 [GFSim C++ 框架](../gfsim/cpp/README.md) 为依据，重新构造 pyCircuit，不在旧 pyCircuit 上修改或继承其特化编译路线。本文记录讨论约定；最小实现、具体语法与已验证范围见 [独立编译器](../pycircuit/README.md)。调度语义见 [GFSim spec](../gfsim/spec.md)，未决底层语义见 [open-questions](../gfsim/open-questions.md)。

## 1. 核心对象

| 前端对象 | 职责与后端映射 |
| --- | --- |
| `@ac.module` | 用 `def` 定义资源、连接和控制逻辑；生成 class、构造代码和 `Work()` |
| `@ac.rule` | 定义原子效果及控制流；生成 Module 内的 Rule 成员函数 |
| `@ac.signal` | 纯 helper 从 Queue current 计算共享组合值；映射到 `Signal<T>` |
| `ac.queue` | 持有持久状态；映射到 `Queue<T>`，支持 Queue array |
| `ac.var` | 表达普通组合值，映射为局部值或 Rule 参数 |

首版值类型沿用当前 C++ 框架：bool、标准定宽整数、固定数组和嵌套 struct。普通纯 helper 用于复用组合计算。

## 2. Module 的构造与 Work

- Module 的输入用函数入参表达，输出用 `return` 表达；端口及连线在构造时固定。
- 资源和实例声明只依赖构造参数，创建和初始化一次。运行时分支只影响计算、Rule 选择和效果。
- 子 Module 调用先只表示静态实例化与连接；构造后的各 Module 由调度器独立激活。
- Module Work 保留控制流，直接选择调用 Rule；Module 本身不是原子事务。
- Queue array 的每项有独立 QueueId；动态下标定位实际 Queue，读取和 proposal 都登记到该项。

## 3. Rule 身份与参数

- Rule 可以定义在 Module 内部或外部。同一个 Module 实例内，同名 Rule 只生成一个成员函数和一个 RuleId；不同调用位置共享记录，不同 Module 实例的记录独立。
- 资源参数和普通参数可以在 `def` 中显式声明，也可以根据调用实参静态推断；认为调用格式正确。两者统一为同一种参数描述，使用相同编译路径。
- 资源参数保留资源身份，Rule 执行到读取处才读取；Module 调用时不预读消息输入 payload。
- 普通参数按值传入并参与候选缓存比较。若调用时资源实参发生变化，其资源身份也参与比较，避免复用旧绑定的候选。
- 捕获的资源、固定配置和本次 Work 的组合值分别按资源绑定、构造配置和普通参数处理。同一 RuleId 同 tick 重复调用仍遵守 GFSim 的参数一致约束。

## 4. Queue 访问与效果

| 前端行为 | 生成操作 |
| --- | --- |
| 实际读取 Rule 的消息输入 payload | 读取并登记依赖，同时提出一次 pop |
| `return value` | 向固定绑定的输出 Queue 提出 push |
| `return None` | 正常完成，不为该位置提出 push |
| 在 Rule 内直接观察内部 Queue 或只读引用 | 读取并登记依赖，不消费 |
| 对 Queue 或其字段赋值 | 提出 revise，修改已有旧队尾 |
| 普通局部赋值 | 即时组合计算 |

前端不强制区分 FIFO 与寄存器两种 Queue 类别。寄存器是容量为 1、构造时给定初值、后续通过 revise 更新的 Queue 用法。是否生成 pop/push/revise 由访问角色和操作确定。

读取保留实际分支，未选路径不预读。Rule 的效果由 GFSim 整体仲裁并在 Xfer 提交；必要读取失败时 abort，正常路径 complete。调用返回不表示效果已经生效。

Signal 显式输入只绑定 Queue，固定配置从构造期捕获；由 runtime 初始化，并在全部 Queue Xfer 后按全部声明输入更新缓存，输出变化后才按固定关系通知 Module／Rule。Module 和 Rule 均可读取 Signal。

## 5. 统一编译路径

```text
ACPy AST → ACIR MLIR → EmitC → C++ 成员函数与静态绑定表 → GFSim
```

ACIR 的 func/cf 保留 Module、Rule 控制流和运行时循环；注册的 acir 操作保留类型、资源身份、实际路径读取和效果，通用 pass 展开正常完成条件及原子边界。不同 Rule 都按这些通用操作生成，不按输入／输出数量或状态组合增加专用编译路线。

编译器生成 Module 可访问资源、Signal 全部输入、Rule 静态 Signal 依赖及可能修改的 Queue 和操作等静态声明。实际读取、dirty、唤醒、候选复用、容量 DFS 和 Xfer 由 GFSim 管理。

首版以用户代码满足约束为前提，只做生成所需的解析、类型处理和绑定，不增加安全性证明、冲突检测或自动纠错。每个 Queue 的 pop／push 来源分别至多一个 Rule，覆盖所有分支和 tick，由用户保证。

Ripes5、既有 OoO 和完整 Queue 版 skyzh CPU 均通过此路径编译；构建、接口和验收见编译器说明。
