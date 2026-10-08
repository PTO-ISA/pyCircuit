"""Serialize queue oracle rows for the existing native/RTL test drivers."""

import importlib.util
import json
from pathlib import Path

_model_path = Path(__file__).resolve().parents[3] / "oracles/queue_source/models.py"
_spec = importlib.util.spec_from_file_location("queue_source_models", _model_path)
_models = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_models)


def failure_contracts():
    return _models.queue_failure_contracts()


def materialize(name, destination):
    if name == "credit_independent":
        checker = _model_path.with_name("credit_independent_check.py")
        spec = importlib.util.spec_from_file_location(
            "queue_credit_independent", checker
        )
        independent = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(independent)
        record = independent.vectors()
    else:
        record = _models.oracle(name)
    record = _models.execution_record(record)
    destination.mkdir(parents=True, exist_ok=True)
    input_bits = record["input_bits"]
    output_bits = record["output_bits"]
    rows = record["rows"]
    header = [
        "#pragma once",
        "#include <iterator>",
        "#include <string_view>",
        f"constexpr unsigned input_bits={input_bits}, output_bits={output_bits};",
        "struct Row { unsigned clk,rst,valid,take; std::string_view data,expected,data_known,data_z,expected_known,expected_z,execution_expected,execution_expected_known,execution_expected_z; bool execution_failure,host_reset; };",
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
        execution_planes = [
            format(row.get(key, default), f"0{output_bits}b")
            for key, default in (
                ("execution_expected", row["expected"]),
                (
                    "execution_expected_known",
                    row.get("expected_known", (1 << output_bits) - 1),
                ),
                ("execution_expected_z", row.get("expected_z", 0)),
            )
        ]
        header.append(
            f"{{{row['clk']},{row['rst']},{row['valid']},{row['take']},"
            f'"{data}","{expected}",'
            + ",".join(f'"{plane}"' for plane in (*planes, *execution_planes))
            + f",{str(row.get('execution_failure', False)).lower()},"
            + f"{str(row.get('host_reset', False)).lower()}"
            + "},"
        )
    header.append("};")
    config = json.dumps(
        {
            "deadlock_window": None,
            "max_domain_cycles": {},
            "max_ticks": len(rows) + 4,
            "schema": "pycircuit-model-config",
            "version": "1",
        },
        separators=(",", ":"),
    )
    header.append(f'constexpr std::string_view configuration=R"({config})";')
    (destination / "queue_source_vectors.hpp").write_text("\n".join(header) + "\n")
    (destination / "queue_source_widths.svh").write_text(
        f"`define Q4_INPUT_BITS {input_bits}\n`define Q4_OUTPUT_BITS {output_bits}\n"
    )

    def four_state_literal(row, prefix, width):
        value = row[prefix]
        known = row.get(prefix + "_known", (1 << width) - 1)
        z = row.get(prefix + "_z", 0)
        text = "".join(
            (
                str((value >> bit) & 1)
                if (known >> bit) & 1
                else "z" if (z >> bit) & 1 else "x"
            )
            for bit in reversed(range(width))
        )
        return f"{width}'b{text}"

    sv = []
    for index, row in enumerate(rows):
        execution = dict(row, expected=row.get("execution_expected", row["expected"]))
        execution["expected_known"] = row.get(
            "execution_expected_known",
            row.get("expected_known", (1 << output_bits) - 1),
        )
        execution["expected_z"] = row.get(
            "execution_expected_z", row.get("expected_z", 0)
        )
        sv.append("`ifdef Q4_CHECKS")
        sv.append(
            f"checked_row({index},1'b{row['clk']},1'b{row['rst']},1'b{row['valid']},1'b{row['take']},"
            f"{four_state_literal(row, 'data', input_bits)},{four_state_literal(execution, 'expected', output_bits)},"
            f"1'b{int(row.get('execution_failure', False))},1'b{int(row.get('host_reset', False))});"
        )
        sv.append("`else")
        sv.append(
            f"pyc_7079635f727374=1'b{row['rst']};valid=1'b{row['valid']};"
            f"take=1'b{row['take']};data={four_state_literal(row, 'data', input_bits)};#1;"
        )
        sv.append(
            f"if(result!=={four_state_literal(row, 'expected', output_bits)})"
            f'$fatal(1,"Q4 {name} row{index} failed");'
        )
        sv.append(f'$display("WORK {index}");pyc_7079635f636c6b=1\'b{row["clk"]};#1;')
        sv.append("`endif")
    (destination / "queue_source_rows.svh").write_text("\n".join(sv) + "\n")
    (destination / "config.json").write_text(config + "\n")
    (destination / "oracle.json").write_text(json.dumps(record, indent=2) + "\n")
    (destination / "expected.stdout").write_text(
        "".join(
            (f"HOST_RESET {index}\n" if row.get("host_reset") else "")
            + f"{'FAILED' if row.get('execution_failure') else 'WORK'} {index}\n"
            for index, row in enumerate(rows)
        )
    )
