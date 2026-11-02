# New pyCircuit project template

The installed SDK owns the template, so a new project can be generated anywhere
without copying this repository. From the repository root:

```sh
pycircuit init designs/my_design --template stateful
pycircuit doctor --rtl
python3 designs/my_design/verification/verify.py \
  --build-dir "$PWD/.pycircuit_out/my_design-verification"
```

Choose `compute` for a per-request transformation, `stateful` for a checksum,
or `backpressure` for delayed response consumption. All variants include:

```text
my_design/
  README.md
  CMakeLists.txt
  project.json
  model.py
  requests.json
  expected.json
  ndf.yaml
  ndf.lock
  spec/
    README.md
    design-input.md
    model-spec.md
    acceptance.md
  verification/
    verify.py
```

Edit the NDF design input before replacing the telemetry algorithm. Update L2
constraints, L3 acceptance criteria, Python definition bindings and independent
expected vectors as the design evolves. Record the pyCircuit SDK revision used
by your project; the NDF tool revision is already pinned in `ndf.lock`.
Follow the generated `spec/README.md` to build that external CLI and run
`pycircuit spec-check`. Both execution policies and both backends are checked
by the supplied verification entrypoint; timing/reset coverage still needs
project-specific oracles. Every initial clause is draft and has no approval or
execution evidence merely because it was generated.

The canonical assets are packaged under
[`python/pycircuit/templates/model`](../../python/pycircuit/templates/model/spec/README.md)
and composed by the installed `pycircuit init` command. Existing directories
are rejected to preserve user files.
