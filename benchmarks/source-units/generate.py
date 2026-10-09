"""Generate design-neutral source-unit scale fixtures for the build reliability baseline.

The fixtures are written for the **current** authoring surface, measured against
the installed compiler rather than assumed:

* every source unit is an implementation unit owning one ``@module``; a bare
  top-level type alias is rejected (``source top level supports imports and
  @module definitions``) and no declaration-only unit can supply a type that a
  portless root has to materialise;
* cross-unit imports are absolute and use the ``--package-prefix`` package
  (``from measurement.leaf import Leaf``); relative imports are rejected;
* a cross-unit call requires the callee to return a *nominal struct*, so each
  leaf declares its own result record and the root declares its own;
* fixtures carry no ``report``/``log`` observation: the current emitter rejects
  observation hardware with ``'ac.observe' op hardware instrumentation emission
  is not implemented``, so an observation oracle cannot be emitted at all.
"""

from __future__ import annotations

import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
# `prepare_output.py` moved from `examples/counter/` to the shared
# `cmake/` directory (it is installed to `share/pycircuit/cmake/prepare_output.py`
# and consumed by `cmake/PycircuitExamples.cmake`). The file content is unchanged
# (byte-identical move), so this generator only needed the path updated.
PREPARE_OUTPUT = ROOT / "cmake/prepare_output.py"

#: Package prefix every generated source unit is compiled with. Imports between
#: units are absolute under this package because relative imports are unsupported.
PACKAGE = "measurement"

#: Width of the generated data path. The current surface spells a bounded
#: integer either as `ac.uN` or inline as `Annotated[int, range(1 << W)]`.
WIDTH = 8


def _write_source(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def _leaf_source(name: str) -> str:
    """One implementation unit: a registered leaf with a unit-local record.

    The record is declared here, not in a shared declaration unit, because an
    imported nominal can be used as a port type but cannot be constructed by the
    importing unit (`nominal declaration is not a module constructor`).
    """

    return f"""# ruff: noqa: N802,F841
import pycircuit as ac
from pycircuit import module, rule


@ac.struct
class {name}Result:
    count: ac.u8


@rule
def advance(count: ac.u8, incoming: ac.u8) -> {name}Result:
    result = {name}Result(count=count)
    count = incoming
    return result


@module
def {name}(incoming: ac.u8) -> {name}Result:
    count: ac.u8 = 0
    return advance(count, incoming)
"""


def _root_source(leaves: list[tuple[str, str]], instances: int) -> str:
    """The portless root: literal source wires plus one instance per leaf call."""

    imports = "".join(f"from {PACKAGE}.{stem} import {name}\n" for stem, name in leaves)
    rows: list[str] = []
    for index in range(instances):
        input_name = f"input_{index}"
        leaf = leaves[0][1] if len(leaves) == 1 else leaves[index][1]
        rows.extend(
            (
                f"    {input_name}: ac.u8 = {index % 251 + 1}\n",
                f"    instance_{index} = {leaf}({input_name})\n",
            )
        )
    fields = "".join(f"    out_{index}: ac.u8\n" for index in range(instances))
    actuals = ", ".join(
        f"out_{index}=instance_{index}.count" for index in range(instances)
    )
    return (
        "# ruff: noqa: N802,F841\n"
        "import pycircuit as ac\n"
        "from pycircuit import module\n"
        "\n"
        f"{imports}"
        "\n"
        "\n"
        "@ac.struct\n"
        "class DesignTopResult:\n"
        f"{fields}"
        "\n"
        "\n"
        "@module\n"
        "def DesignTop() -> DesignTopResult:\n"
        + "".join(rows)
        + f"    return DesignTopResult({actuals})\n"
    )


def _cmake_source(stems: list[str], parents: dict[str, list[str]]) -> str:
    lines = [
        "cmake_minimum_required(VERSION 3.25)",
        "project(PycircuitM6SourceUnits NONE)",
        "find_package(Python3 REQUIRED COMPONENTS Interpreter)",
        "find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)",
        'find_program(_driver NAMES pycircuit HINTS "${PYCIRCUIT_PREFIX}/bin" NO_DEFAULT_PATH REQUIRED)',
        'set(_compiler_dependencies "${_driver}" "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_output.py" "${PYCIRCUIT_PREFIX}/share/pycircuit/cmake/pycircuitConfig.cmake" "${PYCIRCUIT_PREFIX}/share/pycircuit/toolchain-metadata.json")',
        "foreach(_helper_name pycircuit-source-unit pycircuit-link pycircuit-emit)",
        '  find_program(_helper_${_helper_name} NAMES ${_helper_name} HINTS "${PYCIRCUIT_PREFIX}/bin" NO_DEFAULT_PATH REQUIRED)',
        '  list(APPEND _compiler_dependencies "${_helper_${_helper_name}}")',
        "endforeach()",
        'set(_python_implementation "${PYCIRCUIT_PREFIX}/share/pycircuit/python/pycircuit")',
        'file(GLOB_RECURSE _compiler_python CONFIGURE_DEPENDS "${_python_implementation}/*.py")',
        "list(APPEND _compiler_dependencies ${_compiler_python})",
        'set(_src "${CMAKE_CURRENT_SOURCE_DIR}/src")',
        'set(_units "${CMAKE_CURRENT_BINARY_DIR}/units")',
        'set(_prepared "${CMAKE_CURRENT_SOURCE_DIR}/prepare_output.py")',
        'set(_toolchain_config "${CMAKE_CURRENT_SOURCE_DIR}/toolchain-config.txt")',
        'file(MAKE_DIRECTORY "${_units}")',
    ]
    for stem in stems:
        directory = f"${{_units}}/{stem}"
        import_args = "".join(
            f' -I "${{_units}}/{provider}"' for provider in parents[stem]
        )
        dependencies = [
            f'"${{_src}}/{stem}.py"',
            "${_compiler_dependencies}",
            '"${_toolchain_config}"',
        ]
        for provider in parents[stem]:
            dependencies.extend(
                (
                    f'"${{_units}}/{provider}/{provider}.interface.ac"',
                    f'"${{_units}}/{provider}/unit.json"',
                )
            )
        lines.extend(
            (
                f'add_custom_command(OUTPUT "{directory}/{stem}.ac" "{directory}/{stem}.interface.ac" "{directory}/unit.json"',
                f'  BYPRODUCTS "{directory}/{stem}.d"',
                f'  COMMAND "${{Python3_EXECUTABLE}}" "${{_prepared}}" "{directory}"',
                f'  COMMAND "${{_driver}}" compile -c "${{_src}}/{stem}.py" --source-root "${{_src}}" --package-prefix {PACKAGE}{import_args} -o "{directory}" --replace',
                f'  DEPFILE "{directory}/{stem}.d"',
                "  DEPENDS " + " ".join(dependencies),
                "  VERBATIM)",
                f'add_custom_target(unit_{stem} DEPENDS "{directory}/{stem}.ac" "{directory}/{stem}.interface.ac" "{directory}/unit.json")',
            )
        )
    unit_dirs = " ".join(f'"${{_units}}/{stem}"' for stem in stems)
    link_inputs = (
        " ".join(
            f'"${{_units}}/{stem}/{stem}.{suffix}"'
            for stem in stems
            for suffix in ("ac", "interface.ac")
        )
        + " "
        + " ".join(f'"${{_units}}/{stem}/unit.json"' for stem in stems)
    )
    lines.extend(
        (
            'set(_program "${CMAKE_CURRENT_BINARY_DIR}/design_top.ac")',
            'add_custom_command(OUTPUT "${_program}"',
            f'  COMMAND "${{_driver}}" link {unit_dirs} --top {PACKAGE}.design_top.DesignTop -o "${{_program}}" --replace',
            f'  DEPENDS {link_inputs} ${{_compiler_dependencies}} "${{_toolchain_config}}"',
            "  VERBATIM)",
            'add_custom_target(source-unit-all DEPENDS "${_program}")',
        )
    )
    for target in ("cpp", "verilog"):
        # `generated.json` is the declared OUTPUT of the bundle command below and
        # must not also appear as one of its byproducts.
        lines.extend((f"set(_{target}_files CMakeLists.txt",))
        if target == "cpp":
            lines.extend(("  pycircuit_support.hpp pycircuit_system.hpp",))
        else:
            lines.extend(("  design_top.sv",))
        for stem in stems:
            if target == "cpp":
                lines.append(f"  sources/{PACKAGE}/{stem}.hpp")
                lines.append(f"  sources/{PACKAGE}/{stem}.cpp")
            else:
                lines.append(f"  sources/{PACKAGE}/{stem}.v")
            lines.append(f"  sources/{PACKAGE}/{stem}.source-map.json")
        lines.extend(
            (
                ")",
                "set(_bundle_byproducts)",
                f"foreach(_file IN LISTS _{target}_files)",
                f'  list(APPEND _bundle_byproducts "${{CMAKE_CURRENT_BINARY_DIR}}/{target}/${{_file}}")',
                "endforeach()",
                f'add_custom_command(OUTPUT "${{CMAKE_CURRENT_BINARY_DIR}}/{target}/generated.json"',
                "  BYPRODUCTS ${_bundle_byproducts}",
                f'  COMMAND "${{Python3_EXECUTABLE}}" "${{_prepared}}" "${{CMAKE_CURRENT_BINARY_DIR}}/{target}"',
                f'  COMMAND "${{_driver}}" emit "${{_program}}" --target {target} -o "${{CMAKE_CURRENT_BINARY_DIR}}/{target}" --replace',
                '  DEPENDS "${_program}" ${_compiler_dependencies} "${_toolchain_config}"',
                "  VERBATIM)",
                f'add_custom_target(emit_{target} DEPENDS "${{CMAKE_CURRENT_BINARY_DIR}}/{target}/generated.json")',
            )
        )
    return "\n".join(lines) + "\n"


def generate_case(root: Path, axis: str, size: int) -> dict:
    """Write an explicit source graph; all Python calls are literal source text."""

    root.mkdir(parents=True, exist_ok=True)
    source_root = root / "src"
    stems: list[str] = []
    parents: dict[str, list[str]] = {}
    leaves: list[tuple[str, str]] = []
    if axis == "shared":
        # One implementation unit instantiated `size` times by the root.
        leaves = [("leaf", "Leaf")]
        instance_count = size
    elif axis == "distinct":
        # `size` independent implementation units, one instance each.
        leaves = [(f"leaf_{index}", f"Leaf{index}") for index in range(size)]
        instance_count = size
    else:
        raise ValueError(f"unknown axis: {axis}")
    for stem, _name in leaves:
        stems.append(stem)
        parents[stem] = []
    stems.append("design_top")
    parents["design_top"] = [leaf[0] for leaf in leaves]

    for stem, name in leaves:
        _write_source(source_root / f"{stem}.py", _leaf_source(name))
    _write_source(source_root / "design_top.py", _root_source(leaves, instance_count))
    _write_source(root / "CMakeLists.txt", _cmake_source(stems, parents))
    shutil.copyfile(PREPARE_OUTPUT, root / "prepare_output.py")
    _write_source(root / "toolchain-config.txt", "pycircuit-measurement-baseline-v1\n")
    (root / "fixture.json").write_text(
        json.dumps(
            {
                "axis": axis,
                "size": size,
                "source_units": len(stems),
                "definitions": len(leaves) + 1,
                "instances": instance_count,
                "stems": stems,
                "leaf_stems": [stem for stem, _name in leaves],
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
    return json.loads((root / "fixture.json").read_text(encoding="utf-8"))
