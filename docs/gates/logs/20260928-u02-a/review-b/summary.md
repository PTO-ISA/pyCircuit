# U02-A independent source and test review-b

- Reviewer role/model/effort: `code-reviewer` / `gpt-5.6-sol` / `high`
- Candidate baseline HEAD: `03625a3dc3b368be9dbac4bfff7ffdb1d3b54466`
- Bound changed/untracked source and test files: 38
- Content-manifest SHA-256: `c4be7e586f8cca49c11d2e358c5ca996cd1d4c5fe943e12f19cfcb830013ba92`
- Independent test-b: 159/159 selected tests, zero skips; CTest 4/4; format/static checks passed
- Verdict: PASS

## Review conclusion

No actionable findings remain in the U02-A scope.

The review-A static-child defect is closed by inspecting every imported
Parameter category before connection lowering. Required, defaulted, and
explicit static parameters all fail closed in U02-A and cannot disappear into
an empty `ac.static_args` list.

Connection Default metadata follows the approved C1/C2 distinction: the
declaration may retain a typed default, but a child connection still requires
an explicit existing DFFE handle. Omission and literal actuals reject; an
explicit handle succeeds. The earlier proposal to forbid all present connection
defaults was withdrawn because it would tighten the approved contract.

Inactive decorated rules receive scope, subset, logical-type, default, and
return validation without contributing active rule operations or effects. A
legal inactive read of `self.result` remains accepted and inactive; malformed
bodies reject.

All new hardware operations require an enclosing source-stage implementation
unit. Missing or unknown stage/unit-kind values fail closed rather than entering
an incomplete linked/final path.

The shared U02-A StaticExpr evaluator supports literals and verified record
constructor calls without executing Python. It binds positional/keyword/default
arguments, preserves source evaluation order, checks bool/integer ranges and
nominal results, interprets the verified `ac.struct.create/get` and
`func.return` SSA, and rejects unsupported operations and malformed calls.

Body declaration snapshots do not authorize themselves. Before source-unit
publication, `verifyBodySnapshots` rebuilds the authority registry from the
separately supplied provider headers plus the owning interface, checks the
body/header envelope and namespace mirror, and compares every import snapshot
with its canonical declaration using normalized role/location metadata and
region equivalence. Constructor body/default, record order/type, role/owner,
and missing-provider mutations reject while the canonical header remains
unchanged.

## Evidence and scope

- Native module/reset/snapshot contracts: 19/19.
- Preserved foundation, SourceUnit, and N1 native tests: 64/64.
- System tests: 76/76, zero skips.
- Combined selected tests: 159/159; CTest 4/4.
- Clang-format, Ruff format/check, and `git diff --check` passed.
- New raw source/test files are at most 583 lines, below the 600-line limit.

This approval is limited to U02-A source module signatures, source DFFE/reset,
rule/instance shape, registered effects, static-argument capability rejection,
snapshot authority, and header-only parent compilation. It does not establish
U02-B math/SCF, link, final commit closure, backends, complete five-category
namespace support, installed SDK behavior, or full C2/C3 completion.
