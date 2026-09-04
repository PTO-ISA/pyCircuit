from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from agentic_circuit._canonical_json import canonical_json_bytes
from tools.pto_trace_adapter import (
    AdapterError,
    AdapterLimits,
    convert_davincioo_trace,
    parse_davincioo_jsonl,
)


ROOT = Path(__file__).resolve().parents[4]
FIXTURES = Path(__file__).with_name("fixtures")
PRODUCER = "davincioo@e73633301cabed0d871ea5ff66e76a91df870aeb"
PTO = "pto-isa@f6d0567c1cae2d6a7b0ebaf7ad0e3b93f8a39da3"


def row(**overrides: object) -> dict[str, object]:
    value: dict[str, object] = {
        "block_idx": 3,
        "sequence_id": 7,
        "opcode": "TADDS",
        "input_tiles": [
            {"address": "0x20", "shape": [2, 4], "layout": "ND", "dtype": "f32"}
        ],
        "scalar_inputs": [{"dtype": "f32", "value": "1.25"}],
        "output_tiles": [
            {"address": "0x40", "shape": [2, 4], "layout": "ND", "dtype": "f32"}
        ],
    }
    value.update(overrides)
    return value


def jsonl(*rows: dict[str, object]) -> bytes:
    return b"".join(
        json.dumps(item, separators=(",", ":")).encode("utf-8") + b"\n" for item in rows
    )


class DavinciOOAdapterTest(unittest.TestCase):
    def test_pinned_fixture_maps_losslessly_to_canonical_trace(self) -> None:
        source = (FIXTURES / "davincioo-valid.jsonl").read_bytes()
        encoded = convert_davincioo_trace(
            source, source_program="examples/beginner_matmul"
        )
        document = json.loads(encoded)

        self.assertEqual(encoded, canonical_json_bytes(document) + b"\n")
        self.assertEqual(
            {"schema", "version", "contract_epoch", "metadata", "records"},
            set(document),
        )
        self.assertEqual("pto-trace", document["schema"])
        self.assertEqual("0.1", document["version"])
        self.assertEqual("0.5", document["contract_epoch"])
        self.assertEqual(
            {
                "producer": PRODUCER,
                "pto_identity": PTO,
                "source_program": "examples/beginner_matmul",
                "data_layout": "davincioo-tile-address-v1",
                "record_count": 2,
                "content_hash": "sha256:"
                + hashlib.sha256(canonical_json_bytes(document["records"])).hexdigest(),
            },
            document["metadata"],
        )

        first = document["records"][0]
        self.assertEqual(0, first["sequence_id"])
        self.assertEqual("TASSIGN", first["opcode"])
        self.assertEqual([], first["dependencies"])
        self.assertEqual(
            [
                {"kind": "immediate", "type": "uint64", "value": "0"},
                {"kind": "tile", "id": "block/0/tile/0x0"},
            ],
            first["operands"],
        )
        retained = first["attributes"]["davincioo"]
        self.assertEqual(0, retained["block_idx"])
        self.assertEqual(["scalar_input", "output_tile"], retained["operand_roles"])
        self.assertEqual(
            row(
                block_idx=0,
                sequence_id=0,
                opcode="TASSIGN",
                input_tiles=[],
                scalar_inputs=[{"dtype": "uint64", "value": "0"}],
                output_tiles=[
                    {
                        "address": "0x0",
                        "shape": [64, 256],
                        "layout": "outer=col_major,inner=row_major,fractal_size=512",
                        "dtype": "float32",
                    }
                ],
            ),
            {
                "block_idx": retained["block_idx"],
                "sequence_id": first["sequence_id"],
                "opcode": first["opcode"],
                "input_tiles": retained["input_tiles"],
                "scalar_inputs": retained["scalar_inputs"],
                "output_tiles": retained["output_tiles"],
            },
        )

    def test_default_source_identity_is_raw_input_hash(self) -> None:
        source = jsonl(row())
        document = json.loads(convert_davincioo_trace(source))
        self.assertEqual(
            "sha256:" + hashlib.sha256(source).hexdigest(),
            document["metadata"]["source_program"],
        )

    def test_output_is_independent_of_repeated_conversion(self) -> None:
        source = jsonl(row())
        self.assertEqual(
            convert_davincioo_trace(source, source_program="kernel.pto"),
            convert_davincioo_trace(source, source_program="kernel.pto"),
        )

    def test_blank_lines_are_ignored_but_empty_trace_is_valid(self) -> None:
        self.assertEqual((), parse_davincioo_jsonl(b"\n  \n"))
        document = json.loads(convert_davincioo_trace(b"\n"))
        self.assertEqual([], document["records"])

    def test_duplicate_key_and_unknown_field_are_rejected(self) -> None:
        duplicate = (
            b'{"block_idx":0,"block_idx":1,"sequence_id":0,"opcode":"T",'
            b'"input_tiles":[],"scalar_inputs":[],"output_tiles":[]}\n'
        )
        with self.assertRaisesRegex(AdapterError, "ACTRACE-ADAPTER-JSON"):
            parse_davincioo_jsonl(duplicate)
        with self.assertRaisesRegex(AdapterError, "ACTRACE-ADAPTER-SCHEMA"):
            parse_davincioo_jsonl((FIXTURES / "davincioo-invalid.jsonl").read_bytes())

    def test_invalid_utf8_and_trailing_json_are_rejected(self) -> None:
        with self.assertRaisesRegex(AdapterError, "ACTRACE-ADAPTER-JSON"):
            parse_davincioo_jsonl(b"\xff\n")
        with self.assertRaisesRegex(AdapterError, "ACTRACE-ADAPTER-JSON"):
            parse_davincioo_jsonl(jsonl(row()).rstrip() + b" trailing\n")

    def test_sequence_ids_are_unique_strictly_increasing_safe_integers(self) -> None:
        for rows in (
            (row(sequence_id=7), row(sequence_id=7)),
            (row(sequence_id=8), row(sequence_id=7)),
            (row(sequence_id=(1 << 53)),),
            (row(sequence_id=-1),),
            (row(sequence_id=True),),
        ):
            with self.subTest(rows=rows):
                with self.assertRaisesRegex(AdapterError, "ACTRACE-ADAPTER-SEQUENCE"):
                    convert_davincioo_trace(jsonl(*rows))

    def test_tile_and_scalar_shapes_are_closed_and_canonical(self) -> None:
        invalid_tiles = (
            {"address": "0X20", "shape": [2], "layout": "ND", "dtype": "f32"},
            {"address": "0x020", "shape": [2], "layout": "ND", "dtype": "f32"},
            {"address": "0x20", "shape": [0], "layout": "ND", "dtype": "f32"},
            {"address": "0x20", "shape": [True], "layout": "ND", "dtype": "f32"},
            {"address": "0x20", "shape": [2], "layout": "", "dtype": "f32"},
            {"address": "0x20", "shape": [2], "layout": "ND", "dtype": "f32", "x": 1},
        )
        for tile in invalid_tiles:
            with self.subTest(tile=tile):
                with self.assertRaisesRegex(AdapterError, "ACTRACE-ADAPTER-SCHEMA"):
                    convert_davincioo_trace(jsonl(row(input_tiles=[tile])))
        for scalar in (
            {"dtype": "", "value": "1"},
            {"dtype": "u32", "value": 1},
            {"dtype": "u32", "value": "1", "x": 1},
        ):
            with self.subTest(scalar=scalar):
                with self.assertRaisesRegex(AdapterError, "ACTRACE-ADAPTER-SCHEMA"):
                    convert_davincioo_trace(jsonl(row(scalar_inputs=[scalar])))

    def test_limits_fail_before_partial_conversion(self) -> None:
        source = jsonl(row())
        cases = (
            AdapterLimits(max_document_bytes=len(source) - 1),
            AdapterLimits(max_line_bytes=8),
            AdapterLimits(max_record_count=0),
            AdapterLimits(max_tiles_per_record=0),
            AdapterLimits(max_scalars_per_record=0),
            AdapterLimits(max_shape_rank=1),
            AdapterLimits(max_string_bytes=2),
        )
        for limits in cases:
            with self.subTest(limits=limits):
                with self.assertRaisesRegex(AdapterError, "ACTRACE-ADAPTER-LIMIT"):
                    convert_davincioo_trace(source, limits=limits)

    def test_source_program_identity_is_nonempty_and_path_independent(self) -> None:
        source = jsonl(row())
        with self.assertRaisesRegex(AdapterError, "ACTRACE-ADAPTER-SCHEMA"):
            convert_davincioo_trace(source, source_program="")
        left = convert_davincioo_trace(source, source_program="suite/kernel")
        right = convert_davincioo_trace(source, source_program="suite/kernel")
        self.assertEqual(left, right)


class DavinciOOAdapterCommandTest(unittest.TestCase):
    command = ROOT / "compiler/acir/tools" / "import-davincioo-pto-trace.py"

    def run_command(
        self, *arguments: object, cwd: Path | None = None
    ) -> subprocess.CompletedProcess[bytes]:
        return subprocess.run(
            [
                sys.executable,
                os.fspath(self.command),
                *(os.fspath(arg) for arg in arguments),
            ],
            cwd=cwd or ROOT,
            env={
                **os.environ,
                "PYTHONPATH": os.fspath(ROOT / "python/agentic-circuit/src"),
            },
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )

    def test_help_and_exact_argument_surface(self) -> None:
        help_result = self.run_command("--help")
        self.assertEqual(0, help_result.returncode)
        self.assertIn(b"INPUT OUTPUT", help_result.stdout)
        self.assertEqual(b"", help_result.stderr)

        missing = self.run_command()
        self.assertEqual(2, missing.returncode)
        self.assertEqual(b"", missing.stdout)

    def test_command_atomically_publishes_exact_library_bytes(self) -> None:
        source = (FIXTURES / "davincioo-valid.jsonl").read_bytes()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            input_path = root / "input.pto.trace"
            output_path = root / "canonical.json"
            input_path.write_bytes(source)

            result = self.run_command(
                input_path,
                output_path,
                "--source-program",
                "examples/beginner_matmul",
                cwd=root,
            )

            self.assertEqual(0, result.returncode, result.stderr.decode())
            self.assertEqual(b"", result.stdout)
            self.assertEqual(b"", result.stderr)
            self.assertEqual(
                convert_davincioo_trace(
                    source, source_program="examples/beginner_matmul"
                ),
                output_path.read_bytes(),
            )
            self.assertEqual([], list(root.glob(".canonical.json.*.tmp")))

    def test_failure_preserves_existing_output_and_cleans_stage(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            input_path = root / "invalid.pto.trace"
            output_path = root / "canonical.json"
            input_path.write_bytes((FIXTURES / "davincioo-invalid.jsonl").read_bytes())
            output_path.write_bytes(b"prior\n")

            result = self.run_command(input_path, output_path, cwd=root)

            self.assertEqual(2, result.returncode)
            self.assertEqual(b"", result.stdout)
            self.assertIn(b"ACTRACE-ADAPTER-SCHEMA", result.stderr)
            self.assertEqual(b"prior\n", output_path.read_bytes())
            self.assertEqual([], list(root.glob(".canonical.json.*.tmp")))

    def test_command_rejects_aliasing_input_and_missing_parent(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            input_path = root / "input.pto.trace"
            input_path.write_bytes(jsonl(row()))
            alias = self.run_command(input_path, input_path, cwd=root)
            self.assertEqual(2, alias.returncode)
            self.assertIn(b"ACTRACE-ADAPTER-IO", alias.stderr)
            self.assertEqual(jsonl(row()), input_path.read_bytes())

            missing = self.run_command(
                input_path, root / "missing" / "out.json", cwd=root
            )
            self.assertEqual(2, missing.returncode)
            self.assertIn(b"ACTRACE-ADAPTER-IO", missing.stderr)

    def test_output_bytes_do_not_depend_on_root_or_umask(self) -> None:
        source = (FIXTURES / "davincioo-valid.jsonl").read_bytes()
        outputs: list[bytes] = []
        for mask in (0o022, 0o077):
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                input_path = root / "nested" / "input.pto.trace"
                output_path = root / "out.json"
                input_path.parent.mkdir()
                input_path.write_bytes(source)
                previous = os.umask(mask)
                try:
                    result = self.run_command(
                        input_path,
                        output_path,
                        "--source-program",
                        "fixture/stable",
                        cwd=root,
                    )
                finally:
                    os.umask(previous)
                self.assertEqual(0, result.returncode, result.stderr.decode())
                outputs.append(output_path.read_bytes())
        self.assertEqual(outputs[0], outputs[1])


if __name__ == "__main__":
    unittest.main()
