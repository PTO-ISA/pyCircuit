# Issue #40 P1 tutorial façade removal

The package no longer exports or implements `pyc_CircuitModule`,
`pyc_CircuitLogger`, `pyc_ClockDomain`, `pyc_Signal`, `signal`, or identity
`log()`. Repository code, product documentation, examples, and tests had no
consumer of these names.

`CycleAwareSignal`, `ForwardSignal`, and internal `StateSignal` now interpret
`|` only as hardware bitwise OR. Passing a description string raises the normal
unsupported-operand `TypeError` instead of silently discarding the string.

The unit test reads the complete expected `pycircuit.__all__` set from a text
golden and explicitly checks that each removed façade name is absent. Keeping
the export names outside Python test sources avoids misclassifying API-name
coverage as behavioral PYC operation coverage.
