# ACPy：class Module 与统一 Rule

语义以 [核心设计](../pycircuit/docs/core-design.md) 为准；实现与构建见
[编译器 README](../pycircuit/README.md)。模型通过 AST 静态编译，不执行任意 Python 代码。

## Module 与固定连接

```python
from pycircuit import ac

@ac.module
class Stage:
    def __init__() -> None:
        out = ac.queue[ac.u32](capacity=2)

    @ac.rule
    def transfer(source: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return source + 1

    def __call__(source: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        out = transfer(source)
        return out

@ac.module
class Top:
    def __init__() -> None:
        source = ac.queue[ac.u32](initial=[1, 2], capacity=2)
        stage = Stage()

    def __call__() -> ac.queue[ac.u32]:
        stage(source)
        return stage.outputs
```

`__init__` 只表达静态构造；`__call__` 绑定固定接口，保存组合计算与调用选择。
实例只绑定一次；运行时分支不创建组件／子实例。Module 输出必须是固定组件连接。
成员函数省略 self 和 self.。构造配置、成员资源、子实例及 Rule 在本实例内可见。
局部变量和参数优先于同名成员。所有参数和返回类型显式标注，无返回值写 `-> None`。

子实例资源通过显式输出连接或绑定参数访问。其它未绑定资源使用公开 Rule 访问；
`@ac.rule(public=False)` 只允许本 Module 直接调用。构造一次得到共享状态，分别构造得到独立状态。
Module body 中普通 SSA 共享计算不会建立额外事务，也不增加跨拍 valid／缓存状态。

## Rule、调用与 guard

`@ac.rule` 统一表达 guard、计算、控制流、返回值和组件效果。编译器根据实际操作和嵌套调用
推导只读／修改分类。Module body 中每个修改性调用点建立独立事务；嵌套调用归入外层事务。
实际路径全部操作必须成功，否则取消所有状态修改。

```python
@ac.rule(guard=ready)
def send(value: ac.u32) -> None:
    destination = value
```

guard 可以是布尔表达式或接收对应参数并返回 bool 的函数，只允许观察状态。
完整条件包含 Module 选择路径、自身／嵌套 guard、实际组件就绪要求，以及实参和控制值的读取条件。
未执行分支不增加要求。读取／guard 失败不作为 False 进入 else，也不阻塞合流后的无关事务。

普通返回值可以用于只读计算或同事务嵌套调用。独立事务之间通过组件传值；
修改性调用的普通返回值不能传给另一个独立事务。组件类型返回值绑定为固定输出资源的效果。
混合输出用显式 tuple 类型，各槽位的 None 省略该效果；省略输出仍保留该路径上的其它修改。
Cell／Signal 输出绑定已声明对象，Queue 输出也可由直接绑定声明；运行时不产生新身份。

## Rule 关系

```python
@ac.rule(SB=("read",), SBR="strict", C="conflict", CF="independent")
def write() -> None:
    ...
```

字段表示当前 Rule 相对于同一 Module 内目标 Rule 的关系，可使用字符串或字符串 tuple／list。
SB／SBR 表示当前 Rule 在目标之前，均为同拍顺序；C／CF 对称。
SB 允许同事务组合；SBR／C 禁止实际同路径组合；CF 不增加顺序或冲突。
声明汇总为 Module 契约，沿实际嵌套调用向外传播，保留实例身份。
顺序不要求前驱成功，也不表示竞争优先级。所有分支边的节点合并图必须无环。

## 基础组件与赋值

| 组件 | 声明 | 普通值读取 | 已声明组件在赋值左侧 |
| --- | --- | --- | --- |
| Cell | `ac.cell[T](initial=...)` | 读取旧值 | 下拍 write |
| Queue | `ac.queue[T](capacity=1, initial=...)` | Rule 路径中读取旧队头并 pop；Module／guard 只观察 | 下拍 push |
| Signal | `ac.signal[T]()` | 读取本拍有效值，无有效生产者则失败 | 本拍 set |
| EHR | `ac.ehr[T](initial=...)` | 固定端口视图 | 该端口写入 |

Cell／EHR 必须有初值；Signal 不接受初值或容量。Queue 的标量初值表示一个元素，
序列初值表示 FIFO 内容。显式 `q.peek()` 只观察；empty／size 使用旧状态，full 使用同拍容量视图。
同事务重复 Queue 消费按真实资源合并。前序成功 pop 可以为满 Queue 的 push 腾空间；
新 push 数据下拍可读，没有空队列数据旁路。

Cell 写入后读取仍为旧值；字段／数组赋值更新所属 Cell。Signal 的 set → get 是 SBR，
同事务不能实际同时 set 和 get，失败的生产者不产生 valid，下一拍重新失效。
EHR 端口由声明约束分配；固定读取观察低端口获准写入中端口最大的值，
最终 next-state 选择最高端口获准写入。不能由任意事务排序增加可见性，
同事务不能实际访问同 EHR 的不同端口。基础组件集中提供操作契约。

## 普通类型与控制流

bool、定宽整数、声明字段的 struct、`ac.array[T, N]`、tuple 和 `ac.var[T]` 表示组合 payload。
普通 helper 使用显式接口，组合进调用者的求值／事务上下文。算术复用标准 MLIR 方言。
if、短路与合流保留实际 SSA 和路径，不提前计算不同可见性视图的读取。

`ac.array(ac.queue[T], shape=(N,), ...)` 为固定一维 Queue 资源数组，运行时下标选择已有元素。
字面量固定循环静态展开；不能静态展开的循环保留在源层并在有限 RTL lowering 中明确诊断。

不支持旧 `@ac.module def`、`@ac.method` 或 After。无自动仲裁、SystemVerilog、激活调度与跨拍缓存。
