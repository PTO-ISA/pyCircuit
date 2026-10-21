# callable after 向外层事务传播的对照复现

[dut.py](dut.py) 保留同一个 Counter：count 初值为 5，increment 在 count < 7
时加一；read 声明 `after=(increment,)`。Counter 内部不注册这两个 Rule。
[verify.py](verify.py) 保存独立预期并通过公共 compile/link/emit 路径运行
生成的 C++ workers 1/2 与 RTL。

## 当前验收预期

| 入口 | 外层 reader 装饰器 | 四拍读数 |
| --- | --- | --- |
| `CalleeAfterOnly` | `@ac.rule` | 6、7、7、7 |
| `CallerAfter` | `@ac.rule(after=(writer,))` | 6、7、7、7 |

声明的 increment → read 按同一个 counter 实例传播成 writer → reader。
显式外层 after 是重复约束，两个入口应一致。源码注册顺序本身不建立优先级。
前递使用整个前驱事务的 fire 与对应状态的写意图，后继的 guard 和计算均看到
有效候选；前驱不 fire 时，后继仍可读取旧值。Q 在全系统检查通过后才提交。

2026-10-21 使用此隔离工作树构建并安装的候选工具链执行通过：两个入口均为
6、7、7、7，生成的 C++ workers 1/2 与 RTL 一致，每个入口 8 次 Work 观测。
本地证据位于 `docs/gates/logs/issue347/after-call-order-accepted/results.json`，
同目录的 commands.json 和逐命令日志记录完整编译、链接、生成及运行命令。
进一步的同级实例引用、独立源单元、anchor 重载、晚期阻塞、返回可用性、
四态与零提交测试在 `tests/system/test_sibling_calls_and_order.py`。

## 运行

在当前工作树配置的工具链环境中执行：

```sh
python issues/after_call_order/verify.py --output /tmp/after-call-order-current
```

该脚本不读取历史结果作预期。成功后应输出：

```text
CalleeAfterOnly: [6, 7, 7, 7] (C++ workers 1/2 and RTL agree)
CallerAfter: [6, 7, 7, 7] (C++ workers 1/2 and RTL agree)
```

新输出目录包含 results.json、commands.json、源单元、同一个 final 的双目标产物，
以及每条命令的标准输出和错误。候选测试结果与未运行项应以该目录的实际记录为准。

## 保留的历史结果：878f17

2026-10-21 的基线为 `878f17ddb1308a49368023167787c165245c4562`，
包含此前的 `SmallVector<PreparedBranchJoin, 0>` 构建修复。当时没有修改本例的
编译器语义，内部 callable after 未传播到调用者。

| 周期 | 开始的 count | increment | 当时 CalleeAfterOnly | 当时 CallerAfter |
| --- | --- | --- | --- | --- |
| 0 | 5 | 候选 6 | 5 | 6 |
| 1 | 6 | 候选 7 | 6 | 7 |
| 2 | 7 | 不 fire | 7 | 7 |
| 3 | 7 | 不 fire | 7 | 7 |

当时 C++ workers 1/2 与生成 RTL 一致，每个入口运行 4 周期、8 次 Work 观测。
该旧结果说明了原始缺口，不能作为新语义的通过证据。原 main 的
`/home/lc/pyCircuit/issues/after_call_order/` 和历史 generated 目录均保持未修改；
此隔离工作树只复制并更新源码、验证脚本及 README，没有复制历史生成产物。
