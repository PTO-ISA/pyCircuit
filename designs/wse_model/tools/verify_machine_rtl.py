"""Execute test-owned programs through one unchanged full-profile RTL binary.

The adapter only loads host requests, drives managed Work/check/commit phases,
and reads public results. Expected bytes and counts stay in machine_cases.py.
No C++ execution result supplies an RTL oracle.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import time
from pathlib import Path
from types import SimpleNamespace

from machine_cases import cases

HOST_FIELDS = (
    ("kind", 3),
    ("node", 6),
    ("address", 32),
    ("wide", 1),
    ("data", 512),
    ("program_count", 16),
    ("landing_permit", 40),
)
RESULT_FIELDS = (
    ("data", 512),
    ("pc", 16),
    ("stage", 5),
    ("selected_node", 6),
    ("offset", 32),
    ("counted", 32),
    ("active", 1),
    ("complete", 1),
    ("pending", 8),
    ("epoch", 8),
    ("fault", 1),
    ("fault_code", 8),
    ("done", 1),
    ("read_bytes", 64),
    ("write_bytes", 64),
)
FULL_PERMIT = (1 << 40) - 1


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def layout(fields: tuple[tuple[str, int], ...]) -> dict[str, tuple[int, int]]:
    low, result = 0, {}
    for name, width in reversed(fields):
        result[name] = (low, width)
        low += width
    return result


def pack_host(**values: int) -> str:
    value = 0
    values = {"landing_permit": FULL_PERMIT, **values}
    for name, width in HOST_FIELDS:
        part = values.get(name, 0)
        if not 0 <= part < 1 << width:
            raise ValueError(f"host field {name} exceeds {width} bits")
        value = (value << width) | part
    return f"{value:0153x}"


def ports_contract(path: Path) -> tuple[dict, dict, dict]:
    ports = json.loads(path.read_text())
    inputs = {port["name"]: port for port in ports["inputs"]}
    outputs = {port["name"]: port for port in ports["outputs"]}
    for port, fields in ((inputs["host"], HOST_FIELDS), (outputs["result"], RESULT_FIELDS)):
        actual = {".".join(row["path"]): (row["low"], row["width"]) for row in port["fields"]}
        if actual != layout(fields) or port["width"] != sum(width for _, width in fields):
            raise ValueError("published ports disagree with the documented machine interface")
    domains = {name: ports["inputs"][index]["wire"] for name, index in ports["domains"].items()}
    return inputs["host"], outputs["result"], domains


def testbench(host: dict, result: dict, domains: dict, root_name: str) -> str:
    """One runtime-file adapter, with no case-specific constants or expectations."""
    declarations, connections = [], []
    for wire, width in (
        (host["wire"], 610),
        (result["wire"], 759),
        (domains["clock"], 1),
        (domains["reset"], 1),
    ):
        declarations.append(f"logic [{width - 1}:0] {wire};")
        connections.append(f".{wire}({wire})")
    available = result.get("availability") or "1'b1"
    if result.get("availability"):
        declarations.append(f"wire {available};")
        connections.append(f".{available}({available})")
    for name, kind in (
        ("pyc_phase", "logic [2:0]"),
        ("pyc_root_commit_ok", "logic"),
        ("pyc_check_complete", "wire"),
        ("pyc_sampling_error", "wire"),
        ("pyc_local_error", "wire"),
    ):
        declarations.append(f"{kind} {name};")
        connections.append(f".{name}({name})")
    aliases = []
    for name, (low, width) in layout(RESULT_FIELDS).items():
        aliases.append(f"wire [{width - 1}:0] observed_{name}=sampled[{low} +: {width}];")
    template = r"""
module tb;
@DECLARATIONS@
@ROOT@ dut(@CONNECTIONS@);
logic [758:0] sampled;
@ALIASES@
string load_path, query_path;
integer load_file, query_file, scanned, instructions, bound, max_cycles;
integer stall_pc=-1, stall_cycles=0, stalled=0, execution_cycles=0;
bit stall_started=0, withdrawing=0, raw_probe=0;
logic [609:0] command, probe_request;
logic [39:0] permits;
task work_commit;
begin
  pyc_root_commit_ok=0;pyc_phase=1;#1;
  if(pyc_check_complete!==1'b1 || pyc_sampling_error!==1'b0 || pyc_local_error!==1'b0)begin
    $display("CHECK_FAILURE cycle=%0d pc=%0d stage=%0d",execution_cycles,observed_pc,observed_stage);
    pyc_phase=3;#1;$fatal(1,"machine failed before commit");
  end
  if(@AVAILABLE@!==1'b1 || $isunknown(@RESULT@))begin
    pyc_phase=3;#1;$fatal(1,"unavailable or unknown machine result");
  end
  sampled=@RESULT@;
  pyc_root_commit_ok=1;pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
end
endtask
task cycle(input logic [609:0] request);
begin
  @HOST@=request;@CLOCK@=0;#1;work_commit();
  @CLOCK@=1;#1;work_commit();
end
endtask
initial begin
  if(!$value$plusargs("load=%s",load_path) || !$value$plusargs("query=%s",query_path) ||
     !$value$plusargs("instructions=%d",instructions) || !$value$plusargs("max_cycles=%d",max_cycles))
    $fatal(1,"missing runtime input arguments");
  scanned=$value$plusargs("stall_pc=%d",stall_pc);
  scanned=$value$plusargs("stall_cycles=%d",stall_cycles);
  raw_probe=$value$plusargs("probe=%h",probe_request);
  @HOST@=0;@CLOCK@=0;@RESET@=1;pyc_phase=0;pyc_root_commit_ok=0;
  #1; // Settle generated initial-value nets before native Reset preparation.
  pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
  pyc_phase=0;pyc_root_commit_ok=0;@RESET@=0;#1;
  load_file=$fopen(load_path,"r");if(!load_file)$fatal(1,"cannot open load file");
  while(!$feof(load_file))begin
    scanned=$fscanf(load_file,"%h",command);
    if(scanned==1)begin cycle(command);if(observed_fault)$fatal(1,"input loading fault");end
    else if(!$feof(load_file))$fatal(1,"malformed load record");
  end
  $fclose(load_file);
  if(raw_probe)begin
    cycle(probe_request);
    if(!observed_fault)$fatal(1,"invalid raw host request did not set fault");
    cycle({3'd0,6'd0,32'd0,1'b0,512'd0,16'd0,40'hffffffffff});
    if(!observed_fault)$fatal(1,"ordinary host fault was not sticky");
  end else begin
    cycle({3'd3,6'd0,32'd0,1'b0,512'd0, instructions[15:0],40'hffffffffff});
  end
  for(bound=0;bound<max_cycles && !observed_done && !observed_fault;bound=bound+1)begin
    permits=40'hffffffffff;
    if(!stall_started && observed_pc==stall_pc && observed_stage==5 && observed_offset==8)
      stall_started=1;
    withdrawing=stall_started && stalled<stall_cycles;
    if(withdrawing)begin
      permits[33]=0;
      $display("STALL %0d %0d %0d %0d %0d %0d %0d",observed_pc,observed_stage,
        observed_selected_node,observed_offset,observed_read_bytes,observed_write_bytes,observed_counted);
      stalled=stalled+1;
    end
    cycle({3'd0,6'd33,32'd0,1'b0,512'd0,16'd0,permits});
    execution_cycles=execution_cycles+1;
  end
  if(!observed_done && !observed_fault)$fatal(1,"execution cycle bound exhausted");
  if(stalled!=stall_cycles)$fatal(1,"requested permit withdrawal was not reached");
  $display("TERMINAL %0d %0d %0d %0d %0d %0d %0d",execution_cycles,observed_done,
    observed_fault,observed_fault_code,observed_pc,observed_read_bytes,observed_write_bytes);
  query_file=$fopen(query_path,"r");if(!query_file)$fatal(1,"cannot open query file");
  while(!$feof(query_file))begin
    scanned=$fscanf(query_file,"%h",command);
    if(scanned==1)begin
      cycle(command);
      if(raw_probe && !observed_fault)$fatal(1,"fault lost during diagnostic readback");
      if(command[609:607]==4)$display("READ %h",observed_data);
      else if(command[609:607]==5)$display("ACCOUNT %0d %0d %0d %0d %0d %0d",
        command[606:601],observed_counted,observed_active,observed_complete,observed_pending,observed_epoch);
      else $fatal(1,"unsupported observation request");
    end else if(!$feof(query_file))$fatal(1,"malformed query record");
  end
  $fclose(query_file);$display("FINISHED");$finish;
end
endmodule
"""
    replacements = {
        "DECLARATIONS": "\n".join(declarations),
        "CONNECTIONS": ",".join(connections),
        "ROOT": root_name,
        "ALIASES": "\n".join(aliases),
        "AVAILABLE": available,
        "RESULT": result["wire"],
        "HOST": host["wire"],
        "CLOCK": domains["clock"],
        "RESET": domains["reset"],
    }
    for name, value in replacements.items():
        template = template.replace("@" + name + "@", value)
    return template


def inputs_for(case, directory: Path) -> dict[str, str]:
    directory.mkdir(parents=True, exist_ok=False)
    loads, queries = [], []
    for patch in case.patches:
        for offset in range(0, len(patch.data), 8):
            loads.append(
                pack_host(
                    kind=1,
                    node=patch.node,
                    address=patch.address + offset,
                    data=int.from_bytes(patch.data[offset : offset + 8], "little"),
                )
            )
    binary = case.program.pack()
    for offset in range(0, len(binary), 8):
        loads.append(
            pack_host(
                kind=2, address=offset, data=int.from_bytes(binary[offset : offset + 8], "little")
            )
        )
    for region in case.readback:
        for offset in range(0, region.nbytes, 8):
            queries.append(pack_host(kind=4, node=region.node, address=region.address + offset))
        queries.append(pack_host(kind=5, node=region.node))
    (directory / "load.hex").write_text("\n".join(loads) + "\n")
    (directory / "query.hex").write_text("\n".join(queries) + "\n")
    (directory / "program.bin").write_bytes(binary)
    (directory / "program.json").write_text(json.dumps(case.program.manifest(), indent=2) + "\n")
    return {name: digest(directory / name) for name in ("load.hex", "query.hex", "program.bin")}


def check_output(case, path: Path, directory: Path) -> dict:
    actual, accounts, stalls, terminals = bytearray(), [], [], []
    lines = path.read_text().splitlines()
    if lines.count("FINISHED") != 1:
        raise AssertionError("RTL did not finish all requested observations")
    for line in lines:
        fields = line.split()
        if not fields:
            continue
        if fields[0] == "READ":
            actual += int(fields[1], 16).to_bytes(8, "little")
        elif fields[0] == "ACCOUNT":
            accounts.append(
                dict(
                    zip(
                        ("node", "counted", "active", "complete", "pending", "epoch"),
                        map(int, fields[1:]),
                        strict=True,
                    )
                )
            )
        elif fields[0] == "STALL":
            stalls.append(
                dict(
                    zip(
                        (
                            "pc",
                            "stage",
                            "selected_node",
                            "offset",
                            "read_bytes",
                            "write_bytes",
                            "counted",
                        ),
                        map(int, fields[1:]),
                        strict=True,
                    )
                )
            )
        elif fields[0] == "TERMINAL":
            terminals.append(
                dict(
                    zip(
                        (
                            "execution_cycles",
                            "done",
                            "fault",
                            "fault_code",
                            "pc",
                            "read_bytes",
                            "write_bytes",
                        ),
                        map(int, fields[1:]),
                        strict=True,
                    )
                )
            )
    if len(terminals) != 1:
        raise AssertionError("expected one terminal machine observation")
    terminal = terminals[0]
    if terminal["fault"] != int(case.expect_fault) or terminal["done"] != int(
        not case.expect_fault
    ):
        raise AssertionError("machine completion/fault differs from the scenario")
    if case.expect_fault and terminal["write_bytes"] != 0:
        raise AssertionError("range rejection wrote payload before preflight completed")
    if bytes(actual) != case.expected_bytes:
        raise AssertionError(f"{case.name}: real RTL memory differs from independent byte oracle")
    for node, expected in case.expected_counts.items():
        observed = [account for account in accounts if account["node"] == node]
        if not observed or any(account["counted"] != expected for account in observed):
            raise AssertionError(f"{case.name}: node {node} count differs from {expected}")
    (directory / ("output.fault.bin" if case.expect_fault else "output.bin")).write_bytes(actual)
    return {
        "terminal": terminal,
        "accounts": accounts,
        "stall_observations": stalls,
        "output_sha256": hashlib.sha256(actual).hexdigest(),
        "output_bytes": len(actual),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build", type=Path, required=True, help="Shared final.ac/verilog build folder"
    )
    parser.add_argument(
        "--toolchain", type=Path, default=Path(os.environ.get("PYC_TOOLCHAIN_ROOT", "."))
    )
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--timeout", type=int, default=600, help="Seconds per compile/run stage")
    parser.add_argument("--max-cycles", type=int, default=10000)
    args = parser.parse_args()
    build, output, toolchain = args.build.resolve(), args.output.resolve(), args.toolchain.resolve()
    output.mkdir(parents=True, exist_ok=False)
    receipt = {
        "status": "INCOMPLETE",
        "commands": [],
        "runs": [],
        "conditions": {
            "nodes": 40,
            "seen_slots": 56,
            "early_slots": 2,
            "bank_bytes": 65536,
            "beat_bytes": 64,
            "narrow_beat_bytes": 8,
            "host_interface_version": 2,
            "max_cycles": args.max_cycles,
            "timeout_seconds": args.timeout,
            "backend": "iverilog",
        },
    }

    def save():
        (output / "verification.json").write_text(json.dumps(receipt, indent=2) + "\n")

    def run(label, command):
        record = {
            "label": label,
            "command": list(map(str, command)),
            "stdout": str(output / f"{label}.stdout.log"),
            "stderr": str(output / f"{label}.stderr.log"),
            "exit_code": None,
        }
        receipt["commands"].append(record)
        save()
        started = time.monotonic()
        try:
            with (
                Path(record["stdout"]).open("w") as stdout,
                Path(record["stderr"]).open("w") as stderr,
            ):
                record["exit_code"] = subprocess.run(
                    record["command"],
                    stdout=stdout,
                    stderr=stderr,
                    timeout=args.timeout,
                    check=False,
                ).returncode
        except subprocess.TimeoutExpired:
            record["error"] = "TIMEOUT"
            raise
        finally:
            record["seconds"] = time.monotonic() - started
            save()
        if record["exit_code"] != 0:
            raise RuntimeError(
                f"{label} failed with exit {record['exit_code']}; see {record['stderr']}"
            )
        return Path(record["stdout"])

    save()
    try:
        build_receipt = json.loads((build / "build.json").read_text())
        required = {
            "nodes": 40,
            "bank_bytes": 65536,
            "beat_bytes": 64,
            "narrow_beat_bytes": 8,
            "host_interface_version": 2,
            "early_slots": 2,
            "seen_slots": 56,
            "max_commands": 1024,
        }
        if build_receipt["profile"] != required:
            raise ValueError("verification requires the complete 40-node/56-identity-slot profile")
        current_metadata = json.loads(
            (toolchain / "share/pycircuit/toolchain-metadata.json").read_text()
        )
        if current_metadata != build_receipt["toolchain"]:
            raise ValueError(
                "runtime toolchain metadata differs from the recorded compiler toolchain"
            )
        source_paths = [Path(path) for path in build_receipt["sources"]]
        if {str(path): digest(path) for path in source_paths} != build_receipt["sources"]:
            raise ValueError("hardware source changed since the recorded build")
        final = build / "final.ac"
        if digest(final) != build_receipt["final_sha256"]:
            raise ValueError("final artifact differs from build receipt")
        generated = build / "verilog"
        metadata = json.loads((generated / "generated.json").read_text())
        if "wse_machine.machine.WseMachine" not in metadata["entry"]["definition"]:
            raise ValueError("unexpected RTL entry")
        if metadata.get("rtl_mode") != "managed":
            raise ValueError("the test adapter requires the checked managed RTL lifecycle")
        host, result, domains = ports_contract(generated / "ports.json")
        bench = output / "runtime_tb.sv"
        bench.write_text(testbench(host, result, domains, metadata["root_rtl_name"]))
        rtl = [generated / row["path"] for row in metadata["files"] if row["role"] == "rtl"]
        rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
        primitives = sorted((toolchain / "include/verilog").glob("*.v"))
        if not primitives:
            raise ValueError("pinned runtime Verilog primitives are missing")
        tracked = [
            final,
            generated / "ports.json",
            generated / "generated.json",
            bench,
            *rtl,
            *primitives,
            *source_paths,
        ]
        tracked += [
            Path(__file__).resolve(),
            Path(__file__).with_name("machine_cases.py").resolve(),
        ]
        before = {str(path): digest(path) for path in tracked}
        receipt["artifacts"] = before
        receipt["toolchain"] = build_receipt["toolchain"]
        receipt["sources"] = build_receipt["sources"]
        receipt["script_sha256"] = digest(Path(__file__))
        compiler, simulator = shutil.which("iverilog"), shutil.which("vvp")
        if not compiler or not simulator:
            raise ValueError("Icarus Verilog and vvp are required")
        binary = output / "machine.vvp"
        run("compile", [compiler, "-g2012", "-s", "tb", "-o", binary, *rtl, *primitives, bench])
        binary_hash = digest(binary)
        receipt["binary_sha256"] = binary_hash
        selected = cases()
        baseline = None
        baseline_inputs = None
        for name in (*selected, "tree-stalled"):
            stalled = name == "tree-stalled"
            case = selected["tree-copy-duplicate"] if stalled else selected[name]
            directory = output / name
            input_hashes = inputs_for(case, directory)
            command = [
                simulator,
                binary,
                f"+load={directory / 'load.hex'}",
                f"+query={directory / 'query.hex'}",
                f"+instructions={len(case.program.commands)}",
                f"+max_cycles={args.max_cycles}",
            ]
            if stalled:
                command += [f"+stall_pc={case.stall_copy_pc}", "+stall_cycles=3"]
            log = run(name, command)
            observed = check_output(case, log, directory)
            if stalled:
                if (
                    baseline is None
                    or observed["terminal"]["execution_cycles"]
                    != baseline["terminal"]["execution_cycles"] + 3
                ):
                    raise AssertionError(
                        "three withdrawn permit cycles must add exactly three cycles"
                    )
                snapshots = observed["stall_observations"]
                if len(snapshots) != 3 or any(row != snapshots[0] for row in snapshots):
                    raise AssertionError(
                        "blocked write changed PC, target, offset, traffic or count"
                    )
                if input_hashes != baseline_inputs:
                    raise AssertionError("permit test changed program or initialized bytes")
                if observed["output_sha256"] != baseline["output_sha256"]:
                    raise AssertionError("permit withdrawal changed final bytes")
            if name == "tree-copy-duplicate":
                baseline = observed
                baseline_inputs = input_hashes
            if input_hashes != {file: digest(directory / file) for file in input_hashes}:
                raise AssertionError("runtime inputs changed during execution")
            if digest(binary) != binary_hash:
                raise AssertionError("cases did not use the same compiled RTL binary")
            receipt["runs"].append(
                {
                    "case": name,
                    "status": "PASS",
                    "inputs": input_hashes,
                    "binary_sha256": binary_hash,
                    **observed,
                }
            )
            save()
            print(name + ": PASS", flush=True)
        raw_probes = {
            "host-node40-read": pack_host(kind=4, node=40),
            "host-node40-write": pack_host(kind=1, node=40, data=0x1021324354657687),
            "host-read-unaligned": pack_host(kind=4, address=1),
            "host-kind6": pack_host(kind=6),
        }
        for name, request in raw_probes.items():
            directory = output / name
            directory.mkdir()
            load_file, query_file = directory / "load.hex", directory / "query.hex"
            load_file.write_text(pack_host(kind=1, data=0xCCCCCCCCCCCCCCCC) + "\n")
            query_file.write_text(pack_host(kind=4) + "\n" + pack_host(kind=5) + "\n")
            (directory / "probe.hex").write_text(request + "\n")
            input_hashes = {
                file.name: digest(file) for file in (load_file, query_file, directory / "probe.hex")
            }
            log = run(
                name,
                [
                    simulator,
                    binary,
                    f"+load={load_file}",
                    f"+query={query_file}",
                    "+instructions=0",
                    f"+max_cycles={args.max_cycles}",
                    f"+probe={request}",
                ],
            )
            case = SimpleNamespace(
                name=name, expect_fault=True, expected_bytes=b"\xcc" * 8, expected_counts={0: 0}
            )
            observed = check_output(case, log, directory)
            if digest(binary) != binary_hash or input_hashes != {
                file: digest(directory / file) for file in input_hashes
            }:
                raise AssertionError("raw probe binary or input changed during execution")
            receipt["runs"].append(
                {
                    "case": name,
                    "status": "PASS",
                    "inputs": input_hashes,
                    "binary_sha256": binary_hash,
                    **observed,
                }
            )
            save()
            print(name + ": PASS", flush=True)
        if {str(path): digest(path) for path in tracked} != before:
            raise AssertionError(
                "DUT, final artifact, primitives or testbench changed during execution"
            )
        receipt["status"] = "PASS"
    except Exception as error:
        receipt["status"] = "FAIL"
        receipt["error"] = str(error)
        raise
    finally:
        save()


if __name__ == "__main__":
    main()
