from __future__ import annotations

import json
from pathlib import Path

from acir_runtime_search import _dedupe


def test_search_config_covers_open_source_indexes() -> None:
    root = Path(__file__).resolve().parents[4]
    config = json.loads((root / "library/verilog/crawler/search-sources.json").read_text(encoding="utf-8"))
    providers = {source["provider"] for source in config["sources"]}
    assert {"opencores", "github", "gitlab", "codeberg", "librecores"} <= providers


def test_search_deduplicates_repository_urls() -> None:
    items = [{"url": "https://example.invalid/a", "title": "a"}, {"url": "https://example.invalid/a", "title": "duplicate"}]
    assert _dedupe(items) == [items[0]]
