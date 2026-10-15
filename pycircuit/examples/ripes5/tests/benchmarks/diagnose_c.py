"""Build an isolated, instrumented C++ lowerer and record expansion causes.

Production compiler/model sources and the normal compiler binary stay unchanged.
Requires the existing Ninja compiler build and inspect_c's two saved inputs.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

from ..paths import ROOT


def instrument(source):
    def replace(old, new):
        nonlocal source
        assert source.count(old) == 1, old
        source = source.replace(old, new)

    replace('  bool conditionalCycle = false;\n', '''  bool conditionalCycle = false;
  DenseMap<Value, unsigned> diagnosticIds, emitted;
  unsigned diagnosticSerial = 0;
  root.getDefiningOp()->getParentOfType<mlir::ModuleOp>().walk([&](Operation *op) {
    for (auto result : op->getResults()) diagnosticIds[result] = diagnosticSerial++;
  });
  auto describe = [&](Value v) {
    llvm::errs() << "v" << diagnosticIds.lookup(v) << " ";
    auto *op = v.getDefiningOp();
    llvm::errs() << op->getName() << " name=" << cppSourceName(v);
    if (external(v)) llvm::errs() << " shared_block=" << plan.boundary.at(v);
    for (auto key : {"resource", "method", "field", "predicate"})
      if (auto attr = op->getAttr(key)) llvm::errs() << " " << key << "=" << attr;
  };
  std::vector<Value> diagnosticStack;
  std::vector<const char *> diagnosticEdges;
  bool printedCycle = false;
''')
    replace('      conditionalCycle = true;\n', '''      conditionalCycle = true;
      if (!printedCycle) {
        printedCycle = true;
        llvm::errs() << "CYCLE root "; describe(root); llvm::errs() << "\\n";
        auto start = std::find(diagnosticStack.begin(), diagnosticStack.end(), v);
        for (auto it = start; it != diagnosticStack.end(); ++it) {
          describe(*it);
          llvm::errs() << " --" << diagnosticEdges[it - diagnosticStack.begin()] << "-->\\n";
          llvm::errs() << "    operands:";
          for (auto operand : it->getDefiningOp()->getOperands())
            llvm::errs() << " v" << diagnosticIds.lookup(operand);
          llvm::errs() << "\\n    demand_atoms:";
          DenseSet<Value> atoms;
          guards.dependencies(demands.at(*it), atoms);
          for (auto atom : guards.values)
            if (atoms.count(atom)) llvm::errs() << " v" << diagnosticIds.lookup(atom);
          llvm::errs() << "\\n";
        }
        describe(v); llvm::errs() << "\\nEND_CYCLE\\n";
      }
''')
    replace('    marks[v] = 1;\n', '    marks[v] = 1;\n    diagnosticStack.push_back(v);\n')
    replace('      if (controls.count(atom))\n        visit(atom);', '''      if (controls.count(atom)) {
        diagnosticEdges.push_back("control"); visit(atom); diagnosticEdges.pop_back();
      }''')
    replace('      for (auto x : op->getOperands().drop_front(first))\n        visit(x);', '''      for (auto x : op->getOperands().drop_front(first)) {
        diagnosticEdges.push_back("data"); visit(x); diagnosticEdges.pop_back();
      }''')
    replace('    marks[v] = 2;\n', '    diagnosticStack.pop_back();\n    marks[v] = 2;\n')
    replace('      auto *op = v.getDefiningOp();\n      auto n = op->getName().getStringRef();\n      if (!external(v) && n == "arith.constant")', '''      ++emitted[v];
      auto *op = v.getDefiningOp();
      auto n = op->getName().getStringRef();
      if (!external(v) && n == "arith.constant")''')
    replace('    return emit(root);', '''    auto result = emit(root);
    unsigned total = 0;
    std::vector<Value> ranking;
    for (auto item : emitted) { total += item.second; ranking.push_back(item.first); }
    std::sort(ranking.begin(), ranking.end(), [&](Value a, Value b) {
      return emitted[a] != emitted[b] ? emitted[a] > emitted[b] : diagnosticIds[a] < diagnosticIds[b];
    });
    llvm::errs() << "FALLBACK root "; describe(root);
    llvm::errs() << " unique=" << emitted.size() << " emissions=" << total << "\\n";
    for (auto v : llvm::ArrayRef<Value>(ranking).take_front(20)) {
      llvm::errs() << "  count=" << emitted[v] << " "; describe(v); llvm::errs() << "\\n";
    }
    llvm::errs() << "END_FALLBACK\\n";
    return result;''')
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'pycircuit/builds/benchmarks/ripes5-standard/c-cause-20261015')
    args = parser.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    build = ROOT/'pycircuit/builds/compiler'
    source = ROOT/'pycircuit/mlir/CppLowering.cpp'
    modified = out/'CppLowering.diagnostic.cpp'
    modified.write_text(instrument(source.read_text()))
    commands = subprocess.check_output(['ninja', '-t', 'commands', 'mlir/acir-compile'], cwd=build, text=True).splitlines()
    compile_cmd = shlex.split(next(c for c in commands if ' -c ' in c and str(source) in c))
    obj = out/'CppLowering.diagnostic.o'
    compile_cmd[compile_cmd.index('-o')+1] = str(obj)
    compile_cmd[compile_cmd.index('-c')+1] = str(modified)
    compile_cmd[compile_cmd.index('-MF')+1] = str(out/'diagnostic.d')
    compile_cmd += ['-I'+str(source.parent)]
    subprocess.run(compile_cmd, cwd=build, check=True)
    link_cmd = shlex.split(commands[-1])
    if link_cmd[:2] == [':', '&&']: link_cmd = link_cmd[2:]
    if link_cmd[-2:] == ['&&', ':']: link_cmd = link_cmd[:-2]
    binary = out/'acir-compile-diagnostic'
    link_cmd[link_cmd.index('-o')+1] = str(binary)
    link_cmd.insert(link_cmd.index('mlir/libACIRCompiler.a'), str(obj))
    subprocess.run(link_cmd, cwd=build, check=True)
    manifest = dict(production_source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(), compile_command=compile_cmd, link_command=link_cmd, cases=[])
    env = dict(os.environ, LD_LIBRARY_PATH='/home/lc/opt/gcc14/lib')
    inputs = ROOT/'pycircuit/builds/benchmarks/ripes5-standard/c-expansion-20261015'
    for name in ('c_off', 'c_on_same_bytes'):
        target = out/name; target.mkdir(exist_ok=True)
        input_file = inputs/name/'model.transaction.mlir'
        command = [str(binary), str(input_file), str(target), '--stop-after', 'transaction', '--emit-cpp']
        with (target/'diagnostic.log').open('w') as log:
            subprocess.run(command, env=env, check=True, stdout=log, stderr=log)
        old = inputs/name/'cpp/model.hpp'; new = target/'cpp/model.hpp'
        identical = old.read_bytes() == new.read_bytes()
        manifest['cases'].append(dict(name=name, command=command, generated_cpp_identical=identical, input_sha256=hashlib.sha256(input_file.read_bytes()).hexdigest()))
        print(name, 'generated C++ identical:', identical, flush=True)
        assert identical
    (out/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')


if __name__ == '__main__':
    main()
