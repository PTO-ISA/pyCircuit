"""Independent root binding and nested family publication oracles."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

source_root, emitter, mode, scratch = sys.argv[1:]
source_root = Path(source_root)
scratch = Path(scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
work = tempfile.TemporaryDirectory(prefix=f"collection-{mode}-", dir=scratch)
root = Path(work.name)
env = dict(os.environ, PYTHONPATH=str(source_root / "python/pycircuit/src"),
           PYCIRCUIT_EMITTER=emitter, PYTHONDONTWRITEBYTECODE="1")


def run(command, *, accepted=True, timeout=90, **kwargs):
    result = subprocess.run(command, capture_output=True, text=True,
                            timeout=timeout, **kwargs)
    print(json.dumps({'command': command, 'exit_status': result.returncode}), flush=True)
    if accepted:
        assert result.returncode == 0, (command, result.stdout, result.stderr)
    else:
        assert result.returncode != 0, (command, result.stdout, result.stderr)
    return result


def expr(name, value):
    return f'#{name} = #ac.static_expr<{{kind = "literal", location = {{path = "root.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}}, origin = {{site = {{definition = @Root, ast_path = []}}, expansion = []}}, value = {{kind = "integer", value = #ac.math_int<{value}>}}}}>\n'


literal = ''.join(expr(f'w{value}', value) for value in [0, 1, 2, 3, 5, 8, 9, 32, 63, 65536, 9223372036854775807])
types = '!b1 = !ac.bits<#w1>\n!b8 = !ac.bits<#w8>\n'
leaf = '''  "ac.module.import"() {sym_name = "storage", source_owner = {package = "gfsim", path = "dff.py"}, parameters = [], type_parameters = ["T"], function_type = (!b1, !b1, !ac.type_param<@storage, "T">, !ac.type_param<@storage, "T">) -> !ac.type_param<@storage, "T">, input_names = ["clk", "rst", "d", "init"], output_names = ["q"], primitive_kind = "dff", dependency_summary = [{output = {port = 0 : i64, path = []}, inputs = []}]} : () -> ()
'''
occurrence = '{site = {definition = @Root, ast_path = []}, expansion = []}'

if mode == 'resource':
    source = literal + types + '''!data = !ac.table<[#w65536], !b8>
!control = !ac.table<[#w65536], !b1>
module {
''' + leaf + f'''  "ac.module"() ({{
    %clk = "ac.bits.constant"() {{value = #w0}} : () -> !b1
    %data = "ac.bits.constant"() {{value = #w0}} : () -> !b8
    %clocks = "ac.table.splat"(%clk) {{shape = [#w65536]}} : (!b1) -> !control
    %values = "ac.table.splat"(%data) {{shape = [#w65536]}} : (!b8) -> !data
    %q = "ac.collection"(%clocks, %clocks, %values, %values) {{instance_name = "leaves", callee = @storage, parameters = [], type_arguments = [!b8], shape = [#w65536], occurrence = {occurrence}}} : (!control, !control, !data, !data) -> !data
    "ac.yield"() : () -> ()
  }}) {{sym_name = "inner", source_owner = {{package = "", path = "inner.py"}}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = []}} : () -> ()
  "ac.module"() ({{
    "ac.collection"() {{instance_name = "outer", callee = @inner, parameters = [], type_arguments = [], shape = [#w65536], occurrence = {occurrence}}} : () -> ()
    "ac.yield"() : () -> ()
  }}) {{sym_name = "Root", source_owner = {{package = "", path = "root.py"}}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = []}} : () -> ()
  "ac.system"() {{entry = {{callee = @Root, parameters = [], type_arguments = []}}, domain = "default"}} : () -> ()
}}
'''
    invalid = root / 'resource.ac'
    invalid.write_text(source)
    memory_reference = '#aw = #ac.static_expr<{kind = "reference", location = {path = "memory.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @memory, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @memory, name = "ADDR_WIDTH"}}>\n'
    memory_source = literal + types + memory_reference + f'''module {{
  "ac.module.import"() {{sym_name = "memory", source_owner = {{package = "gfsim", path = "sync_mem.py"}}, parameters = [{{name = "ADDR_WIDTH", type = !ac.math_int}}, {{name = "DEPTH", type = !ac.math_int}}], type_parameters = ["T"], function_type = (!b1, !b1, !b1, !ac.bits<#aw>, !b1, !ac.bits<#aw>, !ac.type_param<@memory, "T">, !b1) -> !ac.type_param<@memory, "T">, input_names = ["clk", "rst", "ren", "raddr", "wvalid", "waddr", "wdata", "wstrb"], output_names = ["rdata"], primitive_kind = "sync_mem", dependency_summary = [{{output = {{port = 0 : i64, path = []}}, inputs = []}}]}} : () -> ()
  "ac.module"() ({{
    %zero = "ac.bits.constant"() {{value = #w0}} : () -> !b1
    %one = "ac.bits.constant"() {{value = #w1}} : () -> !b1
    %address = "ac.bits.constant"() {{value = #w0}} : () -> !ac.bits<#w63>
    %data = "ac.bits.constant"() {{value = #w0}} : () -> !b8
    %q = "ac.instance"(%zero, %zero, %zero, %address, %zero, %address, %data, %one) {{instance_name = "memory", callee = @memory, parameters = [#w63, #w9223372036854775807], type_arguments = [!b8], occurrence = {occurrence}}} : (!b1, !b1, !b1, !ac.bits<#w63>, !b1, !ac.bits<#w63>, !b8, !b1) -> !b8
    "ac.yield"() : () -> ()
  }}) {{sym_name = "Root", source_owner = {{package = "", path = "root.py"}}, parameters = [], type_parameters = [], function_type = () -> (), input_names = [], output_names = []}} : () -> ()
  "ac.system"() {{entry = {{callee = @Root, parameters = [], type_arguments = []}}, domain = "default"}} : () -> ()
}}
'''
    huge_memory = root / 'memory-resource.ac'
    huge_memory.write_text(memory_source)
    valid = source_root / 'tests/compiler/lit/CodeGen/Inputs/two-leaves.mlir'
    # The width is 65536*65536*8 = 2^35, derived from nested state planes.
    # Each local module table is legal and only the enclosing multiplier
    # exceeds the emitted Runtime representation; the root has no ports.
    for target in ['cpp', 'verilog']:
        for source, diagnostic in [(invalid, 'including enclosing collections'),
                                   (huge_memory, 'memory')]:
            rejected = run([emitter, str(source), '--target', target], accepted=False)
            assert rejected.stdout == '', 'native reject published partial JSON'
            assert diagnostic in rejected.stderr.lower()
        published = root / target
        command = [sys.executable, '-m', 'pycircuit.cli', 'emit', str(valid),
                   '--target', target, '-o', str(published)]
        run(command, env=env)
        before = {p.relative_to(published): p.read_bytes()
                  for p in published.rglob('*') if p.is_file()}
        assert before
        for source in [invalid, huge_memory]:
            command[4] = str(source)
            run(command + ['--replace'], accepted=False, env=env)
            after = {p.relative_to(published): p.read_bytes()
                     for p in published.rglob('*') if p.is_file()}
            assert after == before, 'resource rejection modified prior output'
            assert not list(published.rglob('*.tmp'))
    print('portless nested family 2^35-bit plane and scalar memory DEPTH=INT64_MAX rejected by both targets; prior outputs preserved')
elif mode == 'binding':
    parameter = '''#n = #ac.static_expr<{kind = "reference", location = {path = "root.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Root, ast_path = []}, expansion = []}, ref = {kind = "parameter", owner = @Root, name = "N"}}>
!formal = !ac.type_param<@Root, "T">
!data = !ac.table<[#n], !formal>
!control = !ac.table<[#n], !b1>
#width = #ac.static_expr<{kind = "type_width", location = {path = "root.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Root, ast_path = []}, expansion = []}, type = !formal}>
#proof = #ac.static_expr<{kind = "binary", location = {path = "root.py", line = 1 : i64, column = 1 : i64, end_line = 1 : i64, end_column = 2 : i64}, origin = {site = {definition = @Root, ast_path = []}, expansion = []}, operator = "add", lhs = #n, rhs = #width}>
'''
    source = literal + types + parameter + 'module {\n' + leaf + f'''  ac.struct "Entry" fields [{{name = "tag", type = !ac.bits<#w5>}}, {{name = "data", type = !ac.bits<#w9>}}]
  "ac.module"() ({{
    %clk = "ac.bits.constant"() {{value = #w0}} : () -> !b1
    %clocks = "ac.table.splat"(%clk) {{shape = [#n]}} : (!b1) -> !control
    %q = "ac.collection"(%clocks, %clocks, %q, %q) {{instance_name = "leaves", callee = @storage, parameters = [], type_arguments = [!formal], shape = [#n], occurrence = {occurrence}}} : (!control, !control, !data, !data) -> !data
    %proof = "ac.bits.constant"() {{value = #proof}} : () -> !ac.bits<#w32>
    "ac.yield"(%proof) : (!ac.bits<#w32>) -> ()
  }}) {{sym_name = "Root", source_owner = {{package = "", path = "root.py"}}, parameters = [{{name = "N", type = !ac.math_int, default = #w2}}], type_parameters = ["T"], function_type = () -> !ac.bits<#w32>, input_names = [], output_names = ["proof"]}} : () -> ()
  "ac.system"() {{entry = {{callee = @Root, parameters = [#w3], type_arguments = [!ac.struct<"Entry">]}}, domain = "default"}} : () -> ()
}}
'''
    final = root / 'binding.ac'
    final.write_text(source)
    override = os.environ.get('PYCIRCUIT_COLLECTION_RUNTIME_PREFIX')
    if override:
        prefix = Path(override).resolve()
    else:
        prefix = root / 'runtime-install'
        runtime_build = root / 'runtime-build'
        run(['cmake', '-S', str(source_root), '-B', str(runtime_build), '-G', 'Ninja',
             '-DPYC_BUILD_COMPILER_DEV=OFF', '-DPYC_BUILD_RUNTIME_LIB=ON',
             '-DPYC_BUILD_TESTING=OFF', '-DPYC_INSTALL_PYTHON=OFF',
             '-DCMAKE_DISABLE_FIND_PACKAGE_LLVM=ON', '-DCMAKE_DISABLE_FIND_PACKAGE_MLIR=ON',
             f'-DCMAKE_INSTALL_PREFIX={prefix}'])
        run(['cmake', '--build', str(runtime_build), '-j', '2'], timeout=180)
        run(['cmake', '--install', str(runtime_build)])
    for header in ['collection.h', 'memory_detail.h']:
        assert (prefix / 'include/gfsim' / header).is_file(), f'Runtime install omitted {header}'
    for target in ['cpp', 'verilog']:
        native = run([emitter, str(final), '--target', target])
        payload = json.loads(native.stdout)
        assert payload['entry']['definition'] == '@"Root"'
        assert len(payload['entry']['arguments']) == 2, 'root actuals omitted from native JSON'
        if target == 'verilog':
            assert payload['root_rtl_name'] == 'pyc_root', 'CMake selected the unbound generic module'
            assert 'module pyc_root' in payload['rtl_core']
        published = root / target
        run([sys.executable, '-m', 'pycircuit.cli', 'emit', str(final),
             '--target', target, '-o', str(published)], env=env)
        build = root / f'{target}-build'
        run(['cmake', '-S', str(published), '-B', str(build), '-G', 'Ninja',
             f'-DCMAKE_PREFIX_PATH={prefix}',
             '-DCMAKE_DISABLE_FIND_PACKAGE_LLVM=ON', '-DCMAKE_DISABLE_FIND_PACKAGE_MLIR=ON'])
        cache = (build / 'CMakeCache.txt').read_text()
        assert 'LLVM_DIR:' not in cache and 'MLIR_DIR:' not in cache, 'Runtime consumer discovered compiler dependencies'
        run(['cmake', '--build', str(build), '-j', '2'], timeout=180)
        consumer = root / f'{target}-consumer'
        consumer.mkdir()
        driver = consumer / 'proof.cpp'
        if target == 'cpp':
            driver.write_text('\n'.join([
                '#include "pycircuit_system.hpp"', '#include <type_traits>', '#include <cstdlib>',
                'static_assert(std::is_same_v<pyc_root, Root<3, Entry>>);',
                'int main() { pyc_root dut("bound"); dut.Build(); dut.Reset(); dut.Xfer(); dut.Work();',
                'if (!dut.proof.isFullyKnown() || dut.proof.value() != gfsim::Bits<32>{17}) std::abort();',
                'dut.DiscardNext(); }', '']))
        else:
            headers = [p for p in build.rglob('V*.h') if '__' not in p.name]
            assert len(headers) == 1, 'public CMake did not expose exactly one top model header'
            # Verilator CMake's model PREFIX may follow its first source file;
            # TOP_MODULE still comes from root_rtl_name. Observe the root wire
            # rather than imposing a generated C++ model-prefix recipe.
            model = headers[0].stem
            driver.write_text(f'#include "{headers[0].name}"\n#include <cstdlib>\ndouble sc_time_stamp() {{ return 0.0; }}\nint main() {{ {model} dut; dut.eval(); if (dut.proof != 17) std::abort(); }}\n')
        # The value 17 is independently 3 + (Entry.tag:5 + Entry.data:9).
        # Default N=2 or an unbound T cannot satisfy this observable oracle.
        (consumer / 'CMakeLists.txt').write_text('\n'.join([
            'cmake_minimum_required(VERSION 3.25)', 'project(BoundRootProof LANGUAGES C CXX)',
            f'add_subdirectory("{published.as_posix()}" generated)',
            'add_executable(bound_root_proof proof.cpp)',
            'target_link_libraries(bound_root_proof PRIVATE pycircuit_modules)', '']))
        consumer_build = root / f'{target}-consumer-build'
        run(['cmake', '-S', str(consumer), '-B', str(consumer_build), '-G', 'Ninja',
             f'-DCMAKE_PREFIX_PATH={prefix}', '-DCMAKE_DISABLE_FIND_PACKAGE_LLVM=ON',
             '-DCMAKE_DISABLE_FIND_PACKAGE_MLIR=ON'])
        run(['cmake', '--build', str(consumer_build), '-j', '2'], timeout=180)
        run([str(consumer_build / 'bound_root_proof')])
    print('explicit Root<N=3,T=Entry> actuals retained; bound cpp/RTL roots and both public CMake artifacts built')
else:
    raise AssertionError(f'unknown test mode {mode}')
