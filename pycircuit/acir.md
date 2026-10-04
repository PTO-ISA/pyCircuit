# ACIR JSON v1

文件保存 `version`、`top`、`types`、常量、`config`、`resources`、`modules`、纯 `functions`、`construction` 及资源导出名。它是类型化的数据流表示，不含 Python AST、源码函数体文本或交给后端解释的 Python 表达式。

构造参数和资源声明保存为显式类型；Queue 保存容量及初值操作的 SSA 引用，Queue 阵列保存 iterable 和初始化函数，Signal 保存 helper 函数及全部可能 Queue 输入。Module 保存 Work、Rule 定义和资源访问表。每条 Rule 保存参数类型、全部可能资源身份、固定输出、操作绑定及函数体。

函数的 `ops` 是线性序列。每项包含：

```json
{
  "op": "queue.read",
  "id": "v5",
  "type": "u32",
  "args": ["v2"],
  "guard": "v4",
  "loc": {"file": "example.py", "line": 12, "column": 8}
}
```

`guard: null` 表示无条件；否则只在该 bool SSA 值为真时执行。值只在有效路径使用，未选路径不读取资源。临时值在 C++ 中默认初始化，使组合谓词可以安全表达非活动域，但这不会执行非活动资源读取。`select` 合并 SSA 值；Rule 中必要 `queue.read` 失败时必须登记实际依赖并中止整条候选。C++ 后端以 `tryPeek()` 加 `abortRule()`／返回表达，不以异常处理正常缺输入。Module 控制读取失败只停止后续选择；Signal 无保护的必要读取失败仍是错误。`complete` 表示正常返回，包括无输出和提前返回路径。

| 操作 | 内容 |
| --- | --- |
| `const / zero / param / config / resource` | 字面量、值初始化、函数参数、构造配置、固定资源身份 |
| `binary / unary / cast` | 显式类型的算术、比较、布尔和转换 |
| `aggregate / field / update / array.update` | 值构造、字段访问、按值更新 |
| `index / length / range / select` | 数组与构造序列、SSA 合并 |
| `call / return / check` | 普通 helper 调用、值返回、断言 |
| `queue.read / empty / full / size` | 当前 Queue 的实际读取 |
| `queue.pop / push / revise` | 受 guard 保护的原子效果；revise 保存静态字段路径 |
| `signal.read` | runtime Signal 结果读取 |
| `rule.call / event / complete` | Module 选择 Rule、未来唤醒请求、正常完成 |

后端为同一 Rule 的所有 `queue.pop` 使用有界局部指针表去重，覆盖实参别名。该表在每次候选重算时重新建立。静态绑定和实际读取分开：Module 访问表／Signal 输入表／Rule 操作表覆盖所有分支及资源阵列元素，Queue 的动态订阅由 GFSim 登记实际执行路径；Signal 两侧关系均静态建立。后端从现有参数 `targets`（所有调用位置的并集）和资源值 SSA 引用追踪到 `signal.read`，生成 `declareInput(ruleId, signal)`，guard 不缩小绑定。普通值不传播资源身份，Work 读取后传普通参数仍由参数比较决定 Rule 重算。独立 ACIR 重载走同一生成路径，无需增加格式字段。

`queue.pop`／`queue.revise` 本身也要求目标非空，即使前面没有 `queue.read`。后端在相应 guard 内检查；只有同一 Queue 已在无条件路径或相同 guard 下通过检查时才省略。`queue.push` 不增加空满检查，输出容量始终交由 GFSim 仲裁。完整代码生成契约见 [GFSim 生成范式](README.md#gfsim-生成范式必要输入)。

所有函数，包括子 Module 的函数，使用相同操作集和后端。`emit` 不需要前端的名称表、HIR 或任何输入源码文件。源码位置为诊断信息，不参与代码重解释。
