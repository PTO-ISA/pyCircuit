# RFC: Tier 分层标注 —— 面向 3D 堆叠(TAO 细粒度逻辑折叠)的源码级层指派

<!-- markdownlint-disable MD032 -->

**Status:** Proposed
**Scope:** pyCircuit 6 cycle-aware API、MLIR 属性、Verilog 发射、sidecar 交付物
**关联文档:**
- 需求来源:`agentic_tao_physical_design_flow.md` §2.4(TAO 后端流程文档,位于 agentic_circuit_optimizer 项目)
- 前置概念：`docs/v6_PyCircuit_Specification.md`（CycleAwareSignal 与周期元数据）、`docs/rfcs/pyc6-decisions.md`（稳定层次命名）

---

## 1. 动机

TAO 物理设计流程把设计**细粒度折叠**到 2–3 层垂直堆叠的裸片(tier)上:同一条组合路径的前半段在 tier 0、后半段在 tier 1,层间通过混合键合点连接,以缩短关键路径线长并提升单位投影面积的晶体管数。分割方案的质量直接决定最终 PPA,而分割器(路径感知网表分割 EDA 工具)需要**来自前端的意图与热启动**:哪些模块预期落在哪层、哪些信号是期望的跨层切换点。

正如程序员用 `domain.next()` 在源码里注入时钟边界,本 RFC 提议让程序员(以及 agentic 优化循环中的智能体)在 PyCircuit 源码中**标注分层边界**:tier 成为信号携带的第二种元数据,与 `.cycle` 并列。

**术语约定:分层维度统一用 tier(裸片层),不用 layer**——后者在物理设计语境里指金属布线层(metal layer)。API 命名(`tier=`、`tier_lock`、`jump_tier`)与所有报告字段遵循此约定。

## 2. 语义基石:tier 是零电路效应的元数据

与周期元数据的对照是理解本扩展的钥匙:

| | `.cycle`(已有) | `.tier`(本提案) |
|---|---|---|
| 语义地位 | **行为语义**:`domain.next()` 切错位置行为就变 | **物理提示**:不产生、不修改任何硬件 |
| 传播规则 | max 规则 + 自动周期平衡(插 DFF,改变电路) | 继承规则,零电路效应 |
| 验证影响 | 必须过等价门(锁步对拍等) | LEC 恒过,功能验证无感 |
| 谁可以改 | 设计决策,须走等价门裁决 | 可自由变异,代价/收益只体现在后端 PPA |

两套元数据的传播机制共存于同一次 elaboration,互不干扰。`jump_tier()` 与 `domain.next()` 的对照同理:前者纯注释性(涂改元数据),后者语义性(推进时间)。

这个不对称的直接推论:**tier 标注是 agentic 循环中最安全的一类旋钮**——智能体可以自由修改它而不触发任何功能验证,因此应该在语法上暴露得尽可能细。

## 3. 语法提案

### 3.1 标注形式一览

```python
from pycircuit import cas, compile_cycle_aware, jump_tier, wire_of

def core(m, domain):
    # ① 信号定义处显式声明
    a = cas(domain, m.input("a", width=8), tier=0)
    pc = domain.signal(width=64, name="pc", tier=0)

    # ② 隐式推断(缺省行为):不标注则从输入表达式继承
    s1 = a + 1                      # s1.tier == 0(唯一输入在 tier 0)

    # ③ 表达式级强制跳层:声明"此处期望一个键合点"
    s2 = jump_tier(s1 * 3, to=1)    # s2 落在 tier 1;不产生任何硬件

    # ④ 锁定:EDA 分割器不得改写
    hot = cas(domain, m.input("b", width=8), tier=1, tier_lock=True)

    # ⑤ 未指派:交给分割器全权决定
    tmp = cas(domain, m.input("c", width=8))   # tier=None

    m.output("y", wire_of(s2 + hot + tmp))

# ⑥ 模块/函数级缺省:端口与内部信号的缺省层
outs = domain.call(alu, inputs={...}, tier=1)
```

| # | 标注位置 | 语法 | 语义 |
|---|---|---|---|
| ① | 信号定义处 | `cas(..., tier=0)`、`domain.signal(..., tier=1)` | 显式声明该信号所在 tier |
| ② | 不标注 | — | 由输入表达式推断:全部(或多数)输入同 tier 则继承;混层时取主导输入方向(键合点最少);推断结果记为弱指派 |
| ③ | 表达式边界 | `s = jump_tier(expr, to=2)` | 强制结果信号落在指定 tier,等价于声明"此处期望一个键合点";零硬件效应,归因链登记 |
| ④ | 定义处修饰 | `tier=1, tier_lock=True` | 硬约束,下游 EDA 必须服从 |
| ⑤ | `tier=None` | — | 自由信号,分割器全权优化 |
| ⑥ | 模块/调用级 | `domain.call(fn, ..., tier=1)` 或模块装饰参数 | 该模块端口与内部信号的**缺省 tier**;内部标注可覆盖 |

### 3.2 强度三态与 EDA 契约

每个信号的 tier 指派携带强度,下游分割器的覆写权限逐级递减:

1. **free**(未指派):分割器全权优化;
2. **hint**(显式/缺省/推断指派):热启动与软约束——分割器**可以改写**,但每次改写必须输出结构化 diff(信号 ID、原 tier → 新 tier、预估收益),不允许静默改写;
3. **locked**(`tier_lock=True`):分割器必须服从;不可满足时报错而非违反。

分割器结果**不回写源码**,写入以稳定 ID 为键的 sidecar tier 指派表;上游(人或智能体)读取"标注 vs 实际指派"diff 后自行决定是否把结果固化回源码。这构成人(意图,locked/条款)、智能体(迭代,hint 调整)、EDA 算法(全局优化,free/hint)三方在同一张表上的分权协作协议(详见 TAO 文档 §2.4"human in the loop 与 agent in the loop")。

## 4. 前端语义细则

1. **传播规则(elaboration 期):**
   - 运算结果的 tier:所有输入同 tier → 继承;混层 → 取主导输入的 tier(实现可选:多数票,平票取编号小者),强度记为推断(hint);
   - `domain.signal()` 前向声明的反馈信号:tier 在声明处确定,`<<=` 赋值侧不改变 tier(反馈环不因赋值表达式的 tier 而漂移;如需跨层反馈,用 `jump_tier` 显式表达);
   - 自动周期平衡插入的对齐 DFF:继承其驱动信号的 tier(平衡寄存器不引入额外跨层)。
2. **`jump_tier(expr, to=k)`:** 返回一个新的 CAS,`.tier == k`、`.cycle` 不变、底层 Wire 不变(或为保稳定 ID 插入一个 `pyc.alias`);`to` 必须是编译期常量。
3. **合法性检查(elaboration 期告警,非错误):** tier 取值超出声明层数报错;locked 信号之间的直接组合依赖若形成"每级门都跨层"的病态模式,给出统计告警(键合点预算问题留给分割器定量裁决)。
4. **JIT/eager 双路径:** tier 与 cut_after 类编译期特化参数正交;两条编译路径都只是把元数据挂到信号上,无控制流影响。`tier=`/`jump_tier` 在 JIT 路径经由 CAS 运算符委托机制透明工作(参见 jit.py 的 cycle-aware interop)。

## 5. IR 与下游交付

1. **MLIR:** 新增可选属性 `pyc.tier`(整数)与 `pyc.tier_strength`(`"hint" | "locked"`),挂在产生该值的 op 上;模块级缺省挂在 `func.func` 属性 `pyc.tier_default`。无标注即无属性(向后兼容)。
2. **Verilog 发射(三条冗余通道):**
   - 标准属性 `(* pyc_tier = 1, pyc_tier_locked = 1 *)` 挂在 wire/instance 声明上(部分综合流程可透传);
   - 层次化命名编码(strict hierarchy 已保证模块名存活综合),锚点挂在 `dont_touch` 边界;
   - **sidecar tier 指派表(主通道):** 稳定 ID → (tier, strength) 的独立文件(建议 JSON Lines),与网表并行交付、并行版本控制。三通道不一致构成流程告警。
3. **C++ 仿真后端:** 忽略 tier(功能语义无关);可选地在 DFX/probe 元数据中携带,便于按 tier 聚合功耗/活动统计。

## 6. 实现草图与阶段

- **阶段 1(前端元数据,~1 人月):** `CycleAwareSignal` 增加 `_tier`/`_tier_strength` 槽位与传播;`cas()`/`domain.signal()`/`domain.call()` 增加 `tier=`/`tier_lock=` 参数;新增 `jump_tier()`;eager 路径 MLIR 属性发射。
- **阶段 2(JIT 与发射,~1 人月):** JIT 路径透传(依托既有 CAS 运算符委托);Verilog 属性/命名通道;sidecar 表导出器。
- **阶段 3(工具对接):** 分割器读入三态表、输出改写 diff 的格式冻结;与 agentic 循环中间件联调。

**兼容性:** 全部参数可选、缺省 `None`,对现有设计零影响;不改变任何现有 IR 语义。

## 7. 开放问题

- 异质层(逻辑层 + 存储层)下,宏单元(SRAM)的 tier 指派是否需要独立的语法(如 `m.mem(..., tier=...)`)与更强的缺省锁定;
- 混层输入的推断规则(多数票 vs 键合点代价最小)是否需要做成可配置策略;
- 层次化编译缓存:同一子模块以不同 `tier` 缺省实例化时,是否进入特化缓存键(倾向:进入,与其他编译期参数一致);
- sidecar 表的稳定 ID 规范与 `pyc6-decisions.md` 中 DFX 路径命名的统一。
