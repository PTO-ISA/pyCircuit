# 固定槽位与自动读取登记：代码及分步实验

本轮只改两件事：构造时建立直接索引表；Queue 读取在 Work 内自动登记依赖。37 项测试通过，三版本各36组程序的逐拍状态、获准集合、事件和全部运行计数一致。

本次测量中，固定映射后耗时降低约1.9%，加上自动登记后相对修改前降低约2.3%。自动登记相对映射版本的额外差值约0.5%，较小，不能排除计时波动。当前证据显示轻微缓解，未消除上一轮 dirty 机制的整体退化。

## 先理解现在的读取路径

假设 Module 3 的资源表为 `[Q2, Q7, Q9]`，Q7 在它里面的位置是1。构造代码保存：

```python
Q7.module_slots[3] = 1
```

Rule 读取 Q7 时，路径为：

```text
Q7.peek()
  → Queue 根据引擎的 active_module / active_rule 确定读取者
  → 直接取 Q7.module_slots[active_module]，得到 slot=1
  → 在 Module 的 slot=1 读者位图设置当前 Rule 的 bit
  → 返回 Q7 的旧队首
```

`module_slots` 只保存固定的位置关系，不保存本次读者集合。实际执行到读取才设置读者位；动态下标先选中实际 Queue，再执行同样的登记。未声明位置为 -1，访问报错。Queue 的可能读者列表仍用于状态变化通知，没有改成扫描整个映射数组。

执行上下文只有两个临时字段：进入 Module Work 设置 active_module；begin_rule 确认要执行 body 后设置 active_rule。complete/abort 后 active_rule 清空，后续读取归属 Module；缓存命中不进入 Rule 上下文。Module 的所有退出路径均清空上下文，异常也不遗留。嵌套 Rule 不支持。

`peek/try_peek/empty/full/size/current` 都遵守这一规则。读空先登记，随后返回空或抛出 NeedInput。Work 外的结果观察、快照与 HTML 采集不产生订阅；查询只登记，不消费元素。pop/revise 仍有目标依赖兜底，纯 push 不建立输出读取关系。

## 生成式代码变成什么样

之前需要显式登记：

```python
if not e.begin_rule(rid):
    return
e.record_read(self.mid, self.source.qid, rid)
inst = self.source.try_peek()
if inst is None:
    e.abort_rule(rid)
    return
self.source.propose_pop(rid)
# 计算并提出其他效果
# ...
e.complete_rule(rid)
```

现在直接读取：

```python
if not e.begin_rule(rid):
    return
inst = self.source.try_peek()  # 自动登记当前 Rule；空输入也会登记
if inst is None:
    e.abort_rule(rid)
    return
self.source.propose_pop(rid)   # 消费仍显式提出，获准后才生效
# 计算并提出其他效果
# ...
e.complete_rule(rid)
```

没有新的 Queue 包装类，也没有模型公共 observe/take/put 操作。`record_read` 保留兼容，显式登记与自动登记使用同一位图去重；新 riscv 阶段中已经移除显式调用。

peek 后 pop 仍会做一次目标依赖去重检查，不会追加第二条订阅。内部 peek 不再调用 public empty，仲裁检查直接读取 count/capacity，避免内部查询走自动登记路径。候选、dirty、参数比较、通知和 DFS 算法保持原有规则。

## 验证与计时结果

从修改前的未提交工作区复制独立源码，按顺序测量三个检查点：

| 版本 | 改动 | 相对修改前耗时比值中位数 |
| --- | --- | ---: |
| before | 本轮修改前的 dirty 引擎 | 1.0000 |
| mapping | 只增加固定槽位映射 | 0.9814 |
| automatic | 再增加自动读取登记、上下文管理并简化 riscv | 0.9770 |

每版本运行 ALU、sum、control 三个完整程序 × 访存延迟1/3/5 × 缓存开关 × Module 正反序，共36配置。每配置5次取中位数；上表再对36个对应配置的耗时比值取中位数，不是总时长比值。

aarch64，Python 3.11.16。使用同一 bench.py 和计时循环：从首拍到 HALT，包含 sim.step 和 HALT 检查；排除构造、进程启动、导入、解释器检查、轨迹保存及 HTML。三版均开启相同统计，关闭观察器。本轮没有顺便关闭统计、优化参数比较或改变事件队列。三个版本分开采集，没有将系统负载和运行时噪声隔离为零。

下表是延迟5、缓存开启、Module 正序的样本中位数：

| 程序 | before | mapping | automatic |
| --- | ---: | ---: | ---: |
| ALU | 11.144 ms | 10.922 ms | 10.921 ms |
| sum | 30.075 ms | 29.529 ms | 29.344 ms |
| control | 3.863 ms | 3.758 ms | 3.776 ms |

三版所有配置的全部统计计数一致。以 sum 的上述配置为例，均有772次 Rule Work、0次缓存命中、3469次资源定位、2429次实际读取登记、2422次旧读者位清除。`reader_lookups` 现在统计直接定位次数，不能再将这个数解释为二分查找次数。自动登记简化了作者代码，没有减少这组程序实际登记入口的次数。

原始样本、引擎与输入源码 SHA256、轨迹 SHA256、逐配置比值见 [read-tracking-results.json](read-tracking-results.json)。上一轮比较保留在 [原报告](report.md) 和 results.json，没有用新数据覆盖旧测量。

## 测试与复现

```bash
PYTHONPATH=gfsim/experiment python3 -m unittest test_engine examples.riscv.test_model review.test_review -v
python3 gfsim/experiment/bench.py --output gfsim/experiment/review-output/read-tracking/rerun --compare gfsim/experiment/review-output/read-tracking/before/results/traces.json
PYTHONPATH=gfsim/experiment python3 -m examples.riscv.run gfsim/experiment/examples/riscv/programs/sum.s --latency 5 --html
```

37项测试包含原有29项和8项新增方法。覆盖六种读取接口在 Module/Rule 内的归属与读空恢复、稀疏声明和别名、动态数组、peek 后修改与只修改、纯 push、缓存命中、complete/abort 后的上下文、异常及缺少完成调用、嵌套拒绝、显式登记兼容、Work 外观察无副作用。通用观察器开关等价测试保留。sum.s 运行结果为232周期、98条退休、x4=55，并导出 HTML。

本地独立检查点与大轨迹位于 Git 忽略目录：

```text
review-output/read-tracking/
  before/source/       before/results/
  mapping/source/      mapping/results/
  automatic/source/    automatic/results/
```

各 source 都是独立可运行的 Python 实验副本，包含对应 engine、construction、riscv 与同一个 bench.py，不包含 ripes5 或生成产物。可用该目录下的 bench.py 重新测量，输出到其他目录保留此次结果。before 是未提交状态，不能仅凭先前 Git 提交重建；若迁移测量，应一并复制这些检查点，JSON 中有运行源码校验值。

## 空间与范围

固定映射额外增加 QueueCount×ModuleCount 个槽位项。本轮64字数据存储的 CPU 有105个 Queue、5个 Module，即525项。若未来 C++ 采用 int32 表项，仅值占2100字节；这不是 Python 实际对象内存，也不是已实现的 C++ 测量。

ripes5 文件未修改，未重新运行原生对照；C++ runtime、模块间 var 传播及其余性能优化保持后续任务。无需增加新模型类或改变消费、仲裁和提交语义来使用自动读取登记。
