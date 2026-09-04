# v0.6.1 — Verilator Toolchain Preflight

## Why v0.6 produced false `corr FAIL`

The RTL was not failing correctness. The Verilator-generated simulation
executable failed to compile:

```text
g++: error: unrecognized command line option '-fcoroutines'
```

The generated timing model requires a C++ compiler that supports C++20
coroutines. v0.6.1 checks this capability *before* running any candidate.

This is deliberately classified as:

```text
ENVIRONMENT / TOOLCHAIN FAILURE
```

and not:

```text
RTL CORRECTNESS FAILURE
```

## First check

```bash
python check_toolchain.py
```

If it fails:

```bash
ls -1 /usr/bin/g++-* 2>/dev/null
```

Test an installed newer compiler:

```bash
python check_toolchain.py --cxx /usr/bin/g++-11
```

or:

```bash
python check_toolchain.py --cxx /usr/bin/g++-12
```

## Run batch pipeline with an explicit compiler

```bash
python run_pipeline.py \
  --all-supported \
  --correctness-profile smoke \
  --stateful-profile smoke \
  --synthesis-profile smoke \
  --cxx /usr/bin/g++-11
```

You may also set the compiler for the current shell:

```bash
export CXX=/usr/bin/g++-11
export LINK=/usr/bin/g++-11
python run_pipeline.py --all-supported
```

The pipeline will now print:

```text
CXX       : /usr/bin/g++-11
C++20 coro: PASS
```

before running RTL candidates.
