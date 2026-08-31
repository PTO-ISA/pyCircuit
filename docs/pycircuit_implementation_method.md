# PyCircuit implementation method (10-step workflow)

This document describes a repeatable workflow for implementing a hardware block
with the pyCircuit 6 CycleAwareSignal API and the structural `@module` / `Circuit`
library surface. It is written for human engineers and autonomous agents.

---

## Where programming style and APIs are documented

All normative and tutorial material for authoring lives under the repository **`docs/`** directory (path: `/docs` relative to repo root). Use these in **Step 1** and keep them open while coding:

| Topic | Primary documents |
|--------|-------------------|
| **V6 编程规范**（API + 教程 + 子模块调用 + 层次化编译 + 仿真） | `docs/v6_PyCircuit_Specification.md` |
| **V6 `@module` / `Circuit` structural frontend** | `docs/FRONTEND_API.md` |
| **Occurrence cycles and automatic balancing** | `docs/v6_PyCircuit_Specification.md`, `docs/cycle_balance_improvement.md` |
| **Testbenches** (low-level `Tb` API) | `docs/TESTBENCH.md` |
| **IR / lowering expectations** | `docs/IR_SPEC.md`, `docs/PIPELINE.md` |
| **Primitives vs generated code** | `docs/PRIMITIVES.md` |
| **Compiler evolution rules** | `docs/pyc6-plan.md`, `docs/rfcs/pyc6-decisions.md`, root `AGENTS.md` |
| **Diagnostics** | `docs/DIAGNOSTICS.md` |
| **API index** | `docs/api/index.md`, `docs/index.md` |

### V6 signal type discipline

These rules apply to all pyCircuit 6 code written using this workflow:

| Rule | Detail |
|------|--------|
| **All signals are `CycleAwareSignal`** | Every value is a `CycleAwareSignal` (or `ForwardSignal`). No other signal type. |
| **`domain.state()` REMOVED** | Use `domain.signal()` + `<<=` instead. `state()` is internal-only (`_state()`). |
| **`.wire` / `.w` REMOVED** | Properties deleted from all signal types. Use `wire_of()` at `m.output()` only. |
| **`wire_of()` at boundaries only** | The sole way to extract raw `Wire` for `m.output()` calls. |
| **Output dicts store CAS** | Sub-module return dicts hold `CycleAwareSignal` (preserving cycle provenance). |

### V6 module signature convention

Every V6 cycle-aware module follows this standard pattern:

```python
def my_module(
    m: CycleAwareCircuit,          # shared circuit
    domain: CycleAwareDomain,      # shared clock domain
    *,
    inputs: dict | None = None,    # None = standalone; dict = composed
    width: int = 64,               # configuration (keyword-only)
    prefix: str = "mod",           # namespace prefix
) -> dict:                         # output signals (CycleAwareSignal values)

my_module.__pycircuit_name__ = "my_module"
```

- `inputs=None` → standalone: creates `m.input()` / `m.output()` ports
- `inputs={...}` → composed: reads parent's signals, returns outputs, no port emission
- All sub-modules called via `domain.call(fn, inputs={...}, **config, prefix=...)` with push/pop cycle isolation

### V6 submodule calling workflow

The full workflow is documented in `docs/v6_PyCircuit_Specification.md` under
"模块签名与层次化组合". The key steps:

1. **Declare inputs** with `submodule_input(inputs, key, m, domain, prefix=prefix, width=W)` — dual-mode: reads from parent dict or creates `m.input()` port
2. **Build `inputs` dict** in the parent — keys must exactly match child's `submodule_input()` key arguments; values must be `CycleAwareSignal`
3. **Call** `domain.call(child_fn, inputs={...}, **config, prefix=f"{prefix}_child")` — push/pop cycle isolation, kwargs forwarded to child
4. **Read outputs** from returned dict — all values are `CycleAwareSignal` with cycle provenance preserved
5. **Chain** outputs to next sub-module's inputs
6. **Collect top-level outputs** in `outs` dict; emit `m.output()` only in standalone mode (`if inputs is None`)

**Package import:** the Python package name is **`pycircuit`** (lowercase). `PYTHONPATH` must include `compiler/frontend` when running CLI or tests outside an installed package.

---

## Where example designs live

Illustrations of **grammar and structure** are under **`designs/`** and subfolders (path: `/designs` relative to repo root). Study these in **Step 1**:

| Area | Examples (non-exhaustive) |
|------|---------------------------|
| **V6 hierarchical composition** (full-scale, `domain.call()` + `submodule_input()` + `wire_of()`) | Repository designs and examples using the pyCircuit 6 surface |
| **V6 cycle-aware style** (single-module) | `designs/BypassUnit/`, `designs/RegisterFile/`, `designs/IssueQueue/`, and `designs/examples/*/` |
| **`@module` + JIT** | `designs/examples/counter/`, `designs/examples/jit_control_flow/`, `designs/examples/hier_modules/` |
| **V6 testbench** (`CycleAwareTb`) | `tests/unit/test_pyc6_surface.py`, `tests/unit/test_v6_state_signal.py`, and repository examples |
| **Testbench layout** (low-level `Tb`) | `designs/examples/*/tb_*.py`, `designs/BypassUnit/tb_bypass_unit.py`, `designs/RegisterFile/tb_regfile.py` |
| **Structured IO** | Designs using `spec` / bundles per `docs/SPEC_STRUCTURES.md` |

Mirror the **directory layout** (design file + `tb_*.py` + optional `README.md`) of the example closest to your block's complexity. For **hierarchical multi-module designs**, use the Davinci project in **[DavinciOO](https://github.com/hengliao1972/DavinciOO)** (`davinci/`) as the canonical reference.

---

## Step 1 — Read programming style documents and examples

**Goal:** Internalize how pyCircuit 6 expresses hardware: static elaboration,
allowed Python control flow, registers, memories, logical occurrence cycles, and
the structural API's explicit occurrence metadata.

**Actions:**

1. Read **`docs/v6_PyCircuit_Specification.md`** end-to-end when the block uses
   **`CycleAwareCircuit` / `CycleAwareDomain`**. Pay special attention to:
   - **Signal Type Discipline**: all signals are `CycleAwareSignal`; `domain.state()` and `.wire` are removed.
   - **Module Signature Convention**: `(m, domain, *, inputs=None, prefix=...) -> dict` pattern.
   - **Sub-Module Calling Convention** (6-step workflow): `domain.call()`, `submodule_input()`, `wire_of()`, key-matching rules, prefix cascade.
   - **Hierarchical MLIR Emission**: `compile_cycle_aware(..., hierarchical=True)`.
   - **Simulation**: `CycleAwareTb` for cycle-aware testbenches.
2. Read **`docs/FRONTEND_API.md`** and **`docs/TESTBENCH.md`** for `@module`, `Circuit`, and simulation contracts.
3. Open **2–3 concrete examples** under `designs/` that match your intended style:
   - **V6 hierarchical**: supported repository designs using `domain.call()`.
   - **V6 single-module**: `designs/BypassUnit/`, `designs/RegisterFile/`.
   - **`@module`**: `designs/examples/counter/`, `designs/examples/hier_modules/`.
4. Note **non-negotiables** from `AGENTS.md`: gate-first IR changes; no backend-only semantic fixes.

**Deliverable:** Short notes in the design documentation: identify whether the
design uses the V6 cycle-aware API, the structural library API, or both, and
name the supported example used as the style reference.

---

## Design specification conversion (prerequisite before Step 2)

**Why:** Autonomous agents and text-first workflows search, diff, and cite specifications most reliably from **plain Markdown**. Raw **`.docx`**, **`.pdf`**, and **`.xlsx`** files are easy for humans to open but are **poor primary sources** for automated analysis: layout noise, embedded objects, multi-sheet structure, and extraction errors make "read the spec" ambiguous.

**Rule:** **Before** analyzing block-specific design documents in **Step 2**, convert every **normative** artifact in those formats into **`.md`** under the block's documentation tree (e.g. `designs/<Block>/docs/` or `designs/<Block>/docs/converted/`). Treat the Markdown as the **working copy** for implementation and traceability; keep the originals as the legal/normative file where the project requires it.

**Conversion expectations:**

| Source format | Target | Notes |
|---------------|--------|--------|
| **`.docx`** | One or more `.md` files | Prefer structure-preserving export (e.g. Pandoc `pandoc -f docx -t markdown`), then fix headings/lists/tables by hand if needed. |
| **`.pdf`** | `.md` (text + tables as Markdown) | PDF→text quality varies; add a short **header** in each `.md` stating source path, extraction tool, and known gaps (figures, scanned pages). |
| **`.xlsx`** | `.md` per sheet or one `.md` with anchored sections | Export tables to Markdown (scripted or via CSV intermediate); preserve **sheet names** and **row/column** semantics so Step 3 width tables can cite them. |

**File hygiene:**

- Use **stable, searchable names** (e.g. `protocol_sheet1_summary.md`) and a one-line **provenance** header: original filename, date converted, tool/command.
- If a conversion is **partial**, say so in the `.md` and in the Step 2 source table—do not pretend the Markdown is complete.

**Deliverable (enters Step 2):** Markdown digests for all binary specs the block depends on, or an explicit waiver (with owner approval) recorded in `ASSUMPTIONS.md` / `REQUIREMENT_SOURCES.md`.

---

## From converted Markdown to feature list, step docs, and test plan

**Purpose:** Once normative inputs live as **`.md`** under the block (typically `designs/<Block>/docs/converted/`), this subsection defines how to **propagate** that text into **`FEATURE_LIST`**, optional **per-step markdown** (`step1.md` … `step10.md`), **`TRACEABILITY`**, and **`TEST_PLAN`** so agents and reviewers share one chain of evidence. A completed block under **`designs/`** can serve as a **reference layout** (e.g. `feature_list.md`, `workflow_substeps.md`, `run_<block>_verification.py`).

### 1. End-to-end pipeline (recommended order)

| Order | Artifact | Typical location | Tied to step |
|-------|----------|------------------|--------------|
| 1 | **Regenerate digests** when vendor **.docx / .pdf / .xlsx** change | Script under block `docs/` or repo `scripts/`; output `docs/converted/` + `converted/README.md` | After **Design specification conversion** |
| 2 | **Source inventory** with **digest paths** | `REQUIREMENT_SOURCES.md` (or equivalent): each SRC / file → **binary path** + **Markdown path** | **Step 2** |
| 3 | **Assumptions & conflicts** | `ASSUMPTIONS.md`: inferred port directions, CDC, spreadsheet vs prose conflicts | **Step 2–3** |
| 4 | **Port / bus contract** | `PORT_LIST.md`: widths from XLSX-derived `.md` rows; directions from prose digest + assumptions | **Step 3** |
| 5 | **Feature list** | `FEATURE_LIST.md`: see §2 below | **Step 3** |
| 6 | **Sequential + pipelined pseudocode** | `function_list.md`, `step4.md` / `ALGORITHM_*.md` — map digest **chapters** to functions | **Steps 4–5** |
| 7 | **Cycle-aware pseudocode / RTL notes** | `step6.md`, implementation — **Spec trace** comments point at digest headings (and opcode tables in `.md`) | **Step 6** |
| 8 | **Traceability matrices** | `TRACEABILITY.md`: port → F-xxx; F-xxx → code region → **T-xxx** or **TBD** + gap | **Step 7** |
| 9 | **Test plan** | `TEST_LIST.md` / `TEST_PLAN.md`: T-xxx ↔ F-xxx; SYS scenarios ↔ multi-feature; golden vectors from converted tables | **Step 8** |
| 10 | **Increments & log** | `incremental_plan.md`, `IMPLEMENTATION_LOG.md` — tag **F-xxx** per PR; rerun digest script when specs change | **Step 9** |
| 11 | **System test & sign-off** | `system_test_spec.md`, README — P0 **F-xxx** complete or waived | **Step 10** |
| — | **Optional:** `workflow_substeps.md` | Splits a single Step into **2a, 3b, …** for large blocks | Any step |
| — | **Optional:** `cycle_budget.md` | `domain.next()` count, occurrence stages, golden **`pyc.reg`** / MLIR checks | **Steps 5–6**, **9** |

### 2. Structure of `FEATURE_LIST.md` (normative for agent-friendly blocks)

1. **Legend** — priority (P0/P1/P2), column meanings.
2. **Numbered features F-001…** — each row: name, priority, **Spec trace** = pointer into **converted** `.md` (heading text or stable section id), trigger, observable effect, dependencies.
3. **Digest index (coarse)** — table: each major spec chapter (`# …` in the primary digest) → **range of F-ids** (or list).
4. **Heading checklist (full)** — for the **primary** architecture/spec Markdown export, enumerate **every** `#`, `##`, and `###` heading line; each row assigns **one or more F-ids** or **—** (TOC, cover, non-RTL meta only).
   - **Maintenance rule:** if Pandoc/export adds or renames headings, **update this table** or add **F-xxx** / **gap** entries in `TRACEABILITY.md`.
5. **Feature → test summary** — which **T-xxx** / SYS cover which **F-xxx** (use **TBD** only with a dated gap).

Spreadsheet-derived behavior (opcodes, field maps) should cite the **specific** `converted/SRC-xx_xlsx_*.md` file in **Spec trace**, and keep any RTL allowlists (e.g. legal opcodes) **in sync** with that file.

### 3. Block `step1.md` … `step10.md` (optional but recommended)

For complex blocks, mirror this repository's **10-step** narrative in **block-local** files so block-specific rules (converted paths, **F-xxx** ranges, **heading checklist**, **cycle_budget**, **workflow_substeps**) do not clutter the generic steps above. Each `stepN.md` should:

- Point to **`docs/converted/`** and the **regenerate** command.
- State which **F-xxx** band or **checklist** rows that step owns or reviews.
- Cross-link **`PORT_LIST`**, **`FEATURE_LIST`**, **`TRACEABILITY`**, **`TEST_LIST`** as appropriate.

### 4. Tests and automation

- **Directed tests:** at least one **regression-sensitive** case per **F-xxx** before milestone close; build opcode / flit matrices from **Markdown tables** in `converted/` where possible.
- **Block runner:** optional `run_<block>_verification.py` that executes **stdlib** checks: digests present, key markdown sections exist, `emit_mlir()` or compile smoke, width/contract assertions.
- **pytest:** optional `test_<block>_steps.py` with markers `step1` … `step10` mirroring the same checks.

### 5. Relation to the generic Steps 2–10 below

**Steps 2–10** in this document remain the **canonical** workflow. The tables in §1–§4 **specialize** those steps for **Markdown-first** specs; when a bullet in Step 3 / 7 / 8 says "every feature", use **`FEATURE_LIST`** + **heading checklist** as the definition of "every" **heading-level** requirement unless the project explicitly waives finer bullets under a parent **F-xxx**.

---

## Step 2 — Read all block-specific requirement documents

**Goal:** Load every normative input for **this** block (i.e. everything under the block's `docs/` folder, including PDF/XLSX/DOCX **after** they have been converted to Markdown per the section above).

**Actions:**

0. **Confirm** Markdown digests exist (or are waived in writing) for every **`.docx` / `.pdf` / `.xlsx`** the block treats as authoritative—see **Design specification conversion** above. Use the **`.md`** files as the primary text for extraction and agent review.
1. Enumerate all files in the block's `docs/` folder (spreadsheets, Word, PDF, markdown—including converted `.md` companions).
2. Extract **clock/reset**, **protocol**, **ordering**, **credit/flow control**, **addressing**, **data widths**, **modes**, **error behavior**.
3. Build a **source table**: requirement → document → section/sheet/cell (as traceable as possible), with **both** the original binary path (if retained) **and** the Markdown digest path.

4. **Parity check:** walk the **heading checklist** in `FEATURE_LIST.md` (see **From converted Markdown to feature list, step docs, and test plan** §2) against the primary spec digest; every heading row must resolve to **F-xxx** or **—**; unresolved items → gap register in `TRACEABILITY.md`.

**Deliverable:** `REQUIREMENT_SOURCES.md` or equivalent table in block `README.md`.

---

## Step 3 — Top-level ports, buses, widths, and itemized feature list

**Goal:** Freeze the **external contract** and decompose the spec into **testable features**.

**Actions:**

1. **Port list:** For every top-level **input** and **output**, record: name, direction, width (or parameterized width), clock domain, synchronous/asynchronous, active level, protocol phase (valid/ready, etc.).
   - For V6 cycle-aware modules: ports are declared via `submodule_input()`
     (inputs) and `m.output(f"{prefix}_{name}", wire_of(sig))` (outputs).
     Document the `prefix` and `key` for each port.
2. **Buses:** Group related pins into **logical buses** (e.g. CHI request channel, response channel). Document packing if the RTL bundles vectors.
3. **Top-level functionality:** One concise paragraph describing the block's role.
4. **Feature list:** Every **function** or **behavior** described in the spec becomes a **numbered feature** (F-001, F-002, …) with: description, triggering condition, expected observable effect on ports, dependency on other features. For blocks using **converted** specs, follow **From converted Markdown to feature list, step docs, and test plan** §2: include **digest index**, **full heading checklist**, and **Spec trace** paths into `converted/*.md`.
5. **Submodule decomposition:** Identify which logical functions become
   separate module functions, their `inputs` and output keys, and the intended
   `domain.call()` chain.

**Deliverable:** `PORT_LIST.md` + `FEATURE_LIST.md` (or sections in `README.md`). These are the **single checklist** for Steps 7–10.

---

## Step 4 — Sequential (imperative) behavior, function-oriented decomposition

**Goal:** Describe behavior as an **imperative program** (C/Python-like), **without** committing to hardware module boundaries yet.

**Actions:**

1. Write a **main routine** (e.g. `main_loop()` or "per-cycle work") that calls **subroutines** with clear names: `accept_request()`, `route_to_peer()`, `update_credit()`, etc.
2. Each subroutine should have **inputs/outputs** expressed as **conceptual data** (structs, not yet ports), and **no hardware partition**.
3. Prefer **pure functional** steps where possible; use **explicit state** variables for anything that must persist cycle-to-cycle.
4. Decompose until each function fits in one screen and has a **single** coherent responsibility.

**Deliverable:** `ALGORITHM_SEQUENTIAL.md` — plain pseudocode or Python-like pseudocode, **no** `domain.next()` yet.

---

## Step 5 — Align sequential description with cycle-aware hardware (pipeline mapping)

**Goal:** Map the imperative algorithm onto **clock cycles** using **`domain.next()`** and `domain.call()` for sub-module isolation, so that **pipeline stages** and **signal cycle tags** are intentional.

**Actions:**

1. Choose **where each major step** lands in the occurrence timeline: e.g. "Cycle 0: accept & decode; Cycle 1: lookup; Cycle 2: response mux."
2. In each subroutine that becomes a V6 cycle-aware submodule:
   - Define the function signature: `def my_sub(m, domain, *, inputs=None, prefix=..., config_params...) -> dict`.
   - Document the **entry cycle** relative to the caller.
   - Parent calls it via **`domain.call(my_sub, inputs={...}, prefix=f"{prefix}_abbrev")`** — this wraps `push()`/`pop()` automatically to isolate the child's `domain.next()` from the parent.
3. For **parent/child signal relations** (cycle provenance):
   - Input signals in the `inputs` dict retain their original cycle from the parent.
   - Output signals in the returned dict retain the cycle from inside the child function.
   - V6 automatic cycle balancing handles arithmetic between signals from
     different logical cycles.
4. Registers: use **`domain.signal(width=W, reset_value=0, name=...)`** + `<<=` for feedback loops.
   - Read cycle (at declaration) vs write cycle (at `<<=`) determines hardware: gap of 1 → DFF, gap of 0 → combinational alias.
   - For conditional updates: `sig.assign(expr, when=cond)`.
5. **Prefix cascade**: plan the prefix hierarchy so all port and register names are globally unique:
   - Top: `prefix="dv"` → child: `f"{prefix}_fe"` → grandchild: `f"{prefix}_fe_bpu"` → names like `dv_fe_bpu_pc`.

**Deliverable:** `ALGORITHM_PIPELINED.md` — same logical steps as Step 4, annotated with **cycle indices**, **sub-module boundaries** (`domain.call()`), **`inputs` dict key mappings**, and **register inference** rules (`domain.signal()` + `<<=` at appropriate cycles).

---

## Step 6 — Full cycle-aware algorithm in detailed pseudocode

**Goal:** One document that an implementer can translate into V6 module functions.

**Actions:**

1. Use V6 notation consistently:
   - `// Cycle k` comments for cycle boundaries
   - `domain.next()` for cycle advances
   - `sig = domain.signal(width=W, reset_value=0, name="...")` for registers
   - `sig <<= expr` for unconditional register assignment
   - `sig.assign(expr, when=cond)` for conditional assignment
   - `mux(cond, true_val, false_val)` for selection
   - `cas(domain, m.input(...), cycle=0)` or `submodule_input(inputs, key, ...)` for inputs
   - `wire_of(sig)` only in `m.output()` calls
   - `domain.call(sub_fn, inputs={...}, prefix=...)` for sub-module invocation
2. Include **reset behavior** and **idle** behavior.
3. Include **back-pressure** and **stall** paths if the spec defines them.
4. Mark **optional** or **parameterized** branches clearly.
5. For each sub-module function, document:
   - Expected `inputs` dict keys and their widths
   - Returned `outs` dict keys and their types (scalar CAS, list of CAS)
   - Internal `domain.next()` calls and resulting pipeline depth

**Deliverable:** `ALGORITHM_CYCLE_AWARE_PSEUDOCODE.md` — complete pseudocode for the agreed scope (can be phased by milestone).

---

## Step 7 — Specification and feature traceability check

**Goal:** Prove **coverage**: every port behavior and every feature from Step 3 is realized in Step 6 (and planned implementation).

**Actions:**

1. **Port coverage matrix:** each port (or bus field) → pseudocode region / function → feature ID(s).
2. **Feature coverage matrix:** each F-xxx → pseudocode region → test case ID (placeholder for Step 8).
3. Re-read original specs; log **gaps** or **TBD** items explicitly (do not silently omit). For Markdown-first blocks, ensure **every F-xxx** from the **heading checklist** appears in the feature matrix with a **test ID** or a **dated gap** (see **From converted Markdown …** §1).

**Deliverable:** `TRACEABILITY.md` with two matrices and an explicit **gap list** (empty if complete).

---

## Step 8 — Itemized test plan (every signal and every feature)

**Goal:** A test plan where **no port pin** and **no feature** is untested.

**Actions:**

1. For **each port** (or grouped bus): at least one directed test that exercises **0→1**, **toggle**, **hold**, or **protocol sequence** as appropriate.
2. For **each feature** F-xxx: at least one scenario that **fails** if the feature is removed (regression-sensitive).
3. Classify tests: **reset**, **smoke**, **directed**, **stress**, **corner** (overflow, credit exhaust, simultaneous channels).
4. Map each test to V6 testbench code:
   - **Preferred: `CycleAwareTb`** — wraps `Tb` with implicit cycle tracking via
     `tb.next()`, mirroring `domain.next()` in design code. Use
     `tb.drive(port, value)` and `tb.expect(port, value)` at the current cycle.
     See the V6 specification's testbench section.
   - **Alternative: raw `Tb`** — explicit `at=cycle` parameter on every drive/expect. See `docs/TESTBENCH.md`.

5. **Coverage rule (Markdown-first blocks):** each **F-xxx** in `FEATURE_LIST.md` (including ranges filled after the **heading checklist**) must have a planned **directed** or **system** test before tape-out, unless waived in `TRACEABILITY.md`; stimulus/expected values may cite **`converted/*.md`** tables.

6. **Test file conventions:**
   - Unit tests: `tests/unit/test_<module>.py` — compile + CycleAwareTb testbenches for individual sub-modules.
   - Integration tests: `tests/integration/test_<scenario>.py` — compile top-level + multi-module testbenches.
   - Each test file should be runnable standalone: `python tests/unit/test_alu.py`.

**Deliverable:** `TEST_PLAN.md` with test IDs (T-001, …), stimulus sketch, expected outputs, and links to features/ports.

---

## Step 9 — Incremental implementation plan (feature-by-feature)

**Goal:** Grow the design **safely**: shell → one feature → test → repeat.

**Actions:**

1. **Increment 0:** Top-level **empty** (or pass-through) design with **all ports declared** via `submodule_input()` and tied to safe defaults; compiles via `compile_cycle_aware()` and emits MLIR; TB applies reset and idle.
2. For each feature F-xxx in dependency order:
   - Implement only that feature (or minimal supporting glue) as a V6 submodule function.
   - Add or extend **tests** from `TEST_PLAN.md` for that feature (using `CycleAwareTb`).
   - Run **full regression** for all previous tests (must stay green).
   - Verify both **flat** (`compile_cycle_aware(..., eager=True)`) and optionally **hierarchical** (`hierarchical=True`) compilation.
3. Record **increments** in `IMPLEMENTATION_LOG.md`: date, feature ID, files touched, tests added, command line used.

**Deliverable:** Ordered **backlog** of increments + log. Prefer small PR-sized steps.

---

## Step 10 — System test (end-to-end, combinations)

**Goal:** Validate the **whole block** under realistic combined traffic, and re-verify **port** and **feature** coverage.

**Actions:**

1. Define **system scenarios** that stress **multiple features together** (e.g. concurrent requests + credit pressure + error injection).
2. Re-run **full traceability**: confirm `TRACEABILITY.md` and `TEST_PLAN.md` have **no unchecked rows**.
3. Optional: long-run **pseudo-random** stimulus if the TB framework supports it; compare to golden or invariants (no deadlock, no X on outputs, etc.).
4. Verify **hierarchical compilation** produces correct multi-module MLIR: `compile_cycle_aware(..., hierarchical=True)` should emit `func.func` for each sub-module and `pyc.instance` ops in the parent.
5. Document **sign-off criteria** in block `README.md`.

**Deliverable:** `SYSTEM_TEST.md` or README section listing scenarios, commands, and expected results; update `TRACEABILITY.md` status to **closed**.

---

## Relationship to block-specific projects

- For any block, block-specific specs live under `designs/<Block>/docs/`. Start Step 2 there after Step 1, **after** any **DOCX/PDF/XLSX → Markdown** conversion for files you will analyze in depth.
- Existing completed blocks under `designs/` can serve as worked examples of the **From converted Markdown to feature list, step docs, and test plan** pipeline (including **heading checklist**, **`workflow_substeps.md`**, **`cycle_budget.md`**, and **`run_<block>_verification.py`**).
- Supported repository examples and V6 tests are the canonical references for
  the hierarchical composition workflow.
- For new blocks, substitute the appropriate `designs/<Block>/docs/` (or project doc root) and reuse the same artifact names where practical.

## Relationship to repository policy

- If a change requires **new IR semantics** or **stricter legality**, follow **`AGENTS.md`**: extend MLIR verifiers/passes **before** relying on backend behavior.

---

Copyright (C) 2024–2026 PyCircuit Contributors.
