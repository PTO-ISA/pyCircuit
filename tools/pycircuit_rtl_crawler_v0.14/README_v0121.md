# pyCircuit v0.12.1 — QoR Netlist Formal Sanitization

## Root cause

The BaseJump FIFO mapped netlist still contained verification statements such as:

```verilog
initial begin
  if (1'h1) begin
    assert (1'h1);
  end
end
```

Those constructs are verification metadata, not implementation hardware, and
the legacy OpenSTA 2.0.17 parser rejects them.

## Methodology correction

Both generic QoR and Liberty-mapped QoR now execute:

```tcl
synth -top pyc_synth_top -flatten
chformal -remove
opt_clean
```

before:

```text
generic cell accounting
technology mapping
mapped-area accounting
STA netlist export
```

This prevents projects with more embedded assertions from being unfairly
penalized in QoR.

## Netlist sanitation gate

```bash
python check_qor_netlist.py mapped_netlist.v
```

Expected:

```text
QOR_NETLIST_SANITATION_PASS
```

The timing backend also refuses residual `initial/assert/assume/cover/restrict`
constructs and reports:

```text
NETLIST_FORMAL_RESIDUE
```

instead of passing a dirty netlist to OpenSTA.

## Required FIFO rerun

Regenerate the scaling results because the QoR methodology changed:

```bash
python run_fifo_benchmark.py \
  --profile scaling \
  --workdir ../pycircuit_rtl_crawler_v0.9.3/work \
  --cxx /usr/bin/g++-10 \
  --liberty ../pycircuit_rtl_crawler_v0.9.3/reference_libs/nangate45/NangateOpenCellLibrary_typical.lib
```

Then check BaseJump D16:

```bash
python check_qor_netlist.py \
  design_class_results/FIFO-SYNC/scaling/basejump_stl/bsg_fifo_1r1w_small/w32_d16/mapped_netlist.v
```

Then run timing:

```bash
python run_fifo_timing.py \
  --profile scaling \
  --results-root design_class_results \
  --liberty ../pycircuit_rtl_crawler_v0.9.3/reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --workers 2 \
  --timeout-sec 30
```

For final methodology consistency, DF-09 should later also be regenerated with
this same formal-sanitized QoR flow before freezing the production Runtime Catalog.
