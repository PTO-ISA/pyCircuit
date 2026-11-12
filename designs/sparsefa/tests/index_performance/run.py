# ruff: noqa: T201 -- CLI emits metrics and evidence paths.
#!/usr/bin/env python3
"""Run one immutable generated product against the independent prefix adapter."""

import argparse
import hashlib
import json
import os
import subprocess
from datetime import datetime, timezone
from pathlib import Path

from cases import prefixes

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--product",
        type=Path,
        required=True,
        help="directory containing product-cpp and product-build",
    )
    parser.add_argument("--toolchain", type=Path, required=True)
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    parser.add_argument("--workers", type=int, nargs="+", default=[1, 2])
    parser.add_argument("--small-only", action="store_true")
    args = parser.parse_args()
    product, toolchain = args.product.resolve(), args.toolchain.resolve()
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    evidence = ROOT / "docs/gates/logs/fa2-index-performance" / stamp
    output = ROOT / ".pycircuit_out" / ("fa2-index-performance-" + stamp)
    evidence.mkdir(parents=True)
    output.mkdir(parents=True)
    cpp = product / "product-cpp"
    library = product / "product-build/libpycircuit_modules.a"
    runtime = toolchain / "lib/libpyc6_runtime.a"
    inputs = [HERE / name for name in ("driver.cpp", "cases.py", "run.py")]
    inputs += [product / "product-final", library, runtime]
    inputs += [
        p
        for base in (cpp, toolchain / "include")
        for p in base.rglob("*")
        if p.is_file()
    ]

    def hashes():
        return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}

    before = hashes()
    (evidence / "before.json").write_text(json.dumps(before, indent=2) + "\n")
    commands, results = [], []

    def execute(name, command):
        command = [str(x) for x in command]
        log = evidence / (name + ".log")
        with log.open("w") as stream:
            code = subprocess.run(
                command, stdout=stream, stderr=subprocess.STDOUT
            ).returncode
        commands.append({"name": name, "command": command, "exit": code})
        (evidence / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        if code:
            raise RuntimeError(f"{name} exit {code}: {log}")
        return log.read_text()

    error = None
    try:
        exe = output / "driver"
        execute(
            "build",
            [
                args.cxx,
                "-std=c++20",
                "-O2",
                "-I" + str(cpp),
                "-I" + str(toolchain / "include"),
                HERE / "driver.cpp",
                library,
                runtime,
                "-pthread",
                "-o",
                exe,
            ],
        )
        executable_sha = hashlib.sha256(exe.read_bytes()).hexdigest()
        all_prefixes = {name: (keys, tokens) for name, keys, tokens in prefixes()}
        selected = [
            (name, 1, "valid", -1)
            for name in all_prefixes
            if (
                len(all_prefixes[name][1]) <= 128
                or (not args.small_only and name.startswith("descending-"))
            )
        ]
        selected += [
            ("descending-128", 4, mode, -1)
            for mode in (
                "valid",
                "duplicate",
                "duplicate-first",
                "range",
                "memory",
                "memory-range",
                "cancel",
            )
        ]
        selected += [("descending-128", 4, "cancel", offset) for offset in (5, 6, 7, 8)]
        for workers in args.workers:
            for name, delay, mode, offset in selected:
                keys, tokens = all_prefixes[name]
                prefix = output / (name + ".txt")
                prefix.write_text(str(keys) + "\n" + "\n".join(map(str, tokens)) + "\n")
                label = f"{name}-{delay}-{mode}-{offset}-w{workers}"
                text = execute(
                    label, [exe, len(tokens), delay, mode, workers, prefix, offset]
                )
                result = json.loads(text)
                results.append(
                    {
                        "name": name,
                        "offset": offset,
                        "prefix_sha256": hashlib.sha256(
                            prefix.read_bytes()
                        ).hexdigest(),
                        **result,
                    }
                )
                print(label, text.strip(), flush=True)
                (evidence / "results.json").write_text(
                    json.dumps(results, indent=2) + "\n"
                )
                assert hashlib.sha256(exe.read_bytes()).hexdigest() == executable_sha
    except Exception as exc:
        error = str(exc)
        raise
    finally:
        after = hashes()
        (evidence / "after.json").write_text(json.dumps(after, indent=2) + "\n")
        (evidence / "manifest.json").write_text(
            json.dumps(
                {
                    "status": "PASS" if error is None and before == after else "FAIL",
                    "error": error,
                    "inputs_unchanged": before == after,
                    "commands": commands,
                    "results": results,
                    "scope": "D1 zeroQK/constantV public-port scheduler latency; no physical-frequency claim",
                },
                indent=2,
            )
            + "\n"
        )
        print(evidence, flush=True)
        if before != after:
            raise RuntimeError("inputs changed during run")


if __name__ == "__main__":
    main()
