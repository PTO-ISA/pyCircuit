# Choose a Frontend

pyCircuit ships one wheel per platform that contains two frontends. Choose from
the abstraction you want to describe; both can target the same verified hardware
backend, and both compilers come from the same install.

## Frontend comparison

| Question | pyCircuit 6 | Agentic Circuit |
| --- | --- | --- |
| Distribution | one wheel, `pycircuit-hisi` | same wheel |
| Python import | `pycircuit` | `agentic_circuit` |
| Authoring focus | Signals, state, hierarchy, and logical cycles | Processes, queues, resources, scheduling, and architecture state |
| Primary representation | Cycle-Aware Signal or structural modules | ACPy and verified ACIR |
| Native simulation | pyc6 C++ cycle model | ACC-generated gfsim C++ |
| Hardware generation | PYC → `pycc` → C++ / Verilog | verified ACIR → `acc` → C++ / bundle / Verilog |
| CLI | `pycircuit` | `acc.py` and `acc` |

## Use pyCircuit 6 for hardware

Choose `pycircuit` when the design is naturally expressed as ports, signals,
registers, memories, combinational logic, pipeline stages, or clock domains.
Cycle-Aware Signal tracks logical-cycle provenance and materializes the delay
state needed to balance mixed-cycle expressions.

```bash
python -m pip install -e "python/semantic-core"
python -m pip install -e ".[dev,docs]"
bash flows/scripts/pyc build
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
```

Start with the [counter quickstart](quickstart.md#build-the-counter-example),
then continue to the [pyCircuit 6 tutorial](tutorial.md).

## Use Agentic Circuit for architecture models

Choose `agentic_circuit` when the model is naturally expressed as processes,
queues, resources, arbitration, scheduling, committed tables, or architecture
state. `@ac.rule` describes schedulable typed behavior while the compiler owns
Queue availability, reservations, backpressure, and atomic commit.

The current frontend supports typed pure helpers, structured payloads,
multi-input and optional multi-output rules, persistent scalar/Table state,
reusable modules, Table selection and arbitration, explicit memories, and the
synthesizable ACIR subset. JIT-dependent widths and fixed shapes, exact-name
record spread, optional fixed/sparse standard-Python enum encoding, and
one-hot conflict reporting remain concrete and verifier checked through ACIR,
QueueGraph, GFSim, and PYC. Unsupported behavior fails at a documented
verifier boundary rather than receiving a silent default.

```bash
python -m pip install -e "python/agentic-circuit[test]"
agentic-circuit --help
```

Build the integrated native tools and run the maintained architecture-model
closure:

```bash
PYC_GATE_RUN_ID=local-ac-$(date +%Y%m%d-%H%M%S) \
bash flows/scripts/run_agentic_circuit.sh
```

Continue with the [Agentic Circuit and ACIR overview](../acir/index.md).

## How the flows meet

```text
agentic_circuit -> acc.py -> verified ACIR -> acc -> gfsim C++ / bundle
                                             `-> PYC -> pycc -> Verilog

pycircuit -> Cycle-Aware Signal / structural modules
                                    `-> PYC -> pycc -> pyc6 C++ / Verilog
```

PYC is the shared verified hardware contract. The Python frontends remain
separate public namespaces and are not interchangeable compatibility layers.

Coding agents implementing a non-trivial design should continue with the
[Pythonic Frontend Guide for Coding Agents](../development/agent-frontend-guide.md)
for decomposition recipes, maintained examples, failure modes, and handoff
requirements.

## Validate the selected path

For pyCircuit changes:

```bash
pytest tests/unit -m unit
bash flows/scripts/run_examples.sh
```

For Agentic Circuit or ACIR changes:

```bash
python tools/agentic-circuit/check-contracts.py
PYC_GATE_RUN_ID=local-ac-$(date +%Y%m%d-%H%M%S) \
bash flows/scripts/run_agentic_circuit.sh
```

See [Testing and Gates](../development/testing-and-gates.md) for the complete
change-to-gate matrix.
