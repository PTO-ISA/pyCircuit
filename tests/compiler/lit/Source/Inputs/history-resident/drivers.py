"""Test-only drivers for borrowed channels and read-only storage snapshots.

Boundary queues are the test environment. They advance only from literal host
stimuli and actual compiled DUT transaction outputs, never oracle results.
"""

import re

QUEUES = {
    "rob": [
        f"{side}_{port}"
        for side in ("left", "right")
        for port in ("flush", "allocate", "completion")
    ]
    + ["left_allocated", "left_retired", "right_allocated", "right_retired"],
    "isq": [
        "left_request",
        "left_readiness",
        "right_request",
        "right_readiness",
        "left_issued",
        "right_issued",
    ],
}
WIDTHS = {"rob": [51] * 10, "isq": [39, 7, 39, 7, 39, 39]}


def instances(header, kind):
    section = header.split(f"class pyc_family_dual_{kind}", 1)[1].split("public:", 1)[
        -1
    ]
    names = re.findall(
        rf"std::shared_ptr<[^;\n]*pyc_family_{kind}<pyc_count>> (pyc_instance_\w+);",
        section,
    )
    assert len(names) == 2, names
    return names


def ports(kind):
    result = []
    for side in ("left", "right"):
        if kind == "rob":
            result += [
                (
                    f"{side}_{p}_available",
                    1,
                    f"occ[{QUEUES[kind].index(side + '_' + p)}]",
                )
                for p in ("flush", "allocate", "completion")
            ]
            result += [
                (f"{side}_{p}", 51, f"data[{QUEUES[kind].index(side + '_' + q)}]")
                for p, q in (
                    ("flush_request", "flush"),
                    ("allocate_request", "allocate"),
                    ("completion", "completion"),
                )
            ]
            result += [
                (f"{side}_{p}_space", 1, f"!occ[{QUEUES[kind].index(side + '_' + p)}]")
                for p in ("allocated", "retired")
            ]
        else:
            for p in ("request", "readiness"):
                q = QUEUES[kind].index(side + "_" + p)
                result += [
                    (f"{side}_{p}_available", 1, f"occ[{q}]"),
                    (f"{side}_{p}", WIDTHS[kind][q], f"data[{q}]"),
                ]
            result += [
                (
                    f"{side}_issued_space",
                    1,
                    f"!occ[{QUEUES[kind].index(side + '_issued')}]",
                )
            ]
    return result


def result_effects(kind, lang):
    lines = []
    per = 107 if kind == "rob" else 42
    for side in ("left", "right"):
        off = per if side == "left" else 0
        if kind == "rob":
            takes = [("flush", 106), ("allocate", 105), ("completion", 104)]
            pushes = [("allocated", 103, 52, 51), ("retired", 51, 0, 51)]
        else:
            takes = [("request", 41), ("readiness", 40)]
            pushes = [("issued", 39, 0, 39)]
        for name, bit in takes:
            q = QUEUES[kind].index(side + "_" + name)
            test = (
                f"result.value().bit({off + bit})"
                if lang == "cpp"
                else f"saved_result[{off + bit}]"
            )
            lines.append(
                f'if ({test}) {{ if (!occ[{q}]) throw std::runtime_error("empty DUT take"); next[{q}] = false; }}'
                if lang == "cpp"
                else f'if ({test}) begin if (!occ[{q}]) $fatal(1, "empty DUT take"); nxt[{q}] = 0; end'
            )
        for name, bit, low, width in pushes:
            q = QUEUES[kind].index(side + "_" + name)
            if lang == "cpp":
                lines.append(
                    f'if (result.value().bit({off + bit})) {{ if (occ[{q}]) throw std::runtime_error("full DUT push"); next[{q}] = true; data[{q}] = gfsim::extract<{width}>(result, {off + low}).value().word(0); }}'
                )
            else:
                lines.append(
                    f'if (saved_result[{off + bit}]) begin if (occ[{q}]) $fatal(1, "full DUT push"); nxt[{q}] = 1; data[{q}] = saved_result[{off + low} +: {width}]; end'
                )
    return "\n".join(lines)


def cpp(kind, names):
    width = 974 if kind == "rob" else 616
    qcount = len(QUEUES[kind])
    snapshot = []
    if kind == "rob":
        for name in names:
            for field in ("head", "tail", "count", "epoch"):
                snapshot.append(
                    f"out += bits(dut.root_->pyc_implementation->{name}->pyc_instance_{field}_state->current(0).q.packed());"
                )
    for name in names:
        snapshot.append(
            f"for (int i=0;i<4;++i) out += bits(dut.root_->pyc_implementation->{name}->pyc_instance_entries_state->current(i).q.packed());"
        )
    if kind == "isq":
        for name in names:
            snapshot.append(
                f"for (int i=0;i<64;++i) out += bits(dut.root_->pyc_implementation->{name}->pyc_instance_ready_tags_state->current(i).q.packed());"
            )
    assignments = "\n".join(
        f"input.{name} = known<decltype(input.{name})>({expr});"
        for name, _, expr in ports(kind)
    )
    return f"""#include "pycircuit_system.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
template<class W> W known(uint64_t v) {{ using P=typename W::packed_type; return W::fromPacked(P::known(gfsim::Bits<W::width>{{v}})); }}
template<unsigned N> std::string bits(const gfsim::FourState<N>& p) {{
 if (!p.isFullyKnown()) throw std::runtime_error("unknown snapshot plane");
 std::string out; for (int b=N-1;b>=0;--b) out += p.value().bit(b)?'1':'0'; return out;
}}
int main(int argc,char**argv) {{
 pyc_dut dut(argc>1?std::stoul(argv[1]):1); pyc_dut::Inputs input{{}};
 input.pyc_7079635f636c6b=known<decltype(input.pyc_7079635f636c6b)>(0);
 input.pyc_7079635f727374=known<decltype(input.pyc_7079635f727374)>(0);
 dut.drive(input); dut.system().Build(); dut.system().Reset();
 std::array<bool,{qcount}> occ{{}},next{{}},offer{{}},inject{{}},take{{}};
 std::array<uint64_t,{qcount}> data{{}},offered{{}},injected{{}};
 constexpr int widths[]={{ {','.join(map(str,WIDTHS[kind]))} }};
 auto snapshot=[&]() {{ std::string out;
 for(int q=0;q<{qcount};++q) {{ out += occ[q]?'1':'0'; for(int b=widths[q]-1;b>=0;--b) out += occ[q]&&((data[q]>>b)&1)?'1':'0'; }}
 {' '.join(snapshot)}
 if(out.size()!={width}) throw std::runtime_error("snapshot width"); return out; }};
 int epochs; std::cin>>epochs; if(epochs>256) return 3;
 for(int epoch=0;epoch<epochs;++epoch) {{
  std::cout<<epoch<<" "<<snapshot()<<" ";
  for(int q=0;q<{qcount};++q) {{ unsigned o,j,t; std::cin>>o>>offered[q]>>j>>injected[q]>>t; offer[q]=o;inject[q]=j;take[q]=t;
   if((offer[q]||inject[q])&&occ[q]) throw std::runtime_error("host full offer");
   if(take[q]&&!occ[q]) throw std::runtime_error("host empty take");
   if(inject[q]) {{ occ[q]=true;data[q]=injected[q]; }} }}
  std::cout<<snapshot()<<" ";
  {assignments}
  input.pyc_7079635f636c6b=known<decltype(input.pyc_7079635f636c6b)>(0); dut.drive(input); if(dut.system().Step()!=gfsim::SimStepResult::Running) return 4;
  auto low=dut.sample().result.packed();
  input.pyc_7079635f636c6b=known<decltype(input.pyc_7079635f636c6b)>(1); dut.drive(input); if(dut.system().Step()!=gfsim::SimStepResult::Running) return 5;
  auto result=dut.sample().result.packed(); if(bits(low)!=bits(result)) throw std::runtime_error("old Q pair changed");
  next=occ; for(int q=0;q<{qcount};++q) if(take[q]) next[q]=false;
  {result_effects(kind,'cpp')}
  for(int q=0;q<{qcount};++q) {{ if(offer[q]) {{ if(next[q]) throw std::runtime_error("host commit full");next[q]=true;data[q]=offered[q]; }} occ[q]=next[q]; }}
  std::cout<<snapshot()<<" "<<bits(result)<<"\\n";
 }}
}}
"""


def verilog(kind, names):
    width = 974 if kind == "rob" else 616
    result_width = 214 if kind == "rob" else 84
    qcount = len(QUEUES[kind])
    snapshots = []
    if kind == "rob":
        for name in names:
            snapshots += [
                f"dut.dut.{name}.pyc_instance_{field}.q_current"
                for field in ("head", "tail", "count", "epoch")
            ]
    for name in names:
        snapshots += [
            f"dut.dut.{name}.pyc_instances_entries[{i}].pyc_instance_entries.q_current"
            for i in range(4)
        ]
    if kind == "isq":
        for name in names:
            snapshots += [
                f"dut.dut.{name}.pyc_instances_ready_tags[{i}].pyc_instance_ready_tags.q_current"
                for i in range(64)
            ]
    connections = ",\n".join(f".{name}({expr})" for name, _, expr in ports(kind))
    queues = "\n".join(
        f"s=(s<<{w+1}) | ({{occ[{q}], {w}'(occ[{q}]?data[{q}]:0)}});"
        for q, w in enumerate(WIDTHS[kind])
    )
    return f"""module tb;
logic clk=0,rst=0;
logic [{result_width-1}:0] result,saved_result,low_result;
bit occ[0:{qcount-1}],nxt[0:{qcount-1}],offer[0:{qcount-1}],inject[0:{qcount-1}],take[0:{qcount-1}];
logic [50:0] data[0:{qcount-1}],offered[0:{qcount-1}],injected[0:{qcount-1}];
integer file,epochs,status,o,j,t;
string stimulus;
pyc_root dut({connections}, .pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),.result(result));
function automatic logic [{width-1}:0] snapshot();
logic [{width-1}:0] s;
s='0;
{queues}
s=(s<<{sum( (2,2,3,16))*2+51*8 if kind=='rob' else 39*8+128}) | {{{', '.join(snapshots)}}};
return s;
endfunction
initial begin
 for(int q=0;q<{qcount};++q) begin occ[q]=0;data[q]=0;end
 rst=1; #2;clk=1;#2;clk=0;rst=0;#2;
 if(!$value$plusargs("stimulus=%s",stimulus)) $fatal(1,"missing stimulus");
 file=$fopen(stimulus,"r");status=$fscanf(file,"%d",epochs); if(epochs>256) $fatal(1,"bound");
 for(int epoch=0;epoch<epochs;++epoch) begin
  $write("%0d %b ",epoch,snapshot());
  for(int q=0;q<{qcount};++q) begin
   status=$fscanf(file,"%d %d %d %d %d",o,offered[q],j,injected[q],t);
   offer[q]=o;inject[q]=j;take[q]=t;
   if((offer[q]||inject[q])&&occ[q]) $fatal(1,"host full offer");
   if(take[q]&&!occ[q]) $fatal(1,"host empty take");
   if(inject[q]) begin occ[q]=1;data[q]=injected[q];end
  end
  #2; $write("%b ",snapshot()); low_result=result;
  clk=1; saved_result=result; #2;
  if(low_result!==saved_result) $fatal(1,"old Q pair changed");
  for(int q=0;q<{qcount};++q) begin nxt[q]=occ[q];if(take[q]) nxt[q]=0;end
  {result_effects(kind,'verilog')}
  for(int q=0;q<{qcount};++q) begin if(offer[q]) begin if(nxt[q]) $fatal(1,"host commit full");nxt[q]=1;data[q]=offered[q];end occ[q]=nxt[q];end
  $display("%b %b",snapshot(),saved_result);
  clk=0;#2;
 end
 $finish;
end
endmodule
"""
