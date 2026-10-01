# GFSim C++20

独立的单线程电路仿真库，导出 CMake 目标 `gfsim::gfsim`。调度语义以 [spec](../spec.md) 为准。核心不依赖 Python、LLVM、MLIR 或第三方测试框架；Python 3.11 仅用于独立参考验收。

## 构建、测试与安装

从仓库根目录执行：

```bash
cmake -S gfsim/cpp -B /tmp/gfsim-release -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/gfsim-release -j3
ctest --test-dir /tmp/gfsim-release --output-on-failure
cmake --install /tmp/gfsim-release --prefix /tmp/gfsim-install
cmake -S gfsim/cpp/examples/external -B /tmp/gfsim-consumer \
  -DCMAKE_PREFIX_PATH=/tmp/gfsim-install
cmake --build /tmp/gfsim-consumer
/tmp/gfsim-consumer/consumer
```

只构建库时设置 `-DBUILD_TESTING=OFF -DGFSIM_BUILD_EXAMPLES=OFF`，完全不查找 Python。保留原生测试但跳过参考测试时设置 `-DGFSIM_REFERENCE_TESTS=OFF`。常规完整验收需要保留参考测试；缺少 Python 会在配置时明确失败。

内存检查使用 GCC 或 Clang，Debug 配置保留符号并为 sanitizer 测试采用 `-O1`，避免 1100 级流水的读者扫描在完全无优化时耗时过长：

```bash
cmake -S gfsim/cpp -B /tmp/gfsim-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++ -DGFSIM_SANITIZERS=ON
cmake --build /tmp/gfsim-asan -j3
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir /tmp/gfsim-asan --output-on-failure
```

LeakSanitizer 需要不受 ptrace 限制的运行环境。`install-and-link` 测试将安装目录迁移到新路径后，以独立 CMake 工程查找、链接并运行生成式 Module，验证导出包不依赖源码或原构建目录。

## 完整生成式 Module

以下代码也是验收中的 [外部链接示例](examples/external/main.cpp)。Module 是普通 class，Rule 是成员函数；参数缓存保存在生成类中，调度记录由 Simulator 保存。Work 返回不表示提交。

```cpp
#include <cstdint>
#include <gfsim/queue.hpp>

// Ordinary generated class: business methods and scheduling records are separate.
class Increment {
  public:
    gfsim::Simulator &sim;
    gfsim::Queue<std::uint32_t> &input;
    gfsim::Queue<std::uint32_t> &output;
    gfsim::ModuleId mid{};
    gfsim::RuleId rid{};
    gfsim::ParameterCache<std::uint32_t> arguments;

    void Work() { workIncrement(7); }
    void workIncrement(std::uint32_t bias) {
        if (!sim.beginRule(rid, arguments, bias))
            return;
        sim.recordRead(mid, input, rid);
        const auto *value = input.tryPeek();
        if (!value) {
            sim.abortRule(rid);
            return;
        }
        input.proposePop(rid);
        output.proposePush(rid, *value + bias);
        sim.completeRule(rid);
    }
    bool arbitrate() { return sim.arbitrateRule(rid); }
};
int main() {
    gfsim::Queue<std::uint32_t> input(2, {10, 20}), output(2);
    gfsim::Simulator sim;
    Increment module{sim, input, output};
    module.mid = sim.addModule<&Increment::Work>(module);
    module.rid = sim.addRule(module.mid, [](void *object, gfsim::Simulator &, gfsim::RuleId) {
        return static_cast<Increment *>(object)->arbitrate();
    });
    sim.addQueue(input);
    sim.addQueue(output);
    sim.bind(module.rid, input, gfsim::Pop);
    sim.bind(module.rid, output, gfsim::Push);
    sim.freeze();
    sim.step();
    sim.step();
    return output.size() == 2 && output.at(0) == 17 && output.at(1) == 27 ? 0 : 1;
}
```

`addModule`、`addRule`、`addQueue`、`bind` 在运行前建立 ID 表和所有可能分支的资源绑定；`freeze()` 分配读者、任务及槽位数组，关闭构造。ModuleId/QueueId 从 0 开始，RuleId 从 1 开始。每个 Queue 留有来源 0 槽位，但没有外部提交接口。模型必须保证唯一 pop/push 来源；绑定检查操作声明，不检测或仲裁多个来源的端口竞争。

Simulator 不拥有 Module 或 Queue，调用方必须保持实例地址稳定，并保证其生命周期覆盖整个仿真。可以像七组例子一样通过 `unique_ptr` 保存 Queue、通过独立对象保存 Module。冻结后不移动 Queue/Simulator，不重新绑定。业务方法和库之间用实例指针与普通函数入口连接，没有运行时函数体分析。

每个有参数的 Rule 使用独立 `ParameterCache<Args>`；`Args` 可以是整数、bool、`std::array` 或嵌套 struct，使用 `operator==` 按值比较。成员指针字段路径和参数缓存完全静态定型，没有 `std::any`、字节比较或统一动态值。无参数 Rule 使用 `beginRule(rid)`。同 tick 同 Rule 的重复调用仅准备一次，参数一致是模型前提。

`recordRead(mid, queue, rid)` 登记 Rule 的实际读取及 Module 订阅，省略 rid 表示 Module 控制读取。`tryPeek()`、`peek()`、`at()`、`size()`、`empty()`、`full()` 本身只是只读 current，不自动登记。必要读取为空时，生成代码必须 `abortRule` 并返回；不能把空 `peek()` 抛出的逻辑错误当成普通控制流。Module 中途必要读取失败直接返回，先前完成的独立 Rule 保留。消息 payload 的读取与 pop 分开生成，重复读取不应重复 proposePop。

`beginRule` 返回 false 表示同 tick 已调用或完整候选缓存命中。命中会重登 deps 订阅。返回 true 后必须以 `completeRule` 或 `abortRule` 结束；发生部分 proposal 后 abort 会清理全部 participants 和未发布事件。空效果 complete 不 firing。旧候选没有重新 Work 时，合法 pop 的容量通知可以直接安排其仲裁，无需重新计算。

整条 Rule 的所有 Queue 检查通过后才确认并发布事件。`requestWakeup(rid, mid, delay)` 要求正延迟，以获准 tick 为起点；可构造纯事件 Rule。调用方通过 `step()` 决定推进多少拍，业务完成条件在调用方判断。

## 字段修改与 current

`Queue<T>` 使用固定容量 `vector<optional<T>>` 环形 FIFO，读接口仅返回 const 元素；`at(i)` 按当前逻辑顺序观察元素。current 的指针和引用不能跨 Xfer 保存，需要保留的组合值应按值复制。Work 始终读取旧状态，Xfer 统一执行所有 revise、再 pop、再 push。pop/push 相同 payload 仍推进版本；无变化 revise 不推进。寄存器使用 `Queue<T>(1, {initial}, true)`，此模式只允许 revise。

```cpp
struct Meta {
    bool valid{};
    std::array<std::int16_t, 3> lanes{};
    bool operator==(const Meta&) const = default;
};
struct Entry {
    Meta meta{};
    std::uint64_t count{};
    bool operator==(const Entry&) const = default;
};
// 位于成功 beginRule 和已检查非空的生成式 Rule 中：
queue.proposeRevise<&Entry::meta, &Meta::valid>(rid, true);
queue.proposeRevise<&Entry::meta, &Meta::lanes>(rid, std::array<std::int16_t, 3>{1, -2, 3});
queue.proposeRevise<&Entry::count>(rid, std::uint64_t{9});
queue.proposeRevise<>(rid, replacement); // 整值替换，也支持标量
```

字段动作通过 `std::function` 按目标字段类型保存已计算的值，Xfer 时赋到旧队尾并判断变化，不重读输入，不捕获 Work 局部引用。动态 Queue array 下标先定位实际 Queue；不是动态 struct 字段路径。整数使用标准 C++ 宽度，示例业务显式使用无符号运算或掩码；库不提供 AC 任意位宽类型。

## 生命周期、存储与观察

| 记录 | 存储和生命周期 |
| --- | --- |
| Queue current | 固定容量环形数组；元素与 proposal 都强类型，pop/push O(1) |
| Queue 来源槽位 | 只按该 Queue 实际绑定的来源分配，另含来源 0；freeze 后地址固定，二分查询 |
| Queue readers | 每个 Queue 固定 `ModuleCount` 个 uint64；新 Work 覆盖读取代号，旧代号失效，无历史列表 |
| Module | 固定 ID 表、实例和 Work 入口；selected/previous vector 循环复用；readGen 仅在 Work 后推进 |
| Rule | 固定 owner/仲裁入口，实际 deps/participants/wakeRequests 使用可复用 vector；deps 线性去重 |
| 参数 | 生成类内的 `ParameterCache<Args>` 保存候选参数；complete=false 时旧参数值不参与缓存有效性判断 |
| 任务/DFS | 预分配 ID 数组、tick 标记和显式 DFS 栈；队列、栈及获准列表容量复用 |
| 事件 | `(wakeTick, ModuleId)` 最小堆；同 tick Module 到期去重，已发布事件独立于候选 |

读者数组主要成本为 `QueueCount × ModuleCount × 8` 字节。来源槽位与强类型 payload 成本取决于各 Queue 的绑定数；不按全局 RuleCount 扩展。deps/participants 的线性去重可能产生 O(d²)/O(p²) 准备成本；容量 DFS 按实际 participants 遍历。状态通知只扫描实际改变的 Queue 的读者数组。`std::function`、vector 和事件堆允许标准库分配，不实现专用内存池。

`step()` 返回的 `span<const RuleId>` 在下一次 step 前有效，顺序是仲裁顺序，不是无依赖 Rule 的额外契约。`module()`、`rule()`、`stats()`、`Queue::stateVersion/readers` 提供只读观察；`events()` 返回排序后的副本。观测序列化只存在于 examples，不进入核心或基准计时区。

tick、版本、读代号、任务标记和工作计数采用 uint64。tick 最大值无法再推进时、计数递增或事件时间相加溢出时，抛出 `overflow_error`，禁止回绕。Work、仲裁、Xfer 异常或实际容量动态环错误都会将实例标为 failed，后续 step/执行接口拒绝继续；失败不提供回滚、恢复或可提交快照。`TestAccess` 仅供窄边界测试注入接近溢出的值，不是模型操作接口。

## 完整电路验收

七组 C++ 组件和连接分别位于：

| 模型 | C++ 组件与连接 | 复用的激励、验收与独立参考 |
| --- | --- | --- |
| 弹性流水 | [pipeline/model.hpp](examples/pipeline/model.hpp) | [pipeline](../experiment/examples/pipeline/test_model.py) |
| 包处理 | [packets/model.hpp](examples/packets/model.hpp) | [packets](../experiment/examples/packets/test_model.py) |
| 双输入原子处理 | [pairs/model.hpp](examples/pairs/model.hpp) | [pairs](../experiment/examples/pairs/test_model.py) |
| 分 bank 存储 | [memory/model.hpp](examples/memory/model.hpp) | [memory](../experiment/examples/memory/test_model.py) |
| 反馈 | [feedback/model.hpp](examples/feedback/model.hpp) | [feedback](../experiment/examples/feedback/test_model.py) |
| 在线查表 | [lookup/model.hpp](examples/lookup/model.hpp) | [lookup](../experiment/examples/lookup/test_model.py) |
| 重试缓冲 | [retry/model.hpp](examples/retry/model.hpp) | [retry](../experiment/examples/retry/test_model.py) |

[compare.py](tests/compare.py) 复用现有 13 项电路测试的同一份输入和独立输出断言，将输入通过文本协议传给 C++ runner；每拍比较获准集合、全部 Queue 内容与版本、事件、Module 激活次数及有效订阅。参考模型通过独立事务描述与固定点许可实现，不调用 C++ 或 Python 被测组件函数。缓存开关两种 C++ 执行均逐拍对照；缓存开启时另与 Python engine 比较 Rule Work 次数和缓存命中数，确保复用不是仅靠最终输出推断。

第 14 项 [1100 级满流水](examples/pipeline/test.cpp) 在原生测试中完整排空，正向与反向 Module 顺序均检查输出序列，正向深度要求超过 1000。其余随机流水和存储沿用固定种子 0–5；存储主场景使用种子 41。控制分支、动态下标、部分 proposal 取消、跨 tick 直接仲裁、缓存订阅重登、事件原子性及动态环均纳入上述完整模型。

额外 [原生组件测试](tests/native.cpp) 连接 Source、包含独立 Rule 的 Module 和 Sink，验证 Module 中途读空保留此前 Rule，以及 bool、标准整数、array、嵌套字段修改和无变化 revise。窄边界覆盖计数器溢出、Work/Xfer 异常、缺失 complete/abort、重复 pop、非法延迟和异常后拒绝继续。任务、读者数组及 proposal 槽位地址也受稳定性检查。

## 性能基准

```bash
/tmp/gfsim-release/gfsim-benchmark 1000 5 20000 512
# 参数：计时 ticks、重复次数、每级计算迭代数、每 bank 表深度
```

三类负载为满流水、重计算背压和稀疏 Queue array。每类先在计时区外逐拍验证缓存开关的获准集合、数据、版本、事件、激活和订阅一致，然后独立构造每次测量；构造、首拍初始化、观察序列化都不计时。计时区只调用 step，报告多次中位数、工作计数、完成输出数和读者数组成本。默认 1000 拍是固定窗口，不声称基准窗口内已经排空。

结果见 [results.json](results.json) 和 [验收报告](report.md)。缓存关闭只禁止 Module 再次调用时复用业务计算；未激活 Module 的完整 Pending 仍遵守跨 tick 直接仲裁契约。满流水和稀疏存储可能没有缓存命中，速度比接近 1 或有波动；收益不预设下限。

## 当前边界

实现平级、单线程 Module 调度。编译器接入、父子 Module 激活、独立 Cell、外部来源 0 协议、已发布事件取消/覆盖、快照恢复及并行执行仍未实现。当前公开的是可编译链接的 C++20 源码接口，不承诺跨版本二进制 ABI。待决语义保留在 [open-questions](../open-questions.md)。
