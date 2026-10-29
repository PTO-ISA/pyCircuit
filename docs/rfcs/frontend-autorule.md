# ac.autorule：自动注册 Rule 的前端语法糖

记录日期：2026-10-26。

本文补充 [前端简化语义](frontend-simplification.md) 和 [Module 组合逻辑与控制流](module-comb-control-flow.md)。这是目标语义草案，不表示当前编译器已经实现。

## 1. 定义

`@ac.autorule` 表示一个自动注册的 Rule。编译器先将它转换为普通 `@ac.rule` 加显式注册，再沿用正常编译流程。

- 自动的是注册，不是无条件提交；每拍仍由 guard 和必要资源条件决定 fire。
- 每个所属 Module 实例自动注册一次，不允许再显式调用该 autorule 注册第二次。
- 当前形式定义在 Module 内，没有显式参数和返回值，直接使用所属 Module 的状态和端口。
- 与普通 Rule 一样，它是 private 原子事务；需要调用处绑定输入输出时，使用普通 Rule。

## 2. 状态机示例

输入连接 Queue。状态机接收一个数、计算加一结果、发布结果，然后接收下一个数。公开 Value Method 用于读取最近发布的结果，初始值为 0。

```python
import pycircuit as ac


@ac.module
def IncrementEngine(inp: ac.u8):
    # 0：等待输入；1：计算；2：发布结果。
    state: ac.u2 = 0
    operand: ac.u8 = 0
    computed: ac.u8 = 0
    published: ac.u8 = 0

    @ac.autorule(guard=lambda: state == 0)
    def receive() -> None:
        operand = inp
        state = 1

    @ac.autorule(guard=lambda: state == 1)
    def compute() -> None:
        computed = operand + 1
        state = 2

    @ac.autorule(guard=lambda: state == 2)
    def publish() -> None:
        published = computed
        state = 0

    @ac.method
    def latest() -> ac.u8:
        return published
```

不需要在 Module 末尾手写 `receive()`、`compute()`、`publish()`。

```text
等待输入 ── 输入可读 ──→ 计算 ──→ 发布结果 ──→ 等待输入
```

输入为 7 时，三个成功提交的周期依次完成：保存 7、计算 8、发布 8。输入为空时 receive 不 fire，状态保持在 0。所有 guard 读取本拍旧 state，因此不会在一拍内连续穿过三个状态。

## 3. 规范化与编译目标

编译器将上例的三个 `@ac.autorule(...)` 替换为保留相同 guard 的 `@ac.rule(...)`，保留函数体，并在所属 Module 中各添加一次注册：

```python
receive()
compute()
publish()
```

之后只处理普通 Rule。该例的 fire 为：

```text
receive_fire = (state == 0) AND 输入 Queue 非空
compute_fire = (state == 1)
publish_fire = (state == 2)
```

三个状态条件互斥，因此三个 Rule 对 state 的写入不构成同拍冲突。

gfsim 中，Module Work 分别判断 fire，调用对应的 private Rule 函数；更新在 Xfer 提交。RTL 中，状态比较和资源条件产生 fire，各 Rule 的更新分别由自己的 fire 控制。

`autorule` 不进入后端成为新的执行机制，不增加周期，也不改变普通 Rule 的原子性和冲突检查。
