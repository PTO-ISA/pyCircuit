"""Bound generated C++ work at every level and preserve transaction semantics."""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

DRIVER = r"""
#include "pycircuit_system.hpp"
#include "gfsim/WorkExecutor.h"
#include <cstdlib>
#include <iostream>
#include <source_location>

void require(bool value, std::source_location at = std::source_location::current()) {
  if (!value) { std::cerr << "outline oracle line " << at.line() << '\n'; std::abort(); }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
template <typename T> void expect(const T &wire, unsigned value) {
  require(wire.packed().isFullyKnown());
  require(wire.packed().value().value() == value);
}
// Independent byte arithmetic; no generated intermediates supply the oracle.
unsigned mix(unsigned x) {
  unsigned value = x;
  for (unsigned i = 0; i < 24; ++i) value = ((value + x) & 255u) ^ 0x5au;
  return (value + (x ^ 0x33u)) & 255u;
}
struct TestSystem final : gfsim::SimSystem {
  explicit TestSystem(pyc_root &r) : root(r) {}
  bool Precheck() noexcept override {
    gfsim::SimFailureInfo failure;
    return root.pyc_validate_checks(failure);
  }
  pyc_root &root;
};
int main() {
  for (unsigned workers : {1u, 2u}) {
    gfsim::WorkExecutor pool(workers);
    pyc_root root("outline", &pool);
    auto drive = [&](unsigned x, unsigned clock, bool allow = true) {
      root.x = known<8>(x);
      root.allow = known<1>(allow);
      root.@CLOCK@ = known<1>(clock);
      root.@RESET@ = known<1>(0);
    };
    auto observe = [&](unsigned x, unsigned saved) {
      expect(root.result_0, mix(x));
      expect(root.result_1, saved);
      expect(root.result_2, x);
    };
    drive(0, 0);
    TestSystem system(root);
    require(system.AddModule(root)); system.Build(); system.Reset();
    unsigned saved = 0;
    for (unsigned x : {3u, 127u, 255u, 0u, 42u}) {
      drive(x, 0);
      require(system.Step() == gfsim::SimStepResult::Running);
      observe(x, saved);
      drive(x, 1);
      // Re-evaluation cannot commit module state or sampled clock history.
      root.Work(); observe(x, saved);
      root.Work(); observe(x, saved);
      root.DiscardNext(); root.Xfer();
      require(system.Step() == gfsim::SimStepResult::Running);
      observe(x, saved);
      saved = mix(x);
    }
    drive(17, 0);
    require(system.Step() == gfsim::SimStepResult::Running);
    observe(17, saved);
    const auto epoch = system.cycle();
    drive(199, 1, false);
    require(system.Step() == gfsim::SimStepResult::Failed);
    require(system.cycle() == epoch);
    // Inspect committed Q without publishing this diagnostic evaluation.
    drive(17, 0);
    root.Work(); observe(17, saved); root.DiscardNext(); root.Xfer();
    // Retry exactly the rejected rising edge. Both data and clock must roll back.
    drive(199, 1);
    root.Work(); observe(199, saved);
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure));
    root.Xfer(); saved = mix(199);
    drive(17, 0);
    root.Work(); observe(17, saved); root.DiscardNext();
    for (bool z : {false, true}) {
      drive(0, 0);
      using B = gfsim::Bits<8>;
      root.x = gfsim::wire<B>::fromPacked(gfsim::FourState<8>::fromMasks(
          B::ones(), B{0}, z ? B::ones() : B{0}));
      root.Work();
      // Struct/Table transport preserves Z, whereas arithmetic produces X.
      const auto raw = root.result_2.packed();
      require(raw.value() == B::ones() && raw.knownMask() == B{0});
      require(raw.zMask() == (z ? B::ones() : B{0}));
      const auto mixed = root.result_0.packed();
      require(mixed.knownMask() == B{0} && mixed.zMask() == B{0});
      expect(root.result_1, saved);
      root.DiscardNext();
    }
    drive(17, 0);
    root.Work(); observe(17, saved); root.DiscardNext();
  }
  std::cout << "PASS\n";
}
"""


def function_body(text, brace):
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[brace:end]


def inspect_work(directory):
    """Check every family, including descendants hidden behind a small root."""
    rows = []
    helpers = []
    for path in directory.rglob("*.hpp"):
        text = path.read_text()
        for family in re.finditer(r"class pyc_family_(\w+)\s+final", text):
            start = family.end()
            following = text.find("class pyc_family_", start)
            body = text[start : following if following >= 0 else len(text)]
            for method in re.finditer(
                r"\bvoid (Work|pyc_work_part_\d+|pyc_work_dispatch_\w+)\([^)]*\)\s*\{",
                body,
            ):
                code = function_body(body, method.end() - 1)
                lines = len(code.splitlines())
                assert lines <= 160, (family[1], method[1], lines)
                assert len(code.encode()) <= 16 * 1024, (family[1], method[1])
                rows.append(
                    {
                        "family": family[1],
                        "method": method[1],
                        "lines": lines,
                        "bytes": len(code.encode()),
                    }
                )
                if method[1] != "Work":
                    helpers.append((family[1], method[1]))
    assert rows, "no generated family Work methods inspected"
    leaf = [
        item
        for item in helpers
        if item[0].startswith("MixingLeaf") and item[1].startswith("pyc_work_part_")
    ]
    assert len(leaf) >= 2, "deep leaf must split across multiple bounded functions"
    return rows, helpers


def main():
    parser = argparse.ArgumentParser()
    for option in ("repo", "source-compiler", "linker", "emitter", "cxx", "scratch"):
        parser.add_argument("--" + option, required=True)
    args = parser.parse_args()
    repo = Path(args.repo).resolve()
    scratch_root = Path(args.scratch).resolve()
    scratch_root.mkdir(parents=True, exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix="work-outline-", dir=scratch_root))
    source = scratch / "source"
    source.mkdir()
    fixture = Path(__file__).with_name("work_function_outline_source.py")
    shutil.copyfile(fixture, source / fixture.name)
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python"),
        PYTHONDONTWRITEBYTECODE="1",
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
    )
    commands = []

    def run(command):
        command = list(map(str, command))
        result = subprocess.run(
            command, cwd=repo, env=env, text=True, capture_output=True, timeout=180
        )
        commands.append(
            {
                "command": command,
                "exit_status": result.returncode,
                "stdout": result.stdout,
                "stderr": result.stderr,
            }
        )
        (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert result.returncode == 0, commands[-1]
        return result.stdout

    def cli(*options):
        return run([sys.executable, "-m", "pycircuit.cli", *options])

    unit, final, cpp = scratch / "unit", scratch / "final.ac", scratch / "cpp"
    cli(
        "compile",
        "-c",
        source / fixture.name,
        "--source-root",
        source,
        "--package-prefix",
        "outline",
        "-o",
        unit,
    )
    cli(
        "link",
        unit,
        "--top",
        "outline.work_function_outline_source.OutlineTop",
        "-o",
        final,
    )
    cli("emit", final, "--target", "cpp", "-o", cpp)
    rows, helpers = inspect_work(cpp)
    (scratch / "work-sizes.json").write_text(json.dumps(rows, indent=2) + "\n")
    ports = json.loads((cpp / "ports.json").read_text())
    inputs = {pin["name"]: pin["wire"] for pin in ports["inputs"]}
    driver = scratch / "driver.cpp"
    driver.write_text(
        DRIVER.replace("@CLOCK@", inputs["pyc_clk"]).replace(
            "@RESET@", inputs["pyc_rst"]
        )
    )
    receipt = json.loads((cpp / "generated.json").read_text())
    sources = [
        cpp / item["path"] for item in receipt["files"] if item["path"].endswith(".cpp")
    ]
    toolroot = Path(args.source_compiler).resolve().parent.parent
    runtime = next(
        (
            path
            for path in (
                toolroot / "runtime/libpyc6_runtime.a",
                toolroot / "lib/libpyc6_runtime.a",
            )
            if path.is_file()
        ),
        None,
    )
    assert runtime, "candidate Runtime archive missing"
    for optimization in ("-O0", "-O1"):
        binary = scratch / ("runner" + optimization)
        run(
            [
                args.cxx,
                "-std=c++20",
                optimization,
                "-pthread",
                "-I" + str(repo / "include"),
                "-I" + str(cpp),
                driver,
                *sources,
                runtime,
                "-o",
                binary,
            ]
        )
        assert run([binary]).strip() == "PASS"
        if optimization == "-O1":
            symbols = run(["nm", "-C", "-S", binary])
            native_sizes = []
            for line in symbols.splitlines():
                match = re.match(r"[0-9a-f]+ ([0-9a-f]+) [tTW] (.*)", line)
                if match and re.search(
                    r"::(?:Work|pyc_work_part_\d+|pyc_work_dispatch_\w+)\(", match[2]
                ):
                    native_sizes.append(
                        {"symbol": match[2], "bytes": int(match[1], 16)}
                    )
            (scratch / "native-sizes.json").write_text(
                json.dumps(native_sizes, indent=2) + "\n"
            )
            for family, helper in helpers:
                assert re.search(
                    re.escape("pyc_family_" + family)
                    + r"<[^>]+>::"
                    + re.escape(helper)
                    + r"\(",
                    symbols,
                ), (family, helper, "helper was inlined or removed")
    print("bounded work, deep helpers and native transaction oracles PASS")


if __name__ == "__main__":
    main()
