# GFSim C++20 后端框架

独立、可安装的单线程 C++20 库，导出 `gfsim::gfsim`，核心只依赖标准库。语义以 [spec](../spec.md) 为准。实现包括 Queue、Signal、参数缓存、实际读者位图、跨 tick 候选、显式栈容量 DFS 与统一 Xfer。

本目录公开模型是 [手写 Ripes5](examples/ripes5/README.md)。五个 Module 提供代码生成的目标样例；独立 [ACPy 编译器](../../pycircuit/README.md) 已用同一 GFSim 接口生成完整模型，并复用本目录的宿主 runner 和回归。已有 Python 实验及工作区设计清理保留。

## 构建、验收和安装

从仓库根目录执行：

```bash
cmake -S gfsim/cpp -B /tmp/gfsim-cpp-release -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/gfsim-cpp-release -j4
ctest --test-dir /tmp/gfsim-cpp-release --output-on-failure
cmake --install /tmp/gfsim-cpp-release --prefix /tmp/gfsim-install
cmake -S gfsim/cpp/tests/external -B /tmp/gfsim-consumer \
  -DCMAKE_PREFIX_PATH=/tmp/gfsim-install
cmake --build /tmp/gfsim-consumer
/tmp/gfsim-consumer/consumer
```

正式 CTest 包括 13 程序 × 缓存开关 × Module 正反序的三方逐拍验收。测试工具需要 Python 3；原生 Ripes 的固定版本和构建方法见 [参考说明](../experiment/examples/ripes5/README.md)。可用 `-DGFSIM_RIPES_REFERENCE=/absolute/path/ripes5-reference` 指定 runner。参考缺失或 Ripes/VSRTL commit 不符会失败，不会跳过或以 Python 替代。

只构建库使用 `-DBUILD_TESTING=OFF -DGFSIM_BUILD_EXAMPLES=OFF`，无需 Python、Qt、LLVM 或原生参考。`-DBUILD_SHARED_LIBS=ON` 可构建共享库。`install-and-link` 将安装 prefix 搬迁，再用独立工程查找、链接及执行，验证 CMake 包可迁移。

```bash
cmake -S gfsim/cpp -B /tmp/gfsim-cpp-clang-asan \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DGFSIM_SANITIZERS=ON
cmake --build /tmp/gfsim-cpp-clang-asan -j4
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir /tmp/gfsim-cpp-clang-asan --output-on-failure
```

Sanitizer 构建使用 `-O1 -g`，保留完整 1100 级链和三方验收。需要编译器配套的 sanitizer 库；本机 GCC 10 缺少该库，内存验收使用已安装的 Clang 22。LeakSanitizer 需要允许其进程检查的环境。

## 编译操作对应

| ACIR 信息或操作 | 生成的 C++ |
| --- | --- |
| Module 实例、选择与分支 | 普通 class、`Work()`；`addModule` 绑定实例指针和函数入口 |
| Rule 原子边界、参数 | `work_rule(args)`、独立 `ParameterCache<Args>`、`beginRule` |
| 正常完成／必要读取失败 | `completeRule`／显式检查 `tryPeek` 后 `abortRule`；`NeedInput` 捕获为兼容路径 |
| 持久寄存器、消息 FIFO | `Queue<T>(1, {initial}, true)`／`Queue<T>(capacity, initial)` |
| 实际读取 current | `peek/tryPeek/at/size/empty/full`，按当前上下文自动登记 |
| 消息消费、输出、状态修改 | 显式 `proposePop/proposePush/proposeRevise<Path...>` |
| 组合共享计算 | 纯 helper 与 `Signal<T>`；通过 `value()` 读取缓存 |
| 固定资源身份和访问范围 | `addQueue/addSignal/declareResource/declareInput/bind/freeze` |
| Queue array 动态索引 | 固定 Queue 引用数组，先取实际表项再调用普通接口 |
| 未来事件 | Rule 内 `requestWakeup(rid, target, delay)`；获准时发布 |
| Rule 仲裁入口 | `arbitrate_rule()` 调用 `sim.arbitrateRule(rid)` |

Module 可变组合值必须作为 args 传入 Rule。标量使用 bool、标准定宽整数；aggregate 使用 `std::array` 或嵌套值 struct，并生成字段式 `operator==`。没有 `std::any`、memcmp 或反射。业务中的定宽溢出须由 lowering 正确表达，例如 uint32 运算；runtime 不实现任意位宽整数。

读取保留原分支，未选路径不预读。消息输入实际读取后另行生成一次 pop；寄存器、Signal helper 和旁路观察不消费。输出容量交给仲裁，不用 current.full() 提前排除本应参加仲裁的生产者。

## 最小 Rule 与构造接口

完整独立链接测试见 [tests/external/main.cpp](tests/external/main.cpp)。Rule 的典型主体是：

```cpp
void work_transfer(std::uint32_t bias) {
    if (!sim.beginRule(rid, arguments, bias)) return;
    const auto* value = input.tryPeek();
    if (!value) {
        sim.abortRule(rid);
        return;
    }
    input.proposePop(rid);
    output.proposePush(rid, *value + bias);
    sim.completeRule(rid);
}
```

运行前依次注册 Module、Rule、Queue、Signal，声明每个 Module 的全部可能资源、每个 Signal 的全部 Queue 输入、每条 Rule 的 Signal 依赖，并绑定 Rule 的全部可能操作。静态声明必须覆盖所有分支和数组项；别名声明会合并。`bind` 不代替 Queue 的 `declareResource`。`declareInput(ruleId, signal)` 同时建立静态 Rule dirty 与所属 Module 激活关系；`declareResource(moduleId, signal)` 只激活 Module。Signal 的实际访问声明在 Debug 校验，Release 依赖构造保证完整性。

`freeze()` 机械建立来源槽位、局部资源表、资源到 Module 的固定链接与直接槽位映射、读者位图以及固定任务数组。来源 0 保留但没有外部提交接口。ModuleId、QueueId、SignalId 从 0 开始，RuleId 从 1 开始。pop/push 各自唯一来源仍为生成模型的前提，多来源竞争不在本版范围。

Simulator 保存非拥有引用。Queue/Signal 地址必须稳定且存活到 Simulator 析构完成；Module 实例及入口须在整个执行期有效。冻结后不得增加资源、改连线或移动实例。Simulator 析构解除资源的回调绑定；不定义重新注册、重启或状态恢复。嵌套 `step()` 被拒绝；执行上下文按线程隔离，但同一实例不支持并行执行。

## 读取、候选与提交

Queue 读取在 Module/Rule Work 中自动记录实际依赖；Signal `value()` 和 helper 内的 Queue 读取不登记动态依赖；外部 testbench 读取不订阅。未注册、其他 Simulator 或未声明的资源读取会报错。`tryPeek()` 对空队列返回空指针；必要 `peek()` 抛 `NeedInput`。生成代码应使用 `tryPeek()` 和显式分支处理正常缺输入：Module 控制读空直接返回，保留已准备的独立 Rule；Rule 先 abort，再返回。兼容的 `peek()` 调用若可能读空，仍必须在 Rule 内捕获 `NeedInput` 并 abort。

检查保留在实际操作及其分支内。没有前置非空证明的 pop／revise 也须先检查目标；输出满由仲裁处理。`abortRule()` 撤销整条候选的部分效果但保留实际读取，不能以简单 return 代替。Signal 必须显式计算空输入对应的值，其异常仍终止实例。ACPy 固定的生成契约和测试见 [编译器说明](../../pycircuit/README.md#gfsim-生成范式必要输入)。

`beginRule` 同 tick 去重。未 dirty 时比较类型化参数；完整且未 dirty 则复用，包含无效果候选。命中不扫描版本、不重登读取。重算清旧 proposal 和读者位；abort/提交保留读取，未再选中的 Rule 清候选、读取及 dirty。纯 push 不依赖输出 current；pop/revise 自动记录目标依赖。

一个 tick 先完成全部 Module Work，再固定候选进行显式栈 DFS。只有满 Queue 上实际必需的容量依赖才递归访问消费者。先检查全部 participants，再整体 accept；pop 只通知已有完整候选，不运行 Work。静态环可运行，实际容量环使实例终止。事件延迟从整体获准 tick 起算。

所有 Queue 按 revise → pop → push 提交。按值保存的字段修改只在 Xfer 赋给旧队尾，不重读 Queue 或捕获局部引用：

```cpp
queue.proposeRevise<&Entry::meta, &Meta::epoch>(rid, next_epoch);
queue.proposeRevise<&Entry::value>(rid, next_value);
queue.proposeRevise<>(rid, replacement);
```

current 的 const 指针/引用不得跨 Xfer 保存。pop 后 push 相同 payload 仍推进元素版本；最终无变化 revise 不推进。字段路径不支持 bit-field、动态数组字段下标或指针解引用；动态 Queue array 索引是另一种操作。

Queue 变化立即标记实际 Rule 读者 dirty，并登记下一 tick 事件。任一声明输入 Queue 变化都会将关联 Signal 去重入队，包括未选分支。全部 Queue 提交后，每个 Signal 每轮至多求值一次。只有返回值改变才按固定 Rule 掩码标脏，并激活静态关联 Module；提交、取消或未读分支均不删除连接。helper 只读取声明的 Queue 和固定配置；跨 Signal、proposal、事件请求均被拒绝。任意未登记宿主状态的纯性仍由生成器保证。

## 布局、观察与边界

Queue 是固定容量环形 `vector<optional<T>>`，来源槽位按该 Queue 的绑定数分配，并二分定位。任务 ID 数组、访问代号及位图在 freeze 后固定；实际 participants/readSlots/事件请求使用可复用 vector。字段赋值捕获使用 `std::function`；事件使用最小堆，允许标准库动态分配。

每 Module 的可访问 Queue 数为 A、Rule 数为 R，读者位图占 A×ceil(R/64) 个 uint64，控制读取占 A 个代号。每资源另有 ModuleCount 个槽位映射项；可能读者链接只含静态声明的连接。Signal 的输入 ID、Queue 反向链接及静态下游关系按声明分配；每个有 Rule 绑定的下游 Module 保存 ceil(R/64) 个掩码字，Module-only 掩码为空，无 QueueCount×SignalCount 全量表。

缓存命中没有依赖扫描；读取登记直接定位位，清理只遍历上次实际 readSlots，通知遍历可能读者及其 Rule 字。`step()` 返回的获准 Rule span 在下一 step 前有效。`module/rule/stats/reads/events`、Queue 版本及 Signal 求值次数供 testbench 只读观察。`reads(module, signal)` 查询静态激活关系，`reads(module, queue)` 仍查询动态读取。Release 的 Signal 输入读取没有二分查找或依赖登记；Debug 保留声明检查。

tick、读代号、状态版本和计数使用 uint64，溢出报错而不回绕。Work、helper、仲裁、Xfer 异常及动态容量环终止实例，后续执行拒绝继续，不承诺回滚。窄边界、异常、延迟值捕获、静态存储地址和资源析构检查见 [tests](tests/native.cpp)。

## 验证、性能和阅读顺序

建议按 [logic.hpp](examples/ripes5/logic.hpp) → [stages.hpp](examples/ripes5/stages.hpp) → [model.hpp](examples/ripes5/model.hpp) → [runner.cpp](examples/ripes5/runner.cpp) 阅读：分别是纯组合逻辑、五阶段业务、静态连接、宿主 testbench。调度能力补充电路位于 `tests/circuits`，边界断言位于 `tests/semantics.cpp`。

[验收报告](report.md) 包含 Release、ASan/UBSan、安装链接、52 配置结果及核心/模型/工具/测试代码量。Ripes5 的 [benchmark](examples/ripes5/bench.py) 使用三个既有程序、同一 CPU、串行子进程、一次预热和七次轮换采样；排除构造、轨迹与 JSON，保留调度计数，不预设速度比。

尚未实现父子 Module 激活、独立 Cell、来源 0 驱动、事件取消、快照恢复或并行调度；源码级接口不承诺稳定二进制 ABI。语义缺口保留在 [open-questions](../open-questions.md)。

Signal 静态依赖调整会改变旧版本的求值和激活计数。验收仍要求架构参考及各配置逐拍一致，见 [本次验证与同核对照](signal-static-report.md)。
