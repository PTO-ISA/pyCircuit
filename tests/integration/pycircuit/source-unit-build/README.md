# Explicit source-unit CMake DAG

This fixture shows a source-per-unit build graph for the public `pycircuit`
driver. Each Python source has one independent compile command that publishes
its body, interface, depfile, and receipt. The parent compiles from child and
type interfaces; the final link names the complete implementation and
declaration closure.

The fixture stops at `linked/parent.ac`. It does not emit backend code, build
an SDK consumer, or split a whole-design compiler invocation.

## Sources and targets

The checked-in sources are:

| Source | Unit kind | Relationship |
| --- | --- | --- |
| `src/types.py` | declarations | Defines the shared `Word` type |
| `src/child.py` | implementation | Defines `Child`, importing `Word` |
| `src/parent.py` | implementation | Defines the separate root `Parent`, instantiating `Child` |
| `src/independent.py` | implementation | Unrelated leaf for selective-rebuild checks |

The `source-unit-child` command receives only the types interface. The
`source-unit-parent` command receives the child and types interfaces; its
explicit CMake dependencies name those interface files and receipts, never
their bodies. The `source-unit-program` link depends on the bodies, interfaces,
and receipts for types, child, and parent. It leaves the independent unit out
of the linked graph.

`source-unit-all` builds the four units and linked program. The individual
targets are `source-unit-types`, `source-unit-child`, `source-unit-parent`,
`source-unit-independent`, and `source-unit-program`.

## Configure and build

The Python source roots and native helpers must come from the same checkout and
toolchain build. This example uses the current checkout's local build at
`.pycircuit_out/w10-pm/build`. Set `REPO` to the checkout containing this
fixture. Configure and build paths may contain spaces:

```sh
REPO="/path/to/pyCircuit"
FIXTURE="$REPO/tests/integration/pycircuit/source-unit-build"
BUILD_DIR="$REPO/.pycircuit_out/source unit fixture build"
TOOLCHAIN="$REPO/.pycircuit_out/w10-pm/build"

cmake -S "$FIXTURE" -B "$BUILD_DIR" -G Ninja \
  -DPYCIRCUIT_REPOSITORY_ROOT="$REPO" \
  -DPYCIRCUIT_PYTHON_EXECUTABLE="$(command -v python3)" \
  "-DPYCIRCUIT_IMPORT_ROOTS=$REPO/python/pycircuit/src" \
  -DPYCIRCUIT_SOURCE_UNIT_HARNESS="$TOOLCHAIN/bin/pycircuit-source-unit" \
  -DPYCIRCUIT_DESIGN_HARNESS="$TOOLCHAIN/bin/pycircuit-link" \
  -DPYCIRCUIT_OUTPUT_ROOT="$BUILD_DIR/generated units" \
  "-DPYCIRCUIT_TOOLCHAIN_CONFIG=$REPO/pyproject.toml;$TOOLCHAIN/pycircuitConfig.cmake;$TOOLCHAIN/toolchain-metadata.json"

cmake --build "$BUILD_DIR" --target source-unit-all
```

To build only the root's prerequisites and the root:

```sh
cmake --build "$BUILD_DIR" --target source-unit-parent
```

The depfile is a declared byproduct of each source compile command, so Ninja's
clean target removes it along with the body, interface, and receipt. A clean
followed by a normal build is supported:

```sh
cmake --build "$BUILD_DIR" --target clean
cmake --build "$BUILD_DIR" --target source-unit-all
```

The build commands use `VERBATIM`; the output paths, source paths, helper
paths, and import roots remain single command arguments. The driver's Make
depfiles escape those paths for Ninja. Ninja may pre-create the declared
output directory before running a custom command, while `pycircuit compile`
requires a missing publication destination. The fixture's
`prepare_unit_output.py` removes that path only when it is empty; populated
directories remain for the driver's `--replace` validation, and other
filesystem errors fail the command. The helper also rejects symlinked output
paths and ancestors before removal; it is a fixture aid and does not claim
protection against an adversarial path swap between that check and removal.

## Public driver commands

The custom commands above run these same public commands. They always pass
`--replace`, which creates a missing publication and safely republishes the
same owner on a later build.

```sh
export PYTHONPATH="$REPO/python/pycircuit/src"
export PYCIRCUIT_SOURCE_COMPILER="$TOOLCHAIN/bin/pycircuit-source-unit"
export PYCIRCUIT_LINKER="$TOOLCHAIN/bin/pycircuit-link"

SOURCE_ROOT="$FIXTURE/src"
UNITS="$BUILD_DIR/manual generated units"
mkdir -p "$UNITS" "$UNITS/linked"

python3 -m pycircuit.cli compile \
  -c "$SOURCE_ROOT/types.py" --source-root "$SOURCE_ROOT" \
  --package-prefix demo -o "$UNITS/types" --replace
python3 -m pycircuit.cli compile \
  -c "$SOURCE_ROOT/child.py" --source-root "$SOURCE_ROOT" \
  --package-prefix demo -I "$UNITS/types" \
  -o "$UNITS/child" --replace
python3 -m pycircuit.cli compile \
  -c "$SOURCE_ROOT/parent.py" --source-root "$SOURCE_ROOT" \
  --package-prefix demo -I "$UNITS/child" -I "$UNITS/types" \
  -o "$UNITS/parent" --replace
python3 -m pycircuit.cli compile \
  -c "$SOURCE_ROOT/independent.py" --source-root "$SOURCE_ROOT" \
  --package-prefix demo -o "$UNITS/independent" --replace
python3 -m pycircuit.cli link \
  "$UNITS/types" "$UNITS/child" "$UNITS/parent" \
  --top demo.parent.Parent -o "$UNITS/linked/parent.ac" --replace
```

These commands work with source and output directories containing spaces.
They require the four named fixture inputs to stay in distinct publication
directories.

## Header-only compile boundary

Compile `types` and `child`, then remove the child's body while preserving its
interface and receipt. Compiling the parent still succeeds because compile
reads supplied interfaces and receipts, not child Python or body files:

```sh
rm "$UNITS/child/child.ac"
python3 -m pycircuit.cli compile \
  -c "$SOURCE_ROOT/parent.py" --source-root "$SOURCE_ROOT" \
  --package-prefix demo -I "$UNITS/child" -I "$UNITS/types" \
  -o "$UNITS/parent" --replace
```

Linking that incomplete closure fails because link validates every listed
unit's complete body/header/receipt set:

```sh
python3 -m pycircuit.cli link \
  "$UNITS/types" "$UNITS/child" "$UNITS/parent" \
  --top demo.parent.Parent -o "$UNITS/linked/parent.ac" --replace
```

Use a disposable output directory for this boundary check. The missing body is
intentional evidence that parent compile and full-closure link consume
different artifacts.
