# pyCircuit Synthesis Harness v0.5.2

## Fixes

### 1. Slang path handling

v0.5.1 generated commands such as:

```text
read_slang ... -I "/mnt/e/.../include" "/mnt/e/.../cc_pkg.sv"
```

In the observed OSS CAD Suite / Yosys 0.68 integration, the quote characters
were forwarded to Slang as part of the argument, producing diagnostics such as:

```text
include directory '"/mnt/e/.../include"': No such file or directory
```

v0.5.2 passes normalized unquoted POSIX paths to `read_slang`.

For now, v0.5.2 explicitly rejects source paths containing whitespace instead
of silently generating an ambiguous command.

### 2. Exact cc_lzc wrapper width

The synthesis wrapper now declares:

```systemverilog
logic [cc_pkg::idx_width(WIDTH)-1:0] cnt_o;
```

to match the upstream PULP interface rather than using `$clog2(WIDTH+1)`.

## Run

Reuse `work/` and `candidates/` from v0.5.1:

```bash
cp -a ../pycircuit_rtl_crawler_v0.5.1/work ./
cp -a ../pycircuit_rtl_crawler_v0.5.1/candidates ./
```

Then:

```bash
python smoke_test_v052.py
python run_synthesis.py cc_lzc --profile smoke
```

Inspect the generated command:

```bash
head -n 3 synthesis_results/pulp_common_cells/cc_lzc/smoke/lzc_w8_leading/generic.ys
```

The `read_slang` source/include paths should no longer contain literal double
quotes.
