# Queue adder

`queue_adder.py` defines a reusable Agentic Circuit module with two `u32`
inputs and one `u32` output. Each input and output is a Queue at the generated
gfsim boundary. The `add_pair` rule consumes one token from each input and
publishes their sum as one atomic firing. If either input is empty or the
output cannot accept a token, neither input is consumed. Addition wraps to
32 bits.

The typed `queue_adder_system` entrypoint in `core.py` creates the input and
output boundaries used to compile and simulate the example.

This example exercises Decision 0189 (rule-backed module inputs and outputs)
and Decision 0269 (independently compiled AC source units). From the repository
root, after building the current checkout's Agentic Circuit toolchain:

```bash
AC_OUT=.pycircuit_out/examples/davo
AC_PYTHONPATH=python/semantic-core/src:python/agentic-circuit/src:.pycircuit_out/acir/dev-llvm22/python
mkdir -p "$AC_OUT/package/sources" "$AC_OUT/package/interfaces"

PYTHONDONTWRITEBYTECODE=1 PYTHONPATH="$AC_PYTHONPATH" \
  .pycircuit_out/agentic-circuit/venv/bin/acc.py \
  --project examples/davo/agentic-circuit.toml \
  -c "$PWD/examples/davo/queue_adder.py" \
  -o "$AC_OUT/package/sources/queue_adder.ac" \
  --header-output "$AC_OUT/package/interfaces/queue_adder.ac"
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH="$AC_PYTHONPATH" \
  .pycircuit_out/agentic-circuit/venv/bin/acc.py \
  --project examples/davo/agentic-circuit.toml \
  -c "$PWD/examples/davo/core.py" -o "$AC_OUT/package/core.ac"

.pycircuit_out/acir/dev-llvm22/bin/acc -c "$AC_OUT/package" -verify
.pycircuit_out/acir/dev-llvm22/bin/acc -c "$AC_OUT/package" \
  -emit-cpp-bundle -o "$AC_OUT/bundle"
cmake -S "$AC_OUT/bundle" -B "$AC_OUT/bundle-build" -G Ninja \
  -DAC_GFSIM_INCLUDE_DIR="$PWD/simulator/gfsim/include"
cmake --build "$AC_OUT/bundle-build" --parallel 4
c++ -std=c++20 -I"$AC_OUT/bundle/include" -Isimulator/gfsim/include \
  examples/davo/queue_adder_harness.cpp \
  "$AC_OUT/bundle-build/libac_generated_model.a" \
  .pycircuit_out/acir/dev-llvm22/gfsim/libgfsim.a \
  -o "$AC_OUT/queue_adder_sim"
"$AC_OUT/queue_adder_sim"
```

The harness checks that an unpaired left token remains queued, then checks
`5 + 7 = 12` and `0xffffffff + 2 = 1` after `u32` wraparound. Successful output:

```text
sums=12,1
```

Use a fresh `AC_OUT` directory when repeating the `acc.py` publication steps;
the compiler does not overwrite an existing AC unit.
