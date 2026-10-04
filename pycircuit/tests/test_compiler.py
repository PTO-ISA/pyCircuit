"""End-to-end contracts, including ACIR reload with the original source removed."""
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

from pycircuit import compile_source, emit
from pycircuit.ir import CompileError, load, save

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
CXX = shlex.split(os.environ.get('ACPY_CXX', 'c++'))


def run(command, **kwargs):
    result = subprocess.run([str(x) for x in command], capture_output=True, text=True, timeout=120, **kwargs)
    if result.returncode:
        raise AssertionError(f'command failed: {command}\n{result.stdout}\n{result.stderr}')
    return result.stdout


class CompilerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix='acpy-test-')
        cls.path = Path(cls.temporary.name)
        cls.include = ROOT / 'gfsim/cpp/include'
        cls.runtime = cls.path / 'runtime.o'
        run([*CXX, '-std=c++20', '-O2', '-I', cls.include, '-c', ROOT / 'gfsim/cpp/src/simulator.cpp', '-o', cls.runtime])

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def circuit(self, top, body, source=None, monitor_missing=False, model=None, optimize=True):
        directory = self.path / top
        directory.mkdir(exist_ok=True)
        model = model or compile_source(source or HERE / 'circuits.py', top)
        save(model, directory / 'model.acir.mlir')
        emit(load(directory / 'model.acir.mlir'), directory, optimize=optimize)
        # Count actual throws from generated code and the unchanged runtime. This
        # is an ELF linker probe, not a modified Queue implementation or mock.
        probe = '''
#include <typeinfo>
static unsigned missing_throws = 0;
extern "C" [[noreturn]] void __real___cxa_throw(void*, void*, void(*)(void*));
extern "C" [[noreturn]] void __wrap___cxa_throw(void* value, void* info, void(*destroy)(void*)) {
    if (*static_cast<std::type_info*>(info) == typeid(gfsim::NeedInput)) ++missing_throws;
    __real___cxa_throw(value, info, destroy);
}
''' if monitor_missing else ''
        check = '\nCHECK(missing_throws==0);' if monitor_missing else ''
        harness = '#include "model.hpp"\n#include <iostream>\nusing namespace ac_generated;\n' + probe + '#define CHECK(x) do {if(!(x)) { std::cerr << __LINE__ << ": " << #x << "\\n"; return 1; }} while(0)\nint main(){\n' + body + check + '\n}\n'
        (directory / 'test.cpp').write_text(harness)
        link = ['-Wl,--wrap=__cxa_throw'] if monitor_missing else []
        run([*CXX, '-std=c++20', '-O2', '-I', self.include, directory / 'model.cpp', directory / 'test.cpp', self.runtime, *link, '-o', directory / 'run'])
        run([directory / 'run'])
        return model

    def test_pipeline(self):
        self.circuit('Pipeline', '''
for(bool reverse:{false,true}) {
    for(const auto& values : {std::vector<std::uint32_t>{1,2,3,4,5}, std::vector<std::uint32_t>{0,0xffffffff,42}, std::vector<std::uint32_t>{}}) {
        Pipeline m(values, reverse);
        CHECK(m.sim.moduleCount()==3);
        std::vector<std::uint32_t> observed;
        for(int i=0;i<40;++i) {
            auto count=m.count.peek(), sum=m.total.peek(); m.sim.step();
            if(m.count.peek()!=count) {
                CHECK(m.count.peek()==count+1);
                observed.push_back(m.total.peek()-sum);
            }
        }
        CHECK(observed.size()==values.size());
        for(std::size_t i=0;i<values.size();++i) CHECK(observed[i]==std::uint32_t(values[i]+values[i]));
        std::uint32_t expected=0; for(auto v:values) expected+=v+v;
        CHECK(m.total.peek()==expected); CHECK(m.count.peek()==values.size());
        CHECK(m.source_produce_out0.empty()); CHECK(m.transformed_double_out0.empty());
    }
}''', ROOT / 'pycircuit/examples/pipeline/model.py')

    def test_atomic_backpressure_and_alias(self):
        model = self.circuit('Atomic', '''
for(bool reverse:{false,true}) {
    Atomic m(reverse);
    m.sim.step();
    CHECK(m.controls.size()==2 && m.left.size()==1);
    auto rid=m.top.rid_route;
    m.sim.step();
    CHECK(m.controls.size()==2 && m.left.size()==1);
    CHECK(m.sim.rule(rid).complete);
    const auto calls=m.sim.rule(rid).calls;
    m.sim.step(); m.sim.step();
    CHECK(m.controls.size()==2 && m.left.size()==1);
    CHECK(m.sim.rule(rid).calls==calls+2); // Clock changes activate the whole Module.
    m.sim.step();
    CHECK(m.controls.size()==1 && m.left.empty());
    CHECK(m.total.peek()==10 && m.count.peek()==1);
    m.sim.step(); CHECK(m.total.peek()==22 && m.count.peek()==2);
    for(int i=0;i<12;++i) m.sim.step();
    CHECK(m.controls.size()==1 && m.controls.peek()==1);
    CHECK(m.right.empty()); CHECK(m.total.peek()==22 && m.count.peek()==2);
    CHECK(!m.sim.rule(rid).complete);
    CHECK(m.top_route_out0.empty() && m.top_route_out1.empty());
}''')
        self.assertEqual(len(model['modules'][-1]['rules']), 2)

    def test_parameter_identity_and_value(self):
        self.circuit('Parameters', '''
for(bool reverse:{false,true}) {
    Parameters m(reverse);
    std::vector<std::uint32_t> observed;
    for(int i=0;i<20;++i) {
        auto count=m.count.peek(), sum=m.total.peek(); m.sim.step();
        if(m.count.peek()!=count) { CHECK(m.count.peek()==count+1); observed.push_back(m.total.peek()-sum); }
    }
    CHECK((observed==std::vector<std::uint32_t>{10,24,35}));
    CHECK(m.count.peek()==3); CHECK(m.total.peek()==69);
    for(auto* q:m.entries.refs) CHECK(q->empty());
    CHECK(m.sim.ruleCount()==3);
}''')

    def test_struct_dynamic_array_signal(self):
        self.circuit('Configured', '''
for(bool reverse:{false,true}) {
    std::vector<std::uint8_t> values{255,2,10};
    Configured m(values,2,reverse);
    for(int i=0;i<16;++i) m.sim.step();
    for(std::size_t i=0;i<values.size();++i) {
        CHECK(m.entries.refs[i]->size()==1);
        CHECK(m.entries.refs[i]->peek().meta.epoch==2);
        CHECK(m.entries.refs[i]->peek().value==std::uint8_t(values[i]+4));
    }
    CHECK(m.shared.evaluations()==7); // All declared array entries invalidate.
    CHECK(m.observed.peek()==1 && m.shared_again.peek()==1);
    CHECK(m.sim.rule(m.observer.rid_observe).calls==1);
}''')

    def test_static_signal_bindings_across_calls_branches_and_reload(self):
        # Serialize, remove the source, then emit through the standalone path.
        source = self.path / 'signals.py'
        source.write_text((HERE / 'circuits.py').read_text())
        model = compile_source(source, 'StaticSignals')
        source.unlink()
        self.circuit('StaticSignals', '''
for(bool reverse:{false,true}) {
    StaticSignals m(reverse);
    for(int i=0;i<5;++i) m.sim.step();
    CHECK(m.left.evaluations()==1 && m.right.evaluations()==5);
    CHECK(m.stable.evaluations()==5 && m.stable.value()==9);
    CHECK(m.sim.module(m.observer.mid).calls==5);
    for(auto rid:{m.observer.rid_conditional, m.observer.rid_selected, m.observer.rid_captured}) {
        CHECK(m.sim.rule(rid).calls==5); // Never read Signal on any executed path.
    }
    CHECK(m.sim.rule(m.observer.rid_ordinary).calls==5);
}''', model=model)
        generated = (self.path / 'StaticSignals/model.cpp').read_text()
        for signal in ('left', 'right'):
            self.assertIn(f'sim.declareResource(observer.mid, {signal});', generated)
        self.assertNotIn('ParameterCache', (self.path / 'StaticSignals/model.hpp').read_text())
        self.assertNotIn('arbitrate_', generated)

    def test_signal_dag_and_source_free_reload(self):
        source = self.path / 'signal_dag.py'
        source.write_text((HERE / 'signal_circuits.py').read_text())
        model = compile_source(source, 'SignalDAG')
        source.unlink()
        for optimize in (False, True):
            with self.subTest(optimize=optimize):
                self.circuit('SignalDAG', '''
for(bool reverse:{false,true}) {
    SignalDAG m(reverse);
    m.sim.step();
    CHECK(m.a_join.value()==34 && m.seen.peek()==20);
    CHECK(m.c_capture.value()==10 && m.o_tail.value()==101);
    m.sim.step();
    CHECK(m.a_join.value()==42 && m.seen.peek()==34);
    CHECK(m.c_capture.value()==14);
    m.sim.step(); m.sim.step();
    CHECK(m.seen.peek()==42);
    CHECK(m.z_root_output.evaluations()==3);
    for(auto* s:{&m.a_join, &m.b_left, &m.b_right, &m.p_parity, &m.c_capture, &m.unused})
        CHECK(s->evaluations()==3);
    CHECK(m.o_tail.evaluations()==1);
    CHECK(m.sim.module(m.filtered.mid).calls==1);
    CHECK(m.sim.events().empty());
}''', model=model, optimize=optimize)
                generated = (self.path / 'SignalDAG/model.cpp').read_text()
                self.assertIn('sim.declareInput(c_capture, z_root_output);', generated)
                self.assertIn('sim.declareInput(unused, z_root_output);', generated)

    def test_signal_connection_diagnostics(self):
        for top, diagnostic in [('SignalCycle', 'Signal dependency cycle'),
                                ('InvalidSignalArgument', 'Queues or Signals')]:
            with self.subTest(top=top), self.assertRaisesRegex(CompileError, diagnostic):
                compile_source(HERE / 'signal_circuits.py', top)

    def test_original_signal_composition_probes(self):
        source = HERE / 'fixtures/skyzh/modular.py'
        for top, output, expected in [('SignalChain', 'selected', 8),
                                      ('CapturedSignal', 'result', 7)]:
            with self.subTest(top=top):
                self.circuit(top, f'''
for(bool reverse:{{false,true}}) {{
    {top} m(reverse);
    m.sim.step(); m.sim.step();
    CHECK(m.{output}.value()=={expected});
    CHECK(m.{output}.evaluations()==1 && m.decoded.evaluations()==1);
}}''', source)

    def test_loop_carried_aggregate_fields(self):
        for optimize in (False, True):
            with self.subTest(optimize=optimize):
                self.circuit('AggregateLoop', '''
for(unsigned n:{0U,1U,5U}) {
    AggregateLoop m(n); m.sim.step();
    const auto &p=m.output.value();
    CHECK(p.tag==n);
    for(unsigned j=0;j<4;++j) CHECK(p.lanes[j]==n*(n-1)/2+(j==2 ? 0 : n*j));
}''', HERE / 'mlir_circuits.py', optimize=optimize)

    def test_module_multi_output_and_capacity(self):
        model = self.circuit('Composed', """
Composed m; for(int i=0;i<6;++i) m.sim.step();
CHECK(m.sink_total.peek()==35);
CHECK(m.array.refs.size()==4);
for(std::size_t i=0;i<4;++i) CHECK(m.array.refs[i]->peek()==i);
""")
        outputs = [r for r in model['resources'] if '_pair_out' in r['name']]
        self.assertEqual(len(outputs), 1)
        declaration = next(op for op in model['construction'] if op['id'] == outputs[0]['capacity'])
        self.assertEqual(declaration['value'], 2)

    def test_event_only(self):
        self.circuit('Events', '''
for(bool reverse:{false,true}) {
    Events m(reverse);
    for(int i=0;i<11;++i) m.sim.step();
    CHECK(m.sim.module(m.top.mid).calls==4);
    CHECK(m.sim.stats().events==4 && m.sim.stats().dueEvents==3);
    CHECK(m.sim.events().size()==1 && m.sim.events()[0].first==12);
}''')

    def test_short_circuit_early_return(self):
        self.circuit('ShortCircuit', '''
for(bool reverse:{false,true}) {
ShortCircuit m(reverse); for(int i=0;i<5;++i) m.sim.step();
CHECK(m.counter.peek()==7 && m.empty.empty());
CHECK(m.sim.rule(m.top.rid_guarded).calls==1);
}
''', monitor_missing=True)

    def test_missing_input_aborts_without_throwing(self):
        self.circuit('MissingInput', '''
for(bool reverse:{false,true}) {
    MissingInput m(reverse);
    for(int i=0;i<3;++i) {
        m.sim.step();
        CHECK(m.first.size()==2 && m.state.peek()==0);
        CHECK(m.marker.peek()==1); // The Rule after take still runs.
        CHECK(!m.sim.rule(m.worker.rid_take).complete);
        for(auto event:m.sim.events()) CHECK(event.first<20);
    }
    m.sim.step();
    CHECK(m.state.peek()==1 && m.first.size()==1 && m.first.peek()==6);
    CHECK(m.worker_take_out0.peek()==15 && m.second_produce_out0.empty());
    for(int i=0;i<5;++i) m.sim.step();
    CHECK(m.state.peek()==1 && m.first.size()==1);
    CHECK(m.sim.events().size()==1 && m.sim.events()[0].first==23);
}
''', monitor_missing=True)

    def test_missing_input_checks_follow_the_active_branch(self):
        self.circuit('ConditionalInput', '''
for(bool reverse:{false,true}) {
    ConditionalInput m(reverse);
    for(int busy=1;busy>=0;--busy) {
        m.sim.step();
        CHECK(m.busy.peek()==unsigned(busy) && m.result.peek()==0);
    }
    m.sim.step();
    CHECK(m.result.peek()==0);
    m.sim.step();
    CHECK(m.result.peek()==10 && m.input_produce_out0.empty());
}
''', monitor_missing=True)

    def test_empty_revise_aborts_without_a_payload_read(self):
        self.circuit('EmptyRevise', '''
for(bool reverse:{false,true}) {
    EmptyRevise m(reverse);
    for(int i=0;i<3;++i) {
        m.sim.step();
        CHECK(m.state.peek()==0 && !m.sim.rule(m.worker.rid_assign).complete);
        for(auto event:m.sim.events()) CHECK(event.first<20);
    }
    m.sim.step();
    CHECK(m.state.peek()==1 && m.input_produce_out0.peek()==42);
    for(int i=0;i<5;++i) m.sim.step();
    CHECK(m.sim.events().size()==1 && m.sim.events()[0].first==23);
}
''', monitor_missing=True)

    def test_missing_control_keeps_earlier_rule_candidates(self):
        self.circuit('MissingControl', '''
for(bool reverse:{false,true}) {
    MissingControl m(reverse);
    for(int i=1;i<=3;++i) {
        m.sim.step();
        CHECK(m.before.peek()==unsigned(i) && m.after.peek()==0);
        CHECK(m.sim.rule(m.worker.rid_late).calls==0);
    }
    m.sim.step();
    CHECK(m.before.peek()==4 && m.after.peek()==10);
    CHECK(m.input_produce_out0.size()==1); // Control reads do not consume.
}
''', monitor_missing=True)

    def test_acir_pop_without_a_payload_read(self):
        model = compile_source(HERE / 'circuits.py', 'MissingInput')
        worker = next(m for m in model['modules'] if m['name'] == 'worker')
        take = next(r for r in worker['rules'] if r['name'] == 'take')['function']
        b = next(p['id'] for p in take['params'] if p['name'] == 'b')
        ops = [o for block in take['blocks'] for o in block['ops']]
        # ACIR may consume an element without using its payload. The backend must
        # still check the pop itself, independently of frontend read/pop pairing.
        read = next(o for o in ops if o['op'] == 'queue.read' and o['args'] == [b])
        read.update(op='zero', args=[])
        self.circuit('MissingInput', '''
for(bool reverse:{false,true}) {
    MissingInput m(reverse);
    for(int i=0;i<3;++i) {
        m.sim.step();
        CHECK(m.first.size()==2 && m.state.peek()==0);
        CHECK(!m.sim.rule(m.worker.rid_take).complete);
    }
    m.sim.step();
    CHECK(m.first.size()==1 && m.state.peek()==1);
    CHECK(m.worker_take_out0.peek()==5 && m.second_produce_out0.empty());
}
''', monitor_missing=True, model=model)

    def test_signal_empty_input_is_explicit_or_fatal(self):
        self.circuit('OptionalSignal', '''
for(bool reverse:{false,true}) {
    OptionalSignal m(reverse);
    m.sim.step(); CHECK(m.value.value()==0);
    m.sim.step(); CHECK(m.value.value()==0);
    m.sim.step(); CHECK(m.value.value()==10);
}
''', monitor_missing=True)
        self.circuit('InvalidSignal', '''
for(bool reverse:{false,true}) {
    InvalidSignal m(reverse);
    bool failed=false;
    try { m.sim.step(); } catch(const gfsim::NeedInput&) { failed=true; }
    CHECK(failed);
    failed=false;
    try { m.sim.step(); } catch(const std::logic_error&) { failed=true; }
    CHECK(failed);
}
''')

    def test_construction_loops_and_queue_lists(self):
        self.circuit('Construction', '''
Construction m; for(int i=0;i<4;++i) m.sim.step();
CHECK(m.result.peek()==3); CHECK(m.sim.moduleCount()==4);
CHECK(m._m0_counter.peek()==1 && m._m1_counter.peek()==2 && m._m2_counter.peek()==3);
''')

    def test_work_closure_parameter(self):
        self.circuit('Closure', '''
Closure m; for(int i=0;i<10;++i) m.sim.step(); CHECK(m.result.peek()==14);
''')

    def test_fixed_width_and_arrays(self):
        self.circuit('Arithmetic', '''
Arithmetic m; m.sim.step(); auto r=m.result.peek();
CHECK(r.value==1 && r.meta.epoch==2);
CHECK(m.fixed.peek()==r.lanes);
CHECK(r.lanes[0]==65535 && r.lanes[1]==1 && r.lanes[2]==65533);
CHECK(ac_detail::add<std::int32_t>(INT32_MAX,1)==INT32_MIN);
CHECK(ac_detail::mul<std::uint16_t>(65535,65535)==1);
CHECK(ac_detail::shl<std::uint32_t>(1,32)==0);
CHECK(ac_detail::shr<std::int32_t>(-2,1)==-1);
CHECK(ac_detail::div<std::int32_t>(INT32_MIN,-1)==INT32_MIN);
''')

    def test_reload_without_python_source(self):
        directory = self.path / 'reload'
        directory.mkdir()
        source = directory / 'source.py'
        source.write_text('from pycircuit import ac\n@ac.module\ndef Saved():\n    q=ac.queue[ac.u32](initial=7)\n    @ac.rule\n    def r():\n        q.value=9\n    r()\n')
        model = compile_source(source, 'Saved')
        save(model, directory / 'only.acir.mlir')
        source.unlink()
        run([os.sys.executable, '-m', 'pycircuit', 'emit', directory / 'only.acir.mlir', '--output', directory / 'out'], cwd=ROOT)
        harness = directory / 'main.cpp'
        harness.write_text('#include "model.hpp"\nint main(){ac_generated::Saved s; s.sim.step(); return s.q.peek()!=9;}\n')
        run([*CXX, '-std=c++20', '-O2', '-I', self.include, '-I', directory / 'out', directory / 'out/model.cpp', harness, self.runtime, '-o', directory / 'run'])
        run([directory / 'run'])
        self.assertNotIn('ast', json.dumps(model).lower())

    def test_mlir_static_outputs_arrays_and_keywords(self):
        source = HERE / 'mlir_circuits.py'
        self.circuit('Forward', """
for(bool reverse:{false,true}) {
  Forward m(reverse); m.sim.step();
  CHECK(m.rob.capacity()==12 && m.station.capacity()==1);
  CHECK(m.rob.peek()==3 && m.station.peek()==3 && m.source.size()==2);
  for(int i=0;i<4;++i) m.sim.step(); CHECK(m.source.size()==2);
}
""", source)
        self.circuit('Implicit', "Implicit m; m.sim.step(); CHECK(m.top_allocate_out0.peek()==9 && m.source.empty());", source)
        self.circuit('Arrays', """
for(bool reverse:{false,true}) {
  Arrays m(2,reverse); m.sim.step();
  for(unsigned i=0;i<4;++i) CHECK(m.slots.refs[i]->peek().lanes[2]==100+i);
  for(auto* q:m.empty_slots.refs) CHECK(q->empty() && q->capacity()==1);
  CHECK(m.record.at(0).lanes[2]==3);
  CHECK(m.record.at(1).lanes[2]==99 && m.record.at(1).tag==5);
}
""", source)
        self.circuit('SignalVar', 'SignalVar m; m.sim.step(); CHECK(m.source.empty() && m.seen.peek()==42);', source)
        self.circuit('NestedOutputs', 'NestedOutputs m; m.sim.step(); CHECK(m.first.peek()==4 && m.second.empty() && m.third.peek()==5 && m.source.empty());', source)
        self.circuit('Keywords', "Keywords m(5); m.sim.step(); CHECK(m.ac_py_737769746368.peek()==12);", source)
        self.circuit('StaticPorts', 'StaticPorts m; for(int i=0;i<3;++i) m.sim.step(); CHECK(m.observed.value()==7 && m.observed.evaluations()==4);', source)
        self.circuit('TemporaryRefs', """
for(bool reverse:{false,true}) {
  TemporaryRefs m(reverse);
  for(int i=0;i<3;++i) m.sim.step();
  CHECK(!m.left.empty() && !m.right.empty() && m.output.peek()==1);
  for(int i=0;i<3;++i) m.sim.step();
  CHECK(m.left.empty() && m.right.empty() && m.output.empty() && m.total.peek()==31);
}
""", source)

    def test_unsupported_syntax_has_location(self):
        path = self.path / 'invalid.py'
        for statement in ('while True:\n            pass', 'x = {1: 2}', 'q = ac.queue[ac.u32]()'):
            path.write_text('from pycircuit import ac\n@ac.module\ndef Bad():\n    @ac.rule\n    def bad():\n        ' + statement + '\n    bad()\n')
            with self.assertRaisesRegex(CompileError, r'invalid.py:6:'):
                compile_source(path, 'Bad')

    def test_historical_expression_regressions(self):
        repro = ROOT / 'pycircuit/examples/ooo/repro'
        self.circuit('Probe', 'Probe m; m.sim.step(); CHECK(m.original.value()==2 && m.workaround.value()==2);', repro / 'guarded_constant.py')
        self.circuit('Probe', 'Probe m; m.sim.step(); CHECK(!m.rows.peek()[0].done && m.rows.peek()[1].done);', repro / 'dynamic_payload.py')

    def test_output_type_cycle_diagnostic(self):
        path = self.path / 'cycle.py'
        path.write_text('from pycircuit import ac\n@ac.module\ndef Cycle():\n    @ac.rule\n    def produce():\n        return out.value\n    out = produce()\n')
        with self.assertRaisesRegex(CompileError, r'cycle.py:6:.*return type annotation'):
            compile_source(path, 'Cycle')


if __name__ == '__main__':
    unittest.main()
