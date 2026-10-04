# GFSim C++20 后端框架

独立、可安装的单线程 C++20 库，导出 `gfsim::gfsim`，核心只依赖标准库。语义以 [spec](../spec.md) 为准。实现包括 Queue、Signal、固定资源激活图、跨 tick pending、delta 容量仲裁与统一 Xfer。

本目录公开模型是 [手写 Ripes5](examples/ripes5/README.md)。五个 Module 提供代码生成的目标样例；独立 [ACPy 编译器](../../pycircuit/README.md) 已用同一 GFSim 接口生成完整模型，并复用本目录的宿主 runner 和回归。Python 模型作为 Ripes5 的独立轨迹参考保留。

## 构建、验收和安装

从仓库根目录执行：

```bash
cmake -S gfsim/cpp -B reference/builds/gfsim-release -DCMAKE_BUILD_TYPE=Release
cmake --build reference/builds/gfsim-release -j4
ctest --test-dir reference/builds/gfsim-release --output-on-failure
cmake --install reference/builds/gfsim-release --prefix reference/builds/gfsim-install
cmake -S gfsim/cpp/tests/external -B reference/builds/gfsim-consumer \
  -DCMAKE_PREFIX_PATH=reference/builds/gfsim-install
cmake --build reference/builds/gfsim-consumer
reference/builds/gfsim-consumer/consumer
```

正式 CTest 包括 13 程序 × Module 正反序的三方逐拍验收。测试工具需要 Python 3；原生 Ripes 的固定版本和构建方法见 [参考说明](../experiment/examples/ripes5/README.md)。可用 `-DGFSIM_RIPES_REFERENCE=/absolute/path/ripes5-reference` 指定 runner。参考缺失或 Ripes/VSRTL commit 不符会失败，不会跳过或以 Python 替代。

只构建库使用 `-DBUILD_TESTING=OFF -DGFSIM_BUILD_EXAMPLES=OFF`，无需 Python、Qt、LLVM 或原生参考。`-DBUILD_SHARED_LIBS=ON` 可构建共享库。`install-and-link` 将安装 prefix 搬迁，再用独立工程查找、链接及执行，验证 CMake 包可迁移。

```bash
cmake -S gfsim/cpp -B reference/builds/gfsim-asan \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DGFSIM_SANITIZERS=ON
cmake --build reference/builds/gfsim-asan -j4
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir reference/builds/gfsim-asan --output-on-failure
```

Sanitizer 构建使用 `-O1 -g`，保留完整 1100 级链和三方验收。需要编译器配套的 sanitizer 库；本机 GCC 10 缺少该库，内存验收使用已安装的 Clang 22。LeakSanitizer 需要允许其进程检查的环境。

## 静态接口与执行

| ACIR 信息或操作 | 生成的 C++ |
| --- | --- |
| Module | 普通 class 和 `Work()`，`addModule` 绑定实例与入口 |
| Rule | `work_rule(args)`、`addRule(owner)`、`beginRule(rid)` |
| 正常完成／必要读取失败 | `completeRule`／分支内非空检查后 `abortRule` |
| 资源和固定连接 | `addQueue/addSignal/declareResource/declareInput/bind/freeze` |
| 读取 current | `peek/tryPeek/at/size/empty/full`，不登记运行时依赖 |
| 消费、输出、状态修改 | `proposePop/proposePush/proposeRevise` |
| 字段或动态下标更新 | 成员指针 revise 或按值捕获的 `proposeReviseWith` |
| 共享组合值 | 纯 helper 与 `Signal<T>::value()` |
| 未来事件 | `requestWakeup(rid, target, delay)`，整体获准时发布 |

完整契约见 [spec](../spec.md)。独立使用示例见 [external/main.cpp](tests/external/main.cpp)：

```cpp
void work_transfer(std::uint32_t bias) {
    if (!sim.beginRule(rid)) return;
    const auto* value = input.tryPeek();
    if (!value) { sim.abortRule(rid); return; }
    input.proposePop(rid);
    output.proposePush(rid, *value + bias);
    sim.completeRule(rid);
}
```

`declareResource(module, queue_or_signal)` 建立固定激活关系，覆盖所有分支和动态下标；`bind` 自动包含目标 Queue→owner Module。`declareInput(signal, queue_or_signal)` 建立 Signal 输入。声明先追加，freeze 时排序去重并建立 Signal 拓扑序，Signal 组合环报错。没有 Rule→Signal dirty 接口、参数缓存、动态读取表或自定义 Rule 仲裁回调；Simulator 默认构造，模型构造函数的最后一个可选参数只剩 `reverse`。

Module 每次激活时清除所属旧 proposal，重新选择、执行 Rule；没有激活的 Module 保留完整 pending。Work 全部结束后进行原子仲裁，获准 pop 沿固定容量边将生产者的 pending 加入下一 delta。只重仲裁，不重跑 Work。Rule 容量图不做 DFS、静态环检查或环求解；不前进的 pending 由宿主周期上限暴露。

全部仲裁结束后统一 Xfer，再按固定拓扑序求受影响 Signal。输出变化通知下游 Signal，在同一轮继续传播；菱形汇合只计算一次，不增加流水拍。首次 Work 前也按此序初始化全部 Signal。Queue 状态或 Signal 值变化直接加入下一拍 Module 激活集合。事件堆只保存显式请求，延迟仍从整体获准 tick 起算。Signal 输出不变、no-op revise 均不通知。

Queue 使用环形 vector 存储，来源槽位在 freeze 后固定。freeze 同时建立 RuleId→槽位的只读索引，执行时直接访问，不再二分查找；同来源集合的 Queue 共用索引表，表只覆盖首个到末个非零来源的 ID 区间。字段 revise 保持按值捕获。去重任务缓冲在 freeze 预留空间，participants、事件请求和事件堆仍可分配。核心不使用资源×Module 稠密映射。

Queue/Signal 须保持地址稳定，并活到 Simulator 析构结束；构造后不可改连线。模型须保证 pop/push 各自唯一来源、声明完整、Signal 纯性及同 tick 同 Rule 重复调用参数一致。读取接口不逐次验证静态声明；非法 proposal 上下文仍报错。任何执行异常终止该实例，不承诺回滚或恢复。

## 验证与性能

CTest 包含静态激活、跨拍 pending、delta 容量链、部分效果撤销、分支替换／未选中清理、Signal 拓扑初始化与 Xfer 屏障、菱形汇合、值过滤与组合环、显式事件、1100 级满链、Module 正反序及独立安装链接。Ripes5 采用 13 程序 × 两种 Module 顺序，Python 历史引擎只作为架构轨迹参考。

完整 CPU 的验收和固定周期 benchmark 入口见 [Ripes5](../../pycircuit/examples/ripes5/README.md) 与 [skyzh OoO](../../pycircuit/examples/skyzh_ooo/README.md)。输出统一写入根目录 `reference/`；旧性能快照和报告已删除。
