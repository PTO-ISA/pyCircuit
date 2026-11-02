"""Bind Python definitions to an externally checked NDF 0.2 specification.

NDF owns document parsing, identities and graph validation. This adapter only
locates standard source comments and projects their bindings into NDF Markdown;
it does not infer that an implementation satisfies a requirement.
"""

from __future__ import annotations

import ast
import hashlib
import io
import json
import shutil
import subprocess
import tokenize
from pathlib import Path


def _bindings(path: Path, root: Path) -> list[dict]:
    text = path.read_text()
    lines = text.splitlines()
    definitions = {}

    def visit(node, parents=()):
        for child in ast.iter_child_nodes(node):
            names = parents
            if isinstance(child, ast.FunctionDef | ast.AsyncFunctionDef | ast.ClassDef):
                names += (child.name,)
                start = min([child.lineno] + [d.lineno for d in child.decorator_list])
                definitions[start] = ".".join(names)
            visit(child, names)

    visit(ast.parse(text, filename=str(path)))
    result = []
    for token in tokenize.generate_tokens(io.StringIO(text).readline):
        if token.type != tokenize.COMMENT:
            continue
        comment = token.string[1:].lstrip()
        if not comment.startswith("ndf:"):
            continue
        cursor = token.start[0]
        while cursor < len(lines) and (
            not lines[cursor].strip() or lines[cursor].lstrip().startswith("#")
        ):
            cursor += 1
        symbol = definitions.get(cursor + 1)
        if symbol is None:
            raise ValueError(f"{path}:{token.start[0]}: NDF binding needs a definition")
        for item in comment[4:].split():
            role, separator, targets = item.partition("=")
            if role not in ("implements", "verifies") or not separator or not targets:
                raise ValueError(f"{path}:{token.start[0]}: invalid source NDF binding")
            result.append(
                {
                    "path": path.relative_to(root).as_posix(),
                    "symbol": symbol,
                    "line": token.start[0],
                    "role": role,
                    "targets": targets.split(","),
                    "sha256": hashlib.sha256(text.encode()).hexdigest(),
                }
            )
    return result


def _ndf(executable: str, arguments: list[str], output: Path) -> dict:
    run = subprocess.run(
        [executable, *arguments], capture_output=True, text=True, timeout=180
    )
    output.write_text(run.stdout)
    output.with_suffix(".stderr").write_text(run.stderr)
    output.with_suffix(".rc").write_text(str(run.returncode) + "\n")
    data = json.loads(run.stdout)
    if run.returncode or data.get("schema_version") != "0.1" or not data.get("ok"):
        raise ValueError(f"NDF {arguments[0]} failed; see {output}")
    return data


def check_spec(source: str, executable: str, output: str) -> dict:
    root = Path(source).resolve(strict=True)
    destination = Path(output).resolve()
    # Never replace a prior receipt, source directory, or generated index.
    destination.mkdir(parents=True, exist_ok=False)
    receipt = destination / "spec-check.json"
    report = {
        "schema": "pycircuit-spec-check-v1",
        "status": "INCOMPLETE",
        "approval": "NOT_RECORDED",
        "semantic_validation": "NOT_RUN",
    }
    receipt.write_text(json.dumps(report, indent=2) + "\n")
    version = subprocess.run(
        [executable, "--machine-version"], capture_output=True, text=True, timeout=180
    )
    (destination / "ndf-version.json").write_text(version.stdout)
    (destination / "ndf-version.rc").write_text(str(version.returncode) + "\n")
    identity = json.loads(version.stdout)
    if version.returncode or identity.get("format_version") != "0.2":
        raise ValueError("spec-check requires the pinned NDF 0.2 CLI")
    report["ndf_version"] = identity
    _ndf(executable, ["check", "--root", str(root)], destination / "check.json")
    index = destination / "spec.sqlite"
    _ndf(
        executable,
        ["build", "--root", str(root), "--output", str(index)],
        destination / "build.json",
    )
    exported = destination / "spec.ndf.json"
    _ndf(
        executable,
        [
            "export",
            "--index",
            str(index),
            "--output",
            str(exported),
            "--format",
            "ndf-json",
        ],
        destination / "export.json",
    )
    graph = json.loads(exported.read_text())
    nodes = {node["id"]: node for node in graph["nodes"]}
    requirements = [n for n in nodes.values() if n["kind"] == "requirement"]
    if not requirements or not any(n["refinement"] == "L1" for n in requirements):
        raise ValueError(
            "NDF graph has no L1 requirements; check heading/metadata adjacency"
        )
    projects = {identity.removeprefix("ndf://").split("/")[0] for identity in nodes}
    if len(projects) != 1:
        raise ValueError("source binding requires one owning NDF project")
    project = projects.pop()
    bindings = []
    for path in sorted(root.rglob("*.py")):
        if any(
            p.startswith(".") or p == "__pycache__"
            for p in path.relative_to(root).parts
        ):
            continue
        bindings.extend(_bindings(path, root))
    if not bindings:
        raise ValueError("no implements/verifies source bindings")
    for binding in bindings:
        binding["targets"] = [
            target if target.startswith("ndf://") else f"ndf://{project}/{target}"
            for target in binding["targets"]
        ]
        for target in binding["targets"]:
            if target not in nodes:
                raise ValueError(
                    f"{binding['path']}:{binding['line']}: unknown clause {target}"
                )
            expected = (
                "requirement" if binding["role"] == "implements" else "verification"
            )
            if nodes[target]["kind"] != expected:
                raise ValueError(
                    f"{target}: {binding['role']} requires a {expected} clause"
                )
    l2 = {n["id"] for n in requirements if n["refinement"] == "L2"}
    implemented = {
        t for b in bindings if b["role"] == "implements" for t in b["targets"]
    }
    verified = {t for b in bindings if b["role"] == "verifies" for t in b["targets"]}
    verification = {n["id"] for n in nodes.values() if n["kind"] == "verification"}
    if not l2 or l2 - implemented or not verification or verification - verified:
        raise ValueError(
            f"unbound Spec/verification clauses: {sorted((l2 - implemented) | (verification - verified))}"
        )
    covered = {
        edge["target"]
        for edge in graph["edges"]
        if edge["type"] == "verifies" and edge["source"] in verified
    }
    while True:
        parents = {
            edge["target"]
            for edge in graph["edges"]
            if edge["type"] == "refines" and edge["source"] in covered
        }
        if parents <= covered:
            break
        covered.update(parents)
    required = {n["id"] for n in requirements if n["refinement"] in ("L1", "L2")}
    if required - covered:
        raise ValueError(
            f"requirements without bound verification: {sorted(required - covered)}"
        )

    # The pinned NDF CLI has no Python adapter. Feed it standard Markdown source
    # and test nodes rather than forking its parser or creating another Spec.
    stage = destination / "bound-project"
    documents = stage / "documents"
    documents.mkdir(parents=True)
    for path in sorted({n["source"]["path"] for n in nodes.values()}):
        original = (root / path).resolve(strict=True)
        if not original.is_relative_to(root) or original.suffix != ".md":
            raise ValueError(
                "binding projection currently requires local Markdown Spec sources"
            )
        shutil.copyfile(
            original, documents / (hashlib.sha256(path.encode()).hexdigest() + ".md")
        )
    (stage / "ndf.yaml").write_text(
        f'format_version: "0.2"\nproject: {project}\nroots:\n  - documents/*.md\n'
        "id_prefixes: [SRC, TEST]\ndomains: []\npolicies: {}\n"
    )
    (stage / "ndf.lock").write_text('format_version: "0.1"\ndependencies: {}\n')
    text = (
        "---\ndoc_id: SRC-DOC-BINDINGS\nstatus: draft\nauthority: generated\n"
        f"owner: {project}\n---\n\n# Source and verification bindings\n\n"
    )
    for binding in bindings:
        kind = "source" if binding["role"] == "implements" else "test"
        key = f"{binding['path']}\0{binding['symbol']}\0{binding['role']}"
        identity = (
            ("SRC" if kind == "source" else "TEST")
            + "-"
            + hashlib.sha256(key.encode()).hexdigest()[:16].upper()
        )
        targets = ",".join(binding["targets"])
        text += (
            f"## {binding['path']}:{binding['symbol']} {{#{identity}}}\n"
            f"<!-- ndf: kind={kind} status=draft {binding['role']}={targets} -->\n\n"
            f"Source: `{binding['path']}:{binding['line']}`, symbol `{binding['symbol']}`.\n"
            f"Content SHA256: `{binding['sha256']}`. This records a binding, not a proof.\n\n"
        )
    (documents / "bindings.md").write_text(text)
    _ndf(executable, ["check", "--root", str(stage)], destination / "bound-check.json")
    _ndf(
        executable,
        ["build", "--root", str(stage), "--output", str(destination / "bound.sqlite")],
        destination / "bound-build.json",
    )
    report.update(
        status="PASS",
        scope="NDF syntax, clause identity and Python definition binding; not behavioral conformance",
        spec_statuses=sorted({n["status"] for n in nodes.values()}),
        clauses=len(nodes),
        bindings=bindings,
    )
    receipt.write_text(json.dumps(report, indent=2) + "\n")
    return report
