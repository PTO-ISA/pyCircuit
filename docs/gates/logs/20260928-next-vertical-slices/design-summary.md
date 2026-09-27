# Next vertical slices: source link and C3 compile

Read-only architecture work on clean isolated candidate `6d56f4a1` used
approved C2-C, C2-N1-C and C3-C, with pinned donor GFSIM `b852ed83`.
`u02c_link_design` and `c3_compile_design` were independent Astra xhigh
architect tasks. Neither changed code or ran tests.

## U02-C private source link

Stage the linker in two independently checkable parts: first admit complete
body/header pairs against one owning-header registry and reject stale N1
bindings; then build a private linked-semantic MLIR with canonical declarations,
specialization identities and distinct instance/state owners. Do not publish
this intermediate product as C3 `program.ac`, which requires final lowering and
verification. Reuse current snapshot/header services; split the body verifier
so a full registry does not insert the same owning header twice.

Initial Probe exit: each source compiled independently, source files hidden at
link, one child specialization reused by two child instances, distinct state
owners, and negative authority/port/handle/namespace mutations. The original
Accumulator/Core exit additionally requires approved A2 source-use and full
effects/instance proof. Link cannot infer required source read occurrences
from an anonymous rule block argument; a design follow-up is determining
whether the accepted IR already retains enough information or a new approved
source-only carrier is needed.

## C3 single-source compile

Start the approved `pycircuit compile` entry independently of link/final. The
single route is capture of one stable Python source snapshot → existing private
AST transport → `compilePythonSourceUnit` → validated body/interface →
transactionally published body, header, depfile and receipt. The parent reads
only explicitly published provider receipt/header; the native registry resolves
the actual dependency closure. A valid stable HeaderView must not demand
provider source/body/depfile, while recovery and full publication validation
remain strict. Publication must implement C3's fixed lock/journal state machine
before the CLI writes product outputs. Old CLI lowering is replaced at the
product cutover, never selected as a fallback.

The current parallel implementation starts with private publication mechanics.
Stable source snapshot/receipt, private native helper, CLI dispatch and
incremental CMake gates follow as separate packages. No C3 DUT I/O interface or
SSM source change is implied by this work.
