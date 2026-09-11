from __future__ import annotations

import copy
import importlib.util
import io
import json
import subprocess
import sys
import tarfile
import tempfile
import unittest
from pathlib import Path

import tomllib

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

    def test_release_workflow_is_manual_sha_pinned_and_accepts_before_tagging(self) -> None:
        workflow = (ROOT / ".github/workflows/release.yml").read_text()
        self.assertIn("workflow_dispatch:", workflow)
        self.assertNotRegex(workflow, r"(?m)^\s+push:\s*$")
        self.assertIn("commit_sha:", workflow)
        self.assertIn("ref: ${{ inputs.commit_sha }}", workflow)
        self.assertIn("needs: [accept-candidate]", workflow)
        self.assertIn("git tag -a", workflow)
        self.assertIn("verify-published", workflow)
        self.assertIn("publish-ghcr:", workflow)
        self.assertIn("verify-platform-candidates:", workflow)
        self.assertIn("verify-stable-platforms:", workflow)
        self.assertIn("needs: [verify-published-bytes]", workflow)
        self.assertIn("release-attestation:", workflow)
        self.assertIn("packaging/sdk/verify_platform_candidate.py", workflow)
        self.assertIn("--forbidden-path \"$GITHUB_WORKSPACE\"", workflow)
        self.assertIn("--wheel \"$(find universal", workflow)
        self.assertIn("--attestation evidence/ACCEPTANCE.json", workflow)
        self.assertIn("sudo apt-get install -y", workflow)
        self.assertIn("patchelf", workflow)
        self.assertIn("--candidate-tag \"v${{ inputs.version }}\"", workflow)
        self.assertIn("platform attestation is not bound to this release", workflow)
        self.assertIn("gh issue comment 61", workflow)
        self.assertNotIn("--attestation candidate/ACCEPTANCE.json", workflow)
        self.assertNotIn("gh release upload", workflow)
        self.assertNotIn("startsWith(github.ref, 'refs/tags/v')", workflow)
        self.assertLess(workflow.index("accept-candidate:"), workflow.index("create-tag:"))
        self.assertLess(workflow.index("create-tag:"), workflow.index("publish-release:"))

        validator_path = ROOT / ".github/scripts/validate_repo_management.py"
        spec = importlib.util.spec_from_file_location("release_validator", validator_path)
        if spec is None or spec.loader is None:
            self.fail("cannot load repository validator")
        validator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(validator)
        document = validator.yaml.safe_load(workflow)
        self.assertEqual([], validator.validate_release_graph(document))

        bypass = copy.deepcopy(document)
        bypass["jobs"]["publish-release"]["needs"] = ["full-validation"]
        self.assertTrue(
            any(
                "publish-release bypasses accept-candidate" in error
                for error in validator.validate_release_graph(bypass)
            )
        )

        rebuild = copy.deepcopy(document)
        rebuild["jobs"]["publish-ghcr"]["steps"].append(
            {"name": "forbidden rebuild", "run": "python3 -m build"}
        )
        self.assertTrue(
            any(
                "publish-ghcr rebuilds candidate bytes" in error
                for error in validator.validate_release_graph(rebuild)
            )
        )

    def test_release_candidate_aggregation_is_deterministic_and_exact(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            candidates = root / "candidates"
            candidates.mkdir()
            source = "a" * 40
            version = "6.0.0"
            artifacts = (
                f"pycircuit_hisi-{version}-py3-none-linux_x86_64.whl",
                f"pycircuit_hisi-{version}-py3-none-macosx_15_0_arm64.whl",
                f"pycircuit_semantic_core-{version}-py3-none-any.whl",
                "agentic_circuit-0.1.0-py3-none-any.whl",
                f"pycircuit-sdk-{version}-linux-x86_64.tar.gz",
                f"pycircuit-sdk-{version}-macos-arm64.tar.gz",
                f"pycircuit-sdk-{version}-linux-x86_64.manifest.json",
                f"pycircuit-sdk-{version}-macos-arm64.manifest.json",
                "LICENSES.tar.gz",
                "RELEASE_NOTES.md",
            )
            for index, name in enumerate(reversed(artifacts)):
                path = candidates / name
                if name.endswith(".manifest.json"):
                    platform = "linux-x86_64" if "linux" in name else "macos-arm64"
                    path.write_text(json.dumps({"source_revision": source, "platform": {"id": platform}}))
                else:
                    path.write_bytes(f"artifact-{index}".encode())

            outputs = []
            for run in ("one", "two"):
                out = root / run
                result = subprocess.run(
                    [
                        sys.executable,
                        ROOT / "packaging/sdk/release_candidate.py",
                        "aggregate",
                        "--candidate-dir",
                        candidates,
                        "--out-dir",
                        out,
                        "--source-revision",
                        source,
                    ],
                    cwd=ROOT,
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(0, result.returncode, result.stdout + result.stderr)
                outputs.append((out / f"pycircuit-sdk-{version}-release-index.json").read_bytes())
            self.assertEqual(outputs[0], outputs[1])
            index = json.loads(outputs[0])
            self.assertEqual(
                [
                    "agentic-circuit",
                    "pycircuit-hisi-linux-x86_64",
                    "pycircuit-hisi-macos-arm64",
                    "pycircuit-semantic-core",
                ],
                list(index["wheels"]),
            )
            lock = json.loads(
                (
                    root
                    / "one"
                    / f"pycircuit-sdk-{version}-linux-x86_64.lock.json"
                ).read_text()
            )
            self.assertTrue(
                all("size" not in wheel for wheel in lock["wheels"].values())
            )
            self.assertNotIn("size", lock["release_index"])
            for platform in ("linux-x86_64", "macos-arm64"):
                checked_lock = subprocess.run(
                    [
                        sys.executable,
                        ROOT / "tools/agentic-circuit/check-sdk-contract.py",
                        "--document",
                        root / "one" / f"pycircuit-sdk-{version}-{platform}.lock.json",
                    ],
                    cwd=ROOT,
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(
                    0, checked_lock.returncode, checked_lock.stdout + checked_lock.stderr
                )

            tampered = root / "one" / "agentic_circuit-0.1.0-py3-none-any.whl"
            tampered.write_bytes(b"tampered")
            rejected_bytes = subprocess.run(
                [
                    sys.executable,
                    ROOT / "packaging/sdk/release_candidate.py",
                    "accept",
                    "--candidate-dir",
                    root / "one",
                    "--source-revision",
                    source,
                    "--attestation",
                    root / "rejected-attestation.json",
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(0, rejected_bytes.returncode)
            self.assertIn("accepted asset bytes changed", rejected_bytes.stderr)

            (candidates / "agentic_circuit-0.1.0-py2-none-any.whl").write_bytes(b"duplicate")
            rejected = subprocess.run(
                [
                    sys.executable,
                    ROOT / "packaging/sdk/release_candidate.py",
                    "aggregate",
                    "--candidate-dir",
                    candidates,
                    "--out-dir",
                    root / "rejected",
                    "--source-revision",
                    source,
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(0, rejected.returncode)
            self.assertIn("unexpected candidate asset", rejected.stderr)

    def test_platform_generator_embeds_three_wheels_and_schema_valid_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            install = root / "install"
            required = (
                "bin/agentic-circuit",
                "bin/acir-opt",
                "bin/acir-queue-plan",
                "bin/acir-queue-cxxgen",
                "include/gfsim/model_api.h",
                "lib/libgfsim.a",
                "lib/cmake/AgenticCircuit/AgenticCircuitConfig.cmake",
                "share/pycircuit/schemas/model-plan.schema.json",
            )
            for name in required:
                path = install / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(name.encode())
            cached = install / "lib/python/site-packages/example/__pycache__/x.pyc"
            cached.parent.mkdir(parents=True)
            cached.write_bytes(str(ROOT).encode())
            wheels = (
                root / "pycircuit_hisi-6.0.0-py3-none-linux_x86_64.whl",
                root / "pycircuit_semantic_core-6.0.0-py3-none-any.whl",
                root / "agentic_circuit-0.1.0-py3-none-any.whl",
            )
            for wheel in wheels:
                wheel.write_bytes(wheel.name.encode())
            out = root / "out"
            command = [
                sys.executable,
                ROOT / "packaging/sdk/create_platform_manifest.py",
                "--install-dir",
                install,
                "--platform",
                "linux-x86_64",
                "--source-revision",
                "a" * 40,
                "--out-dir",
                out,
            ]
            for wheel in wheels:
                command.extend(("--wheel", wheel))
            generated = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
            self.assertEqual(0, generated.returncode, generated.stdout + generated.stderr)
            manifest = out / "pycircuit-sdk-6.0.0-linux-x86_64.manifest.json"
            manifest_value = json.loads(manifest.read_text())
            actual_compiler = subprocess.run(
                ["c++", "--version"], text=True, capture_output=True, check=True
            ).stdout.splitlines()[0]
            self.assertEqual(actual_compiler, manifest_value["platform"]["cxx_compiler"])
            checked = subprocess.run(
                [sys.executable, ROOT / "tools/agentic-circuit/check-sdk-contract.py", "--document", manifest],
                cwd=ROOT,
                text=True,
                capture_output=True,
            )
            self.assertEqual(0, checked.returncode, checked.stdout + checked.stderr)
            archive = out / "pycircuit-sdk-6.0.0-linux-x86_64.tar.gz"
            attestation = out / "platform-attestation.json"
            verified = subprocess.run(
                [
                    sys.executable,
                    ROOT / "packaging/sdk/verify_platform_candidate.py",
                    "--archive",
                    archive,
                    "--manifest",
                    manifest,
                    "--attestation",
                    attestation,
                    "--candidate-tag",
                    "v6.0.0",
                    "--release-url",
                    "https://github.com/PTO-ISA/pyCircuit/releases/tag/v6.0.0",
                    "--skip-install",
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
            )
            self.assertEqual(0, verified.returncode, verified.stdout + verified.stderr)
            attested = json.loads(attestation.read_text())
            self.assertEqual("v6.0.0", attested["candidate_tag"])
            self.assertFalse(attested["verified"])
            self.assertFalse(attested["relocated_model_consumer"])
            self.assertFalse(attested["incremental_determinism"])
            with tarfile.open(archive, "r:gz") as bundle:
                self.assertNotIn(
                    "lib/python/site-packages/example/__pycache__/x.pyc",
                    bundle.getnames(),
                )
                embedded = sorted(
                    name for name in bundle.getnames() if name.startswith("python/wheelhouse/")
                )
            self.assertEqual(
                sorted(f"python/wheelhouse/{wheel.name}" for wheel in wheels), embedded
            )

            poisoned = root / "poisoned.tar.gz"
            with (
                tarfile.open(archive, "r:gz") as source,
                tarfile.open(poisoned, "w:gz") as destination,
            ):
                for member in source.getmembers():
                    stream = source.extractfile(member) if member.isfile() else None
                    destination.addfile(member, stream)
                payload = b"unlisted"
                backdoor = tarfile.TarInfo("bin/unlisted-backdoor")
                backdoor.size = len(payload)
                destination.addfile(backdoor, io.BytesIO(payload))
            rejected_tree = subprocess.run(
                [
                    sys.executable,
                    ROOT / "packaging/sdk/verify_platform_candidate.py",
                    "--archive",
                    poisoned,
                    "--manifest",
                    manifest,
                    "--skip-install",
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
            )
            self.assertNotEqual(0, rejected_tree.returncode)
            self.assertIn("unlisted-backdoor", rejected_tree.stderr)


if __name__ == "__main__":
    unittest.main()
