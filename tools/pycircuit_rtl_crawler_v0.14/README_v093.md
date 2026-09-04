# pyCircuit v0.9.3 — Fast and Strict OpenSTA Flow

## What the debug run proved

A manual N=16 timing run completed in about one second when using:

```tcl
report_checks -path_delay max -group_count 1
```

Therefore the previous 600 second timeout was not caused by design size or
Liberty parsing. The expensive report option combination was the problem.

v0.9.3 removes:

```text
-endpoint_count
-sort_by_slack
-format full
```

from the benchmark path query.

## Important second issue: netlist parse errors

The manual OpenSTA run also showed Verilog syntax errors containing `{`.
OpenSTA continued and printed timing anyway. Such timing is not trustworthy.

Yosys mapped-netlist export is now:

```text
write_verilog -noattr -simple-lhs
```

so connection assignments use simple LHS forms without concatenations.

The timing runner also treats any OpenSTA `Error:` as:

```text
NETLIST_PARSE_FAIL
```

instead of accepting a timing report produced after parse errors.

## Required action

Because the mapped-netlist writer changed, regenerate mapped netlists before
timing:

```bash
python run_technology_benchmark.py \
  --class-id DF-09 \
  --profile scaling \
  --liberty reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --cxx /usr/bin/g++-10
```

Then test one N=16 netlist directly:

```bash
python check_mapped_netlist.py \
  design_class_results/DF-09/scaling/basejump_stl/bsg_arb_round_robin/n16/mapped_netlist.v \
  --liberty reference_libs/nangate45/NangateOpenCellLibrary_typical.lib
```

Expected:

```text
NETLIST_PARSE_PASS
```

Finally run timing:

```bash
python run_timing_benchmark.py \
  --class-id DF-09 \
  --profile scaling \
  --liberty reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --only-n 16 \
  --workers 1 \
  --timeout-sec 60
```

A few-hundred-cell arbiter should not need a 600 second timeout once the
path-report query is simplified.
