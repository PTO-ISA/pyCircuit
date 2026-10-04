# ACPy → MLIR → GFSim

Python 只解析 AST、静态构造和类型推导，不执行硬件函数。行为保存为注册的 ACIR dialect 与标准 `func`、`arith`、`cf` 操作；C++ pass 展开 GFSim 事务语义并转换为 EmitC，使用 MLIR 的 C++ exporter 生成代码。旧 JSON ACIR、HIR、guard 展平和 Python 行为代码生成器已移除。

## 构建与两个入口

需要 Python 3.10+、CMake、C++20 和 LLVM/MLIR 22.1.8。当前开发环境：

```bash
export LD_LIBRARY_PATH=/home/lc/opt/gcc14/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
cmake -S pycircuit -B /tmp/acpy-mlir-build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/home/lc/opt/pycircuit-dev/bin/cc \
  -DCMAKE_CXX_COMPILER=/home/lc/opt/pycircuit-dev/bin/c++ \
  -DMLIR_DIR=/home/lc/opt/llvm-22.1.8/lib/cmake/mlir
cmake --build /tmp/acpy-mlir-build -j6
export ACPY_MLIR_COMPILER=/tmp/acpy-mlir-build/mlir/acir-compile
python3 -m pycircuit compile pycircuit/examples/ripes5/model.py \
  --top CPU --output /tmp/acpy-compiled
python3 -m pycircuit emit /tmp/acpy-compiled/model.acir.mlir \
  --output /tmp/acpy-emitted
ctest --test-dir /tmp/acpy-mlir-build --output-on-failure -j4
```

`compile` 与 `emit` 共用 C++ 后端，输出 `model.acir.mlir`、`model.emitc.mlir`、`model.hpp`、`model.cpp`、`ac_support.hpp`。`emit` 只读取保存的 MLIR；删除源码后仍可生成。`--no-opt` 关闭 canonicalize/CSE，保留同一语义展开与转换路径。

原生 Ripes 参考必须存在且符合固定版本；可用 `-DGFSIM_RIPES_REFERENCE=/path/to/ripes5-reference` 指定，构建见[参考说明](../gfsim/experiment/examples/ripes5/README.md)。

## 前端范式

```python
from pycircuit import ac

@ac.module
def Example(initial: ac.u32):
    input = ac.queue[ac.u32](initial=initial)
    out = ac.queue[ac.u32](capacity=4)

    @ac.rule
    def transform(message, bias: ac.var[ac.u32]):
        return message.value + bias

    if out.empty():
        out = transform(input, 3)
    return out
```

Module 函数体直接表达 Work，不再支持 `@ac.work`。前端先收集资源、子 Module、Signal、Rule 输出及固定连接，再编译运行逻辑。条件内绑定的输出始终存在，可在调用前或 Rule 内引用；显式 Queue 决定容量、初值，隐式输出默认容量 1、初始为空。同 Module 的同一 Rule 共享身份、固定输出及参数缓存。

Queue/Signal 参数保留资源身份；Module 的 `ac.var[T]` 运行时输入必须绑定 Signal。其他构造参数保持不可变。Signal 的显式输入只接受 Queue（包括固定 Queue 阵列），配置通过外层 Module 捕获；输入变化、输出变化过滤、静态通知遵循 GFSim。

Rule 的消息输入实际读 payload 才生成 pop；返回 payload 生成 push；给 Queue 数据赋值生成 revise。观察捕获资源及 `ac.ref[T]` 参数不消费。别名重复消费在运行时按 Queue 身份去重；所有效果原子提交，读空撤销当前 Rule 的部分效果。Module 读空只退出当前 Work；Signal 无保护读空仍是模型错误。

支持 bool、8/16/32/64 位有符号及无符号整数、结构体和定长 payload 数组。支持分支、短路、提前返回和运行时 `for i in range(stop)` / `range(start, stop)`；运行扫描保留 CFG 循环。定宽算术显式截断，带符号除法向下取整，越宽移位按 ACPy 规则处理。

资源数组写作 `ac.array(ac.queue[T], shape=(N,), capacity=..., initial=...)`，首版一维，长度构造时固定，可动态索引。省略 initial 时各元素为空；纯初始化 helper 接收元素下标。既有 Queue 列表／推导仍可用于固定构造。支持普通聚合值的字段与索引更新，以及 `q.value.field[index] = value`：索引和值在 Work 捕获，Xfer 修改旧队尾。嵌套 tuple/list 输出逐个叶子绑定；`None` 只省略对应 push。

当前不支持运行时资源构造、while/break/continue、跨 Signal 依赖、任意整数位宽或多维资源数组。源码报错含位置。不同消费者负责同一阵列的不同范围时，用固定元素列表连接各自端口；静态分析不推导任意整数路径条件。用户仍需满足 GFSim 每个 Queue 的 pop/push 唯一来源约束，以及同 tick 同 Rule 重复调用参数一致的约束。完整语义见 [ACPy spec](../acpy/spec.md)。

## Dialect 与 pass

- [ACIR.td](mlir/ACIR.td)：ODS 操作、类型、约束及标准内存效果；[Dialect.cpp](mlir/Dialect.cpp) 补充验证。CMake 通过 `mlir-tblgen` 生成 C++ 注册代码。
- [Passes.td](mlir/Passes.td)：三个注册 pass；[Compiler.cpp](mlir/Compiler.cpp) 实现资源传播、生命周期展开、EmitC 转换和统一静态布局。
- [frontend.py](frontend.py) / [mlir_text.py](mlir_text.py)：静态构造、类型明确的 SSA/CFG 与 MLIR 文本；[backend.py](backend.py) 仅调用 C++ 工具。
- [support.hpp](support.hpp)：定宽计算、保持身份的 Queue 阵列视图、资源适配、pop 去重；不实现另一套调度器。

可以独立观察 pass 输出：

```bash
/tmp/acpy-mlir-build/mlir/acir-opt /tmp/acpy-compiled/model.acir.mlir \
  --acir-analyze-resources --canonicalize --cse \
  --acir-lower-gfsim --acir-convert-to-emitc -o /tmp/model.emitc.mlir
```

Queue `read/query` 同时观察状态和登记依赖，声明 `MemRead + MemWrite`，没有 `Pure` 或可推测执行属性。必要读取检查保留在实际分支中；Rule 入口比较参数并 `beginRule`，正常出口 `completeRule`，读空出口 `abortRule`。详见 [ACIR 保存格式](acir.md)。

## 验收与性能

CTest 覆盖小电路、ODS 拒绝非法 IR、优化不能删除读取依赖、删源码后重载、Ripes5 五方逐拍对照、既有 OoO，以及[完整 Queue CPU](examples/skyzh_ooo/README.md)。Queue CPU 的 5 个完整程序逐条提交对照独立 RV32I 解释器，比较优化开关、缓存开关、Module 正反序和直接／重载生成；另有写回阻塞变体使 ROB 确实填满。

Sanitizer 使用同一工程：

```bash
cmake -S pycircuit -B /tmp/acpy-mlir-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=/home/lc/opt/pycircuit-dev/bin/cc \
  -DCMAKE_CXX_COMPILER=/home/lc/opt/pycircuit-dev/bin/c++ -DGFSIM_SANITIZERS=ON
cmake --build /tmp/acpy-mlir-asan -j6
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir /tmp/acpy-mlir-asan --output-on-failure -j4
```

重构前快照及构建分别保存在 `/tmp/acpy-mlir-baseline`、`/tmp/acpy-before-mlir`。可重复的新旧同模型对比：

```bash
python3 -m pycircuit.benchmark_migration \
  --baseline-source /tmp/acpy-mlir-baseline --baseline-build /tmp/acpy-before-mlir \
  --build /tmp/acpy-mlir-build --cxx /home/lc/opt/pycircuit-dev/bin/c++ \
  --output /tmp/acpy-migration-benchmark
```

结果报告编译时间、生成字节数、峰值 RSS、固定 tick 区间耗时和架构吞吐。历史 Ripes5/OoO 报告保留各自版本指纹，不能视为本次 MLIR 的结果；本次证据见 [MLIR 验收记录](mlir/results.md)。
