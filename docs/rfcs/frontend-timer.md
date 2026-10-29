# Timer：时延表达与事件调度

记录日期：2026-10-26。

本文补充 [前端语义](frontend-simplification.md)、[autorule](frontend-autorule.md) 和 [CombWork/Work/Xfer](module-comb-control-flow.md)。这是目标设计，不表示当前编译器已经实现或验证。

## 1. 前端表达

用一个 timer 表达“事务提交后经过 N 周期，完成条件变为满足”。timer 自己保存等待状态，不要求作者另写计数器或安排仿真唤醒。

下面的组件一次接收一个输入，至少等待 8 周期后输出：

```python
import pycircuit as ac


@ac.module
def Delay8(inp: ac.u8) -> ac.u8:
    saved: ac.u8 = 0
    timer: ac.timer

    @ac.autorule(guard=lambda: timer.idle())
    def accept() -> None:
        saved = inp
        timer.start(8)

    @ac.rule(guard=lambda: timer.done())
    def finish() -> ac.u8:
        timer.clear()
        return saved

    out = finish()
    return out
```

输入按 Queue 连接，输出默认 Queue。accept 的输入出队、保存数据和启动 timer 原子提交；finish 的输出入队和清除 timer 原子提交。

## 2. Timer 语义

| 操作 | 含义 |
| --- | --- |
| `idle()` | 初始或清除后的空闲状态，可以启动 |
| `start(n)` | 随事务提交启动，经过 n 个周期后到期 |
| `done()` | 到期条件，保持为真直到清除 |
| `clear()` | 随事务提交返回空闲状态 |

`idle()`、`done()` 是只读条件；`start()`、`clear()` 是状态更新提议。start 不暂停当前 Rule，也不读取后续变化的输入来替换已经保存的数据。

请求在时钟沿 t 提交启动 8 周期 timer，则结果最早在 t+8 的提交边界交付。这里规定的是最早可交付时间；输出阻塞可以推迟实际交付。

```text
空闲 → 接受输入并启动 → 等待到期 → 到期且输出可接受 → 输出并清除
```

到期后输出仍满时，timer 保持完成条件，saved 保持原值，不丢失结果，也不接受下一笔输入。

## 3. gfsim：deadline 与到期事件

编译器识别 timer 操作，并记录 `timer.done()` 到相关 Rule fire 的依赖。timer 是框架内建的时序资源，其只读条件可用于 guard，不是需要作者编写的跨模块组合调用。

start 所属事务成功提交时，运行时保存：

```text
active = true
deadline = 当前提交周期 + n
向事件队列登记：deadline 时唤醒依赖这个 timer 的计算
```

`done()` 可以按 `active AND 当前周期 >= deadline` 求值，不需要每拍递减计数器。没有其他相关事件时，等待期间不必反复运行该组件的 Work。

到期事件使相关 CombWork/Work 重新求值，仍由普通 Rule 的 fire 决定更新：

```text
finish_fire = timer.done() AND 输出 Queue 可入队
```

事件负责唤醒，不直接替 Rule 提交结果。若输出阻塞，保持 done；之后由输出 Queue 可用性的变化再次唤醒。clear 成功提交后返回空闲。

## 4. RTL：计时状态与到期信号

同一个 timer lower 为活动状态、计数器和到期信号。start、clear 使用所属事务的使能；finish 使用到期条件和输出资源条件共同产生 fire。

计数器的到期判断必须与 gfsim 使用相同的周期边界，保证 t 启动、最早 t+n 提交结果，不能因再寄存一次 done 而额外增加一拍。

这是一种约定的延迟模型，不要求编译器把普通计算自动转换为真实的多周期运算器或完整内存系统。前端表达可观察的时延，gfsim 用事件实现，RTL 用时序电路实现，两者遵守相同的公共语义。
