# Quickstart

Run these commands from the repository root. Generated output stays under
`.pycircuit_out/quickstart/`.

## Prepare the checkout

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -e "python/semantic-core"
python -m pip install -e ".[dev,docs]"

bash flows/scripts/pyc build
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
```

## Build the counter example

```bash
PYTHONPATH=python/pycircuit/src \
python -m pycircuit.cli build \
  examples/pycircuit/basics/counter/tb_counter.py \
  --out-dir .pycircuit_out/quickstart/counter \
  --target both \
  --jobs 8
```

This command emits the frontend manifest, PYC MLIR, C++ model and executable,
Verilog, and Verilator inputs for one source design.

## Inspect the frontend output

Emit canonical PYC without running the native backends:

```bash
mkdir -p .pycircuit_out/quickstart
PYTHONPATH=python/pycircuit/src \
python -m pycircuit.cli emit \
  examples/pycircuit/basics/counter/counter.py \
  -o .pycircuit_out/quickstart/counter.pyc
```

## Try Agentic Circuit

Install the second frontend and generate frozen ACIR, a QueueGraph plan, and
gfsim C++ for the routed dependency example:

```bash
python -m pip install -e "python/agentic-circuit[test]"
mkdir -p .pycircuit_out/quickstart/agentic

PYTHONPATH=python/semantic-core/src:python/agentic-circuit/src \
python compiler/acir/tools/ac-queue-cxxgen.py \
  examples/agentic-circuit/pipelines/routed_dependency_pipeline.py \
  --system routed_dependency_pipeline \
  --acir-output .pycircuit_out/quickstart/agentic/routed_dependency.ac.mlir \
  --plan-output .pycircuit_out/quickstart/agentic/routed_dependency.plan.json \
  --acir-opt "$PYC_TOOLCHAIN_ROOT/bin/acir-opt" \
  --queue-plan-tool "$PYC_TOOLCHAIN_ROOT/bin/acir-queue-plan" \
  --queue-cxxgen-tool "$PYC_TOOLCHAIN_ROOT/bin/acir-queue-cxxgen" \
  --output .pycircuit_out/quickstart/agentic/routed_dependency.cpp
```

Run `bash flows/scripts/run_agentic_circuit.sh` for the complete installed
frontend, schema-resource, native compiler, and backend validation lane.

## Run smoke gates

```bash
bash flows/scripts/run_examples.sh
bash flows/scripts/run_sims.sh
```

The examples lane validates compilation and contracts. The simulation lane
checks C++ and Verilator behavior.

## Continue learning

- [pyCircuit 6 tutorial](tutorial.md)
- [Language and API reference](../reference/index.md)
- [Agentic Circuit and ACIR](../acir/index.md)
- [Testing and gates](../development/testing-and-gates.md)
