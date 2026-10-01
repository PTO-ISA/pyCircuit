# M3-E01：record final 投影设计/独立 oracle 包

状态：design complete，精确修订 B 已批准；产品实现/验收尚未完成。日期：2026-10-01。
依据：[M3-P01 准入结论](m3-contract-admission.md)及独立 Astra 核对。
目标用例：一个 portless 模块，immutable 两字段 record `(lo, hi)`，初值 `(3,17)`，
基于同拍旧 Q 提出 `(hi, lo+1)`，Xfer 后 `(17,4)`、再 `(4,18)`；reset 重跑。
不引入非空 static、集合 shape、外部 typed DUT 或一等 SYSTEM/role。

## 已批准行为与需要冻结的边界

C1/C2/R1 的 nominal finite record、完整构造/不可原地修改、helper path/valid/
展开、普通 reg whole-record D/E/reset/唯一 StateID 的硬件语义保持，不整体重批。
当前 C2-DECL 的精确 final envelope 只允许 scalar alias/constant，保留 record/
helper preflight rejection；源码可构造/header 可读取不证明 final/CPP/RTL 已支持。

本包只对缺失的 final nominal-record declaration projection 给精确增补：

- 哪些已有 record 字段/owner/origin/order/nominal references 留在 source-owned final unit。
- executable helper body 移除后，constructor reference 的确切处置及 reparse resolution。
- reset/value-use/proof 能否用现有完整形状表达 record；能复用的算法标 implementation-only。
- 若新增/改变 op/type/attr/IR/header/generated declaration 形状，逐字段列 before/after，
  不默认添加新 primitive，不删除当前 preflight 来假装 carrier 已闭合。
- CPP/RTL 必须消费同一 verified declaration/state/value 表示，源属 C++声明由该 IR生成。

## 文件、角色与交付

实际派发时再冻结当前 HEAD、批准原字节/哈希、精确 writable files 与 run-id。
Astra 设计作者仅可写新的 `docs/rfcs/migration/c2-decl-record-final.md`（尚不存在）
及本包允许的设计证据；另一个 Astra 独立验证精确提案/两个 backend 映射。
PM 唯一改状态/index，原 SYSTEM/EXPECT B 不接管。用户精确批准之前不写 ODS、
import/link/final、generated C++或 runtime 产品代码。批准后再由 Sol 分解文件归属、
Luna 实现、独立 Luna tests、Sol code review；不存在当前 active implementation 任务。

## Checklist / verification

- [ ] 真实 source forms/profile、字段表、类型/布局、阶段不变量、owner/错误契约完整。
- [ ] source/header/saved-final 再解析仍保持 nominal identity 与原型/constructor引用。
- [ ] provider body/source 移除后 parent 仅 header 编译；一 source一 producer/file group。
- [ ] literal Q/Work/Proposal/Xfer/reset oracle逐阶段定义，不把两 backend 一致当独立正确。
- [ ] defaults/kwargs、同形不同 nominal、字段交换/range/reset/proof篡改反例对应具体 verifier。
- [ ] helper local candidate 与重新读 enclosing reg 的旧 Q 分开；unsafe demanded path失败
  保持全树零提交，不需要 unsafe 表达式运行后再补 expect。
- [ ] 明确物理 storage/StateID、整 record D/E、alias与来源；不偷偷复制 state或新增 latency。
- [ ] 使用既有标量 field projection观察/独立测试，不新增 public record report/typed ABI。
- [ ] common IR 双后端、源属 CPP declaration、codegen名称碰撞/输出保护和retirement责任明确。
- [ ] 独立设计审阅绑定原字节，PM 提交精确用户批准请求；设计通过不记为实现 PASS。

完成出口：精确 design/oracle 获独立 approval-ready，缺口与复用语义各有明确边界。
这不是 record 产品实现或新 public profile 的验收。

## 精确批准与下一包

用户在2026-10-01明确批准 [C2-DECL-R B](../rfcs/migration/approvals/c2-decl-record-final.md)，
SHA-256 `ea242da0d85de4f51c439051c80c2e7ce12c17dca5ef9a63b8743f0f280a0043`。
产品提交 `1efec35e` 归档两轮独立设计review和最终oracle，已经授权按B实现，
不再重复请求同一批准。下一步 [S1–S4实施顺序](m3-record-implementation.md)。
record产品代码/backend验收未在设计步骤完成；其他未批准扩展不随此开放。

- [完整设计审阅](https://github.com/PTO-ISA/pyCircuit/blob/1efec35e/docs/reviews/20261001-m3-record-final-design-review.md)
- [Oracle与原始审阅](https://github.com/PTO-ISA/pyCircuit/blob/1efec35e/docs/gates/logs/20261001-m3-record-design/README.md)
