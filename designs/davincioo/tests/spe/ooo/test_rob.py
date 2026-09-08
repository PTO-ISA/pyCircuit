"""Generated gfsim expected results, scan parity, and opt-in recording parity.

Reproduce with the pyc6 environment:
  python -m pytest -q designs/davincioo/tests/spe/ooo/test_rob.py
Set PYC_DAVINCIOO_ROB_OUT to choose the retained artifact directory.
"""

from __future__ import annotations

import os
import shlex
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[5]
SCENARIOS = ("capacity", "backpressure", "invalid", "recovery", "conflicts")


def run(command, cwd):
    result = subprocess.run(command, cwd=cwd, text=True, capture_output=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return result


def check_recorded_fields(out, flows=2, system="dual_rob_system"):
    """Use the independent reader to check the actual recorded completion data."""
    run(
        [
            "env",
            f"PYTHONPATH={ROOT / 'third_party/circuit-flow-viewer/src'}",
            "python",
            "-c",
            f"flows = {flows}; system = {system!r}\n"
            + """
from pathlib import Path
from circuit_flow_viewer.reader import read_replay
replay = read_replay(Path('execution.pyctrace'))
assert replay.complete, replay.diagnostic
operations = [o for o in replay.manifest['objects'] if o['name'].startswith('firing_')]
expected = {f'{system}/rob[{i}]/{name}' for i in range(flows)
            for name in ('recover', 'acknowledge', 'complete', 'handoff', 'allocate')}
assert {o['display_path'] for o in operations} == expected
for o in operations:
    assert o['display_name'] == o['display_path'].rsplit('/', 1)[1]
    assert o['display_parent_path'] == o['display_path'].rsplit('/', 1)[0]

commits = [e['value'] for e in replay.events if e['action'] == 'enqueue'
           and e['object']['bits'] in tuple(str(4 * flows + 2 * i + 1) for i in range(flows))]
assert commits
def exact(value, bits, width):
    assert value == {'bits': str(bits), 'width': width, 'signed': False}
for value in commits:
    sequence = int(value['inst']['instruction_sequence']['bits'])
    exact(value['kind'], 4, 3)
    exact(value['status'], 2, 3)
    exact(value['fault_code'], 0xdeadbeef, 32)
    exact(value['fault_arg0'], 0xffffeeeeaaaabbbb, 64)
    exact(value['result'], 0xfedcba9876540000 + sequence, 64)
    exact(value['inst']['original_pc'], 0x1000000000000000 + sequence, 64)
    exact(value['result_valid'], 1, 1)
    assert value['rob']['slot']['width'] == 4
    assert value['epoch']['flow']['launch_generation']['width'] == 16
print('recursive values and enum widths verified')
""",
        ],
        out,
    )


def generate_rob(system, out, scenarios=SCENARIOS, single=False):
    import agentic_circuit as ac
    from agentic_circuit._jit import _lower_acir_to_cpp, _lower_queue_acir
    from agentic_circuit._queue_frontend import lower_queue_source

    system_name = "rob_system" if single else "dual_rob_system"
    out.mkdir(parents=True, exist_ok=True)
    specialization = ac.jit(system, workspace=ROOT)
    raw = lower_queue_source(specialization._source(), system_name, host_results=True)
    (out / "raw.mlir").write_text(raw)
    (out / "frozen.mlir").write_text(_lower_queue_acir(raw))
    (out / "model.cpp").write_text(_lower_acir_to_cpp(raw))
    shutil.copyfile(Path(__file__).with_name("rob_driver.cpp"), out / "driver.cpp")
    build = ROOT / ".pycircuit_out/local-clang22/build"
    cache = {}
    for line in (build / "CMakeCache.txt").read_text().splitlines():
        if "=" in line and not line.startswith(("//", "#")):
            key, value = line.split("=", 1)
            cache[key.split(":", 1)[0]] = value
    llvm_flags = run(["llvm-config", "--ldflags", "--libs", "support"], out).stdout
    command = [
        "c++",
        "-std=c++20",
        *(["-DDAVINCIOO_SINGLE_ROB"] if single else []),
        "-I",
        str(ROOT / "simulator/gfsim/include"),
        str(out / "driver.cpp"),
        str(build / "compiler/acir/gfsim/libgfsim.a"),
        str(build / "compiler/acir/lib/Bindings/libACIRBindings.a"),
        *shlex.split(llvm_flags),
        "-lrt",
        "-ldl",
        "-lm",
        cache["ZLIB_LIBRARY_RELEASE"],
        cache["zstd_LIBRARY"],
        "-o",
        str(out / "rob_driver"),
    ]
    (out / "compile-command.txt").write_text(shlex.join(command) + "\n")
    run(command, out)
    write_replay_index(out, scenarios, single)
    return out


def write_replay_index(out, scenarios, single=False):
    title = "DavinciOO 单 ROB 回放" if single else "DavinciOO 双 ROB 回放"
    descriptions = {
        "capacity": "满表阻塞、乱序完成、顺序交接与槽位复用",
        "backpressure": "分配输出和提交输出背压，解除阻塞后继续执行",
        "invalid": "错误身份、重复响应与不可靠确认的拒绝",
        "recovery": "flush 恢复、保留已交接事务与迟到响应",
        "conflicts": "同周期冲突与两个 flow 的独立运行",
    }
    (out / "index.html").write_text(
        f'<!doctype html><meta charset="utf-8"><title>{title}</title>'
        "<style>body{font-family:system-ui;max-width:900px;margin:48px auto;line-height:1.8}li{margin:14px 0}</style>"
        f"<h1>{title}</h1><p>各场景已通过独立期望值、scan/activation "
        "以及录制前后逐提交状态一致性检查。打开动画后可播放、单步和查看字段。</p><ul>"
        + "".join(
            f'<li><a href="{name}/replay.html">{descriptions[name]}</a> '
            f'（{name}） · <a href="{name}/execution.pyctrace">trace</a></li>'
            for name in scenarios
        )
        + '</ul><p><a href="model.cpp">生成模型</a> · '
        '<a href="driver.cpp">测试驱动</a> · '
        '<a href="compile-command.txt">编译命令</a></p>'
    )


@pytest.fixture(scope="module")
def generated_rob():
    from designs.davincioo.spe.ooo.rob import dual_rob_system

    out = Path(
        os.environ.get(
            "PYC_DAVINCIOO_ROB_OUT", ROOT / ".pycircuit_out/davincioo-rob/20260908-rob"
        )
    ).resolve()
    return generate_rob(dual_rob_system, out)


@pytest.mark.parametrize("scenario", SCENARIOS)
def test_rob_expected_results_and_replay_parity(generated_rob, scenario):
    verify_scenario(generated_rob, scenario)


def verify_scenario(generated_rob, scenario, flows=2, system="dual_rob_system"):
    out = generated_rob / scenario
    out.mkdir(exist_ok=True)
    executable = generated_rob / "rob_driver"
    normal = run([str(executable), scenario], out)
    (out / "normal.log").write_text(normal.stdout)
    projection = (out / "projection.bin").read_bytes()
    recorded = run(["env", "PYC_RECORD_REPLAY=1", str(executable), scenario], out)
    (out / "recorded.log").write_text(recorded.stdout)
    assert projection == (out / "projection.bin").read_bytes()
    assert normal.stdout == recorded.stdout
    check_recorded_fields(out, flows, system)
    viewer = ROOT / "third_party/circuit-flow-viewer/src"
    run(
        [
            "env",
            f"PYTHONPATH={viewer}",
            "python",
            "-m",
            "circuit_flow_viewer.cli",
            "render",
            "execution.pyctrace",
            "--output",
            "replay.html",
        ],
        out,
    )
