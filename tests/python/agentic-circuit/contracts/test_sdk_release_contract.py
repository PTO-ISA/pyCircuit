from __future__ import annotations

import json
import subprocess
import sys
import tomllib
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
VERSION_MAP = ROOT / "packaging/sdk/version-map.json"
CONTRACT = ROOT / "docs/development/sdk-release-contract.md"
SCHEMAS = {
    "pycircuit-sdk-version-map": "sdk-version-map.schema.json",
    "pycircuit-sdk-platform-manifest": "sdk-manifest.schema.json",
    "pycircuit-sdk-release-index": "release-index.schema.json",
    "agentic-circuit-model-plan": "model-plan.schema.json",
    "agentic-circuit-model-manifest": "model-manifest.schema.json",
    "pycircuit-sdk-lock": "consumer-lock.schema.json",
}


def project_version(path: Path) -> str:
    with path.open("rb") as stream:
        return str(tomllib.load(stream)["project"]["version"])


class SdkReleaseContractTest(unittest.TestCase):
    def test_version_map_matches_distribution_metadata(self) -> None:
        version_map = json.loads(VERSION_MAP.read_text())
        self.assertEqual("pycircuit-sdk-version-map", version_map["schema"])
        self.assertEqual("1", version_map["version"])
        self.assertEqual("v6.0.0", version_map["candidate_tag"])
        self.assertEqual("exact_identity_tuple", version_map["compatibility"])
        self.assertEqual(
            {
                "pycircuit-hisi": project_version(ROOT / "pyproject.toml"),
                "pycircuit-semantic-core": project_version(
                    ROOT / "python/semantic-core/pyproject.toml"
                ),
                "agentic-circuit": project_version(
                    ROOT / "python/agentic-circuit/pyproject.toml"
                ),
            },
            version_map["distributions"],
        )
        self.assertEqual(
            ["linux-x86_64", "macos-arm64"],
            [platform["id"] for platform in version_map["platforms"]],
        )
        self.assertTrue(
            all(platform["python"] == "3.11" for platform in version_map["platforms"])
        )
        self.assertTrue(
            all(platform["cxx_standard"] == "20" for platform in version_map["platforms"])
        )

    def test_public_v1_schemas_are_closed_and_epoch_bound(self) -> None:
        for identity, name in SCHEMAS.items():
            with self.subTest(schema=identity):
                schema = json.loads(
                    (ROOT / "schemas/agentic-circuit" / name).read_text()
                )
                self.assertEqual(
                    "https://json-schema.org/draft/2020-12/schema",
                    schema["$schema"],
                )
                self.assertFalse(schema["additionalProperties"])
                self.assertEqual(
                    "0.5", schema["properties"]["contract_epoch"]["const"]
                )
                self.assertEqual(identity, schema["properties"]["schema"]["const"])
                self.assertEqual("1", schema["properties"]["version"]["const"])

    def test_document_freezes_installed_commands_and_opaque_abi(self) -> None:
        contract = CONTRACT.read_text()
        self.assertIn("agentic-circuit model plan", contract)
        self.assertIn("agentic-circuit model emit-cpp", contract)
        self.assertIn("COMPONENTS Runtime", contract)
        self.assertIn("COMPONENTS CompilerDev", contract)
        self.assertIn("agentic_model_query_v1", contract)
        self.assertIn("create -> configure -> reset -> step", contract)
        self.assertIn("fixed 24-byte result", contract)
        for forbidden in (
            "load_trace_json",
            "agentic-model-trace",
            "trace_position",
            "SuperScalarModel",
        ):
            self.assertNotIn(forbidden, contract)

    def test_examples_and_adversarial_contracts_are_checked(self) -> None:
        result = subprocess.run(
            [sys.executable, ROOT / "tools/agentic-circuit/check-sdk-contract.py"],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertIn("6 schemas, 6 documents", result.stdout)


if __name__ == "__main__":
    unittest.main()
