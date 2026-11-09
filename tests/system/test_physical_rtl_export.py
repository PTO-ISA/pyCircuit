"""Independent HWIR physical RTL export and four-state execution coverage."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.system
ROOT = Path(__file__).resolve().parents[2]
WIDE65 = "1" + "x" * 62 + "xz"
WIDE130 = ("10xz" * 32) + "xz"

SOURCE = """import pycircuit as ac
@ac.rule
def write_first(saved: ac.u8, incoming: ac.u8, enabled: ac.u1) -> None:
    if enabled:
        saved = incoming
@ac.rule
def write_second(saved: ac.u8, incoming: ac.u8, enabled: ac.u1) -> None:
    if enabled:
        saved = incoming
@ac.rule
def hold_wide(first: ac.bits[65], second: ac.bits[130]) -> None:
    first = first
    second = second
@ac.module
def Pair(first: ac.u8, second: ac.u8, enable_first: ac.u1,
         enable_second: ac.u1) -> {"first_q": ac.u8, "second_q": ac.u8}:
    first_saved: ac.u8 = 0
    second_saved: ac.u8 = 0
    write_first(first_saved, first, enable_first)
    write_second(second_saved, second, enable_second)
    return {"first_q": first_saved, "second_q": second_saved}
@ac.module
def WideInitial(incoming: ac.u8) -> {"first_q": ac.bits[65], "second_q": ac.bits[130]}:
    first: ac.bits[65] = (1 << 64) + 1
    second: ac.bits[130] = (1 << 129) + 3
    hold_wide(first, second)
    return {"first_q": first, "second_q": second}
@ac.module
def FourStateInitial(incoming: ac.u8) -> {"first_q": ac.bits[65], "second_q": ac.bits[130]}:
    first: ac.bits[65] = ac.four_state("__WIDE65__")
    second: ac.bits[130] = ac.four_state("__WIDE130__")
    hold_wide(first, second)
    return {"first_q": first, "second_q": second}
""".replace(
    "__WIDE65__", WIDE65
).replace(
    "__WIDE130__", WIDE130
)


def _environment() -> dict[str, str]:
    environment = os.environ.copy()
    environment["PYTHONPATH"] = os.pathsep.join(
        item
        for item in (str(ROOT / "python"), environment.get("PYTHONPATH", ""))
        if item
    )
    return environment


def _run(
    arguments: list[str], *, timeout: int = 180
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        arguments,
        cwd=ROOT,
        env=_environment(),
        text=True,
        capture_output=True,
        check=False,
        timeout=timeout,
    )


def _ok(result: subprocess.CompletedProcess[str]) -> str:
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def _cli(*arguments: str) -> subprocess.CompletedProcess[str]:
    return _run([sys.executable, "-m", "pycircuit.cli", *arguments], timeout=900)


def _toolchain_root() -> Path:
    configured = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if configured:
        return Path(configured).resolve()
    candidate = ROOT / ".pycircuit_out/toolchain/install"
    assert (
        candidate / "share/pycircuit/cmake/pycircuitConfig.cmake"
    ).is_file(), candidate
    return candidate


def _build_structural_cpp(generated: Path, tmp_path: Path) -> Path:
    """Build a generated structural bundle and return its static library."""
    build = tmp_path / "cpp-build"
    configured = _run(
        [
            "cmake",
            "-S",
            str(generated),
            "-B",
            str(build),
            "-G",
            "Ninja",
            "-DCMAKE_PREFIX_PATH=" + str(_toolchain_root()),
        ],
        timeout=180,
    )
    _ok(configured)
    _ok(
        _run(
            [
                "cmake",
                "--build",
                str(build),
                "--target",
                "pycircuit_modules",
                "--parallel",
                "4",
            ],
            timeout=900,
        )
    )
    library = build / (
        "pycircuit_modules.lib" if os.name == "nt" else "libpycircuit_modules.a"
    )
    assert library.is_file(), library
    return library


def _compile_final(tmp_path: Path, source_text: str, top: str) -> Path:
    source = tmp_path / "negative.py"
    source.write_text(source_text, encoding="utf-8")
    unit = tmp_path / "negative-unit"
    _ok(
        _cli(
            "compile",
            "-c",
            str(source),
            "--source-root",
            str(tmp_path),
            "--package-prefix",
            "negative",
            "-o",
            str(unit),
        )
    )
    final = tmp_path / "negative.ac"
    _ok(_cli("link", str(unit), "--top", f"negative.negative.{top}", "-o", str(final)))
    return final


def _physical_rejected(final: Path, tmp_path: Path, diagnostic: str) -> None:
    output = tmp_path / "protected-output"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(output)))
    before = {
        p.relative_to(output): p.read_bytes() for p in output.rglob("*") if p.is_file()
    }
    rejected = _cli(
        "emit",
        str(final),
        "--target",
        "verilog",
        "--rtl-mode",
        "physical",
        "-o",
        str(output),
        "--replace",
    )
    assert rejected.returncode != 0
    assert diagnostic in rejected.stdout + rejected.stderr
    assert before == {
        p.relative_to(output): p.read_bytes() for p in output.rglob("*") if p.is_file()
    }


@pytest.fixture(scope="module")
def physical_designs(tmp_path_factory: pytest.TempPathFactory):
    root = tmp_path_factory.mktemp("physical-hwir-export")
    source = root / "design.py"
    source.write_text(SOURCE, encoding="utf-8")
    unit = root / "unit"
    _ok(
        _cli(
            "compile",
            "-c",
            str(source),
            "--source-root",
            str(root),
            "--package-prefix",
            "physical",
            "-o",
            str(unit),
        )
    )
    result = {}
    for name in ("Pair", "WideInitial", "FourStateInitial"):
        final = root / f"{name}.ac"
        _ok(
            _cli(
                "link", str(unit), "--top", f"physical.design.{name}", "-o", str(final)
            )
        )
        output = root / name
        _ok(
            _cli(
                "emit",
                str(final),
                "--target",
                "verilog",
                "--rtl-mode",
                "physical",
                "-o",
                str(output),
            )
        )
        result[name] = (final, output)
    return result


def _simulate(output: Path, tmp_path: Path, body: str, *, reset: bool = True) -> str:
    assert shutil.which("iverilog") and shutil.which("vvp"), "four-state tools required"
    manifest = json.loads((output / "generated.json").read_text(encoding="utf-8"))
    ports = json.loads((output / "ports.json").read_text(encoding="utf-8"))
    assert ports["schema_version"] == 1
    assert ports["target"] == "verilog"
    assert ports["controls"] == []
    declarations, bindings = [], []
    data_names = set()
    domains = ports["domains"]
    for row in ports["inputs"]:
        name = (
            "clk"
            if row["ordinal"] == domains["clock"]
            else "rst" if row["ordinal"] == domains["reset"] else row["name"]
        )
        declarations.append(f"logic [{row['width'] - 1}:0] {name};")
        bindings.append(f".{row['wire']}({name})")
        if name not in {"clk", "rst"}:
            data_names.add(name)
    for index, row in enumerate(ports["outputs"]):
        declarations.append(f"wire [{row['width'] - 1}:0] result_{index};")
        bindings.append(f".{row['wire']}(result_{index})")
    tb = tmp_path / "physical_tb.sv"
    initial_values = {
        "first": "8'h12",
        "second": "8'h34",
        "enable_first": "1",
        "enable_second": "1",
        "incoming": "8'h3c",
        "value": "8'h3c",
        "valid": "1",
        "take": "0",
        "address": "0",
        "raddr": "0",
        "raddr0": "0",
        "raddr1": "0",
        "ren0": "0",
        "ren1": "0",
        "wvalid": "0",
        "waddr": "0",
        "wdata": "0",
        "wstrb": "0",
        "gauge": "8'h01",
    }
    initialization = "".join(
        f"{name}={initial_values[name]};" for name in sorted(data_names)
    )
    tb.write_text(
        "module tb;\n"
        + "\n".join(declarations)
        + "\n"
        + "pyc_root dut("
        + ",".join(bindings)
        + ");\n"
        + "task edge_up; #2;clk=1;#2;endtask\n"
        "task edge_down; #2;clk=0;#2;endtask\n"
        "initial begin clk=0;rst=1;"
        + initialization
        + "\n"
        + ("edge_up();edge_down();rst=0;\n" if reset else "#1;rst=0;\n")
        + body
        + '\n$display("PHYSICAL_TEST_COMPLETE");$finish;end\nendmodule\n',
        encoding="utf-8",
    )
    rtl = [
        str(output / row["path"])
        for row in manifest["files"]
        if row["path"].endswith((".v", ".sv"))
    ]
    binary = tmp_path / "physical.vvp"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "tb",
                "-o",
                str(binary),
                *rtl,
                str(ROOT / "include/verilog/dff.v"),
                str(ROOT / "include/verilog/dffe.v"),
                str(ROOT / "include/verilog/fifo.v"),
                str(ROOT / "include/verilog/byte_mem.v"),
                str(ROOT / "include/verilog/sync_mem.v"),
                str(ROOT / "include/verilog/sync_mem_dp.v"),
                str(tb),
            ]
        )
    )
    transcript = _ok(_run(["vvp", str(binary)]))
    assert "PHYSICAL_TEST_COMPLETE" in transcript
    return transcript


def _synthesis_elaborates(output: Path, tmp_path: Path) -> None:
    manifest = json.loads((output / "generated.json").read_text(encoding="utf-8"))
    rtl = [
        str(output / row["path"])
        for row in manifest["files"]
        if row["path"].endswith((".v", ".sv"))
    ]
    primitives = [
        str(ROOT / "include/verilog" / name)
        for name in (
            "dff.v",
            "dffe.v",
            "fifo.v",
            "byte_mem.v",
            "sync_mem.v",
            "sync_mem_dp.v",
        )
    ]
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-DSYNTHESIS",
                "-s",
                "pyc_root",
                "-o",
                str(tmp_path / "synthesis.vvp"),
                *rtl,
                *primitives,
            ]
        )
    )


def test_physical_reset_hold_and_four_state_payload(
    physical_designs, tmp_path: Path
) -> None:
    transcript = _simulate(
        physical_designs["Pair"][1],
        tmp_path,
        """
if(result_0!==0 || result_1!==0)$fatal(1,"reset");
edge_up();
if(result_0!==8'h12 || result_1!==8'h34)$fatal(1,"first edge");
edge_down();enable_first=0;enable_second=1;first=8'hff;second=8'b10xz0011;edge_up();
if(result_0!==8'h12 || result_1!==8'b10xz0011)$fatal(1,"hold/payload");
edge_down();rst=1;enable_first=1'bx;enable_second=1'bz;edge_up();
if(result_0!==0 || result_1!==0)$fatal(1,"reset priority");
""",
    )
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript


@pytest.mark.parametrize("fault", ["guard", "reset", "clock"])
def test_physical_fault_is_sticky_and_prevents_partial_commit(
    physical_designs, tmp_path: Path, fault: str
) -> None:
    inject = {
        "guard": "enable_second=1'bx;edge_up();",
        "reset": "rst=1'bx;edge_up();",
        "clock": "#2;clk=1'bx;#2;",
    }[fault]
    transcript = _simulate(
        physical_designs["Pair"][1],
        tmp_path,
        "edge_up();edge_down();first=8'haa;second=8'hbb;"
        + inject
        + """
if(result_0!==8'h12 || result_1!==8'h34)$fatal(1,"partial commit");
clk=0;rst=0;enable_first=1;enable_second=1;#2;edge_up();
if(result_0!==8'h12 || result_1!==8'h34)$fatal(1,"sticky failure");
""",
    )
    assert transcript.count("PYCIRCUIT_PHYSICAL_FAULT") == 1


def test_wide_static_initializer_precedes_clock_and_reset(
    physical_designs, tmp_path: Path
) -> None:
    transcript = _simulate(
        physical_designs["WideInitial"][1],
        tmp_path,
        """
if(result_0!==65'h10000000000000001)$fatal(1,"65 bit startup");
if(result_1!==130'h200000000000000000000000000000003)$fatal(1,"130 bit startup");
edge_up();
if(result_0!==65'h10000000000000001)$fatal(1,"startup hold");
""",
        reset=False,
    )
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript


def test_four_state_static_initializer_preserves_all_planes(
    physical_designs, tmp_path: Path
) -> None:
    transcript = _simulate(
        physical_designs["FourStateInitial"][1],
        tmp_path,
        f"""
if(result_0!==65'b{WIDE65})$fatal(1,"65 bit four-state startup");
if(result_1!==130'b{WIDE130})$fatal(1,"130 bit four-state startup");
edge_up();
if(result_0!==65'b{WIDE65} || result_1!==130'b{WIDE130})
  $fatal(1,"four-state startup hold");
""",
        reset=False,
    )
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript


def test_python_four_state_values_reach_both_hwir_backends(tmp_path: Path) -> None:
    final = _compile_final(
        tmp_path,
        f"""import pycircuit as ac
@ac.module
def FourState() -> {{'one': ac.u1, 'wide65': ac.bits[65],
                     'wide130': ac.bits[130], 'exact': ac.u1,
                     'known': ac.u1}}:
    one = ac.four_state('z')
    wide65 = ac.four_state({WIDE65!r})
    wide130 = ac.four_state({WIDE130!r})
    exact = ac.case_eq(wide65, ac.four_state({WIDE65!r}))
    known = ac.is_known(wide130)
    return {{'one': one, 'wide65': wide65, 'wide130': wide130,
             'exact': exact, 'known': known}}
""",
        "FourState",
    )
    cpp = tmp_path / "four-state-cpp"
    rtl = tmp_path / "four-state-verilog"
    _ok(_cli("emit", str(final), "--target", "structural-cpp", "-o", str(cpp)))
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(rtl)))
    _build_structural_cpp(cpp, tmp_path)
    cpp_text = "\n".join(
        path.read_text(encoding="utf-8")
        for path in cpp.rglob("*")
        if path.suffix in {".cpp", ".hpp"}
    )
    rtl_text = "\n".join(
        path.read_text(encoding="utf-8")
        for path in rtl.rglob("*")
        if path.suffix in {".v", ".sv"}
    )
    assert "gfsim::FourState<1>::fromMasks" in cpp_text
    assert "gfsim::FourState<65>::fromMasks" in cpp_text
    assert "gfsim::FourState<130>::fromMasks" in cpp_text
    assert "gfsim::ComparePredicate::CASE_EQ" in cpp_text
    assert "1'bz" in rtl_text
    assert f"65'b{WIDE65}" in rtl_text
    assert f"130'b{WIDE130}" in rtl_text
    assert " === " in rtl_text


def test_python_four_state_structural_cpp_runs_from_generated_bundle(
    tmp_path: Path,
) -> None:
    final = _compile_final(
        tmp_path,
        "import pycircuit as ac\n"
        "@ac.module\ndef FourState() -> {'value': ac.u4, 'known': ac.u1}:\n"
        "    value = ac.four_state('10xz')\n"
        "    known = ac.is_known(value)\n"
        "    return {'value': value, 'known': known}\n",
        "FourState",
    )
    generated = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "structural-cpp", "-o", str(generated)))
    _build_structural_cpp(generated, tmp_path)
    driver = generated / "four_state_driver.cpp"
    driver.write_text(
        '#include "design.hpp"\n'
        "#include <iostream>\n"
        "int main(){ pyc_dut dut; dut.system().Build(); dut.system().Reset(); "
        "if(dut.system().Step()==gfsim::SimStepResult::InvalidState) return 1; "
        "auto out=dut.sample(); if(out.value.packed().value().value()!=8 || "
        "out.value.packed().knownMask().value()!=12 || "
        "out.value.packed().zMask().value()!=1 || "
        "out.known.packed().value().value()!=0) return 2; "
        'std::cout << "FOUR_STATE_CPP_OK\\n"; }\n',
        encoding="utf-8",
    )
    build = tmp_path / "cpp-driver-build"
    cmake = tmp_path / "driver.cmake"
    cmake.write_text(
        "cmake_minimum_required(VERSION 3.25)\nproject(FourStateDriver LANGUAGES CXX)\n"
        "find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)\n"
        "add_executable(four_state_driver four_state_driver.cpp)\n"
        "target_include_directories(four_state_driver PRIVATE .)\n"
        "target_link_libraries(four_state_driver PRIVATE pycircuit::pyc6_runtime)\n",
        encoding="utf-8",
    )
    # Build the generated translation unit and the small independent driver
    # together, using the installed Runtime package exactly like public users.
    generated_cmake = generated / "CMakeLists.txt"
    original = generated_cmake.read_text(encoding="utf-8")
    generated_cmake.write_text(
        original
        + "\nadd_executable(four_state_driver four_state_driver.cpp)\ntarget_link_libraries(four_state_driver PRIVATE pycircuit_modules)\n",
        encoding="utf-8",
    )
    try:
        _ok(
            _run(
                [
                    "cmake",
                    "-S",
                    str(generated),
                    "-B",
                    str(build),
                    "-G",
                    "Ninja",
                    "-DCMAKE_PREFIX_PATH=" + str(_toolchain_root()),
                ],
                timeout=180,
            )
        )
        _ok(
            _run(
                [
                    "cmake",
                    "--build",
                    str(build),
                    "--target",
                    "four_state_driver",
                    "--parallel",
                    "4",
                ],
                timeout=900,
            )
        )
        executable = build / "four_state_driver"
        if os.name == "nt":
            executable = executable.with_suffix(".exe")
        assert "FOUR_STATE_CPP_OK" in _ok(_run([str(executable)], timeout=120))
    finally:
        generated_cmake.write_text(original, encoding="utf-8")


def test_synthesis_preprocessing_has_no_sticky_fault_history(
    physical_designs, tmp_path: Path
) -> None:
    output = physical_designs["Pair"][1]
    manifest = json.loads((output / "generated.json").read_text(encoding="utf-8"))
    rtl = [
        str(output / row["path"])
        for row in manifest["files"]
        if row["path"].endswith((".v", ".sv"))
    ] + [str(ROOT / "include/verilog/dff.v"), str(ROOT / "include/verilog/dffe.v")]
    preprocessed = tmp_path / "synthesis.sv"
    _ok(
        _run(["iverilog", "-g2012", "-DSYNTHESIS", "-E", "-o", str(preprocessed), *rtl])
    )
    assert "pyc_physical_fault" not in preprocessed.read_text(encoding="utf-8")


def test_physical_mode_rejects_non_verilog_without_replacing_output(
    physical_designs, tmp_path: Path
) -> None:
    final, _ = physical_designs["Pair"]
    output = tmp_path / "structural-cpp"
    _ok(_cli("emit", str(final), "--target", "structural-cpp", "-o", str(output)))
    before = {
        p.relative_to(output): p.read_bytes() for p in output.rglob("*") if p.is_file()
    }
    rejected = _cli(
        "emit",
        str(final),
        "--target",
        "structural-cpp",
        "--rtl-mode",
        "physical",
        "-o",
        str(output),
        "--replace",
    )
    assert rejected.returncode != 0
    assert before == {
        p.relative_to(output): p.read_bytes() for p in output.rglob("*") if p.is_file()
    }


def test_physical_rejects_closed_system_and_preserves_managed_output(
    tmp_path: Path,
) -> None:
    final = _compile_final(
        tmp_path,
        "import pycircuit as ac\n@ac.system\ndef Closed():\n    value: ac.u8 = 0\n",
        "Closed",
    )
    _physical_rejected(
        final,
        tmp_path,
        "physical RTL requires an externally driven ordinary module root",
    )


def test_physical_observation_channel_has_metadata_and_committed_payload(
    tmp_path: Path,
) -> None:
    final = _compile_final(
        tmp_path,
        "import pycircuit as ac\n"
        "@ac.rule\ndef watch(value: ac.u8) -> None:\n    ac.log('info', 'value', value)\n"
        "@ac.module\ndef Observed(value: ac.u8) -> {'result': ac.u8}:\n"
        "    saved: ac.u8 = 0\n"
        "    @ac.rule\n    def update() -> None:\n        saved = value\n"
        "    update()\n    watch(value)\n"
        "    return {'result': saved}\n",
        "Observed",
    )
    output = tmp_path / "observed-physical"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "physical",
            "-o",
            str(output),
        )
    )
    ports = json.loads((output / "ports.json").read_text(encoding="utf-8"))
    channel = ports["observation_channel"]
    assert channel == {
        "protocol": "pycircuit-physical-observation-v1",
        "timing": "registered_on_committed_rising_edge",
        "count": 1,
        "slot_width": 8,
        "valid": "pyc_observation_valid",
        "path": "pyc_observation_path",
        "value": "pyc_observation_value",
        "known": "pyc_observation_known",
        "z": "pyc_observation_z",
    }
    assert ports["observations"][0]["kind"] == "log"
    transcript = _simulate(
        output,
        tmp_path,
        """
edge_up();#1;
if(dut.pyc_observation_valid!==1'b1)$fatal(1,"observation valid");
if(dut.pyc_observation_value!==8'h3c)$fatal(1,"input observation");
if(dut.pyc_observation_known!==8'hff || dut.pyc_observation_z!==0)$fatal(1,"planes");
edge_down();value=8'h5a;edge_up();#1;
if(dut.pyc_observation_value!==8'h5a)$fatal(1,"next input observation");
edge_down();edge_up();#1;
if(dut.pyc_observation_value!==8'h5a)$fatal(1,"stable observation");
""",
    )
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript


@pytest.mark.parametrize(
    ("top", "source_text"),
    [
        (
            "Queued",
            "import pycircuit as ac\n@ac.module\ndef Queued(valid: ac.u1, value: ac.u8, take: ac.u1) -> {'result': ac.u8}:\n"
            "    ready, available, data = ac.queue[ac.u8](valid, value, take, depth=1)\n"
            "    return {'result': data}\n",
        ),
        (
            "Memory",
            "import pycircuit as ac\n@ac.module\ndef Memory(value: ac.u8, address: ac.u1) -> {'result': ac.u8}:\n"
            "    raw = ac.sync_mem[ac.u8](1 == 1, address, 1 == 1, address, value, 1, depth=2)\n"
            "    return {'result': raw}\n",
        ),
    ],
)
def test_physical_fifo_and_memory_storage(
    tmp_path: Path, top: str, source_text: str
) -> None:
    final = _compile_final(tmp_path, source_text, top)
    output = tmp_path / f"{top}-physical"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "physical",
            "-o",
            str(output),
        )
    )
    body = (
        """
if(result_0!==0)$fatal(1,"fifo reset");
edge_up();#1;
if(result_0!==8'h3c)$fatal(1,"fifo payload");
edge_down();valid=0;take=1;edge_up();
edge_down();valid=1;take=0;value=8'hxx;edge_up();#1;
if(dut.pyc_root_commit_ok!==1'b1)$fatal(1,"fifo unknown payload permit");
if(result_0!==8'hxx)$fatal(1,"fifo unknown payload preservation");
"""
        if top == "Queued"
        else """
edge_up();edge_down();value=8'ha5;address=1;edge_up();edge_down();edge_up();#1;
if(result_0!==8'ha5)$fatal(1,"memory round trip");
"""
    )
    transcript = _simulate(output, tmp_path, body)
    _synthesis_elaborates(output, tmp_path)
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript


@pytest.mark.parametrize(
    ("top", "source_text", "signal"),
    [
        (
            "MemoryFault",
            "import pycircuit as ac\n@ac.module\ndef MemoryFault(value: ac.u8, address: ac.u2, wvalid: ac.u1) -> {'result': ac.u8}:\n"
            "    raw = ac.sync_mem[ac.u8](1 == 1, address, wvalid, address, value, 1, depth=4)\n"
            "    return {'result': raw}\n",
            "address",
        ),
        (
            "MemoryEnableFault",
            "import pycircuit as ac\n@ac.module\ndef MemoryEnableFault(value: ac.u8, address: ac.u2, wvalid: ac.u1) -> {'result': ac.u8}:\n"
            "    raw = ac.sync_mem[ac.u8](1 == 1, address, wvalid, address, value, 1, depth=4)\n"
            "    return {'result': raw}\n",
            "wvalid",
        ),
    ],
)
def test_physical_memory_unknown_control_is_sticky(
    tmp_path: Path, top: str, source_text: str, signal: str
) -> None:
    final = _compile_final(tmp_path, source_text, top)
    output = tmp_path / f"{top}-physical"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "physical",
            "-o",
            str(output),
        )
    )
    injected = "address=2'bxx;" if signal == "address" else "wvalid=1'bx;"
    transcript = _simulate(
        output,
        tmp_path,
        "wvalid=1;address=1;value=8'h5a;edge_up();edge_down();"
        + injected
        + 'edge_up();#1;if(dut.pyc_root_commit_ok!==1\'b0)$fatal(1,"memory fault permit");'
        + "clk=0;wvalid=0;address=1;#2;edge_up();#1;"
        + 'if(result_0!==0)$fatal(1,"sticky memory commit");',
    )
    assert transcript.count("PYCIRCUIT_PHYSICAL_FAULT") == 1


def test_physical_fifo_depth_two_latency_and_full_replace(tmp_path: Path) -> None:
    final = _compile_final(
        tmp_path,
        "import pycircuit as ac\n@ac.module\ndef QueuePolicy(valid: ac.u1, value: ac.u8, take: ac.u1) -> {'ready': ac.u1, 'available': ac.u1, 'data': ac.u8}:\n"
        "    ready, available, data = ac.queue[ac.u8](valid, value, take, depth=2, latency=2, ready_policy='downstream_pop')\n"
        "    return {'ready': ready, 'available': available, 'data': data}\n",
        "QueuePolicy",
    )
    output = tmp_path / "queue-policy-physical"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "physical",
            "-o",
            str(output),
        )
    )
    transcript = _simulate(
        output,
        tmp_path,
        "value=8'h11;valid=1;take=0;edge_up();edge_down();"
        "value=8'h22;edge_up();edge_down();"
        "if(result_1!==1'b1 || result_2!==8'h11)$fatal(1,\"latency/head\");"
        "value=8'h33;take=1;edge_up();edge_down();"
        "if(result_1!==1'b1 || result_2!==8'h22)$fatal(1,\"full pop push\");",
    )
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript


@pytest.mark.parametrize(
    ("top", "source_text", "body"),
    [
        (
            "ByteMemory",
            "import pycircuit as ac\n@ac.module\ndef ByteMemory(raddr: ac.u2, wvalid: ac.u1, waddr: ac.u2, wdata: ac.u16, wstrb: ac.u2) -> {'result': ac.u16}:\n"
            "    raw = ac.byte_mem[ac.u16](raddr, wvalid, waddr, wdata, wstrb, depth=4)\n"
            "    return {'result': raw}\n",
            """
wvalid=1;waddr=1;wdata=16'hbeef;wstrb=2'b11;edge_up();edge_down();
wvalid=0;raddr=1;#1;if(result_0!==16'hbeef)$fatal(1,"byte memory");
raddr=2'bxx;edge_up();#1;
if(dut.pyc_root_commit_ok!==1'b0)$fatal(1,"byte fault permit");
clk=0;raddr=1;wvalid=1;waddr=1;wdata=16'h1234;#2;edge_up();edge_down();
if(result_0!==16'hbeef)$fatal(1,"byte sticky write");
""",
        ),
        (
            "DualMemory",
            "import pycircuit as ac\n@ac.module\ndef DualMemory(ren0: ac.u1, raddr0: ac.u2, ren1: ac.u1, raddr1: ac.u2, wvalid: ac.u1, waddr: ac.u2, wdata: ac.u8, wstrb: ac.u1) -> {'first': ac.u8, 'second': ac.u8}:\n"
            "    first, second = ac.sync_mem_dp[ac.u8](ren0, raddr0, ren1, raddr1, wvalid, waddr, wdata, wstrb, depth=4)\n"
            "    return {'first': first, 'second': second}\n",
            """
wvalid=1;waddr=2;wdata=8'ha6;wstrb=1;edge_up();edge_down();
wvalid=0;ren0=1;ren1=1;raddr0=2;raddr1=2;edge_up();#1;
if(result_0!==8'ha6 || result_1!==8'ha6)$fatal(1,"dual memory");
""",
        ),
    ],
)
def test_physical_byte_and_dual_port_memory(
    tmp_path: Path, top: str, source_text: str, body: str
) -> None:
    final = _compile_final(tmp_path, source_text, top)
    output = tmp_path / f"{top}-physical"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "physical",
            "-o",
            str(output),
        )
    )
    transcript = _simulate(output, tmp_path, body)
    _synthesis_elaborates(output, tmp_path)
    if top == "ByteMemory":
        assert transcript.count("PYCIRCUIT_PHYSICAL_FAULT") == 1
    else:
        assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript


def test_physical_observation_group_four_state_and_fault_discard(
    tmp_path: Path,
) -> None:
    final = _compile_final(
        tmp_path,
        "import pycircuit as ac\n"
        "@ac.rule\ndef watch(value: ac.u8, gauge: ac.u8) -> None:\n"
        "    ac.log('info', 'pair', value, value)\n"
        "    ac.report('level', gauge)\n"
        "@ac.module\ndef ObservedGroup(value: ac.u8, gauge: ac.u8) -> {'result': ac.u8}:\n"
        "    saved: ac.u8 = 0\n"
        "    @ac.rule\n    def hold() -> None:\n        saved = value\n"
        "    hold()\n    watch(value, gauge)\n    return {'result': saved}\n",
        "ObservedGroup",
    )
    output = tmp_path / "observed-group-physical"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "physical",
            "-o",
            str(output),
        )
    )
    ports = json.loads((output / "ports.json").read_text(encoding="utf-8"))
    assert ports["observation_channel"]["count"] == 3
    assert [row["group_size"] for row in ports["observations"]] == [2, 2, 1]
    transcript = _simulate(
        output,
        tmp_path,
        """
edge_up();#1;
if(dut.pyc_observation_valid!==3'b111)$fatal(1,"group publish");
edge_down();value=8'b10xz0011;edge_up();#1;
if(dut.pyc_observation_valid!==3'b111)$fatal(1,"four state publish");
if(dut.pyc_observation_value[7:0]!==8'b10000011)$fatal(1,"value plane");
if(dut.pyc_observation_known[7:0]!==8'b11001111)$fatal(1,"known plane");
if(dut.pyc_observation_z[7:0]!==8'b00010000)$fatal(1,"z plane");
edge_down();value=8'h55;gauge=8'hxx;edge_up();#1;
if(dut.pyc_observation_valid!==3'b000)$fatal(1,"fault discard");
edge_down();gauge=8'h01;edge_up();#1;
if(dut.pyc_observation_valid!==3'b000)$fatal(1,"sticky observation");
""",
    )
    _synthesis_elaborates(output, tmp_path)
    assert transcript.count("PYCIRCUIT_PHYSICAL_FAULT") == 1


def test_physical_wide_observation_preserves_130_bit_planes(tmp_path: Path) -> None:
    value_plane = "".join("1" if bit == "1" else "0" for bit in WIDE130)
    known_plane = "".join("1" if bit in "01" else "0" for bit in WIDE130)
    z_plane = "".join("1" if bit == "z" else "0" for bit in WIDE130)
    final = _compile_final(
        tmp_path,
        f"import pycircuit as ac\n@ac.rule\ndef watch(value: ac.bits[130]) -> None:\n    ac.log('info', 'wide', value)\n"
        f"@ac.rule\ndef hold(value: ac.bits[130]) -> None:\n    value = value\n"
        f"@ac.module\ndef WideObserved() -> {{'result': ac.bits[130]}}:\n"
        f"    value: ac.bits[130] = ac.four_state({WIDE130!r})\n"
        f"    hold(value)\n    watch(value)\n    return {{'result': value}}\n",
        "WideObserved",
    )
    output = tmp_path / "wide-observed-physical"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "physical",
            "-o",
            str(output),
        )
    )
    ports = json.loads((output / "ports.json").read_text(encoding="utf-8"))
    assert ports["observation_channel"]["slot_width"] == 130
    transcript = _simulate(
        output,
        tmp_path,
        f'edge_up();#1;if(dut.pyc_observation_valid!==1\'b1)$fatal(1,"wide valid");'
        f'if(dut.pyc_observation_value!==130\'b{value_plane})$fatal(1,"wide value");'
        f'if(dut.pyc_observation_known!==130\'b{known_plane})$fatal(1,"wide known");'
        f'if(dut.pyc_observation_z!==130\'b{z_plane})$fatal(1,"wide z");',
    )
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript


def test_physical_nested_observation_routes_through_explicit_sideband(
    tmp_path: Path,
) -> None:
    final = _compile_final(
        tmp_path,
        "import pycircuit as ac\n"
        "@ac.struct\nclass Result:\n    result: ac.u8\n"
        "@ac.rule\ndef watch_leaf(value: ac.u8) -> None:\n    ac.log('info', 'leaf', value)\n"
        "@ac.module\ndef Leaf(value: ac.u8) -> Result:\n"
        "    saved: ac.u8 = 0\n"
        "    watch_leaf(value)\n"
        "    return Result(result=saved)\n"
        "@ac.module\ndef NestedObserved(value: ac.u8) -> Result:\n"
        "    leaf = Leaf(value)\n    return Result(result=leaf.result)\n",
        "NestedObserved",
    )
    output = tmp_path / "nested-observed-physical"
    _ok(
        _cli(
            "emit",
            str(final),
            "--target",
            "verilog",
            "--rtl-mode",
            "physical",
            "-o",
            str(output),
        )
    )
    ports = json.loads((output / "ports.json").read_text(encoding="utf-8"))
    assert ports["observations"][0]["instance"].startswith("root/")
    source = "\n".join(
        (output / row["path"]).read_text(encoding="utf-8")
        for row in json.loads((output / "generated.json").read_text(encoding="utf-8"))[
            "files"
        ]
        if row["path"].endswith((".sv", ".v"))
    )
    assert ".pyc_observation_path_raw(" in source
    assert "dut.pyc_net_" not in source
    transcript = _simulate(
        output,
        tmp_path,
        'edge_up();#1;if(dut.pyc_observation_value!==8\'h3c)$fatal(1,"nested observation");',
    )
    _synthesis_elaborates(output, tmp_path)
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript
