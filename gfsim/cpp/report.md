# C++ 核心验收与性能报告（历史记录）

以下记录上一版引擎的测试，未覆盖现行 spec 的动态 dirty 方案。本轮不迁移／测试 C++；旧 Python examples 与跨语言脚本、CMake 注册已移除，原生测试保留。

验证日期：2026-10-01。环境：aarch64，Clang 22.1.8，Python 3.11.16；独立共享库及外部消费另用 GCC 10.3.1 验证。核心不链接 Python 或 LLVM。

## 验收

| 检查 | 结果与内容 |
| --- | --- |
| Release CTest | 四组通过：reference-circuits、native-components-and-boundaries、benchmark-equivalence、install-and-link |
| Debug + ASan/UBSan | 四组通过；保留 Debug 符号，采用 `-O1`、`-fsanitize=address,undefined`，开启 LeakSanitizer，错误立即终止 |
| 独立核心构建 | `BUILD_TESTING=OFF`、`GFSIM_BUILD_EXAMPLES=OFF`；不查找 Python；共享库构建、安装、外部链接和执行通过 |
| 安装可迁移性 | 安装后迁移 prefix，独立工程通过 `find_package(gfsim CONFIG REQUIRED)` 和 `gfsim::gfsim` 链接运行 |
| 地址与生命周期 | 任务数组、Queue 读者数组、proposal 槽位地址保持稳定；Work/Xfer 异常和动态环之后拒绝继续 |

LeakSanitizer 在受 ptrace 限制的沙箱中无法完成退出检查，因此正式内存验收在已批准的沙箱外执行，保持 `detect_leaks=1`，未通过关闭泄漏检查绕过。完全无优化的长链读者扫描超过初始时限；正式 sanitizer 配置使用 Debug + `-O1`，保留完整 1100 级双方向排空测试。

## 完整模型与参考

历史 `tests/compare.py`（已移除） 复用 Python experiment 的七组模型、原始输入及输出断言。13 项测试内的所有子场景分别运行缓存开启/关闭的 C++ 模型，逐拍与独立 Python Reference 对照：

- 获准 RuleId 集合及无重复 firing。
- 所有 Queue 的逻辑内容与版本。
- 保留重复记录的完整未来事件集合。
- Module Work 累计次数与每个 Module 的有效订阅集合。
- 缓存开启模式另外比较 Python engine 的每 Rule Work 次数与缓存命中计数。

独立参考使用重新描述的组件事务、前向读集合、固定点许可和 Kahn 动态环判定；不复用 C++ DFS、存储、缓存或 Xfer。Python 原有输出断言核对数据值、每通路顺序、完成性、存储最终状态；这些断言与逐拍全状态等价组合，覆盖 C++ 的端到端结果。

| 组 | 主要验证 |
| --- | --- |
| 流水 | 延迟/吞吐、满背压、跨 tick 不 Work 的直接仲裁、重计算缓存复用、多元素环形 FIFO 回绕和排空、固定种子随机刺激 |
| 包处理 | Module 分支与 Rule 分支、旧候选取消、丢包的 pop-only firing、独立通路与优先合流 |
| 双输入 | 必要读取失败时取消部分 pop/push 和事件、多输出原子性、事件以实际获准 tick 起算 |
| 分 bank 存储 | 动态地址、表项字段修改、服务延迟、跨 Rule revise/pop、响应和值的 scoreboard、最终存储；固定种子随机刺激 |
| 反馈 | 有空间和退出分支下的静态环、自身 pop/push、真实动态容量环及实例终止 |
| 查表 | 输出阻塞期间控制参数和实际依赖变化、动态下标、旧订阅失效、缓存订阅重登 |
| 重试 | 整值 revise、revise/pop/push 顺序、相同 payload 的新元素身份和版本推进 |

第 14 项原有场景以 [原生长链测试](examples/pipeline/test.cpp) 验收：1100 级满流水正向/反向 Module 顺序完整排空；正向 DFS 最大栈深度超过 1000，输出序列和值全部验证。长链使用明确 scoreboard，不再运行规模相同的 Python 参考。

[额外原生电路](tests/native.cpp) 连接两个 Source、带独立 Rule 的普通 Module、寄存器和 Sink。Module 在已准备第一条 Rule 后遇到必要控制读取为空，第一条 Rule 仍正确提交；后续控制输入到达后执行第二条 Rule，修改嵌套 bool、`std::array<int16_t,3>` 和 uint64 字段。最终值、无变化 revise 不继续唤醒、数组与来源槽位稳定性均有断言。

窄边界检查通过测试友元注入接近 uint64 上限的 tick、readGen、stateVersion；另外覆盖事件时间加法溢出、非法零延迟、重复 pop、遗漏 complete/abort、Work 异常、Xfer 赋值异常、无效果 Rule 不 firing，以及失败实例后续调用被拒绝。测试断言在 Release 中仍生效，不依赖 `assert`。

## 性能测量

命令：`gfsim-benchmark 1000 5 20000 512`。Clang Release `-O3 -DNDEBUG`，每类 1000 个计时 tick、5 次取中位数。每个模式每次重新构造，首拍初始化不计时；计时区只执行 `step()`，构造、独立等价验证、快照、事件复制及序列化均在区外。原始结果见 [results.json](results.json)。

| 负载 | 缓存开 ms | 缓存关 ms | 关/开时间比 | Rule Work 开/关 | 命中 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 16 级满流水 | 6.736 | 6.693 | 0.994 | 18000 / 18000 | 0 |
| 两级重计算背压，每级 20000 次迭代 | 4.336 | 62.934 | 14.513 | 2255 / 4151 | 1896 |
| 两 bank、每 bank 512 项稀疏表 | 1.275 | 1.271 | 0.996 | 3662 / 3662 | 0 |

每类负载先在独立的非计时运行中逐拍验证缓存开关的获准集合、数据、版本、事件、Module 激活和有效订阅一致；每次计时结束再核对最终 Queue 数据。计数器只统计计时窗口；完成输出数包含不计时的首拍。

重计算背压中的 1896 次命中跳过昂贵的数值循环，剩余 Rule Work 包括较便宜的 Source、Config 和 Sink，因此耗时收益不与总 Rule Work 的比例相同。本机该负载约 14.5 倍；这是特定业务计算量和背压条件下的测量，不是框架的普遍保证。满流水和稀疏表没有命中，测得时间基本一致，微小差异不足以说明收益。

固定窗口分别产生 1001、50、331 条输出，不宣称基准已经排空。三个模型的 Queue 读者数组分别占 2736、384、49632 字节，不包含元素、proposal、vector 容量或事件堆成本。

## 实现边界

本次交付为单线程、平级 Module 的源码级 C++20 核心。未接入编译器自动生成，未定义父子激活、Cell、来源 0 驱动、已发布事件取消、快照恢复或并行同步，也不提供跨版本二进制 ABI 保证。端口唯一来源仍是模型前提；没有加入竞争检测、额外仲裁策略或动态环联合提交。确认的 uint64 溢出终止策略已写入 [Q11](../open-questions.md#q11)，其余未决项保留。
