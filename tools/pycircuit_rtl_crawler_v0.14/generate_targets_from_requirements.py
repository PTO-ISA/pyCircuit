#!/usr/bin/env python3
"""Generate the crawler target catalog from the search-stage requirements HTML.

The requirements document is the human-maintained canonical list.  This small
stdlib-only importer keeps targets.json reproducible and prevents the crawler's
keyword catalog from drifting away from the HTML matrix.
"""

from __future__ import annotations

import argparse
import html
import json
import re
from html.parser import HTMLParser
from pathlib import Path


def clean(value: str) -> str:
    value = html.unescape(value)
    value = re.sub(r"\s+", " ", value)
    return value.strip()


def slug(value: str) -> str:
    value = clean(value).lower()
    value = re.sub(r"[^a-z0-9]+", "_", value)
    return value.strip("_") or "target"


class RequirementsParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.heading = ""
        self._in_h2 = False
        self._h2_parts: list[str] = []
        self._in_table = False
        self._in_row = False
        self._in_cell = False
        self._cell_parts: list[str] = []
        self._row: list[str] = []
        self._table_rows: list[list[str]] = []
        self._table_heading = ""
        self.rows: list[tuple[str, list[str], str]] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        tag = tag.lower()
        if tag == "h2":
            self._in_h2 = True
            self._h2_parts = []
        elif tag == "table":
            self._in_table = True
            self._table_rows = []
            self._table_heading = self.heading
        elif tag == "tr" and self._in_table:
            self._in_row = True
            self._row = []
        elif tag in {"th", "td"} and self._in_row:
            self._in_cell = True
            self._cell_parts = []

    def handle_endtag(self, tag: str) -> None:
        tag = tag.lower()
        if tag == "h2" and self._in_h2:
            heading = clean("".join(self._h2_parts))
            if heading:
                self.heading = re.sub(r"^\d+\.\s*", "", heading)
            self._in_h2 = False
        elif tag in {"th", "td"} and self._in_cell:
            self._row.append(clean("".join(self._cell_parts)))
            self._in_cell = False
        elif tag == "tr" and self._in_row:
            if self._row:
                self._table_rows.append(self._row)
            self._in_row = False
        elif tag == "table" and self._in_table:
            self._consume_table(self._table_heading, self._table_rows)
            self._in_table = False

    def handle_data(self, data: str) -> None:
        if self._in_h2:
            self._h2_parts.append(data)
        if self._in_cell:
            self._cell_parts.append(data)

    def _consume_table(self, heading: str, rows: list[list[str]]) -> None:
        if not rows:
            return
        header = [clean(item).lower() for item in rows[0]]
        if {"id", "design class", "variant / configuration", "search keywords / aliases", "level", "current", "priority", "domain"}.issubset(set(header)):
            pos = {name: header.index(name) for name in ("id", "design class", "variant / configuration", "search keywords / aliases", "level", "current", "priority", "domain")}
            for row in rows[1:]:
                if len(row) <= max(pos.values()):
                    continue
                target_id = clean(row[pos["id"]])
                if re.fullmatch(r"[A-Z]+-\d+", target_id):
                    self.rows.append((heading, row, "primitive"))
        elif {"id", "composite target", "典型构成", "search aliases", "domain"}.issubset(set(header)):
            # Composite rows intentionally have no primitive-level priority or
            # timing contract.  Keep them searchable, but mark them as
            # search-only and fill conservative defaults for the crawler CSV.
            pos = {name: header.index(name) for name in ("id", "composite target", "典型构成", "search aliases", "domain")}
            for row in rows[1:]:
                if len(row) <= max(pos.values()):
                    continue
                target_id = clean(row[pos["id"]])
                if re.fullmatch(r"[A-Z]+-\d+", target_id):
                    normalized = [
                        target_id,
                        clean(row[pos["composite target"]]),
                        clean(row[pos["典型构成"]]),
                        clean(row[pos["search aliases"]]),
                        "L3",
                        "M",
                        "P1",
                        clean(row[pos["domain"]]),
                    ]
                    self.rows.append((heading, normalized, "composite"))


def aliases(value: str) -> list[str]:
    result: list[str] = []
    for item in re.split(r"[,;]", clean(value)):
        item = item.strip()
        if item and item.lower() not in {x.lower() for x in result}:
            result.append(item)
    return result


def generate(source: Path) -> dict:
    parser = RequirementsParser()
    parser.feed(source.read_text(encoding="utf-8", errors="replace"))
    targets: list[dict] = []
    for family, row, target_kind in parser.rows:
        # The accepted table schema is fixed above, so these indexes are stable.
        target_id, design_class, variant, search_terms, level, current, priority, domain = row[:8]
        keywords = aliases(search_terms)
        if not keywords:
            keywords = [design_class]
        targets.append({
            "target_id": target_id,
            "gap_id": target_id,
            "family": family,
            "operation": slug(design_class),
            "variant": variant,
            "level": level,
            "current": current,
            "priority": priority,
            "domain": domain,
            "keywords": keywords,
            "search_aliases": search_terms,
            "target_kind": target_kind,
            "search_only": target_kind == "composite",
            "source_document": str(source.resolve()),
        })
    targets.sort(key=lambda item: item["target_id"])
    ids = [item["target_id"] for item in targets]
    if len(ids) != len(set(ids)):
        raise SystemExit("duplicate target IDs in requirements document")
    return {
        "schema": "pycircuit-rtl-targets-v0.3",
        "source_document": str(source.resolve()),
        "generated_from": "search-stage requirements HTML",
        "target_count": len(targets),
        "targets": targets,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    result = generate(args.source.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output.resolve()), "targets": result["target_count"]}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
