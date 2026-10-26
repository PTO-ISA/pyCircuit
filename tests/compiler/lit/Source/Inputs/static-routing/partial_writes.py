"""Disjoint routed writers retain each contributor's own enable and mask."""

# ruff: noqa: F823, F841 -- captured state writes and hardware allocations.
import pycircuit as ac


@ac.struct
class Packet:
    left: ac.u8
    right: ac.u8
    held: ac.u8


@ac.module
def Bank():
    packet: Packet = Packet(left=5, right=9, held=ac.four_state("10xz0011"))
    entries: ac.table[4, ac.u8] = (5, 9, ac.four_state("10xz0011"), 23)

    @ac.method
    def read() -> tuple[Packet, ac.table[4, ac.u8]]:
        return packet, entries

    @ac.method
    def put_left(field: ac.u8, element: ac.u8) -> None:
        packet.left = field
        entries[0] = element

    @ac.method
    def put_right(field: ac.u8, element: ac.u8) -> None:
        packet.right = field
        entries[1] = element

    @ac.method
    def put_disabled(index: ac.u2, incoming: ac.u8) -> None:
        packet.left = incoming
        entries[index] = incoming

    @ac.rule
    def inspect() -> None:
        ac.log("info", "partial-state", packet.left, packet.right, packet.held,
               entries[0], entries[1], entries[2], entries[3])



@ac.module
def LeftWorker(choose_disabled: ac.u1, bank: Remote):
    progress: ac.u8 = 0

    @ac.rule(guard=lambda: not choose_disabled)
    def step() -> None:
        saved_packet, saved_entries = bank.read()
        bank.put_left(saved_packet.left + 3, saved_entries[0] + 7)
        candidate_packet, candidate_entries = bank.read()
        progress = progress + 1
        ac.log("info", "partial-left", saved_packet.left, saved_packet.right,
               candidate_packet.left, candidate_packet.right,
               saved_entries[0], saved_entries[1], candidate_entries[0], candidate_entries[1],
               candidate_packet.held, candidate_entries[2], candidate_entries[3])



@ac.module
def RightWorker(bank: Remote):
    progress: ac.u8 = 0

    @ac.rule
    def step() -> None:
        saved_packet, saved_entries = bank.read()
        bank.put_right(saved_packet.right + 5, saved_entries[1] + 11)
        candidate_packet, candidate_entries = bank.read()
        progress = progress + 1
        ac.log("info", "partial-right", saved_packet.left, saved_packet.right,
               candidate_packet.left, candidate_packet.right,
               saved_entries[0], saved_entries[1], candidate_entries[0], candidate_entries[1],
               candidate_packet.held, candidate_entries[2], candidate_entries[3])



@ac.module
def DisabledWorker(enabled: ac.u1, index: ac.u2, bank: Remote):
    progress: ac.u8 = 0

    @ac.rule(guard=lambda: enabled)
    def step() -> None:
        bank.put_disabled(index, 199)
        progress = progress + 1
        ac.log("info", "disabled-writer-ran", progress)



@ac.module
def Remote():
    bank = Bank()
    @ac.method
    def read() -> tuple[Packet, ac.table[4, ac.u8]]:
        packet, entries = bank.read()
        return packet, entries
    @ac.method
    def put_left(field: ac.u8, element: ac.u8) -> None:
        bank.put_left(field, element)
    @ac.method
    def put_right(field: ac.u8, element: ac.u8) -> None:
        bank.put_right(field, element)
    @ac.method
    def put_disabled(index: ac.u2, incoming: ac.u8) -> None:
        bank.put_disabled(index, incoming)


@ac.module
def LeftBranch(choose_disabled: ac.u1, bank: Remote):
    worker = LeftWorker(choose_disabled, bank)


@ac.module
def RightBranch(enabled: ac.u1, bank: Remote):
    worker = RightWorker(bank)
    disabled = DisabledWorker(enabled, 0, bank)


@ac.system(samples=((0, 0), (1, 0), (1, 0), (0, 0), (1, 1), (0, 0), (1, 0)))
def Top():
    disabled_gate: ac.u1 = 0
    remote = Remote()
    left = LeftBranch(disabled_gate, remote)
    right = RightBranch(disabled_gate, remote)
    phase: ac.u8 = 0

    @ac.rule
    def advance() -> None:
        phase = phase + 1

    @ac.rule
    def check() -> None:
        assert phase < 255, "partial_late_failure"
