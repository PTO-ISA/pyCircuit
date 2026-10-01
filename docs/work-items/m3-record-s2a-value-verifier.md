# M3-E01 S2A accepted: finite record verifier

状态：done for bounded intermediate packet，2026-10-01。产品提交 `114b1ae1`，base `4c3a4be6`。
按已批准 C2-DECL-R B 完成 read/get/create 义务、actual SSA/handle、跨 unit nominal
解析、严格有限 value.binding/use、单项 selector 和属性落点验证。
独立 Sol APPROVE、Astra CONFORMANT。当前 source 支持范围保持 scalar profile。

- [完整任务与验收](https://github.com/PTO-ISA/pyCircuit/blob/114b1ae1/docs/work-items/m3-record-s2a-value-verifier.md)
- [独立审阅](https://github.com/PTO-ISA/pyCircuit/blob/114b1ae1/docs/reviews/20261001-m3-record-s2a-review.md)
- [命令/原始证据](https://github.com/PTO-ISA/pyCircuit/blob/114b1ae1/docs/gates/logs/20261001-m3-record-s2a/README.md)

19 文件 aggregate：`eaee80593852c78d3a5c5150c42afe47184283d669b72fb5599964914040e3ec`。
新 native 11 pass，reviewer FinalProgram 69 pass；19 binaries/279 cases、
S1/scalar/source/namespace/CLI 129 cases 全通过，零 fail/error/skip/disabled。
SourceMath 91 中 88 pass/三个既有 APInt failure，不宣称整个原生 lane PASS。
同一最终测试源码在 fresh baseline 逐例是 2 个普通失败、9 个 SIGABRT；旧
IntegerType getter 不接纳 record，栈和原始退出结果留档，不改写为普通十一失败。

这是 rule/op seam，source-free rule 重解析成功但整个 record hardware/emit
继续拒绝。Next source/helper producer、动态 guard/check/numeric 组合、
S2C/S3/S4 与完整 record 执行仍开放。下一步按有界实施顺序分解 S2B。
