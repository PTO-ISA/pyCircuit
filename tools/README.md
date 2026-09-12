# Repository Tools

Tools are grouped by the product surface they maintain:

- [`pycircuit/`](pycircuit/) contains PYC inventory, trace, graph, and
  visualization utilities.
- [`agentic-circuit/`](agentic-circuit/) contains AC contract, catalog,
  release-layout, SDK, and development-environment utilities.

Build and validation orchestration lives under `flows/`. In particular,
`flows/tools/` contains helpers called by repository gates rather than a third
user-facing tool namespace.

Run a tool from the repository root so relative schema, compiler, and
documentation paths resolve consistently. Generated output belongs under
`.pycircuit_out/`.
