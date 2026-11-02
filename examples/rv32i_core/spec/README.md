# RV32I design contract and reproducible workflow

The design input, specification and acceptance criteria are separate authored
NDF 0.2 documents. They use the public format and CLI at
[`5e3e6b5`](https://github.com/PTO-ISA/normative_language/tree/5e3e6b5f09e06b09aaf52844c4b37e38362ac097),
locked in this example's `ndf.lock`. This is an example-scoped workflow, not a
repository-wide adoption of NDF.

| Artifact | Meaning |
| --- | --- |
| [Design input](design-input.md) | L0 intent and L1 architectural/platform requirements |
| [Core Spec](core-spec.md) | L2 ISA/EEI, ownership, five stages, interfaces, dependencies, flush, effects and reset |
| [Acceptance](acceptance.md) | L3 independent expectations and selected verification entrypoints |
| `../*.py` `implements=` comments | Actual named modules, state declarations, calculations and atomic transaction |
| `../verification/*.py` `verifies=` comments | Independent ISA/state/store, cycle and cross-view checks |

All clauses are **draft** revision 1. Their Git content, stable IDs and NDF
lifecycle are distinct from implementation and execution status. Graph validation
cannot approve a design or prove that a CPU implements it. Existing results are
retained in [RESULTS](../RESULTS.md); they are not automatically relabelled as
this candidate's Spec conformance.

## Authority and interpretation

The architectural reference is [RV32I 2.1, manual v20260120](https://docs.riscv.org/reference/isa/v20260120/unpriv/rv32.html).
The demo selects its own blocking Harvard EEI, trap/halt policy and memory window.
L1 requirements derive from those authorities and the five-stage design goals;
L2 constrains the implementation, and L3 describes independent acceptance.

NDF defines Markdown front matter, heading IDs, metadata, relationships, project
and lock schemas. In this workflow the Spec is the L1/L2 normative Markdown; there
is no second invented Spec serialization or revived legacy checker. Metadata
immediately follows its heading, as required by the pinned parser. Python
annotations follow the public repository adapter's `implements=`/`verifies=`
spelling; pyCircuit's bridge additionally records the attached qualified symbol.

Independent review and any semantic approval follow
[project governance](../../../docs/development/project-governance.md#independent-review).
No approval is inferred from a document's `authority: normative`, an NDF PASS,
a previous simulation or a mapped area. Lifecycle changes require review;
this workflow never promotes draft to active automatically.

## Install the locked NDF CLI

The example requires the shared-model SDK named in `../pycircuit-dependency.json`
and [the documented firmware/simulation tools](../verification/TOOLS.md).
NDF additionally requires Rust 1.94.0. From the repository root:

```sh
export DEMO="$PWD/examples/rv32i_core"
export NDF_SOURCE="$DEMO/.pycircuit_out/ndf"
git clone https://github.com/PTO-ISA/normative_language.git "$NDF_SOURCE"
git -C "$NDF_SOURCE" checkout 5e3e6b5f09e06b09aaf52844c4b37e38362ac097
cargo build --manifest-path "$NDF_SOURCE/Cargo.toml" --release --locked -p ndf-cli
export NDF="$NDF_SOURCE/target/release/ndf"
"$NDF" --machine-version
```

Reuse an existing exact clean checkout instead of cloning over it. The native
NDF check verifies the project's dependency lock. It checks documents and
relationships; the pinned CLI does not directly index Python files. Public
`pycircuit spec-check` checks the attached Python definitions and feeds standard
NDF source/test nodes back to that CLI. The generated graph is disposable,
while authored requirements and source remain unchanged.

## Run traceability and real model execution

Select a new output directory and an installed SDK containing `spec-check`:

```sh
export SPEC_OUT="$PWD/.pycircuit_out/rv32i-spec-validation"
python3 examples/rv32i_core/verification/spec_workflow.py \
  --prefix "$PYC_TOOLCHAIN_ROOT" --ndf "$NDF" \
  --clang "$RV32_CLANG" --spike "$SPIKE" --output "$SPEC_OUT"
```

The entry first invokes installed `pycircuit spec-check`, then compiles fresh
`families`, `throughput`, `smoke`, `c-array`, `ecall` and `illegal_fence_i` programs and obtains independent
Spike or explicitly selected closed-RV32I expectations. FENCE.I rejection
remains separate from Spike agreement. It selects existing fast scenarios and smoke's
reset-during-waiting-store scenario, without modifying those stimuli or the
180-second per-stage run limit. Public `pycircuit run` generates and executes
functional/timing × C++/Icarus, followed by independent raw/state/RAM, exact-store,
throughput/hazard and native/RTL cycle checks. This bounded selection does not
stand for the complete historical test campaign or official ISA certification.

Each stage retains its command and independent exit code. `workflow.json` stays
INCOMPLETE on failure. `traceability/spec-check.json` identifies source symbols,
locations and content; the graph includes implementation/verification edges.
`validation/raw-validation.json` reports actual view/state/IR checks. The
standard NDF 0.1 `evidence.json` links the selected execution to verification
clauses and the tested source revision. Review it with its command and selected
scope; a graph edge alone does not establish behavioral correctness.

To check only the document/source graph, use a different new output directory:

```sh
"$PYC_TOOLCHAIN_ROOT/bin/pycircuit" spec-check "$DEMO" \
  --ndf "$NDF" --output "$PWD/.pycircuit_out/rv32i-spec-graph"
```

The tool rejects missing references, unbound Spec/test criteria and an empty
requirement graph. Hardware still follows source capture → explicit unit
compile/link → common IR → backend generation → runtime. Spec annotations do
not supply DUT values, loosen admission, change ISA semantics or replace an oracle.
