# 前端简化语义：从完整示例到编译目标

更新日期：2026-10-26。

本文记录讨论确认的目标语义。代码是设计示例，后端片段是 lowering 示意，不代表当前编译器已经实现或验证这些能力。当前实现以 [语言参考](../reference/language.md) 为准。

核心范式：**Module 管结构和持久状态，Rule 管原子事务，Method 提供同拍调用接口。**

## 1. 完整示例

输入样本先加一，再计算累计和。累加器最多接受四个样本，之后保持；输出阻塞时，累加事务也保持。

```python
import pycircuit as ac


# 外部 Rule：通过显式输入输出连接。
@ac.rule
def add_one(src: ac.u8) -> ac.u8:
    x = src
    return x + 1


@ac.module
def BiasStage(inp: ac.u8) -> ac.u8:
    out = add_one(inp)          # 注册 Rule，输出默认 Queue
    return out                 # 导出已有输出对象


@ac.module
def Accumulator():
    total: ac.u16 = 0
    samples: ac.u8 = 0

    @ac.method(guard=lambda: samples < 4)
    def add(x: ac.u8) -> ac.u16:
        new_total = total + x
        total = new_total
        samples = samples + 1
        return new_total       # 返回同拍计算值


@ac.module
def SumStage(inp: ac.u8, acc: ac.module) -> ac.u16:
    # acc 是上层传入的实例引用，具体模块在连接时确定。
    accepted: ac.u8 = 0

    @ac.rule
    def accumulate(src: ac.u8) -> ac.u16:
        x = src
        result = acc.add(x)
        accepted = accepted + 1  # 直接操作所在 Module 的寄存器
        return result

    out = accumulate(inp)      # 显式注册内部 Rule
    return out


@ac.module
def Pipeline(inp: ac.u8) -> ac.u16:
    acc: Accumulator           # 显式创建具名实例
    summer: SumStage

    intermediate = BiasStage(inp)  # 隐式创建实例，绑定输出 Queue
    out = summer(intermediate, acc)  # 连接已有实例，传入模块引用
    return out
```

实例层级与连接是两件事：

```text
实例层级：
Pipeline
├─ BiasStage（隐式实例）
├─ summer: SumStage
└─ acc: Accumulator

数据与调用连接：
输入 Queue → BiasStage → 中间 Queue → summer → 输出 Queue
                                        │
                                        └─ 调用 acc.add(x)
```

`summer` 与 `acc` 是平级实例。SumStage 不创建 Accumulator，而是使用上层传入的引用。

输入依次为 `3、5、2、7`，预处理值为 `4、6、3、8`，成功提交后的累计输出为 `4、10、13、21`。完成后 `accepted = samples = 4`，`total = 21`。这是语义推演，未作为已运行的仿真结果。

## 2. Module：定义、实例和端口

Module 使用 `def` 声明，可以包含状态声明、组合计算、Rule/Method 定义、子模块实例与连接。

### 实例引用与输出对象分开

| 写法 | 含义 |
| --- | --- |
| `acc: Accumulator` | 显式创建实例，`acc` 是实例引用 |
| `intermediate = BiasStage(inp)` | 隐式创建实例并连接，`intermediate` 只表示输出对象 |
| `out = summer(inp, acc)` | 连接已有 `summer` 实例，不再创建实例 |
| `acc.add(x)` | 调用已有实例的方法，不创建实例 |

不再让一个名字同时表示模块输出和实例句柄。冒号声明是 pyCircuit 的硬件声明，不只是普通 Python 的类型提示。

### 数据类型与组件类型分开

Module 和 Rule 的 `ac.u8`、`ac.u16` 等端口注解描述传输的数据类型。连接 Queue 还是 Reg，由调用处的实际对象决定；没有显式声明时默认 Queue。

```python
stage: BiasStage
out = stage(inp)        # 默认 Queue，元素类型为 u8
```

另一种独立写法：

```python
stage: BiasStage
out: ac.u8             # 显式声明寄存器
out = stage(inp)        # 输出绑定到这个寄存器
```

输入绑定传入的实际对象，类型必须匹配。导出或继续连接已有对象不复制状态，也不额外增加存储。组件类型沿端口绑定关系传播；不能在每层 Module 返回时重复创建 Queue。

当前只考虑 Queue 和 Reg 端口，不加入 Wire 端口。

### 模块引用参数

```python
def SumStage(inp: ac.u8, acc: ac.module) -> ac.u16:
```

`acc: ac.module` 表示接收已有实例的引用，不限定为 Accumulator，也不应用默认 Queue 规则。编译器在连接时确定具体实例，并检查它是否提供签名和效果兼容的 `add` 方法。

这是编译期静态绑定，不是运行时动态选择模块。传入引用不创建实例、不复制状态，也不改变被引用实例的层级归属。

### 状态与组合局部值

```python
accepted: ac.u8 = 0        # Module 内的寄存器
next_value = accepted + 1  # 普通组合计算，不创建存储
```

端口注解、Module 引用参数、内部状态声明按所在位置和类型区分。Rule/Method 内的普通局部计算同样不创建持久状态。

Module 顶层可调用只读 Method，不能隐式消耗 Queue。查看队头可使用只读 `peek()`，其可用条件传播给使用结果的事务。

## 3. Rule：一次调用注册一个原子事务

Rule 可定义在 Module 内部或外部，定义本身不执行。Module 顶层显式调用 Rule，静态注册一个每拍参与 fire 判断的事务。

Rule 是所属实例的 **private 事务入口**，由该 Module 的 Work 判断 fire 后调用，不能通过 `instance.rule()` 对外调用。外部定义的 Rule 是可复用的源码模板，注册后仍形成所属实例的私有事务。

- 外部 Rule 通过显式参数连接对象，例如 `add_one`。
- 内部 Rule 还可以直接操作所属 Module 的寄存器，例如 `accumulate` 更新 `accepted`。
- Rule 可以有输入输出，调用处决定端口连接的组件类型。

对象读写采用赋值形式，按实际组件解释：

| 组件 | 读取 | 写入 |
| --- | --- | --- |
| Queue | pop | push |
| Reg | read | write |

```python
x = src       # src 是 Queue 时，提出一次 pop，取得局部值
return x + 1  # 对这次 Rule 调用的输出对象提出写入
```

重复使用局部值 `x` 不会重复 pop。Rule 的 `return` 表示输出写入；Module 的 `return out` 表示导出已有接口。

一次 Rule 事务包含输入消费、内部状态更新、输出写入和被调用 Method 的更新。这些操作共同提交；任一必要条件不满足，整个事务保持。不同 Rule 通过存储连接，不因此合并事务。

## 4. Method：带 guard 的同拍接口

Method 是 Module 的 **public 接口**，可通过已有实例引用被其他 Module 的 Rule/Method 调用。

| 类别 | 返回同拍值 | 提出状态更新 |
| --- | --- | --- |
| Value | 是 | 否 |
| Action | 否 | 是 |
| ActionValue | 是 | 是 |

Method 的参数和返回值是同拍值，不默认创建 Queue 或寄存器。Method 可有自己的 guard；Rule 可以调用 Method，Method 可以嵌套调用 Method，也可以跨模块调用。Value 必须保持只读。

Method 操作目标实例自己的状态。它不独立提交，实际调用路径上的 guard 和更新都纳入调用者 Rule 的事务。

例如 `acc.add(x)` 同拍返回累计值，同时向 `acc.total` 和 `acc.samples` 提出更新。`accepted` 属于 summer，累计状态属于 acc，但三者由同一次事务控制。

返回值的可用条件由编译器管理，作者不需要手写 valid。候选数据可以被组合计算；若事务不获准提交，就不能把它作为成功调用的结果发布。

## 5. 状态、fire 与原子性

- **读旧状态**：寄存器读取本拍已提交的值，不做隐式同拍转发；Method 可以返回组合计算的新值。
- **多写报错**：同一寄存器的同拍多写冲突由编译器报错，不隐式仲裁。
- **统一 fire**：编译器合并 Rule 条件、资源条件和必要 Method 的 guard。fire 成立才提出整个事务的全部更新，否则保持。

## 6. 编译目标：相同语义，两种后端

编译器确定实例和存储身份、端口连接、组合表达式、Rule fire，以及各存储的 next state 和更新使能。C++ 与 RTL 从同一份验证后的公共 IR 生成，保持相同的提交行为。

### gfsim

每个 Module 保留 Work/Xfer。Work 读取旧状态，计算组合结果和 fire；fire 成立时调用私有 Rule 函数。Rule 函数执行事务计算，调用公开 Method，并提出全部更新。Method 也是独立计算入口，向其所属实例提出 next-state proposal。

SumStage 的 Work 和私有 Rule 函数概念上是：

```cpp
void SumStage::Work() {
    // acc 引用 Pipeline 创建的平级实例。
    bool fire = inp.CanPop() && out.CanPush() && acc->AddGuard();
    if (fire)
        AccumulateWork();
}

// private：只能由所属 Module 调度。
void SumStage::AccumulateWork() {
    uint16_t result = acc->AddWork(inp.Head());
    inp.ProposePop();
    out.ProposePush(result);
    accepted_d = accepted_q + 1;
    accepted_enable = true;
}
```

Accumulator 的公开 Method 及其 guard 查询概念上是：

```cpp
bool Accumulator::AddGuard() const {
    return samples_q < 4;
}

uint16_t Accumulator::AddWork(uint8_t x) {
    uint16_t result = total_q + x;
    total_d = result;
    samples_d = samples_q + 1;
    update_enable = true;
    return result;
}
```

所有 Work 和必要系统检查完成后，各存储所有者在 Xfer 提交自身更新；没有更新则保持。Queue 也在提交阶段处理出入队。引用和别名不能导致同一个存储对象重复提交。

这些片段省略初始化、proposal 标记清理和调度框架，只展示计算与状态归属，不规定具体运行时 ABI。

### RTL

| 前端对象 | RTL 对应 |
| --- | --- |
| Module 实例 | 模块及层级连接 |
| Queue | FIFO 存储与出入队接口 |
| 寄存器 | Q、D、更新使能及存储单元 |
| 局部计算 | 组合逻辑和中间线网 |
| private Rule | 模块内部的 fire 和受其控制的更新逻辑 |
| public Method | 参数、返回值、guard 和调用使能端口 |

Method 的接口方向如下：

| 信号 | 方向 |
| --- | --- |
| 参数、调用使能 enable | 调用者 → 被调用者 |
| 返回值、可执行条件 ready | 被调用者 → 调用者 |

下面用 SystemVerilog 给出 SumStage、Accumulator 的完整模块声明，以及 Pipeline 层的连接片段。Queue 本体不在两个模块内，用队头、可用条件和出入队请求表示接口；FIFO 实现不在此展开。clock/reset 是后端接口，示例中的同步复位用于说明初始值，不增加前端端口要求或规定完整 reset 契约。

#### SumStage：私有 Rule 控制全部操作

```systemverilog
module SumStage (
    input  logic        clk,
    input  logic        reset,

    // 输入 Queue：读取队头，并提出出队请求。
    input  logic [7:0]  in_data,
    input  logic        in_valid,
    output logic        in_pop,

    // 输出 Queue：传递入队数据，并提出入队请求。
    output logic [15:0] out_data,
    input  logic        out_ready,
    output logic        out_push,

    // 调用外部 acc.add 的接口。
    output logic [7:0]  acc_add_x,
    input  logic [15:0] acc_add_result,
    input  logic        acc_add_ready,
    output logic        acc_add_en
);

    logic [7:0] accepted;

    // private Rule：accumulate，没有对外的 Rule 调用端口。
    logic        accumulate_fire;
    logic [15:0] accumulate_result;

    assign accumulate_fire =
        !reset && in_valid && out_ready && acc_add_ready;

    // 同拍传递 Method 参数和返回值。
    assign acc_add_x          = in_data;
    assign accumulate_result = acc_add_result;
    assign out_data          = accumulate_result;

    // 同一个 fire 控制事务的全部操作。
    assign acc_add_en = accumulate_fire;
    assign in_pop     = accumulate_fire;
    assign out_push   = accumulate_fire;

    always_ff @(posedge clk) begin
        if (reset)
            accepted <= 8'd0;
        else if (accumulate_fire)
            accepted <= accepted + 8'd1;
    end

endmodule
```

`in_valid` 表示输入 Queue 非空，`out_ready` 表示输出 Queue 可入队。Rule 的私有函数在 RTL 中对应上述内部逻辑，不要求生成一个执行过程或函数调用指令。

#### Accumulator：公开 Method 的端口与状态

```systemverilog
module Accumulator (
    input  logic        clk,
    input  logic        reset,

    // public ActionValue Method：add(x)。
    input  logic [7:0]  add_x,
    input  logic        add_en,
    output logic        add_ready,
    output logic [15:0] add_result
);

    logic [15:0] total;
    logic [7:0]  samples;

    // Method guard 与同拍返回值。
    assign add_ready  = samples < 8'd4;
    assign add_result = total + {8'b0, add_x};

    // Method 更新自己所属模块的状态。
    always_ff @(posedge clk) begin
        if (reset) begin
            total   <= 16'd0;
            samples <= 8'd0;
        end else if (add_en) begin
            total   <= add_result;
            samples <= samples + 8'd1;
        end
    end

endmodule
```

`add_en` 表示调用已经获准执行，调用者必须将 `add_ready` 合入 fire。返回数据可以持续组合计算，但只有获准执行时才产生对应的状态更新和输出入队。

#### Pipeline：连接平级实例

以下片段位于 Pipeline 内。`middle_*` 连接 BiasStage 输出的中间 Queue，`output_*` 连接最终输出 Queue；四组 `add_*` 信号连接两个平级实例的 Method 接口。

```systemverilog
logic [7:0]  add_x;
logic [15:0] add_result;
logic        add_ready;
logic        add_en;

SumStage summer (
    .clk            (clk),
    .reset          (reset),

    .in_data        (middle_head_data),
    .in_valid       (middle_head_valid),
    .in_pop         (middle_pop),

    .out_data       (output_push_data),
    .out_ready      (output_push_ready),
    .out_push       (output_push),

    .acc_add_x      (add_x),
    .acc_add_result (add_result),
    .acc_add_ready  (add_ready),
    .acc_add_en     (add_en)
);

Accumulator acc (
    .clk        (clk),
    .reset      (reset),
    .add_x      (add_x),
    .add_result (add_result),
    .add_ready  (add_ready),
    .add_en     (add_en)
);
```

前端 `out = summer(intermediate, acc)` 绑定已有实例和连接对象，`acc.add(x)` 则使用生成的 Method 接口。编译器负责端口及层级连线，不复制 Accumulator 的状态，也不增加调用周期。

RTL 中没有“先进入 accumulate，再执行 add”的软件执行顺序：参数、返回值、guard 和 enable 都通过组合电路连接。在正常工作周期，同一个 fire 使输入出队、输出入队、accepted 更新和 acc 的寄存器更新在时钟沿共同发生；fire 为假则这些事务更新全部保持。

## 7. 后续演进

同拍 Work 之间的依赖问题，认为可以在上述基础框架上继续升级，通过完善依赖分析和计算调度来支持。这里先明确 Module、Rule、Method 的职责和编译目标，不预先限定后续能力边界。
