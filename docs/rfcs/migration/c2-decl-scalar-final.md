# C2-DECL：标量声明的 final 保留与源属头文件

修订：A。状态：提案，尚未获用户批准、尚未实施。
日期：2026-09-30。实现基线：产品分支 `a3df1ade`。
PM 编写；架构建议：other_agent_arch_review，Astra xhigh；代码事实：
declaration_ir_map，Astra low。独立设计审阅另行绑定本文件 SHA-256。

## 1. 目标与批准边界

已批准的 C2-C、C3-C、N1-C 已规定 owning-header authority、SourceOwner、
`ac.type_alias`、`ac.constant` 和按来源生成声明头。它们不需要重复批准。
本提案只冻结尚缺的 **final declaration-unit 接纳/投影与 C++ 标量声明映射**。

不新增 op、type、attribute 名称，不增加 Python DSL、source/API 参数、CLI
选项、runtime wrapper 或 IR 版本标记。保持 reg/proposal/Work/Xfer、实例树、
reset、checks 和两后端执行语义。SYSTEM/EXPECT 修订 B 不在本包；公开新
`emit` 仍归 M5。完整 generated.json 发布、DUT ABI、SDK/安装也不在本包。

本包支持的声明：LogicalType 为 bool/integer 的 alias；StaticType/StaticValue
为 bool/integer 的 constant。record/list alias、record/list constant、helper
或 constructor 的 final 投影不在本包。不能静默省略一个未支持的 canonical
声明后，把输出称作完整声明头；见 §2 的精确接纳边界。

## 2. 权威、选择和源单元清单

1. 先完成现有完整 source/header 配对、N1 消费/exports、一致性及 namespace
   检查。不得因为只需要一个 scalar 声明而跳过未使用的 header 内容校验。
2. 从每个显式 supplied unit 的 **owning interface** 选择本 owner、
   `ac.declaration_role="definition"` 的全部受支持 alias/constant。
   不依据 runtime 使用、DCE 或 `ac.exports` 筛选；private-name 定义也保留。
   这不新增 Python export，也不把私有源名字变成 Python 公共 API。
3. body 副本、外来 import_snapshot 和 facade re-export 不能建立新 authority。
   不能把 snapshot 改标为 definition。投影前由现有 registry 确认唯一权威。
4. 每个已接纳的 supplied SourceOwner 恰有一个 final unit。implementation
   仍必须属于现有可执行闭包；本提案不放开 unreachable implementation。
   纯声明、纯 facade 和空声明源都有 declarations unit，即使其声明数为零。
5. 同一个 implementation source 自有的 scalar 声明放进它已有的 final
   implementation unit，与 ac.module 同级；不产生第二个同 owner 的 unit。
6. 未支持的 alias/constant 类别不能按“未使用”静默丢弃：materialization
   必须以未支持声明投影拒绝。既有 record/helper 全 supplied-unit preflight
   继续生效，包括 owning header 中的未使用定义。注意：基线并未证明所有
   unused list alias 都已被拒绝；对此类原先可能被擦除的声明，本条是本提案
   明示的 bounded-profile 接纳收紧，不能声称零接纳变化。

输入清单取已经验证的 link unit 集，不扫描目录，不读取额外 Python/header，
不增加 sidecar 或伪造 implementation module。既有重复 owner、qualified
module、大小写歧义和 `foo.py`/`foo/__init__.py` 歧义拒绝规则保持。

## 3. 精确 final envelope 与声明字段

最外层 final package 的既有属性及 ac.system entry descriptor 完全不变。
每个 source-owned 内层 builtin module 的属性 **恰为**：

| 属性 | 值 |
| --- | --- |
| `ac.stage` | StringAttr `"final"` |
| `ac.unit_kind` | StringAttr `"implementation"` 或 `"declarations"` |
| `ac.source_owner` | 既有 SourceOwner |

implementation unit：零或多个本源 scalar declarations，随后恰一个既有
ac.module。declarations unit：仅零或多个本源 scalar declarations；禁止
module、reg、rule、instance、helper、state、yield 或额外 region/SSA 内容。
units 按既有 SourceOwner 结构比较严格排序；unit 内声明按 canonical
sym_name 的 UTF-8 字节排序且无重复。

声明复用现有无 operand/result/region 的 op，不增加字段：

| op | inherent properties | 普通属性，恰为 |
| --- | --- | --- |
| `ac.type_alias` | `sym_name:StringAttr`, `target:LogicalType` | `ac.source_owner`, `ac.origin:Occurrence`, `ac.declaration_role="definition"` |
| `ac.constant` | `sym_name:StringAttr`, `type:StaticType`, `value:StaticValue` | 同上 |

原 MLIR loc 原样保留，它不是额外字典字段。sym_name、source_owner、origin、
role 和 target/type/value 均来自 canonical definition，不从 import alias、
使用位置、首个实例、文件名猜测或重新合成。source_owner 必须等于 enclosing
unit owner；symbol 必须符合该 owner 的 canonical qualified-module 归属。

final 禁止 ac.interfaces、ac.exports、ac.import_bindings、ac.module.import、
import_snapshot、source helper bodies、StaticExpr 和 source-only carrier。
不能整份保留 interface，不能把三个 envelope 字段当成验证证书。

### 3.1 Before / after

源文件示例：

```python
from typing import Annotated
Word = Annotated[int, range(256)]
Limit = 7
```

owning interface 的 envelope 是 stage=source、unit_kind=interface，并携带
既有 ac.interfaces、N1 exports/import_bindings；两个声明的 role 是 definition。
link 先验证这些内容，再把同一声明投影到以下 **final package 内的单元片段**：

```mlir
module attributes {
  ac.stage = "final", ac.unit_kind = "declarations",
  ac.source_owner = {package = "demo", path = "types.py"}
} {
  "ac.constant"() <{
    sym_name = "demo.types.Limit",
    type = {kind = "integer"},
    value = {kind = "integer", value = #ac.math_int<7>}
  }> {
    ac.source_owner = {package = "demo", path = "types.py"},
    ac.origin = {site = {definition = @"demo.types.Limit", ast_path = []},
                 expansion = []},
    ac.declaration_role = "definition"
  } : () -> () loc("types.py":3:1)
  "ac.type_alias"() <{
    sym_name = "demo.types.Word",
    target = {kind = "integer", storage = i8,
              lower = #ac.math_int<0>, upper = #ac.math_int<256>,
              interpretation = "unsigned"}
  }> {
    ac.source_owner = {package = "demo", path = "types.py"},
    ac.origin = {site = {definition = @"demo.types.Word", ast_path = []},
                 expansion = []},
    ac.declaration_role = "definition"
  } : () -> () loc("types.py":2:1)
}
```

这里只展示新单元；完整 final 仍需原 entry、instance_bindings、ac.system
及可执行 implementation units。上述形状是待批准扩展，不能用当前编译器
对它的拒绝冒充实施失败或已通过的新 verifier gate。

## 4. 共同 verifier、重解析与不可变性

共同 final verifier 必须闭合检查 §3 的 envelope、允许 op 集、属性/property
集合、排序及 source ownership；声明和 module definitions 共用 canonical
symbol 唯一性检查。SourceOwner/Occurrence/LogicalType/StaticType/StaticValue
复用现有 shared validators，不能在 C++ 后端另写一套接受规则。

fresh-final 还必须重建以下上下文检查，不能只检查字典形状：

- 每个声明的 `ac.origin.site.definition` 必须精确等于其 canonical `sym_name`
  对应的 FlatSymbolRef。不能用结构合法的 foreign origin 冒充本声明来源。
- 对全部 implementation/declarations/空源/facade unit 重算 canonical
  import-module identity 并要求唯一，继续执行既有 SourceOwner 大小写歧义
  规则。不同 owner 即使没有声明或不同名声明，也不能映射到同一 import module。
- 声明的 immediate parent 必须是其 source-owned final unit；不得嵌入
  ac.module、rule 或其它 region 后靠 walk 收集绕过 placement 规则。

- alias：仅 bool 或 integer；integer 使用 C1/C2 的最小规范宽度、区间和
  signed/unsigned 解释，当前 `1 <= N <= 64`；整数 i1 不等于 bool。
- constant：bool/int category 与 StaticType 精确匹配；可选上下界保持且检查。
  整数 MathInt 保留任意精度，不能在 common final 中缩窄为 i64。
- 声明不产生实例、StateID、寄存器、rule、clock/reset domain 或提交义务。
  可执行闭包验证继续只从 implementation/ac.module 重建。
- materializer 在抹除原声明前完成 authority 选择，并比较投影清单和输入的
  已选 owner/声明集合，禁止漏项、复制、换 owner 或按需生成。
- fresh-process 只读 final 即可重建声明清单和可执行图。FinalProgram 冻结
  snapshot 也覆盖声明，emit 前后验证不能漏掉声明被修改的情况。
- C++、Verilog 和 final verify-only 都先执行同一完整 final 验证。
  RTL 不根据这些静态声明另建状态或重解释已闭合的硬件语义。

完整性边界：重新读取 final 时验证的是现存 artifact，不认证历史源码。
没有新增 inventory 属性；透明 alias 已解析进运行时 LogicalType 时，fresh
verifier 无法证明一个未使用 alias 或空 unit 曾被删除。不得宣称此类删除
一定被发现。producer 清单比较与 frozen-object mutation 检查是独立门槛。
合法修改不相关声明也不构成恢复 source/header authority 的凭证。

## 5. C++ 声明映射与文件组织

复用 C3 legalization 和 SourceOwner 到路径/namespace 的映射。例如
`{package="demo",path="types.py"}` 生成 `sources/demo/types.hpp`，namespace
为 `demo::types`。`pkg/__init__.py` 的 namespace 去除末尾 `__init__`，文件
路径保留真实 stem 后合法化为 `sources/demo/pkg/init.hpp`。不引入特例目录、
内容摘要或按参数值分文件。

| IR 声明 | C++ 声明 |
| --- | --- |
| bool alias | `using name = bool;` |
| unsigned integer alias，包括 i1 | `using name = ::std::uint64_t;` |
| signed integer alias，包括 i1 | `using name = ::std::int64_t;` |
| bool constant | `inline constexpr bool name = true/false;` |
| 非负整数，至 `2^64-1` | `inline constexpr ::std::uint64_t name = UINT64_C(value);` |
| 负整数，自 `-2^63` | `inline constexpr ::std::int64_t name = -INT64_C(magnitude);` |

`INT64_MIN` 单独使用 `(-INT64_C(9223372036854775807) - INT64_C(1))`，不能
先形成越界正 literal。header 自包含所需 `<cstdint>`，常量 inline 避免 ODR
冲突。新声明及同一 source 中的既有 module/system sections 对标准库、
runtime 的依赖引用一律显式使用 `::std::`、`::gfsim::`；不能被源声明
`Std`、`Gfsim` 合法化后的局部名字劫持。不要为回避此问题新增 Python
名字禁令。上述例子生成 `using word = ::std::uint64_t;` 和
`inline constexpr ::std::uint64_t limit = UINT64_C(7);`。

这些 alias 是透明的 native **值载体**，不是 range-checked C++ 类型，也不是
硬件位宽/布局 ABI。C++ 的常规转换/算术不成为 Python 数学语义；既有模块
bit-pattern storage、数值 proof、表达式生成和模型 ABI 不使用它替换语义。
不按物理 width 单独判 bool，不复用 `cppType(width)` 来解释 logical alias。

整数常量超出 `[-2^63,2^64-1]`：link/common final/原本合法的 RTL 仍接受和
保留其精确 MathInt；**C++ 声明生成**在返回/发布任何结果前 capability-reject，
指出 owner、symbol、值和能力范围。不截断、不静默跳过、不替换成 C3
static_integer 身份 tag、不新增 bigint/runtime wrapper。

所有宣称提供本包声明的 C++ renderer 共用该投影/命名/范围检查，不得用旧
单字符串 renderer 绕过拒绝。现有私有 monolithic gate 可直接拼装同一声明
sections；不得把整段 C++ 再按名字切开，或在 backend 回读 source/header。

文件/符号规则：

- declaration-only source 恰有自己的 `.hpp`，无 `.cpp`、空 object 或假 module。
- 空声明源及 pure facade 的 header 为空声明头；不在 facade namespace
  复制 provider 的声明，也不重造 Python import/re-export 别名。
- implementation-owned 声明进入该 source 的已有 `.hpp`；不另开同 owner 文件。
- aliases、constants、module families、namespace components 使用同一碰撞检查；
  任何 scope/path/name 冲突必须拒绝，不能加数字或摘要消歧。
- C++ 占用的全局 scope 也必须进入碰撞检查：用户声明/namespace 不得占用
  `::std`、`::gfsim`，或与本次生成的 support/system glue 全局符号相撞。
  这是 C++ target 拒绝，不改变 source/link/RTL 接纳；`demo::std`、
  `demo::gfsim` 等嵌套源 namespace 不属于这项全局占用，仍须可用。
- 仍按 owner/文件路径稳定排序；header 可单独 include。标量声明无跨源
  nominal reference；不添加无必要的总 types 头或所有声明头全互相 include。
- 未来 C3 generated.json 用已有 files/source_groups 列出 header-only 组，不加
  schema 字段。本包不交付该公开发布入口；私有结果/测试 transport 的表示是
  内部实现，必须能表示缺席的 source 文件，不能伪造 `.cpp` 满足旧 pair 假设。

## 6. 错误、发布与 hard-break 边界

共同验证应区分非法 envelope/op/字段、owner/symbol 不一致、重复 authority、
残留 source metadata/snapshot、非法 type/value/range 和未支持声明类别。
C++ capability 或命名拒绝必须与 malformed final 区分。沿用现有非零失败
约定和 source diagnostics，不新增公开错误码、忽略错误模式或 fallback。

验证在返回文件清单和任何发布之前完成；已有 C3 文件/目录发布保护继续
适用，失败不得覆盖旧产物。standalone final 不要求原 source/header 可读。

删除/替换范围仅是无条件擦除“已选 scalar declarations”和遗漏声明 owner
的逻辑；source-only exports、imports、helper、snapshot 的清理不能回退。
shared final verification 先行，两 backend 同候选适配。不存在旧/新 declaration
schema 选择开关；无声明源的合法 final 仍是当前合同中的合法形状，并非兼容
分支。旧 API、公开 emit 切换与安装资产退役继续由 M5 一次处理。

回退以整个实现候选为边界：共同 verifier、投影、重解析、两个 backend
适配及对应 gates 一起回退；不能只撤掉拒绝检查或保留第二套入口。保存的
final 使用与其 schema 匹配的外部 toolchain/revision；不提供原地降级转换
或在 IR 中增加兼容版本字段。已有输出由原 C3 publication 协议保护。

## 7. 给实现 agent 的 checklist

未获本提案精确批准前，下面均为 planned，不能开始产品实现。

- [ ] D1：共同 final envelope/op verifier；enclosing owner、全局 symbol、
  字段/类别/排序、纯声明 unit 不含硬件内容；保留所有旧硬件 closure 反例。
- [ ] D2：从 owning interface 选择/投影，在既有擦除前完成；全输入清单核对，
  source-only 字段照旧消除；新增声明 frozen snapshot 和 final-only 重解析。
- [ ] D3：同一个 C++ 结构化 emission core 生成声明 sections 与 header-only 组；
  namespace/name/path 冲突统一检查；无新语义引擎或 runtime 类型。
- [ ] D4：独立测试按 §8 实施；实现作者不得修改 oracle 使现状过关。
- [ ] D5：候选冻结、从该 checkout 重建、独立 code review、原有回归和 ledger
  更新。M4 其余 generated publication/完整运行交付不得因此标完成。

建议文件边界：D1 为 ACIRHardwareClosure/FinalContracts 及 shared declaration
validation；D2 为 FinalProgram/FinalHardware/重解析；D3 为 FinalCppEmission/
FinalCppSourceParts。ODS op 定义无扩张需求。共享 CMake 与 API 声明单 owner，
测试与实现分离；先确认 native 编译，再运行对应 gate。

## 8. Verification 与完成标准

独立 oracle 来自输入 owning declarations 和以下确定值，不来自生成器
自己的 manifest 或日志。

| 门槛 | 必须观察到的证据 |
| --- | --- |
| canonical authority | provider、facade、root 分别编译；final 保留 provider 的原 owner/符号，facade 无副本；旧 snapshot/stale binding 反例仍拒绝 |
| 清单完整 | unused/private scalar、implementation-owned 声明、空源、pure facade 都按 §2/§5 出现；与已验证输入清单逐项比较 |
| fresh final only | 保存 `.ac` 后移走 Python/headers/body/deps，新进程重解析，两 backend 接受；声明 header 无需外部语义输入 |
| 类型精确 | bool 与 unsigned/signed i1、窄 signed、u64 边界分别验证 C++ type traits 与 final 区间/解释；不是仅检查字符串存在 |
| 常量精确 | false/true、0、1、-1、INT64_MIN、UINT64_MAX 编译期断言；两 TU include/link 验证 ODR |
| wide MathInt | `-(2^63)-1`、`2^64` 及更大整数的 link/final roundtrip/RTL 成功；C++ 声明生成明确拒绝且无输出/旧目标字节不变 |
| 独立文件 | 声明源仅 hpp；implementation 源保留独立 hpp/cpp 和 object；facade 无伪实现 TU，无全局 types 头 |
| 共同反例 | 错 owner/symbol、只改 origin.site.definition、重复声明/owner、两个空/facade owner 映射同一 import module、snapshot、exports/import_bindings/helper residue、非法类别/type/value、非规范宽度、硬件混入声明 unit、声明嵌入 ac.module/rule，两 backend/verify-only 均拒绝 |
| C++ 名字遮蔽正例 | declaration-only 及 implementation-owned `Std`/`Gfsim` alias，nested `demo::std` source namespace；声明头和实际模块 TU 均独立编译/链接成功 |
| C++ 命名反例 | alias/constant/module/namespace 及 Unicode/大小写/路径合法化碰撞，用户占用全局 std/gfsim 或已生成 glue 符号，正常失败且无部分输出 |
| 非标量边界 | unused list alias/constant、record/helper 等明确拒绝新投影，不静默输出不完整头；不声称 baseline 全部已拒绝 |
| 行为保持 | 同一 fixture 的 C++/RTL 逐拍、reset、失败不提交、reg 数和 alias 连接保持；定义/输入单元重排不改变输出 |
| 修改与信任 | frozen 声明改动被发现；不把手改后仍自洽的 unused 声明/空 unit 删除误称为可检测的源认证 |

计划 gate（文件/测试在批准后创建；现在不声称已执行）：

```sh
cmake --build .pycircuit_out/w10-pm/build --target \
  acir-source-unit-harness acir-design-harness acir-cpp-source-parts-harness \
  acir-backend-closure-harness ACIRFinalProgramTests \
  ACIRExecutableBackendClosureTests -j 4
pytest tests/system/test_final_scalar_declarations.py \
  tests/system/test_cpp_source_parts.py \
  tests/system/test_generic_multi_assignment.py \
  tests/system/test_generic_assignment_roundtrip.py \
  tests/system/test_source_design_bridge.py
ctest --test-dir .pycircuit_out/w10-pm/build \
  -R '^(ACIRFinalProgramTests|ACIRExecutableBackendClosureTests)$' \
  --output-on-failure
```

环境使用当前 checkout 的 native helpers/LLVM 22.1.8；不能复制其他 worktree
的工具链。诊断注入、声明 native tests 注册及 pytest helper 环境须在任务包
写明。当前平台验收不声称 Windows、安装、SDK 或完整 release matrix 已过。
