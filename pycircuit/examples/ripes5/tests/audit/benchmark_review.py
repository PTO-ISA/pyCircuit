"""Audit frozen benchmark binaries with paired runs and a same-byte C control."""
import hashlib,json,os,platform,statistics,subprocess,time
from pathlib import Path
from pycircuit import analyze,compile_source
from pycircuit.examples.ripes5.tests.program import ProgramImage,make_source
from pycircuit.examples.ripes5.tests.e2e import NATIVE,project
from pycircuit.tests.support import cpp_compiler
from pycircuit.examples.ripes5.tests.paths import ROOT
BASE=ROOT/'pycircuit/builds/benchmarks/rv32imc-performance';OUT=ROOT/'pycircuit/builds/benchmarks/rv32imc-audit';K=1000;N=100000
core=min(os.sched_getaffinity(0));env=dict(os.environ);env['LD_LIBRARY_PATH']=str(ROOT/'pycircuit/builds/ripes-qt/lib')+':/home/lc/opt/gcc14/lib:'+env.get('LD_LIBRARY_PATH','')
meta={'host':platform.uname()._asdict(),'cpu':core,'compiler':subprocess.check_output(cpp_compiler()+['--version'],text=True).splitlines()[0], 'native_binary_sha256':hashlib.sha256(NATIVE.read_bytes()).hexdigest(),'warmup_cycles':K,'measured_cycles':N,'repetitions':5,'order':'alternating native-first / C++-first; one process at a time','cases':[]}
# Same exact 32-bit instruction bytes, but enable C in both implementations.
d=OUT/'im_bytes_c_enabled';d.mkdir(exist_ok=True);image=ProgramImage.load(BASE/'im/input.json');image.isa='IMC';image.save(d/'input.json')
analyze(compile_source(make_source(d,image),'Top'),d,emit_cpp=True)
cpp=d/'cpp';(cpp/'bench.cpp').write_text((BASE/'im/cpp/bench.cpp').read_text())
subprocess.run(cpp_compiler()+['-std=c++20','-O3',str(cpp/'bench.cpp'),'-o',str(cpp/'bench-O3')],check=True)
for label,d in [('im',BASE/'im'),('imc',BASE/'imc'),('im_bytes_c_enabled',d)]:
 image=ProgramImage.load(d/'input.json');binary=d/'cpp/bench-O3';record={'name':label,'input_sha256':image.checksum(),'text_sha256':hashlib.sha256(image.text).hexdigest(),'cpp_binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'runs':[]}
 for r in range(5):
  pair={}
  for engine in (['native','cpp'] if r%2==0 else ['cpp','native']):
   command=[str(NATIVE),str(d/'input.json'),'--benchmark-fixed',str(K),str(N)] if engine=='native' else [str(binary)]
   raw=json.loads(subprocess.check_output(['taskset','-c',str(core)]+command,env=env,text=True));pair[engine]=raw
  actual=project(pair['cpp']['final_state'],len(image.text));ref=pair['native']['final_state']
  for key in ('registers','data','retired'):assert actual[key]==ref[key],(label,key)
  assert actual['pc']==ref['fetch_pc']
  assert pair['native']['cycles']==K+N and pair['native']['measured_cycles']==N
  assert pair['native']['retired_delta']==75000
  record['runs'].append({'native_ns':pair['native']['run_ns'],'cpp_ns':pair['cpp']['run_ns'],'retired_delta':pair['native']['retired_delta'],'final_state_equal':True})
  print(label,r,record['runs'][-1],flush=True)
 med_cpp=statistics.median(x['cpp_ns'] for x in record['runs']);med_native=statistics.median(x['native_ns'] for x in record['runs'])
 record.update(cpp_cycles_per_second=N*1e9/med_cpp,native_cycles_per_second=N*1e9/med_native,speedup=med_native/med_cpp)
 meta['cases'].append(record);(OUT/'benchmark-review.json').write_text(json.dumps(meta,indent=2)+'\n')
print(json.dumps(meta,indent=2),flush=True)
