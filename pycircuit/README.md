# 最小 ACPy → ACIR → GFSim 编译器

独立 Python 编译器；不导入或执行硬件函数，不依赖旧 pyCircuit。完整 Ripes5 的译码、五阶段、前递、stall、跳转、寄存器堆、数据阵列及两个 Signal 都来自 [ACPy 源码](examples/ripes5/model.py)。宿主输入、观察、结束判断和计时复用现有 GFSim runner。

## 两阶段编译

从仓库根目录运行，Python 3.10+，无需第三方 Python 包：

```bash
python3 -m pycircuit compile pycircuit/examples/ripes5/model.py \
  --top CPU --output /tmp/acpy-compiled
python3 -m pycircuit emit /tmp/acpy-compiled/model.acir.json \
  --output /tmp/acpy-emitted
```

两个命令生成 `model.acir.json`、`model.hpp`、`model.cpp`、`ac_support.hpp`。`compile` 在前端之后调用同一个 `emit`。后端输入仅为 JSON；源码路径只用于诊断注释。测试会删除临时 ACPy 源码，然后独立执行 `emit`、编译并运行。

构建、运行完整验收（C++20 编译器和 CMake 3.20+）：

```bash
cmake -S pycircuit -B /tmp/acpy-build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/acpy-build -j4
ctest --test-dir /tmp/acpy-build --output-on-failure
```

构建树的 `compiled/` 与 `emitted/` 分别来自上述两个入口，并分别链接成 `acpy-ripes5-compiled` 与 `acpy-ripes5-emitted`。所有模型使用同一个未修改的 GFSim 调度器。

CTest 同时运行编译器电路测试、生成版输入拒绝测试及既有 GFSim 回归（组件／语义、完整补充电路、安装后独立链接、手写模型三方验收）。正式五方验收比较生成 C++、独立 ACIR 重载 C++、手写 C++、当前 Python 与固定原生 Ripes，覆盖 13 程序 × 缓存开关 × Module 正反序，共 52 配置。原生参考缺失或 commit 不符直接失败。可用 `-DGFSIM_RIPES_REFERENCE=/absolute/path/ripes5-reference` 指定参考，构建方法见 [现有参考说明](../gfsim/experiment/examples/ripes5/README.md)。

```bash
cmake -S pycircuit -B /tmp/acpy-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++ -DGFSIM_SANITIZERS=ON
cmake --build /tmp/acpy-asan -j4
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir /tmp/acpy-asan --output-on-failure
```

## 前端小例子

`@ac.module` 的外层建立固定资源和连接，`@ac.work` 的函数体描述每次激活时的选择与组合计算。简单 Rule 调用也可直接写在 Module 外层。Rule 输出由编译器静态分配，Module 的 `return` 导出资源连接。

```python
from pycircuit import ac

@ac.rule(capacity=1)
def transform(message, bias: ac.var[ac.u32]):
    return message.value + bias

@ac.module
def Transform(input_queue, bias):
    @ac.work
    def work():
        out = transform(input_queue, bias)
    return out

@ac.module
def Example(initial: ac.u32):
    input_queue = ac.queue[ac.u32](initial=initial)
    out = Transform(input_queue, 3)
    return out
```

ACPy 是按 AST 编译的 Python 语法子集；这些文件不是 Python 仿真脚本。顶层普通参数成为不可变 C++ 构造参数。`ac.vector[T]` 用于 ROM、初始数据等长度在模型构造时确定的配置；编译一次即可运行不同程序。完整消息链见 [Source → Transform → Sink](examples/pipeline/model.py)。

- `ac.queue[T](capacity=1, initial=...)`：标量初值形成一个元素；列表初值形成 FIFO 初始序列。若 T 本身是固定数组，匹配 T 的列表就是单个数组 payload。省略 initial 得到空 Queue。
- `q.value` 读取；`q.value = v` 和 `q.value.meta.epoch = v` 生成整值／字段 revise。修改按值捕获；分支外不预读 Queue。
- Rule 的 Queue 参数默认是消息输入，实际读取后提出一次 pop。别名和重复读取按实际 Queue 指针去重。`ac.ref[T]` 参数表示观察／revise 引用；直接捕获的 Module Queue 不消费。Signal 与 Module 控制读取不消费。
- Rule 返回普通值生成 push。`return a, b` 绑定两个固定输出；每个位置可返回 `None`。同一 Module 内，同名 Rule 的多个调用位置共享 RuleId、参数缓存和对应输出；不同 Module 实例独立。
- 嵌套和外部 Rule 使用同一路径；参数可显式声明或由调用推断。Work 中的局部值可显式传参，也可由嵌套 Rule 捕获，后者同样转换成缓存参数。普通值和可变资源身份都参与比较。
- `[ac.queue[T](initial=v) for v in initial_values]` 建立构造后固定的资源阵列；也可使用 Queue 列表或 `range(n)` 推导。运行时索引只读写选中的元素，静态声明覆盖整张表。
- `@ac.signal` helper 构造时绑定 Queue 和配置，`signal.value` 读取结果。helper 的返回类型可推断或注明；不允许读取另一个 Signal。编译器声明全部输入 Queue 和静态 Rule 依赖；GFSim 管理初始化、输出变化过滤和静态下游通知。未选分支和 Queue 数组全部表项仍是输入。Work 将值转换成普通参数时继续使用参数比较。
- `ac.wakeup(delay)` 在 Rule 获准时向所属 Module 请求未来激活，直接调用 `requestWakeup`。纯生产者若要持续产生相同值，应显式安排事件或依赖状态变化。

## 类型、控制流与边界

支持 bool、`ac.u8/u16/u32/u64`、`ac.i8/i16/i32/i64`、`ac.array[T,N]` 和仅包含带类型字段的 struct。字段可有整数／bool 默认值，否则值初始化。普通 helper 使用显式参数和返回类型，支持按值 struct 更新和固定数组局部索引更新。

支持变量、字段、索引、整数算术／位运算／比较、字面量集合成员比较、短路布尔表达式、条件表达式、`if/elif/else`、提前返回、assert 和字面量 `range(...)` 的构造展开。Queue 推导的 iterable 可来自构造参数。所有整数运算明确截断到目标宽度，避免窄整数提升与有符号溢出的未定义行为；带符号 `//` 和 `%` 按向下取整定义。越宽移位得到零（负数右移得到全一）；混合非字面量整数类型需要显式转换。

首版不支持运行时资源声明、运行时循环、异常处理、生成器、Python 对象反射、跨 Signal 依赖、任意位宽整数，以及动态下标的 Queue payload 字段 revise。动态 Queue 阵列索引已支持。资源选择／合并需要保持消息输入或观察引用的访问角色；不同角色应将读取写在各自分支内。普通局部值传递不构成 Module 间动态连接。构造循环中的独立 Module 调用可展开，资源阵列使用列表／推导表达。Queue 推导初值由当前元素和纯 helper 表达。依赖文件采用本地 `from ... import ...`，第一版使用统一的名称表，要求导入定义不重名。遇到不支持的语法报出文件及行号，不执行或静默省略。

首版依照 GFSim 约束，用户负责确保每个 Queue 的 pop／push 各至多一个 Rule 来源，以及同 tick 同一 Rule 的重复调用参数一致。编译器做生成所需的名称、类型和绑定处理，不做额外冲突证明。

## 实现与生成代码阅读顺序

[frontend.py](frontend.py) 做 AST 解析、静态连接与类型处理；[hir.py](hir.py) 保留内部类型化块及分支。构建 HIR 时建立共享谓词与 SSA 值，冻结时线性化为带 guard 的 ACIR 操作，不枚举分支组合。分支合并使用 `select`，提前返回关闭后续路径。保存格式见 [ACIR 格式](acir.md)。

[backend.py](backend.py) 只遍历保存后的 ACIR，统一生成所有 Module、Rule 和 Signal。[support.hpp](support.hpp) 仅提供定宽值计算、固定 Queue 阵列存储和单次候选内的 pop 去重；不维护调度、dirty、订阅或提交。必要输入通过显式检查处理，失败时清理整条候选，所有正常路径调用 `completeRule`。

生成代码建议按以下顺序阅读：

1. `model.hpp` 的强类型 struct、Module 方法／参数缓存、模型资源成员。
2. `model.cpp` 的纯 helper、Signal helper、`Module_*::Work()` 与 `work_*()`；操作旁有源码位置。
3. 模型构造函数末尾的注册、访问声明、操作绑定及 `freeze()`。
4. [共享宿主 runner](../gfsim/cpp/examples/ripes5/runner.cpp) 的输入、快照和计时。`ACPY_GENERATED_MODEL` 仅选择模型头文件／namespace。

## GFSim 生成范式：必要输入

`queue.read` 在 Rule 内生成以下操作，保留原 ACIR guard；不在入口提前检查所有可能输入：

```cpp
if (selected_path) {
    const auto* input = queue->tryPeek();
    if (!input) {
        model.sim.abortRule(rid_rule);
        return;
    }
    value = *input;
}
```

`tryPeek()` 自动登记实际读取，包括读空。`abortRule()` 撤销整条 Rule 已提出的 pop、push、revise 和未来事件请求，并保留实际读取依赖，等待输入改变后唤醒。不能只 `return`，也不能给缺失 payload 补零后继续执行。正常路径仍调用 `completeRule()`；输出满交给仲裁，不能以 `full()` 检查取代完整候选和跨拍反压。

| 位置／操作 | 生成要求 |
| --- | --- |
| Rule 必要读取 | `tryPeek()`，空时 `abortRule()` 后返回；已完成的其他 Rule 不受影响 |
| Module Work 控制读取 | `tryPeek()`，空时直接返回，保留此前选出的独立 Rule |
| Rule `pop`／`revise` | 若前面没有证明目标非空，先检查 `empty()`，空时中止整条 Rule |
| 条件分支、短路、动态 Queue 下标 | 仅在实际操作处检查实际 Queue；不预读未选分支 |
| Signal | 模型显式用 `empty()` 等表达缺输入时的返回值；无保护的必要读取失败仍终止实例 |

Work 期间 Queue current 不变，因此同一 Queue 在无条件路径或相同 guard 下已经通过检查，后续 pop／revise 可省略重复检查。该证明仅限本次生成函数调用，不跨拍缓存，不混同不同动态下标或不同 guard。消息 pop 仍按实际 Queue 身份去重。

Rule 包装层保留 `NeedInput` 捕获作为既有资源 helper 调用的兼容边界；直接生成的必要 Queue 操作不通过异常等待输入。普通 helper、Signal 和构造表达式的异常含义没有改变；其他运行错误继续报告。GFSim API、ACIR 格式和调度语义均不变，独立重载 ACIR 使用同一范式。

[编译器测试](tests/test_compiler.py) 覆盖部分效果撤销、读空依赖与重试、分支进度、Module 选择边界、独立 pop／revise 和 Signal 空输入。Linux／ELF 测试通过链接器包装 `__cxa_throw`，断言正常缺输入不抛 `NeedInput`，不改 Queue 或调度器实现。

## 证据和计时

逐拍输入、完整 JSONL、首个差异及相邻周期保存在 `examples/ripes5/output/<构建类型>/`。已记录的验收、代码量和测试摘要见 [results.json](examples/ripes5/results.json) 与 [报告](examples/ripes5/report.md)。这些文件与性能数字对应各自记录的源码及二进制指纹。

当前三方同批对比使用五个至少十万拍的有界长程序，统一 GCC 14、C++20、`-O3 -DNDEBUG`、架构选项及关闭 LTO。每版先执行 K=1024 拍，再只计时到 marker 提交的固定 N 拍；三方各预热一次，串行轮换采样 15 次。原生版关闭观察通知，并逐拍核对通知开关前后轨迹。构建、完整验收、测速和报告命令见 [Ripes5 测速说明](examples/ripes5/README.md)，当前结果见 [三方报告](examples/ripes5/benchmark-report.md)。

历史 `timing.json` 保留原短程序、七次采样和含结束检查／首次 Signal 初始化的测量口径；新样本单独保存在 `timing-fixed.json`，不跨批计算速度比。

## 小型乱序 CPU

[ooo 示例](examples/ooo/README.md) 使用现有 ACPy 实现 8 项 ROB、寄存器重命名、
整数／访存双发射和提交时分支恢复。提供汇编运行入口、独立顺序解释器，
以及缓存开关、Module 正反序、ACIR 独立重载的逐拍验收。
随上述 CMake 工程构建，测试名以 `acpy-ooo-` 开头。

### Queue 版 skyzh 参考模型（集成受阻）

[skyzh_ooo](examples/skyzh_ooo/README.md) 尝试用 Queue 空满表示保留站占用，并用 pop/push 表达流水与反压。
目前已有 RV32I 执行组件、固定原生参考和完整程序检查；CPU 顶层因输出 Queue 引用、独立输出容量的表达缺口而暂停，
尚未完成 CPU 端到端验收或双模型 benchmark。问题及最小复现见 [findings](examples/skyzh_ooo/findings.md)。
