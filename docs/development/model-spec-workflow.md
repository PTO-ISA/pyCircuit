# NDF specifications and Python traceability

`pycircuit spec-check PROJECT --ndf NDF --output NEW_DIRECTORY` checks a model's
NDF documents and their Python definition bindings. It is an optional workflow;
it does not change the hardware language or approve a specification.

## Format and dependency

The supported input is the real NDF 0.2 Markdown/project/lock format from
[`PTO-ISA/normative_language` at 5e3e6b5](https://github.com/PTO-ISA/normative_language/tree/5e3e6b5f09e06b09aaf52844c4b37e38362ac097).
Use its `ndf.yaml`, `ndf.lock`, document front matter, stable clause IDs and
canonical `kind`, `modality`, `refinement`, `status` and relationship fields.
NDF metadata must immediately follow its clause heading. A successful NDF
check with only document nodes does not establish requirement coverage.

The CLI executable is supplied explicitly. Pin its source in the project's
lock, build that revision with its locked Cargo workflow and retain the command
output. NDF checks dependency identity and graph consistency. pyCircuit does
not install or fetch this dependency implicitly.

## Design and implementation

Keep design intent (L0), externally observable requirements (L1), implementation
constraints (L2) and verification criteria (L3) present together. Connect them
with `refines` and `verifies`. Derive requirements from the design's architectural
authority and selected environment before choosing implementation mechanisms.

Attach standard NDF source comments to named Python definitions, immediately
before their decorators or declaration:

```python
# ndf: implements=COUNT-STATE-001
@ac.module
def Counter(value: ac.u8) -> ac.u8:
    ...

# ndf: verifies=COUNT-VER-001
def test_counter():
    ...
```

Qualified `ndf://PROJECT/ID` targets are also accepted. `implements` targets
requirement clauses; `verifies` targets verification clauses. Bind the actual
state-owning Module, atomic transaction and pure calculation where appropriate.
A test binding points to the independent assertion's entry, not its expected
result generator inside a DUT.

## What is checked

The command invokes the external NDF CLI to check, build and export the source
Spec. It resolves Python comment tokens to actual definitions, checks all L2
requirements have implementation bindings and all verification criteria have
test bindings, and checks L1/L2 coverage through verification/refinement edges.
Empty requirement graphs, dangling targets and unattached annotations fail.

The pinned NDF CLI does not expose its Python repository adapter. This bridge
therefore generates ordinary NDF Markdown source/test nodes and checks that
bound graph with the same CLI. Original Spec and NDF JSON evidence files are
copied byte-for-byte;
the generated index is disposable. The receipt records each qualified symbol,
source location and content digest. It is not injected into calculation IR.

`status: draft` remains draft after a passing check. The receipt explicitly
separates traceability PASS, approval NOT_RECORDED and behavioral validation
NOT_RUN. Independent tests still run through public compile/link/emit/run;
their candidate-bound results are separate evidence. Comments alone cannot
prove instruction semantics, timing, effects or RTL correctness.

Output must be a new directory. Failures retain an INCOMPLETE receipt and the
external command's stdout/stderr/exit code rather than a previous success.

## Verification and review

Use independent results and state/effect checks, then link evidence using the
NDF 0.1 evidence schema. Record exact source and dependency revisions, selected
programs, conditions and unexecuted criteria. Do not turn a subset into a full
conformance claim. Review and any required approval follow
[project governance](project-governance.md#independent-review); graph validity
and a passing execution do not supply either.

Candidate-bound checks are recorded in [validation results](model-spec-validation.md).
