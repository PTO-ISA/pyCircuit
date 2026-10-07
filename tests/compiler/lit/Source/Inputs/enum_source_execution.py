"""Existing Enum vector/execution functions, shared by selected case families."""
import json
import re


def execution_tools(*, args, repo, fixtures, build, source, units,
                    declaration_sources, runtime, compile_unit, cli, run,
                    payload, digest, clock):
    CLOCK = clock  # noqa: N806 - preserve the existing encoded-pin vector protocol
    def make_vectors(
        directory,
        inputs,
        fields,
        rows,
        gold,
        state=False,
        probes=(),
        probe_gold=(),
        failures=(),
        known_prefix=None,
        plane_rows=(),
        probe_planes=(),
        plane_inputs=None,
        latent=False,
    ):
        result_width = sum(fields.values())
        (directory / "enum-source-width.hpp").write_text(f"constexpr unsigned result_width={result_width};\n")
        offsets = {name: sum(list(fields.values())[index + 1 :]) for index, name in enumerate(fields)}
        cpp = [f"constexpr unsigned row_count={len(rows)};"]

        def arrays(label, frames):
            return [f"const std::string_view {label}_{name}[]={{" + ",".join(json.dumps(row[name]) for row in frames) + "};" for name in inputs]

        def drive_cpp(function, label):
            return (
                [f"void {function}(pyc_dut::Inputs &p,unsigned row){{"]
                + [f"p.{name}=decltype(p.{name})::fromPacked(input<{width}>({label}_{name}[row]).packed());" for name, width in inputs.items()]
                + ["}"]
            )

        cpp += arrays("normal", rows) + drive_cpp("drive", "normal")
        cpp += ["const std::string_view expected[]={" + ",".join(json.dumps("".join(g.values())) for g in gold) + "};"]

        def plane_cpp(function, label, expected_rows, masks=()):
            code = [f"template<class Output> void {function}(const Output &o,unsigned row){{"]
            if state:
                for name, width in fields.items():
                    condition = f"plane_{label}_{name}[row]" if masks else "true"
                    code.append(f"if({condition})planes(o.result,{offsets[name]},input<{width}>({label}_{name}[row]));")
            elif plane_inputs is not None:
                for field, specification in plane_inputs.items():
                    if isinstance(specification, dict):
                        control, yes, no = specification["control"], specification["yes"], specification["no"]
                        code.append(
                            f'if(normal_{control}[row]=="0"||normal_{control}[row]=="1")planes(o.result,{offsets[field]},input<{fields[field]}>(normal_{control}[row]=="1"?normal_{yes}[row]:normal_{no}[row]));'
                        )
                    else:
                        input_name, source_offset = specification if isinstance(specification, tuple) else (specification, 0)
                        code.append(f"planes(o.result,{offsets[field]},input<{inputs[input_name]}>(normal_{input_name}[row]),{source_offset},{fields[field]});")
            else:
                for name, source_name in (("left_raw", "left"), ("wide_raw", "wide"), ("huge_raw", "huge")):
                    code.append(f"planes(o.result,{offsets[name]},input<{fields[name]}>(normal_{source_name}[row]));")
                for name in ("wider_direct", "wider_alias", "wider_field"):
                    code.append(f'planes(o.result,{offsets[name]},input<9>(std::string("0000000")+std::string(normal_left[row])));')
                code.append(
                    f'if(normal_choose[row]=="0"||normal_choose[row]=="1")planes(o.result,{offsets["selected"]},input<2>(normal_choose[row]=="1"?normal_left[row]:normal_right[row]));'
                )
            return code + ["}"]

        if state:
            for label, frames in (("normal_gold", gold), ("probe_gold", probe_gold)):
                cpp += [f"const std::string_view {label}_{name}[]={{" + ",".join(json.dumps(g[name]) for g in frames) + "};" for name in fields]
        if state:
            for label, masks in (("normal_gold", plane_rows), ("probe_gold", probe_planes)):
                if masks:
                    cpp += [f"const bool plane_{label}_{name}[]={{" + ",".join("true" if name in mask else "false" for mask in masks) + "};" for name in fields]
        cpp += plane_cpp("checkPlanes", "normal_gold", gold, plane_rows)
        if state:
            cpp += [
                f"constexpr unsigned probe_count={len(probes)},failure_count={len(failures)};",
                "const unsigned probe_action[]={" + ",".join(str(action) for _, action in probes) + "};",
            ]
            cpp += arrays("probe", [row for row, _ in probes]) + drive_cpp("driveProbe", "probe")
            cpp += ["const std::string_view probe_expected[]={" + ",".join(json.dumps("".join(g.values())) for g in probe_gold) + "};"]
            cpp += plane_cpp("checkProbePlanes", "probe_gold", probe_gold, probe_planes)
            cpp += arrays("failure", failures) + drive_cpp("driveFailure", "failure")
            cpp += ["void driveRoot(pyc_root &root,const pyc_dut::Inputs &p){", *[f"root.{name}=p.{name};" for name in inputs], "}"]
        if latent:
            cpp += [
                'void replayLatentTuples(unsigned workers){gfsim::WorkExecutor pool(workers);pyc_root root("latent",&pool);',
                "for(unsigned row=0;row<row_count;++row)for(unsigned pattern=0;pattern<4;++pattern){",
            ]
            cpp += [
                f"const auto {name}=latentInput<{width}>(normal_{name}[row],pattern);root.{name}=decltype(root.{name})::fromPacked({name}.packed());"
                for name, width in inputs.items()
            ]
            cpp += ["root.Work();check(root,expected[row]);"]
            cpp += [f"planes(root.result,{offsets[field]},{input_name});" for field, input_name in plane_inputs.items()]
            cpp += ["root.DiscardNext();root.Xfer();}}"]
        (directory / "enum-source-vectors.hpp").write_text("\n".join(cpp) + "\n")
        known = [index < known_prefix if known_prefix is not None else all(c in "01" for value in row.values() for c in value) for index, row in enumerate(rows)]
        sv = [f"localparam integer row_count={len(rows)};", *[f"logic[{width - 1}:0] {name}=0;" for name, width in inputs.items()], f"wire[{result_width - 1}:0] result;"]
        if state:
            sv += ["logic next_clock;"]
        sv += ["task drive(input integer row);case(row)"]
        for index, row in enumerate(rows):
            sv += [f"{index}:begin", *[f"{'next_clock' if state and name == CLOCK else name}={inputs[name]}'b{value};" for name, value in row.items()], "end"]
        sv += ["endcase endtask", f"function automatic logic[{result_width - 1}:0] golden(input integer row);case(row)"]
        sv += [f"{index}:golden={result_width}'b{''.join(g.values())};" for index, g in enumerate(gold)]
        sv += ["default:golden='x;endcase endfunction", "function automatic bit known_row(input integer row);case(row)"]
        sv += [f"{index}:known_row={int(value)};" for index, value in enumerate(known)] + ["default:known_row=0;endcase endfunction"]
        if state and failures:
            sv += [f"task drive_failure;{CLOCK}=0;" + "".join(f"{name}={inputs[name]}'b{value};" for name, value in failures[0].items() if name != CLOCK) + "endtask"]
        (directory / "enum-source-vectors.svh").write_text("\n".join(sv) + "\n")
        return known

    def execute(
        name,
        text,
        top,
        inputs,
        fields,
        rows,
        gold,
        state=False,
        probes=(),
        probe_gold=(),
        failures=(),
        known_prefix=None,
        plane_rows=(),
        probe_planes=(),
        plane_inputs=None,
        latent=False,
    ):
        output = build / ("semantics-" + name)
        output.mkdir()
        filename = name + ".py"
        (source / filename).write_text(text)
        unit = output / "unit"
        compile_unit(filename, unit, list(units.values()))
        final = output / "design.ac"
        cli("link", *[units[n] for n in declaration_sources], unit, "--top", "enums." + name + "." + top, "-o", final)
        run([args.optimizer, final, "--ac-verify-hardware", "-o", output / "verified.ac"])
        for target in ("cpp", "verilog"):
            cli("emit", final, "--target", target, "-o", output / target)
        known = make_vectors(output, inputs, fields, rows, gold, state, probes, probe_gold, failures, known_prefix, plane_rows, probe_planes, plane_inputs, latent)
        cpp_receipt = json.loads((output / "cpp/generated.json").read_text())
        cpp = [output / "cpp" / item["path"] for item in cpp_receipt["files"] if item["path"].endswith(".cpp")]
        defines = ["-DENUM_SOURCE_STATE"] if state else []
        if latent:
            defines.append("-DENUM_SOURCE_LATENT")
        runner = output / "runner"
        run(
            [
                args.cxx,
                "-O2",
                "-std=c++20",
                "-pthread",
                *defines,
                "-I" + str(repo / "include"),
                "-I" + str(output / "cpp"),
                "-I" + str(output),
                fixtures / "enum-source.cpp",
                *cpp,
                runtime,
                "-o",
                runner,
            ]
        )
        config = output / "config.json"
        config.write_text(
            json.dumps(
                {"schema": "pycircuit-model-config", "version": "1", "max_ticks": len(rows) + 16, "max_domain_cycles": {}, "deadlock_window": None},
                separators=(",", ":"),
                sort_keys=True,
            )
            + "\n"
        )
        traces = []
        for workers in (1, 2):
            result = run([runner, "--workers", workers, "--config", config])
            (output / f"workers-{workers}.stdout").write_text(result.stdout)
            traces.append([line for line in result.stdout.splitlines() if line.startswith("WORK ")])
        assert traces[0] == traces[1] and len(traces[0]) == len(rows)
        rtl_receipt = json.loads((output / "verilog/generated.json").read_text())
        rtl = [output / "verilog" / item["path"] for item in rtl_receipt["files"] if item["role"] == "rtl"]
        rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
        primitives = [repo / "include/verilog/dff.v", repo / "include/verilog/dffe.v"]
        run(
            [
                args.iverilog,
                "-g2012",
                "-DENUM_SOURCE_FOUR_STATE",
                *defines,
                "-I" + str(output),
                "-s",
                "tb",
                "-o",
                output / "four.vvp",
                *primitives,
                *rtl,
                fixtures / "enum-source.sv",
            ]
        )
        observed = run([args.vvp, output / "four.vvp"]).stdout
        assert [line for line in observed.splitlines() if line.startswith("WORK ")] == traces[0]
        run(
            [
                args.verilator,
                "--binary",
                "--timing",
                "--top-module",
                "tb",
                "--prefix",
                "Venumsource",
                "--Mdir",
                output / "rtl-build",
                "-j",
                "2",
                "-Wno-fatal",
                *defines,
                "-I" + str(output),
                *primitives,
                *rtl,
                fixtures / "enum-source.sv",
            ]
        )
        observed = run([output / "rtl-build/Venumsource"]).stdout
        assert [line for line in observed.splitlines() if line.startswith("WORK ")] == [line for line, included in zip(traces[0], known, strict=True) if included]
        if state and failures:
            run(
                [
                    args.iverilog,
                    "-g2012",
                    "-DENUM_SOURCE_FOUR_STATE",
                    "-DENUM_SOURCE_FAILURE",
                    *defines,
                    "-I" + str(output),
                    "-s",
                    "tb",
                    "-o",
                    output / "failure.vvp",
                    *primitives,
                    *rtl,
                    fixtures / "enum-source.sv",
                ]
            )
            failed = run([args.vvp, output / "failure.vvp"], 1)
            assert "enable must be known" in failed.stdout + failed.stderr
        if name.startswith("tuple_raw_once"):
            witnesses = {}
            for phase, path in (("source_body", payload(unit, "body")), ("verified_final", output / "verified.ac")):
                text_ir = path.read_text()
                extracts = [line for line in text_ir.splitlines() if '"ac.bits.extract"' in line]
                producers = [line for line in text_ir.splitlines() if '"ac.enum.from_bits"' in line]
                assert len(producers) == 1, (phase, producers)
                extracted = re.search(r'"ac.enum.from_bits"\((%[\w]+)\)', producers[0]).group(1)
                raw_extracts = [line for line in extracts if re.match(r"\s*" + re.escape(extracted) + r" =", line)]
                assert len(raw_extracts) == 1 and '"ac.bits.extract"(%arg0)' in raw_extracts[0], (phase, raw_extracts)
                # A legitimate same-width membership boundary may add another
                # extract. Only the raw source expression's dependency is counted.
                input_extracts = [line for line in extracts if '"ac.bits.extract"(%arg0)' in line]
                assert input_extracts == raw_extracts, (phase, input_extracts)
                lhs = producers[0].split("=", 1)[0].strip()
                produced = [lhs.split(":")[0] + "#0", lhs.split(":")[0] + "#1"] if lhs.endswith(":2") else [name.strip() for name in lhs.split(",")]
                uses = text_ir.replace(producers[0], "")
                assert len(produced) == 2 and all(re.search(re.escape(name) + r"(?![\w])", uses) for name in produced), (phase, produced)
                witnesses[phase] = {"path": str(path), "sha256": digest(path), "raw_extract": raw_extracts[0], "producer": producers[0], "results": produced}
            (output / "raw-once-witness.json").write_text(json.dumps(witnesses, indent=2) + "\n")
        return {
            "name": name,
            "unit": str(unit),
            "final": str(final),
            "output": str(output),
            "rows": len(rows),
            "known_rows": sum(known),
            "probe_actions": len(probes),
            "latent_replay_actions_per_worker": len(rows) * 4 if latent else 0,
            "failure_recovery_cases": len(failures),
            "final_sha256": digest(final),
            "runner_sha256": digest(runner),
        }

    return make_vectors, execute
