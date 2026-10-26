"""Independent public-flow oracles for Integer-parameterized owned state.

Provider publication must preserve templates. Actual binding, storage identity,
and generated execution are checked separately, without evaluating DUT Python.
"""

from __future__ import annotations

import hashlib
import json
import os
import shutil
from pathlib import Path

import pytest
from test_saved_anchor import _json, _save_manifest, _tree
from test_scalar_generic_modules import (
    _compile_replace,
    _link,
    _parameters,
    _rejected,
)
from test_source_system_execution import (
    _bundle,
    _cli,
    _compile,
    _install,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system

COUNTER = """import pycircuit as ac
@ac.module
def Counter(enable: ac.u1, *, W: int = 8) -> ac.bits[W]:
    count: ac.bits[W] = 0
    @ac.rule
    def increment():
        if enable:
            count = count + 1
    return count
"""

BANK = """import pycircuit as ac
@ac.module
def Bank(enable: ac.u1, *, N: int = 4) -> ac.u8:
    entries: ac.table[N, ac.u8] = 0
    @ac.rule
    def increment():
        if enable:
            entries[0] = entries[0] + 1
    return entries[0]
"""

PROVIDER = (
    COUNTER
    + """
@ac.module
def Required(enable: ac.u1, *, W: int) -> ac.bits[W]:
    count: ac.bits[W] = 0
    @ac.rule
    def increment():
        if enable:
            count = count + 1
    return count

@ac.module
def Bank(enable: ac.u1, *, W: int = 8, N: int = 4) -> ac.table[N, ac.bits[W]]:
    entries: ac.table[N, ac.bits[W]] = 3
    @ac.rule
    def increment():
        last: ac.u64 = N - 1
        if enable:
            entries[last] = entries[last] + 1
    return entries

@ac.module
def BankParent(enable: ac.u1, *, WIDTH: int = 8, DEPTH: int = 4) -> ac.table[DEPTH, ac.bits[WIDTH]]:
    child = Bank(enable, W=WIDTH, N=DEPTH)
    return child

@ac.module
def Parent(enable: ac.u1, *, WIDTH: int = 8) -> ac.bits[WIDTH]:
    child = Counter(enable, W=WIDTH)
    return child
"""
)


def _source(tmp_path, text, name="provider.py"):
    source = tmp_path / "source"
    source.mkdir(exist_ok=True)
    (source / name).write_text(text)
    return source


@pytest.mark.parametrize(
    "text,top",
    [
        (
            COUNTER.replace(", *, W: int = 8", "").replace("ac.bits[W]", "ac.u8"),
            "Counter",
        ),
        (
            "import pycircuit as ac\n@ac.module\ndef Generic(value: ac.bits[W], *, W: int = 8) -> ac.bits[W]:\n    return value | 1\n",
            "Generic",
        ),
        (COUNTER, "Counter"),
        (BANK, "Bank"),
    ],
    ids=["fixed-state-control", "scalar-generic-control", "counter", "bank"],
)
def test_public_parameterized_state_repro(tmp_path: Path, text: str, top: str):
    source = _source(tmp_path, text)
    unit = tmp_path / "unit"
    _ok(_compile(source, "provider.py", unit))
    _ok(_link([unit], "checks.provider." + top, tmp_path / "final.ac"))


def _published(tmp_path, consumer, provider_text=PROVIDER):
    source = _source(tmp_path, provider_text)
    (source / "bench.py").write_text(consumer)
    provider, bench = tmp_path / "provider", tmp_path / "bench"
    _ok(_compile(source, "provider.py", provider))
    before = _tree(provider)
    manifest = json.loads((provider / "unit.json").read_text())
    body = provider / manifest["files"]["body"]
    interface = (provider / manifest["files"]["interface"]).read_text()
    for name in ("Counter", "Required", "Bank", "BankParent", "Parent"):
        assert interface.count(f'sym_name = "checks.provider.{name}"') == 1
    # Definitions retain declaration-relative types, even though all consumers
    # in this test happen to use small closed layouts.
    for owner, parameter in (("Counter", "W"), ("Bank", "W"), ("Bank", "N")):
        assert (
            f'name = "{parameter}", owner = @checks.provider.{owner}'
            in body.read_text()
        )
    hidden = tmp_path / "hidden-body.ac"
    (source / "provider.py").unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "bench.py", bench, [provider]))
    finally:
        hidden.rename(body)
    assert _tree(provider) == before
    return source, provider, bench


BENCH = """import pycircuit as ac
from checks.provider import Counter, Required, Bank, BankParent, Parent
@ac.system
def Exercise():
    phase: ac.u8 = 0
    @ac.rule
    def advance():
        phase = phase + 1
    a = Parent(1, WIDTH=3)
    b = Parent(phase[0], WIDTH=5)
    c = Counter(~phase[0], W=3)
    d = Counter(1)
    e = Required(1, W=5)
    x = BankParent(1, WIDTH=3, DEPTH=3)
    y = BankParent(phase[0], WIDTH=5, DEPTH=5)
    z = Bank(~phase[0], W=3, N=3)
    one = Bank(1, W=5, N=1)
    @ac.rule
    def observe():
        ac.log('info', 'parameterized state', a, b, c, d, e,
               x[0], x[1], x[2], y[0], y[1], y[2], y[3], y[4],
               z[0], z[1], z[2], one[0])
"""


def _expected(cycles):
    a = b = c = d = e = 0
    x, y, z, one = [3] * 3, [3] * 5, [3] * 3, 3
    rows = []
    for phase in range(cycles):
        rows.extend([(a, b, c, d, e, *x, *y, *z, one)] * 2)
        a, d, e = (a + 1) % 8, (d + 1) % 256, (e + 1) % 32
        x[-1], one = (x[-1] + 1) % 8, (one + 1) % 32
        if phase % 2:
            b, y[-1] = (b + 1) % 32, (y[-1] + 1) % 32
        else:
            c, z[-1] = (c + 1) % 8, (z[-1] + 1) % 8
    return rows


def _execute(final, root, cycles=36):
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    transcripts = []
    for target in ("cpp", "verilog"):
        directory = root / target
        directory.mkdir()
        binary = _bundle(final, directory, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            args = (
                ["--cycles", str(cycles), "--workers", str(workers)]
                if target == "cpp"
                else [f"+cycles={cycles}"]
            )
            records = _records(_ok(_run([str(binary), *args], timeout=60)))
            rows = [
                r
                for r in records
                if r["kind"] == "log" and r["spec"]["event"] == "parameterized state"
            ]
            assert [
                tuple(int(v["value"]) for v in r["values"]) for r in rows
            ] == _expected(cycles)
            assert [int(r["evaluation_epoch"]) for r in rows] == list(range(cycles * 2))
            result = [r for r in records if r["kind"] == "result"]
            assert len(result) == 1 and result[0]["status"] == "TERMINATED"
            transcripts.append([r for r in records if r["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


@pytest.mark.parametrize("tail_binding", ["local", "default"])
def test_interface_only_provider_mixed_instances_execute_identically(
    tmp_path, tail_binding
):
    provider_text = PROVIDER
    if tail_binding == "default":
        # A typed local Rule formal has an explicit hardware binding. A typed
        # Module annotation would instead introduce persistent storage.
        provider_text = provider_text.replace(
            "def increment():\n        last: ac.u64 = N - 1",
            "def increment(last: ac.u64 = N - 1):",
        )
        assert provider_text != PROVIDER
    _, provider, bench = _published(tmp_path, BENCH, provider_text)
    final = tmp_path / "final.ac"
    missing = _link([bench], "checks.bench.Exercise", final)
    _rejected(missing, "missing", "dependency", "closure", "unresolved")
    assert not final.exists()
    _ok(_link([provider, bench], "checks.bench.Exercise", final))
    _execute(final, tmp_path)


@pytest.mark.parametrize(
    "top,bindings",
    [
        ("Counter", {"W": 3}),
        ("Counter", {"W": 5}),
        ("Required", {"W": 5}),
        ("Parent", {"WIDTH": 3}),
        ("Bank", {"W": 3, "N": 3}),
        ("Bank", {"W": 5, "N": 5}),
    ],
)
def test_generic_root_actuals_are_retained(tmp_path, top, bindings):
    source = _source(tmp_path, PROVIDER)
    unit = tmp_path / "unit"
    _ok(_compile(source, "provider.py", unit))
    parameters = _parameters(tmp_path / "parameters.json", **bindings)
    anchor = tmp_path / "anchor"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.provider." + top,
            "--parameters",
            str(parameters),
            "--target",
            "anchor",
            "-o",
            str(anchor),
        )
    )
    shutil.rmtree(source)
    shutil.rmtree(unit)
    before = _tree(anchor)
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    final = tmp_path / "final.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    assert _tree(anchor) == before
    # The final root's actual binding must survive source-absent reload.
    root = next(
        line for line in final.read_text().splitlines() if '"ac.system"' in line
    )
    for value in bindings.values():
        assert f"#ac.math_int<{value}>" in root


def test_bound_width_depth_and_initializer_failures_preserve_output(tmp_path):
    source = _source(tmp_path, PROVIDER)
    unit = tmp_path / "unit"
    _ok(_compile(source, "provider.py", unit))
    final = tmp_path / "final.ac"
    _ok(
        _link(
            [unit],
            "checks.provider.Bank",
            final,
            _parameters(tmp_path / "valid.json", W=5, N=3),
        )
    )
    before = final.read_bytes()
    for name, top, bindings in [
        ("width-zero", "Counter", {"W": 0}),
        ("width-negative", "Counter", {"W": -1}),
        ("depth-zero", "Bank", {"N": 0}),
        ("depth-negative", "Bank", {"N": -1}),
        ("initializer-fit", "Bank", {"W": 1, "N": 3}),
        ("missing-required", "Required", {}),
    ]:
        result = _link(
            [unit],
            "checks.provider." + top,
            final,
            _parameters(tmp_path / (name + ".json"), **bindings),
            replace=True,
        )
        _rejected(
            result,
            "width",
            "positive",
            "extent",
            "fit",
            "required",
            "default",
            "parameter",
        )
        assert final.read_bytes() == before


@pytest.mark.parametrize(
    "mutation",
    [
        "foreign-width",
        "wide-guard",
        "runtime-initializer",
        "closed-zero-width",
        "closed-initializer-overflow",
        "noninteger-formal",
    ],
)
def test_bad_state_templates_preserve_publication(tmp_path, mutation):
    source = _source(tmp_path, COUNTER)
    unit = tmp_path / "unit"
    _ok(_compile(source, "provider.py", unit))
    before = _tree(unit)
    text = {
        "foreign-width": COUNTER.replace("ac.bits[W]", "ac.bits[FOREIGN]"),
        "wide-guard": COUNTER.replace("enable: ac.u1", "enable: ac.bits[W]"),
        "runtime-initializer": COUNTER.replace("= 0\n", "= enable\n"),
        "closed-zero-width": COUNTER.replace(
            ", *, W: int = 8", ", *, W: int = 8, N: int = 4"
        ).replace("count: ac.bits[W]", "count: ac.bits[0]"),
        "closed-initializer-overflow": COUNTER.replace(
            ", *, W: int = 8", ", *, W: int = 8, N: int = 4"
        )
        .replace("ac.bits[W]", "ac.u8")
        .replace("= 0\n", "= 256\n"),
        "noninteger-formal": COUNTER.replace("W: int = 8", "W: bool = True"),
    }[mutation]
    (source / "provider.py").write_text(text)
    result = _compile_replace(source, "provider.py", unit)
    _rejected(
        result,
        "static",
        "scope",
        "unknown",
        "one-bit",
        "one bit",
        "boolean",
        "Boolean",
        "initializer",
        "guard",
        "width",
        "fit",
        "positive",
        "bounds",
    )
    assert _tree(unit) == before


def test_saved_mixed_instance_anchor_runs_without_sources(tmp_path):
    source, provider, bench = _published(tmp_path, BENCH)
    anchor = tmp_path / "anchor"
    _ok(
        _cli(
            "link",
            str(provider),
            str(bench),
            "--top",
            "checks.bench.Exercise",
            "--target",
            "anchor",
            "-o",
            str(anchor),
        )
    )
    manifest = json.loads((anchor / "anchor.json").read_text())
    assert any(
        '"ac.table.read"' in (anchor / row["body"]).read_text()
        for row in manifest["definitions"]
    ), "saved bodies must retain the intrinsic checked-read opcode"
    for directory in (source, provider, bench):
        shutil.rmtree(directory)
    before = _tree(anchor)
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    final = tmp_path / "final.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    _execute(final, tmp_path)
    assert '"ac.table.read"' in final.read_text()
    assert _tree(anchor) == before


PROBE = """import pycircuit as ac
from checks.provider import Counter
@ac.module
def Probe(enable: ac.u1, other: ac.u1, fail: ac.u1) -> ac.bits[11]:
    a = Counter(1, W=3)
    b = Counter(enable, W=5)
    c = Counter(other, W=3)
    @ac.rule
    def check():
        assert fail == 0, 'deliberate whole-system failure'
    return ac.concat(a, b, c)
"""


LIFECYCLE_CPP = r"""#include "pycircuit_system.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using Bit = gfsim::wire<gfsim::Bits<1>>;
static Bit bit(unsigned value) {
  if (value < 2) return Bit::known(gfsim::Bits<1>{value});
  return Bit::fromPacked(value == 2 ? gfsim::FourState<1>::unknown()
                                  : gfsim::FourState<1>::highImpedance());
}
static void require(bool ok) { if (!ok) throw std::runtime_error("state parameter lifecycle"); }
int main() {
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    auto drive = [&](unsigned enable, unsigned other, unsigned fail, bool clock, bool reset=false) {
      dut.drive({bit(enable),bit(other),bit(fail),bit(clock),bit(reset)});
    };
    drive(1,0,0,false); dut.system().Build();
    auto root = dut.root_->pyc_implementation;
    auto a = root->pyc_instance_a->pyc_instance_count_state;
    auto b = root->pyc_instance_b->pyc_instance_count_state;
    auto c = root->pyc_instance_c->pyc_instance_count_state;
    auto snapshot = [&]() {
      return std::array<std::uint64_t,12>{
        a->current(0).q.packed().value().value(), a->current(0).q.packed().knownMask().value(), a->current(0).q.packed().zMask().value(), a->current(0).clock,
        b->current(0).q.packed().value().value(), b->current(0).q.packed().knownMask().value(), b->current(0).q.packed().zMask().value(), b->current(0).clock,
        c->current(0).q.packed().value().value(), c->current(0).q.packed().knownMask().value(), c->current(0).q.packed().zMask().value(), c->current(0).clock};
    };
    auto current = [&](unsigned x,unsigned y,unsigned z,bool clock) {
      require(snapshot()==std::array<std::uint64_t,12>{x,7,0,clock,y,31,0,clock,z,7,0,clock});
    };
    auto reset = [&]() { drive(1,0,0,false); dut.system().Reset(); current(0,0,0,false); };
    reset();
    const auto initial=snapshot(); drive(1,0,0,true);
    dut.root_->Work(); dut.root_->DiscardNext(); dut.observations().DiscardNext();
    require(snapshot()==initial && dut.system().cycle()==0);
    require(dut.system().Step()==gfsim::SimStepResult::Running); current(1,1,0,true);
    require(dut.sample().result.value().value()==0); // Work samples old Q.
    require(dut.system().Step()==gfsim::SimStepResult::Running); current(1,1,0,true);
    unsigned x=1,y=1,z=0;
    for(unsigned cycle=1;cycle<70;++cycle) {
      drive(cycle%2,1-cycle%2,0,false);
      require(dut.system().Step()==gfsim::SimStepResult::Running); current(x,y,z,false);
      drive(cycle%2,1-cycle%2,0,true);
      require(dut.system().Step()==gfsim::SimStepResult::Running);
      require(dut.sample().result.value().value()==((x<<8)|(y<<3)|z));
      x=(x+1)&7; y=(y+cycle%2)&31; z=(z+1-cycle%2)&7; current(x,y,z,true);
    }
    drive(1,1,0,false); require(dut.system().Step()==gfsim::SimStepResult::Running);
    drive(1,1,0,true,true); require(dut.system().Step()==gfsim::SimStepResult::Running); current(0,0,0,true);
    for(unsigned failure : {1u,2u,3u}) {
      reset(); drive(1,0,0,true); require(dut.system().Step()==gfsim::SimStepResult::Running);
      drive(1,0,0,false); require(dut.system().Step()==gfsim::SimStepResult::Running);
      const auto before=snapshot(); const auto epoch=dut.system().cycle();
      drive(failure==1?1:failure,1,failure==1,true);
      require(dut.system().Step()==gfsim::SimStepResult::Failed);
      require(snapshot()==before && dut.system().cycle()==epoch);
      require(dut.system().Step()==gfsim::SimStepResult::Failed && snapshot()==before);
      reset(); drive(1,1,0,true); require(dut.system().Step()==gfsim::SimStepResult::Running); current(1,1,1,true);
    }
  }
  std::cout << "PARAMETER_STATE_LIFECYCLE_CPP_OK\n";
}
"""


LIFECYCLE_RTL = r"""module lifecycle;
logic enable=1, other=0, fail=0, clk=0, rst=0, permit=0;
logic [2:0] phase=0;
wire [10:0] result;
wire error, complete, sampling_error;
pyc_root dut(.enable(enable),.other(other),.fail(fail),.result(result),
 .pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),.pyc_phase(phase),
 .pyc_root_commit_ok(permit),.pyc_local_error(error),
 .pyc_check_complete(complete),.pyc_sampling_error(sampling_error));
task current(input integer a,b,c,input logic clock);
  if(dut.dut.pyc_instance_a.pyc_instance_count.q_current !== a ||
     dut.dut.pyc_instance_b.pyc_instance_count.q_current !== b ||
     dut.dut.pyc_instance_c.pyc_instance_count.q_current !== c) $fatal(1,"peer state");
  if(dut.dut.pyc_instance_a.pyc_instance_count.managed.clock_current !== clock ||
     dut.dut.pyc_instance_b.pyc_instance_count.managed.clock_current !== clock ||
     dut.dut.pyc_instance_c.pyc_instance_count.managed.clock_current !== clock) $fatal(1,"peer clock");
endtask
task prepare;
  #1; phase=1; #1;
endtask
task commit;
  if(error!==0 || complete!==1 || sampling_error!==0) $fatal(1,"unexpected error");
  permit=1; phase=2; #1; phase=0; permit=0; #1;
endtask
task discard;
  permit=0; phase=2; #1; phase=3; #1; phase=0; #1;
endtask
task reset_model;
  enable=1;other=0;fail=0;clk=0;rst=0;permit=1;phase=4;#1;
  phase=2;#1;phase=0;permit=0;#1;current(0,0,0,0);
endtask
integer a,b,c,i,f;
initial begin
  reset_model(); clk=1;prepare();discard();current(0,0,0,0);
  prepare();if(result!==0)$fatal;commit();current(1,1,0,1);
  prepare();commit();current(1,1,0,1);
  a=1;b=1;c=0;
  for(i=1;i<70;i=i+1) begin
    enable=i%2;other=1-i%2;clk=0;prepare();commit();current(a,b,c,0);
    clk=1;prepare();if(result!==((a<<8)|(b<<3)|c))$fatal(1,"old Q");commit();
    a=(a+1)&7;b=(b+i%2)&31;c=(c+1-i%2)&7;current(a,b,c,1);
  end
  clk=0;prepare();commit();clk=1;rst=1;prepare();commit();current(0,0,0,1);
  for(f=1;f<4;f=f+1) begin
    reset_model();clk=1;prepare();commit();clk=0;prepare();commit();
    other=1;fail=(f==1);enable=(f==1)?1:(f==2?1'bx:1'bz);clk=1;prepare();
    if(error!==1 && sampling_error!==1)$fatal(1,"missing whole-system error");
    discard();current(1,1,0,0);
    reset_model();other=1;clk=1;prepare();commit();current(1,1,1,1);
  end
  $display("PARAMETER_STATE_LIFECYCLE_RTL_OK");$finish;
end
endmodule
"""


def _lifecycle(final, tmp_path):
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    generated = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(generated)))
    # Test-owned observation of emitted state never writes a DUT result.
    for header in generated.rglob("*.hpp"):
        header.write_text(header.read_text().replace("private:", "public:"))
    (generated / "lifecycle.cpp").write_text(LIFECYCLE_CPP)
    with (generated / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(lifecycle lifecycle.cpp)\n"
            "target_link_libraries(lifecycle PRIVATE pycircuit_modules)\n"
        )
    build = tmp_path / "cpp-build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(generated),
                "-B",
                str(build),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={_install() / 'share/pycircuit/cmake'}",
            ]
        )
    )
    _ok(_run(["cmake", "--build", str(build), "--target", "lifecycle"]))
    assert "PARAMETER_STATE_LIFECYCLE_CPP_OK" in _ok(
        _run([str(next(build.rglob("lifecycle")))])
    )
    rtl = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(rtl)))
    bench = tmp_path / "lifecycle.sv"
    bench.write_text(LIFECYCLE_RTL)
    manifest = json.loads((rtl / "generated.json").read_text())
    sources = [rtl / row["path"] for row in manifest["files"] if row["role"] == "rtl"]
    binary = tmp_path / "lifecycle.vvp"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "lifecycle",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dffe.v"),
                *map(str, sources),
                str(bench),
            ]
        )
    )
    assert "PARAMETER_STATE_LIFECYCLE_RTL_OK" in _ok(_run(["vvp", str(binary)]))
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def test_different_sized_peers_discard_reset_failure_and_wrap(tmp_path):
    _, provider, bench = _published(tmp_path, PROBE)
    final = tmp_path / "probe.ac"
    _ok(_link([provider, bench], "checks.bench.Probe", final))
    _lifecycle(final, tmp_path)


COUNTER_PROVIDER = (
    COUNTER
    + """
@ac.module
def Parent(enable: ac.u1, *, WIDTH: int = 8) -> ac.bits[WIDTH]:
    child = Counter(enable, W=WIDTH)
    return child
"""
)


def _counter_program(tmp_path, consumer):
    """Isolate scalar-state progress without weakening the full Bank matrix."""
    source = _source(tmp_path, COUNTER_PROVIDER)
    (source / "bench.py").write_text(consumer)
    provider, bench = tmp_path / "provider", tmp_path / "bench"
    _ok(_compile(source, "provider.py", provider))
    before = _tree(provider)
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "hidden-body.ac"
    (source / "provider.py").unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "bench.py", bench, [provider]))
    finally:
        hidden.rename(body)
    assert _tree(provider) == before
    return provider, bench


@pytest.mark.parametrize(
    "root_override", [False, True], ids=["closed-root", "generic-root"]
)
def test_counter_only_generic_lifecycle_checkpoint(tmp_path, root_override):
    consumer = PROBE
    if root_override:
        consumer = consumer.replace(
            "fail: ac.u1) -> ac.bits[11]",
            "fail: ac.u1, *, W: int = 8) -> ac.bits[W + 6]",
        ).replace("Counter(enable, W=5)", "Counter(enable, W=W)")
    provider, bench = _counter_program(tmp_path, consumer)
    final = tmp_path / "probe.ac"
    actuals = _parameters(tmp_path / "actuals.json", W=5) if root_override else None
    _ok(_link([provider, bench], "checks.bench.Probe", final, actuals))
    _lifecycle(final, tmp_path)


def test_counter_only_parameterized_parent_checkpoint(tmp_path):
    provider, bench = _counter_program(
        tmp_path,
        """import pycircuit as ac
from checks.provider import Counter, Parent
@ac.system
def ExerciseCounterParents():
    phase: ac.u8 = 0
    @ac.rule
    def advance():
        phase = phase + 1
    a = Parent(1, WIDTH=3)
    b = Parent(phase[0], WIDTH=5)
    c = Parent(~phase[0], WIDTH=3)
    @ac.rule
    def observe():
        ac.log('info', 'counter parents', a, b, c)
""",
    )
    final = tmp_path / "parents.ac"
    _ok(_link([provider, bench], "checks.bench.ExerciseCounterParents", final))
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    expected = []
    a = b = c = 0
    for cycle in range(70):
        expected.extend([(a, b, c)] * 2)
        a, b, c = (a + 1) % 8, (b + cycle % 2) % 32, (c + 1 - cycle % 2) % 8
    transcripts = []
    for target in ("cpp", "verilog"):
        directory = tmp_path / target
        directory.mkdir()
        binary = _bundle(final, directory, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            args = (
                ["--cycles", "70", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=70"]
            )
            records = _records(_ok(_run([str(binary), *args], timeout=60)))
            logs = [row for row in records if row["kind"] == "log"]
            assert [
                tuple(int(v["value"]) for v in row["values"]) for row in logs
            ] == expected
            assert [int(row["evaluation_epoch"]) for row in logs] == list(range(140))
            transcripts.append(logs)
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


@pytest.mark.parametrize("width", [3, 5])
def test_counter_root_override_execution_checkpoint(tmp_path, width):
    source = _source(tmp_path, COUNTER)
    unit, final = tmp_path / "unit", tmp_path / "root.ac"
    _ok(_compile(source, "provider.py", unit))
    _ok(
        _link(
            [unit],
            "checks.provider.Counter",
            final,
            _parameters(tmp_path / "actuals.json", W=width),
        )
    )
    _execute_counter_root(final, tmp_path, width)


def _execute_counter_root(final, tmp_path, width):
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    cpp = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(cpp)))
    (cpp / "root_probe.cpp").write_text(
        r"""#include "pycircuit_system.hpp"
#include <iostream>
#include <stdexcept>
using Bit=gfsim::wire<gfsim::Bits<1>>;
static Bit bit(bool value){return Bit::known(gfsim::Bits<1>{value});}
static void require(bool ok){if(!ok)throw std::runtime_error("root actual binding");}
int main(){
  constexpr unsigned mask=(1u<<WIDTH)-1;
  for(unsigned workers:{1u,2u}){
    pyc_dut dut(workers);unsigned expected=0;
    auto step=[&](bool enable,bool clock,bool reset=false){
      dut.drive({bit(enable),bit(clock),bit(reset)});
      require(dut.system().Step()==gfsim::SimStepResult::Running);
      require(dut.sample().result.value().value()==expected);
    };
    dut.drive({bit(0),bit(0),bit(0)});dut.system().Build();dut.system().Reset();
    for(unsigned cycle=0;cycle<100;++cycle){
      bool enable=cycle%3!=1;step(enable,false);step(enable,true);
      expected=(expected+enable)&mask;
    }
    step(true,false);step(true,true,true);expected=0;step(false,false);
  }
  std::cout<<"COUNTER_ROOT_ACTUAL_CPP_OK\n";
}
""".replace("WIDTH", str(width))
    )
    with (cpp / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(root_probe root_probe.cpp)\n"
            "target_link_libraries(root_probe PRIVATE pycircuit_modules)\n"
        )
    build = tmp_path / "cpp-build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(cpp),
                "-B",
                str(build),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={_install() / 'share/pycircuit/cmake'}",
            ]
        )
    )
    _ok(_run(["cmake", "--build", str(build), "--target", "root_probe"]))
    assert "COUNTER_ROOT_ACTUAL_CPP_OK" in _ok(
        _run([str(next(build.rglob("root_probe")))])
    )
    rtl = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(rtl)))
    bench = tmp_path / "root_probe.sv"
    bench.write_text(
        r"""module root_probe;
localparam W=WIDTH;
logic enable=0,clk=0,rst=0,permit=0;logic[2:0] phase=0;
wire[W-1:0] result;wire error,complete,sampling_error;
pyc_root dut(.enable(enable),.result(result),.pyc_7079635f636c6b(clk),
 .pyc_7079635f727374(rst),.pyc_phase(phase),.pyc_root_commit_ok(permit),
 .pyc_local_error(error),.pyc_check_complete(complete),.pyc_sampling_error(sampling_error));
integer expected=0,i;
task step;
 #1;phase=1;#1;
 if(error!==0||complete!==1||sampling_error!==0||result!==expected)$fatal(1,"root actual binding");
 permit=1;phase=2;#1;phase=0;permit=0;#1;
endtask
initial begin
 permit=1;phase=4;#1;phase=2;#1;phase=0;permit=0;#1;
 for(i=0;i<100;i=i+1)begin
  enable=(i%3!=1);clk=0;step();clk=1;step();expected=(expected+enable)&((1<<W)-1);
 end
 enable=1;clk=0;step();clk=1;rst=1;step();expected=0;clk=0;rst=0;enable=0;step();
 $display("COUNTER_ROOT_ACTUAL_RTL_OK");$finish;
end
endmodule
""".replace("WIDTH", str(width))
    )
    manifest = json.loads((rtl / "generated.json").read_text())
    sources = [rtl / row["path"] for row in manifest["files"] if row["role"] == "rtl"]
    binary = tmp_path / "root.vvp"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "root_probe",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dffe.v"),
                *map(str, sources),
                str(bench),
            ]
        )
    )
    assert "COUNTER_ROOT_ACTUAL_RTL_OK" in _ok(_run(["vvp", str(binary)]))
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


RAW_BANK = """import pycircuit as ac
@ac.module
def RawBank(enable: ac.u1, index: ac.u2, *, N: int = 4, W: int = 3) -> ac.bits[W]:
    entries: ac.table[N, ac.bits[W]] = 3
    @ac.rule
    def update():
        if enable:
            entries[index] = entries[index] + 1
    return entries[index]
"""


def test_direct_index_actual_bounds_reject_without_replacement(tmp_path):
    source = _source(tmp_path, RAW_BANK)
    unit = tmp_path / "unit"
    _ok(_compile(source, "provider.py", unit))
    final = tmp_path / "final.ac"
    _ok(
        _link(
            [unit],
            "checks.provider.RawBank",
            final,
            _parameters(tmp_path / "valid.json", N=4, W=3),
        )
    )
    before = final.read_bytes()
    # The full two-bit selector permits 3. Neither depth 1 nor 3 covers it.
    # These accessors have no modulo, clamping, or caller-supplied range hint.
    for depth in (1, 3):
        result = _link(
            [unit],
            "checks.provider.RawBank",
            final,
            _parameters(tmp_path / f"depth-{depth}.json", N=depth, W=3),
            replace=True,
        )
        _rejected(result, "index", "range", "bound")
        assert final.read_bytes() == before


def test_write_only_invalid_selector_cannot_hide_behind_checked_read(tmp_path):
    source = _source(
        tmp_path,
        """import pycircuit as ac
@ac.module
def WriteOnly(enable: ac.u1, index: ac.u2, *, N: int = 4) -> ac.table[N, ac.u8]:
    entries: ac.table[N, ac.u8] = 0
    @ac.rule
    def write():
        if enable:
            entries[index] = 7
    return entries
""",
    )
    unit, final = tmp_path / "unit", tmp_path / "final.ac"
    _ok(_compile(source, "provider.py", unit))
    receipt = json.loads((unit / "unit.json").read_text())
    body = (unit / receipt["files"]["body"]).read_text()
    assert '"ac.state.write"' in body
    assert '"ac.table.read"' not in body and '"ac.table.get"' not in body
    _ok(
        _link(
            [unit],
            "checks.provider.WriteOnly",
            final,
            _parameters(tmp_path / "valid.json", N=4),
        )
    )
    before = final.read_bytes()
    result = _link(
        [unit],
        "checks.provider.WriteOnly",
        final,
        _parameters(tmp_path / "invalid.json", N=3),
        replace=True,
    )
    _rejected(result, "index", "range", "bound")
    assert final.read_bytes() == before


def test_fixed_table_conditional_index_keeps_existing_source_range_proof(tmp_path):
    source = _source(
        tmp_path,
        """import pycircuit as ac
@ac.module
def Fixed(choose: ac.u1) -> ac.table[3, ac.u8]:
    entries: ac.table[3, ac.u8] = 0
    @ac.rule
    def write():
        index: ac.u64 = 1 if choose else 2
        entries[index] = 7
    return entries
@ac.system
def Exercise():
    choose: ac.u1 = 0
    rows = Fixed(choose)
    @ac.rule
    def step():
        ac.log('info', 'conditional index', rows[0], rows[1], rows[2])
        choose = ~choose
""",
    )
    unit, final = tmp_path / "unit", tmp_path / "final.ac"
    _ok(_compile(source, "provider.py", unit))
    _ok(_link([unit], "checks.provider.Exercise", final))
    transcripts = []
    expected = [
        row for row in [(0, 0, 0), (0, 0, 7), (0, 7, 7), (0, 7, 7)] for _ in range(2)
    ]
    for target in ("cpp", "verilog"):
        directory = tmp_path / target
        directory.mkdir()
        binary = _bundle(final, directory, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "4", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=4"]
            )
            records = _records(_ok(_run([str(binary), *arguments])))
            logs = [row for row in records if row["kind"] == "log"]
            assert [
                tuple(int(v["value"]) for v in row["values"]) for row in logs
            ] == expected
            transcripts.append(logs)
    assert transcripts[0] == transcripts[1] == transcripts[2]


@pytest.mark.parametrize("proof", ["comparison", "remainder"])
def test_fixed_table_input_range_proofs_execute_every_slot(tmp_path, proof):
    bounded = "index if index < 5 else 0" if proof == "comparison" else "index % 5"
    source = _source(
        tmp_path,
        f"""import pycircuit as ac
@ac.module
def Fixed(index: ac.u8) -> ac.table[5, ac.u8]:
    entries: ac.table[5, ac.u8] = 0
    @ac.rule
    def write():
        bounded = {bounded}
        entries[bounded] = 7
    return entries
@ac.system
def Exercise():
    phase: ac.u8 = 0
    rows = Fixed(phase + 3)
    @ac.rule
    def step():
        ac.log('info', 'bounded table input', rows[0], rows[1], rows[2], rows[3], rows[4])
        phase = phase + 1
""",
    )
    unit, final = tmp_path / "unit", tmp_path / "final.ac"
    _ok(_compile(source, "provider.py", unit))
    _ok(_link([unit], "checks.provider.Exercise", final))
    expected, contents = [], [0] * 5
    for raw in range(3, 19):
        expected.extend([tuple(contents)] * 2)
        position = raw % 5 if proof == "remainder" else raw if raw < 5 else 0
        contents[position] = 7
    assert sum(contents) == (35 if proof == "remainder" else 21)
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    transcripts = []
    for target in ("cpp", "verilog"):
        directory = tmp_path / target
        directory.mkdir()
        binary = _bundle(final, directory, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            args = (
                ["--cycles", "16", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=16"]
            )
            records = _records(_ok(_run([str(binary), *args], timeout=60)))
            logs = [row for row in records if row["kind"] == "log"]
            assert [
                tuple(int(v["value"]) for v in row["values"]) for row in logs
            ] == expected
            assert [int(row["evaluation_epoch"]) for row in logs] == list(range(32))
            transcripts.append(logs)
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


def _checked_index_dag(depth, sharing):
    """Build O(depth) SSA with no serialized proof or source-range annotation."""
    provenance = (
        'location = {path = "dag.py", line = 1 : i64, column = 1 : i64, '
        "end_line = 1 : i64, end_column = 2 : i64}, "
        "origin = {site = {definition = @Root, ast_path = []}, expansion = []}"
    )
    aliases = "\n".join(
        f'#n{n} = #ac.static_expr<{{kind = "literal", {provenance}, '
        f'value = {{kind = "integer", value = #ac.math_int<{n}>}}}}>'
        for n in (1, 2, 5, 8, 64)
    )
    lines = [
        '%left0 = "ac.bits.constant"() {value = #n1} : () -> !wide',
        '%right0 = "ac.bits.constant"() {value = #n2} : () -> !wide',
    ]
    for level in range(1, depth + 1):
        previous = level - 1
        right = f"%left{previous}" if sharing == "same" else f"%right{previous}"
        lines.append(
            f'%left{level} = "ac.bits.select"(%choose, %left{previous}, {right}) : (!bit, !wide, !wide) -> !wide'
        )
        if sharing == "diamond":
            lines.append(
                f'%right{level} = "ac.bits.select"(%choose, %right{previous}, %left{previous}) : (!bit, !wide, !wide) -> !wide'
            )
    lines += [
        f'%result = "ac.table.read"(%values, %left{depth}) : (!table, !wide) -> !byte',
        '"ac.yield"(%result) : (!byte) -> ()',
    ]
    return (
        aliases
        + "\n!bit = !ac.bits<#n1>\n!byte = !ac.bits<#n8>\n!wide = !ac.bits<#n64>\n!table = !ac.table<[#n5], !byte>\n"
        + """module {
  "ac.module"() ({
  ^bb0(%values: !table, %choose: !bit):
    """
        + "\n    ".join(lines)
        + """
  }) {sym_name = "Root", source_owner = {package = "", path = "dag.py"},
      parameters = [], type_parameters = [], function_type = (!table, !bit) -> !byte,
      input_names = ["values", "choose"], output_names = ["result"]} : () -> ()
}
"""
    )


@pytest.mark.parametrize("sharing", ["same", "diamond"])
@pytest.mark.parametrize("depth", [0, 24, 32])
def test_checked_index_shared_select_dag_has_bounded_verification(
    tmp_path, sharing, depth
):
    source, output = tmp_path / "dag.ac", tmp_path / "verified.ac"
    source.write_text(_checked_index_dag(depth, sharing))
    assert source.read_text().count('"ac.bits.select"') == depth * (
        2 if sharing == "diamond" else 1
    )
    optimizer = str(Path(os.environ["PYCIRCUIT_LINKER"]).with_name("pycircuit-opt"))
    # A correct shared-DAG proof is tiny. TimeoutExpired is a failing outcome,
    # never evidence of rejection or successful bounded verification.
    _ok(_run([optimizer, str(source), "-o", str(output)], timeout=5))
    assert '"ac.table.read"' in output.read_text()


def test_unknown_direct_table_read_preserves_full_selector(tmp_path):
    source = _source(tmp_path, RAW_BANK)
    (source / "bench.py").write_text("""import pycircuit as ac
from checks.provider import RawBank
@ac.system(samples=((1, 0),))
def ExerciseUnknown():
    known = RawBank(0, 3, N=4, W=3)
    high_x = RawBank(0, ac.four_state('x0'), N=4, W=3)
    high_z = RawBank(0, ac.four_state('z0'), N=4, W=3)
    @ac.rule
    def observe():
        ac.log('info', 'unknown state index', known, high_x, high_z)
""")
    provider, bench = tmp_path / "provider", tmp_path / "bench"
    _ok(_compile(source, "provider.py", provider))
    _ok(_compile(source, "bench.py", bench, [provider]))
    final = tmp_path / "final.ac"
    _ok(_link([provider, bench], "checks.bench.ExerciseUnknown", final))
    transcripts = []
    for target in ("cpp", "verilog"):
        directory = tmp_path / target
        directory.mkdir()
        binary = _bundle(final, directory, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = ["--workers", str(workers)] if target == "cpp" else []
            records = _records(_ok(_run([str(binary), *arguments])))
            logs = [row for row in records if row["kind"] == "log"]
            assert len(logs) == 1
            assert int(logs[0]["values"][0]["value"]) == 3
            assert (
                logs[0]["values"][1:]
                == [{"kind": "bits4", "width": 3, "value": "0", "known": "0", "z": "0"}]
                * 2
            )
            transcripts.append(logs)
    assert transcripts[0] == transcripts[1] == transcripts[2]


def test_resigned_anchor_binding_tamper_cannot_change_actual_layout(tmp_path):
    source = _source(tmp_path, COUNTER)
    unit = tmp_path / "unit"
    _ok(_compile(source, "provider.py", unit))
    anchor = tmp_path / "anchor"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.provider.Counter",
            "--parameters",
            str(_parameters(tmp_path / "actuals.json", W=3)),
            "--target",
            "anchor",
            "-o",
            str(anchor),
        )
    )
    final = tmp_path / "final.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    before = final.read_bytes()
    manifest = json.loads((anchor / "anchor.json").read_text())
    assert manifest["configuration"]["integer_arguments"] == [
        {"name": "W", "value": "3"}
    ]
    manifest["configuration"]["integer_arguments"][0]["value"] = "5"
    _save_manifest(anchor, manifest)
    # Recompute outer manifest identities, so this is not merely a stale hash test.
    result = _cli(
        "link",
        "--from-anchor",
        str(anchor),
        "--target",
        "final",
        "-o",
        str(final),
        "--replace",
    )
    _rejected(result, "configuration", "argument", "parameter", "binding", "root")
    assert final.read_bytes() == before
    native = _run(
        [os.environ["PYCIRCUIT_LINKER"], "--from-anchor", str(anchor), "--verify-only"]
    )
    assert native.returncode != 0, native.stdout + native.stderr


def test_resigned_checked_read_anchor_rejects_malformed_opcode_without_sources(
    tmp_path,
):
    source = _source(tmp_path, RAW_BANK)
    unit, anchor = tmp_path / "unit", tmp_path / "anchor"
    _ok(_compile(source, "provider.py", unit))
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.provider.RawBank",
            "--parameters",
            str(_parameters(tmp_path / "actuals.json", N=4, W=3)),
            "--target",
            "anchor",
            "-o",
            str(anchor),
        )
    )
    shutil.rmtree(source)
    shutil.rmtree(unit)
    final = tmp_path / "final.ac"
    _ok(
        _cli(
            "link", "--from-anchor", str(anchor), "--target", "final", "-o", str(final)
        )
    )
    before = final.read_bytes()
    manifest = json.loads((anchor / "anchor.json").read_text())
    definition = next(
        row
        for row in manifest["definitions"]
        if '"ac.table.read"' in (anchor / row["body"]).read_text()
    )
    body = anchor / definition["body"]
    body.write_text(body.read_text().replace('"ac.table.read"', '"ac.table.get"', 1))
    definition["body_sha256"] = hashlib.sha256(body.read_bytes()).hexdigest()
    old_variant = definition["variant_id"]
    new_variant = hashlib.sha256(
        _json(
            {
                key: definition[key]
                for key in ("source", "definition", "spec_sha256", "body_sha256")
            }
        )
    ).hexdigest()
    (anchor / "definitions" / old_variant).rename(anchor / "definitions" / new_variant)
    definition["variant_id"] = new_variant
    for role in ("spec", "body"):
        definition[role] = f"definitions/{new_variant}/{role}.ac"
    for row in manifest["files"]:
        if row["path"].startswith(f"definitions/{old_variant}/"):
            row["path"] = row["path"].replace(old_variant, new_variant)
            if row["role"] == "body":
                row["sha256"] = definition["body_sha256"]
    manifest["files"].sort(key=lambda row: row["path"])
    configuration = manifest["configuration"]
    if configuration["root"]["variant_id"] == old_variant:
        configuration["root"]["variant_id"] = new_variant
    configuration["definitions"] = [
        {
            key: row[key]
            for key in ("definition", "variant_id", "spec_sha256", "body_sha256")
        }
        for row in manifest["definitions"]
    ]
    _save_manifest(anchor, manifest)
    # All inventory, variant and configuration digests are consistent. The
    # replacement now has raw TableGet's two-result ABI but only one result.
    result = _cli(
        "link",
        "--from-anchor",
        str(anchor),
        "--target",
        "final",
        "-o",
        str(final),
        "--replace",
    )
    _rejected(result, "table.get", "result", "operation")
    assert final.read_bytes() == before
    native = _run(
        [os.environ["PYCIRCUIT_LINKER"], "--from-anchor", str(anchor), "--verify-only"]
    )
    _rejected(native, "table.get", "result", "operation")


ENTRY_COUNTER = (
    COUNTER
    + """
@ac.module
def Closed(enable: ac.u1) -> ac.u3:
    return Counter(enable, W=3)
"""
)

ENTRY_PARENT = (
    COUNTER_PROVIDER
    + """
@ac.module
def Closed(enable: ac.u1) -> ac.u3:
    return Parent(enable, WIDTH=3)
"""
)

ENTRY_BANK = """import pycircuit as ac
@ac.module
def Bank(enable: ac.u1, *, N: int = 4, W: int = 8) -> ac.table[N, ac.bits[W]]:
    entries: ac.table[N, ac.bits[W]] = 3
    @ac.rule
    def increment():
        last: ac.u64 = N - 1
        if enable:
            entries[last] = entries[last] + 1
    return entries
@ac.module
def Closed(enable: ac.u1) -> ac.table[3, ac.u3]:
    return Bank(enable, N=3, W=3)
"""


def _entry_units(tmp_path, kind):
    text = {"counter": ENTRY_COUNTER, "parent": ENTRY_PARENT, "bank": ENTRY_BANK}[kind]
    source = _source(tmp_path, text)
    unit = tmp_path / "unit"
    _ok(_compile(source, "provider.py", unit))
    return source, unit


def _entry_export_contract(final, logical, width, *, boundary=True):
    payload = json.loads(
        _ok(_run([os.environ["PYCIRCUIT_EMITTER"], str(final), "--target", "cpp"]))
    )
    assert payload["entry"]["definition"] == f'@"checks.provider.{logical}"'
    implementation = payload["root_cpp_name"]
    assert implementation in payload["system_header"]
    assert "using pyc_root = " in payload["system_header"]
    assert "class pyc_dut" in payload["system_header"]
    class_name = implementation.rsplit("::", 1)[-1]
    assert any(
        f"class {class_name}" in row["header"] for row in payload["source_groups"]
    )
    ports = payload["port_availability"]
    assert [(row["name"], row["width"]) for row in ports["inputs"]] == [
        ("enable", 1),
        ("pyc_clk", 1),
        ("pyc_rst", 1),
    ]
    assert [(row["name"], row["width"]) for row in ports["outputs"]] == [
        ("result", width)
    ]
    assert ("ac.entry_boundary" in final.read_text()) == boundary
    if boundary:
        assert "ac.routed_contract = 3 : i64" in final.read_text()
        assert "pycircuit.__link_entry__" in final.read_text()
    return ports


@pytest.mark.parametrize("kind", ["counter", "parent"])
@pytest.mark.parametrize("selected", ["logical", "closed"])
def test_entry_boundary_scalar_and_forwarded_roots_execute_once(
    tmp_path, kind, selected
):
    _, unit = _entry_units(tmp_path, kind)
    logical = "Counter" if kind == "counter" else "Parent"
    actual_name = "W" if kind == "counter" else "WIDTH"
    width = 5 if selected == "logical" else 3
    top = logical if selected == "logical" else "Closed"
    parameters = (
        _parameters(tmp_path / "actuals.json", **{actual_name: width})
        if selected == "logical"
        else None
    )
    final = tmp_path / "final.ac"
    _ok(_link([unit], "checks.provider." + top, final, parameters))
    _entry_export_contract(final, top, width, boundary=selected == "logical")
    _execute_counter_root(final, tmp_path, width)


def _entry_counter_anchor(tmp_path):
    source, unit = _entry_units(tmp_path, "counter")
    actuals = _parameters(tmp_path / "actuals.json", W=5)
    direct, anchor = tmp_path / "direct.ac", tmp_path / "anchor"
    _ok(_link([unit], "checks.provider.Counter", direct, actuals))
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.provider.Counter",
            "--parameters",
            str(actuals),
            "--target",
            "anchor",
            "-o",
            str(anchor),
        )
    )
    manifest = json.loads((anchor / "anchor.json").read_text())
    assert (manifest["schema"], manifest["routed_contract"]) == (2, 3)
    assert manifest["root"] == '@"checks.provider.Counter"'
    config = manifest["configuration"]
    assert config["root"]["definition"] == manifest["root"]
    assert config["implementation_root"]["definition"] == '@"pycircuit.__link_entry__"'
    assert config["integer_arguments"] == [{"name": "W", "value": "5"}]
    definitions = {row["definition"] for row in manifest["definitions"]}
    assert (
        manifest["root"] in definitions
        and config["implementation_root"]["definition"] in definitions
    )
    assert '@"checks.provider.Closed"' not in definitions
    shutil.rmtree(source)
    shutil.rmtree(unit)
    return direct, anchor


def test_entry_boundary_schema2_portable_reload_keeps_logical_ports(tmp_path):
    direct, anchor = _entry_counter_anchor(tmp_path)
    portable = tmp_path / "relocated" / "portable-anchor"
    shutil.copytree(anchor, portable)
    shutil.rmtree(anchor)
    anchor = portable
    before = _tree(anchor)
    _ok(_cli("link", "--from-anchor", str(anchor), "--verify-only"))
    reloaded = tmp_path / "reloaded.ac"
    _ok(
        _cli(
            "link",
            "--from-anchor",
            str(anchor),
            "--target",
            "final",
            "-o",
            str(reloaded),
        )
    )
    assert _entry_export_contract(direct, "Counter", 5) == _entry_export_contract(
        reloaded, "Counter", 5
    )
    _execute_counter_root(reloaded, tmp_path, 5)
    assert _tree(anchor) == before


@pytest.mark.parametrize(
    "mutation",
    ["missing-implementation", "swap-roots", "actual", "downgrade", "extra-key"],
)
def test_entry_boundary_schema2_mutations_preserve_final(tmp_path, mutation):
    final, anchor = _entry_counter_anchor(tmp_path)
    before = final.read_bytes()
    manifest = json.loads((anchor / "anchor.json").read_text())
    config = manifest["configuration"]
    if mutation == "missing-implementation":
        del config["implementation_root"]
    elif mutation == "swap-roots":
        config["root"], config["implementation_root"] = (
            config["implementation_root"],
            config["root"],
        )
    elif mutation == "actual":
        config["integer_arguments"][0]["value"] = "3"
    elif mutation == "downgrade":
        manifest["schema"], manifest["routed_contract"] = 1, 2
        del config["implementation_root"]
    else:
        config["implementation_root"]["trusted"] = True
    _save_manifest(anchor, manifest)
    result = _cli(
        "link",
        "--from-anchor",
        str(anchor),
        "--target",
        "final",
        "-o",
        str(final),
        "--replace",
    )
    assert result.returncode != 0, result.stdout + result.stderr
    assert "Traceback" not in result.stderr
    assert final.read_bytes() == before
    native = _run(
        [os.environ["PYCIRCUIT_LINKER"], "--from-anchor", str(anchor), "--verify-only"]
    )
    assert native.returncode != 0, native.stdout + native.stderr


def _entry_dual_probe(final, tmp_path, cpp_text, rtl_text):
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    generated = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(generated)))
    ports = json.loads((generated / "ports.json").read_text())
    availability = ports["outputs"][0]["availability"]
    if "AVAIL" in cpp_text:
        assert availability
        cpp_text = cpp_text.replace("AVAIL", availability)
    cpp_before = _tree(generated)
    # Only the separate diagnostic driver may inspect private state. The
    # emitted library, headers, build manifest, and hardware stay byte-exact.
    driver = tmp_path / "cpp-driver"
    driver.mkdir()
    (driver / "entry_probe.cpp").write_text(cpp_text)
    (driver / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.20)\n"
        "project(entry_probe LANGUAGES CXX)\n"
        f"add_subdirectory([=[{generated}]=] generated)\n"
        "add_executable(entry_probe entry_probe.cpp)\n"
        "target_compile_features(entry_probe PRIVATE cxx_std_20)\n"
        "target_compile_options(entry_probe PRIVATE -fno-access-control)\n"
        "target_link_libraries(entry_probe PRIVATE pycircuit_modules)\n"
    )
    build = tmp_path / "cpp-build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(driver),
                "-B",
                str(build),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={_install() / 'share/pycircuit/cmake'}",
            ]
        )
    )
    _ok(_run(["cmake", "--build", str(build), "--target", "entry_probe"]))
    assert "ENTRY_PROBE_CPP_OK" in _ok(_run([str(next(build.rglob("entry_probe")))]))
    assert _tree(generated) == cpp_before
    rtl = tmp_path / "rtl"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(rtl)))
    rtl_before = _tree(rtl)
    rtl_ports = json.loads((rtl / "ports.json").read_text())
    if "AVAIL" in rtl_text:
        assert rtl_ports["outputs"][0]["availability"]
        rtl_text = rtl_text.replace("AVAIL", rtl_ports["outputs"][0]["availability"])
    bench = tmp_path / "entry_probe.sv"
    bench.write_text(rtl_text)
    manifest = json.loads((rtl / "generated.json").read_text())
    sources = [rtl / row["path"] for row in manifest["files"] if row["role"] == "rtl"]
    binary = tmp_path / "entry.vvp"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "entry_probe",
                "-o",
                str(binary),
                str(_install() / "include/verilog/dffe.v"),
                *map(str, sources),
                str(bench),
            ]
        )
    )
    assert "ENTRY_PROBE_RTL_OK" in _ok(_run(["vvp", str(binary)]))
    assert _tree(generated) == cpp_before
    assert _tree(rtl) == rtl_before
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest


@pytest.mark.parametrize("selected", ["logical", "closed"])
def test_entry_boundary_table_actuals_preserve_every_slot(tmp_path, selected):
    _, unit = _entry_units(tmp_path, "bank")
    extent = width = 5 if selected == "logical" else 3
    top = "Bank" if selected == "logical" else "Closed"
    actuals = (
        _parameters(tmp_path / "actuals.json", N=extent, W=width)
        if selected == "logical"
        else None
    )
    final = tmp_path / "final.ac"
    _ok(_link([unit], "checks.provider." + top, final, actuals))
    _entry_export_contract(final, top, width * extent, boundary=selected == "logical")
    cpp = r"""#include "pycircuit_system.hpp"
#include <iostream>
#include <stdexcept>
using Bit=gfsim::wire<gfsim::Bits<1>>;
static Bit bit(bool v){return Bit::known(gfsim::Bits<1>{v});}
static void require(bool ok){if(!ok)throw std::runtime_error("entry table actuals");}
int main(){for(unsigned workers:{1u,2u}){
 pyc_dut dut(workers);unsigned last=3;dut.drive({bit(0),bit(0),bit(0)});dut.system().Build();dut.system().Reset();
 auto step=[&](bool enable,bool clock,bool reset=false){
  dut.drive({bit(enable),bit(clock),bit(reset)});require(dut.system().Step()==gfsim::SimStepResult::Running);
  auto result=dut.sample().result;
  for(unsigned i=0;i<DEPTH;++i)require(result.element(i).value().value()==(i+1==DEPTH?last:3));
 };
 for(unsigned cycle=0;cycle<100;++cycle){bool enable=cycle%3!=1;step(enable,0);step(enable,1);last=(last+enable)&((1u<<WIDTH)-1);}
 step(1,0);step(1,1,1);last=3;step(0,0);
}std::cout<<"ENTRY_PROBE_CPP_OK\n";}
""".replace("DEPTH", str(extent)).replace("WIDTH", str(width))
    rtl = r"""module entry_probe;
localparam N=DEPTH,W=WIDTH;
logic enable=0,clk=0,rst=0,permit=0;logic[2:0] phase=0;
wire[N*W-1:0] result;wire error,complete,sampling_error;
pyc_root dut(.enable(enable),.result(result),.pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),
 .pyc_phase(phase),.pyc_root_commit_ok(permit),.pyc_local_error(error),.pyc_check_complete(complete),.pyc_sampling_error(sampling_error));
integer last=3,i,j;
task step;
 #1;phase=1;#1;if(error!==0||complete!==1||sampling_error!==0)$fatal;
 for(j=0;j<N;j=j+1)if(((result>>((N-1-j)*W))&((1<<W)-1))!==(j+1==N?last:3))$fatal(1,"entry table slot");
 permit=1;phase=2;#1;phase=0;permit=0;#1;
endtask
initial begin
 permit=1;phase=4;#1;phase=2;#1;phase=0;permit=0;#1;
 for(i=0;i<100;i=i+1)begin enable=(i%3!=1);clk=0;step();clk=1;step();last=(last+enable)&((1<<W)-1);end
 enable=1;clk=0;step();clk=1;rst=1;step();last=3;clk=0;rst=0;enable=0;step();
 $display("ENTRY_PROBE_RTL_OK");$finish;
end
endmodule
""".replace("DEPTH", str(extent)).replace("WIDTH", str(width))
    _entry_dual_probe(final, tmp_path, cpp, rtl)


ENTRY_GUARDED = """import pycircuit as ac
@ac.module
def Guarded(enable: ac.u1, fail: ac.u1, *, W: int = 8) -> ac.bits[W]:
    count: ac.bits[W] = 0
    @ac.rule(guard=lambda: enable)
    def update() -> ac.bits[W]:
        previous = count
        count = count + 1
        assert fail == 0, 'entry rollback'
        return previous
    return update
@ac.module
def Closed(enable: ac.u1, fail: ac.u1) -> ac.u3:
    return Guarded(enable, fail, W=3)
"""


def test_entry_boundary_guarded_zero_availability_and_rollback(tmp_path):
    source = _source(tmp_path, ENTRY_GUARDED)
    unit, final = tmp_path / "unit", tmp_path / "final.ac"
    _ok(_compile(source, "provider.py", unit))
    _ok(
        _link(
            [unit],
            "checks.provider.Guarded",
            final,
            _parameters(tmp_path / "actuals.json", W=5),
        )
    )
    assert "ac.entry_boundary" in final.read_text()
    cpp = r"""#include "pycircuit_system.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using Bit=gfsim::wire<gfsim::Bits<1>>;
static Bit bit(bool v){return Bit::known(gfsim::Bits<1>{v});}
static void require(bool ok){if(!ok)throw std::runtime_error("guarded entry lifecycle");}
int main(){for(unsigned workers:{1u,2u}){
 pyc_dut dut(workers);
 auto drive=[&](bool en,bool fail,bool clock,bool reset=false){dut.drive({bit(en),bit(fail),bit(clock),bit(reset)});};
 drive(1,0,0);dut.system().Build();dut.system().Reset();
 auto state=dut.root_->pyc_implementation->pyc_instance_entry->pyc_instance_count_state;
 auto snapshot=[&](){auto q=state->current(0);return std::array<std::uint64_t,4>{q.q.packed().value().value(),q.q.packed().knownMask().value(),q.q.packed().zMask().value(),q.clock};};
 auto step=[&](bool en,bool clock,unsigned expected,bool reset=false){drive(en,0,clock,reset);require(dut.system().Step()==gfsim::SimStepResult::Running);auto sample=dut.sample();require(sample.AVAIL.value().value()==en);if(en)require(sample.result.value().value()==expected);};
 step(1,0,0);drive(1,0,1);auto before=snapshot();auto epoch=dut.system().cycle();
 dut.root_->Work();dut.root_->DiscardNext();dut.observations().DiscardNext();require(snapshot()==before&&dut.system().cycle()==epoch);
 step(1,1,0);step(0,0,0);step(0,1,0);step(1,0,1);step(1,1,1);
 unsigned expected=2;
 for(unsigned i=0;i<70;++i){step(1,0,expected);step(1,1,expected);expected=(expected+1)&31;}
 step(1,0,expected);before=snapshot();epoch=dut.system().cycle();drive(1,1,1);
 require(dut.system().Step()==gfsim::SimStepResult::Failed);require(snapshot()==before&&dut.system().cycle()==epoch);
 drive(1,0,0);dut.system().Reset();step(1,0,0);step(1,1,0);step(1,0,1);step(1,1,1,true);step(1,0,0);
}std::cout<<"ENTRY_PROBE_CPP_OK\n";}
"""
    rtl = r"""module entry_probe;
logic enable=1,fail=0,clk=0,rst=0,permit=0;logic[2:0] phase=0;
wire[4:0] result;wire available,error,complete,sampling_error;
pyc_root dut(.enable(enable),.fail(fail),.result(result),.AVAIL(available),
 .pyc_7079635f636c6b(clk),.pyc_7079635f727374(rst),.pyc_phase(phase),.pyc_root_commit_ok(permit),
 .pyc_local_error(error),.pyc_check_complete(complete),.pyc_sampling_error(sampling_error));
task prepare(input integer expected);
 #1;phase=1;#1;if(error!==0||complete!==1||sampling_error!==0||available!==enable)$fatal;
 if(enable&&result!==expected)$fatal(1,"guarded valid payload");
endtask
task commit;permit=1;phase=2;#1;phase=0;permit=0;#1;endtask
task discard;permit=0;phase=2;#1;phase=3;#1;phase=0;#1;endtask
task reset_model;enable=1;fail=0;clk=0;rst=0;permit=1;phase=4;#1;phase=2;#1;phase=0;permit=0;#1;endtask
integer expected,i;
initial begin
 reset_model();prepare(0);commit();clk=1;prepare(0);discard();prepare(0);commit();
 enable=0;clk=0;prepare(0);commit();clk=1;prepare(0);commit();enable=1;clk=0;prepare(1);commit();clk=1;prepare(1);commit();
 expected=2;
 for(i=0;i<70;i=i+1)begin clk=0;prepare(expected);commit();clk=1;prepare(expected);commit();expected=(expected+1)&31;end
 clk=0;prepare(expected);commit();clk=1;fail=1;#1;phase=1;#1;if(error!==1)$fatal(1,"missing failed check");discard();
 if(dut.dut.pyc_instance_entry.pyc_instance_count.q_current!==expected||dut.dut.pyc_instance_entry.pyc_instance_count.managed.clock_current!==0)$fatal(1,"failed entry committed");
 reset_model();prepare(0);commit();clk=1;prepare(0);commit();clk=0;prepare(1);commit();clk=1;rst=1;prepare(1);commit();rst=0;clk=0;prepare(0);commit();
 $display("ENTRY_PROBE_RTL_OK");$finish;
end
endmodule
"""
    _entry_dual_probe(final, tmp_path, cpp, rtl)


def test_wide_parameterized_state_carry_wrap_and_independent_instances(tmp_path):
    near129 = (1 << 129) - 2
    near130 = (1 << 130) - 2
    source = _source(
        tmp_path,
        f"""import pycircuit as ac
@ac.module
def Wide(enable: ac.u1, *, W: int = 130) -> ac.bits[W]:
    count: ac.bits[W] = {near129}
    @ac.rule
    def increment():
        if enable:
            count = count + 1
    return count
@ac.module
def Wrap(enable: ac.u1, *, W: int = 131) -> ac.bits[W]:
    count: ac.bits[W] = {near130}
    @ac.rule
    def increment():
        if enable:
            count = count + 1
    return count
""",
        "wide.py",
    )
    (source / "bench.py").write_text("""import pycircuit as ac
from checks.wide import Wide, Wrap
@ac.system
def ExerciseWide():
    phase: ac.u3 = 0
    a = Wide(1, W=129)
    b = Wide(1, W=130)
    c = Wide(phase[0], W=129)
    d = Wide(~phase[0])
    e = Wrap(1, W=130)
    @ac.rule
    def observe():
        ac.log('info', 'wide state words',
               a[:64], a[64:128], a[128:129],
               b[:64], b[64:128], b[128:130],
               c[:64], c[64:128], c[128:129],
               d[:64], d[64:128], d[128:130],
               e[:64], e[64:128], e[128:130])
        phase = phase + 1
""")
    provider, bench = tmp_path / "provider", tmp_path / "bench"
    _ok(_compile(source, "wide.py", provider))
    before = _tree(provider)
    receipt = json.loads((provider / "unit.json").read_text())
    body = provider / receipt["files"]["body"]
    hidden = tmp_path / "unavailable-wide-body.ac"
    (source / "wide.py").unlink()
    body.rename(hidden)
    try:
        _ok(_compile(source, "bench.py", bench, [provider]))
    finally:
        hidden.rename(body)
    assert _tree(provider) == before
    final = tmp_path / "wide.ac"
    _ok(_link([provider, bench], "checks.bench.ExerciseWide", final))
    digest = hashlib.sha256(final.read_bytes()).hexdigest()
    widths = (129, 130, 129, 130, 130)
    values = [near129, near129, near129, near129, near130]
    expected = []
    for cycle in range(8):
        words = []
        for value, width in zip(values, widths, strict=True):
            words.extend(
                (value & ((1 << 64) - 1), (value >> 64) & ((1 << 64) - 1), value >> 128)
            )
            assert value < 1 << width
        expected.extend([tuple(words)] * 2)
        increments = (1, 1, cycle % 2, 1 - cycle % 2, 1)
        values = [
            (value + increment) % (1 << width)
            for value, increment, width in zip(values, increments, widths, strict=True)
        ]
    # At the third old-Q sample, W129 wrapped while W130 carried into bit129;
    # the independently enabled W129 instance has not yet wrapped.
    assert expected[4][0:3] == (0, 0, 0)
    assert expected[4][3:6] == (0, 0, 2)
    assert expected[4][6:9] == ((1 << 64) - 1, (1 << 64) - 1, 1)
    assert expected[4][12:15] == (0, 0, 0)
    transcripts = []
    for target in ("cpp", "verilog"):
        directory = tmp_path / target
        directory.mkdir()
        binary = _bundle(final, directory, target)
        for workers in (1, 2) if target == "cpp" else (1,):
            arguments = (
                ["--cycles", "8", "--workers", str(workers)]
                if target == "cpp"
                else ["+cycles=8"]
            )
            records = _records(_ok(_run([str(binary), *arguments], timeout=60)))
            rows = [row for row in records if row["kind"] == "log"]
            for row in rows:
                for value in row["values"]:
                    if value["kind"] == "bits4":
                        assert int(value["known"]) == (1 << int(value["width"])) - 1
                        assert int(value["z"]) == 0
            assert [
                tuple(int(value["value"]) for value in row["values"]) for row in rows
            ] == expected
            assert [int(row["evaluation_epoch"]) for row in rows] == list(range(16))
            transcripts.append(rows)
    assert transcripts[0] == transcripts[1] == transcripts[2]
    assert hashlib.sha256(final.read_bytes()).hexdigest() == digest
    assert _tree(provider) == before
