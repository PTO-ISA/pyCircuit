# Finite FA2 numeric checks

`python designs/sparsefa/tests/numeric/run.py` builds this checkout's
`fa2_numeric.py` and three independent generated Systems using the installed
checkout toolchain. It runs C++ workers 1 and 2 and emits managed Verilog from
the identical verified artifacts. Verilog simulation is excluded by the user for this acceptance run. Earlier
environment probes found no Icarus installation, but that is not the current
reason for skipping RTL execution.

The numeric module uses binary32 sign/exponent/significand arithmetic, with
rounding toward zero, subnormal input/output flushing, and finite saturation
on overflow. NaN, infinity and division by zero are outside the admitted
profile. Exact cancellation may retain either zero sign. FP16 and BF16 casts
truncate and flush; FP16 overflow saturates to 65504. These are deliberate
bounded-profile semantics, not full IEEE-754 conformance. Consumer bounds
must prevent intermediate overflow.

Exponential accepts nonpositive finite arguments. It evaluates a fourth-degree
Taylor approximation at x/256 and squares eight times. Inputs at or below -16
return zero, with absolute mathematical error at most exp(-16). Positive
arguments are outside the profile. The accepted empirical relative-error limit
for the 4096-point sweep in [-16,-1] is 4e-5; this is measured coverage, not a
proof for every FP32 argument. The current maximum was 2.95546e-5. Near-zero
coverage includes exact zero and the seeded System's half-step sweep.

`generate.py` deterministically constructs 32 independent Python float oracle
rows. The resulting `bench.py` asserts arithmetic within two binary32 ULPs,
exponential within 1024 ULPs, and exact normal cast truncation. `edges.py`
checks cancellation, an opposite-sign exponent gap, signed zeros, FTZ,
FP16 overflow, the exponential clamp and exact division. `exp_sweep.py`
logs 4096 hardware exponential results; `check_sweep.py` compares them with
Python `math.exp` and rejects incomplete or failed execution. Host values are
only expected values; they never supply DUT results.

Evidence lives under ignored `docs/gates/logs/fa2-p0-completion/numeric-*`.
An initial 256-row all-operator System hit the compiler's source Method summary
analysis budget. The retained 32-row System and table-free dense sweep both
pass; no compiler budget or semantic bypass is used.
