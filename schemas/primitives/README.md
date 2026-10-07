# Primitive reference data

The retained vendor-neutral semantic registries and RTL implementation catalog separate
semantic intent from implementation choices. They are reference data for
future library/selection work, not another compiler or a claim of supported
primitive selection in the current value profile.

`tools/pycircuit/generate-semantic-primitive-registry.py` validates and renders
the declarative primitive table. Licensed RTL implementations remain under
`library/verilog/`; the current final emitter does not install an alternate
PYC selection pipeline. The retired PYC inventory/coverage gate is available
only through Git history.
