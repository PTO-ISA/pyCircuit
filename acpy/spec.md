# ACPy：class Module 与统一 Rule

语义以 [核心设计](../pycircuit/docs/core-design.md) 为准；实现与构建见
[编译器 README](../pycircuit/README.md)。模型通过 AST 静态编译，不执行任意 Python 代码。

## Module 与固定连接

```python
from pycircuit import ac

@ac.module
class Stage:
    def __init__() -> None:
        pass

    @ac.rule
    def transfer(source: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return source + 1

    def __call__(source: ac.queue[ac.u32]) -> ac.queue[ac.u32]:
        return transfer(source)

@ac.module
class Top:
    def __init__() -> None:
        source = ac.queue[ac.u32](initial=[1, 2], capacity=2)
        out = ac.queue[ac.u32](capacity=2)
        stage = Stage()

    def __call__() -> None:
        out = stage(source)
```

`__init__` 只表达静态构造；`__call__` 绑定固定接口，保存组合计算与调用选择。
实例只绑定一次；运行时分支不创建组件／子实例。组件只能在 __init__ 中构造，禁止文件级状态。
可以在 __init__ 绑定已有实例依赖，或在 __call__ 绑定固定接口；编译器先收集全部静态绑定再展开行为，允许互相引用，禁止递归调用。
运行时 if 中的子 Module 调用保留为 module.call，将路径条件及有效性传播到内部事务；不创建 Module 级原子事务。输出组件在父 Module 的 `__init__` 中明确声明，
用 `out = stage(source)` 绑定目的地，赋值后 out 仍是同一个 Queue。`__call__` 的组件返回类型
声明输出接口，主体返回对应 Rule 的输出；允许条件化 return／None，未选择路径不提出输出效果。
输出写入与 Rule 的 guard、消费和状态修改属于同一个原子事务，不额外建立转发事务。
只提供状态和公开 Rule 的组件可以省略 `__call__`；构造实例后直接调用其 Rule，
无需额外写 `component()`。省略主体不自动调度任何 Rule，也不生成空的 Module body。
需要绑定输入、输出连接或选择每拍 Rule 的模块才定义并连接 `__call__`。
`.outputs` 和返回内部组件引用的导出包装已删除，不再隐式创建输出 Queue。
需要多个输出时先声明对应组件，再写 `a, b = stage(source)`；已有组件也可作为显式参数传入。
内部状态观察直接使用逐拍轨迹，不需要为了观测而给组件添加输出包装。
成员函数省略 self 和 self.。构造配置、成员资源、子实例及 Rule 在本实例内可见。
局部变量和参数优先于同名成员。所有参数和返回类型显式标注，无返回值写 `-> None`。

共享连接资源通过父模块声明及显式绑定参数访问。其它未绑定资源使用公开 Rule 访问；
`@ac.rule(public=False)` 只允许本 Module 直接调用。构造一次得到共享状态，分别构造得到独立状态。
Module body 中普通 SSA 共享计算不会建立额外事务，也不增加跨拍 valid／缓存状态。

例如寄存器组件只需构造状态和声明 Rule：

```python
@ac.module
class Register:
    def __init__() -> None:
        value = ac.cell[ac.u32](initial=0)

    @ac.rule
    def read() -> ac.u32:
        return value

    @ac.rule
    def write(data: ac.u32) -> None:
        value = data
```

父 Module 在 `__init__` 中写 `register = Register()`，之后调用 `register.read()` 或
`register.write(data)`。嵌套调用保留原有的 guard、组件可见性及外层原子事务语义。

## Rule、调用与 guard

`@ac.rule` 统一表达 guard、计算、控制流、返回值和组件效果。编译器根据实际操作和嵌套调用
推导只读／修改分类。Module body 中每个修改性调用点建立独立事务；嵌套调用归入外层事务。
实际路径全部操作必须成功，否则取消所有状态修改。

```python
@ac.rule(guard=ready)
def send(value: ac.u32) -> None:
    destination = value
```

guard 可以是 bool 表达式或接收对应参数并返回 bool 的只读 Rule／payload helper。
helper 不因用于 guard 获得状态访问权限；需要状态时使用只读 Rule，或先观察状态再把 payload 交给 helper。
完整条件包含 Module 选择路径、自身／嵌套 guard、实际组件就绪要求，以及实参和控制值的读取条件。
未执行分支不增加要求。读取／guard 失败不作为 False 进入 else，也不阻塞合流后的无关事务。

Rule 参数可接收 payload 或基础组件引用；Module 实例依赖通过 __init__／__call__ 静态绑定，不作为 Rule 参数传递。
普通返回值可以用于只读计算或同事务嵌套调用。独立事务之间通过组件传值；
修改性调用的普通返回值不能传给另一个独立事务。组件类型返回值绑定为固定输出资源的效果。
混合输出用显式 tuple 类型，各槽位的 None 省略该效果；省略输出仍保留该路径上的其它修改。
Cell／Signal／Queue 输出均绑定此前明确声明的对象，组件种类与 payload 类型必须一致。
Module 主体不能把普通 u32 返回值绑定到 Queue；Rule 内显式写 Queue 可以使用普通 payload。
禁止所有链式赋值；多输出使用显式 tuple 绑定。赋值连接不创建新组件或改变身份。

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

bool、定宽整数、声明字段的 struct、tuple 和 `ac.array(T, shape=(...))` 的 payload 形式表示组合值。
纯组合 struct/tuple 的字段投影表示具体信号的连接：`consume(bundle.enable)` 只继承该字段的
数据就绪条件，不因另一个未使用字段的普通载荷未就绪而额外停顿。嵌套字段、静态字段更新及
条件合流保留这个区分；读取已存储的结构体仍使用原组件的同一个读取快照。
字段投影不能消除显式断言、调用 guard 或修改性调用的事务来源约束。
传完整结构体仍要求完整载荷就绪；不会隐式删掉调用接口中声明的输入。
普通 helper 只接收和返回 payload，不接收组件／Module 引用，不隐式捕获运行时状态，不调用 Rule。
值的类型和位宽严格匹配；字面量按目标类型定宽，不提供 bool(value) 或隐式扩展／截断。
显式整数转换写 `ac.u8(x)`、`ac.u16(x)`、`ac.u32(x)`、`ac.u64(x)` 及对应 `i8/i16/i32/i64`。
只接受一个整数参数。扩展由**源类型**的符号性决定：signed 符号扩展，unsigned 零扩展；
缩窄保留低位；同宽转换保留位模式并改变后续运算的符号语义。不支持 bool 转换，不增加隐式转换。
转换 lower 到标准 MLIR 整数扩展／截断操作；同宽为整数恒等操作。

if、guard、短路与条件表达式的条件严格为 bool，整数非零判断写 `value != 0`。
前端直接生成 scf.if，SSA 合流不提前求值不同视图的读取；所有循环必须在编译期静态展开。
禁止函数主体中的嵌套声明，多余／重复／未知调用参数明确诊断。

array 是固定正数 shape 的容器，不是 Module。使用完整嵌套列表或静态推导式构造，禁止直接 array of array；struct 可以含 array。
`a[i,j]` 使用普通整数坐标，`a[mask]` 保留原 shape／元素身份并与已有选取取交集。
`match(lambda item: predicate)` 或命名 payload helper 只计算临时 mask，不隐式读取组件；`mask[i,j]` 查询 bool 选取位。
对外传选取后的 array，helper 仅能接收 payload array，选取域经调用和合流保留。
`cells[mask] = value` 在同一事务内向选中组件写同一 payload；无批量读取。
空 mask 正常，仲裁返回 `(found, position)`；需要失败时明确把 found 纳入 guard。
静态越界报错，动态越界／访问未选元素传播未就绪。选取后的 array 或 mask 不可跨拍存储。
Queue[array] 从空队列开始；完整构造、状态初值及 MLIR 重载均校验 shape。详见 [array 设计](../pycircuit/docs/array-design.md)。

不支持旧 array/vector/qarray/ref/var、`@ac.module def`、`@ac.method` 或 After。无自动仲裁、SystemVerilog、激活调度与跨拍缓存。
