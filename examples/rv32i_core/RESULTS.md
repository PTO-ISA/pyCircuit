# RV32I evidence summary

The machine-readable `results-baseline.json` preserves the complete bounded
pre-split baseline from local candidate64730b9. It includes all40 scenario rows,
27 program identities,160 view outcomes, reference split, instruction coverage,
C signature, physical boundary and fixed library identity. This historical
candidate is not claimed as the test head of the publication branch.

| Historical measurement | Result |
| --- | --- |
| Architecture views |160 PASS:156 Spike agreement and4 separate FENCE.I admission checks |
| Trace comparisons |40 full timing native/RTL pairs and40 functional compressed pairs |
| Complete state |40 four-view groups; all32GPR/PC/RAM, exact stores and signatures checked |
| Instruction coverage |All40 RV32I families, including ECALL/EBREAK trap execution |
| C application |434 normal retirements plus expected trap,567 cycles; signature10208/424583933/309/329 |
| Long waits |6481 virtual cycles,2782 executed functional ticks,3699 idle ticks folded |
| Hazards/reset |Forwarding, one smoke load-use bubble, store dependencies, redirects/wrong-path cancellation, waiting-store reset |
| Open-cell mapping |13596 cells:2076 sequential,11520 combinational;107784.624 µm² |
| Area boundary |GPR file, pipeline/control and observation cones included; external imem/dmem and test adapter excluded |
| Mapped program |Actual Icarus SKY130 models,567 cycles/434 normal retirements; complete final state checked |

The fixed Spike build accepts FENCE.I with its RV32I setting. Those four views
use separately authored RV32I2.1/closed-EEI illegal-encoding expectations; they
are not counted as Spike agreement. Source/ELF execution coverage is distinct
from exhaustive ISA correctness or official certification.

The mapping flow uses standard Yosys/ABC and scalar-port serialization. Exact
cell/type/parameter and logical-port/pin connectivity is checked before/after
normalization. This avoids Icarus wide strength/concat propagation overhead;
no architecture cells are deleted to obtain a pass. Artificial functional cell
unit delays and the generous simulation clock are not PVT timing signoff.

Public reproduction starts with the pinned SDK bootstrap in README.md, then
`verification/prepare.py`, `suite.py`, `check_results.py`, `synthesize.py` and
`mapped_check.py`. Results are emitted in the selected output directory;
source commits contain summaries and reproducible commands, not private logs,
ELFs, SDKs, netlists or dependency bundles. Original raw logs remain local ignored
artifacts. No public raw-artifact URL is claimed for the historical run.

Publication-head results will be appended after new clean-consumer and mapping
checks. Unchanged baseline data is reusable only after explicit source/final-IR/
physical-RTL and synthesis-input identity checks.

Official architecture tests/certification, full formal equivalence, independent
review, OpenSTA and P&R remain NOT_RUN. Area is mapped standard-cell area, not
layout/die area. SOTA remains NOT_ESTABLISHED.
