# ACPy 表达记录

以下两项是旧编译路径中的表达问题，MLIR 迁移保留最小复现并加入执行回归。

## F1：分支内首次引用构造常量（已修复）

自然写法在 Signal 循环内比较 `ready and value == wanted`。旧 guard 展平路径把首次受条件控制的常量初始化错误复用，第一项未启用时后续项看到默认值，曾输出 `natural=0 hoisted=2 expected=2`。

[guarded_constant.py](repro/guarded_constant.py) 已迁移至 Signal 只接受 Queue、配置由构造期捕获的范式。新的 SSA/CFG 导入在函数入口物化构造值，运行时循环保留实际控制流。自然写法和历史显式转换写法都应返回 2，runner 与编译器回归会同时检查两者。

## F2：动态 payload 字段 revise（已支持）

[dynamic_payload.py](repro/dynamic_payload.py) 的 `rows.value[index.value].done = True` 现在通过通用 `acir.revise` path/indices 转换。Work 捕获下标与新值，Xfer 更新旧队尾；回归检查只修改指定元素。

当前 OoO 模型仍保留独立 Queue 阵列表达，状态拆分是模型选择，不再是字段 revise 的编译限制。

## 重现

```bash
export ACPY_MLIR_COMPILER=/tmp/acpy-mlir-build/mlir/acir-compile
export ACPY_CXX=/home/lc/opt/pycircuit-dev/bin/c++
python3 -m unittest pycircuit.tests.test_compiler.CompilerTests.test_historical_expression_regressions -v
```

本模型仍不提供 trap/CSR；64 位 epoch/序号溢出不在有界测试范围内。访存使用保守顺序，没有 Store 转发、缓存或访存推测。
