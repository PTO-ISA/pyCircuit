from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]


class RepositoryContractTest(unittest.TestCase):
    def test_active_json_schemas_compile(self) -> None:
        self.assertIsNotNone(importlib.util.find_spec("jsonschema"))
        from jsonschema.validators import Draft202012Validator

        schemas = sorted((ROOT / "schemas/agentic-circuit").glob("*.schema.json"))
        self.assertTrue(schemas)
        for path in schemas:
            document = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(
                "https://json-schema.org/draft/2020-12/schema",
                document.get("$schema"),
                path.name,
            )
            Draft202012Validator.check_schema(document)

    def test_llvm_lock_is_exact_and_structural(self) -> None:
        document = json.loads(
            (ROOT / "toolchains/agentic-circuit/llvm.lock.json").read_text(
                encoding="utf-8"
            )
        )
        self.assertEqual(1, document["lock_version"])
        self.assertEqual(
            {
                "release": "22.1.8",
                "upstream_commit": "ca7933e47d3a3451d81e72ac174dcb5aa28b59d1",
                "source_url": (
                    "https://github.com/llvm/llvm-project/releases/download/"
                    "llvmorg-22.1.8/llvm-project-22.1.8.src.tar.xz"
                ),
                "local_prefix": "/opt/homebrew/opt/llvm",
                "supported_host_triples": [
                    "arm64-apple-darwin",
                    "x86_64-linux-gnu",
                    "x86_64-pc-windows-msvc",
                ],
                "package_version_policy": "exact",
            },
            document["llvm"],
        )

    def test_diagnostic_catalog_matches_sources(self) -> None:
        completed = subprocess.run(
            [
                sys.executable,
                "tools/agentic-circuit/generate-diagnostic-catalog.py",
                "--check",
            ],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, completed.returncode, completed.stderr)

    def test_repository_contract_checker_passes(self) -> None:
        completed = subprocess.run(
            [sys.executable, "tools/agentic-circuit/check-contracts.py"],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, completed.returncode, completed.stderr)


if __name__ == "__main__":
    unittest.main()
