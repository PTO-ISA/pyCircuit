# ruff: noqa: F841 -- autonomous instances are real hardware.
from pycircuit import module, autorule
from serial_methods.dynamic_provider import (
    AutonomousOwner,
    CandidateEntryOwner,
    ManualOwner,
)


@module
def IndependentTop():
    left = AutonomousOwner()
    right = AutonomousOwner()
    spectator = AutonomousOwner()


@module
def AtomicTop():
    left = ManualOwner()
    right = ManualOwner()
    spectator = AutonomousOwner()

    @autorule
    def step() -> None:
        left.advance()
        right.advance()



@module
def EntryTop():
    left = CandidateEntryOwner()
    right = CandidateEntryOwner()
    spectator = CandidateEntryOwner()
