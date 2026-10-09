"""Read-only generated-state access for full routed graph runtime tests."""

import re

QUEUE_NAMES = (
    "incoming",
    "prepared",
    "scheduled",
    "route_0",
    "route_1",
    "route_2",
    "route_3",
    "route_0_done",
    "route_1_done",
    "route_2_done",
    "route_3_done",
    "completed",
    "ordered",
    "output",
)
DEPTHS = (16, 4, 16, 8, 8, 8, 8, 1, 1, 1, 1, 8, 8, 1)


def family(header, name):
    section = header.split("class pyc_family_" + name, 1)[1]
    return section.split("template <std::size_t pyc_count>", 1)[0].split(
        "class " + name + " final", 1
    )[0]


def members(section):
    return re.findall(
        r"std::shared_ptr<[^;\n]*::pyc_family_(\w+)<pyc_count>> (pyc_instance_\w+);",
        section,
    )


def fifo_members(section):
    return [
        (name, int(depth))
        for depth, name in re.findall(
            r"fifo_kernel<[^;\n]*WorkItem, (\d+),[^;\n]*> (pyc_queue_\w+)_state;",
            section,
        )
    ]


def layout(header):
    section = family(header, "pipeline")
    children = members(section)
    direct = fifo_members(section)
    assert len(direct) == 8, direct
    assert [depth for _, depth in direct] == [16, 16, 8, 8, 8, 8, 8, 8]
    by_type = {}
    for typ, name in children:
        by_type.setdefault(typ, []).append(name)
    assert {key: len(values) for key, values in by_type.items()} == {
        "frontend": 1,
        "dependency": 1,
        "branch": 4,
        "merge": 1,
        "reorder": 1,
        "output_stage": 1,
    }, by_type
    queues = [
        ("", direct[0][0]),
        (by_type["frontend"][0], fifo_members(family(header, "frontend"))[0][0]),
        ("", direct[1][0]),
    ]
    queues += [("", name) for name, _ in direct[2:6]]
    queues += [
        (name, fifo_members(family(header, "branch"))[0][0])
        for name in by_type["branch"]
    ]
    queues += [
        ("", direct[6][0]),
        ("", direct[7][0]),
        (
            by_type["output_stage"][0],
            fifo_members(family(header, "output_stage"))[0][0],
        ),
    ]
    assert len(queues) == 14
    return {
        "queues": queues,
        "scheduler": by_type["dependency"][0],
        "merge": by_type["merge"][0],
        "reorder": by_type["reorder"][0],
    }


def cpp(layout, graph="dut.root_->pyc_implementation", system=False):
    accesses = []
    for child, name in layout["queues"]:
        accesses.append((graph + ("->" + child if child else ""), name))
    sched = graph + "->" + layout["scheduler"]
    merge = graph + "->" + layout["merge"]
    reorder = graph + "->" + layout["reorder"]
    snap = [
        f"out += bits({sched}->pyc_instance_epoch_state->current(0).q.packed());",
        f"out += bits({merge}->pyc_instance_cursor_state->current(0).q.packed());",
        f"out += bits({reorder}->pyc_instance_next_key_state->current(0).q.packed());",
    ]
    raw = []
    flags = []
    for i, ((owner, name), depth) in enumerate(zip(accesses, DEPTHS, strict=True)):
        snap += [f"append_fifo<{depth}>(out, {owner}->{name}_state->current(0));"]
        raw += [f"raw_fifo(out, {owner}->{name}_state->current(0));"]
        flags += [
            f"if ({owner}->{name}_out_valid.element(0).value().toBool() && {owner}->{name}_out_ready.element(0).value().toBool()) pop |= (1u<<{i});",
            f"if ({owner}->{name}_in_ready.element(0).value().toBool() && {owner}->{name}_in_valid.element(0).value().toBool()) push |= (1u<<{i});",
        ]
    for owner, field, count in (
        (sched, "entries", 8),
        (merge, "cursor", 1),
        (reorder, "entries", 64),
        (reorder, "next_key", 1),
        (sched, "epoch", 1),
    ):
        raw += [
            f"for(int i=0;i<{count};++i) {{ auto& q={owner}->pyc_instance_{field}_state->current(i); out+=q.clock?'1':'0';out+=bits(q.q.packed()); }}"
        ]
    snap += [
        f"for(int i=0;i<8;++i) out += bits({sched}->pyc_instance_entries_state->current(i).q.packed());",
        f"for(int i=0;i<64;++i) out += bits({reorder}->pyc_instance_entries_state->current(i).q.packed());",
    ]
    inputs = (
        ""
        if system
        else "input.valid=known<decltype(input.valid)>(valid); input.take=known<decltype(input.take)>(take); gfsim::Bits<106> item;item.setWord(0,lo);item.setWord(1,hi); using Payload=std::remove_cvref_t<decltype(input.data)>;input.data=Payload::fromPacked(Payload::packed_type::known(item));"
    )
    initial = (
        ""
        if system
        else "input.valid=known<decltype(input.valid)>(0);input.take=known<decltype(input.take)>(0);input.data=known<decltype(input.data)>(0);"
    )
    result = f"{graph}->result.element(0).packed()"
    return f"""#include "pycircuit_system.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
#include <type_traits>
template<class W> W known(uint64_t value){{using P=typename W::packed_type;return W::fromPacked(P::known(gfsim::Bits<W::width>{{value}}));}}
template<unsigned W>std::string bits(const gfsim::FourState<W>& value){{if(!value.isFullyKnown())throw std::runtime_error("unknown state plane");std::string s;for(int i=W-1;i>=0;--i)s+=value.value().bit(i)?'1':'0';return s;}}
template<unsigned W>std::string four_bits(const gfsim::FourState<W>& value){{std::string s;for(int i=W-1;i>=0;--i)s+=value.zMask().bit(i)?'z':!value.knownMask().bit(i)?'x':value.value().bit(i)?'1':'0';return s;}}
std::string number(uint64_t value,int width){{std::string s;for(int i=width-1;i>=0;--i)s+=(value>>i)&1?'1':'0';return s;}}
std::string hex(std::string s){{s=std::string((4-s.size()%4)%4,'0')+s;std::string h;for(size_t i=0;i<s.size();i+=4){{unsigned v=0;for(int j=0;j<4;++j)v=(v<<1)|(s[i+j]=='1');h+="0123456789abcdef"[v];}}return h;}}
template<int D,class Q>void append_fifo(std::string& out,const Q& q){{if(!q.initialized)throw std::runtime_error("uninitialized fifo");out+=number(q.count,std::bit_width(unsigned(D)));for(int i=0;i<D;++i)out+=i<int(q.count)?bits(q.storage[(q.rd+i)%D].packed()):std::string(106,'0');}}
template<class Q>void raw_fifo(std::string& out,const Q& q){{out+=std::to_string(q.rd)+":"+std::to_string(q.wr)+":"+std::to_string(q.count)+":"+std::to_string(q.clock)+":"+std::to_string(q.initialized);for(auto& v:q.storage)out+=four_bits(v.packed());}}
int main(int argc,char**argv){{
 pyc_dut dut(argc>1?std::stoul(argv[1]):1);pyc_dut::Inputs input{{}};{initial}
 input.pyc_7079635f636c6b=known<decltype(input.pyc_7079635f636c6b)>(0);input.pyc_7079635f727374=known<decltype(input.pyc_7079635f727374)>(0);dut.drive(input);dut.system().Build();dut.system().Reset();
 auto snapshot=[&](){{std::string out;{' '.join(snap)}if(out.size()!=21934)throw std::runtime_error("snapshot width");return hex(out);}};
 auto raw=[&](){{std::string out;{' '.join(raw)}return out;}};
 unsigned count;std::cin>>count;
 for(unsigned attempt=0;attempt<count;++attempt){{
  unsigned valid,take,reset;uint64_t lo,hi;std::cin>>valid>>take>>reset>>std::hex>>lo>>hi>>std::dec;{inputs}
  input.pyc_7079635f636c6b=known<decltype(input.pyc_7079635f636c6b)>(0);dut.drive(input);auto before=snapshot();if(reset){{dut.system().Reset();std::cout<<"ROW "<<attempt<<" 1 0 "<<before<<" "<<snapshot()<<" - 0 0 - "<<snapshot()<<"\\n";continue;}}
  auto work=snapshot();auto physical_before=raw();auto cycle=dut.system().cycle();
  auto status=dut.system().Step();bool failed=status==gfsim::SimStepResult::Failed;unsigned pop=0,push=0;std::string public_value="-",error="-";
  if(!failed){{if(status!=gfsim::SimStepResult::Running)throw std::runtime_error("low step");auto low={result};public_value=hex(bits(low));{' '.join(flags)}
   input.pyc_7079635f636c6b=known<decltype(input.pyc_7079635f636c6b)>(1);dut.drive(input);status=dut.system().Step();failed=status==gfsim::SimStepResult::Failed;if(!failed&&hex(bits({result}))!=public_value)throw std::runtime_error("old Q pair changed");}}
  if(failed){{pop=push=0;auto failure=dut.system().failureInfo();if(failure.phase!=gfsim::SimFailurePhase::Check || failure.code!="source_check_failed" || failure.instance.empty() || failure.sourceJson.empty() || failure.checkIdJson.empty())throw std::runtime_error("failure attribution");error=std::string(failure.message);if(raw()!=physical_before||dut.system().cycle()!=cycle)throw std::runtime_error("failed state changed");bool rejected=false;try{{dut.sample();}}catch(const std::logic_error&){{rejected=true;}}if(!rejected)throw std::runtime_error("failed sample exposed");}}
  std::cout<<"ROW "<<attempt<<" "<<!failed<<" "<<failed<<" "<<before<<" "<<snapshot()<<" "<<public_value<<" "<<pop<<" "<<push<<" "<<error<<" "<<work<<"\\n";
 }}
}}
"""


def verilog(layout, graph="dut.dut", system=False):
    accesses = [
        (
            graph
            + ("." + child if child else "")
            + "."
            + name.replace("pyc_queue_", "pyc_instance_", 1)
        )
        for child, name in layout["queues"]
    ]
    sched = graph + "." + layout["scheduler"]
    merge = graph + "." + layout["merge"]
    reorder = graph + "." + layout["reorder"]
    snap = [
        f"s={{{sched}.pyc_instance_epoch.q_current,{merge}.pyc_instance_cursor.q_current,{reorder}.pyc_instance_next_key.q_current}};"
    ]
    raw = []
    flags = []
    for i, (owner, depth) in enumerate(zip(accesses, DEPTHS, strict=True)):
        snap += [
            f"s=(s<<{depth.bit_length()})|{owner}.count;",
            f"for(int i=0;i<{depth};++i) s=(s<<106)|(i<{owner}.count?{owner}.storage[({owner}.rd+i)%{depth}]:106'b0);",
        ]
        raw += [
            f's={{s,$sformatf("%b", {{{owner}.rd,{owner}.wr,{owner}.count,{owner}.initialized,{owner}.latency_one.managed.clock_current}})}};',
            f'for(int i=0;i<{depth};++i) s={{s,$sformatf("%b",{owner}.storage[i])}};',
        ]
        flags += [
            f"pop[{i}]={owner}.out_valid & {owner}.out_ready;",
            f"push[{i}]={owner}.in_ready & {owner}.in_valid;",
        ]
    for owner, field, count in ((sched, "entries", 8), (reorder, "entries", 64)):
        for i in range(count):
            path = f"{owner}.pyc_instances_{field}[{i}].pyc_instance_{field}"
            snap += [f"s=(s<<$bits({path}.q_current))|{path}.q_current;"]
            raw += [
                f's={{s,$sformatf("%b",{{{path}.q_current,{path}.managed.clock_current}})}};'
            ]
    for owner, field in ((sched, "epoch"), (merge, "cursor"), (reorder, "next_key")):
        raw += [
            f's={{s,$sformatf("%b",{{{owner}.pyc_instance_{field}.q_current,{owner}.pyc_instance_{field}.managed.clock_current}})}};'
        ]
    public = graph + ".result"
    ports = "" if system else ".valid(valid),.data(data),.take(take),.result(result),"
    return f"""module tb;
logic valid=0,take=0;logic[105:0]data=0;logic[535:0]result;
logic pyc_7079635f636c6b=0,pyc_7079635f727374=0;
logic[2:0]pyc_phase=0;logic pyc_root_commit_ok=0;wire pyc_local_error;
pyc_root dut({ports}.pyc_7079635f636c6b(pyc_7079635f636c6b),.pyc_7079635f727374(pyc_7079635f727374),.pyc_phase(pyc_phase),.pyc_root_commit_ok(pyc_root_commit_ok),.pyc_local_error(pyc_local_error));
function automatic logic[21933:0] snapshot();logic[21933:0]s;s='0;{' '.join(snap)}return s;endfunction
function automatic string raw();string s;s="";{' '.join(raw)}return s;endfunction
integer file,epochs,status,v,t,reset_host;logic[63:0]lo,hi;string stimulus,before_state,physical_before,work_state,after_state,public_value;
logic[535:0]low_result;logic[13:0]pop,push;bit failed=0;
task automatic host_reset();pyc_phase=4;#1;pyc_root_commit_ok=1;pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;failed=0;#1;endtask
initial begin
 #1;host_reset();if(!$value$plusargs("stimulus=%s",stimulus))$fatal(1,"stimulus");file=$fopen(stimulus,"r");status=$fscanf(file,"%d",epochs);
 for(int attempt=0;attempt<epochs;++attempt)begin
  status=$fscanf(file,"%d %d %d %h %h",v,t,reset_host,lo,hi);valid=v;take=t;data={{hi[41:0],lo}};pyc_7079635f636c6b=0;#1;
  before_state=$sformatf("%h",snapshot());if(reset_host)host_reset();work_state=$sformatf("%h",snapshot());physical_before=raw();pop=0;push=0;public_value="-";
  if(!failed&&!reset_host)begin
   pyc_phase=1;#1;pyc_root_commit_ok=(pyc_local_error===1'b0);
   if(!pyc_root_commit_ok)begin
    pyc_phase=2;#1;if(raw()!=physical_before)$fatal(1,"denied Xfer changed physical state");pyc_phase=3;#1;failed=1;
   end else begin
    low_result={public};public_value=$sformatf("%h",low_result);{' '.join(flags)}
    pyc_phase=2;#1;pyc_phase=0;pyc_root_commit_ok=0;#1;
    pyc_7079635f636c6b=1;#1;pyc_phase=1;#1;pyc_root_commit_ok=(pyc_local_error===1'b0);
    if(!pyc_root_commit_ok)$fatal(1,"unexpected high-only fault");if({public}!==low_result)$fatal(1,"old Q pair changed");
    pyc_phase=2;#1;
   end
   pyc_phase=0;pyc_root_commit_ok=0;#1;
  end
  if(failed)begin pop=0;push=0;if(raw()!=physical_before)$fatal(1,"failure changed physical state");end
  after_state=$sformatf("%h",snapshot());
  $display("ROW %0d %0d %0d %s %s %s %0d %0d - %s",attempt,!failed,failed,before_state,after_state,public_value,pop,push,work_state);
 end
 $finish;
end
endmodule
"""
