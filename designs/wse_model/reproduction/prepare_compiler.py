"""Reconstruct the historical compiler source tree in a fresh local worktree."""

import argparse
import gzip
import json
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    history = json.loads((root / "compiler-history.json").read_text())
    output = args.output.resolve()
    if output.exists():
        parser.error("--output must be a new directory")
    subprocess.run(
        [
            "git",
            "-C",
            str(args.repository.resolve()),
            "worktree",
            "add",
            "--detach",
            str(output),
            history["base_revision"],
        ],
        check=True,
    )
    patch = gzip.decompress((root / history["patch_archive"]).read_bytes())
    subprocess.run(
        [
            "git",
            "-c",
            "user.name=WSE reproducer",
            "-c",
            "user.email=reproducer@example.invalid",
            "-C",
            str(output),
            "am",
        ],
        input=patch,
        check=True,
    )
    tree = subprocess.check_output(
        ["git", "-C", str(output), "rev-parse", "HEAD^{tree}"], text=True
    ).strip()
    if tree != history["historical_tree"]:
        raise RuntimeError("reconstructed source tree differs from historical compiler")
    print(f"Historical compiler tree verified: {tree}")
    print(f"Compiler checkout: {output}")


if __name__ == "__main__":
    main()
