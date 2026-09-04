# v0.6.2 — Explicit Verilator Internal-Make Compiler Override

## Root cause fixed

With OSS CAD Suite Verilator, simply exporting:

```bash
export CXX=/usr/bin/g++-10
```

was not sufficient to guarantee the generated/internal Make invocation used
that compiler.

v0.6.2 passes the compiler explicitly through Verilator:

```text
-MAKEFLAGS CXX=/usr/bin/g++-10
-MAKEFLAGS LINK=/usr/bin/g++-10
```

This makes the compiler selection part of the benchmark command itself.

## Recommended test

First verify GCC 10 accepts the coroutine flag:

```bash
echo 'int main(){return 0;}' | \
/usr/bin/g++-10 -std=c++20 -fcoroutines -x c++ - -fsyntax-only
```

No output + return to shell means PASS.

Then run:

```bash
python run_correctness.py cc_lzc \
  --profile smoke \
  --cxx /usr/bin/g++-10
```

The harness should print:

```text
CXX : /usr/bin/g++-10
```

and the generated Verilator command will force the internal make to use it.

## Batch run

```bash
python run_pipeline.py \
  --all-supported \
  --correctness-profile smoke \
  --stateful-profile smoke \
  --synthesis-profile smoke \
  --cxx /usr/bin/g++-10
```
