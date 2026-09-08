"""Source names survive native lowering without becoming execution identities."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest

from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE, lower_queue_source

ROOT = Path(__file__).resolve().parents[4]
SOURCE = """
import agentic_circuit as ac
@ac.module
def leaf(a: ac.u8, b: ac.u8, *, bias: ac.const[int]) -> tuple[ac.u8, ac.u8]:
    total: ac.u8 = 0
    @ac.rule
    def advance(item):
        nonlocal total
        total = total + item
        return total
    x = advance(a)
    y = advance(b)
    return x, y
@ac.system
def nested(a: ac.u8, b: ac.u8, c: ac.u8, d: ac.u8) -> tuple[ac.u8, ac.u8, ac.u8, ac.u8]:
    x, y = leaf(a, b, bias=1)
    z, w = leaf(c, d, bias=2)
    return x, y, z, w
"""


class SourceNamesNativeTest(unittest.TestCase):
    def setUp(self):
        build = ROOT / ".pycircuit_out/local-clang22/build/bin"
        self.opt = Path(os.environ.get("ACIR_OPT", build / "acir-opt"))
        self.plan = Path(os.environ.get("ACIR_QUEUE_PLAN", build / "acir-queue-plan"))
        self.codegen = Path(
            os.environ.get("ACIR_QUEUE_CXXGEN", build / "acir-queue-cxxgen")
        )
        if not all(p.is_file() for p in (self.opt, self.plan, self.codegen)):
            self.skipTest("current-checkout native tools unavailable")

    def lower(self, raw):
        return subprocess.run(
            [str(self.opt), f"--pass-pipeline={RULE_LOWERING_PIPELINE}"],
            input=raw,
            text=True,
            capture_output=True,
        )

    def test_nested_instances_and_duplicate_calls(self):
        raw = lower_queue_source(SOURCE, "nested", host_results=True)
        lowered = self.lower(raw)
        self.assertEqual(0, lowered.returncode, lowered.stderr)
        with tempfile.TemporaryDirectory() as directory:
            frozen = Path(directory) / "model.mlir"
            frozen.write_text(lowered.stdout)
            result = subprocess.run(
                [str(self.plan), str(frozen)], text=True, capture_output=True
            )
            self.assertEqual(0, result.returncode, result.stderr)
            self.assertIn('"display_name":"advance"', result.stdout)
            generated = subprocess.run(
                [str(self.codegen), str(frozen)], text=True, capture_output=True
            )
            self.assertEqual(0, generated.returncode, generated.stderr)
            for text in (
                'setDisplayName("leaf[0]")',
                'setDisplayName("leaf[1]")',
                'setDisplayName("advance", "advance[0]")',
                'setDisplayName("advance", "advance[1]")',
            ):
                self.assertIn(text, generated.stdout)

    def test_pure_nested_module_instances(self):
        source = (
            ROOT
            / "examples/agentic-circuit/pipelines/inferred_nested_module_pipeline.py"
        ).read_text()
        lowered = self.lower(
            lower_queue_source(
                source, "inferred_nested_module_pipeline", host_results=True
            )
        )
        self.assertEqual(0, lowered.returncode, lowered.stderr)
        self.assertIn('ac.source_name = "increment"', lowered.stdout)
        with tempfile.TemporaryDirectory() as directory:
            frozen = Path(directory) / "model.mlir"
            frozen.write_text(lowered.stdout)
            generated = subprocess.run(
                [str(self.codegen), str(frozen)], text=True, capture_output=True
            )
            self.assertEqual(0, generated.returncode, generated.stderr)
            for text in (
                'setDisplayName("wrapper[0]")',
                'setDisplayName("wrapper[1]")',
                'setDisplayName("increment[0]")',
            ):
                self.assertIn(text, generated.stdout)

    def test_rule_lowered_to_transform_keeps_source_name(self):
        source = """
import agentic_circuit as ac
@ac.rule
def increment(item):
    return item + 1
@ac.module
def leaf(item: ac.u8) -> ac.u8:
    output = increment(item)
    return output
@ac.system
def simple(item: ac.u8) -> ac.u8:
    result = leaf(item)
    return result
"""
        lowered = self.lower(lower_queue_source(source, "simple", host_results=True))
        self.assertEqual(0, lowered.returncode, lowered.stderr)
        self.assertIn('ac.source_name = "increment"', lowered.stdout)
        with tempfile.TemporaryDirectory() as directory:
            frozen = Path(directory) / "model.mlir"
            frozen.write_text(lowered.stdout)
            generated = subprocess.run(
                [str(self.codegen), str(frozen)], text=True, capture_output=True
            )
            self.assertEqual(0, generated.returncode, generated.stderr)
            self.assertIn('block_.setDisplayName("increment")', generated.stdout)
            self.assertIn("scope_.setDisplayTransparent()", generated.stdout)

    def test_invalid_source_metadata_rejected(self):
        raw = lower_queue_source(SOURCE, "nested")
        for original in ('"advance"', '"leaf"'):
            for invalid in ("7 : i64", '""'):
                with self.subTest(original=original, invalid=invalid):
                    result = self.lower(
                        raw.replace(
                            "ac.source_name = " + original,
                            "ac.source_name = " + invalid,
                        )
                    )
                    self.assertNotEqual(0, result.returncode)
                    self.assertIn(
                        "ac.source_name must be a non-empty string", result.stderr
                    )

    def test_source_metadata_does_not_change_frozen_execution_ir(self):
        import re

        raw = lower_queue_source(SOURCE, "nested")
        strip = lambda s: re.sub(r',? ?ac.source_name = "[^"]*"', "", s)
        labeled = self.lower(raw)
        plain = self.lower(strip(raw))
        self.assertEqual(0, labeled.returncode, labeled.stderr)
        self.assertEqual(0, plain.returncode, plain.stderr)

        # Integrity hashes cover all frozen attributes, including source labels.
        def execution_ir(text):
            text = strip(text).replace("{} ", "").replace("{, ", "{")
            return re.sub(r'"(?:sha256:)?[0-9a-f]{64}"', '"<integrity-hash>"', text)

        self.assertEqual(execution_ir(labeled.stdout), execution_ir(plain.stdout))
