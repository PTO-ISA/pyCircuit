"""Restore pinned upstream snapshots and verified public dataset payloads.

No benchmark setup scripts, containers, models or proprietary tools are run.
Hugging Face mirror transport is opt-in; Git commit and LFS hashes stay fixed.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import urllib.parse
import urllib.request
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOCK = ROOT / "benchmark/sources.lock.json"


def git(*args, cwd=None):
    return subprocess.check_output(["git", *map(str, args)], cwd=cwd, text=True).strip()


def lfs_pointer(path):
    if path.stat().st_size > 1024:
        return None
    text = path.read_text(errors="replace")
    match = re.fullmatch(
        r"version https://git-lfs.github.com/spec/v1\noid sha256:([0-9a-f]{64})\nsize (\d+)\n?",
        text,
    )
    return (match[1], int(match[2])) if match else None


def payload(source, relative, endpoint):
    folder = (ROOT / source["path"]).resolve()
    relative_path = Path(relative)
    if (
        relative_path.is_absolute()
        or ".." in relative_path.parts
        or not relative_path.parts
    ):
        raise ValueError("Payload path must remain inside its pinned checkout")
    path = folder / relative_path
    if not path.resolve().is_relative_to(folder):
        raise ValueError("Payload path escapes its pinned checkout through a symlink")
    if not path.with_suffix(path.suffix + ".download").resolve().is_relative_to(folder):
        raise ValueError("Temporary payload path escapes its pinned checkout")
    # Read the committed pointer, not mutable remote metadata.
    committed = subprocess.check_output(
        ["git", "-C", str(folder), "show", source["revision"] + ":" + relative]
    )
    match = re.fullmatch(
        rb"version https://git-lfs.github.com/spec/v1\noid sha256:([0-9a-f]{64})\nsize (\d+)\n?",
        committed,
    )
    if not match:
        if path.read_bytes() != committed:
            raise ValueError(f"Modified pinned data: {path}")
        return {
            "path": relative,
            "sha256": hashlib.sha256(committed).hexdigest(),
            "bytes": len(committed),
            "transport": "git",
        }
    expected, size = match[1].decode(), int(match[2])
    if (
        path.stat().st_size == size
        and hashlib.sha256(path.read_bytes()).hexdigest() == expected
    ):
        return {
            "path": relative,
            "sha256": expected,
            "bytes": size,
            "transport": "existing_verified_payload",
        }
    url = (
        endpoint.rstrip("/")
        + "/datasets/"
        + source["dataset"]
        + "/resolve/"
        + source["revision"]
        + "/"
        + urllib.parse.quote(relative, safe="/")
    )
    temporary = path.with_suffix(path.suffix + ".download")
    request = urllib.request.Request(
        url, headers={"User-Agent": "pyCircuit-benchmark-fetch"}
    )
    digest = hashlib.sha256()
    received = 0
    try:
        with (
            urllib.request.urlopen(request, timeout=90) as response,
            temporary.open("wb") as stream,
        ):
            while block := response.read(1024 * 1024):
                received += len(block)
                if received > size:
                    raise ValueError(f"Payload larger than pinned LFS size: {relative}")
                digest.update(block)
                stream.write(block)
        if received != size or digest.hexdigest() != expected:
            raise ValueError(f"LFS digest or size mismatch: {relative}")
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)
    return {"path": relative, "sha256": expected, "bytes": size, "transport": url}


def fetch_source(source, hf_endpoint, include_context):
    folder = ROOT / source["path"]
    if not folder.exists():
        repository = (
            source.get("repository")
            or "https://huggingface.co/datasets/" + source["dataset"]
        )
        transport = (
            hf_endpoint.rstrip("/") + "/datasets/" + source["dataset"]
            if "dataset" in source
            else repository
        )
        folder.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            ["git", "clone", "--no-checkout", "--depth", "1", transport, str(folder)],
            env=dict(os.environ, GIT_LFS_SKIP_SMUDGE="1"),
            check=True,
            timeout=300,
        )
        if git("rev-parse", "HEAD", cwd=folder) != source["revision"]:
            subprocess.run(
                ["git", "fetch", "--depth", "1", "origin", source["revision"]],
                cwd=folder,
                check=True,
                timeout=300,
            )
        subprocess.run(
            ["git", "checkout", "--detach", source["revision"]],
            cwd=folder,
            env=dict(os.environ, GIT_LFS_SKIP_SMUDGE="1"),
            check=True,
        )
    if git("rev-parse", "HEAD", cwd=folder) != source["revision"]:
        raise ValueError(f"Existing checkout differs from pin: {folder}")
    selected = list(source.get("question_files", []))
    if include_context:
        selected.extend(source.get("context_files", []))
    selected = list(dict.fromkeys(selected))
    artifacts, failures = [], []
    for relative in selected:
        try:
            artifacts.append(payload(source, relative, hf_endpoint))
        except Exception as error:
            failures.append({"path": relative, "error": str(error)})
    return {
        "name": source["name"],
        "revision": source["revision"],
        "artifacts": artifacts,
        "failures": failures,
        "context_requested": include_context,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sources", nargs="*")
    parser.add_argument("--hf-endpoint", default="https://huggingface.co")
    parser.add_argument(
        "--include-context",
        action="store_true",
        help="also download pinned repository/context bundles",
    )
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    sources = json.loads(LOCK.read_text())["sources"]
    if args.sources:
        sources = [row for row in sources if row["name"] in args.sources]
        if {row["name"] for row in sources} != set(args.sources):
            parser.error("unknown source")
    reports = []
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for result in pool.map(
            lambda row: fetch_source(row, args.hf_endpoint, args.include_context),
            sources,
        ):
            reports.append(result)
            print(
                result["name"],
                len(result["artifacts"]),
                "verified payloads;",
                len(result["failures"]),
                "failed",
                flush=True,
            )
    output = ROOT / "benchmark/downloads"
    output.mkdir(exist_ok=True)
    (output / "download-report.json").write_text(json.dumps(reports, indent=2) + "\n")
    if any(row["failures"] for row in reports):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
