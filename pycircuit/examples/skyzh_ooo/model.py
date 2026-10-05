"""skyzh's eight-slot ring and ten independent execution stations.

Queues contain state and occupancy; Signals form the combinational network.
All state owners commit at the same Xfer boundary. No extra pipeline Queues.
"""
from .types import ac, MemoryPage, ROBEntry, Station, LSUState, RenameEntry, PredictorPage
from .frontend import Frontend
from .execution import ExecutionCluster
from .commit import CommitControl
from .reorder_buffer import ReorderBuffer
from .register_file import RegisterFile
from .memory import Memory
from .predictor import Predictor


@ac.module
def CPU(image: ac.vector[MemoryPage]):
    pc = ac.queue[ac.u32](initial=0)
    head = ac.queue[ac.u32](initial=1)
    tail = ac.queue[ac.u32](initial=1)
    rob = ac.array(ac.queue[ROBEntry], shape=(8,))
    retained_dest = ac.array(ac.queue[ac.u32], shape=(8,), initial=0)
    stations = ac.array(ac.queue[Station], shape=(10,))
    stages = ac.array(ac.queue[LSUState], shape=(10,), initial=LSUState())
    rename = ac.array(ac.queue[RenameEntry], shape=(32,), initial=RenameEntry())
    registers = ac.array(ac.queue[ac.u32], shape=(32,), initial=0)
    memory = [ac.queue[MemoryPage](initial=page) for page in image]
    predictor = ac.array(ac.queue[PredictorPage], shape=(16384,), initial=PredictorPage())

    # The three shared combinational buses. Queue feedback adds the state boundary.
    retirement = CommitControl(rob, head)
    allocation = Frontend(pc, memory, rename, registers, rob, head, tail,
                          stations, predictor, retirement)
    completion = ExecutionCluster(stations, stages, rob, head, memory, allocation, retirement)

    reorder_buffer = ReorderBuffer(rob, retained_dest, head, tail, allocation, completion, retirement)
    register_file = RegisterFile(rename, registers, allocation, retirement)
    memory_unit = Memory(memory, retirement)
    branch_predictor = Predictor(predictor, retirement)
