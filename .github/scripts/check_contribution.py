#!/usr/bin/env python3
"""Check issue routing and PR description completeness, never semantic acceptance."""

from __future__ import annotations

import json
import os
import re
import sys
from pathlib import Path

AREAS = {
    "Capture",
    "MLIR",
    "IR",
    "Link",
    "CPP",
    "Verilog",
    "Runtime",
    "System",
    "CLI",
    "Build",
    "Package",
    "CI",
    "Docs",
    "Other",
}
SECTIONS = (
    "Summary",
    "Change type",
    "Related issues",
    "Changes",
    "Validation",
    "Review checklist",
    "Reviewer notes",
)


def validate_issue_title(title: str) -> list[str]:
    match = re.fullmatch(r"\[([^\[\]\s]+)\] +([^\r\n]+)", title)
    if not match or match[1] not in AREAS or not match[2].strip():
        return ["Use [Area] short summary; see the issue form for areas (or Other)."]
    return []


def has_text(content: str) -> bool:
    for line in content.splitlines():
        line = line.strip()
        if not line or line.startswith(("#", "```", "~~~")):
            continue
        if re.match(r"[-*+]\s+\[[ xX]\]", line):
            continue
        if re.fullmatch(r"[-*+| :]+", line):
            continue
        return True
    return False


def mask_fenced_code(content: str) -> str:
    """Preserve offsets while hiding code from heading and checkbox matching."""
    lines = []
    fence = ""
    for line in content.splitlines(keepends=True):
        marker = re.match(r" {0,3}(`{3,}|~{3,})(.*)", line)
        if fence:
            lines.append(re.sub(r"[^\n]", " ", line))
            if (
                marker
                and marker[1][0] == fence[0]
                and len(marker[1]) >= len(fence)
                and not marker[2].strip()
            ):
                fence = ""
        elif marker and not (marker[1][0] == "`" and "`" in marker[2]):
            fence = marker[1]
            lines.append(re.sub(r"[^\n]", " ", line))
        else:
            lines.append(line)
    return "".join(lines)


def checked(content: str) -> bool:
    return (
        re.search(r"(?m)^\s*-\s+\[[xX]\]\s+\S", mask_fenced_code(content)) is not None
    )


def validate_pr_body(body: str) -> list[str]:
    body = body.replace("\r\n", "\n")
    comments = re.finditer(r"<!--.*?(?:-->|$)", mask_fenced_code(body), flags=re.S)
    for comment in reversed(list(comments)):
        body = body[: comment.start()] + body[comment.end() :]
    headings = list(
        re.finditer(r"(?m)^##[ \t]+([^\n]+?)[ \t]*$", mask_fenced_code(body))
    )
    sections: dict[str, str] = {}
    errors = []
    for index, heading in enumerate(headings):
        name = heading[1]
        if name in sections:
            errors.append(f"Duplicate section: {name}")
        end = headings[index + 1].start() if index + 1 < len(headings) else len(body)
        sections[name] = body[heading.end() : end]
    for name in SECTIONS:
        if name not in sections:
            errors.append(f"Missing section: {name}")
    for name in (
        "Summary",
        "Related issues",
        "Changes",
        "Validation",
        "Reviewer notes",
    ):
        if not has_text(sections.get(name, "")):
            errors.append(f"Fill {name} with actual details, or N/A and a reason.")
    for name in ("Change type", "Validation"):
        if not checked(sections.get(name, "")):
            errors.append(f"Select at least one applicable checkbox in {name}.")
    return errors


def main() -> int:
    event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text())
    if os.environ["GITHUB_EVENT_NAME"] == "issues":
        errors = validate_issue_title(event["issue"]["title"])
        with Path(os.environ["GITHUB_OUTPUT"]).open("a") as output:
            output.write(f"invalid={'true' if errors else 'false'}\n")
        for error in errors:
            sys.stdout.write(f"{error}\n")
        return 0  # Issue routing uses a label, not a failing code check.
    errors = validate_pr_body(event["pull_request"].get("body") or "")
    for error in errors:
        sys.stdout.write(f"{error}\n")
    if errors:
        sys.stdout.write(
            "Complete .github/PULL_REQUEST_TEMPLATE.md; metadata is not test evidence.\n"
        )
    else:
        sys.stdout.write(
            "PR description completeness passed; test results still require review.\n"
        )
    return int(bool(errors))


if __name__ == "__main__":
    raise SystemExit(main())
