from __future__ import annotations

import re
from typing import Dict, List, Tuple


def normalize(text: str) -> str:
    text = text.lower()
    text = re.sub(r"[^a-z0-9]+", "_", text)
    return text.strip("_")


def keyword_score(module: str, file_path: str, keyword: str) -> int:
    """
    Heuristic discovery score:
      10: normalized keyword == normalized module
       6: keyword occurs in module name
       4: keyword occurs in filename stem/path
       0: no match

    v0.1 deliberately does not scan full RTL bodies to reduce false positives.
    """
    m = normalize(module)
    p = normalize(file_path)
    k = normalize(keyword)
    if not k:
        return 0
    if m == k:
        return 10
    if k in m:
        return 6
    if k in p:
        return 4
    return 0


def match_record(record: Dict, targets: List[Dict]) -> List[Dict]:
    hits = []
    for target in targets:
        matched_keywords = []
        total = 0
        best = 0
        for kw in target.get("keywords", []):
            s = keyword_score(record["module"], record["file"], kw)
            if s > 0:
                matched_keywords.append(kw)
                total += s
                best = max(best, s)

        if matched_keywords:
            hits.append({
                **target,
                **record,
                "matched_keywords": ";".join(matched_keywords),
                "match_score": total + best,
            })
    return hits
