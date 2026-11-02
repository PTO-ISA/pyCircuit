"""Typed bridge for checked finite clocked models, independent of any ISA.

Only raw ports, normal lifecycle and complete-state equality live here.
Memory/event policies are explicit adapters, never instruction semantics.
"""

from __future__ import annotations

import json
import os
import re
import select
import subprocess
from pathlib import Path

from ._driver import _DriverError
from ._emit import _cmake_quote
from ._run import _invoke


def write_changed(path, text):
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def bridge_source(ports, state_output):
    domain = ports["domains"]
    data_inputs = [p for p in ports["inputs"] if p["ordinal"] not in domain.values()]
    outputs = ports["outputs"]
    if len(data_inputs) != 1 or len(outputs) != 2:
        raise _DriverError(
            "clocked bridge needs one record input, level/event output and complete state output"
        )
    incoming = data_inputs[0]
    state = outputs[state_output]
    packet = outputs[1 - state_output]
    clock, reset = [
        ports["inputs"][domain[name]]["wire"] for name in ("clock", "reset")
    ]
    for name in [incoming["wire"], state["wire"], packet["wire"], clock, reset]:
        if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name) is None:
            raise _DriverError("invalid generated bridge port identifier")
    text = r"""
#include "pycircuit_system.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <stdexcept>
using Root=pyc_root;
auto bit(bool x){return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{x});}
template<class Wire> Wire decode(const std::string& text){
 if(text.size()!=Wire::width)throw std::runtime_error("input width");
 using Mask=typename Wire::packed_type::Mask;Mask value{0},known{0},z{0};
 for(unsigned i=0;i<Wire::width;++i){char c=text[Wire::width-1-i];auto b=uint64_t{1}<<(i%64);
  if(c=='1')value.setWord(i/64,value.word(i/64)|b);
  if(c=='0'||c=='1')known.setWord(i/64,known.word(i/64)|b);
  else if(c=='z')z.setWord(i/64,z.word(i/64)|b);
  else if(c!='x')throw std::runtime_error("input symbol");}
 return Wire::fromPacked(Wire::packed_type::fromMasks(value,known,z));}
template<class Wire> std::string bits(const Wire& w){auto p=w.packed();std::string s;
 for(unsigned n=Wire::width;n;--n){unsigned i=n-1;auto b=uint64_t{1}<<(i%64);
  s+=p.knownMask().word(i/64)&b?(p.value().word(i/64)&b?'1':'0'):(p.zMask().word(i/64)&b?'z':'x');}return s;}
template<class Wire> bool same(const Wire& a,const Wire& b){
 if(!a.isFullyKnown()||!b.isFullyKnown())return false;
 auto x=a.packed(),y=b.packed();for(unsigned i=0;i<(Wire::width+63)/64;++i)
  if(x.value().word(i)!=y.value().word(i))return false;return true;}
class System final:public gfsim::SimSystem{Root& r;gfsim::SimFailureInfo f{};
 bool Precheck()noexcept override{return r.pyc_validate_checks(f);}
 public:explicit System(Root& x):r(x){} };
int main(){try{
 Root root("clocked");root.@INPUT@=decode<decltype(root.@INPUT@)>(std::string(decltype(root.@INPUT@)::width,'0'));
 root.@CLK@=bit(false);root.@RST@=bit(true);System system(root);system.AddModule(root);system.Build();system.Reset();
 root.@RST@=bit(false);root.Work();root.DiscardNext();
 std::string command,raw;
 while(std::cin>>command){
  if(command=="S"){root.Work();std::cout<<bits(root.@STATE@)<<std::endl;root.DiscardNext();continue;}
  if(!(std::cin>>raw))throw std::runtime_error("input record missing");
  root.@INPUT@=decode<decltype(root.@INPUT@)>(raw);
  root.@CLK@=bit(true);root.@RST@=bit(command=="R");
  if(command=="Q"){
   root.Work();gfsim::SimFailureInfo f{};
   if(!root.pyc_validate_checks(f)){root.DiscardNext();throw std::runtime_error("query precheck failed");}
   std::cout<<bits(root.@PACKET@)<<std::endl;root.DiscardNext();continue;
  }
  auto before=root.@STATE@;
  bool failed=false;
  try{failed=system.Step()!=gfsim::SimStepResult::Running;}
  catch(const std::exception&){failed=true;}
  if(failed){
   root.DiscardNext();root.@INPUT@=decode<decltype(root.@INPUT@)>(std::string(decltype(root.@INPUT@)::width,'0'));
   root.@CLK@=bit(false);root.@RST@=bit(false);root.Work();
   std::cout<<"FAIL "<<bits(root.@STATE@)<<std::endl;root.DiscardNext();continue;
  }
  auto result=root.@PACKET@;
  root.@CLK@=bit(false);
  if(system.Step()!=gfsim::SimStepResult::Running)throw std::runtime_error("low sample failed");
  std::cout<<bits(result)<<' '<<(same(before,root.@STATE@)?1:0)<<std::endl;
 }
 return 0;
}catch(const std::exception& e){std::cerr<<"clocked bridge: "<<e.what()<<'\n';return 1;}}
"""
    for key, value in {
        "INPUT": incoming["wire"],
        "STATE": state["wire"],
        "PACKET": packet["wire"],
        "CLK": clock,
        "RST": reset,
    }.items():
        text = text.replace("@" + key + "@", value)
    return text


def build_rtl_bridge(generated, output, prefix, ports, state_output, timeout):
    receipt = json.loads((generated / "generated.json").read_text())
    if receipt.get("rtl_mode") != "physical" or ports["controls"]:
        raise _DriverError(
            "clocked RTL bridge requires a physical artifact without managed controls"
        )
    domains = ports["domains"]
    incoming = next(p for p in ports["inputs"] if p["ordinal"] not in domains.values())
    state, packet = ports["outputs"][state_output], ports["outputs"][1 - state_output]
    clock, reset = [
        ports["inputs"][domains[name]]["wire"] for name in ("clock", "reset")
    ]
    declarations, bindings = [], []
    for row in ports["inputs"] + ports["outputs"]:
        declarations.append(f"logic [{row['width']-1}:0] {row['wire']};")
        bindings.append(f".{row['wire']}({row['wire']})")
        if row.get("availability"):
            declarations.append(f"wire {row['availability']};")
            bindings.append(f".{row['availability']}({row['availability']})")
    tb = f"""module bridge;
{chr(10).join(declarations)}
{receipt['root_rtl_name']} dut({','.join(bindings)});
reg [8*8-1:0] command;
reg [{state['width']-1}:0] before_state;
reg [{packet['width']-1}:0] saved_packet;
integer got;
initial begin
 {clock}=0;{reset}=1;{incoming['wire']}=0;#10;
 while(!$feof(32'h80000000))begin
  got=$fscanf(32'h80000000,"%s",command);
  if(got==1)begin
   if(command=="S")begin $display("%b",{state['wire']});$fflush(32'h80000001);end
   else begin
    got=$fscanf(32'h80000000,"%b",{incoming['wire']});
    if(got!=1)$fatal(1,"missing clocked input");
    {clock}=0;{reset}=(command=="R");#10;
    if(command=="Q")begin $display("%b",{packet['wire']});$fflush(32'h80000001);end
    else begin
     before_state={state['wire']};saved_packet={packet['wire']};
     {clock}=1;#10;{clock}=0;#10;
     $display("%b %0d",saved_packet,((before_state==={state['wire']}) && (^before_state!==1'bx)));
     $fflush(32'h80000001);
    end
   end
  end
 end
 $finish;
end
endmodule
"""
    write_changed(output / "bridge.sv", tb)
    rtl = [generated / row["path"] for row in receipt["files"] if row["role"] == "rtl"]
    rtl.sort(key=lambda p: (p.name != "design_top.sv", str(p)))
    rtl += [prefix / "include/verilog/dff.v", prefix / "include/verilog/dffe.v"]
    executable = output / "bridge.vvp"
    _invoke(
        [
            "iverilog",
            "-g2012",
            "-s",
            "bridge",
            "-o",
            str(executable),
            *map(str, rtl),
            str(output / "bridge.sv"),
        ],
        timeout=timeout,
        env=dict(os.environ),
    )
    return ["vvp", str(executable)]


def build_bridge(generated, output, prefix, state_output=1, timeout=180):
    generated, output, prefix = (Path(x).resolve() for x in (generated, output, prefix))
    ports = json.loads((generated / "ports.json").read_text())
    output.mkdir(parents=True, exist_ok=True)
    receipt = json.loads((generated / "generated.json").read_text())
    if receipt["target"] == "verilog":
        return (
            build_rtl_bridge(generated, output, prefix, ports, state_output, timeout),
            ports,
        )
    source = bridge_source(ports, state_output)
    write_changed(output / "main.cpp", source)
    write_changed(
        output / "CMakeLists.txt",
        "cmake_minimum_required(VERSION 3.25)\nproject(ClockedBridge LANGUAGES CXX)\n"
        + f"add_subdirectory({_cmake_quote(str(generated))} generated)\n"
        + "add_executable(clocked main.cpp)\ntarget_compile_features(clocked PRIVATE cxx_std_20)\n"
        + "target_link_libraries(clocked PRIVATE pycircuit_modules)\n",
    )
    env = dict(os.environ)
    _invoke(
        [
            "cmake",
            "-S",
            str(output),
            "-B",
            str(output / "build"),
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            f"-Dpycircuit_DIR={prefix / 'share/pycircuit/cmake'}",
        ],
        timeout=timeout,
        env=env,
    )
    _invoke(
        ["cmake", "--build", str(output / "build"), "--parallel", "2"],
        timeout=timeout,
        env=env,
    )
    return output / "build/clocked", ports


class ClockedBridge:
    def __init__(self, executable):
        self.process = subprocess.Popen(
            executable if isinstance(executable, list) else [str(executable)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )

    def command(self, operation, payload=None):
        self.process.stdin.write(
            operation + (" " + payload if payload is not None else "") + "\n"
        )
        self.process.stdin.flush()
        if not select.select([self.process.stdout], [], [], 30)[0]:
            raise _DriverError("clocked bridge response timeout")
        line = self.process.stdout.readline()
        if not line:
            raise _DriverError("clocked bridge stopped: " + self.process.stderr.read())
        if line.startswith("FAIL "):
            raise _DriverError(
                "clocked tick failed; committed state: " + line[5:].strip()
            )
        if "PYCIRCUIT_PHYSICAL_FAULT" in line:
            raise _DriverError("physical source/clock fault: " + line)
        return line.strip()

    def close(self):
        self.process.stdin.close()
        rc = self.process.wait(timeout=30)
        if rc:
            raise _DriverError("clocked bridge failed: " + self.process.stderr.read())
