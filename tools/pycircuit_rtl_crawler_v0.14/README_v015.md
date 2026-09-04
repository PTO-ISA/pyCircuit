# RTL candidate validation v0.6

This wave closes the dependency/configuration gaps found in the 149-candidate
failure set.  `sources.expanded.v0.4.json` now materializes the CVA6 CVFPU
package, deterministic NVDLA `PRAND_OFF` settings, OpenPiton generated-header
shims, and an optional `forced_files` list for legacy `.v.pyv` units.

`build_candidate.py` records generated/forced files in each manifest and emits
them in the Verilator/Yosys file list.  `dependency_closure.py` classifies
interface instances separately from module instances, allowing Vortex
interfaces to enter the closure.  The generated files are candidate-local and
do not modify an upstream checkout.

## Full 149-candidate rerun

Batch report:

`build/batch-validation-v0.6/failed-149-v0.6/batch_report.json`

Aggregate report:

`build/batch-validation-v0.6/failed-149-v0.6/all-batches-report-v0.6.json`

| Stage | v0.5 completed subset (124) | v0.6 full set (149) |
|---|---:|---:|
| Dependency closure PASS | 117 | 129 |
| Verilator PASS | 88 | 97 |
| Yosys PASS / structural PPA | 22 | 31 |
| Functional simulation | adapter required | adapter required |

The v0.6 run completed all 149 candidates with one bounded process.  The
remaining failures are dominated by unsupported SystemVerilog constructs or
project-specific interface/memory wrappers (CVA6/Vortex/BlackParrot/PULP),
and OpenPiton's deeper LSU generated configuration.  They remain explicitly
reported as FAIL/BLOCKED rather than being hidden by guessed shims.

## Targeted post-run checks

- OpenPiton `demux_process_pkt`, `l2_amo_alu`, `l2_broadcast_counter`,
  `l2_broadcast_counter_wrap`, and `sync_fifo_vr`: closure, Verilator, and
  Yosys pass with generated headers.
- OpenPiton `lsu_qctl1`: closure and Verilator pass after forcing
  `lsu_rrobin_picker2.v` (the remaining warnings are legacy width/timescale
  diagnostics).
- Vortex `VX_ptw`: closure is complete after interface classification; the
  remaining Verilator errors are language/struct support issues.
- NVDLA SDP CMUX and CDMA arbiter candidates: closure, Verilator, and Yosys
  pass with `PRAND_OFF` and assertion pruning.
