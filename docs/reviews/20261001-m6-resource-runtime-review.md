# M6-04 资源/有限长运行独立审阅

日期：2026-10-01。实现 `m3_e01_oracle` / Luna high；独立测试
`m6_04_measurement_tests` / 另一 Luna high；review `m6_resource_independent_review`
/ Sol high，未写被审阅代码。基线 `9ff015a2`。

结论：**APPROVE，有界开发测量工具及四个selected尺寸证据**。
五文件 candidate aggregate：
`4418364a20201086fd83aa9c085625025705951e88ad81576232b201ebc61cf7`。
最终 selected report：
`8b013fc93bc1309710230ac64525a8725d63026719da86f87ad291915aa2c274`。

独立review probe发现并修复：超时leader退场仍残留忽略TERM的child；非有限
interval/timeout导致deadline失效与NaN JSON；cleanup deadline负sleep race；
旧report未绑定实际untracked工具字节；source group仅总数检查可掩盖owner交换、
缺失SV/glue/重复路径；consumer未校验API table size与七callback。对应独立
regression保留，候选在最终Black24.10/Ruff check后重新冻结和重新测量。
未用stale selected/smoke report证明新字节。

最终review确认4cases、32成功phase、24次有限CABIrun，exact input hashes、
producer/source-owned CPP/RTL inventories、containedregular所有receipt文件与
每source实现路径、三拍CPP oracle、实际end epoch/TERMINATED/gauges/error/
reset replay。独立39tests通过，PM post-format39tests亦通过；无timeout失败。
report/source/test/input hashes符合当前候选；没有publicAPI/schema或IR identity变化。

Resource数据是POSIX sampled process-group RSS sum，不是exclusive物理内存或
精确峰值；共享页重复、短进程漏峰、detachedgroup、ps overhead均披露。
速率包括CABIcallback/执行器或process启动采样成本，不是裸kernel或steady state。
shared-1初段与PM testworkload约12秒重叠，数字是exploratory，不建立阈值或速度承诺。
RTLemit/树清单已验证；未在本包执行RTL仿真或测其吞吐，完整尺寸矩阵仍开放。
[完整证据](../gates/logs/20261001-m6-resource-runtime/README.md)留档。
