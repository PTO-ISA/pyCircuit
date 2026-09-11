# Issue #106 readable transition review

QueueGraph now carries source-facing rule, interface, local-value, and source
location metadata separately from semantic queue names, stable IDs, and
definition or specialization fingerprints. The Python frontend emits normalized
project-relative paths; native lowering propagates rule provenance; structured
module interfaces preserve display names with strict type and arity checks.

GFSim generation uses one shared ASCII/C++ identifier legalizer and stable
collision allocation for locals, ports, tables, and rules. Stateful transition
policies expose named state indices and next values, write presence, optional
outputs, reservation values, and the functional rule condition while retaining
the existing `StateTransitionPlan` order and runtime ABI.

Review found two closure gaps and resolved them before the final gates. First,
the ACIR lit expectations still asserted positional generated names; all 17
affected checks now assert the readable contract. Second, plan-local display
names and generator-level symbols had separate legalizers; both paths now use
the same deterministic C++ keyword-aware implementation. No remaining P0-P2
semantic, determinism, path-leakage, or ABI findings remain.
