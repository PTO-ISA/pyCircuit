from __future__ import annotations

import importlib.util
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]


@pytest.fixture
def fetch_module():
    spec = importlib.util.spec_from_file_location(
        "benchmark_fetch", ROOT / "benchmark/fetch.py"
    )
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.mark.parametrize("relative", ("../escape.jsonl", "/tmp/escape.jsonl", ""))
def test_payload_rejects_paths_outside_checkout(
    fetch_module, tmp_path: Path, relative: str
) -> None:
    fetch_module.ROOT = tmp_path
    (tmp_path / "upstream/source").mkdir(parents=True)
    source = {
        "path": "upstream/source",
        "revision": "0" * 40,
        "dataset": "owner/data",
    }

    with pytest.raises(ValueError, match="inside"):
        fetch_module.payload(source, relative, "https://example.invalid")


def test_payload_rejects_symlink_escape(fetch_module, tmp_path: Path) -> None:
    fetch_module.ROOT = tmp_path
    checkout = tmp_path / "upstream/source"
    checkout.mkdir(parents=True)
    outside = tmp_path / "outside.jsonl"
    outside.write_text("outside\n")
    (checkout / "question.jsonl").symlink_to(outside)
    source = {
        "path": "upstream/source",
        "revision": "0" * 40,
        "dataset": "owner/data",
    }

    with pytest.raises(ValueError, match="symlink"):
        fetch_module.payload(source, "question.jsonl", "https://example.invalid")


def test_fetch_source_deduplicates_question_and_context_paths(
    fetch_module, tmp_path: Path, monkeypatch
) -> None:
    fetch_module.ROOT = tmp_path
    checkout = tmp_path / "upstream/source"
    checkout.mkdir(parents=True)
    source = {
        "name": "fixture",
        "path": "upstream/source",
        "revision": "1" * 40,
        "dataset": "owner/data",
        "question_files": ["questions.jsonl", "questions.jsonl"],
        "context_files": ["questions.jsonl", "context.bundle"],
    }
    calls = []
    monkeypatch.setattr(fetch_module, "git", lambda *args, cwd=None: source["revision"])

    def payload(row, relative, endpoint):
        calls.append(relative)
        return {"path": relative, "sha256": "0" * 64, "bytes": 1}

    monkeypatch.setattr(fetch_module, "payload", payload)

    report = fetch_module.fetch_source(source, "https://example.invalid", True)

    assert calls == ["questions.jsonl", "context.bundle"]
    assert [item["path"] for item in report["artifacts"]] == calls
