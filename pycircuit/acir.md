# ACIR MLIR v2

保存入口是 `model.acir.mlir`。它是注册 dialect 的可验证 MLIR，不包含 JSON、Python AST 或需要重新解释的源码。`module` 的 `acir.model` 字典记录 `format_version = 2`、顶层构造参数、结构体字段、声明顺序、Module/Rule 身份和导出连接。初始化表达式也编译成 `func.func`，因此重载不依赖源文件。

## 类型与函数

整数使用 MLIR `i1/i8/i16/i32/i64`。ACPy 有符号性保存在 `acir.source_type`、函数参数元数据和块参数 NameLoc；比较谓词本身区分 signed/unsigned。canonicalize 丢弃普通整数属性时，转换通过位宽、比较谓词、接口签名和显式转换保持语义。

ODS 类型为 `!acir.queue<T>`、`!acir.signal<T>`、`!acir.qarray<T>`、`!acir.struct<"Name">`、`!acir.array<T, N>` 和构造配置用 `!acir.vector<T>`。结构体的字段类型来自模型字典。操作类型约束和补充 verifier 见 [ACIR.td](mlir/ACIR.td)、[Dialect.cpp](mlir/Dialect.cpp)。

Module Work、Rule、Signal、helper 和初始化函数统一是 `func.func`。`acir.function` 属性记录角色、所属实例、普通参数和结果类型。普通值是 SSA，分支和提前返回使用 `cf.br/cf.cond_br/func.return`，运行时循环是带参数的 CFG 回边，不展开 ROB/RS 扫描。

## 操作

| 操作 | 含义 |
| --- | --- |
| `acir.resource` / `acir.get` | 静态资源声明／获取身份 |
| `acir.read` / `acir.query` | 读取 payload/Signal，查询 empty/full/size |
| `acir.pop/push/revise` | 当前路径的 proposal；revise 的 path/indices 捕获旧尾更新位置 |
| `acir.invoke` / `acir.event` | Module 选择 Rule／事务内请求未来激活 |
| `acir.aggregate/extract/update/length` | 聚合值构造、读取、按值更新、长度 |
| `acir.config/range` | 固定配置与初始化序列 |
| `acir.cast/unary/compare/check/unreachable` | 定宽转换、前端值操作、断言／非法路径适配 |

普通整数计算复用 `arith`，helper 调用复用 `func.call`。越宽移位、带符号整除和溢出边界使用带 intrinsic 属性的 helper 声明，避免错误赋予 MLIR poison 语义。转换也接受优化产生的 `arith.select` 与整数扩展／截断。

`read/query` 本轮保留保守效果，不标记 Pure；缺输入检查和读取必须留在实际分支内。运行时不再登记依赖。ODS 使用标准 `MemoryEffectOpInterface` 的读写效果。聚合索引没有状态效果，但可能报告越界，因此不可推测执行。源码 `loc` 保留到 ACIR；最终 EmitC 工件保留函数位置。

## 三组 pass

1. `acir-analyze-resources`：沿参数、别名、CFG 边和动态索引传播可能资源集合，生成 `acir.accesses/effects`。Signal 输入包含全部绑定端口与捕获依赖，允许 Queue 或 Signal；检查 Signal DAG 无环，保存的 MLIR 重载也执行此检查。常量下标和固定子列表保留元素身份；动态索引静态声明覆盖该输入视图的全部可能元素，运行时只操作实际元素。生成端使用轻量资源视图，临时列表共享持有指针表，避免在 Work/Rule 传参时复制整张资源表。
2. `acir-lower-gfsim`：Rule 入口生成 begin，正常返回 complete，必要读取在原位置分裂 CFG，空时 abort；Module 读空仅返回，Signal 读空保持错误语义，静态依赖包含全部绑定输入。pop 在适配层按实际资源去重。
3. `acir-convert-to-emitc`：将类型与行为转换为 EmitC 调用、标准函数和分支。类声明、资源构造、注册表和 proposal 绑定来自同一份静态信息。MLIR `translateToCpp` 输出函数体。

这些 pass 不识别 CPU、ROB、RS 或其他组件名称。`acir-compile` 默认在资源分析之后执行 canonicalize/CSE，`--no-opt` 关闭它们。`acir-opt` 可分别执行上述 pass；完整模型编译命令会一并输出头文件及构造注册代码。

## 示例片段

以下为合法的 dialect 函数片段（完整 emit 还需模型、角色和资源元数据）：

```mlir
func.func @transfer(%input: !acir.queue<i32>, %output: !acir.queue<i32>, %enable: i1) {
  cf.cond_br %enable, ^selected, ^done
^selected:
  %value = "acir.read"(%input) : (!acir.queue<i32>) -> i32
  "acir.pop"(%input) : (!acir.queue<i32>) -> ()
  "acir.push"(%output, %value) : (!acir.queue<i32>, i32) -> ()
  cf.br ^done
^done:
  return
}
```

未选择路径不会预读 input，也不会产生 pop/push。正常 None 返回只缺少 push；必要读取失败则由生命周期 pass 撤销整条候选。
