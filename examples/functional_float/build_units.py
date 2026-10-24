#!/usr/bin/env python3
"""Publish reusable numeric providers through the public compile/link/emit CLI.

The selected top's complete provider closure is compiled independently. C++
and Verilog are emitted from its same linked final artifact. No design function
is executed by Python, and no generated bundle is checked into source control.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import subprocess
import sys

SOURCES = {
    "fp32_types": (),
    "fp32_product": ("fp32_types",),
    "fp32_sum": ("fp32_types",),
    "fp32_product_sum": ("fp32_types", "fp32_product", "fp32_sum"),
    "fp32_exp": ("fp32_types", "fp32_product", "fp32_sum"),
    "fp32_divide": ("fp32_types", "fp32_product"),
    "fp32_max": ("fp32_types",),
    "fp16_types": (),
    "fp32_to_fp16": ("fp16_types",),
    "bf16_types": (),
    "fp32_to_bf16": ("bf16_types",),
    "fp64_types": (),
    "fp64_round": ("fp64_types",),
    "fp64_product": ("fp64_types", "fp64_round"),
    "fp64_sum": ("fp64_types", "fp64_round"),
    "fp64_divide": ("fp64_types", "fp64_round"),
    "float_compare": (),
    "float_from_integer": ("fp64_types",),
    "float_to_integer": ("fp64_types",),
    "float_to_integer_bits": ("fp64_types",),
    "float_round_integral": (
        "fp64_types",
        "float_to_integer_bits",
        "float_from_integer",
    ),
    "packed4_formats": (),
    "packed4_encode": ("fp64_types", "packed4_formats"),
    "packed4_decode": ("fp64_types", "packed4_formats"),
    "float_formats": ("fp64_types",),
    "float_convert": ("fp64_types", "float_formats"),
    "float_fused": ("fp64_types", "float_formats"),
    "float_fused_format": ("fp64_types", "float_formats", "float_fused"),
    "e8m0_encode": ("fp64_types", "float_formats"),
    "lowp_formats": (),
    "lowp_decode": ("fp64_types",),
    "lowp_encode": ("fp64_types", "float_formats", "lowp_formats"),
    "finite_lowp_cast": ("fp64_types", "float_formats", "lowp_formats"),
    "fp19_expansion": (),
    "fp32_negative_power": (),
    "fp32_log_step": (),
    "fp32_log": ("fp64_types", "float_formats", "fp32_log_step"),
    "float_sqrt_step": (),
    "float_sqrt": (
        "fp32_types",
        "fp32_product",
        "fp64_types",
        "fp64_round",
        "float_sqrt_step",
    ),
    "word_division": ("word_arithmetic",),
    "fp32_trunc_modulo": ("fp32_types", "fp32_product"),
    "fp32_remainder": ("fp32_types", "fp32_product", "fp32_sum", "fp32_divide"),
    "word_arithmetic": (),
    "bit_transform": ("word_arithmetic",),
    "tracked_bit_transform": ("word_arithmetic", "bit_transform"),
    "address": (),
    "integer_element": (),
    "fp32_u32_add": (),
    "input_expand": (),
}
TOPS = {
    "fp32_product": "FP32Product",
    "fp32_sum": "FP32Sum",
    "fp32_product_sum": "FP32ProductSum",
    "fp32_exp": "FP32Exp",
    "fp32_divide": "FP32Divide",
    "fp32_max": "FP32Max",
    "fp32_to_fp16": "FP32ToFP16",
    "fp32_to_bf16": "FP32ToBF16",
    "fp64_product": "FP64Product",
    "fp64_sum": "FP64Sum",
    "fp64_divide": "FP64Divide",
    "float_compare": "FloatCompare",
    "float_from_integer": "FloatFromInteger",
    "float_to_integer": "FloatToInteger",
    "float_to_integer_bits": "FloatToIntegerBits",
    "float_round_integral": "FloatRoundIntegral",
    "packed4_encode": "Packed4Encode",
    "packed4_decode": "Packed4Decode",
    "float_sqrt": "FloatSqrt",
    "float_convert": "FloatConvert",
    "float_fused": "FloatFused",
    "float_fused_format": "FloatFusedFormat",
    "fp32_log": "FP32Log",
    "e8m0_encode": "E8M0Encode",
    "lowp_decode": "LowpDecode",
    "lowp_encode": "LowpEncode",
    "finite_lowp_cast": "FiniteLowpCast",
    "fp19_expansion": "FP19FiniteExpand",
    "fp32_negative_power": "FP32NegativePower",
    "fp32_trunc_modulo": "FP32TruncModulo",
    "word_division": "WordDivision",
    "fp32_remainder": "FP32FloorModulo",
    "word_arithmetic": "WordArithmetic",
    "bit_transform": "BitTransform",
    "tracked_bit_transform": "TrackedBitTransform",
    "address": "RowMajorAddress",
    "integer_element": "IntegerElement",
    "fp32_u32_add": "U32Add",
    "input_expand": "InputExpand",
}


def closure(unit: str) -> list[str]:
    result = []
    for dependency in SOURCES[unit]:
        for item in closure(dependency):
            if item not in result:
                result.append(item)
    result.append(unit)
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--only", choices=TOPS, action="append")
    parser.add_argument("--replace", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    out = args.output.resolve()
    if out == root or root in out.parents:
        parser.error("choose an output directory outside the example sources")
    (out / "units").mkdir(parents=True, exist_ok=True)
    (out / "linked").mkdir(exist_ok=True)
    replace = ["--replace"] if args.replace else []
    selected = args.only or list(TOPS)
    compiled = set()

    def run(*command: str) -> None:
        subprocess.run([sys.executable, "-m", "pycircuit.cli", *command], check=True)

    for unit in selected:
        providers = closure(unit)
        for provider in providers:
            if provider in compiled:
                continue
            imports = [
                arg
                for dependency in SOURCES[provider]
                for arg in ("-I", str(out / "units" / dependency))
            ]
            run(
                "compile",
                "-c",
                str(root / f"{provider}.py"),
                "--source-root",
                str(root),
                "--package-prefix",
                "functional_float",
                *imports,
                "-o",
                str(out / "units" / provider),
                *replace,
            )
            compiled.add(provider)
        linked = out / "linked" / f"{unit}.ac"
        run(
            "link",
            *(str(out / "units" / provider) for provider in providers),
            "--top",
            f"functional_float.{unit}.{TOPS[unit]}",
            "-o",
            str(linked),
            *replace,
        )
        for target in ("cpp", "verilog"):
            run(
                "emit",
                "--target",
                target,
                "-o",
                str(out / f"{target}_{unit}"),
                str(linked),
                *replace,
            )
        print(f"{unit}: compile/link/cpp/verilog PASS", flush=True)


if __name__ == "__main__":
    main()
