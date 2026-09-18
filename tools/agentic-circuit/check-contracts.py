#!/usr/bin/env python3
import importlib.util
import json
import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[2]
GOVERNANCE_FILES = (
    "LICENSE",
    "CONTRIBUTING.md",
    "CODE_OF_CONDUCT.md",
    "SECURITY.md",
    "CHANGELOG.md",
)
LINK_EXCLUDED_PATHS = {"docs/rfcs/pyc6-decisions.md"}
LINK_EXCLUDED_PREFIXES = ("docs/gates/logs/",)
EXPECTED_LLVM = {
    "release": "22.1.8",
    "upstream_commit": "ca7933e47d3a3451d81e72ac174dcb5aa28b59d1",
    "source_url": (
        "https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.8/"
        "llvm-project-22.1.8.src.tar.xz"
    ),
    "local_prefix": "/opt/homebrew/opt/llvm",
    "supported_host_triples": [
        "arm64-apple-darwin",
        "x86_64-linux-gnu",
        "x86_64-pc-windows-msvc",
    ],
    "package_version_policy": "exact",
}

AVAILABLE_STDLIB_COMPONENTS = {
    "ac.Queue",
    "ac.Scheduler",
    "ac.Compute",
    "ac.Link",
    "ac.Memory",
    "ac.Sink",
    "ac.ready_valid",
    "ac.request_response",
}
AVAILABLE_STDLIB_BINDINGS = {
    "ac.Queue": ("gfsim/queue.h", "gfsim::Queue"),
    "ac.Scheduler": ("gfsim/components.h", "gfsim::Scheduler"),
    "ac.Compute": ("gfsim/components.h", "gfsim::Compute"),
    "ac.Link": ("gfsim/components.h", "gfsim::Link"),
    "ac.Memory": ("gfsim/components.h", "gfsim::Memory"),
    "ac.Sink": ("gfsim/components.h", "gfsim::Sink"),
    "ac.ready_valid": ("gfsim/components.h", "gfsim::ReadyValid"),
    "ac.request_response": (
        "gfsim/components.h",
        "gfsim::RequestResponse",
    ),
}
UNAVAILABLE_STDLIB_COMPONENTS = {
    "ac.Arbiter",
    "ac.Dispatcher",
    "ac.Scoreboard",
    "ac.DependencyTracker",
    "ac.Bus",
    "ac.Crossbar",
    "ac.Router",
    "ac.Switch",
    "ac.Dma",
    "ac.Packetizer",
    "ac.Reassembler",
    "ac.LoadStore",
    "ac.RegisterFile",
    "ac.Scratchpad",
    "ac.Cache",
    "ac.Tlb",
    "ac.MemoryController",
    "ac.Fork",
    "ac.Join",
    "ac.Broadcast",
    "ac.Barrier",
    "ac.ProtocolAdapter",
    "ac.WidthAdapter",
    "ac.TimeDomainBridge",
    "ac.AddressTranslator",
    "ac.MemoryManagement",
    "ac.TrafficSource",
}


def check_governance(errors):
    for name in GOVERNANCE_FILES:
        path = ROOT / name
        if not path.is_file() or not path.read_text().strip():
            errors.append(f"missing or empty governance file: {name}")


def check_schemas(errors):
    if importlib.util.find_spec("jsonschema") is None:
        errors.append("jsonschema is unavailable; install requirements-dev.lock")
        return
    from jsonschema.exceptions import SchemaError
    from jsonschema.validators import Draft202012Validator

    for path in sorted((ROOT / "schemas/agentic-circuit").glob("*.schema.json")):
        document = json.loads(path.read_text())
        if document.get("$schema") != "https://json-schema.org/draft/2020-12/schema":
            errors.append(f"{path.relative_to(ROOT)} is not draft 2020-12")
            continue
        try:
            Draft202012Validator.check_schema(document)
        except SchemaError as exc:
            errors.append(f"{path.relative_to(ROOT)} does not compile: {exc.message}")


def check_stdlib_catalog(errors):
    if importlib.util.find_spec("jsonschema") is None:
        return
    from jsonschema.validators import Draft202012Validator

    catalog_root = ROOT / "schemas/agentic-circuit" / "stdlib"
    catalog_path = catalog_root / "catalog.json"
    if not catalog_path.is_file():
        errors.append(
            "missing standard-library catalog: "
            "schemas/agentic-circuit/stdlib/catalog.json"
        )
        return

    catalog = json.loads(catalog_path.read_text())
    expected_names = AVAILABLE_STDLIB_COMPONENTS | UNAVAILABLE_STDLIB_COMPONENTS
    expected_availability = {
        **dict.fromkeys(AVAILABLE_STDLIB_COMPONENTS, "available"),
        **dict.fromkeys(UNAVAILABLE_STDLIB_COMPONENTS, "declared_unavailable"),
    }
    if set(catalog) != {"catalog", "version", "entries"}:
        errors.append("standard-library catalog has unknown or missing fields")
        return
    if (
        catalog["catalog"] != "ac"
        or catalog["version"] != "0.1"
        or not isinstance(catalog["entries"], list)
    ):
        errors.append("standard-library catalog identity is invalid")
        return

    entries = catalog["entries"]
    names = [entry.get("canonical_name") for entry in entries]
    if names != sorted(expected_names):
        errors.append("standard-library catalog names are incomplete or non-canonical")
        return

    component_schema = json.loads(
        (ROOT / "schemas/agentic-circuit/component.schema.json").read_text()
    )
    validator = Draft202012Validator(component_schema)
    seen_paths = set()
    for entry in entries:
        if set(entry) != {"canonical_name", "availability", "schema_path"}:
            errors.append(f"catalog entry has invalid fields: {entry!r}")
            continue
        name = entry["canonical_name"]
        if entry["availability"] != expected_availability[name]:
            errors.append(f"catalog availability mismatch for {name}")
        schema_path = ROOT / entry["schema_path"]
        try:
            schema_path.relative_to(catalog_root)
        except ValueError:
            errors.append(
                f"catalog schema escapes schemas/agentic-circuit/stdlib: {name}"
            )
            continue
        if not schema_path.is_file():
            errors.append(f"missing catalog component schema: {entry['schema_path']}")
            continue
        seen_paths.add(schema_path.resolve())
        record = json.loads(schema_path.read_text())
        record_errors = sorted(
            validator.iter_errors(record), key=lambda error: list(error.path)
        )
        if record_errors:
            errors.append(
                f"invalid component schema {name}: {record_errors[0].message}"
            )
            continue
        if record["canonical_name"] != name:
            errors.append(f"component schema canonical name mismatch for {name}")
        if record["cpp_binding"] is None:
            errors.append(f"component schema omits frozen C++ binding for {name}")
        elif entry["availability"] == "available":
            binding = record["cpp_binding"]
            expected_header, expected_symbol = AVAILABLE_STDLIB_BINDINGS[name]
            if (
                binding["header"] != expected_header
                or binding["symbol"] != expected_symbol
                or binding["concept"] != "gfsim::Component"
            ):
                errors.append(f"available component binding mismatch for {name}")
            header = ROOT / "simulator/gfsim/include" / binding["header"]
            if not header.is_file():
                errors.append(f"available component header is missing for {name}")
    extra_paths = {
        path.resolve()
        for path in catalog_root.glob("*.json")
        if path.name != "catalog.json"
    } - seen_paths
    if extra_paths:
        errors.append("unlisted standard-library component schema files are present")

    generation = subprocess.run(
        [
            sys.executable,
            ROOT / "tools/agentic-circuit/generate-stdlib-catalog.py",
            "--check",
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    if generation.returncode:
        errors.append(generation.stderr.strip() or "standard-library catalog is stale")


def check_diagnostic_catalog(errors):
    generation = subprocess.run(
        [
            sys.executable,
            ROOT / "tools/agentic-circuit/generate-diagnostic-catalog.py",
            "--check",
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    if generation.returncode:
        errors.append(generation.stderr.strip() or "diagnostic catalog is stale")


def tracked_markdown_files():
    result = subprocess.run(
        ["git", "ls-files", "-z", "--", "*.md"],
        cwd=ROOT,
        check=True,
        capture_output=True,
    )
    paths = []
    for raw_path in result.stdout.split(b"\0"):
        if not raw_path:
            continue
        relative = raw_path.decode()
        if relative in LINK_EXCLUDED_PATHS or relative.startswith(
            LINK_EXCLUDED_PREFIXES
        ):
            continue
        path = ROOT / relative
        if path.is_file():
            paths.append(path)
    return paths


def markdown_destination(raw_target):
    target = raw_target.strip()
    if target.startswith("<") and ">" in target:
        return target[1 : target.index(">")]
    return target.split(maxsplit=1)[0]


def markdown_prose_lines(path):
    prose = []
    fence_character = None
    fence_length = 0
    for line in path.read_text().splitlines():
        if fence_character is not None:
            closing_fence = (
                rf"^ {{0,3}}{re.escape(fence_character)}{{{fence_length},}}\s*$"
            )
            if re.match(closing_fence, line):
                fence_character = None
                fence_length = 0
            prose.append("")
            continue

        opening_fence = re.match(r"^ {0,3}(`{3,}|~{3,})", line)
        if opening_fence:
            fence = opening_fence.group(1)
            fence_character = fence[0]
            fence_length = len(fence)
            prose.append("")
        elif line.startswith("    ") or line.startswith("\t"):
            prose.append("")
        else:
            prose.append(line)
    return prose


def github_heading_anchors(path):
    anchors = set()
    counts = {}
    lines = markdown_prose_lines(path)
    headings = []
    for index, line in enumerate(lines):
        atx = re.match(r"^ {0,3}#{1,6}\s+(.+?)\s*#*\s*$", line)
        if atx:
            headings.append(atx.group(1))
        elif index and re.match(r"^ {0,3}(?:=+|-+)\s*$", line):
            headings.append(lines[index - 1].strip())

        for explicit in re.findall(
            r"<(?:a\s+[^>]*name|[a-z][a-z0-9-]*\s+[^>]*id)=[\"']([^\"']+)[\"']",
            line,
            re.IGNORECASE,
        ):
            anchors.add(explicit)

    for heading in headings:
        rendered = re.sub(r"!\[([^\]]*)\]\([^)]*\)", r"\1", heading)
        rendered = re.sub(r"\[([^\]]+)\]\([^)]*\)", r"\1", rendered)
        rendered = re.sub(r"<[^>]+>", "", rendered)
        rendered = re.sub(r"[`*_~]", "", rendered).lower()
        slug = re.sub(r"[^\w\- ]", "", rendered, flags=re.UNICODE).replace(" ", "-")
        count = counts.get(slug, 0)
        counts[slug] = count + 1
        anchors.add(slug if count == 0 else f"{slug}-{count}")
    return anchors


def check_links(errors):
    pattern = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
    anchor_cache = {}
    for path in tracked_markdown_files():
        prose = "\n".join(markdown_prose_lines(path))
        for raw_target in pattern.findall(prose):
            target = markdown_destination(raw_target)
            if (
                not target
                or target.startswith("//")
                or re.match(r"^[a-z][a-z0-9+.-]*:", target, re.IGNORECASE)
            ):
                continue
            destination, separator, fragment = target.partition("#")
            target_path = (
                path if not destination else path.parent / unquote(destination)
            )
            target_path = target_path.resolve()
            if not target_path.exists():
                errors.append(f"broken link: {path.relative_to(ROOT)} -> {target}")
                continue
            if separator and target_path.suffix.lower() == ".md":
                anchors = anchor_cache.setdefault(
                    target_path, github_heading_anchors(target_path)
                )
                decoded_fragment = unquote(fragment)
                if decoded_fragment not in anchors:
                    errors.append(
                        f"broken fragment: {path.relative_to(ROOT)} -> {target}"
                    )


def check_placeholders(errors):
    marker = re.compile(r"\b(?:TODO|TBD|FIXME|XXX)\b")
    for name in ("README.md", *GOVERNANCE_FILES):
        path = ROOT / name
        if path.is_file() and marker.search(path.read_text()):
            errors.append(f"placeholder marker in {name}")
    readme = (ROOT / "README.md").read_text()
    if (
        "proposed Python" in readme
        or "No implementation contract is approved yet" in readme
    ):
        errors.append("README.md still describes a specification-only placeholder")


def check_llvm_lock(errors):
    lock = json.loads((ROOT / "toolchains/agentic-circuit/llvm.lock.json").read_text())
    if lock.get("lock_version") != 1:
        errors.append("LLVM lock_version must equal 1")
    if lock.get("llvm") != EXPECTED_LLVM:
        errors.append("LLVM lock does not match the approved 22.1.8 toolchain")


def check_release_layout(errors):
    for script in ("check-release-layout.py", "check-ndf.py"):
        completed = subprocess.run(
            [sys.executable, ROOT / "tools/agentic-circuit" / script],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        if completed.returncode:
            errors.append(completed.stderr.strip() or f"{script} failed")


def check_pyc_inventory(errors):
    completed = subprocess.run(
        [sys.executable, ROOT / "tools/pycircuit/check-pyc-inventory.py"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode:
        errors.append(completed.stderr.strip() or "PYC IR inventory check failed")


def check_zero_content_identity_contract(errors):
    """Reject byte-derived identity from active compiler product surfaces."""

    roots = (
        ROOT / "compiler",
        ROOT / "python",
        ROOT / "schemas",
        ROOT / "packaging",
        ROOT / "flows",
        ROOT / "tests",
        ROOT / "docs",
    )
    historical_prefixes = (
        "docs/gates/logs/",
        "docs/rfcs/pyc6-decisions.md",
        "docs/acir/spec/refs/history.md",
    )
    explanatory_files = {
        "docs/development/implementation-guide.md",
        "docs/development/sdk-release-contract.md",
        "docs/reference/name-mangling.md",
        "tools/agentic-circuit/check-contracts.py",
    }
    source_suffixes = {
        ".c",
        ".cc",
        ".cmake",
        ".cpp",
        ".h",
        ".hpp",
        ".inc",
        ".json",
        ".md",
        ".mlir",
        ".py",
        ".ps1",
        ".sh",
        ".td",
        ".toml",
        ".yaml",
        ".yml",
    }
    forbidden_terms = (
        "finger" + "print",
        "sha" + "256",
        "topology_" + "di" + "gest",
        "check" + "sum",
        "di" + "gest",
    )
    forbidden = re.compile(
        r"(?i)(?:" + "|".join(re.escape(term) for term in forbidden_terms) + r")"
    )
    violations = []
    for root in roots:
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if any(
                component in {"build", ".pycircuit_out", "__pycache__", ".venv"}
                for component in path.relative_to(ROOT).parts
            ):
                continue
            if not path.is_file() or path.suffix.lower() not in source_suffixes:
                continue
            relative = path.relative_to(ROOT).as_posix()
            if relative in explanatory_files or relative.startswith(
                historical_prefixes
            ):
                continue
            try:
                lines = path.read_text(encoding="utf-8").splitlines()
            except UnicodeError:
                continue
            for line_number, line in enumerate(lines, start=1):
                if forbidden.search(line):
                    violations.append(f"{relative}:{line_number}")
                    if len(violations) == 25:
                        break
            if len(violations) == 25:
                break
        if len(violations) == 25:
            break
    if violations:
        errors.append(
            "zero-content-identity contract violation: "
            + ", ".join(violations)
            + (" (first 25)" if len(violations) == 25 else "")
        )


def main():
    errors = []
    check_governance(errors)
    check_schemas(errors)
    check_stdlib_catalog(errors)
    check_diagnostic_catalog(errors)
    check_links(errors)
    check_placeholders(errors)
    check_llvm_lock(errors)
    check_release_layout(errors)
    check_pyc_inventory(errors)
    check_zero_content_identity_contract(errors)
    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1
    print(
        "repository contracts: OK (public schemas, 35 stdlib components, LLVM 22.1.8)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
