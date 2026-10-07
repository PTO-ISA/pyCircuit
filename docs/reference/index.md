# Language reference

The public Python package is `pycircuit`. All designs use the same source-unit
compiler and the `pycircuit compile`, `link` and `emit` commands. Import aliases
such as `pyc` and `ac` do not select a different frontend.

- [Python source language](language.md)
- [Language and execution semantics](language-specification.md)
- [Diagnostics](diagnostics.md)
- [Source ownership and generated names](name-mangling.md)
- [Tables and collections](spec-collections.md)
- [Enums](spec-enums.md)
- [Queues](spec-queues.md)

Source admission and internal IR capabilities have distinct boundaries. See
[known limitations](../development/known-limitations.md) before relying on a
feature outside the documented source subset.
