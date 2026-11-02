"""Typed input adapter for a native-link-generated request/response view.

Only marshals inputs, honors the generated handshake, samples public outputs
and drives the existing lifecycle. No worker computation or model state lives
here. Numerical and timing oracles belong to the caller's tests.
"""

from __future__ import annotations

import hashlib
import json
import re
import shutil
import sys
import time
from pathlib import Path

from ._driver import _DriverError
from ._emit import _cmake_quote


def _write(path: Path, text: str) -> None:
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def read_requests(path: Path) -> list[dict]:
    rows = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(rows, list) or not rows:
        raise _DriverError("model inputs require a nonempty JSON list")
    if any(type(row) is not dict or set(row) != {"active", "request"} for row in rows):
        raise _DriverError("each input record requires exactly active and request")
    return rows


def _bits(value, width: int) -> str:
    if isinstance(value, int):
        if value < 0 or value >= 1 << width:
            raise _DriverError("input integer does not fit its declared Bits width")
        return format(value, f"0{width}b")
    if isinstance(value, str):
        if value.startswith("0x"):
            return _bits(int(value, 16), width)
        value = value.removeprefix("0b").lower()
        if len(value) == width and not set(value) - set("01xz"):
            return value
    raise _DriverError("input requires width-exact raw Bits, including explicit X/Z")


def _pack(rows, request):
    fields = request["fields"]
    if any(len(field["path"]) != 1 for field in fields):
        raise _DriverError("execution input protocol requires flat Bits records")
    keys = {field["path"][0] for field in fields}
    packed = []
    for index, row in enumerate(rows):
        try:
            active = _bits(row["active"], 1)
        except _DriverError as error:
            raise _DriverError(f"inputs[{index}].active: {error}") from error
        payload = row["request"]
        if type(payload) is dict:
            if set(payload) != keys:
                raise _DriverError(
                    "input fields differ from the declared request record"
                )
            parts = ["x"] * request["width"]
            for field in fields:
                try:
                    value = _bits(payload[field["path"][0]], field["width"])
                except _DriverError as error:
                    raise _DriverError(
                        f"inputs[{index}].request.{field['path'][0]}: {error}"
                    ) from error
                start = request["width"] - field["low"] - field["width"]
                parts[start : start + field["width"]] = value
            payload = "".join(parts)
        else:
            payload = _bits(payload, request["width"])
        packed.append(active + payload)
    return packed


def _protocol(receipt, ports, strategy):
    descriptor = receipt.get("execution_model")
    if (
        type(descriptor) is not dict
        or descriptor.get("strategy") != strategy
        or descriptor.get("protocol") != "request_response_v1"
    ):
        raise _DriverError("inputs require a native-generated execution model entry")
    inputs = {row["name"]: row for row in ports["inputs"]}
    domains = {key: ports["inputs"][value] for key, value in ports["domains"].items()}
    data = set(inputs) - {row["name"] for row in domains.values()}
    expected = {"request", "active"} | ({"consume"} if strategy == "timing" else set())
    outputs = {row["name"]: row for row in ports["outputs"]}
    if (
        data != expected
        or not {"ack", "completion"} <= set(outputs)
        or any(
            not name.startswith("snapshot_")
            for name in set(outputs) - {"ack", "completion"}
        )
    ):
        raise _DriverError(
            "generated model entry does not implement the declared protocol"
        )
    for name in ["active", *(["consume"] if strategy == "timing" else [])]:
        if inputs[name]["width"] != 1:
            raise _DriverError("model handshake controls require one bit")
    for row in [*inputs.values(), *outputs.values()]:
        if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_$]*", row["wire"]) is None:
            raise _DriverError("invalid published model port identifier")
    return inputs, outputs, domains


def _cpp_adapter(inputs, outputs, domains, strategy):
    request, active = inputs["request"]["wire"], inputs["active"]["wire"]
    clock, reset = domains["clock"]["wire"], domains["reset"]["wire"]
    ack = outputs["ack"]
    available = (
        f"control(high.{ack['availability']})" if ack.get("availability") else "true"
    )
    consume_high = (
        f"input.{inputs['consume']['wire']}=flag(cycle>=consumer);"
        if strategy == "timing"
        else ""
    )
    consume_low = (
        f"input.{inputs['consume']['wire']}=flag(false);"
        if strategy == "timing"
        else ""
    )
    reports = []
    for name, port in outputs.items():
        sample = "low" if name.startswith("snapshot_") else "high"
        valid = (
            f"control({sample}.{port['availability']})"
            if port.get("availability")
            else "true"
        )
        reports.append(f'field({json.dumps(name)}, {sample}.{port["wire"]}, {valid});')
    text = r"""
#include "pycircuit_system.hpp"
#include <chrono>
#include <string_view>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
auto flag(bool x){return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{x});}
template<class Wire> Wire decode(const std::string& text){
  if(text.size()!=Wire::width)throw std::runtime_error("input width");
  using Mask=typename Wire::packed_type::Mask;
  Mask value{0},known{0},z{0};
  for(unsigned i=0;i<Wire::width;++i){
    char c=text[Wire::width-1-i];auto bit=uint64_t{1}<<(i%64);
    if(c=='1')value.setWord(i/64,value.word(i/64)|bit);
    if(c=='0'||c=='1')known.setWord(i/64,known.word(i/64)|bit);
    else if(c=='z')z.setWord(i/64,z.word(i/64)|bit);
    else if(c!='x')throw std::runtime_error("input symbol");
  }
  return Wire::fromPacked(Wire::packed_type::fromMasks(value,known,z));
}
template<class Wire> std::string bits(const Wire& wire){
  auto p=wire.packed();std::string result;
  for(unsigned i=Wire::width;i;--i){unsigned n=i-1;auto bit=uint64_t{1}<<(n%64);
    result+=p.knownMask().word(n/64)&bit?(p.value().word(n/64)&bit?'1':'0'):
            (p.zMask().word(n/64)&bit?'z':'x');}
  return result;
}
template<class Wire> bool control(const Wire& wire){
  if(!wire.isFullyKnown())throw std::runtime_error("unknown output availability");
  return wire.value().toBool();
}
bool first, summary=false;
uint64_t fingerprint=14695981039346656037ULL, completions=0;
std::vector<std::string> expected_packets;
template<class Wire> void field(const char* name,const Wire& wire,bool available){
  if(summary){
    if(std::string_view(name)=="completion" && available){
      auto raw=bits(wire);
      if(completions>=expected_packets.size() || raw!=expected_packets[completions])
        throw std::runtime_error("benchmark completion differs from independent expected raw packet");
      for(unsigned char c:raw){fingerprint^=c;fingerprint*=1099511628211ULL;}
      fingerprint^='|';fingerprint*=1099511628211ULL;++completions;
    }
    return;
  }
  if(!first)std::cout<<',';first=false;
  std::cout<<'"'<<name<<"\":{\"available\":"<<(available?"true":"false")<<",\"bits\":";
  if(available)std::cout<<'"'<<bits(wire)<<'"';else std::cout<<"null";
  std::cout<<'}';
}
int main(int argc,char**argv){try{
  if(argc!=6 && argc!=9)throw std::runtime_error("model input arguments");
  summary=argc==9 && std::string(argv[7])=="summary";
  uint64_t batches=summary?std::stoull(argv[6]):1;
  if(!batches)throw std::runtime_error("positive batches required");
  if(summary){std::ifstream golden(argv[8]);std::string packet;
    while(golden>>packet)expected_packets.push_back(packet);}
  std::ifstream stream(argv[1]);std::vector<std::string> rows;std::string row;
  while(stream>>row)rows.push_back(row);
  auto cycles=std::stoull(argv[2]),consumer=std::stoull(argv[3]),workers=std::stoull(argv[4]);
  bool functional=std::string(argv[5])=="functional";
  for(uint64_t batch=0;batch<batches;++batch){
  fingerprint=14695981039346656037ULL;completions=0;
  pyc_dut dut(workers);pyc_dut::Inputs input{};
  input.@ACTIVE@=flag(false);input.@CLOCK@=flag(false);input.@RESET@=flag(true);
  @CONSUME_LOW@
  dut.drive(input);dut.system().Build();dut.system().Reset();input.@RESET@=flag(false);
  std::size_t cursor=0;pyc_dut::Outputs final{};
  auto started=std::chrono::steady_clock::now();
  for(uint64_t cycle=0;cycle<cycles;++cycle){
    bool present=cursor<rows.size();
    input.@REQUEST@=present?decode<decltype(input.@REQUEST@)>(rows[cursor].substr(1)):
                              decltype(input.@REQUEST@)::unknown();
    input.@ACTIVE@=present?decode<decltype(input.@ACTIVE@)>(rows[cursor].substr(0,1)):flag(false);
    input.@CLOCK@=flag(true);@CONSUME_HIGH@
    dut.drive(input);
    if(dut.system().Step()!=gfsim::SimStepResult::Running)throw std::runtime_error("model high sample failed");
    auto high=dut.sample();
    bool acknowledged=@ACK_AVAILABLE@ && control(high.@ACK@);
    bool inactive=present&&rows[cursor][0]=='0';
    if(functional && present && !inactive && !acknowledged)
      throw std::runtime_error("functional entry failed to acknowledge a logical offer");
    if(functional||acknowledged||inactive){if(present)++cursor;input.@ACTIVE@=flag(false);
       input.@REQUEST@=decltype(input.@REQUEST@)::unknown();}
    input.@CLOCK@=flag(false);@CONSUME_LOW@
    dut.drive(input);
    if(dut.system().Step()!=gfsim::SimStepResult::Running)throw std::runtime_error("model low sample failed");
    auto low=dut.sample();
    if(!summary)std::cout<<"{\"cycle\":"<<cycle<<",\"outputs\":{";first=true;
    @REPORTS@
    if(!summary)std::cout<<"}}\n";final=low;
  }
  auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-started).count();
  if(cursor!=rows.size())throw std::runtime_error("requests remain unaccepted at the timing cycle limit");
  if(summary){
    if(completions!=expected_packets.size())throw std::runtime_error("benchmark completion count mismatch");
    std::cout<<"{\"loop_ns\":"<<ns<<",\"completions\":"<<completions<<",\"fingerprint\":\""<<fingerprint<<"\",\"final_state\":[";
    @FINAL_STATE@
    std::cout<<"]}\n";
  }
  }
  return 0;
}catch(const std::exception& e){std::cerr<<"model run: "<<e.what()<<'\n';return 1;}}
"""
    for key, value in {
        "REQUEST": request,
        "ACTIVE": active,
        "CLOCK": clock,
        "RESET": reset,
        "CONSUME_HIGH": consume_high,
        "CONSUME_LOW": consume_low,
        "ACK_AVAILABLE": available,
        "ACK": ack["wire"],
        "REPORTS": "\n".join(reports),
        "FINAL_STATE": "\n".join(
            ("std::cout<<char(44);" if n else "")
            + f'std::cout<<char(34)<<bits(final.{port["wire"]})<<char(34);'
            for n, port in enumerate(
                port for name, port in outputs.items() if name.startswith("snapshot_")
            )
        ),
    }.items():
        text = text.replace("@" + key + "@", value)
    return text


def _rtl_adapter(
    inputs, outputs, domains, strategy, rows, cycles, consumer, vector_file, root
):
    declarations, connections, seen = [], [], set()
    for row in [*inputs.values(), *outputs.values()]:
        wire = row["wire"]
        if wire not in seen:
            declarations.append(f"logic [{row['width']-1}:0] {wire};")
            connections.append(f".{wire}({wire})")
            seen.add(wire)
        valid = row.get("availability")
        if valid and valid not in seen:
            declarations.append(f"wire {valid};")
            connections.append(f".{valid}({valid})")
            seen.add(valid)
    controls = {
        "pyc_phase": ("logic [2:0]", "0"),
        "pyc_root_commit_ok": ("logic", "0"),
        "pyc_check_complete": ("wire", ""),
        "pyc_sampling_error": ("wire", ""),
        "pyc_local_error": ("wire", ""),
    }
    for name, (kind, initial) in controls.items():
        declarations.append(f"{kind} {name}" + (f"={initial}" if initial else "") + ";")
        connections.append(f".{name}({name})")
    # Values and availability are sampled before the commit barrier. Snapshot
    # outputs are sampled at the following low-clock Work, after the rising commit.
    capture, reports = [], []
    for name, port in outputs.items():
        if name.startswith("snapshot_"):
            value = port["wire"]
            available = port.get("availability") or "1'b1"
        else:
            value, available = "saved_" + name, "available_" + name
            declarations += [
                f"logic [{port['width']-1}:0] {value};",
                f"logic {available};",
            ]
            availability = port.get("availability") or "1'b1"
            capture += [f"{value}={port['wire']};", f"{available}={availability};"]
        reports.append((name, value, available))
    request, active = inputs["request"]["wire"], inputs["active"]["wire"]
    clock, reset = domains["clock"]["wire"], domains["reset"]["wire"]
    consume_high = (
        f"{inputs['consume']['wire']}=(cycle>={consumer});"
        if strategy == "timing"
        else ""
    )
    consume_low = f"{inputs['consume']['wire']}=0;" if strategy == "timing" else ""
    printing = []
    for n, (name, value, available) in enumerate(reports):
        prefix = "" if n == 0 else ","
        printing.append(f'$write("{prefix}\\"{name}\\":{{\\"available\\":");')
        printing.append(
            f"""if({available}===1'b1)$write("true,\\\"bits\\\":\\\"%b\\\"}}",{value});
else if({available}===1'b0)$write("false,\\\"bits\\\":null}}");
else $fatal(1,"unknown model availability");"""
        )
    return f"""module tb;
{chr(10).join(declarations)}
{root} dut({','.join(connections)});
logic [{inputs['request']['width']}:0] rows[0:{len(rows)-1}];
integer cursor=0;
task prepare;
  pyc_root_commit_ok=0;pyc_phase=1;#1;
  if(pyc_check_complete!==1'b1 || pyc_sampling_error!==1'b0 || pyc_local_error!==1'b0)begin
    pyc_phase=3;#1;$fatal(1,"model sample failed before commit");end
endtask
task commit;
  pyc_root_commit_ok=1;pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
endtask
initial begin
  $readmemb({json.dumps(str(vector_file))},rows);
  {request}='x;{active}=0;{clock}=0;{reset}=1;{consume_low}
  pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;
  pyc_phase=0;pyc_root_commit_ok=0;{reset}=0;#1;
  for(integer cycle=0;cycle<{cycles};cycle=cycle+1)begin
    if(cursor<{len(rows)})begin {active}=rows[cursor][{inputs['request']['width']}];
      {request}=rows[cursor][{inputs['request']['width']-1}:0];end
    else begin {active}=0;{request}='x;end
    {clock}=1;{consume_high}#1;prepare();
    {''.join(capture)}commit();
    if({1 if strategy == 'functional' else 0} && {active}===1'b1 &&
       !(available_ack===1'b1 && saved_ack===1'b1))$fatal(1,"functional offer not acknowledged");
    if({1 if strategy == 'functional' else 0} ||
       (available_ack===1'b1 && saved_ack===1'b1) || {active}===1'b0)begin
      if(cursor<{len(rows)})cursor=cursor+1;{active}=0;{request}='x;end
    {clock}=0;{consume_low}#1;prepare();
    $write("{{\\\"cycle\\\":%0d,\\\"outputs\\\":{{",cycle);
    {chr(10).join(printing)}
    $display("}}}}");commit();
  end
  if(cursor!={len(rows)})$fatal(1,"requests remain unaccepted at the timing cycle limit");
  $finish;
end
endmodule
"""


def _successful_records(trace: Path) -> int:
    count = 0
    if trace.exists():
        for line in trace.read_text().splitlines():
            if line.startswith('{"cycle":'):
                try:
                    row = json.loads(line)
                except json.JSONDecodeError:
                    continue
                if row.get("cycle") == count and isinstance(row.get("outputs"), dict):
                    count += 1
    return count


def run_requests(
    *,
    generated,
    build,
    installed,
    receipt,
    rows,
    strategy,
    target,
    cycles,
    consumer_start,
    workers,
    timeout,
    env,
    invoke,
    build_metrics=None,
):
    build_metrics = {} if build_metrics is None else dict(build_metrics)
    ports = json.loads((generated / "ports.json").read_text())
    inputs, outputs, domains = _protocol(receipt, ports, strategy)
    packed = _pack(rows, inputs["request"])
    steps = (
        len(rows) if strategy == "functional" else (10 if cycles is None else cycles)
    )
    directory = build / "model-inputs"
    directory.mkdir(exist_ok=True)
    vectors = directory / "requests.mem"
    _write(vectors, "\n".join(packed) + "\n")
    executable = directory / "model_input"
    backend = "cpp" if target == "cpp" else "iverilog"
    if target == "cpp":
        _write(directory / "main.cpp", _cpp_adapter(inputs, outputs, domains, strategy))
        _write(
            directory / "CMakeLists.txt",
            "cmake_minimum_required(VERSION 3.25)\nproject(ModelInput LANGUAGES CXX)\n"
            + f"add_subdirectory({_cmake_quote(str(generated))} generated)\n"
            + "add_executable(model_input main.cpp)\ntarget_compile_features(model_input PRIVATE cxx_std_20)\n"
            + "target_link_libraries(model_input PRIVATE pycircuit_modules)\n",
        )
        native = directory / "build"
        configured_at = time.perf_counter()
        invoke(
            [
                "cmake",
                "-S",
                str(directory),
                "-B",
                str(native),
                "-G",
                "Ninja",
                "-DCMAKE_BUILD_TYPE=Release",
                f"-Dpycircuit_DIR={installed / 'share/pycircuit/cmake'}",
            ],
            timeout=timeout,
            env=env,
        )
        build_metrics["native_configure_seconds"] = time.perf_counter() - configured_at
        built_at = time.perf_counter()
        invoke(
            [
                "cmake",
                "--build",
                str(native),
                "--target",
                "model_input",
                "--parallel",
                "2",
            ],
            timeout=timeout,
            env=env,
        )
        build_metrics["native_cpp_compile_seconds"] = time.perf_counter() - built_at
        executable = native / "model_input"
        command = [
            str(executable),
            str(vectors),
            str(steps),
            str(consumer_start),
            str(workers),
            strategy,
        ]
    else:
        tools = [
            shutil.which(name, path=env.get("PATH")) for name in ("iverilog", "vvp")
        ]
        if not all(tools):
            raise _DriverError("model input RTL execution requires Icarus and vvp")
        bench = directory / "input_tb.sv"
        _write(
            bench,
            _rtl_adapter(
                inputs,
                outputs,
                domains,
                strategy,
                packed,
                steps,
                consumer_start,
                vectors,
                receipt["root_rtl_name"],
            ),
        )
        files = [
            generated / row["path"] for row in receipt["files"] if row["role"] == "rtl"
        ]
        files.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
        files += sorted((installed / "include/verilog").glob("*.v"))
        invoke(
            [
                tools[0],
                "-g2012",
                "-s",
                "tb",
                "-o",
                str(executable),
                *map(str, files),
                str(bench),
            ],
            timeout=timeout,
            env=env,
        )
        command = [tools[1], str(executable)]
    trace = build / "model-trace.jsonl"
    status = "failed"
    lines = []
    try:
        with trace.open("w") as stream:
            invoke(command, timeout=timeout, env=env, stdout=stream)
        lines = [
            line
            for line in trace.read_text().splitlines()
            if line.startswith('{"cycle":')
        ]
        if len(lines) != steps or _successful_records(trace) != steps:
            raise _DriverError(
                "model execution did not publish every requested sampling record"
            )
        status = "success"
        for line in lines:
            sys.stdout.write(line + "\n")
    finally:
        observed = _successful_records(trace)
        (build / "run-execution.json").write_text(
            json.dumps(
                {
                    "schema": "pycircuit-model-run-v2",
                    "status": status,
                    "backend": backend,
                    "build_metrics": build_metrics,
                    "native_build_type": "Release" if target == "cpp" else None,
                    "execution_model": receipt["execution_model"],
                    "rising_samples": observed,
                    "sampling_epochs": 2 * observed,
                    "requested_rising_samples": steps,
                    "input_records": len(rows),
                    "trace": str(trace),
                    "input_snapshot": hashlib.sha256(vectors.read_bytes()).hexdigest(),
                    "validation": "execution_only; numerical and timing expectations belong to independent tests",
                },
                indent=2,
            )
            + "\n"
        )
