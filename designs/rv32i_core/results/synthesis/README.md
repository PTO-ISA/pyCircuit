# SKY130 mapping deliverable

This is the fresh mapping of the repaired main-based physical RTL, not the old
processor netlist. The [experiment](../../EXPERIMENT.md#synthesis-area-and-mapped-simulation)
explains the boundary, dependencies, measured results and reproduction commands.

| File | Meaning |
| --- | --- |
| `mapped.v` | Complete actual mapped Verilog, unchanged from Yosys output |
| `synthesis.v` | Actual sv2v conversion input to Yosys, unchanged |
| `synth.ys.in` | Actual command sequence; only Liberty/input paths use named placeholders |
| `yosys.txt` | Complete original Yosys/ABC log with local path prefixes made portable |
| `stat.json`, `area.json`, `cell-counts.csv` | Native statistics and per-type inventory; only invocation/dependency paths are normalized |
| `normalization.json` | Exact cell/type/parameter and logical-port/pin connectivity check during normalization |
| `ports.json` | Port objects extracted verbatim from the original mapped graph JSON, sufficient for scalar pin adaptation |

The full intermediate Yosys JSON graphs are omitted because they duplicate the
netlist and can be produced by the synthesis entrypoint. This port map is not
presented as a full graph. The original file checksums and report transformations
are in [the artifact manifest](../../generated/artifacts.json).

The new result is 13,860 mapped cells and 109,284.8128 square micrometres of
Liberty cell area at HD TT025C1v80. The file `area.json` records the synthesis
stage; its mapped-simulation field is NOT_RUN at that stage. The subsequent
[frozen-netlist verification](../reproduction.json) and
[simulation log](../validation/mapped-simulation.txt) record the actual successful
mapped C application.

The selected Volare distribution and external view checksums are fixed. Cell
library files are Apache-2.0 dependencies, not embedded in this directory. The
area includes the register file, pipeline/control and observation logic, excludes
external RAM/testbench, and is not layout area or timing signoff.
