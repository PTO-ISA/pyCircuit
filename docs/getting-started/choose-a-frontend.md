# Choose a Frontend

pyCircuit contains two supported Python distributions and authoring models.
Choose the frontend from the abstraction you want to describe, not from the
backend you eventually want to run.

## Frontend comparison

| Question | pyCircuit 6 | Agentic Circuit |
| --- | --- | --- |
| Distribution | `pycircuit-hisi` | `agentic-circuit` |
| Python import | `pycircuit` | `agentic_circuit` |
| Primary authoring model | Signals, state, hierarchy, and logical cycles | Architecture, processes, queues, resources, and workloads |
| Source contract | Cycle-Aware Signal or structural modules | ACPy contract epoch `0.4` |
| Primary IR | PYC MLIR | ACIR, then ACSim or PYC |
| Native simulation | pyc6 C++ cycle model | ACSim/gfsim |
| Hardware generation | PYC → `pycc` → C++ and Verilog | Synthesizable ACIR → PYC → `pycc` → C++ and Verilog |
| CLI | `pycircuit` / `python -m pycircuit.cli` | `agentic-circuit` |

## Use pyCircuit 6 for hardware

Choose `pycircuit` when the design is expressed in terms of ports, signals,
registers, memories, combinational logic, pipeline stages, or clock domains.
Cycle-Aware Signal tracks logical-cycle provenance and inserts explicit delay
state when mixed-cycle expressions need balancing.

Install and build:

```bash
python3 -m pip install -e ".[dev,docs]"
bash flows/scripts/pyc build
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
```

Compile the repository counter example:

```bash
PYTHONPATH=python/pycircuit/src \
python3 -m pycircuit.cli build \
  examples/pycircuit/counter/tb_counter.py \
  --out-dir /tmp/pyc_counter \
  --target both
```

Continue with the [pyCircuit 6 tutorial](../v6_PyCircuit_Tutorial.md).

## Use Agentic Circuit for architecture models

Choose `agentic_circuit` when the source model describes processes, queues,
resources, scheduling, architectural state, workloads, or simulator behavior.
ACIR remains an upper-level dialect; ACSim/gfsim is its native architecture
simulation path.

Install the second distribution from the same checkout:

```bash
python3 -m pip install -e "python/agentic-circuit[test]"
agentic-circuit --help
```

Configure the ACIR toolchain, generated schema resources, and validation
environment:

```bash
PYC_GATE_RUN_ID=local-ac-$(date +%Y%m%d-%H%M%S) \
bash flows/scripts/run_agentic_circuit.sh
```

Then check a maintained workspace example from the repository root:

```bash
AC_ROOT="$PWD"
AC_PYTHON="$PWD/.pycircuit_out/agentic-circuit/venv/bin/python"
cd "$AC_ROOT/examples/agentic-circuit/workspaces/producer_queue_consumer"
PYTHONPATH="$AC_ROOT/python/agentic-circuit/src:$AC_ROOT/.pycircuit_out/acir/dev-llvm22/python" \
"$AC_PYTHON" -m agentic_circuit._cli check architecture.py \
  --project agentic-circuit.toml \
  --json
```

The generated resource tree contains the packaged AC schemas and diagnostics;
an editable Python install by itself does not create it. Return to the
repository root before running more repository gates. Continue with the
[Agentic Circuit and ACIR overview](../acir/index.md).

## How the flows meet

```text
agentic_circuit -> ACPy -> ACIR -> ACSim -> gfsim
                              `-> PYC -> pycc -> pyc6 C++ / Verilog

pycircuit -> Cycle-Aware Signal / structural modules
                              `-> PYC -> pycc -> pyc6 C++ / Verilog
```

PYC is the shared verified hardware contract. It does not make the two Python
frontends interchangeable, and Agentic Circuit symbols are not re-exported
from `pycircuit`.

## Validate your path

For pyCircuit-only changes, start with:

```bash
pytest tests/unit -m unit
bash flows/scripts/run_examples.sh
```

For Agentic Circuit or ACIR changes, run the integrated closure:

```bash
PYC_GATE_RUN_ID=local-ac-$(date +%Y%m%d-%H%M%S) \
bash flows/scripts/run_agentic_circuit.sh
```

See [Testing and Gates](../development/testing-and-gates.md) for the required
lane matrix.
