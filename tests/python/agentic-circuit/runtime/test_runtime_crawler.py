from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from acir_runtime_crawler import (
    crawl_runtime,
    dependency_closure,
    detect_provider,
    discover_rtl_files,
    index_rtl,
    parse_sources,
    parse_rtl_modules,
)


class RuntimeCrawlerTest(unittest.TestCase):
    def test_provider_detection_covers_supported_hosts(self) -> None:
        self.assertEqual(detect_provider("https://github.com/foo/bar.git"), "github")
        self.assertEqual(detect_provider("https://gitlab.com/foo/bar"), "gitlab")
        self.assertEqual(detect_provider("https://codeberg.org/foo/bar"), "codeberg")
        self.assertEqual(detect_provider("https://git.sr.ht/~foo/bar"), "sourcehut")
        self.assertEqual(detect_provider("ssh://git.example.org/foo/bar"), "git")
        self.assertEqual(detect_provider("../rtl"), "local")
        self.assertEqual(detect_provider(r"E:\rtl"), "local")

    def test_index_and_dependency_closure_are_deterministic(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "child.sv").write_text("module child(input logic a, output logic y); assign y=a; endmodule\n", encoding="utf-8")
            (root / "top.sv").write_text("module top(input wire a, output wire y); child u_child(.a(a), .y(y)); endmodule\n", encoding="utf-8")
            modules = index_rtl(root)
            self.assertEqual(list(modules), ["child", "top"])
            self.assertEqual([m.name for m in dependency_closure(modules, ["top"])], ["top", "child"])
            self.assertEqual(parse_rtl_modules(root / "top.sv")[0].ports[0].name, "a")

    def test_local_crawl_emits_wrappers_and_catalog(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rtl = root / "rtl"
            rtl.mkdir()
            (rtl / "impl.v").write_text("module impl #(parameter integer WIDTH=8)(input wire [WIDTH-1:0] a, output wire [WIDTH-1:0] y); assign y=a; endmodule\n", encoding="utf-8")
            config = root / "config.json"
            config.write_text(json.dumps({
                "sources": [{"name": "fixture", "provider": "local", "url": "rtl"}],
                "targets": [{"name": "identity", "source": "fixture", "module": "impl", "interface": {
                    "wrapper_module": "pyc_runtime_identity",
                    "parameters": [{"name": "WIDTH", "source": "WIDTH", "default": 8}],
                    "ports": [
                        {"name": "a", "direction": "input", "width": "WIDTH"},
                        {"name": "y", "direction": "output", "width": "WIDTH"}
                    ]
                }}]
            }), encoding="utf-8")
            output = root / "out"
            catalog = crawl_runtime(config, output, no_tools=True, max_candidates=1)
            self.assertEqual(catalog["accepted"], 1)
            self.assertEqual(catalog["entries"][0]["status"], "accepted")
            wrapper = output / "verilog" / "identity.v"
            self.assertIn("pyc_runtime_identity", wrapper.read_text(encoding="utf-8"))
            self.assertTrue((output / "catalog.json").exists())

    def test_checked_in_runtime_catalog_and_wrappers_are_present(self) -> None:
        root = Path(__file__).resolve().parents[4]
        catalog = json.loads((root / "library/verilog/catalog.json").read_text(encoding="utf-8"))
        self.assertEqual(catalog["schema"], "acir-runtime-catalog-v0.1")
        for entry in catalog["entries"]:
            self.assertEqual(entry["status"], "accepted")
            # Promoted entries may use SystemVerilog wrappers or a nested
            # vendor path; the catalog's wrapper field is the canonical path.
            wrapper = entry.get("wrapper") or f"verilog/{entry['module']}.v"
            self.assertTrue((root / "library/verilog" / wrapper).exists())

    def test_strict_policy_rejects_missing_validation_tool(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "rtl").mkdir()
            (root / "rtl" / "impl.v").write_text("module impl(input wire a, output wire y); assign y=a; endmodule\n", encoding="utf-8")
            config = root / "config.json"
            config.write_text(json.dumps({
                "policy": {"allow_skipped_tools": False},
                "sources": [{"name": "fixture", "url": "rtl"}],
                "targets": [{"name": "identity", "source": "fixture", "module": "impl", "interface": {
                    "wrapper_module": "pyc_runtime_identity",
                    "ports": [{"name": "a", "direction": "input"}, {"name": "y", "direction": "output"}]
                }}]
            }), encoding="utf-8")
            catalog = crawl_runtime(config, root / "out", no_tools=True, max_candidates=1)
            self.assertEqual(catalog["rejected"], 1)

    def test_source_license_is_recorded_in_provenance(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rtl = root / "rtl"
            rtl.mkdir()
            (rtl / "impl.v").write_text("module impl(input wire a, output wire y); assign y=a; endmodule\n", encoding="utf-8")
            config = root / "config.json"
            config.write_text(json.dumps({
                "sources": [{"name": "fixture", "provider": "local", "url": "rtl", "license": "Apache-2.0"}],
                "targets": [{"name": "identity", "source": "fixture", "module": "impl", "interface": {
                    "wrapper_module": "pyc_runtime_identity",
                    "ports": [{"name": "a", "direction": "input"}, {"name": "y", "direction": "output"}]
                }}]
            }), encoding="utf-8")
            catalog = crawl_runtime(config, root / "out", no_tools=True, max_candidates=1)
            self.assertEqual(catalog["entries"][0]["provenance"]["license"], "Apache-2.0")

    def test_v02_source_metadata_and_legacy_repo_key(self) -> None:
        sources = parse_sources({"sources": [{
            "name": "fixture", "repo": "https://github.com/example/rtl.git",
            "priority": "A", "families": ["Integer", "Dataflow"]
        }]}, config_dir=Path.cwd())
        self.assertEqual(sources[0].url, "https://github.com/example/rtl.git")
        self.assertEqual(sources[0].priority, "A")
        self.assertEqual(sources[0].families, ("Integer", "Dataflow"))

    def test_pinned_commit_is_retained_in_source_spec(self) -> None:
        sources = parse_sources({"sources": [{
            "name": "fixture", "url": "https://github.com/example/rtl.git",
            "commit": "0123456789abcdef0123456789abcdef01234567",
        }]}, config_dir=Path.cwd())
        self.assertEqual(sources[0].commit, "0123456789abcdef0123456789abcdef01234567")

    def test_package_mode_copies_dependency_closure(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rtl = root / "rtl"
            rtl.mkdir()
            (rtl / "impl.v").write_text("module impl(input wire a, output wire y); assign y=a; endmodule\n", encoding="utf-8")
            config = root / "config.json"
            config.write_text(json.dumps({
                "sources": [{"name": "fixture", "provider": "local", "url": "rtl"}],
                "targets": [{"name": "identity", "source": "fixture", "module": "impl", "interface": {
                    "wrapper_module": "pyc_runtime_identity",
                    "ports": [{"name": "a", "direction": "input"}, {"name": "y", "direction": "output"}]
                }}]
            }), encoding="utf-8")
            output = root / "out"
            catalog = crawl_runtime(config, output, no_tools=True, max_candidates=1, package=True)
            package = output / "package"
            self.assertTrue((package / "sources" / "fixture" / "impl.v").exists())
            self.assertEqual(catalog["entries"][0]["package"]["root"], "package")

    def test_file_overflow_can_be_deterministically_truncated(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for index in range(3):
                (root / f"{index}.v").write_text(f"module m{index}; endmodule\n", encoding="utf-8")
            with self.assertRaises(Exception):
                discover_rtl_files(root, max_files=2)
            bounded = discover_rtl_files(root, max_files=2, overflow="truncate")
            self.assertEqual([path.name for path in bounded], ["0.v", "1.v"])

    def test_resume_reuses_matching_checkpoint(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rtl = root / "rtl"
            rtl.mkdir()
            (rtl / "impl.v").write_text("module impl(input wire a, output wire y); assign y=a; endmodule\n", encoding="utf-8")
            config = root / "config.json"
            config.write_text(json.dumps({
                "sources": [{"name": "fixture", "provider": "local", "url": "rtl"}],
                "targets": [{"name": "identity", "source": "fixture", "module": "impl", "interface": {
                    "wrapper_module": "pyc_runtime_identity",
                    "ports": [{"name": "a", "direction": "input"}, {"name": "y", "direction": "output"}]
                }}]
            }), encoding="utf-8")
            output = root / "out"
            first = crawl_runtime(config, output, no_tools=True, max_candidates=1)
            second = crawl_runtime(config, output, no_tools=True, max_candidates=1, resume=True)
            self.assertEqual(first["accepted"], 1)
            self.assertTrue(second["resumed"])
            self.assertEqual(second["entries"][0]["candidate_id"], first["entries"][0]["candidate_id"])


if __name__ == "__main__":
    unittest.main()
