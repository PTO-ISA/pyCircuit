#!/usr/bin/env python3
"""Bounded discovery across public RTL indexes and forge APIs.

Discovery is intentionally separate from promotion.  The command records
candidate URLs, titles, licenses and source revisions when a website exposes
them, but never vendors or executes an unreviewed design.  A candidate still
has to pass dependency-closure, Verilator, functional-smoke and Yosys gates
before it can enter ``library/verilog``.
"""

from __future__ import annotations

import argparse
import html
import json
import re
import sys
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Mapping, Sequence


DEFAULT_CONFIG = Path("library/verilog/crawler/search-sources.json")
DEFAULT_OUTPUT = Path(".pycircuit_out/runtime-source-search/report.json")
USER_AGENT = "agentic-circuit-runtime-crawler/0.1"


class SearchError(RuntimeError):
    pass


def _fetch(url: str, *, timeout: int) -> tuple[str, str]:
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT, "Accept": "application/json,text/html;q=0.9"})
    with urllib.request.urlopen(request, timeout=max(1, timeout)) as response:
        body = response.read().decode("utf-8", errors="replace")
        content_type = response.headers.get("Content-Type", "")
    return body, content_type


def _json(url: str, *, timeout: int) -> Any:
    body, _ = _fetch(url, timeout=timeout)
    try:
        return json.loads(body)
    except json.JSONDecodeError as exc:
        raise SearchError(f"{url} did not return JSON: {exc}") from exc


def _candidate(source: str, provider: str, *, title: str, url: str, **metadata: Any) -> dict[str, Any]:
    result: dict[str, Any] = {"source": source, "provider": provider, "title": html.unescape(title).strip(), "url": url}
    result.update({key: value for key, value in metadata.items() if value not in (None, "", [])})
    return result


def _search_opencores(source: Mapping[str, Any], keywords: Sequence[str], *, timeout: int, limit: int) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    body, _ = _fetch(str(source["url"]), timeout=timeout)
    # The current OpenCores page embeds a JSON project index.  Keep the
    # expression narrow so arbitrary page text cannot become a candidate.
    pattern = re.compile(r'\{"slug":"([^"]+)","vcs":"([^"]+)","title":"([^"]+)","category":"([^"]+)","language":"([^"]+)".*?"license":"([^"]*)".*?"hasSvnFiles":(true|false)', re.S)
    terms = [term.lower() for term in keywords]
    results: list[dict[str, Any]] = []
    for match in pattern.finditer(body):
        slug, vcs, title, category, language, license_name, has_svn = match.groups()
        haystack = f"{title} {category} {slug}".lower()
        if language.lower() not in {"verilog", "systemverilog"} or not any(term in haystack for term in terms):
            continue
        project_url = f"https://opencores.org/websvn/listing?repname={urllib.parse.quote(slug)}&path=%2F{urllib.parse.quote(slug)}%2Ftrunk%2F"
        results.append(_candidate(str(source["name"]), "opencores", title=title, url=project_url, slug=slug, vcs=vcs, category=category, language=language, license=license_name, has_svn_files=has_svn == "true", review_status="index-only"))
        if len(results) >= limit:
            break
    return results, {"status": "ok", "projects_seen": len(pattern.findall(body)), "matched": len(results)}


def _search_github(source: Mapping[str, Any], keywords: Sequence[str], *, timeout: int, limit: int) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    results: list[dict[str, Any]] = []
    for keyword in keywords:
        query = f"{keyword} language:verilog"
        url = str(source["url"]) + "?" + urllib.parse.urlencode({"q": query, "per_page": limit})
        payload = _json(url, timeout=timeout)
        for item in payload.get("items", []) if isinstance(payload, Mapping) else []:
            if not isinstance(item, Mapping) or not item.get("html_url"):
                continue
            results.append(_candidate(str(source["name"]), "github", title=str(item.get("full_name", item.get("name", ""))), url=str(item["html_url"]), clone_url=item.get("clone_url"), default_branch=item.get("default_branch"), license=((item.get("license") or {}).get("spdx_id") if isinstance(item.get("license"), Mapping) else None), stars=item.get("stargazers_count"), matched_query=keyword, review_status="metadata-only"))
    return _dedupe(results), {"status": "ok", "queries": len(keywords), "matched": len(_dedupe(results))}


def _search_gitlab(source: Mapping[str, Any], keywords: Sequence[str], *, timeout: int, limit: int) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    results: list[dict[str, Any]] = []
    for keyword in keywords:
        url = str(source["url"]) + "?" + urllib.parse.urlencode({"search": keyword, "per_page": limit})
        payload = _json(url, timeout=timeout)
        for item in payload if isinstance(payload, list) else []:
            if not isinstance(item, Mapping) or not item.get("web_url"):
                continue
            results.append(_candidate(str(source["name"]), "gitlab", title=str(item.get("path_with_namespace", item.get("name", ""))), url=str(item["web_url"]), clone_url=item.get("http_url_to_repo"), default_branch=item.get("default_branch"), matched_query=keyword, review_status="metadata-only"))
    return _dedupe(results), {"status": "ok", "queries": len(keywords), "matched": len(_dedupe(results))}


def _search_codeberg(source: Mapping[str, Any], keywords: Sequence[str], *, timeout: int, limit: int) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    results: list[dict[str, Any]] = []
    for keyword in keywords:
        url = str(source["url"]) + "?" + urllib.parse.urlencode({"q": keyword, "limit": limit})
        payload = _json(url, timeout=timeout)
        items = payload.get("data", payload) if isinstance(payload, Mapping) else payload
        for item in items if isinstance(items, list) else []:
            if not isinstance(item, Mapping) or not item.get("html_url"):
                continue
            results.append(_candidate(str(source["name"]), "codeberg", title=str(item.get("full_name", item.get("name", ""))), url=str(item["html_url"]), clone_url=item.get("clone_url"), default_branch=item.get("default_branch"), matched_query=keyword, review_status="metadata-only"))
    return _dedupe(results), {"status": "ok", "queries": len(keywords), "matched": len(_dedupe(results))}


def _search_librecores(source: Mapping[str, Any], keywords: Sequence[str], *, timeout: int, limit: int) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    body, _ = _fetch(str(source["url"]), timeout=timeout)
    # LibreCores no longer hosts an authoritative project repository.  Record
    # the reachable index so the report is explicit rather than claiming a
    # zero-result source was never checked.
    return [], {"status": "index-only", "reachable": bool(body), "matched": 0, "note": "follow linked repositories and validate their exact commits"}


def _dedupe(items: Sequence[dict[str, Any]]) -> list[dict[str, Any]]:
    seen: set[str] = set()
    result: list[dict[str, Any]] = []
    for item in items:
        key = str(item.get("url", ""))
        if not key or key in seen:
            continue
        seen.add(key)
        result.append(item)
    return result


def search(config_path: Path, *, timeout: int, source_filter: Sequence[str] = ()) -> dict[str, Any]:
    config = json.loads(config_path.read_text(encoding="utf-8"))
    if not isinstance(config, Mapping) or not isinstance(config.get("sources"), list):
        raise ValueError("search config must contain a sources array")
    keywords = [str(item) for item in config.get("keywords", [])]
    limit = max(1, int(config.get("max_results_per_query", 8)))
    wanted = set(source_filter)
    reports: list[dict[str, Any]] = []
    candidates: list[dict[str, Any]] = []
    handlers = {"opencores": _search_opencores, "github": _search_github, "gitlab": _search_gitlab, "codeberg": _search_codeberg, "librecores": _search_librecores}
    for source in config["sources"]:
        if not isinstance(source, Mapping) or not source.get("enabled", True) or (wanted and str(source.get("name")) not in wanted):
            continue
        name = str(source.get("name", ""))
        provider = str(source.get("provider", ""))
        handler = handlers.get(provider)
        if handler is None:
            reports.append({"name": name, "provider": provider, "status": "unsupported"})
            continue
        try:
            found, report = handler(source, keywords, timeout=timeout, limit=limit)
            candidates.extend(found)
            reports.append({"name": name, "provider": provider, **report})
        except Exception as exc:  # Keep one unavailable website from hiding other results.
            reports.append({"name": name, "provider": provider, "status": "error", "error": str(exc)})
    candidates = sorted(_dedupe(candidates), key=lambda item: (str(item.get("provider", "")), str(item.get("title", "")).lower(), str(item.get("url", ""))))
    return {"schema": "acir-runtime-source-search-v0.1", "generated_at": datetime.now(timezone.utc).isoformat(), "config": str(config_path.resolve()), "keywords": keywords, "sources": reports, "summary": {"sources": len(reports), "candidates": len(candidates), "index_only": sum(report.get("status") == "index-only" for report in reports)}, "candidates": candidates}


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--timeout", type=int, default=20)
    parser.add_argument("--source", action="append", default=[], help="restrict to a named source; repeatable")
    args = parser.parse_args(argv)
    try:
        report = search(args.config.resolve(), timeout=max(1, args.timeout), source_filter=args.source)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"acir-runtime search: error: {exc}", file=sys.stderr)
        return 2
    print(json.dumps(report["summary"], ensure_ascii=False, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
