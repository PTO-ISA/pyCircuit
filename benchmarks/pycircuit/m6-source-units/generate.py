"""Generate design-neutral source-unit scale fixtures for the M6 baseline."""

from __future__ import annotations

import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
PREPARE_OUTPUT = ROOT / "examples/pycircuit/counter/prepare_output.py"


def _write_source(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def _types_source(width: int = 256) -> str:
    return f"from typing import Annotated\n\nWord = Annotated[int, range({width})]\n"


def _leaf_source(name: str, topic: str) -> str:
    return f"""# ruff: noqa: N802,F841
from pycircuit import module, report, rule

from .types import Word


@module
def {name}(incoming: Word, outgoing: Word):
    count: Word = 0

    @rule
    def tick():
        nonlocal count, outgoing
        report("{topic}", count)
        outgoing = count
        count = incoming

    tick()
"""


def _root_source(leaves: list[tuple[str, str]], instances: int) -> str:
    imports = "".join(f"from .{stem} import {name}\n" for stem, name in leaves)
    rows: list[str] = []
    for index in range(instances):
        input_name = f"input_{index}"
        output_name = f"output_{index}"
        leaf = leaves[0][1] if len(leaves) == 1 else leaves[index][1]
        rows.extend(
            (
                f"    {input_name}: Word = {index % 251 + 1}\n",
                f"    {output_name}: Word = 0\n",
                f"    instance_{index} = {leaf}({input_name}, {output_name})\n",
            )
        )
    return (
        "# ruff: noqa: N802,F841\n"
        "from pycircuit import module\n\n"
        "from .types import Word\n"
        f"{imports}\n"
        "@module\n"
        "def DesignTop():\n" + "".join(rows)
    )


def _cmake_source(stems: list[str], parents: dict[str, list[str]]) -> str:
    lines = [
        "cmake_minimum_required(VERSION 3.25)",
        "project(PycircuitM6SourceUnits NONE)",
        "find_package(Python3 REQUIRED COMPONENTS Interpreter)",
        "find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)",
        'find_program(_driver NAMES pycircuit HINTS "${PYCIRCUIT_PREFIX}/bin" NO_DEFAULT_PATH REQUIRED)',
        'set(_compiler_dependencies "${_driver}" "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_output.py" "${PYCIRCUIT_PREFIX}/share/pycircuit/cmake/pycircuitConfig.cmake" "${PYCIRCUIT_PREFIX}/share/pycircuit/toolchain-metadata.json")',
        "foreach(_helper_name acir-source-unit-harness acir-design-harness acir-cpp-source-parts-harness)",
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
                f'  COMMAND "${{_driver}}" compile -c "${{_src}}/{stem}.py" --source-root "${{_src}}" --package-prefix m6{import_args} -o "{directory}" --replace',
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
            f'  COMMAND "${{_driver}}" link {unit_dirs} --top m6.design_top.DesignTop -o "${{_program}}" --replace',
            f'  DEPENDS {link_inputs} ${{_compiler_dependencies}} "${{_toolchain_config}}"',
            "  VERBATIM)",
            'add_custom_target(source-unit-all DEPENDS "${_program}")',
        )
    )
    for target in ("cpp", "verilog"):
        lines.extend((f"set(_{target}_files CMakeLists.txt",))
        if target == "cpp":
            lines.extend(
                (
                    "  dut.h model_api.cpp pycircuit_support.hpp pycircuit_system.hpp",
                    "  runner_metadata.hpp runner_main.cpp",
                )
            )
        else:
            lines.extend(
                (
                    "  design_top.sv runner_bridge.sv rtl_system.hpp",
                    "  runner_metadata.hpp runner_main.cpp",
                )
            )
        for stem in stems:
            if stem == "types":
                if target == "cpp":
                    lines.append("  sources/m6/types.hpp")
            else:
                extension = "cpp" if target == "cpp" else "sv"
                lines.append(f"  sources/m6/{stem}.{extension}")
                if target == "cpp":
                    lines.append(f"  sources/m6/{stem}.hpp")
            lines.append(f"  sources/m6/{stem}.source-map.json")
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
    stems = ["types"]
    parents: dict[str, list[str]] = {"types": []}
    leaves: list[tuple[str, str]] = []
    if axis == "shared":
        leaves = [("leaf", "Leaf")]
        stems.append("leaf")
        parents["leaf"] = ["types"]
        instance_count = size
    elif axis == "distinct":
        leaves = [(f"leaf_{index}", f"Leaf{index}") for index in range(size)]
        instance_count = size
        for index, (_stem, _name) in enumerate(leaves):
            stem = f"leaf_{index}"
            stems.append(stem)
            parents[stem] = ["types"]
    else:
        raise ValueError(f"unknown axis: {axis}")
    stems.append("design_top")
    parents["design_top"] = ["types", *(leaf[0] for leaf in leaves)]

    _write_source(source_root / "types.py", _types_source())
    for index, (stem, name) in enumerate(leaves):
        _write_source(source_root / f"{stem}.py", _leaf_source(name, f"count_{index}"))
    _write_source(source_root / "design_top.py", _root_source(leaves, instance_count))
    _write_source(root / "CMakeLists.txt", _cmake_source(stems, parents))
    shutil.copyfile(PREPARE_OUTPUT, root / "prepare_output.py")
    _write_source(root / "toolchain-config.txt", "pycircuit-m6-baseline-v1\n")
    (root / "fixture.json").write_text(
        json.dumps(
            {
                "axis": axis,
                "size": size,
                "source_units": len(stems),
                "definitions": len(leaves) + 1,
                "instances": instance_count,
                "stems": stems,
                "leaf_stems": [
                    "leaf" if axis == "shared" else f"leaf_{i}" for i in range(size)
                ],
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
    return json.loads((root / "fixture.json").read_text(encoding="utf-8"))
