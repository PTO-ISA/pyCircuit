# pyCircuit benchmark ports

本目录保存题目来源、分类、pyCircuit 实现和可复现的评测入口。前端写成
普通的顺序程序：Module 拥有变量，Rule 可以定义在 Module 内或外，通过调用绑定
实际状态。单值直接返回，Struct 只组合多个相关字段。clk/reset、寄存器 q/d
以及提交控制不进入作者代码；外部名称和物理接口属于编译器导出/测试适配边界。

## 当前结果与边界

| 数据集 | 已取得的题目库存 | 当前执行 |
| --- | --- | --- |
| VerilogEval 2023 | Human 156，Machine 143；两个 prompt 变体不可相加当成独立问题 | 本轮88/88已迁移题通过（全库88/156）；79项strict、9项显式if_one；68项尚未移植 |
| VerilogEval v2 | spec-to-RTL 156，code-completion 156；与旧版有重叠 | 已分类，未运行 |
| CVDP v1.1.0 | 872 条：749 generation +123 comprehension，7个配置 | 题目已下载；原容器/形式/PPA协议未运行 |
| HWE-Bench | 417 个真实修复任务，6个仓库 | 已下载和分类；没有把模块重写当成 fail-to-pass 修复分数 |
| RealBench | 60个模块、4个系统配置；描述为已下载的 GPG 加密文件 | 本机缺 gpg；未解密、未运行原协议 |
| ChipVerilog | 64个模块题目 | 已下载和分类；未完成 pyCircuit 移植 |
| ResBench | 56题、12类 | 43/56 通过未修改测试台的 Icarus 功能检查；13项明确 unsupported；Vivado资源分数未运行 |
| RTL-Repo | train/test Parquet 均已完整下载并通过 LFS SHA校验 | 本机无 Parquet reader，尚未逐行编目；不以文档声称的4098条冒充已解析 |
| NotSoTiny 25-12 | 1114个任务，采用CVDP格式聚合文件 | 已下载和分类；与原格式不重复计数；TuRTLe评测未运行 |
| HWFix/issue_benchmark | 500条原始 issue 记录 | 已下载和分类；仅将body/title与files.base作为任务上下文，description/modified/patch为参考答案；未确认可执行修复评分协议 |
| CirFix | 63个buggy variant，21个基础设计 | 已下载和分类；原repair协议未运行 |
| HSCo-Bench | 10个应用工作区 | 已下载和分类；ESP/Vivado/Catapult/FPGA流程未运行 |

这是**人工/agent辅助移植通过率**，不是独立抽样的LLM pass@1，也不是论文排名。
不同任务协议不合并成一个分数。编译成功、未运行、工具缺失、适配不支持均不算PASS。
C++仅完成emit，本轮功能判定使用Icarus12.0。ResBench上游使用Vivado，因此这里的
功能检查是补充证据，不是其官方Vivado/PPA评分。`bugs_mux2` 及两个ResBench算术例
有公开oracle辅助修订，详见 findings，不能宣称纯prompt首次生成。

## 内容

- `sources.lock.json`：官方仓库/数据集URL、精确commit、题目文件与上下文清单。
- `upstream/`：本地第三方快照，含原LICENSE/NOTICE；不加入产品Git历史。
- `catalog.jsonl` / `inventory.json`：逐题类别、题目指针、哈希、协议和移植状态。
  当前已编目3771条（含明示的密文描述指针），包含明确标注的变体，不能作为3771个独立问题的声称。
- `solutions/`：可读pyCircuit源码及外部接口映射元数据，不包含参考RTL替代品。
- `results/`：本轮逐题结果与汇总；完整产物和日志在忽略的 `runs/`。
- `runs/four-state-verilogeval-final/`：2026-10-19四态候选的88通过记录；
  `results/`保留对应逐题结果及候选工具哈希，旧运行目录仍保留此前基线。
- [COMPILER_FINDINGS.md](COMPILER_FINDINGS.md)：前端、MLIR、codegen、适配和数据问题。

## 取得固定版本

```sh
python3 benchmark/fetch.py
# 当官方HF传输不可达时，显式选择镜像；commit和LFS内容hash保持不变：
python3 benchmark/fetch.py --hf-endpoint https://hf-mirror.com
python3 benchmark/inventory.py rebuild
```

已有目录必须匹配锁定commit；不会重置用户修改。下载只取得文件，不执行上游安装
脚本、容器或模型。Git验证固定提交；LFS内容严格核对已提交pointer的SHA256和大小。
完整修复仓库/重型上下文可用 `--include-context` 请求，不默认拉取约200GB的HWE镜像。
RealBench官方说明允许 `make decrypt`，但本次未执行，密文不冒充可用明文。

许可证随各自快照保留。不要把框架LICENSE推断为所有第三方设计的LICENSE；
ChipVerilog有独立第三方条款，ResBench/HWFix等许可信息不完整。下载不等于再分发授权。

## 运行已有实现

先按本仓文档构建当前checkout的CompilerDev工具链，并使用具有pyCircuit Python
依赖的解释器。默认工具目录是 `.pycircuit_out/toolchain/build`，可用 `--build` 指定
同一checkout的构建。不要引用另一工作树的二进制。

```sh
python benchmark/run.py --suite verilogeval --output benchmark/runs/my-verilogeval
python benchmark/run.py --suite resbench --output benchmark/runs/my-resbench
# 先确认测试能检测错误：只反转候选输出，不改上游测试台。
python benchmark/run.py --suite verilogeval --tasks andgate zero \
  --negative-control --output benchmark/runs/my-controls
```

输出目录必须是新目录。每个实现经历原有 `compile → link → emit cpp/verilog`，
两种emit共用一个final ACIR。RTL adapter只匹配模块名、端口名和packed位序，不生成
题目的逻辑，不使用canonical solution替代DUT。测试台逐字来自固定上游版本，
原刺激长度和成功判据保留。只有明确完成原检查才通过；负对照必须产生行为失败，
不能靠编译错误证明oracle有效。

接入使用编译器的 `ports.json` 和 `root_rtl_name`，不再从生成RTL文本猜测端口类型、
名称或domain角色。声明外部domain的时序题目请求 `--rtl-mode physical`，其余默认
导出。`PYCIRCUIT_PHYSICAL_FAULT` 始终判失败，即使原TB同时打印成功。未知reset的
协议由题目映射的 `domain.reset_semantics` 显式选择，链接为共同final策略，
并记录在逐题结果中；原Verilog `if (reset)` 的同步复位题使用 `if_one`。
其余题保持 `strict`。两种策略分别统计，不能通过改TB、注入reset或屏蔽故障迎合测试。

上游v1 Python执行器的实际执行区被注释，且有全局pkill清理；这里直接运行同一原始
测试台及其 `Mismatches` 判据，超时只终止本任务进程组。没有运行上游全局清理。
ResBench补充检查使用同一测试台及独立完整成功行，不把缺少成功标记的退出0算通过。

## 后续移植要求

先读 [.codex authoring skill](../.codex/skills/pycircuit-authoring/SKILL.md) 和
[前端手册](../docs/development/agent-frontend-guide.md)。普通typed `def`用于纯计算；
Rule用于事务或状态转移。全局Rule通过参数接收值/实际状态，不隐式捕获调用者局部。
业务接口多个相关字段才组合成Struct；一个值不包Struct或singleton tuple。

不要从上游Verilog端口表照抄clk/reset到Python，不写q/d接线。固定外部名称应在
通用codegen导出接口处理；当前adapter是透明的评测接入，不是新的编译路线。
出现managed phase/check/commit端口就明确标为不兼容，不固定commit许可、不增加
等待周期、不偷偷补reset来迎合测试。原始失败和最小复现进入findings。
