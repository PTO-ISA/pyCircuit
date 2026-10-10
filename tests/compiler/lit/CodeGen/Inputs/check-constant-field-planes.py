#!/usr/bin/env python3
"""Bounded generated-C++ checks for constant field-plane materialization."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

STATIC = re.compile(
    r"static const\s+([^;=]+)\s+(pyc_value_[0-9]+)\s*=\s*\[\]\s*\{(.*?)\n\s*\}\(\);",
    re.S,
)


def require(c, m):
    if not c:
        raise AssertionError(m)


def validate(text):
    blocks = list(STATIC.finditer(text))
    require(len(blocks) >= 3, "expected at least three promoted constant field planes")
    for block in blocks:
        body = block.group(3)
        require("return " in body, "static initializer has no returned plane")
        require(
            "this" not in body and "[&" not in body and "[=" not in body,
            "static initializer captures outside state",
        )
    require(not re.search(r"static\s+(?!const\b)", text), "writable static storage")
    constant_definition = re.search(
        r"class pyc_family_ConstantChild.*?class ConstantChild", text, re.S
    )
    require(constant_definition is not None, "constant child definition missing")
    constant_blocks = list(STATIC.finditer(constant_definition.group(0)))
    types = [block.group(1) for block in constant_blocks]
    for width in ("1", "65", "130"):
        require(
            any(f"Bits<{width}>" in plane_type for plane_type in types),
            f"eligible Bits<{width}> field plane missing",
        )
    require(
        any("ConstantMode" in plane_type for plane_type in types),
        "eligible enum field plane missing",
    )
    require(
        not any("Bits<8>" in plane_type for plane_type in types),
        "mixed/dynamic Bits<8> plane was promoted",
    )
    outside = STATIC.sub("", text)
    for width in ("65", "130"):
        require(
            f"hardware_traits<gfsim::Bits<{width}>>::width>::known" not in outside,
            f"eligible Bits<{width}> hot-path literal construction retained",
        )
    task_counts = [
        int(value)
        for value in re.findall(r"std::array<gfsim::WorkItem, ([0-9]+)>", text)
    ]
    require(
        task_counts and max(task_counts) >= 3,
        "independent delegated WorkItems not generated",
    )
    require(
        "pyc_work_executor_->run(pyc_tasks)" in text,
        "WorkExecutor delegation not generated",
    )
    require("pyc_family_ConstantChild" in text, "constant child family missing")
    for role in ("pair", "left", "right"):
        require(f"pyc_instance_{role}.get()" in text, f"delegated {role} child missing")
    generic = re.search(
        r"class pyc_family_GenericNegative.*?class GenericNegative", text, re.S
    )
    require(generic is not None, "generic negative definition missing")
    require(
        "static const" not in generic.group(0), "unbound demanded shape was promoted"
    )
    excluded = re.search(
        r"class pyc_family_ExcludedNegative.*?class ExcludedNegative", text, re.S
    )
    require(excluded is not None, "excluded-operation definition missing")
    excluded_blocks = list(STATIC.finditer(excluded.group(0)))
    require(
        len(excluded_blocks) == 1 and "Bits<8>" in excluded_blocks[0].group(1),
        "excluded definition must promote only its literal source table",
    )
    return len(blocks)


def self_test():
    good = """static const Plane<gfsim::Bits<1>> pyc_value_1 = [] {\n Plane x;\n return x;\n}();\nstatic const Plane<gfsim::Bits<65>> pyc_value_2 = [] {\n Plane x; return x;\n}();\nstatic const Plane<gfsim::Bits<130>> pyc_value_3 = [] {\n Plane x; return x;\n}();\nstatic const Plane<ConstantMode> pyc_value_4 = [] {\n Plane x; return x;\n}();\nstd::array<gfsim::WorkItem, 4> pyc_tasks; pyc_work_executor_->run(pyc_tasks);\npyc_instance_pair.get(); pyc_instance_left.get(); pyc_instance_right.get();\npyc_family_ConstantChild<(pyc_count * ((1 * (2))))> collection;\nclass pyc_family_GenericNegative { void Work() {} }; class GenericNegative {};"""
    good += "\nclass pyc_family_ExcludedNegative { void Work() { static const Plane<gfsim::Bits<8>> pyc_value_20 = [] {\n Plane x; return x;\n}(); } }; class ExcludedNegative {};"
    constants, negatives = good.split("class pyc_family_GenericNegative", 1)
    good = (
        "class pyc_family_ConstantChild {\n"
        + constants
        + "\n}; class ConstantChild {};\nclass pyc_family_GenericNegative"
        + negatives
    )
    require(validate(good) == 5, "self-test good")
    bad = (
        good.replace("static const Plane<", "static Plane<", 1),
        "writable static storage",
    )
    captured = (
        good.replace("pyc_value_1 = []", "pyc_value_1 = [&]"),
        "eligible Bits<1>",
    )
    dynamic = (good.replace("Bits<65>", "Bits<8>", 1), "eligible Bits<65>")
    retained = (
        good + "\nhardware_traits<gfsim::Bits<130>>::width>::known",
        "hot-path literal",
    )
    generic_promoted = (
        good.replace(
            "class pyc_family_GenericNegative { void Work() {} }",
            "class pyc_family_GenericNegative { void Work() { static const Plane cached{}; } }",
        ),
        "unbound demanded shape",
    )
    excluded_promoted = (
        good.replace(
            "Plane x; return x;\n}(); } }; class ExcludedNegative",
            "Plane x; return x;\n}(); static const Plane<gfsim::Bits<8>> pyc_value_21 = [] {\n Plane y; return y;\n}(); } }; class ExcludedNegative",
        ),
        "promote only its literal source table",
    )
    for name, text, diagnostic in (
        ("mutable", *bad),
        ("captured", *captured),
        ("dynamic", *dynamic),
        ("retained", *retained),
        ("generic", *generic_promoted),
        ("excluded", *excluded_promoted),
    ):
        try:
            validate(text)
        except AssertionError as e:
            require(diagnostic in str(e), "self-test diagnostic")
        else:
            raise AssertionError(f"structural mutation accepted: {name}")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("generated", type=Path, nargs="?")
    p.add_argument("--self-test", action="store_true")
    a = p.parse_args()
    if a.self_test:
        self_test()
        return
    require(a.generated is not None, "generated directory required")
    paths = [*a.generated.rglob("*.cpp"), *a.generated.rglob("*.hpp")]
    text = "\n".join(path.read_text() for path in paths)
    count = validate(text)
    print(f"constant field-plane structure passed: {count} static planes")


if __name__ == "__main__":
    main()
