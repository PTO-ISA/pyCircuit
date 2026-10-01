# M6-03 首次发布恢复独立审阅

日期：2026-10-01。独立测试作者：`m5_semantic_oracle_migration` / Luna high。
独立 reviewer：`m6_03_independent_review` / Sol high；无被审阅 source/test 写入。
产品基线 `5b0d610d798fca8be7188ec10674355e07dbab47`。

结论：**APPROVE，有界 macOS/POSIX C3 first-publication tests**。

| 审阅文件 | SHA-256 |
| --- | --- |
| tests/system/test_m6_publication_process_recovery.py | ddf8282d495386c09f3be1e201c5befcbbc70185d4a0d7c8e27d18ab5e20d73a |
| tests/integration/agentic-circuit/m6-publication/crash_runner.py | 3eca8995532644fa2e67cdff1bcf7e2c526ac2aa2a4feef883bed3ed949e72bf |

final/CPP/RTL × 五可达点为 15 个真实 first-publish self-SIGKILL 场景。
assert 精确 point marker/-SIGKILL、phase/owner/had_previous、stage/destination/
previous bytes、control owner/lock identity；after_previous_saved 是不可达 N/A。

final 的 public emit reader 直接证明 commit 前 rollback 到 absence，且拒绝
原因是 missing/unsafe stable artifact；committed final 保留 bytes 并产生 expected
verified CPP。CPP/RTL 的 public writer 在恢复后 next preparing hook 暂停，先
检查八个 precommit row 的 absence 或两个 commit row 的原 bytes，再 kill/recover
follow-up writer并正常重发。这十次额外恢复没有误计为 15 首次场景。

独立 reviewer 在显式 fresh `.pycircuit_out/m6-03-root` 上 Python 3.12 实跑：
5 passed in 20.41s；最终格式化后两文件哈希稳定，product source diff 为空。
原 24 replacement、5 source-first、3 reentrant recovery 和 waiting-lock case全保留。
PM Python 3.14 最终 gate：5 passed in 23.56s；publication/filesystem/driver unit
regression：131 passed，3 个 Windows-only skip。测试迭代中 canonical owner 和
specific diagnostic 的断言由作者修正；没有发现或修改产品协议缺陷。

范围：真实进程/锁恢复的 Darwin/POSIX evidence，不证明 Windows、网络文件系统
或掉电持久性。没有新增 product fault flag、receipt/journal field、IR/CLI/runtime ABI。
[完整日志与 manifest](../gates/logs/20261001-m6-first-publication/README.md)留档。
