import os
import shlex
import shutil
import subprocess
import tempfile

import lit.formats

config.name = "pycircuit"
config.test_format = lit.formats.ShTest(execute_external=True)
config.suffixes = [".mlir", ".test"]
config.excludes = ["Inputs", "lit.cfg.py", "lit.site.cfg.py"]
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(
    config.pycircuit_obj_root, "tests", "compiler", "lit"
)
os.makedirs(config.test_exec_root, exist_ok=True)

for name, path in (
    ("%pycircuit_opt", config.pycircuit_opt),
    ("%pycircuit_emit", config.pycircuit_emit),
    ("%pycircuit_source_unit", config.pycircuit_source_unit),
    ("%pycircuit_link", config.pycircuit_link),
    ("%FileCheck", config.filecheck),
    ("%cxx", config.cxx),
    ("%python", config.python),
    ("%include", os.path.join(config.pycircuit_src_root, "include")),
    ("%src", config.pycircuit_src_root),
):
    config.substitutions.append((name, shlex.quote(path)))
config.substitutions.append(
    ("%inputs", shlex.quote(os.path.join(config.test_source_root, "CodeGen", "Inputs")))
)
config.environment["PATH"] = (
    os.path.dirname(config.filecheck) + os.pathsep + os.environ.get("PATH", "")
)
config.environment["PYTHONDONTWRITEBYTECODE"] = "1"
for name in ("verilator", "iverilog", "vvp"):
    path = shutil.which(name)
    if path:
        config.available_features.add(name)
        config.substitutions.append(("%" + name, shlex.quote(path)))
# Probe the actual packed-struct/type-parameter X/Z behavior rather than
# inferring four-state capability from a tool's name or mere presence.
iverilog, vvp = shutil.which("iverilog"), shutil.which("vvp")
if iverilog and vvp:
    with tempfile.TemporaryDirectory(
        prefix="rtl-capability-", dir=config.test_exec_root
    ) as probe:
        binary = os.path.join(probe, "probe")
        source = os.path.join(
            config.test_source_root, "CodeGen", "Inputs", "four_state_probe.sv"
        )
        primitive = os.path.join(
            config.pycircuit_src_root, "include", "verilog", "dffe.v"
        )
        try:
            build = subprocess.run(
                [iverilog, "-g2012", "-s", "tb", "-o", binary, primitive, source],
                capture_output=True,
                timeout=15,
            )
            if build.returncode == 0:
                run = subprocess.run([vvp, binary], capture_output=True, timeout=15)
                if run.returncode == 0:
                    config.available_features.add("rtl-four-state-parameter-types")
        except (OSError, subprocess.TimeoutExpired):
            pass
