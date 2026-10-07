"""Public source closure and finite typed-DUT/RTL computed-output oracles."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

parser=argparse.ArgumentParser()
for name in ("repo","source-compiler","linker","emitter","cxx","scratch"):
    parser.add_argument("--"+name,required=True)
parser.add_argument("--four-state",action="store_true")
for name in ("verilator","iverilog","vvp"):
    parser.add_argument("--"+name)
args=parser.parse_args()
if args.four_state:
    assert args.iverilog and args.vvp,"four-state gate requires Icarus and vvp"
else:
    assert args.verilator,"known-value gate requires Verilator"
repo=Path(args.repo).resolve()
scratch=Path(args.scratch).resolve()
scratch.mkdir(parents=True,exist_ok=True)
private=tempfile.TemporaryDirectory(prefix="computed-",dir=scratch)
root=Path(private.name)
source=root/"source"
source.mkdir()
fixtures=Path(__file__).resolve().parent
for name in ("computed_child.py","computed_parent.py","computed_bad_width.py",
             "computed_bad_call.py","computed_bad_add.py"):
    shutil.copyfile(fixtures/name,source/name)
env=dict(os.environ,PYTHONPATH=str(repo/"python/pycircuit/src"),
         PYTHONDONTWRITEBYTECODE="1",PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
         PYCIRCUIT_LINKER=args.linker,PYCIRCUIT_EMITTER=args.emitter)
commands=[]


def run(command,accepted=True):
    result=subprocess.run(list(map(str,command)),env=env,cwd=repo,
                          capture_output=True,text=True,timeout=180)
    record={"command":list(map(str,command)),"exit_status":result.returncode,
            "stdout":result.stdout,"stderr":result.stderr}
    commands.append(record)
    (scratch/"commands.json").write_text(json.dumps(commands,indent=2)+"\n")
    assert "Assertion failed" not in result.stderr and "Traceback" not in result.stderr,record
    assert result.returncode==(0 if accepted else 1),record
    return result


def cli(*arguments,accepted=True):
    return run([sys.executable,"-m","pycircuit.cli",*arguments],accepted)


def compile_unit(name,target,imports=(),replace=False):
    arguments=["compile","-c",source/name,"--source-root",source,
               "--package-prefix","computed","-o",target]
    for unit in imports:
        arguments.extend(("-I",unit))
    if replace:
        arguments.append("--replace")
    return arguments


def snapshot(path):
    return {p.relative_to(path).as_posix():p.read_bytes()
            for p in path.rglob("*") if p.is_file()}


child,parent=root/"child",root/"parent"
cli(*compile_unit("computed_child.py",child))
cli(*compile_unit("computed_parent.py",parent,[child]))
final=root/"computed.ac"
cli("link",child,parent,"--top","computed.computed_parent.Top","-o",final)
for target in ("cpp","verilog"):
    cli("emit",final,"--target",target,"-o",root/target)

# All rejected replacements have the already-published parent's source owner.
# Unit, linked artifact and generated products must survive each failure.
before=snapshot(parent)
final_before=final.read_bytes()
products={target:snapshot(root/target) for target in ("cpp","verilog")}
rejections=() if args.four_state else (("computed_bad_width.py","kinds cannot be implicitly converted"),
                                      ("computed_bad_call.py","unsupported hardware expression"),
                                      ("computed_bad_add.py","not proven within destination bounds"))
for name,diagnostic in rejections:
    absent=root/name.removesuffix(".py")
    rejected=cli(*compile_unit(name,absent),accepted=False)
    assert diagnostic in rejected.stderr.lower(),rejected.stderr
    assert not absent.exists()
    (source/"computed_parent.py").write_text((source/name).read_text())
    cli(*compile_unit("computed_parent.py",parent,replace=True),accepted=False)
    assert snapshot(parent)==before
    assert final.read_bytes()==final_before
    assert all(snapshot(root/t)==products[t] for t in products)

receipt=json.loads((root/"cpp/generated.json").read_text())
cpp=[root/"cpp"/entry["path"] for entry in receipt["files"]
     if entry["path"].endswith(".cpp")]
# Both supported layouts belong to the same checkout's compiler build/install.
toolroot=Path(args.source_compiler).resolve().parent.parent
candidates=[toolroot/"simulator/gfsim/libpyc6_runtime.a",toolroot/"lib/libpyc6_runtime.a"]
runtime=next((p for p in candidates if p.is_file()),None)
assert runtime is not None,"Runtime archive missing from compiler build/install"
executable=root/"computed-cpp"
stem="computed_four_state" if args.four_state else "computed_outputs"
run([args.cxx,"-std=c++20","-pthread","-I"+str(repo/"include"),
     "-I"+str(root/"cpp"),fixtures/(stem+".cpp"),*cpp,runtime,"-o",executable])
traces=[]
for workers in (1,2):
    traces.append([line for line in run([executable,str(workers)]).stdout.splitlines()
                   if line.startswith("WORK ")])
assert len(traces[0])==(6 if args.four_state else 8) and traces[0]==traces[1]
receipt=json.loads((root/"verilog/generated.json").read_text())
rtl=[root/"verilog"/entry["path"] for entry in receipt["files"] if entry["role"]=="rtl"]
rtl.sort(key=lambda p:(p.name!="design_top.sv",str(p)))
if args.four_state:
    rtl_executable=root/"computed-rtl"
    run([args.iverilog,"-g2012","-s","tb","-o",rtl_executable,*rtl,
         fixtures/(stem+".sv")])
    rtl_run=[args.vvp,rtl_executable]
else:
    rtl_build=root/"rtl-build"
    run([args.verilator,"--binary","--timing","--top-module","tb","--prefix","Vcomputed",
         "--Mdir",rtl_build,"-Wno-fatal",*rtl,fixtures/(stem+".sv")])
    rtl_run=[rtl_build/"Vcomputed"]
rtl_trace=[line for line in run(rtl_run).stdout.splitlines() if line.startswith("WORK ")]
assert rtl_trace==traces[0]
if args.four_state:
    print("computed outputs: public closure, 6 fixed X/Z mask C++/Icarus frames, workers 1/2 passed")
else:
    print("computed outputs: public closure, 8 golden C++/RTL frames, workers 1/2, retained diagnostics and publication protection passed")
