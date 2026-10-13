# 串行 Rule 与 Method 扩展验收规范

本文规定[扩展提案](README.md)的验收义务，配合[源语义](source-semantics.md)、[MLIR 流程](mlir-pipeline.md)、[扩展边界](extensions.md)与[实现计划](implementation-plan.md)使用。它描述目标行为，不证明当前支持、批准实施或测试通过。现有能力仍以[语言参考](../../reference/language.md)为准；以下完整范围尚未验收；已实现的局部 IR/producer 检查不代表完整扩展通过。

## 验收范围与判定规则

第一批验收限于同一时钟域内的 Module、自主 Rule 和继承外层事务的 Method。Module 实例保有独立状态；Method 调用不得复制状态或产生独立提交点。普通函数负责纯计算。独立 Rule 默认读取旧 Q；同一 Rule 内显式串行状态依赖使用事务局部候选版本。memory 与其他原语的已声明延迟、端口和可见性契约继续有效。

Method owner 是直接包含嵌套公开 `@method` 的 `@module`，首批没有作者定义的 runtime 参数或普通返回；`Counter()` 一类静态构造产生实例句柄。普通 typed-result module 接口不因此改变。Method 内绑定外层 owner 更新，不使用 `nonlocal`，不允许同名局部 shadow owner。只读 Method 也只能在 Rule 事务中调用。

纯 helper 只使用现有已准入的 typed 表达式／语句子集及 source-semantics 中的明确限制；“纯”或“有限”均不授权一般 Python 语法、递归或新的静态求值能力。
`METHOD-PURE-HELPER-CUTOVER` 同时执行[纯函数专项 checklist](pure-helpers.md)，包括
词法快照、内部名称碰撞、native ABI 权威、跨源发布和旧调用者数值／时序回归。

首批 guard 只用 `guard=lambda ...`，形式参数与被装饰函数必需位置参数同名同序。owner 读取使用调用入口的候选版本，普通自由值固定为定义位置的词法 SSA；guard 不可调用 Method/Rule 或产生效果。不能证明 total 的 guarded Rule 必须 `-> None`，普通输出有效性接口延后；total Rule 保留普通返回。每个 Module 实例的 Rule 声明恰好静态注册一次，重复、运行时、循环注册及事务中调用自主 Rule 均拒绝。纯 `def` 不隐含读取 owner，保留的纯 `@rule` helper 调用者须显式迁移，不能静默重解释旧语义。

事务式 Queue 集成是可选后续范围，只能采用[扩展边界](extensions.md)中的明确契约，不能从 Method 基线推断其支持。Signal/EHR 源接口、跨域自动调度和一般静态参数化均延后；基线不得通过引入这些接口绕过现有所有者。保留的 `@system` 必须完成公共 C++/Verilog 仿真闭环，编译器继续生成其时钟、复位、使能与测试系统脚手架。

每项需求使用以下稳定语义标识。测试实现与结果须分别记录：存在文件不等于执行成功，局部成功不等于 gate 或 nightly 成功。负例要求相应诊断类别、源位置和责任对象可识别；本文不预设尚未实现的诊断代码或逐字文案。

## 必须建立的语义证据

跨实例执行还必须关闭[专项 checklist](cross-instance-lowering.md#cross-instance-checklist)
中的六个 `METHOD-*` 需求组；它们细化实例身份、物理根 ABI 和接线原子性，不替代本表。
每项剩余能力的前置合同与接单检查见[实施矩阵](extension-roadmap.md)。

| 需求标识 | 正例与独立期望 | 负例／边界与预期诊断类别 | 最小证据层 |
| --- | --- | --- | --- |
| `METHOD-INSTANCE-IDENTITY` | 同一实例重复调用共享一个 owner；两次结构实例化保有独立状态，名称变化不改变行为 | 无法解析的 owner、动态实例化、跨域事务：所有权／实例／域不支持 | typed effects、公共双目标 |
| `METHOD-SOURCE-OWNER` | 零 authored runtime 参数、无普通返回的 owner 模块提供静态句柄和公开嵌套 Method；捕获更新直接作用于候选 owner | owner 模块 runtime 参数／普通返回、局部 shadow owner、`nonlocal`、事务外只读 Method：源接口／绑定不合法 | capture 到 typed effects、公共 compile |
| `METHOD-SOURCE-TYPE-AUTHORITY` | Boolean、同宽 Integer 和 fixed bits 经真实 capture、typed IR 打印重读及独立接口传递后保持各自类型约束；captures 不改变显式参数数量 | 缺失必填契约、错误形状／arity／return form、非法 type/domain、foreign origin 或 SourceOwner 路径：类型／来源契约不合法；不得按位宽补默认值 | 原生 verifier、真实 capture；独立发布完成后补公共跨源 compile/link |
| `METHOD-GUARD-ENTRY` | 同一 Method 多次调用的 guard 分别读取调用入口候选，后续 body 写不反向改 guard；普通自由值保持词法快照 | guard 参数名／顺序不符、任意函数 guard、有 Method/Rule／资源／观测效果：guard 签名／效果不合法 | state SSA、admission、公共双目标 |
| `METHOD-RESULT` | total Rule 可返回普通值；可能不发射的 Rule 仅 `-> None`；Method 普通返回在有效路径内使用 | guarded Rule 普通 payload 逃逸或制造隐藏 valid 寄存器：结果有效性／首批不支持 | typed validity、公共 compile/link 负例 |
| `METHOD-REGISTRATION` | 每个静态模块实例恰好注册一次其 Rule；不同实例事务身份独立 | 重复、运行时、循环注册，Rule 内嵌套调用自主 Rule：注册／事务入口不合法 | typed identity、公共负例与发布保护 |
| `METHOD-PURE-HELPER-CUTOVER` | 纯 `def` 只以显式值参数和固定词法 SSA 计算；迁移保留的纯 `@rule` helpers 后双目标结果不变 | 纯函数隐含 owner 读取或 effect、旧 nested autonomous Rule 调用：纯度／调用边界不合法 | call summary、现有 helper caller 回归 |
| `METHOD-DIRECT-MODULE-PRESERVATION` | 普通 typed-result module 的输入、返回、实例身份、Struct/Table transport 与现有 gate oracle 保持一致 | 不能因 Method owner 新分类而把普通 module 调用解释成句柄 | 现有 API/example gate、公共双目标 |
| `METHOD-STATE-ORDER` | 设计中立的有序槽队列在一个事务中依次完成、释放头项、分配：前一步候选状态影响后一步；对应 ROB complete→retire→allocate 的需求 | 不得把三步变成独立旧 Q Rule；未授权资源转发须拒绝 | state SSA、原生 workers 1/2、RTL、外置 oracle |
| `METHOD-SNAPSHOT-OLD-Q` | 保存局部值后更新 owner，保存值保持调用时快照；另一自主 Rule 仍读旧 Q；交换独立 Rule 注册顺序不改结果 | owner 绑定与值快照混用：绑定／类型不合法；不受支持的别名写：别名冲突 | state SSA、公共双目标 |
| `METHOD-PATH-VALIDITY` | 仅执行路径上的 required 调用参与 admission；未执行分支不会索取资源；无效读取 payload 不得作为 false 进入 else | 使用无效返回值：值有效性无法证明；分支缺失绑定：未定义值 | typed admission、公共双目标 |
| `METHOD-REQUIRED-ATOMICITY` | 设计中立的多个库存 owner 共同发布一组更新：全部 required 条件成立时全更新，任一不成立时该 Rule 全部保持；对应 S1 多 owner all-or-none | 单 owner 先提交、循环内隐含部分提交：事务原子性不合法 | admission、最终 IR、原生与 RTL 状态 oracle |
| `METHOD-OPTIONAL-PROGRESS` | optional 步骤返回 accepted/success/retry，只有显式 required 条件阻止事务；不能退休时仍允许合法完成；把 status 字段改名不改变 admission | 根据 `accepted`、`valid`、`success`、`retry` 字段名把普通 status 升格成隐含 guard：需由语义 oracle 捕获 | admission、公共双目标 |
| `METHOD-RETRY-CURSOR` | 设计中立的槽位探测器 accepted=true、success=false、retry=true 时仍推进探测游标；对应 PFL retry 与游标前进 | 把 busy 当 required guard 导致游标永久停留：语义回归 | state SSA、admission、多 epoch oracle |
| `METHOD-GUARD-LOCAL-HOLD` | 新增 required guard 为已知 false 时，只阻止所属外层 Rule；无关联的 Rule 正常提交，系统无硬错误，成功采样继续维护其时钟历史 | 不得转为全局 check failure；未知控制另按有效性／四态契约判断 | 公共原生与 RTL |
| `METHOD-GLOBAL-ZERO-COMMIT` | 任意全局硬错误发生在检查阶段：所有 owner 的 Q、时钟历史、提交计数及成功观测均不前进；恢复前先证明零提交，随后按既有 Reset／重建协议恢复再测试 | 部分 owner 或 clock history 已提交：全系统提交协议错误；SimSystem sticky Failed 不因仅修改输入而恢复 | 独立状态／时钟探针，原生 workers 1/2 与 RTL |
| `METHOD-FOUR-STATE` | X/Z payload、保存快照、分支选择、聚合 transport 保持现有四态意义；active edge 的未知有效 Method guard／call control 全局硬错误且零提交；已知 false 支配路径不激活下层检查；reset 抑制新增 admission faults | 将 X/Z 当 false、纯 X/Z 数据无故失败、reset 时新 fault 激活、删除有效检查：四态／未知控制／reset 错误 | 原生四态测试与真实四态 RTL；Verilator 两态运行不能代替 Icarus X/Z 证据 |
| `METHOD-ALIAS-CONFLICT` | 同一资源的不同引用归一化；可证明的独立字段／元素允许并行，串行候选覆盖只用于同事务明确允许的 owner | 别名掩盖双重消费、重叠写、超端口：别名／冲突／资源限制 | summaries、resource verifier、负例发布保护 |
| `METHOD-CAUSALITY` | 独立纯计算无多余串行边；真实状态、有效性、ready 与顺序依赖均保留 | 数据／有效性／ready／转发组合成环，包括未观测死环：因果循环；不得插入隐含周期 | 图分析、最终验证、公共负例 |
| `METHOD-SUMMARY-AUTHORITY` | provider 独立编译，consumer 仅读已发布接口，link 完整闭包重验 body/summary；相关变换后重算摘要 | 伪造 imported Method 的读写、required guard、域、端口、顺序或 owner 摘要；即便同步伪造发布哈希仍被 body 重验拒绝：接口权威／摘要不一致 | native pair verifier 与公共 compile/link |
| `METHOD-EFFECT-RETENTION` | effects、观测、写意图、身份写、资源义务保留至对应证明完成；无用返回值不意味着调用无效 | 把 effectful op 标 Pure，DCE/CSE 隐藏非法冲突或丢失检查：effect／验证不合法 | typed IR、优化前后、独立负例 |
| `METHOD-DUAL-FINAL-IR` | source-owned 发布物与完整 link 闭包得到一个 verified final artifact，两目标消费完全相同字节 | 目标重捕获 Python、目标侧补语义、闭包缺失／重复：闭包／目标支持错误 | 公共命令、哈希、manifest |
| `METHOD-SYSTEM-CLOSURE` | 保留 `@system` 的自主 Rule、终止边界、assert/log/report、复位、失败 discard 与双目标模拟；原生 workers 1/2 与 RTL 对照外置 oracle | 以 module-only 宿主驱动替代 system；静默丢弃观测；错误根类型：系统／根接口错误 | 现有 system 测试及扩展集成证据 |
| `METHOD-OUTPUT-PROTECTION` | compile/link/两目标 emit 失败均保持先前发布物字节和清单；新诊断输出不能别名输入、已有文件或正式输出 | 无效 summary、非法效果、输出别名／写失败：输入验证／输出保护 | 公共失败替换与 native verifier |

`METHOD-FOUR-STATE` 必须包含一对顺序反例：先到达 X required guard、后续 guard 为 false，仍全系统失败且零提交；先到达 false guard、后续 guard 为 X，则只 hold 该 Rule 而不激活后续新错误。外层 Rule guard 的 X 也不能被 body 后续 false 屏蔽。无状态写入但有 guard/check 的 Rule 必须绑定唯一采样域；未知域拒绝，不能因无写而删掉检查。

受限直线动态 guard 的完整子矩阵见[采样准入验收](sampled-admission.md#acceptance-checklist)：
包括所有 transaction 的 uniform sampling、false fire 的 history 提交、Discard 后同 high
重采样、物理 clock/reset X/Z、guard span、原 plan 与独立 final 两层故障注入及预算保护。
它细化上述需求，不另开 gate；完整 Method System 和后续分支/资源仍分别验收。

上述槽队列、库存发布和槽位探测器是框架测试中的设计中立类比；不得把完整 ROB、S1、PFL consumer 代码、ISA schema 或 oracle 放入编译器。变化名、宽度、初值、槽数、语句形状与实例顺序的有界变体用于排除场景识别。变体保留相同语义义务，不要求当前尚未支持的一般参数化。

## 可选 Queue 集成的独立出口

| 需求标识 | 验收内容 | 证据 |
| --- | --- | --- |
| `METHOD-QUEUE-REQUEST-ACCEPT` | requested 与 accepted transfer 分开表示；required 与 optional 调用规则明确；别名不可增加端口或重复消费 token | typed resource IR、独立 Queue oracle |
| `METHOD-QUEUE-PROSPECTIVE-CAPACITY` | 同一事务的预期容量计算先于握手物化；允许行为精确匹配选定原语契约；不改变 no-empty-bypass、延迟或 capacity 定义 | 满／空／同时操作／延迟边界，原生与 RTL |
| `METHOD-QUEUE-NO-SELF-CYCLE` | 不从已物化 fire 推导自身 ready/pop；真实因果环拒绝，不用固定点或隐含寄存器求解 | ready/data 联合图与负例诊断 |

Queue 出口未完成时，报告“Method 同域基线已验收、事务式 Queue 未验收”只能建立在基线真实证据上；不能宣称全部扩展完成。

Queue 句柄、资源参数、隐式 pop，以及 Signal/EHR 的源拼写仍是单独的源接口决策检查点。内部 IR 的资源正例即使通过，也不能计为对应 Python 源功能通过。

## 阶段证据与候选绑定

每个语义切片保留输入／输出 IR、验证结论、分析失效与重算记录。拟议 typed effects、Method summary、state SSA、admission、resource 与 transaction lowering 边界的证据接口须在对应实现派发前冻结；它们目前不是已存在的 CLI 选项。

早期切片可只完成 typed IR／unit 层的局部检查。公共 compile/link/双目标执行依赖最终闭环尚未具备时，对应结果保持 pending/NOT-RUN；mock、脚本计算或直接 IR 测试不能补作公共源功能执行证据。切片局部通过不等于其全部需求验收。

现有 native `--source-import-out` 保存 **source lowering 后、record-wire simplification 前**的完整硬件 IR。它不是 raw capture，也不是尚待实现的 typed effect 或 state SSA 快照。若以 native 重现补充公共流程证据，须绑定 capture、headers、owner、工具与实际命令，并比较重现 body/interface 与公共发布物。该诊断文件置于发布目录外，不提供 provider authority。

候选记录遵守[治理](../../development/project-governance.md)和[gate 证据政策](../../gates/README.md)：Git revision、dirty 内容绑定、工具与 Runtime、源／测试／oracle 字节、完整命令、退出状态、输出与 manifest 哈希、缺失工具、跳过项。记录置于 `docs/gates/logs/<run-id>/`，测试不得读取该目录。只记 IR 摘录、计数或图形截图不构成完整阶段验证。

建议维护如下追溯表；每个需求可有多行，以区分编译器证明、执行行为和发布保护。

| 需求标识 | 源条款／原语契约 | 所有者与 pass 边界 | 独立测试文件／用例 | 验证层 | 候选与证据路径 | 实现状态 | 执行结果 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `METHOD-STATE-ORDER` | source-semantics 的串行候选与快照条款 | state SSA 输入／输出 | 派发前冻结 | typed IR + 公共双目标 | 待实际运行填写 | 未实现 | NOT-RUN |
| `METHOD-GLOBAL-ZERO-COMMIT` | whole-system 检查先于 Xfer | admission／最终验证／runner | 派发前冻结 | 状态与时钟探针 | 待实际运行填写 | 未实现 | NOT-RUN |

## 可复现执行与 gate 选择 {#commands-and-gates}

先在本 checkout 构建、安装，再运行已注册的局部用例。`PYC_BUILD_DIR` 与 `PYC_TOOLCHAIN_ROOT` 必须属于该 checkout；scratch 为源树外的全新目录。以下为既有命令模板，变量由实际运行记录填入；它们不是本提案的执行结果。

```bash
export PYC_BUILD_DIR=/absolute/path/to/this-checkout-build
export PYC_TOOLCHAIN_ROOT=/absolute/path/to/this-checkout-install
cmake --build "$PYC_BUILD_DIR" --parallel 4
cmake --install "$PYC_BUILD_DIR" --prefix "$PYC_TOOLCHAIN_ROOT"
export PATH="$PYC_TOOLCHAIN_ROOT/bin:$PATH"

# 先列出实际登记项；--list 不构成通过。
bash tools/run_api_tests.sh --tier gate --list
ctest --test-dir "$PYC_BUILD_DIR" -N
# 将测试所有者确认过的非空选择填入，不虚构未来 CTest 名称。
ctest --test-dir "$PYC_BUILD_DIR" -R "$METHOD_CTEST_REGEX" \
  --output-on-failure --no-tests=error

# 局部语义测试与独立复核成功之后执行适用的短 gate。
bash tools/run_api_tests.sh --tier gate
bash tools/run_examples.sh --tier gate
```

公共双目标复现保持一源一 unit、明确完整闭包，示例中的 provider、consumer 和 `TOP` 由实际测试确定：

```bash
mkdir -p "$METHOD_SCRATCH/units"
pycircuit compile -c "$METHOD_SOURCE_ROOT/provider.py" \
  --source-root "$METHOD_SOURCE_ROOT" --package-prefix "$METHOD_PACKAGE" \
  -o "$METHOD_SCRATCH/units/provider"
pycircuit compile -c "$METHOD_SOURCE_ROOT/consumer.py" \
  --source-root "$METHOD_SOURCE_ROOT" --package-prefix "$METHOD_PACKAGE" \
  -I "$METHOD_SCRATCH/units/provider" -o "$METHOD_SCRATCH/units/consumer"
pycircuit link "$METHOD_SCRATCH/units/provider" \
  "$METHOD_SCRATCH/units/consumer" --top "$METHOD_TOP" \
  -o "$METHOD_SCRATCH/final.ac"
pycircuit emit "$METHOD_SCRATCH/final.ac" --target cpp \
  -o "$METHOD_SCRATCH/cpp"
pycircuit emit "$METHOD_SCRATCH/final.ac" --target verilog \
  -o "$METHOD_SCRATCH/verilog"
```

若实际闭包包含更多源，须逐源 compile 并显式全部 link；不得省略 provider。执行交给现有 lit、CTest 和 system 测试所有者，复用 generated CMake 与 Runtime。module 测试使用 typed DUT/SystemRunner；`@system` 使用现有生成的 `pycircuit_sim`，不另造测试运行协议。workers 1/2、RTL、真实 X/Z 与失败路径分别列出结果。

长期差分、参考模型、mutation 与完整注册矩阵归现有 nightly；普通实现或文档复核不得自动启动。请求或调度 nightly 时使用：

```bash
bash tools/run_api_tests.sh --tier nightly
bash tools/run_examples.sh --tier nightly
```

本表的完整实现验收及 gate/nightly 不能从局部测试推断。未实际执行的范围必须记录为 **NOT-RUN**；已有原生 IR/producer 结果只覆盖其明确的检查点。测试文件本身仅表明责任入口。

## 效率验收

结构义务与性能测量分别报告。指定合法 fixture 必须在派发前冻结源、oracle、输入、目标及比较版本；只对这些明确样本作结构承诺。

| 标识 | 必须满足的结构义务 | 测量项目与限制 |
| --- | --- | --- |
| `METHOD-NO-ADDED-LATENCY` | Method 抽象及优化不增加寄存器、隐含 clock cycle 或原语延迟；真实多周期协议状态不删减 | 报告 storage／ports 与逐 epoch trace；不能以吞吐变好掩盖周期变化 |
| `METHOD-UNUSED-FIELD-ELIMINATION` | 指定合法 fixture 中未使用的临时聚合字段计算在最终 IR 与双目标实际生成逻辑中消除，同时保留检查、写意图和资源义务；公开 Struct/layout 与 owner 可观察字段布局保持原契约 | 记录临时字段、live-use 证明、前后完整 IR 与生成输出，不以删除公开字段满足义务 |
| `METHOD-PURE-SHARING` | 指定 fixture 中类型、guards、支配关系及四态语义相同的重复纯计算，在共享作用域只保留一次；effectful 调用与不同状态版本不可合并 | 分别报告运算、mux 深度及扇出；共享可能改变关键路径，不普遍保证更快 |

另外分别记录 capture/IR 大小、compile/link 时间与峰值内存、硬件状态／端口／mux、可用时的综合面积和时序、仿真吞吐。记录工具、机器、首次初始化与重复执行条件；没有综合工具就写未测。不得设定没有基准支持的统一速度阈值，不把 IR 操作数下降当作面积或时序通过。

## 作者与独立复核者清单

作者提交前：

- [ ] 在任务包中冻结需求、输入、独占文件、新文件精确路径、依赖、移除范围、局部测试和证据边界。
- [ ] 保持单一路径和通用 owner/effect/type 推导；无 fixture 名称规则、shim 或后端补语义。
- [ ] 每个 mutating stage 验证输入／输出，声明分析失效并重算摘要。
- [ ] 外置 oracle 与 DUT 隔离，覆盖串行、快照、跨 Rule 旧 Q、required/optional、retry、四态和零提交。
- [ ] 同一 final artifact 双目标、保留 system、发布失败保护均有实际候选证据。
- [ ] 明确基线／可选 Queue／延后接口与 NOT-RUN，未把删除覆盖算作成功。

独立复核者提交前：

- [ ] 使用与实现者分离的实例检查 source 条款、IR、资源与双目标行为；记录实际 role/model/effort。
- [ ] 用设计中立反例验证局部 guard false 不造成全局失败，以及全局硬错误无状态／时钟提交。
- [ ] 检查跨源伪造摘要、别名、冲突、超端口和因果环在优化前不会被隐藏。
- [ ] 核对每个需求的独立测试、完整阶段文件、命令、退出状态与候选绑定。
- [ ] 核对三个结构效率义务及测量局限，拒绝无依据的普遍速度结论。
- [ ] 单独列出未实现、未执行、不可用工具和未通过项目；只有实际完成的范围可以验收。
