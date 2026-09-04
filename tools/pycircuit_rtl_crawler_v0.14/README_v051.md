# pyCircuit Yosys Synthesis / QoR Harness v0.5.1

## Why v0.5.1 exists

The first real PULP synthesis run failed while using:

```text
read_verilog -sv
```

at `cc_pkg.sv`, because Yosys' native Verilog frontend has only limited
SystemVerilog support.

v0.5.1 introduces a frontend policy:

```text
--frontend auto
    ↓
read_slang available?
    ├─ yes → Slang / sv-elab
    └─ no  → native read_verilog -sv
```

For modern SystemVerilog sources such as PULP / OpenTitan / Gemmini, Slang is
the preferred frontend.

## Commands

Check:

```bash
yosys -p "help read_slang"
```

Run the harness smoke test:

```bash
python smoke_test_v051.py
```

Then rerun LZC:

```bash
python run_synthesis.py cc_lzc --profile smoke
```

The terminal should now show:

```text
frontend      : slang
read_slang    : available
```

You can force a frontend for debugging:

```bash
python run_synthesis.py cc_lzc --profile smoke --frontend slang
python run_synthesis.py cc_lzc --profile smoke --frontend native
```

## Important

Do not modify the upstream PULP RTL to make the native Yosys parser accept it.
The source already passes Verilator compile and correctness gates. Frontend
compatibility is part of the benchmark toolchain, not a reason to rewrite the
candidate.
