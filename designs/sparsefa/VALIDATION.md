# FA2 当前验证记录

## Shared-KV 批处理目标

新增的 [Shared-KV 多Head合同](ndf/l1/fa2-core-skv-0002.md)、
[64项Gather与双/多缓冲微架构](ndf/l2/fa2-core-skv-1005.md) 和
[泳道图](ndf/l2/fa2-core-skv-swimlane-1005.md) 均为设计草案。
目前没有 `SharedKVCommand`、批量DMA、KV slot manager、跨Head feeder 或完整
System 源码；因此该目标硬件的 compile、link、C++执行、传输掩盖和物理性能全部
**NOT RUN**。下方的P0S及独立三维阵列记录不能视为Shared-KV目标的验收。

另有[测试自有的 C++ 周期模型](tests/timing_model/README.md)已实际执行并输出
[泳道图](tests/timing_model/assets/shared-kv-2slot-model.png)：H=64、D=64、
Count=128、8个 KV 行 credit、32 B/cycle 总线、16周期行延迟的固定场景中，
包含 Count 4 B 与 Q 8192 B 的传输时间，单 slot 2170 周期，双 slot 1898
周期，模型节省272周期（12.5%）。模型假定预先生成的128个 Index ID 均有效、
互异且未被 causal mask 屏蔽；每项预留1周期校验时延，但未输入或校验实际 ID。
原始 CSV 和图的[可审阅快照](tests/timing_model/README.md)已保存；
原始运行文件与命令证据保存在忽略的
`docs/gates/logs/sparsefa-skv-timing-284d0921/`。这是独立资源/日程模型，
不计算注意力结果，也不是 pyCircuit 生成硬件的 C++ 仿真。当前工作树重新
编译 P0S 源码时遇到旧式局部 Rule 显式调用；临时改为现行语法后，整机与
单独行引擎的 native link 均超过默认分析预算，因此没有新的 P0S 硬件 trace。
上述试验改动未保留，失败证据在忽略的 `docs/gates/logs/fa2-p0-completion/`。

同一独立周期模型的 [2048 Index 长序列运行](tests/timing_model/README.md) 共
32 批、2048 次 KV 行请求；[全程图](tests/timing_model/assets/shared-kv-2048-overview.png)
和[前五批计算放大图](tests/timing_model/assets/shared-kv-2048-detail.png)由原始
CSV 事件生成。单 slot 为26,410模型周期，双 slot 为17,978模型周期，差8,432周期
（31.9%）。模型先顺序读取并预留校验全部 Index 的时延，Q 加载结束后才开始
KV gather；这仍不是生成硬件或真实 Index 内容的仿真。

## 当前三维 MAC 阵列

新增 `fa2_array3d_interfaces.py` 和 `fa2_array3d.py`，顶层为
`fa2.fa2_array3d.Array16x16x16`。源码层级是 16 Row × 16 Cell × 16 Product，
共 4096 个独立乘积寄存子模块；随后是四级真实归约寄存器和 256 个累加器。
数据状态合计 8192 个 u32，即 32 KiB，不含控制/占用和待实现的供数 buffer。
该数量是源码结构清单，尚无 verified final 或综合网表的结构验收。

当前 DUT SHA256 为 `d69a86b634aa2ee2cc03b923702f8fec45ce7618718bf704bec7bdebbbbf36dc`。
树形归约使用独立 RTZ/FTZ 逐节点参考，不能把旧二维逐项累加的结果直接当作期望。
硬件接口包含尾组、cell/row/column/K mask、组号检查、显式推进/保持、END 排空和
ABORT 优先丢弃。外部 index 仍由调用方提前生成；此阵列只消费已准备好的 A/B 操作数。
P0S、旧二维阵列、numeric 源码和冻结 L0 保持不变。三维源码完成后，为启动
C++ 仿真，已对编译器调用入口状态记录做通用优化，详见下方本轮记录。

| 检查 | 最终候选结果 | 范围 |
| --- | --- | --- |
| 独立 Fraction 数值/生命周期/日志校验 | PASS，7/7 host tests | 树形与线性/FMA 区别、FTZ/signed zero、mask、tail、取消、错拍/错值拒绝；不等于 DUT 执行 |
| 输入审计和 Ruff | PASS | 459 条指令、11 份操作数矩阵；bench 只含输入，不注入参考结果 |
| 五个独立 source unit compile | PASS | numeric、旧 Row16 类型、新接口、完整三维源码、System bench |
| 完整 4096-product System 默认 link | FAIL | 最新 frontier 工具链：transaction plans，requested 4、used 1048575、limit 1048576 |
| verified final、同一 final 的 C++/Verilog emit、C++ workers 1/2 | NOT RUN | link 未生成 final，不能宣称阵列仿真通过 |
| Queue take+push 反压、中途 reset、X/Z 故障注入 | NOT RUN | 当前 host 模型/日程不能替代这些生成硬件测试 |
| Verilog 仿真 | NOT RUN | 按用户要求排除 |
| SRAM/gather/softmax 集成、完整 attention、物理时序/PPA | NOT RUN | 独立阵列之外的集成和物理验收 |

最终 fixture 准备了 43 个成功 tile，每个 worker 将核对 11,008 个输出元素；覆盖
K=1/15/16/17/31/32/33/48/63/64，其中 K=33 单独覆盖第三组的一项有效尾部。
461-cycle horizon 是测试终止界限，不是阵列延迟。连续供数时 `G=ceil(K/16)`，
begin-to-take 的设计公式为 G+6、相邻 begin 间隔 G+7；K=64 对应 10/11 拍。
组 II=1、每组 2 KiB 供数均为设计合同，尚非实测吞吐或时钟频率。
只有动作日志中的 begin/take/group 间隔可作为外部可观察周期；runner 的 `model_`
字段使用参考模型的 RESULT 边界，不能称为独立测得的内部 ready 时刻。

本轮完整记录：`docs/gates/logs/fa2-array3d/20261104T082711.511978Z/manifest.json`，
包括命令、退出码、产物和 154 个已记录输入/工具文件的前后相同 hash。
工具链为本 checkout 构建的 `.pycircuit_out/fa2-frontier-install`。
此前 `fa2-array3d/20261104T074254.674027Z/manifest.json` 保留原工具链的失败记录
（requested 512、used 1048269），当时记录了 181 个文件。
host 记录在 `fa2-array3d/host-final-k33/manifest.json`；独立源码/作者合同审查在
`fa2-array3d/final-source-authoring-review.json`，结论 clear，不替代失败的完整 link。
上述路径后两项均相对 `docs/gates/logs/`。此前 449 指令/42 tile 的失败记录保留在
`fa2-array3d/20261104T073610.941015Z/`，不能与最终候选混用。

最初 source compile 的 Table query 展开限制通过真实 Product 子模块、标量尾掩码
helper 和行掩码表达式消除。此次仿真重试未改动 DUT、输入日程或参考模型，也未
缩小阵列或提高预算。

### 本轮 C++ 仿真启动与编译器修复

调用入口原先复制所有已发现 owner 的版本表。现在只保存 owner 数量和版本边界，
按被访问 owner 的前驱链恢复入口版本；新增 owner 仍取 Q0，分支日志和原子提交
语义不变。修复前后各 309/309 项 native 事务测试通过，22 份完整计划/产物逐字节
一致；独立实现审查 clear。部分小用例因额外校验而增加分析工作量，不能据此宣称
整体编译加速或阵列性能提升。记录位于
`docs/gates/logs/fa2-array3d-simulation/native-baseline/frontier-candidate-summary.json`。

旧 P0S product/row 重新 compile/link/同一 final 双 emit，通过 C++ 库构建，56/56
份产物与原基线逐字节一致。D1 index 的 46/46 个 C++ 用例通过，workers 1/2
结果和周期指标一致；这是已有索引模块回归，不是三维阵列执行。记录分别位于
`docs/gates/logs/fa2-array3d-simulation/frontier-p0s-regression-20261104T083108.032259Z/`
和 `docs/gates/logs/fa2-index-performance/20261104T083145.820572Z/`。

默认预算诊断在 `Array3DCell.clear` 建立 owner 时终止。独立检查保留的归一化 IR
进一步确认：`Product.load` 含 88 个单结果纯操作，4096 个调用需展开 360,448 份
操作；仅这部分在当前 lowerer 的准备阶段最低计费 1,802,240，独立来源校验最低
计费 7,208,960，均超过 1,048,576，尚未计加法树、状态和其他操作。因此只压缩
状态快照或事件记录不足以完成链接。下一步需要通用层级化 lowering 或等效的
共享定义方案，实际减少重复结构与检查工作，同时保留所有空间实例和原子提交。
该方案尚未实现，不能用它宣称完整仿真可用。详细证据见
`docs/gates/logs/fa2-array3d-simulation/frontier-diagnostic/independent-flat-clone-floor.json`。
诊断插桩已移除，编译器已恢复到经过审查和测试的 frontier 版本；未执行 RTL 仿真。
完整 API/example/nightly gate 未运行，309 项 native 专项不能替代这些完整入口。

设计与供数合同分别见 [ARRAY3D-1002](ndf/l2/fa2-core-array3d-1002.md) 和
[ARRAY3D-BUF-1004](ndf/l2/fa2-core-array3d-buf-1004.md)，重现入口见
[三维阵列测试](tests/array3d/README.md)。

## 当前索引优化结果与验收边界

当前源码以精确Patricia树替换逐项重复扫描，NDF见[IDX-1003](ndf/l2/fa2-core-idx-1003.md)。
节点4096×64增加32KiB，总主要数据存储40.75KiB；不生成index，保留输入顺序、range/
duplicate/memory优先级和取消协议。L0未改，数值算术及单outstanding接口未改。

当前engine SHA256 `9fd0141f0fb2eaf9be1b0b29fbf734bfa568ed1dd17bcf6ad20f58f2e62f9ab2`。
四个互斥节点写阶段在同一个原子step Rule中共用一个Memory write，地址/数据mux读取
提交前状态；指针更新、容量断言、取消和发布时序保持不变。独立源码审查通过。
整机和独立row已在默认1,048,576预算下link，分别从同一verified final生成C++/Verilog；
整机C++库构建通过。工具链为本checkout构建的`.pycircuit_out/fa2-resolution-install`，
历史安装保留，所有测试期间输入和工具hash不变。

相同冻结host适配器对旧/新生成DUT检查全部索引与K/V顺序、单outstanding、反压稳定性和
精确输出。表内负载为D=1、Q=K=0、V=1、FP16、降序列表、总线响应延迟1；是命令
接受后的真实成功rising-edge拍数，不是host耗时或System日程。

| 有效索引数 | 旧首Q | 新首Q | 旧完成 | 新完成 | 完成加速比 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 128 | 16,772 | 2,804 | 18,443 | 4,475 | 4.12× |
| 512 | 263,684 | 13,296 | 270,347 | 19,959 | 13.55× |
| 2048 | 4,200,452 | 61,420 | 4,227,083 | 88,051 | 48.01× |

扩展基线记录：`docs/gates/logs/fa2-index-performance/expanded-baseline-summary.json`。
旧workers1完成23/23项，旧workers2完成有界21/21项，后者不含N512/2048。当前候选
workers1/2各23/23通过，共46项，全部共同指标一致；与旧基线可逐worker比较的44项
adapter源码和prefix hash相同。覆盖uint32高位/边界、32个分歧位、重复/越界/访存优先级、
取消写入边界及同DUT重启。请求在运行时逐次核验，未额外记录实际请求流hash。
候选与比较记录：`docs/gates/logs/fa2-index-performance/20261103T212449.289947Z/`
中的`manifest.json`和`comparison.json`。首K/V均在首Q之后4拍。

这项优化有明确代价：新增32KiB节点存储；N4高位用例完成91→107拍，33项反向深树
用例1628→2750拍，第2项即重复的错误返回23→26拍。不能声称所有列表都加速。
D1用于隔离索引成本，不能外推成D64实测、16×16阵列性能或物理吞吐。

| 当前候选检查 | 结果 |
| --- | --- |
| 四provider compile；整机/row默认link、同final双emit、整机库构建 | PASS |
| 公开整机端口D1索引矩阵 | PASS，46/46，C++ workers1/2 |
| Layout System、controller21四态 | PASS，C++ workers1/2 |
| Tensor、Recovery、Lifecycle、Vectors、Faults五个完整System | FAIL，默认original-source proof预算耗尽；未生成final，未执行 |
| 完整16×16阵列System | FAIL，transaction plans阶段，requested2805/used1048103/limit1048576；仍在clear分析，尚无阵列执行 |
| Verilog仿真、综合/时序/PPA、完整P0T attention集成 | NOT RUN；RTL仿真按用户要求排除 |

当前完整System回归整体仍为FAIL，不能用D1矩阵替代D64/BF16/multi-query验收。
运行记录：`docs/gates/logs/fa2-p0s-uniqueness/resolution-regression-20261103T212337.147794Z/`。
较早多Rule/单step四写点及调度器合并实验的失败记录均保留；调度器已恢复原源码。

通用编译优化保留稀疏分支回滚记录及完整静态绑定的方法解析复用。后者按原call和完整
owner/integer/type绑定比较，实际PE状态、资源、参数及guards仍逐次处理。独立native
事务测试308/308通过，18份完整计划/产物与基线相同；三个解析专项的analysis work为
47806→44669、69850→64316、129087→117161。这是编译分析工作量，不是硬件周期。
记录：`docs/gates/logs/fa2-static-resolution-reuse/{candidate,independent-native-review,install}.json`。
阵列精确定位和恢复记录在`docs/gates/logs/fa2-resolution-diagnostics/`；事件存储压缩仅为
下一步提案，尚未实施。默认预算未提高，阵列未缩小。

以下历史PASS均对应各段明确记录的线性校验器候选，不自动适用于新的Patricia源码。

## 线性校验器历史基线

状态更新：2026-11-04。P0S 原始基线的源码、命令、退出码、输入与产物hash保存在
`docs/gates/logs/fa2-p0-completion/`，编译器修复的原始基线保存在
`docs/gates/logs/fa2-engine-p0s/`。P0S的六个完整System均已通过生成C++执行；以下
区分整机行为、编译器通用回归和未执行的全域/物理检查。

| 检查 | 当前结果 | 范围 |
| --- | --- | --- |
| 独立数学/地址模型 | PASS，20个测试 | 数学集合、分块、布局、地址边界；不等于DUT |
| 浮点基本随机样例 | PASS，32组，生成C++ workers1/2 | add/mul/div、FP16/BF16转换 |
| 有理数逐位RTZ样例 | PASS，32组，生成C++ workers1/2 | cancellation、sticky、FTZ、饱和、指数边界 |
| 指数采样 | PASS，4096点，生成C++ workers1/2 | [-16,-1]，最大观测相对误差2.95545e-5 |
| 浮点边界System | PASS，生成C++ workers1/2 | ±0、转换、-16截尾等 |
| 握手Rule四态顺序 | PASS，原始/嵌套两种写法各16组，生成C++ workers1/2 | 外/内guard取0/1/X/Z；失败时状态和epoch均不提交 |
| 控制器错误响应四态 | PASS，21组，生成C++ workers1/2 | COUNT/INDEX响应error=1时，data=X/Z仍得到code5；含合法/越界、无响应和cancel对照；仅验证ControllerTransition |
| 完整engine/scheduler源编译 | PASS | 完整接口、数值、行引擎和tensor调度闭包 |
| 独立行引擎link和双目标生成 | PASS | 当前完整行引擎默认预算link通过；同一final生成C++/Verilog，尚不等于行行为通过 |
| 完整tensor顶层link、双目标生成和C++库构建 | PASS | 最终borrowed-normalization候选在默认route/native预算下通过；未缩减源码闭包，C++/Verilog使用同一final；Runtime-only配置和静态库构建通过 |
| Tensor布局System | PASS，生成C++ workers1/2 | 正常终止于epoch4，确认tensor_layout_pass事件；不替代attention数据路径 |
| Tensor调度System | PASS，生成C++ workers1/2 | epoch2400终止，tensor_scheduler_pass；B/H/query完整数据路径 |
| Recovery System | PASS，生成C++ workers1/2 | epoch1000终止，tensor_recovery_pass |
| Vectors System | PASS，生成C++ workers1/2 | epoch10000终止，vector_pass；BF16多维、FP16 D64 |
| Faults System | PASS，生成C++ workers1/2 | epoch6000终止，fault_pass；访存/输入异常及重启 |
| Lifecycle System | PASS，生成C++ workers1/2 | 默认预算link；epoch4000终止，生命周期完成事件；8条命令覆盖索引错误、取消/drain、重启、空列表与count2048后首token越界 |
| 编译器独立专项回归 | 最终候选255/255项PASS | 事务/写域111、wiring/容量/availability122、Memory源码3、identity19；含多mask链正例和5类篡改负例 |
| 编译器24个native gate suites | 合并证据23/24 PASS，整体仍FAIL | 首次21项通过；修正两处测试后SourceInterface 226/226、HardwareSourceChecks 196/196通过；剩余SourceIdentifier的3个Unicode oracle测试要求Python3.14.6/Unicode16，当前配置3.12.13/Unicode15 |
| Memory/Queue生成C++回归 | PASS，Memory 6项，Queue 322组，workers1/2 | 沿用独立资源用例和原oracle；同一final双emit；不计作完整资源测试套件或RTL执行 |
| Verilog生成 | 六个完整System、数值/控制专项与行/整机顶层已emit | 各自与C++使用同一final artifact |
| Verilog仿真 | NOT RUN，用户明确不要求 | 不再下载/安装RTL仿真器，不作为本次交付门槛 |
| 完整API/example gate、nightly矩阵 | NOT RUN | 完整入口包含RTL仿真；24个native suites与专项结果不能称为完整API gate |
| 全域精度/最大2048项有效列表 | NOT RUN | 不能从小规模测试或声明容量推断 |
| 综合/时序/面积/功耗/加速比 | NOT RUN | 无目标平台/PPA验收；P0T完整链路尚未集成 |

L0冻结文档保持原hash。L1/L2仍draft：接口和数值执行语义已具体化，但全域数值与物理
实现不因文档齐全而自动冻结。历史16-entry/map原型的链接结果不计入当前P0S。

编译器修改保留默认检查预算与原始DUT/Oracle，减少不可变属性、接口和作用域的重复
遍历及复制，并保留完整身份、类型、绑定、资源和事务校验。借用的endpoint仅在私有
证明中只读使用；持久owner仍拥有实际副本。实现、独立测试与评审分别记录。

最终工具链安装记录：`docs/gates/logs/fa2-engine-p0s/borrowed-normalization-install.json`。
整机结果：`docs/gates/logs/fa2-p0-completion/installed-system-gates-20261103T131546.527419Z/`，
包含命令/退出码、12份workers运行记录、同一final的双目标产物清单，以及前后相同的
源码/安装树hash。生命周期final的SHA256为
`5b60ae0464724eaad30835c2a27688be9ba4b60f4ff44887efb76b0838a735d6`。

数值结果：`final-numeric-20261103T131535.997341Z/`；四态guard结果：
`guard-order/20261103T211610-632235751/`；资源结果：
`final-support-20261103T131808.241768Z/`，均位于`docs/gates/logs/fa2-p0-completion/`。
资源启动时的Python/工具路径配置失败保留在此前两次support记录中；修正环境后沿用
原测试和独立Oracle通过，没有删除断言或替换DUT结果。

通用native gate首次记录在`final-native-gates-borrowed/`，两项测试修正及完整复验在
`native-gate-test-repairs/`。队列测试增加raw字段的实际断言，保留原capture/别名/方向
检查；观测测试改用有稳定生命周期的数组，保留全部三种观测与provenance检查。
这些修正未改变编译器或已安装工具链，故上述硬件运行证据仍对应最终代码。
Unicode版本不匹配记录在`final-native-gates-borrowed/identifier-environment-diagnosis.json`；
没有修改固定版本或Oracle来制造通过结果，也不把23项通过称为整套native gate通过。

## P0T 启动与 P0S 回归

P0S C++ 检查通过的是功能及周期行为，尚未完成整机吞吐、外部带宽、利用率和加速比
验收。上述固定终止 epoch 是各 System 的测试日程，不是 attention 命令的实测延迟。

新增 `fa2_array_interfaces.py` 和 `fa2_array.py`，实现 16 个 Row、256 个独立 PE、
非融合 FP32 编码乘加及 begin/step/end/take/abort 生命周期。接口与方法语义见
[ARRAY-1002](ndf/l2/fa2-core-array-1002.md)，测试口径见
[ARRAY-1003](ndf/l3/fa2-core-array-1003.md)。L0 两个文档未修改。

为在 callable 内复用算术，`fa2_numeric.py` 将原 add/mul 和 widen/shift 的表达式
提取为 typed pure helper，原 Module 接口保留。独立 AST/源码审查通过，数值行为未改。
本次 numeric SHA256 为 `d1e48bb0c99fbc97e93eede5d9f91a3c6a708b3d38b621c6b3c46a240799ddc7`；
原始基线 hash 保留在历史证据，不能称本次源码与旧快照完全相同。

| 检查 | 本次结果 | 证据/边界 |
| --- | --- | --- |
| P0S numeric、exact、edges、exp4096 | PASS，C++ workers 1/2 | `fa2-p0t-start/p0s-numeric-regression-20261103T172532.497260Z/manifest.json` |
| 原六个完整 P0S System | PASS，C++ workers 1/2 | `fa2-p0t-start/p0s-regression-20261103T172532.554329Z/result.md`；同 final 双 emit |
| P0S product library、独立 row、controller21 | PASS | 同一回归目录；原35文件闭包与旧基线相比只有 numeric 改动 |
| 阵列独立 host oracle/日志校验器 | PASS，5 tests | Fraction 手算锚点与缺失事件、错误元素、错拍/重复输出负例；不等于DUT运行 |
| numeric/interfaces/array/bench 四个独立 source unit | PASS | `fa2-p0t-array/20261104T013007/commands.json` |
| 完整256PE System 默认 link | FAIL，事务分析预算超限 | requested 6144、used 1047468、limit 1048576、phase transaction plans |
| 阵列 final结构、C++/Verilog emit、C++ workers1/2 | NOT RUN | link 没有产生 verified final，不报告阵列仿真或周期通过 |
| Queue take+push、中途reset、X/Z故障 | NOT RUN | 首批日程未包含，延迟take不能替代资源背压 |
| 完整P0T attention、DMA/softmax集成、性能和PPA | NOT RUN | 独立阵列不等于完整加速器 |

表内路径均相对 `docs/gates/logs/`。失败运行的完整输入/安装树hash及阶段结果在
`fa2-p0t-array/20261104T013007/manifest.json`；所有182个记录文件在该次运行前后一致。
只读诊断位于同目录 `compiler-readonly-diagnosis.json`：事务计划对所有已发现owner
反复复制快照存在通用开销；具体失败debit位置未被运行时定位。后续编译器优化和
阵列重试必须记录新候选，不能把本次失败改写为通过，也不提高默认预算。

## 通用快照优化与阵列重试

`SourceTransactionStateAnalysis.cpp` 去掉嵌套call与invocation之间重复的入口快照
复制：callee在同步调用期间只读借用caller拥有的快照，root仍自己保存；新增owner
仍按Q0取值，分支恢复、所有类型/身份/事务检查和默认1,048,576预算保持不变。
这是编译器通用优化，未按阵列名称、尺寸或用例进行特判。

独立测试在修改前后各通过58项，0/8/32个无关owner的三个完整事务计划逐字相同，
没有排除字段；候选完整 `ACIRTransactionEffectsTests` 为302/302 PASS。
没有测得analysis-work或运行速度的前后差值，不把静态减少复制量称为实测加速。
证据位于 `docs/gates/logs/fa2-p0t-link-snapshot/` 的
`snapshot-candidate.json`、`snapshot-full-candidate.json` 和独立审查记录。

候选工具链安装到 `.pycircuit_out/fa2-p0t-snapshot-install`，原安装保留。
安装记录为 `fa2-p0t-link-snapshot/install.json`。原numeric与controller的host runner
仅修改INSTALL选择以遵循 `PYC_TOOLCHAIN_ROOT`，默认路径、输入和oracle均保留；
这两项脚本变更在 `runner-install-selection.json` 单列，未冒称旧快照完全不变。

候选工具链上的P0S再次回归PASS：六个完整System的workers1/2、product库、独立row
和controller21用例通过，四组数值System的workers1/2也通过。记录在同一证据根下
`p0s-regression-20261103T174640.578343Z/result.json` 和
`p0s-numeric-regression-20261103T174640.515230Z/manifest.json`；源码/安装树前后一致。
八份P0S final产物与快照优化前逐字相同，比较记录为回归目录中的
`baseline-final-artifact-equality.json`。
本次仍未执行Verilog仿真或完整API/example/nightly矩阵，不用302项专项替代完整gate。

| 阵列重试 | 结果 | 证据 |
| --- | --- | --- |
| 快照优化后，原始四个unit与原235条指令 | FAIL，transaction plans预算 | `fa2-p0t-link-snapshot/array-link.json`；requested1、used1048576 |
| 相同输入改为14组向量常量表 | 输入等价审查PASS、四个unit compile PASS、link FAIL | `fa2-p0t-link-snapshot/operand-pool-independent-audit.json` 和 `fa2-p0t-array/20261104T014911/manifest.json`；requested1024、used1048429 |

向量常量表只共享重复输入，保留235条指令、全部mask/非法调用/结果期望和256个PE。
最后一次运行的182个输入/工具文件hash前后一致。所有阵列尝试均未生成verified final，
因此C++/Verilog emit、最终256个owner检查、阵列C++执行及实际II/吞吐仍为NOT RUN。
K+2延迟和K+3占用仍是设计合同，不能写成已测量结果。

下一项编译器工作需进一步减少通用事务计划的全owner快照/分支恢复开销，验证新的
表示对Q0、两个分支、嵌套调用和全局零提交的保持；不能只提高预算。其后才能执行
现有完整阵列日程，再补齐Queue/reset/X/Z和完整attention集成。
