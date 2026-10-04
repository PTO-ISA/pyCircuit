"""skyzh's eight-slot ring and ten independent execution stations.

Queues contain state and occupancy; Signals form the combinational network.
All state owners commit at the same Xfer boundary. No extra pipeline Queues.
"""
from .frontend import *
from .execution import *
from .storage import *


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

    instruction = fetch(pc, memory)
    sources = operands(rename, registers, rob)
    allocation = dispatch(pc, instruction, sources, head, tail, stations, predictor)
    integer = alu(stations)
    memory_lanes = lsu(stations, stages, rob, head, memory)
    execution = broadcast(integer, memory_lanes)
    retirement = retire(rob, head)

    frontend = Frontend(pc, allocation, retirement)
    pointers = ROBPointers(head, tail, allocation, retirement)
    register_file = RegisterFile(rename, registers, allocation, retirement)
    memory_unit = Memory(memory, retirement)
    branch_predictor = Predictor(predictor, retirement)
    for i in range(8):
        ROBSlot(rob[i], retained_dest[i], i + 1, allocation, execution, retirement)
    for i in range(10):
        ReservationStation(stations[i], stages[i], i, allocation, execution, retirement)
