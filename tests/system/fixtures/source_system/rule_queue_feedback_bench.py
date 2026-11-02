# ruff: noqa: F823 -- captured module-owned state proposal.
import pycircuit as ac
from checks.rule_queue_feedback_provider import BranchJoinedFeedback


@ac.system
def ExerciseBranchJoinedFeedback():
    phase: ac.u8 = 0
    owner = BranchJoinedFeedback()

    @ac.autorule
    def send() -> None:
        if phase < 3:
            owner.send(phase + 10)

    @ac.autorule
    def step() -> None:
        owner.step(phase)

    @ac.autorule
    def receive() -> None:
        token = owner.take()
        ac.log("info", "queue feedback retired", phase, token)

    @ac.autorule
    def tick() -> None:
        phase = phase + 1  # noqa: F841 - captured hardware state or child instance.
