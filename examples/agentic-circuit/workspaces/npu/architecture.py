from __future__ import annotations

from agentic_circuit import module, process, scope, system


@module
def top() -> None:
    Npu(name="trace_source")
    with scope("frontend"):
        NpuNode(name="frontend_decode")
        NpuNode(name="frontend_dispatch")
    with scope("backend"):
        NpuNode(name="backend_dependencies")
        NpuNode(name="backend_issue_scalar")
        NpuNode(name="backend_issue_vector")
        NpuNode(name="backend_issue_cube")
        NpuNode(name="backend_issue_tma")
    with scope("execution"):
        NpuNode(name="execution_scalar_unit_0")
        NpuNode(name="execution_vector_unit_0")
        NpuNode(name="execution_vector_unit_1")
        NpuNode(name="execution_cube_unit_0")
        NpuNode(name="execution_tma_unit_0")
    with scope("memory"):
        NpuNode(name="memory_load_store")
        NpuNode(name="memory_scratchpad")
        NpuNode(name="memory_controller")
    NpuNode(name="completion")
    NpuNode(name="retirement")


@process(kind="workload")
def workload() -> None:
    yield_sim()


@system(root="top")
def main() -> None:
    return
