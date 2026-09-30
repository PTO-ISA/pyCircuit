import os, subprocess, json, sys
from pathlib import Path
root=Path.cwd()
out=root/".pycircuit_out/decl-tests"/sys.argv[1]
out.mkdir(parents=True,exist_ok=True)
build=root/".pycircuit_out/w10-pm/build"
env=dict(os.environ)
for key,name in {"ACIR_SOURCE_UNIT_HARNESS":"acir-source-unit-harness","ACIR_DESIGN_HARNESS":"acir-design-harness","ACIR_CPP_SOURCE_PARTS_HARNESS":"acir-cpp-source-parts-harness","ACIR_BACKEND_CLOSURE_HARNESS":"acir-backend-closure-harness"}.items(): env[key]=str(build/"bin"/name)
pytest="/opt/homebrew/Cellar/pytest/9.0.2_1/libexec/bin/python"
lanes={
"scalar":[pytest,"-m","pytest","tests/system/test_final_scalar_declarations.py","-q"],
"regression":[pytest,"-m","pytest",*["tests/system/"+n+".py" for n in ["test_cpp_source_parts","test_generic_multi_assignment","test_generic_assignment_roundtrip","test_source_design_bridge","test_masked_next_register"]],"-q"],
"source-driver":[pytest,"-m","pytest",*["tests/system/"+n+".py" for n in ["test_source_unit_pair_verification","test_source_namespace_bindings","test_driver_compile_link","test_source_unit_cmake_build"]],"-q"],
"native":["ctest","--test-dir",str(build),"-R","^(ACIRFinalProgramTests|ACIRExecutableBackendClosureTests)$","--output-on-failure"],
}
for name in sys.argv[2:]:
    command=list(lanes[name])
    if name!="native":command.append("--junitxml="+str(out/(name+".xml")))
    else:command += ["--output-junit",str(out/(name+".xml"))]
    (out/(name+"-command.json")).write_text(json.dumps({"cwd":str(root),"command":command,"env":{k:v for k,v in env.items() if k.startswith("ACIR_")}},indent=2)+"\n")
    with (out/(name+".log")).open("w") as log: result=subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT)
    (out/(name+"-exit.txt")).write_text(str(result.returncode)+"\n")
    print(name,result.returncode,flush=True)
    if result.returncode:sys.exit(result.returncode)
