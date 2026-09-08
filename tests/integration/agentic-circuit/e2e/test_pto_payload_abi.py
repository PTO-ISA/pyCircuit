from __future__ import annotations

import copy
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE, lower_queue_source

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "compiler/acir/tools"))

from pto_payload_abi import (  # noqa: E402
    PACKED_BITS,
    pack_payload,
    project_davincioo_record,
)
from pto_trace_adapter import parse_davincioo_jsonl  # noqa: E402

EXAMPLE = ROOT / "examples/agentic-circuit/pipelines/pto_payload_abi.py"
TRACE = (
    ROOT
    / "third_party/references/davincioo-gfsim/upstream/tests/fixtures/traces"
    / "examples_intermediate_softmax.pto.trace"
)
PROJECTION = ROOT / "tests/goldens/agentic-circuit/davincioo/softmax-projection.json"
DEFAULT_TOOLCHAIN = ROOT / ".pycircuit_out/toolchain/install"
ABI = json.loads(
    (ROOT / "schemas/agentic-circuit/pto-payload-abi.json").read_bytes()
)


def _words(data: bytes, width: int) -> list[int]:
    return _value_words(int.from_bytes(data, "little"), width)


def _value_words(value: int, width: int) -> list[int]:
    return [
        (value >> (64 * index)) & ((1 << 64) - 1) for index in range((width + 63) // 64)
    ]


def _fixture() -> tuple[dict[str, object], list[int], list[int]]:
    records = parse_davincioo_jsonl(TRACE.read_bytes())
    projection = json.loads(PROJECTION.read_bytes())
    payload = project_davincioo_record(
        records[4],
        opcode_ids=projection["opcode_ids"],
        routes=projection["routes"],
    )
    expected = copy.deepcopy(payload)
    expected["block_id"] += 1
    return (
        payload,
        _words(pack_payload(payload), PACKED_BITS),
        _words(pack_payload(expected), PACKED_BITS),
    )


def _plan_leaf_layout(plan: dict[str, object]) -> list[dict[str, int | str]]:
    payloads = {item["name"]: item for item in plan["payloads"]}
    enums = {item["name"]: item for item in plan["enums"]}
    aggregates = {item["type"]: item for item in plan["aggregates"]}

    def walk(value_type: str, prefix: str) -> list[tuple[str, int]]:
        if value_type.startswith("i"):
            return [(prefix, int(value_type[1:]))]
        enum_prefix = "!ac.enum<@types::@"
        struct_prefix = "!ac.struct<@types::@"
        if value_type.startswith(enum_prefix):
            name = value_type[len(enum_prefix) : -1]
            return [(prefix, enums[name]["width"])]
        if value_type.startswith(struct_prefix):
            name = value_type[len(struct_prefix) : -1]
            leaves: list[tuple[str, int]] = []
            for field in payloads[name]["fields"]:
                path = f"{prefix}.{field['name']}" if prefix else field["name"]
                nested = walk(field["type"], path)
                assert sum(width for _, width in nested) == field["width"]
                leaves.extend(nested)
            return leaves
        aggregate = aggregates[value_type]
        if aggregate["kind"] == "array":
            element_types = [aggregate["elements"][0]] * aggregate["length"]
        else:
            element_types = aggregate["elements"]
        leaves = []
        for index, element_type in enumerate(element_types):
            leaves.extend(walk(element_type, f"{prefix}[{index}]"))
        assert sum(width for _, width in leaves) == aggregate["width"]
        return leaves

    declared = walk("!ac.struct<@types::@PTOExecutionPayload>", "")
    cursor = sum(width for _, width in declared)
    result: list[dict[str, int | str]] = []
    for path, width in declared:
        cursor -= width
        result.append({"path": path, "width": width, "lsb": cursor})
    assert cursor == 0
    return result


def _assert_plan_matches_abi(test: unittest.TestCase, plan: dict[str, object]) -> None:
    published = [
        {key: field[key] for key in ("path", "width", "lsb")}
        for field in ABI["fields"]
    ]
    test.assertEqual(published, _plan_leaf_layout(plan))
    enums = {item["name"]: item["enumerants"] for item in plan["enums"]}
    test.assertEqual(
        ABI["catalogs"]["engine_kind"],
        {name.lower(): index for index, name in enumerate(enums["EngineKind"])},
    )
    test.assertEqual(
        ABI["catalogs"]["dtype"],
        {name.lower(): index for index, name in enumerate(enums["DType"])},
    )
    test.assertEqual(
        ABI["catalogs"]["tile_layout"],
        {name: index for index, name in enumerate(enums["TileLayout"])},
    )


class PTOPayloadABITest(unittest.TestCase):
    def test_wide_payload_generates_gfsim_cpp_from_the_same_schema(self) -> None:
        cxx = shutil.which("c++")
        tools = {
            "opt": ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt",
            "plan": ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-plan",
            "cxxgen": ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen",
        }
        if cxx is None or any(not path.is_file() for path in tools.values()):
            self.skipTest("C++ and native QueueGraph tools are required")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            acir = root / "model.mlir"
            plan = root / "plan.json"
            model = root / "model.cpp"
            generated = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools/ac-queue-cxxgen.py"),
                    str(EXAMPLE),
                    "--system",
                    "pto_payload_abi",
                    "--acir-output",
                    str(acir),
                    "--plan-output",
                    str(plan),
                    "--acir-opt",
                    str(tools["opt"]),
                    "--queue-plan-tool",
                    str(tools["plan"]),
                    "--queue-cxxgen-tool",
                    str(tools["cxxgen"]),
                    "--output",
                    str(model),
                ),
                cwd=ROOT,
                env={
                    **os.environ,
                    "PYTHONPATH": os.pathsep.join(
                        (
                            str(ROOT / "python/semantic-core/src"),
                            str(ROOT / "python/agentic-circuit/src"),
                        )
                    ),
                },
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, generated.returncode, generated.stderr)
            plan_value = json.loads(plan.read_bytes())
            _assert_plan_matches_abi(self, plan_value)
            source = model.read_text(encoding="utf-8")
            self.assertIn("gfsim::UInt<616> input_tiles", source)
            self.assertIn("gfsim::UInt<276> scalar_inputs", source)

            payload, input_words, expected_words = _fixture()
            packed = int.from_bytes(pack_payload(payload), "little")

            def field_value(path: str) -> int:
                field = next(item for item in ABI["fields"] if item["path"] == path)
                return (packed >> field["lsb"]) & ((1 << field["width"]) - 1)

            def initializer(value: int, width: int) -> str:
                return ", ".join(
                    f"0x{word:016x}ULL" for word in _value_words(value, width)
                )

            input_tiles = (packed >> 589) & ((1 << 616) - 1)
            scalar_inputs = (packed >> 310) & ((1 << 276) - 1)
            output_tiles = packed & ((1 << 308) - 1)
            harness = root / "harness.cpp"
            executable = root / "gfsim_model"
            harness_source = '''#include "@MODEL@"
#include <cstdint>
#include <iostream>

int main() {
  ac_generated::PTOExecutionPayload input{
      gfsim::UInt<16>{@OPCODE@},
      static_cast<ac_generated::EngineKind>(@ENGINE@),
      gfsim::UInt<16>{@SEQUENCE@},
      gfsim::UInt<16>{@BLOCK@},
      gfsim::UInt<3>{@INPUT_COUNT@},
      gfsim::UInt<616>{gfsim::UInt<616>::word_array_type{@INPUT_TILES@}},
      gfsim::UInt<3>{@SCALAR_COUNT@},
      gfsim::UInt<276>{gfsim::UInt<276>::word_array_type{@SCALARS@}},
      gfsim::UInt<2>{@OUTPUT_COUNT@},
      gfsim::UInt<308>{gfsim::UInt<308>::word_array_type{@OUTPUT_TILES@}}};
  ac_generated::PtoPayloadAbi model;
  if (!model.incoming().proposePush(input)) return 1;
  model.incoming().doXfer({0, 0});
  auto rows = model.dispatch_rows();
  for (unsigned tick = 1; tick < 6; ++tick) {
    const gfsim::Epoch epoch{tick, 0};
    for (auto &row : rows) row.work(row.object, epoch);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
    for (auto &row : rows) row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  }
  if (model.sink_0_values().size() != 1) return 2;
  const auto &output = model.sink_0_values().front();
  const auto value = gfsim::bitConcat(
      output.opcode_id,
      gfsim::UInt<2>{static_cast<std::uint64_t>(output.engine_kind)},
      output.sequence_id, output.block_id, output.input_tile_count,
      output.input_tiles, output.scalar_input_count, output.scalar_inputs,
      output.output_tile_count, output.output_tiles);
  for (std::size_t index = 0; index < value.word_count; ++index) {
    if (index) std::cout << ' ';
    std::cout << value.word(index);
  }
  std::cout << '\\n';
}
'''
            replacements = {
                "@MODEL@": model.name,
                "@OPCODE@": str(field_value("opcode_id")),
                "@ENGINE@": str(field_value("engine_kind")),
                "@SEQUENCE@": str(field_value("sequence_id")),
                "@BLOCK@": str(field_value("block_id")),
                "@INPUT_COUNT@": str(field_value("input_tile_count")),
                "@INPUT_TILES@": initializer(input_tiles, 616),
                "@SCALAR_COUNT@": str(field_value("scalar_input_count")),
                "@SCALARS@": initializer(scalar_inputs, 276),
                "@OUTPUT_COUNT@": str(field_value("output_tile_count")),
                "@OUTPUT_TILES@": initializer(output_tiles, 308),
            }
            for key, value in replacements.items():
                harness_source = harness_source.replace(key, value)
            harness.write_text(
                harness_source,
                encoding="utf-8",
            )
            compiled = subprocess.run(
                (
                    cxx,
                    "-std=c++20",
                    "-I",
                    str(ROOT / "simulator/gfsim/include"),
                    str(harness),
                    "-o",
                    str(executable),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, compiled.returncode, compiled.stderr)
            executed = subprocess.run(
                (str(executable),), text=True, capture_output=True, check=False
            )
            self.assertEqual(0, executed.returncode, executed.stderr)
            self.assertEqual(
                " ".join(str(word) for word in expected_words) + "\n",
                executed.stdout,
            )
            self.assertEqual(20, len(input_words))

    def test_wide_payload_matches_published_layout_in_cpp_and_verilog(self) -> None:
        toolchain = Path(os.environ.get("PYC_TOOLCHAIN_ROOT", DEFAULT_TOOLCHAIN))
        pycc = toolchain / "bin/pycc"
        metadata = toolchain / "share/pycircuit/toolchain-metadata.json"
        acir_opt = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt-internal"
        pycgen = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-pycgen"
        cxx = shutil.which("c++")
        verilator = shutil.which("verilator")
        if not all(
            (
                pycc.is_file(),
                metadata.is_file(),
                acir_opt.is_file(),
                pycgen.is_file(),
                cxx is not None,
                verilator is not None,
            )
        ):
            self.skipTest("pinned ACIR/PYC toolchain, C++, and Verilator are required")

        source = EXAMPLE.read_text(encoding="utf-8")
        _, input_words, expected_words = _fixture()
        input_initializer = ", ".join(f"0x{word:016x}ULL" for word in input_words)
        expected_line = " ".join(str(word) for word in expected_words) + "\n"

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            raw = root / "pto-payload.raw.ac.mlir"
            frozen = root / "pto-payload.frozen.ac.mlir"
            output = root / "output"
            raw.write_text(
                lower_queue_source(source, "pto_payload_abi"), encoding="utf-8"
            )
            raw_text = raw.read_text(encoding="utf-8")
            self.assertIn(
                "!ac.value_array<4 x !ac.struct<@types::@PTOTileOperand>>", raw_text
            )
            self.assertIn("!ac.value_array<5 x i16>", raw_text)

            optimized = subprocess.run(
                (
                    str(acir_opt),
                    f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                    str(raw),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, optimized.returncode, optimized.stderr)
            frozen.write_text(optimized.stdout, encoding="utf-8")
            completed = subprocess.run(
                (
                    str(ROOT / "compiler/acir/tools/ac-queue-pyc-build.py"),
                    str(frozen),
                    "--pycgen-tool",
                    str(pycgen),
                    "--pycc",
                    str(pycc),
                    "--toolchain-lock",
                    str(ROOT / "toolchains/agentic-circuit/pyc.lock.json"),
                    "--toolchain-metadata",
                    str(metadata),
                    "--cxx",
                    cxx,
                    "--verilator",
                    verilator,
                    "--pyc-output",
                    str(output / "model.pyc"),
                    "--cpp-output-dir",
                    str(output / "cpp"),
                    "--verilog-output-dir",
                    str(output / "verilog"),
                    "--manifest",
                    str(output / "manifest.json"),
                ),
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)
            pyc = (output / "model.pyc").read_text(encoding="utf-8")
            verilog = (output / "verilog/pto_payload_abi.v").read_text(encoding="utf-8")
            self.assertIn("%in_data: i1258", pyc)
            self.assertIn("{lsb = 1208} : i1258 -> i16", pyc)
            self.assertNotIn("vector<", pyc)
            self.assertIn("input [1257:0] in_data", verilog)
            self.assertIn("output [1257:0] out_data", verilog)

            cpp_harness = root / "cpp_harness.cpp"
            cpp_executable = root / "cpp_model"
            cpp_harness.write_text(
                f"""#include "pto_payload_abi.hpp"
#include <cstdint>
#include <iostream>

int main() {{
  pyc::gen::pto_payload_abi dut;
  for (std::uint64_t cycle = 0; cycle < 8; ++cycle) {{
    dut.rst = pyc::cpp::Wire<1>(cycle == 0 ? 1 : 0);
    dut.in_valid = pyc::cpp::Wire<1>(cycle == 1 ? 1 : 0);
    dut.in_data = cycle == 1
                      ? pyc::cpp::Wire<1258>{{{{{input_initializer}}}}}
                      : pyc::cpp::Wire<1258>{{}};
    dut.out_ready = pyc::cpp::Wire<1>(1);
    dut.clk = pyc::cpp::Wire<1>(0);
    dut.step();
    dut.clk = pyc::cpp::Wire<1>(1);
    dut.step();
    if (dut.out_valid.value()) {{
      for (std::size_t index = 0; index < 20; ++index) {{
        if (index) std::cout << ' ';
        std::cout << dut.out_data.word(index);
      }}
      std::cout << '\\n';
    }}
    dut.clk = pyc::cpp::Wire<1>(0);
    dut.step();
  }}
}}
""",
                encoding="utf-8",
            )
            cpp_sources = sorted(output.joinpath("cpp").glob("pto_payload_abi*.cpp"))
            self.assertGreater(len(cpp_sources), 0)
            cpp_build = subprocess.run(
                (
                    cxx,
                    "-std=c++17",
                    "-I",
                    str(output / "cpp"),
                    "-I",
                    str(toolchain / "include"),
                    *(str(path) for path in cpp_sources),
                    str(cpp_harness),
                    str(toolchain / "lib/libpyc6_runtime.a"),
                    "-o",
                    str(cpp_executable),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, cpp_build.returncode, cpp_build.stderr)
            cpp_run = subprocess.run(
                (str(cpp_executable),), text=True, capture_output=True, check=False
            )
            self.assertEqual(0, cpp_run.returncode, cpp_run.stderr)
            self.assertEqual(expected_line, cpp_run.stdout)

            verilator_harness = root / "verilator_harness.cpp"
            verilator_harness.write_text(
                f"""#include "Vpto_payload_abi.h"
#include <array>
#include <cstdint>
#include <iostream>

int main() {{
  Vpto_payload_abi dut;
  const std::array<std::uint64_t, 20> input{{{{{input_initializer}}}}};
  for (std::uint64_t cycle = 0; cycle < 8; ++cycle) {{
    dut.rst = cycle == 0 ? 1 : 0;
    dut.in_valid = cycle == 1 ? 1 : 0;
    for (std::size_t index = 0; index < 40; ++index) {{
      const std::uint64_t word = input[index / 2];
      dut.in_data[index] = static_cast<std::uint32_t>(word >> (32 * (index % 2)));
    }}
    dut.out_ready = 1;
    dut.clk = 0;
    dut.eval();
    dut.clk = 1;
    dut.eval();
    if (dut.out_valid) {{
      for (std::size_t index = 0; index < 20; ++index) {{
        const std::uint64_t word =
            static_cast<std::uint64_t>(dut.out_data[index * 2]) |
            (static_cast<std::uint64_t>(dut.out_data[index * 2 + 1]) << 32);
        if (index) std::cout << ' ';
        std::cout << word;
      }}
      std::cout << '\\n';
    }}
    dut.clk = 0;
    dut.eval();
  }}
}}
""",
                encoding="utf-8",
            )
            object_dir = root / "verilator_obj"
            verilator_build = subprocess.run(
                (
                    verilator,
                    "--cc",
                    "--exe",
                    "--build",
                    "-Wno-fatal",
                    "--top-module",
                    "pto_payload_abi",
                    "--Mdir",
                    str(object_dir),
                    str(output / "verilog/pyc_primitives.v"),
                    str(output / "verilog/pto_payload_abi.v"),
                    str(verilator_harness),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verilator_build.returncode, verilator_build.stderr)
            verilator_run = subprocess.run(
                (str(object_dir / "Vpto_payload_abi"),),
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, verilator_run.returncode, verilator_run.stderr)
            self.assertEqual(expected_line, verilator_run.stdout)
            self.assertEqual(cpp_run.stdout, verilator_run.stdout)


if __name__ == "__main__":
    unittest.main()
