# RTL candidate validation v0.9

This wave extends the v0.8 dependency work with typed validation contexts for
SystemVerilog parameter-type modules and fixed CVA6 project configuration.  The
run remains deliberately single-process and uses bounded Verilator/Yosys
timeouts so it can be repeated on a constrained WSL environment.

## Configuration and dependency updates

- `build_candidate.py` now seeds candidate filelists from configured
  `include_hints` and `vendor_hints`, including roots from transitive sibling
  repositories.
- `forced_files_by_module` makes legacy generated RTL opt-in per top module;
  OpenPiton's `lsu_rrobin_picker2.v` is therefore added only to `lsu_qctl1`,
  instead of contaminating unrelated OpenPiton candidates.
- `sources.expanded.v0.4.json` materializes technology/common-cell and
  HardFloat dependencies, CVA6 CVFPU, OpenPiton generated headers, Vortex
  fixed XLEN defines, and NVDLA deterministic assertion/PRAND configuration.
- `validation_wrappers` generates candidate-local wrappers for PULP AXI
  (`axi_demux`, `axi_demux_simple`, AXI-Lite demux/mux, burst splitters and
  interface variants).  Wrappers instantiate the unchanged upstream DUT with
  the repository's real AXI typedef/interface types, and record
  `validation_top` in `manifest.json`.
- Yosys now prefers the OSS CAD Suite `read_slang` frontend when present, so
  the same struct/interface source accepted by Verilator can be elaborated for
  synthesis.  It falls back to `read_verilog -sv` on older installations and
  passes manifest `defines` to either frontend.
- CVA6 now materializes `common/local/util` from the sparse checkout, prunes
  verification-only `uvm_pkg`/`uvm_macros.svh`, and supplies exact-interface,
  synthesizable `AsyncDpRam`, `SyncDpRam_ind_r_w`, and `SyncDpRam` models for
  the FPGA branches.  These are functional memory implementations, not
  black-box substitutions.  A fixed `btb` wrapper uses the checked-in
  `cv64a6_imafdc_sv39` configuration through `build_config_pkg`.
- `materialize_source_pool.py --no-update` reuses local checkouts without
  resetting dirty user worktrees; the refreshed manifest covers 22 sources.

## Full 149-candidate rerun

Batch report:

`build/batch-validation-v0.8/failed-149-v0.8/batch_report.json`

Aggregate report:

`build/batch-validation-v0.8/all-batches-report-v0.8.json`

| Stage | Result |
|---|---:|
| Dependency closure PASS | 137 / 149 |
| Verilator lint PASS | 99 / 149 |
| Yosys PASS / structural PPA measured | 31 / 149 |
| Functional simulation | 149 `ADAPTER_REQUIRED` |

The 31 Yosys values are structural proxies (cell/wire statistics), not a
technology-mapped area or timing signoff.  Functional simulation is not marked
PASS without a design-specific oracle/testbench.

## Targeted v0.9 regression

The following candidates now pass closure, Verilator, and Yosys (slang), with
structural PPA statistics emitted by the batch driver:

| Candidate | Validation top | Closure | Verilator | Yosys |
|---|---|---|---|---|
| `pulp_axi/axi_demux_simple` | `pycircuit_validate_axi_demux_simple` | PASS | PASS | PASS |
| `pulp_axi/axi_demux` | `pycircuit_validate_axi_demux` | PASS | PASS | PASS |
| `pulp_axi/axi_burst_splitter` | `pycircuit_validate_axi_burst_splitter` | PASS | PASS | PASS |
| `pulp_axi/axi_burst_splitter_gran` | `pycircuit_validate_axi_burst_splitter_gran` | PASS | PASS | PASS |
| `pulp_axi/axi_cdc` | `pycircuit_validate_axi_cdc` | PASS | PASS | PASS |
| `pulp_axi/axi_lite_demux` | `pycircuit_validate_axi_lite_demux` | PASS | PASS | PASS |
| `pulp_axi/axi_lite_mux` | `pycircuit_validate_axi_lite_mux` | PASS | PASS | PASS |
| `pulp_axi/axi_demux_intf` | `pycircuit_validate_axi_demux_intf` | PASS | PASS | PASS |
| `pulp_axi/axi_lite_demux_intf` | `pycircuit_validate_axi_lite_demux_intf` | PASS | PASS | PASS |
| `pulp_axi/axi_lite_mux_intf` | `pycircuit_validate_axi_lite_mux_intf` | PASS | PASS | PASS |
| `pulp_axi/axi_mux_intf` | `pycircuit_validate_axi_mux_intf` | PASS | PASS | PASS |
| `cva6/cva6_fifo_v3` | `pycircuit_validate_cva6_fifo_v3` | PASS | PASS | PASS |
| `cva6/btb` | `pycircuit_validate_btb` | PASS | PASS | PASS |

The per-run reports are under `build/batch-validation-v0.4/targeted-*` and
the direct CVA6 build manifests are under
`build/candidate-validation-v0.4/cva6/targeted_*`.

## Remaining failure classes

- PULP AXI modules without a registered wrapper still need a typed context;
  interface wrappers cover the main demux/mux paths and can be extended from
  the same mechanism.
- CVA6 full-core candidates still require project-level Bender target closure
  and their complete UVM environment; standalone SRAM/BTB paths now have a
  fixed, reproducible synthesis context.
- OpenPiton: deeper LSU/MSHR generated macro sets and legacy source variants
  still require candidate-specific generated configuration.
- Yosys failures on interface-heavy SystemVerilog are kept separate from
  closure results and are not silently converted to success.
