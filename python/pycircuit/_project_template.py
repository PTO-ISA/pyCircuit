"""Small portable projects using the current, explicit source API."""

import json


def project_files(template):
    if template not in ("compute", "stateful", "backpressure"):
        raise ValueError("unknown project template")
    recurrent = template != "compute"
    depth, latency, consumer = (2, 2, 6) if template == "backpressure" else (2, 1, 0)
    expression = "total + normalized" if recurrent else "normalized"
    source = (
        '''# ruff: noqa: F823 -- captured module-owned state proposals.
"""Normalize incoming telemetry words and track a wrapping checksum."""
import pycircuit as ac

@ac.struct
class Request:
    tag: ac.u8
    enable: ac.u1
    value: ac.u16
    mask: ac.u16

@ac.struct
class Response:
    tag: ac.u8
    valid: ac.u1
    invalid: ac.u1
    value: ac.u16

@ac.struct
class State:
    total: ac.u16
    completed: ac.u16
    rejected: ac.u16

@ac.module
def Worker():
    total: ac.u16 = 0
    completed: ac.u16 = 0
    rejected: ac.u16 = 0

    @ac.method
    def process(request: Request) -> Response:
        valid: ac.u1 = request.mask != 0
        normalized: ac.u16 = request.value ^ request.mask
        value: ac.u16 = EXPRESSION
        if valid:
            total = value
        else:
            rejected = rejected + 1
        completed = completed + 1
        return Response(tag=request.tag, valid=valid, invalid=~valid,
                        value=value if valid else 0)

    @ac.method
    def snapshot() -> State:
        return State(total=total, completed=completed, rejected=rejected)

@ac.module(execution={
    "worker": "worker", "ingress": "ingress", "egress": "egress",
    "process": "process", "snapshot": "snapshot", "submit": "submit",
    "take": "take", "transfer": "transfer", "enabled": "enable",
    "valid": "valid", "accepted": "accepted", "interval": 1, "wait_state": "",
})
def Pipeline():
    ingress = ac.queue[Request](depth=DEPTH, latency=1)
    egress = ac.queue[Response](depth=1, latency=LATENCY)
    worker = Worker()
    accepted: ac.u16 = 0

    @ac.method
    def submit(request: Request) -> ac.u1:
        if request.enable:
            ingress.push(request)
            accepted = accepted + 1
        return 1

    @ac.method
    def take() -> Response:
        return egress.pop()

    @ac.method
    def snapshot() -> tuple[ac.u16, ac.u16, ac.u16, ac.u16]:
        state = worker.snapshot()
        return accepted, state.total, state.completed, state.rejected

    @ac.rule
    def transfer() -> None:
        request = ingress.pop()
        response = worker.process(request)
        if response.valid:
            egress.push(response)
'''.replace(
            "EXPRESSION", expression
        )
        .replace("DEPTH", str(depth))
        .replace("LATENCY", str(latency))
    )
    rows = [
        (0, 1, 1, 0x1234, 0x00FF),
        (1, 1, 1, 0xABCD, 0xF0F0),
        (2, 1, 0, 99, 7),
        (3, 1, 1, 7, 0),
        (4, 0, 1, 15, 3),
        (5, 1, 1, 0xFFF0, 0x00F0),
    ]
    requests = [
        {
            "active": active,
            "request": {"tag": tag, "enable": enable, "value": value, "mask": mask},
        }
        for tag, active, enable, value, mask in rows
    ]
    # Independent supplied vectors. These constants are not used inside DUTs.
    values = [4811, 28168, 27912] if recurrent else [4811, 23357, 65280]
    publications = [
        {"tag": tag, "valid": 1, "invalid": 0, "value": value}
        for tag, value in zip((0, 1, 5), values, strict=True)
    ]
    completions = (
        publications[:2]
        + [{"tag": 3, "valid": 0, "invalid": 1, "value": 0}]
        + publications[2:]
    )
    expected = {
        "publications": publications,
        "functional_completions": completions,
        "final_state": [4, values[-1], 4, 1],
    }
    project = {
        "schema": "pycircuit-model-project-v1",
        "model": "design.model.Pipeline",
        "inputs": "requests.json",
        "expectations": "expected.json",
        "cycles": 16,
        "consumer_start": consumer,
    }
    cmake = """cmake_minimum_required(VERSION 3.25)
project(ModelProject LANGUAGES CXX)
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
include("${PYCIRCUIT_CMAKE_DIR}/PycircuitExamples.cmake")
pycircuit_add_example(model PACKAGE design TOP design.model.Pipeline
  ENTRY_SOURCE model SOURCES model.py MODEL_SOURCES model.py CYCLES 16)
"""
    readme = """# Source model project

Edit model.py, requests.json and independent expected.json together. One source
selects functional or timing execution; C++/RTL are separate backend choices.
The full role declaration is explicit current API; no automatic hierarchy
projection or independent interstage workers are claimed.

From a directory outside this project, with PYC_TOOLCHAIN_ROOT selected:

```sh
pycircuit doctor --rtl
pycircuit verify path/to/project --build-dir .pycircuit_out/check --rtl
pycircuit benchmark path/to/project --output .pycircuit_out/measurement
```

verify compares exact completion flags/raw values, valid publication order and
final architectural state with the supplied expectations. It does not prove
arbitrary cycle timing or synthesis. Detailed timing oracles belong to tests.
benchmark reports separately native loop and process/I/O costs, independent
exact packet/state checks per batch, warmups and raw repeats. The project model
is not edited to run a benchmark. No second functional model or private driver
is needed. Existing benchmark output directories are never overwritten.
"""
    files = {"model.py": source, "CMakeLists.txt": cmake, "README.md": readme}
    for name, data in (
        ("project.json", project),
        ("requests.json", requests),
        ("expected.json", expected),
    ):
        files[name] = json.dumps(data, indent=2) + "\n"
    return files
