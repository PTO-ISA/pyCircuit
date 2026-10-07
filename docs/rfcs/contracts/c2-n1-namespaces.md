# C2-N1：既有声明的 named import 与 re-export

修订：C。日期：2026-09-28。状态：待独立审阅与用户批准。设计建议由 interface_design（Astra xhigh）给出，PM 整理；本文不授权实施。

## 范围与原合同关系

本增补仅补齐 C2-C 五类声明在逐源 named import/re-export 中的名称发布与消费快照：record、类型别名、常量、纯值 helper、硬件 module 类。它区分某源发布的本地名字与其指向的 canonical declaration。

[C1-C](c1-pythonic-source.md)、[C2-C](c2-mlir-contract.md)、[C3-C](c3-driver-runtime.md)及其批准原文保持冻结。本增补不改变数学、record、helper、state、rule、特化、发布、runtime 或后端合同；只增加本文精确定义的两个 MLIR 属性及相应验证义务。

不新增 operation、dialect type、CLI 参数、receipt 字段、runtime ABI 或第二条编译路线。直接 Packet/Accumulator/Core 导入的既有获准工作继续进行；新属性与重导出实施须待本增补批准。

本文不规定星号导入、`__all__`、namespace value 转发或 intrinsic re-export。它们保持未分类或待后续合同处理，不因本增补被永久退役。本地既有 intrinsic/namespace 处理保持原有边界，不纳入这两张表。

## 缺口与预期结果

```python
# facade.py
from .packet import Request as R

# parent.py
from .facade import R
```

Parent 自己的 AST 只提供 facade 和 R。canonical packet.Request snapshot 不能说明 Facade 是否将它发布为 R，也不能与 Facade 把 R 改指向另一声明的版本区分。

增补后，Facade 的 owning interface 发布 `(Facade SourceOwner, "R") → @"demo.packet.Request"`。Parent 保存实际消费的这项绑定。即使 Packet 全部声明没有改变，Facade 改变 R 的目标也会使旧 Parent 在 link 失败。

当前源的 `from .packet import Request as R` 可直接由本源 AST 建立本地名字，不创建新 record、类型别名声明或 declaration owner；N1 将该名字跨源边界的发布与消费显式记录。

## 精确 MLIR 属性与闭合记录

新增两个 builtin module 属性：

| 属性 | 精确类型 |
| --- | --- |
| `ac.exports` | ArrayAttr，元素为 ExportBinding DictionaryAttr |
| `ac.import_bindings` | ArrayAttr，元素为 ImportBindingUse DictionaryAttr |

本增补切换后，唯一工具链生成和接纳的 source-stage interface、implementation、declarations 单元均必须具有两个属性。没有项目时写空数组；不以缺失属性表示另一种 schema，不提供可选择的 N1 模式或旧 schema fallback。

```text
NamespaceSite = {
  ast_path:Array<PathComponent>,
  location:SourceSpan
}
ExportBinding = {
  name:StringAttr,
  target:FlatSymbolRefAttr,
  site:NamespaceSite
}
ImportBindingUse = {
  source:SourceOwner,
  name:StringAttr,
  target:FlatSymbolRefAttr,
  site:NamespaceSite
}
```

SourceOwner、PathComponent、SourceSpan 沿用 C2。所有 DictionaryAttr 均为闭合记录；未知字段、缺失字段、错误类型拒绝。

target 只能指向 canonical declaration，不指向另一个 export-table entry。不得将 `@"demo.facade.R"` 当成待递归解释的别名路径，除非它本身确实是具有唯一 authority 的既有声明。

这些记录不是 MLIR symbol declaration，没有 sym_name、ac.declaration_role 或新模型 identity，也不形成第二个 executable graph。

## 五类目标及其身份

| 目标类别 | canonical authority |
| --- | --- |
| record | owning interface 中的 ac.struct |
| 类型别名 | owning interface 中的 ac.type_alias |
| 常量 | owning interface 中的 ac.constant |
| 纯值 helper | ac.helper_kind="value" 的 owning func.func |
| 硬件 module 类 | owning interface 中的 ac.module.import |

目标类别从已验证声明取得，不在 N1 记录中重复编码。透明类型别名保留 alias symbol，常量保留 constant symbol，helper 保留原 helper symbol。不能因展开后类型、常量值或 helper 签名相同而合并绑定身份。

record constructor helper 经 ac.struct.constructor 到达，不自动成为新的顶层源 export。header 出现某个 helper 或 record snapshot，不自动说明存在同名 Python export。

Facade、alias 和 consumer 不改变目标的 owner、origin、nominal identity、签名、默认值、helper body 或 effects。真正的源类型别名/常量声明如何形成仍由 C1/C2 决定，N1 不把 import 合成为新声明。

## 来源、名字与规范顺序

name 是 Python AST 中规范化后的单个 identifier，区分大小写。不得以 C++ mangling、位置、文件顺序或内容派生值代替源名字。

NamespaceSite.ast_path 从包含记录的 source Module AST 根开始，不借用或伪造 class/function definition。location.path 必须等于该单元 ac.source_owner.path；行列与 end-exclusive 沿用 C2。

export site 指建立最终本地绑定的位置；import-use site 指本源消费提供者名字的 import alias 项。既有合法 qualified-name 处理若消费同一 header 成员，可记录实际查找位置，N1 不据此扩张 namespace 源语法。

ac.exports 按 name 的 UTF-8 字节序排序，name 唯一。ac.import_bindings 按 source 的结构顺序、name、site 的结构顺序排序，target 为最后比较项；完全重复项拒绝。

同一 source/name 可出现在不同 site，但一次 producer 的所有这些项必须具有相同 target，因为它使用同一输入 header snapshot。多个本地名字可共享 target，不构成重复 authority。

## 导出成员与必要绑定规则

ac.exports 包含完成既有合法顶层绑定处理后，当前绑定属于五类目标的全部名字，包括本源声明和普通 named import 得到的名字。是否被本源计算使用不影响导出；不能仅因 unused 删除导入名字。

前导下划线不限制显式 named import；_Request、_helper 可以明确导入。N1 不引入 privacy marker、__all__ 或同名 as 重写要求。

named-import 绑定事件使用 as 指定的本地名字，否则使用被导入名字。重复 named import 合法，后一次对同一本地名字的绑定覆盖前一次；export 表保存最终绑定，所有实际 import 查找仍保留消费快照。

此覆盖规则只处理名称环境，不是持久 state 的重复 next 写入规则。它不重新定义一般赋值、声明注解、default、函数体全局解析或局部作用域的 C1 语义，也不引入重复 nominal definition 的新准入。

若既有合法声明或赋值随后改变本地名字，export 表反映既有语义处理后的结果。N1 不通过后缀 symbol 解决原合同不允许的重复 declaration authority。

表中缺少某名字，只表示它没有作为这五类声明发布；不能据此宣称该名字在任意 Python namespace 中不存在，或永久拒绝尚未分类的 import 能力。

## Producer 与本地 alias

当前源的 named import 依次执行：

1. 依 C2/C3 source-module 规则确定唯一提供者 SourceOwner。
2. 从提供者已验证 owning header 查找 ac.exports[name]。
3. 将 canonical target 绑定到本源本地名字。
4. 记录提供者、被查找名字、target 和本源 site。
5. 若最终仍是模块级五类声明绑定，将本地名字加入本源 exports。

必须查 export 表，不能拼接 provider-module 与 name 绕过它。直接导入使用同一规则，自有声明 export 通常恰好指向该拼接的 canonical symbol。

未使用或随后被遮蔽的 import 仍须解析、记录；否则优化可能掩盖缺失或错误导入。消费项在名称解析时生成，不从 surviving IR references 反推。

本地 alias 不改变 target；普通本地使用不反复制造远端 import 事件，也不要求每个本地读取复制消费项。

## Facade 与 Parent 示例

以下 P/F/U 为 Packet/Facade/Parent 的完整 SourceOwner，SF/SU 为各自 import alias 项的完整 NamespaceSite。

```text
facade.interface.ac 与 facade.ac:
  ac.source_owner = F
  ac.exports = [
    {name="R", target=@"demo.packet.Request", site=SF}
  ]
  ac.import_bindings = [
    {source=P, name="Request", target=@"demo.packet.Request", site=SF}
  ]
```

```python
# parent.py
from .facade import R as T
```

```text
parent.interface.ac 与 parent.ac:
  ac.source_owner = U
  ac.exports = [
    {name="T", target=@"demo.packet.Request", site=SU}
  ]
  ac.import_bindings = [
    {source=F, name="R", target=@"demo.packet.Request", site=SU}
  ]
```

Parent 保存实际消费的 F/R，不能改写成 P/Request 而丢失 Facade 改绑检查。必要 declaration snapshots 仍保留 Packet 原 owner/origin；NamespaceSite 不替换 declaration provenance。

## Header/body 镜像与独立权威

同一 producer 的 header/body 两个 N1 属性必须完全一致，包括 sites。header 是 export-map 唯一 authority，body 镜像不构成第二份 authority。

import_bindings 在两份文件中都是该 source 自己的消费快照。Parent 不复制 Facade 消费项冒充自己；Facade 保留自己查询 Packet 或上一级 Facade 的记录。

N1 不要求仅为新属性复制全部目标声明。必要 declaration snapshots 按 C2 产生和验证，canonical target 经 registry 解析。

名称一致性与 declaration 一致性独立：映射相同不证明 layout/default/helper/effects 相同，declaration snapshots 相同也不证明 Facade 名称映射相同。

## 依赖闭包与循环边界

compile 必须显式取得实际 provider、所验证 facade 链及 canonical declaration owner 的 headers。仅有 snapshot 不建立 authority；读取仍限 C3 receipts/headers，不包括依赖 Python/body。

ac.interfaces 包含本 owner 与实际读取的 header 依赖闭包，按 C2 排序去重；depfile 记录实际接口及 receipts。依赖 body 只在 link 按 C3 验证。

export target 已是 canonical declaration，不需要递归追踪 export-name 链。伪造 A.R→B.R、B.R→A.R 而没有真实 declaration authority，按缺失 authority 拒绝，不能互相认证。

跨 source 消费快照图含环，不单独构成 N1 schema 错误。完整 headers 均可用、targets 均有真实 authority 时，每个消费项可有限地独立比较。验证器用 visited/worklist 防止重复遍历，不把访问过当成验证成功。

Producer 要求不同于结构校验：每次独立编译需可接受输入 headers。从零构建的 producer 需求有环且所需 headers 均缺失时，没有可执行首个 producer；诊断列明缺失输入和需求环。

N1 不提供破环机制，不授权半初始化 header、隐式预声明批处理、未验证旧快照或依赖源码读取，也不对无关依赖、合法既有 header 集合、类型/evaluator/实例图增加统一无环限制；后几类循环仍按各自 C2 合同处理。

## Link 校验与删除时点

link 在 declaration 去重和 specialization 前完成：

1. 按 C3 检查完整 unit 闭包，比较每源 N1 header/body 镜像。
2. 建立唯一 SourceOwner→header 索引和独立 canonical declaration authority 索引。
3. 验证每个 export target 属于五类声明且有正确、唯一 authority。
4. 每项消费查询 provider.exports[name]，要求存在且 target 与快照结构相等。
5. 独立执行 C2 declaration/helper body/signature/effects snapshot 比较。
6. 全部成功后再去重、特化、物理化及关闭其余安全义务。

不能以目标相同绕过 source/name 查找，不能 mismatch 后自动重绑旧 body。

跨版本消费比较不比较 provider 源码位置，单纯移动声明/import 不造成 stale binding。同源 header/body 镜像仍精确比较自身 sites，拒绝混合 producer 输出。

完成 namespace 检查后，在丢弃 source-unit 包装边界前统一删除两个属性。共同 final IR 禁止残留，实际类型、helper 调用、module 引用须已使用既有 canonical identity。

Facade 不改变 SpecKey.definition、ordered arguments、StateID、OwnerRef、reset image 或生成 source group。声明-only Facade 不产生 executable module、实例或额外实现 TU。

## 诊断与信任边界

诊断区分：缺 provider、未发布 named declaration、歧义 authority、非法 target、stale binding、header/body 不一致、无法启动的 producer 需求环。stale binding 报告消费源 site、provider owner/name、期望 target 与实际 target；缺失 export 明确报告缺失。

不得降为 warning、继续生成 final IR 或回退到源码搜索。N1 是编译器/链接器一致性合同，不是源码认证协议；不声称发现同时一致重写源语义、两个属性、全部 declaration snapshots 与 body 的改写。

## 独立正例

| 编号 | 必须证明的行为 |
| --- | --- |
| P01 | 五类声明分别直接导入、使用 as、经 Facade named re-export |
| P02 | 双本地名字指向同一 target，authority 唯一 |
| P03 | _Request/_helper 可显式 named import |
| P04 | unused named import 仍发布最终名字并保留消费项 |
| P05 | 重复 import 与后续遮蔽；早期消费项不删除 |
| P06 | Packet→Facade1→Facade2→Parent 保留实际 provider |
| P07 | record re-export 的 defaults/kwargs 仍执行原 constructor |
| P08 | alias/constant/helper 的 canonical identity 不按展开等价合并 |
| P09 | 每源独立编译；依赖 Python/body 不可访问 |
| P10 | roundtrip/header 输入顺序置换保留规范元数据 |
| P11 | 完整、一致的循环消费快照图不被统一无环检查拒绝 |
| P12 | 移动 provider site 不导致旧 consumer binding mismatch |

P11 是 header/link 验证，不证明缺输入 header 的循环工程可从零构建。

## 独立负例

| 编号 | 必须拒绝的情况 |
| --- | --- |
| N01 | R/S 交换 targets，而 canonical declarations 不变 |
| N02 | R→A 改成 R→B 后 link 旧 Parent，即使 A/B 同 layout |
| N03 | provider 删除/改名已消费名字，原目标仍存在 |
| N04 | 缺 owning header，只有相同 declaration snapshots |
| N05 | 映射不变但 layout/default/helper/effects snapshot 被篡改 |
| N06 | declaration snapshots 不变，namespace 与消费快照不一致 |
| N07 | 只改 header 或 body 的任一 N1 属性 |
| N08 | duplicate authority、悬空/错误类别 target、伪造 Facade declaration owner |
| N09 | 无真实 authority 的互相引用 |
| N10 | 缺 header 阻止 producer 启动，不得读依赖源码破环 |
| N11 | constructor helper 仅因出现在 header 就被自动顶层 export |
| N12 | 缺任一顶层属性、错误外层类型、非 dictionary 元素、嵌套字段缺失/多余/错型，以及重复 export name/use、不规范顺序、非法 site |
| N13 | 同一 producer 对同一 provider/name 记录不同 target |
| N14 | 向原本合法 final IR 分别注入任一 N1 属性，经 cpp/verilog emit 均拒绝且不发布输出 |
| N15 | 无效 provider/name 的 import 即使 unused，或随后被合法 import 遮蔽，也必须拒绝 |

## 计划验收门槛与命令

本节目标、测试文件和用例均为**计划交付项，尚未实现或执行**，不是当前工具已经通过的证据。实施期间允许先验收已就绪的内部子集；N1 完整验收必须执行下列全部 lane，未就绪不能报通过。

| lane | 计划测试载体 | 对应义务与独立观察 |
| --- | --- | --- |
| structural | ACIRNamespaceContractsTests；NamespaceContractsTest.cpp | P02/P08/P10/P11，N04/N08/N09/N11/N12/N13；直接构造正反 IR，闭合字段、目标、规范顺序与 authority 不依赖 producer 自证 |
| source/header | tests/system/test_source_namespace_bindings.py 中 source/header cases | P01–P10，N03/N04/N08/N10/N11/N15；真实 Python 逐源编译、实际 header-only 访问审计、独立预期绑定表；五类目标均有实例 |
| link | 同一 system 文件中的 link cases | P06/P08/P11/P12，N01–N10/N13；独立替换 facade/provider 后链接旧 consumer，诊断类别与失败发布行为可观察 |
| final/emit | 同一 system 文件中的 final/emit cases | 正例证明两个属性均被删除；N14 从已验证合法 final IR 分别注入各属性，通过两个真实 emit 入口逐项失败 |

P10 的 oracle 固定 expected bindings，不能只比较两次 producer 输出。至少用不同 target 的 z、é、α 名字验证 UTF-8 顺序；用同一 provider/name 下 AST 数字索引 2、10 验证按数值而非打印字符串排序；置换显式 header 输入后仍与该独立表相同。按继承的 C2 数值合同接纳合法 IntegerAttr 表示，不额外固定所有 u64 字段的宿主位宽为 i64。

N12 分别测试 ac.exports/ac.import_bindings 缺失、外层错型、元素错型，以及每种嵌套记录的缺失/未知/错型字段。N15 各有 unused 和 later-shadowed 两种真实源码；即便最终本地名字可绑定有效目标，之前的失败查找仍导致编译失败。

N14 对两种属性、两个目标形成四个独立拒绝用例。每个例子先证明未篡改输入可通过 final verifier；再调用实际 C3 emit 入口，检查非零退出、对应残留属性诊断及没有新发布的 generated tree。已有稳定输出的 replace 变体保持原输出不变；不得用 mock emitter、旧路线或仅扫描文本替代。正向 lowering 同时断言无残留且两条 emit 可消费同一 final IR。

实施时以下路径变量必须指向本 checkout 构建的候选工具，运行前记录解析后的绝对路径、源码候选和 LLVM/MLIR 22.1.8。PYC_N1_BUILD 为该候选 CMake build 目录；PYC_N1_DRIVER 为按 C3 构建的 pycircuit driver；禁止从其他 checkout 复制工具或隐式采用系统旧 driver。

```bash
: "${PYC_N1_BUILD:?set the current-checkout build directory}"
: "${PYC_N1_DRIVER:?set the current-checkout C3 driver path}"
cmake --build "$PYC_N1_BUILD" --target ACIRNamespaceContractsTests -j 6
ctest --test-dir "$PYC_N1_BUILD" -N -R '^ACIRNamespaceContractsTests$'
ctest --test-dir "$PYC_N1_BUILD" -R '^ACIRNamespaceContractsTests$' --no-tests=error --output-on-failure
PYCIRCUIT_TEST_DRIVER="$PYC_N1_DRIVER" pytest tests/system/test_source_namespace_bindings.py --collect-only -q
PYCIRCUIT_TEST_DRIVER="$PYC_N1_DRIVER" pytest tests/system/test_source_namespace_bindings.py -m system -q --junitxml="$PYC_N1_BUILD/namespace-system.xml"
```

system gate 中的三个产品子命令必须使用 C3 已批准形式：

```text
pycircuit compile -c <one-source.py> --source-root <root>
  --package-prefix <prefix> [-I <published-unit>]... -o <unit-dir>
pycircuit link <unit-dir>... --top <qualified-module> -o <program.ac>
pycircuit emit <program.ac> --target cpp|verilog -o <generated-dir>
```

collect/CTest discovery 和执行记录均必须非空；native executable 内也必须实际运行断言，不能以只有 CTest wrapper 存在代替。证据索引将 P01–P12/N01–N15 每行绑定到已运行 test ID，注明五种声明类别及两个 emit 目标的参数化实例。缺行、零断言、失败、skip 或 xfail 均不关闭对应义务。预期拒绝由测试断言非零结果并验证诊断/产物，使测试自身以 0 退出。

最终 evidence 保存命令、退出码、测试发现与执行计数、候选内容绑定、规范表 oracle、consumer/provider 版本组合、访问审计和产物路径。中间原生 harness 可以提供子集证据，不能替代这里的 C3 driver、link 与双 emit 完整门槛。

## 实施与批准边界

先加入记录/source-unit verifier，再接 producer、header resolver、linker。独立测试从本文表项生成，不以实现输出为唯一 oracle；AST capture 或 direct Packet 通过不替代完整验收。

按[批准边界](approvals/c2-c3-foundation.md)单独绑定精确文本、独立审阅结论和用户批准；作者不审阅自己的提案。实施和独立测试由不同实例承担。失败时回退 N1 实现，不恢复旧路线或改写冻结合同。C3 HeaderView/full-unit、锁、发布及恢复合同继续适用。
