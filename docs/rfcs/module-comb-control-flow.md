# Module 组合逻辑与控制流：表达和编译目标

记录日期：2026-10-26。

本文补充 [前端简化语义](frontend-simplification.md)，重点说明 Module 内、Rule 之外的组合计算与控制流。代码采用讨论中的目标语义；C++/RTL 是对应的设计示意，不是当前编译器生成结果或实现验证。

核心规则：**组合逻辑计算本拍可用的值，Module 控制流选择启用哪些 Rule，Rule 的 fire 决定是否提出整个事务的更新。**

## 1. 完整例子：四个样本组成一批

控制器每次读取四个样本，通过平级累加器完成求和。收集完成后，输出一个批次结果，再开始下一批。输出阻塞时保留当前批次，不继续消费输入。

```python
import pycircuit as ac


@ac.module
def BatchAccumulator():
    total: ac.u16 = 0
    samples: ac.u8 = 0

    # Module 组合逻辑：只读本地旧状态，不创建寄存器。
    slots_left = 4 - samples
    can_add = slots_left != 0
    complete = slots_left == 0

    # public Value Method：直接返回已计算的组合值。
    @ac.method
    def remaining() -> ac.u8:
        return slots_left

    # public Action Method：使用组合条件作为 guard。
    @ac.method(guard=lambda: can_add)
    def add(x: ac.u8) -> None:
        total = total + x
        samples = samples + 1

    # public ActionValue Method：返回本批结果，同时清空批次状态。
    @ac.method(guard=lambda: complete)
    def take() -> ac.u16:
        result = total
        total = 0
        samples = 0
        return result


@ac.module
def BatchController(inp: ac.u8, acc: ac.module) -> ac.u16:
    accepted: ac.u8 = 0

    # Module 组合逻辑供 Rule 和 Method 使用。
    next_accepted = accepted + 1
    slots_left = 4 - accepted

    @ac.method
    def remaining() -> ac.u8:
        return slots_left

    @ac.rule
    def collect(src: ac.u8) -> None:
        x = src
        acc.add(x)
        accepted = next_accepted

    @ac.rule
    def emit() -> ac.u16:
        result = acc.take()
        accepted = 0
        return result

    # 声明两个事务及其互斥的启用条件。
    if accepted < 4:
        collect(inp)
    else:
        out = emit()

    return out


@ac.module
def BatchPipeline(inp: ac.u8) -> ac.u16:
    acc: BatchAccumulator
    controller: BatchController

    out = controller(inp, acc)
    return out
```

```text
BatchPipeline
├─ controller: BatchController
└─ acc: BatchAccumulator

输入 Queue → controller → 输出 Queue
                 │
                 ├─ acc.add(x)：提交一个样本
                 └─ acc.take()：取出批次结果并清空
```

`acc` 是编译期绑定的平级实例引用，不在 controller 内创建。`out` 默认是 Queue；数据由 emit Rule 入队。两个 `remaining()` 是可供其他事务查询的只读接口，本例不额外设置监控事务。

输入 `3、5、2、7` 时，本批输出为 `17`。第 4 次收集提交后，下次求值才选择 emit；输出成功提交后，下次求值才重新选择 collect。这是语义推演，不是仿真执行证据。

## 2. 组合逻辑的语义

普通表达式绑定是组合值，Module 内的显式状态声明才创建寄存器：

```python
accepted: ac.u8 = 0        # 持久状态
next_accepted = accepted + 1  # 本拍组合值
```

组合值可以用于计算数据、汇总状态、提供 Method 返回值，以及形成 Rule 的启用条件和 Method guard。内部 Rule/Method 可直接引用这些值。

本例中，`next_accepted` 被 collect 使用，`slots_left` 被 remaining 使用。它们都基于本拍旧状态；Rule 提出更新后，本轮组合值不会自动改变。控制条件可以直接写成 `if accepted < 4`，不要求先声明一个布尔变量。

当前采用的简化是：

- Module 顶层组合逻辑只依赖本地旧状态、已连接的 Queue/Reg 输入以及纯计算。
- Queue 输入只能只读观察，不能在组合计算中 pop；不可用条件由编译器传播，不用零代替。
- 顶层组合逻辑不直接或间接调用其他 Module 的 Method。跨模块调用放在 Rule 的事务执行路径中，保留 Method 嵌套调用。
- Method 参数到达后才能进行的计算，仍属于该 Method 的计算，不提前放进 Module 的 CombWork。

因此，`acc.add(x)`、`acc.take()` 出现在 Rule 内；不能在 Module 顶层先调用 `acc.remaining()` 再缓存其结果。这个限制针对实际依赖，也不能通过辅助函数或本地 Method 间接绕过。

## 3. 控制流表达 Rule 的启用关系

Module 顶层的 if/else 是硬件路径条件，不是在仿真时动态创建或删除事务。

```python
if accepted < 4:
    collect(inp)
else:
    out = emit()
```

编译器静态注册 collect 和 emit 两个 Rule，并形成：

```text
collect_fire = (accepted < 4) AND 输入非空 AND acc.add.guard
emit_fire    = NOT (accepted < 4) AND 输出可入队 AND acc.take.guard
```

Rule 调用中的输出绑定也静态建立。因此，即使 `out = emit()` 写在 else 内，输出 Queue 仍然始终存在，Module 可以导出它；else 只控制是否向它提出入队。

`else` 取决于 `accepted < 4` 的真假，不取决于 collect 是否成功。条件为真而输入为空时，collect 和 emit 都不执行，不会自动退回另一个分支。两个独立 if 也不产生隐含优先级。

输入消费、被调用 Method 更新、controller 状态更新和输出操作分别纳入对应 Rule 的原子事务。collect 和 emit 对 accepted 以及累加器状态的写入由 if/else 保证互斥，不构成同拍多写冲突。

如果一般化例子中的条件值带有可用条件，编译器将其合入相关 fire；条件不可用不等于 False，不能误入 else。本例的比较只依赖本地寄存器。

## 4. gfsim：先准备组合结果，再执行事务

每轮求值采用以下组织方式：

```text
清理本轮 proposal 标记
    ↓
所有 Module 的 CombWork：准备需缓存的组合值和条件
    ↓
所有 Module 的 Work：判断 fire，调用 private Rule/public Method
    ↓
必要系统检查完成后，各存储所有者 Xfer 提交
```

在 Work 阶段开始前，所有 Module 的 CombWork 都已完成。外部调用某个 Method 时，不要求该实例自己的 Work 已执行，只要求本轮 CombWork 已完成。

组合缓存是软件求值结果，不是硬件寄存器，不参与 Xfer；每轮在状态和输入稳定后重新准备。当前限制避免了 CombWork 之间经由跨模块 Method 调用产生依赖。

简单的控制表达式可以直接在 Work 中计算，例如 `if (accepted_q < 4)`。CombWork 保证被引用的组合缓存已准备好，不要求所有比较都拆成成员变量；无论是否缓存，读取的都是本轮旧状态。

以下 C++ 省略 Queue 类型定义、复位和调度框架，展示状态归属、缓存与调用职责。`CanPop/CanPush/Head` 读取旧队列状态，`Propose*` 只提出更新。

### 累加器：public Method 读取组合缓存

```cpp
class BatchAccumulator {
public:
    void Begin() { update_enable = false; }

    void CombWork() {
        slots_left = 4 - samples_q;
        can_add = slots_left != 0;
        complete = slots_left == 0;
    }

    void Work() {}  // 没有自主注册的 Rule。

    uint8_t Remaining() const { return slots_left; }
    bool AddGuard() const { return can_add; }
    bool TakeGuard() const { return complete; }

    void AddWork(uint8_t x) {
        total_d = total_q + x;
        samples_d = samples_q + 1;
        update_enable = true;
    }

    uint16_t TakeWork() {
        uint16_t result = total_q;
        total_d = 0;
        samples_d = 0;
        update_enable = true;
        return result;
    }

    void Xfer() {
        if (update_enable) {
            total_q = total_d;
            samples_q = samples_d;
        }
    }

private:
    uint16_t total_q = 0, total_d = 0;
    uint8_t samples_q = 0, samples_d = 0;
    bool update_enable = false;

    // 组合缓存，不是额外硬件存储。
    uint8_t slots_left = 0;
    bool can_add = false, complete = false;
};
```

### 控制器：Work 判断条件，private Rule 提出更新

```cpp
class BatchController {
public:
    BatchController(Queue<uint8_t>& input,
                    Queue<uint16_t>& output,
                    BatchAccumulator& accumulator)
        : inp(input), out(output), acc(accumulator) {}

    void Begin() { accepted_enable = false; }

    void CombWork() {
        next_accepted = accepted_q + 1;
        slots_left = 4 - accepted_q;
    }

    uint8_t Remaining() const { return slots_left; }

    void Work() {
        if (accepted_q < 4) {
            if (inp.CanPop() && acc.AddGuard())
                CollectWork();
        } else {
            if (out.CanPush() && acc.TakeGuard())
                EmitWork();
        }
    }

    void Xfer() {
        if (accepted_enable)
            accepted_q = accepted_d;
    }

private:
    void CollectWork() {
        acc.AddWork(inp.Head());
        inp.ProposePop();
        accepted_d = next_accepted;
        accepted_enable = true;
    }

    void EmitWork() {
        uint16_t result = acc.TakeWork();
        out.ProposePush(result);
        accepted_d = 0;
        accepted_enable = true;
    }

    Queue<uint8_t>& inp;
    Queue<uint16_t>& out;
    BatchAccumulator& acc;  // 引用上层创建的平级实例。
    uint8_t accepted_q = 0, accepted_d = 0;
    bool accepted_enable = false;

    // 组合缓存。
    uint8_t next_accepted = 0, slots_left = 0;
};
```

### 上层组织阶段

```cpp
// 本轮开始：每个状态所有者清理一次 proposal。
inp.Begin();
out.Begin();
acc.Begin();
controller.Begin();

// 第一阶段：所有组合缓存准备完成。
controller.CombWork();
acc.CombWork();

// 第二阶段：即使 controller 先 Work，也能安全使用 acc 的缓存。
controller.Work();
acc.Work();

// 必要系统检查成功后，进入第三阶段。
controller.Xfer();
acc.Xfer();
inp.Xfer();
out.Xfer();
```

这不是逐个模块执行 `CombWork → Work → Xfer`，而是每个阶段覆盖全部模块后再进入下一阶段。跨模块更新在同一个提交边界生效，不能让某个模块提前 Xfer。

## 5. RTL：组合依赖变成连线，路径条件变成 fire

RTL 不需要 CombWork 的软件缓存或调用顺序。组合表达式直接成为线网，Rule 的路径条件和必要 guard 合并为 fire；Method 参数、结果、ready 和 enable 通过上层连接。

RTL 示例保留 `collecting` 中间信号来命名 `accepted < 4` 的比较结果，便于看清两个 fire 的组成。这是后端的表达方式，不要求前端或 gfsim 也声明同名变量，也不增加存储。

下面给出完整模块声明和上层 Method 连线。输入、输出 Queue 以存储接口表示，FIFO 本体不在此展开；复位采用同步清零，仅用于展示初始化。示例中的时钟与复位属于后端接口。

### BatchAccumulator

```systemverilog
module BatchAccumulator (
    input  logic        clk,
    input  logic        reset,

    // Value Method：remaining()，始终可读。
    output logic [7:0]  remaining_result,

    // Action Method：add(x)。
    input  logic [7:0]  add_x,
    input  logic        add_en,
    output logic        add_ready,

    // ActionValue Method：take()。
    input  logic        take_en,
    output logic        take_ready,
    output logic [15:0] take_result
);
    logic [15:0] total;
    logic [7:0] samples;
    logic [7:0] slots_left;
    logic can_add, complete;

    // Module 顶层组合逻辑，同时被 Method guard 和返回端口使用。
    assign slots_left = 8'd4 - samples;
    assign can_add = slots_left != 8'd0;
    assign complete = slots_left == 8'd0;

    assign remaining_result = slots_left;
    assign add_ready = can_add;
    assign take_ready = complete;
    assign take_result = total;

    always_ff @(posedge clk) begin
        if (reset) begin
            total <= 16'd0;
            samples <= 8'd0;
        end else begin
            if (add_en) begin
                total <= total + {8'b0, add_x};
                samples <= samples + 8'd1;
            end
            if (take_en) begin
                total <= 16'd0;
                samples <= 8'd0;
            end
        end
    end
endmodule
```

各 Method 的更新分别由自己的 enable 控制，不把独立 Method 合并成优先级分支。本例调用端保证 add_en 与 take_en 互斥，且分别包含对应 ready；若不能证明写入互斥，编译器应报告多写冲突，不能依赖 RTL 语句顺序解决。外层 reset/else 只表示本例的复位优先行为。

### BatchController

```systemverilog
module BatchController (
    input  logic        clk,
    input  logic        reset,

    input  logic [7:0]  in_data,
    input  logic        in_valid,
    output logic        in_pop,
    output logic [15:0] out_data,
    input  logic        out_ready,
    output logic        out_push,

    output logic [7:0]  remaining_result,

    output logic [7:0]  acc_add_x,
    output logic        acc_add_en,
    input  logic        acc_add_ready,
    output logic        acc_take_en,
    input  logic        acc_take_ready,
    input  logic [15:0] acc_take_result
);
    logic [7:0] accepted;
    logic collecting;
    logic [7:0] next_accepted, slots_left;
    logic collect_fire, emit_fire;

    // Module 组合逻辑，不创建额外寄存器。
    assign collecting = accepted < 8'd4;
    assign next_accepted = accepted + 8'd1;
    assign slots_left = 8'd4 - accepted;
    assign remaining_result = slots_left;

    // Module 的 if/else 变为互斥的路径条件。
    assign collect_fire = !reset && collecting
                          && in_valid && acc_add_ready;
    assign emit_fire = !reset && !collecting
                       && out_ready && acc_take_ready;

    // collect Rule 的效果。
    assign acc_add_x = in_data;
    assign acc_add_en = collect_fire;
    assign in_pop = collect_fire;

    // emit Rule 的效果。
    assign acc_take_en = emit_fire;
    assign out_data = acc_take_result;
    assign out_push = emit_fire;

    always_ff @(posedge clk) begin
        if (reset)
            accepted <= 8'd0;
        else begin
            if (collect_fire)
                accepted <= next_accepted;
            if (emit_fire)
                accepted <= 8'd0;
        end
    end
endmodule
```

前端显式的 if/else 已体现在 collect_fire 的 collecting 与 emit_fire 的 !collecting 中。更新端各自使用 if (fire)，不需要再构造一个 Rule 优先级链。

### BatchPipeline 的平级连接

上层端口连接输入 Queue 的队头与出队请求，以及输出 Queue 的入队端。本例的 remaining 接口保留在子模块上，没有对外新增前端输出。

```systemverilog
module BatchPipeline (
    input  logic        clk,
    input  logic        reset,
    input  logic [7:0]  in_data,
    input  logic        in_valid,
    output logic        in_pop,
    output logic [15:0] out_data,
    input  logic        out_ready,
    output logic        out_push
);
    logic [7:0] add_x;
    logic add_en, add_ready;
    logic take_en, take_ready;
    logic [15:0] take_result;

    BatchController controller (
        .clk(clk), .reset(reset),
        .in_data(in_data), .in_valid(in_valid), .in_pop(in_pop),
        .out_data(out_data), .out_ready(out_ready), .out_push(out_push),
        .remaining_result(),
        .acc_add_x(add_x), .acc_add_en(add_en), .acc_add_ready(add_ready),
        .acc_take_en(take_en), .acc_take_ready(take_ready),
        .acc_take_result(take_result)
    );

    BatchAccumulator acc (
        .clk(clk), .reset(reset),
        .remaining_result(),
        .add_x(add_x), .add_en(add_en), .add_ready(add_ready),
        .take_en(take_en), .take_ready(take_ready), .take_result(take_result)
    );
endmodule
```

## 6. 用一次背压说明两个后端的一致性

以下只记录控制器成功消费的样本与批次输出；假设输入样本按需可用，输出在收满后先阻塞一轮：

| 轮次 | 轮初 accepted / samples | 启用分支 | 条件与结果 | 轮末 total |
| --- | --- | --- | --- | --- |
| 1 | 0 / 0 | collect | 消费 3，提交 | 3 |
| 2 | 1 / 1 | collect | 消费 5，提交 | 8 |
| 3 | 2 / 2 | collect | 消费 2，提交 | 10 |
| 4 | 3 / 3 | collect | 消费 7，提交 | 17 |
| 5 | 4 / 4 | emit | 输出不可入队，全部保持 | 17 |
| 6 | 4 / 4 | emit | 输出可入队，输出 17 并清空批次 | 0 |
| 7 | 0 / 0 | collect | 开始下一批；输入为空则保持 | 取决于新输入 |

第 4 轮不会在 collect 成功后立即执行 emit，因为分支条件读取旧 accepted。第 5 轮不会因为 emit 受阻改为 collect。第 6 轮返回旧 total，同时提出清零；输出入队和清零共同提交。

gfsim 通过组合缓存、条件函数调用和 Xfer 表达这些关系；RTL 通过组合线网、共同使能和时钟沿表达相同行为。二者应由同一份验证后的公共 IR 生成。

## 7. 后续演进

当前先用“本地组合计算全部准备，再执行事务”的组织方式消除 Method 对未准备缓存的依赖。后续更复杂的同拍 Work 依赖，可以在这个基础上完善依赖分析和计算调度，不在此预先限定能力边界。

本文仅新增语义说明，未修改编译器。代码和时序表为说明性示意，不能替代前端编译与双后端执行验证。
