"""Measure C-extension representation growth with identical program bytes.

No CPU/compiler edits and no timing-based performance claims. Artifacts use the
current sources; the historical mixed loop supplies only reproducible bytes.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import time

from pycircuit import analyze, compile_source
from pycircuit.ir import save
from pycircuit.tests.support import read_trace, check_cpp
from ..program import ProgramImage, make_source
from ..paths import ROOT


def metrics(path):
    text = path.read_text()
    matches = re.findall(r'^\s*(?:%[^=\n]+\s*=\s*)?(?:"((?:acir|arith|scf|func|cf)\.[\w.]+)"|((?:acir|arith|scf|func|cf)\.[\w.]+))(?=[(\s])', text, flags=re.M)
    ops = Counter(a or b for a, b in matches)
    return dict(bytes=path.stat().st_size, lines=len(text.splitlines()), operations=dict(ops), operation_count=sum(ops.values()))


def cpp_functions(path):
    lines=path.read_text().splitlines()
    methods=[]
    context=[]
    for index,line in enumerate(lines):
        indent=len(line)-len(line.lstrip())
        if line.lstrip().startswith('}'):
            context=[item for item in context if item[0]<indent]
        match=re.search(r'struct (\w+)\s*\{',line)
        if match:
            indent=len(line)-len(line.lstrip())
            context=[item for item in context if item[0]<indent]+[(indent,match[1])]
        if 'template<class Context> void try_' not in line:
            continue
        name='.'.join(item[1] for item in context)+'.'+re.search(r'void (\w+)',line)[1]
        depth=0; end=index
        for j in range(index,len(lines)):
            # Generated methods have no braces in string literals except the
            # assertion message; only the method braces are counted here.
            depth+=lines[j].count('{')-lines[j].count('}')
            if depth==0:
                end=j+1;break
        body='\n'.join(lines[index:end])+'\n'
        methods.append(dict(name=name,start_line=index+1,lines=end-index,bytes=len(body.encode()),
                            ifs=len(re.findall(r'\bif\s*\(',body))))
    return methods


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'pycircuit/builds/benchmarks/ripes5-standard/c-expansion-20261015')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    original=ProgramImage.load(ROOT/'pycircuit/builds/benchmarks/rv32imc-performance/im/input.json')
    records=[];traces=[]
    for name,isa in [('c_off','IM'),('c_on_same_bytes','IMC')]:
        image=ProgramImage(original.text,original.ram,original.registers[:],isa,original.entry,original.end_pc,original.max_cycles)
        directory=out/name;directory.mkdir(exist_ok=True);image.save(directory/'input.json')
        source=make_source(directory,image)
        start=time.perf_counter();model=compile_source(source,'Top');save(model,directory/'model.frontend.mlir')
        frontend_seconds=time.perf_counter()-start
        start=time.perf_counter();analyze(model,directory,cycles=64,emit_cpp=True);lower_seconds=time.perf_counter()-start
        transaction=read_trace(directory/'transaction-trace.json');rtl=read_trace(directory/'rtl-trace.json')
        assert transaction==rtl
        check_cpp(transaction,directory)
        traces.append(transaction)
        record=dict(name=name,isa=isa,text_sha256=hashlib.sha256(image.text).hexdigest(),text_bytes=len(image.text),frontend_seconds=frontend_seconds,lower_seconds=lower_seconds,
                    stages={stage:metrics(directory/('model.'+stage+'.mlir')) for stage in ('frontend','source','transaction','rtl')},
                    cpp=metrics(directory/'cpp/model.hpp'),methods=cpp_functions(directory/'cpp/model.hpp'))
        records.append(record)
        (out/'summary.json').write_text(json.dumps(dict(cases=records),indent=2)+'\n')
        print(name+': '+json.dumps({stage:row['operation_count'] for stage,row in record['stages'].items()})+'; C++ '+str(record['cpp']['lines'])+' lines',flush=True)
    assert traces[0]==traces[1],'same bytes changed CPU behavior with C enabled'
    source_files=[ROOT/'pycircuit/examples/ripes5/model/model.py',ROOT/'pycircuit/examples/ripes5/model/isa.py',ROOT/'pycircuit/mlir/CppLowering.cpp',ROOT/'pycircuit/mlir/CppEmitter.cpp',Path(__file__)]
    summary=dict(same_text=True,same_initial_registers_and_ram=True,cycles_checked=64,all_cpp_and_ir_traces_equal=True,
                 source_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source_files},cases=records)
    (out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')

if __name__=='__main__':main()
