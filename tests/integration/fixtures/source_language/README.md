# Source language fixtures

These are source-language inputs for capture and compiler tests. Each executable
Python file owns one module; `packet.py` is a type-only source. These fixtures do
not by themselves claim a passing end-to-end pipeline.

`expected.json` is private test data, not a public schema/model ABI. The vectors
come from the approved source semantics and independent arithmetic, not compiler
output. The independent review confirmed both core and bank traces.

- Core recurrence: at each edge, left adds one when the old valid bit is true,
  right adds two, and valid toggles only at the edge. Reset returns output Q to
  7/19 while child totals reset to zero. Work alone preserves committed Q.
- Bank recurrence: the first output is the old element value (0 or 5), then
  the constant input (9 or 13). Different private array sizes and reset images
  cannot share instance state.
- Integer vectors distinguish mathematical arithmetic, declared-range checks,
  explicit low-bit masking, floor division and path-enabled errors.
- Conflict and alias vectors enforce precommit failure and per-ordinal effects.

Validation performed during preparation: `ast.parse` on all five sources and a
standalone Python calculation of the approved recurrence and arithmetic vectors.
This proves source syntax and oracle consistency only. It does not prove type
analysis, source-unit linking, codegen, runtime or C++/Verilog equivalence.
No self-checking placeholder backend test is registered.
