"""Golden source maps for the constructs the AC bundle flow can reach today.

Issue #128 item V05 asks for source-map golden tests covering the helper,
module, projection, and specialization constructs. The native AC bundle flow
(`acc -c <unit>.ac -emit-cpp-bundle`) publishes `share/generated/source-map.json`
for a single non-hierarchical unit. A hierarchical unit needs a linked AC
package directory, which no frontend surface produces yet (issue #180), so this
module pins the two flat constructs — an inlined `@ac.inline` helper and a
nested-record projection — and the module and specialization constructs stay
open until the package flow exists.

Each golden is validated against the published
`schemas/agentic-circuit/source-map.schema.json` before it is compared, so a
golden cannot silently drift into an invalid document, and the helper golden
also pins the `inline_callsite` frames that prove inline expansion kept its
definition site.
"""

from __future__ import annotations

import json
import subprocess
import tempfile
import unittest
from pathlib import Path

from jsonschema import Draft202012Validator

from agentic_circuit._queue_frontend import lower_queue_source

REPOSITORY = Path(__file__).resolve().parents[4]
GOLDENS = REPOSITORY / "tests" / "goldens" / "agentic-circuit" / "source-map"
SCHEMA = REPOSITORY / "schemas" / "agentic-circuit" / "source-map.schema.json"
RULE_LOWERING_PIPELINE = (
    "builtin.module("
    "ac-lower-rules,"
    "ac-inline-pure-helpers,"
    "canonicalize,cse,"
    "ac-verify-rule-closure,"
    "ac-freeze-topology)"
)


def _repository_tool(name: str) -> Path | None:
    for candidate in (
        REPOSITORY / ".pycircuit_out/toolchain/build/bin" / name,
        REPOSITORY / ".pycircuit_out/acir/dev-llvm22/bin" / name,
    ):
        if candidate.is_file():
            return candidate
    return None


HELPER_SYSTEM = """
import agentic_circuit as ac

@ac.struct
class Item:
    value: ac.u8

@ac.inline
def pure_inc(value: ac.u8) -> ac.u8:
    return value + 1

@ac.rule
def keep(v):
    return v.with_fields(value=pure_inc(v.value))

@ac.system
def pipeline() -> None:
    incoming = ac.source(Item, depth=2, latency=1)
    outgoing = keep(incoming)
    ac.sink(outgoing)
"""

NESTED_PROJECTION = """
import agentic_circuit as ac

@ac.struct
class Inner:
    left: ac.u8
    right: ac.u8

@ac.struct
class Envelope:
    inner: Inner
    tag: ac.u4

@ac.system
def pipeline() -> None:
    incoming = ac.source(Envelope, depth=2, latency=1)
    outgoing = incoming.apply(lambda item: item.with_fields(
        inner=item.inner.with_fields(left=item.inner.left + 1),
    ))
    ac.sink(outgoing)
"""


class SourceMapGoldenTest(unittest.TestCase):
    """Item V05 for the flat constructs the bundle flow currently admits."""

    def _source_map(self, source: str) -> bytes:
        acir_opt = _repository_tool("acir-opt")
        acc = _repository_tool("acc")
        if acir_opt is None or acc is None:
            self.skipTest("the ACIR code generation tools are not built")

        with tempfile.TemporaryDirectory() as temporary:
            raw = Path(temporary) / "unit.raw.mlir"
            raw.write_text(
                lower_queue_source(
                    source, "pipeline", source_path="generated/module.py"
                ),
                encoding="utf-8",
            )
            frozen = Path(temporary) / "unit.ac"
            frozen_result = subprocess.run(
                [
                    str(acir_opt),
                    f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                    str(raw),
                    "-o",
                    str(frozen),
                ],
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, frozen_result.returncode, frozen_result.stderr)

            bundle = Path(temporary) / "bundle"
            emitted = subprocess.run(
                [str(acc), "-c", str(frozen), "-emit-cpp-bundle", "-o", str(bundle)],
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(0, emitted.returncode, emitted.stderr)

            published = bundle / "share" / "generated" / "source-map.json"
            self.assertTrue(published.is_file(), sorted(str(p) for p in bundle.rglob("*")))
            return published.read_bytes()

    def _assert_golden(self, name: str, source: str) -> dict[str, object]:
        document = self._source_map(source)

        schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
        validator = Draft202012Validator(schema)
        validator.validate(json.loads(document))

        expected = (GOLDENS / f"{name}.json").read_bytes()
        self.assertEqual(expected, document, f"{name} source-map golden drifted")
        return json.loads(document)

    def test_inline_helper_source_map_matches_golden(self) -> None:
        document = self._assert_golden("helper", HELPER_SYSTEM)

        frames = [
            frame
            for block in document["blocks"]
            for expression in block["expressions"]
            for origin in expression["source_provenance"]["origins"]
            for frame in origin["frames"]
        ]
        # Inline expansion records the helper definition and the call site.
        kinds = {frame["kind"] for frame in frames}
        self.assertIn("inline_callsite", kinds)
        self.assertIn("statement", kinds)

    def test_nested_projection_source_map_matches_golden(self) -> None:
        document = self._assert_golden("projection", NESTED_PROJECTION)

        files = {
            frame["file"]
            for block in document["blocks"]
            for origin in block["source_provenance"]["origins"]
            for frame in origin["frames"]
        }
        files |= {
            frame["file"]
            for block in document["blocks"]
            for expression in block["expressions"]
            for origin in expression["source_provenance"]["origins"]
            for frame in origin["frames"]
        }
        self.assertEqual({"generated/module.py"}, files)


if __name__ == "__main__":
    unittest.main()
