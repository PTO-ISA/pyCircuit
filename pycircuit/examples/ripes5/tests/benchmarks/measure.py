import os,json,time,subprocess,statistics,hashlib,sys
from pathlib import Path
from pycircuit import analyze,compile_source
from pycircuit.examples.ripes5.tests.program import compile_program,make_source
from pycircuit.examples.ripes5.tests.e2e import NATIVE,project
from pycircuit.tests.support import cpp_compiler
from pycircuit.examples.ripes5.tests.paths import ROOT
OUT=ROOT/'pycircuit/builds/benchmarks/rv32imc-performance'
K=1000;N=100000;REPEATS=5
ASM=(Path(__file__).resolve().parents[1]/'programs/benchmarks/mixed_loop.S').read_text()
BENCH=Path(__file__).with_name('runner.cpp').read_text()
env=dict(os.environ);env['LD_LIBRARY_PATH']=str(ROOT/'pycircuit/builds/ripes-qt/lib')+':/home/lc/opt/gcc14/lib:'+env.get('LD_LIBRARY_PATH','')
results=[]
for isa in ('IM','IMC'):
 d=OUT/isa.lower();image=compile_program(ASM,d,isa,max_cycles=K+N+8)
 source=make_source(d,image)
 start=time.perf_counter();analyze(compile_source(source,'Top'),d,emit_cpp=True);lower=time.perf_counter()-start
 cpp=d/'cpp';(cpp/'bench.cpp').write_text(BENCH.replace('WARMUP',str(K)).replace('MEASURED',str(N)))
 row=dict(isa=isa,program_sha256=image.checksum(),warmup_cycles=K,measured_cycles=N,repetitions=REPEATS,lowering_seconds=lower,program_bytes=len(image.text),cpp_source_bytes=(cpp/'model.hpp').stat().st_size,cpp={})
 native=[]
 for r in range(REPEATS):
  output=subprocess.check_output([str(NATIVE),str(d/'input.json'),'--benchmark-fixed',str(K),str(N)],env=env,text=True)
  raw=json.loads(output);(d/f'native-{r}.json').write_text(output);native.append(raw)
 row['native']={'run_ns':[r['run_ns'] for r in native],'construct_ns':[r['construct_ns'] for r in native]}
 print(isa+' native '+str(row['native']['run_ns']),flush=True)
 for optimization in ('O1','O3'):
  binary=cpp/('bench-'+optimization)
  start=time.perf_counter();subprocess.run(cpp_compiler()+['-std=c++20','-'+optimization,str(cpp/'bench.cpp'),'-o',str(binary)],check=True);built=time.perf_counter()-start
  runs=[]
  for r in range(REPEATS):
   output=subprocess.check_output([str(binary)],text=True);raw=json.loads(output)
   (d/f'cpp-{optimization}-{r}.json').write_text(output)
   actual=project(raw['final_state'],len(image.text));reference=native[r]['final_state']
   for key in ('registers','data','retired'):
    assert actual[key]==reference[key],(isa,optimization,key)
   assert actual['pc']==reference['fetch_pc'],(isa,optimization,'pc')
   runs.append(raw)
  row['cpp'][optimization]={'compile_seconds':built,'run_ns':[r['run_ns'] for r in runs],'construct_ns':[r['construct_ns'] for r in runs],'final_native_equal':True}
  print(isa+' '+optimization+' '+str(row['cpp'][optimization]['run_ns']),flush=True)
 row['native']['median_cycles_per_second']=N*1e9/statistics.median(row['native']['run_ns'])
 row['native']['retired_in_window']=native[0]['retired_delta']
 for value in row['cpp'].values():
  value['median_cycles_per_second']=N*1e9/statistics.median(value['run_ns'])
  value['speedup_vs_native']=statistics.median(row['native']['run_ns'])/statistics.median(value['run_ns'])
 results.append(row)
 (OUT/'summary.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results,indent=2),flush=True)
