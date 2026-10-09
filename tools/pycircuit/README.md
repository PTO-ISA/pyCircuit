# Compiler maintenance tools

| Tool | Responsibility |
| --- | --- |
| [example_catalog.py](example_catalog.py) | Render example navigation from `examples/catalog.json` and publish verified generated-output excerpts. |
| [generate_source_identifier_unicode.py](generate_source_identifier_unicode.py) | Regenerate the pinned Unicode source-identifier tables and their provenance. |

Build, validation and measurement orchestration lives in
[`flows/`](../../flows/). These generators do not implement another compiler or
extend source admission. The public interface is `pycircuit compile`, `link`,
`emit` and `run`.
