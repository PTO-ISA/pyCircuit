"""Serialize queue oracle rows for the existing native/RTL test drivers."""

import importlib.util
import json
from pathlib import Path

_model_path = Path(__file__).resolve().parents[3] / "oracles/queue_source/models.py"
_spec = importlib.util.spec_from_file_location("queue_source_models", _model_path)
_models = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_models)


def materialize(name, destination):
    record = _models.oracle(name)
    destination.mkdir(parents=True, exist_ok=True)
    input_bits = record["input_bits"]
    output_bits = record["output_bits"]
    rows = record["rows"]
    header = [
        "#pragma once",
        "#include <iterator>",
        "#include <string_view>",
        f"constexpr unsigned input_bits={input_bits}, output_bits={output_bits};",
        "struct Row { unsigned clk,rst,valid,take; std::string_view data,expected,data_known,data_z,expected_known,expected_z; };",
        "constexpr Row rows[]={",
    ]
    for row in rows:
        data = format(row["data"], f"0{input_bits}b")
        expected = format(row["expected"], f"0{output_bits}b")
        planes = [
            format(row.get(key, default), f"0{width}b")
            for key, default, width in (
                ("data_known", (1 << input_bits) - 1, input_bits),
                ("data_z", 0, input_bits),
                ("expected_known", (1 << output_bits) - 1, output_bits),
                ("expected_z", 0, output_bits),
            )
        ]
        header.append(
            f"{{{row['clk']},{row['rst']},{row['valid']},{row['take']},"
            f'"{data}","{expected}",'
            + ",".join(f'"{plane}"' for plane in planes)
            + "},"
        )
    header.append("};")
    (destination / "queue_source_vectors.hpp").write_text("\n".join(header) + "\n")
    (destination / "queue_source_widths.svh").write_text(
        f"`define Q4_INPUT_BITS {input_bits}\n`define Q4_OUTPUT_BITS {output_bits}\n"
    )

    def four_state_literal(row, prefix, width):
        value = row[prefix]
        known = row.get(prefix + "_known", (1 << width) - 1)
        z = row.get(prefix + "_z", 0)
        text = "".join(
            str((value >> bit) & 1)
            if (known >> bit) & 1
            else "z"
            if (z >> bit) & 1
            else "x"
            for bit in reversed(range(width))
        )
        return f"{width}'b{text}"

    sv = []
    for index, row in enumerate(rows):
        sv.append(
            f"pyc_7079635f727374=1'b{row['rst']};valid=1'b{row['valid']};"
            f"take=1'b{row['take']};data={four_state_literal(row, 'data', input_bits)};#1;"
        )
        sv.append(
            f"if(result!=={four_state_literal(row, 'expected', output_bits)})"
            f'$fatal(1,"Q4 {name} row{index} failed");'
        )
        sv.append(f'$display("WORK {index}");pyc_7079635f636c6b=1\'b{row["clk"]};#1;')
    (destination / "queue_source_rows.svh").write_text("\n".join(sv) + "\n")
    (destination / "config.json").write_text(
        json.dumps(
            {
                "deadlock_window": None,
                "max_domain_cycles": {},
                "max_ticks": len(rows) + 4,
                "schema": "pycircuit-model-config",
                "version": "1",
            },
            separators=(",", ":"),
        )
        + "\n"
    )
    (destination / "expected.stdout").write_text(
        "".join(f"WORK {index}\n" for index, _ in enumerate(rows))
    )
