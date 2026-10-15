import ast,copy,hashlib,json,struct
from pathlib import Path
from pycircuit.examples.ripes5.tests.program import ProgramImage
from pycircuit.examples.ripes5.tests.e2e import compare_native,check_oracle,native_run,NATIVE
from pycircuit.tests.support import read_trace
from pycircuit.examples.ripes5.tests.paths import ROOT
BASE=ROOT/'pycircuit/builds/benchmarks/rv32imc';OUT=ROOT/'pycircuit/builds/benchmarks/rv32imc-audit'
known=json.loads((ROOT/'pycircuit/examples/ripes5/tests/fixtures/reference-differences.json').read_text())
records=[];additional=[]
for summary in json.loads((BASE/'summary.json').read_text()):
 name=summary['name'];d=BASE/name
 if not (d/'input.json').exists():d=BASE/'legacy-live'/name
 image=ProgramImage.load(d/'input.json');assert image.checksum()==summary['input_sha256']
 tree=ast.parse((d/'source.py').read_text());literals={}
 for node in ast.walk(tree):
  if isinstance(node,ast.AnnAssign) and isinstance(node.target,ast.Name):
   if node.target.id in ('program','registers','data'):literals[node.target.id]=ast.literal_eval(node.value)
 assert literals['program']==image.words(image.text,4096)
 assert literals['registers']==image.registers and literals['data']==image.words(image.ram,4096)
 native=[json.loads(l) for l in (d/'native.jsonl').read_text().splitlines()]
 assert native[0]['registers']==image.registers and native[0]['data']==image.words(image.ram,4096)
 trace=read_trace(d/'transaction-trace.json');assert trace==read_trace(d/'rtl-trace.json')
 cppfile='sanitized-trace.json' if summary['cpp_metrics']['sanitized'] else 'trace.json'
 assert trace==read_trace(d/'cpp'/cppfile),(name,'cpp')
 assert all(row['cycle']==i for i,row in enumerate(native)),name
 diffs=compare_native(trace,copy.deepcopy(native),image,collect=True)
 assert diffs==(known[name]['expected_differences'] if name in known else []),(name,diffs)
 oracle=json.loads((d/'oracle.json').read_text());check_oracle(trace,image,oracle)
 elf=(d/'program.elf').read_bytes();table=int.from_bytes(elf[32:36],'little');stride=int.from_bytes(elf[46:48],'little');count=int.from_bytes(elf[48:50],'little');s=struct.unpack_from('<10I',elf,table+stride*int.from_bytes(elf[50:52],'little'));names=elf[s[4]:s[4]+s[5]]
 sections=[]
 for idx in range(count):
  sec=struct.unpack_from('<10I',elf,table+idx*stride);nm=names[sec[0]:].split(b'\0')[0].decode()
  if sec[2]&2 and sec[5]:
   sections.append((nm,sec[3],sec[5]))
   assert nm in ('.text','.data','.bss'),(name,nm)
   assert sec[3]==0 if nm=='.text' else sec[3]>=4096
 if name in ('load_branch_wrong_path','m_extremes','upstream_rvc'):
  fresh=OUT/'recapture'/name;fresh.mkdir(parents=True,exist_ok=True)
  current=native_run(image,fresh)
  nd=compare_native(trace,current,image,collect=True);assert nd==diffs
 records.append({'name':name,'cycles':len(trace),'input_cpp_ir_native_consistent':True,'expected_differences':len(diffs),'sections':sections})
 print(name+': recorded inputs, three traces and native projection rechecked',flush=True)
(OUT/'corpus-review.json').write_text(json.dumps({'count':len(records),'native_binary_sha256':hashlib.sha256(NATIVE.read_bytes()).hexdigest(),'records':records},indent=2)+'\n')
