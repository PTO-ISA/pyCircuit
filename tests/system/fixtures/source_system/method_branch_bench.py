import pycircuit as ac
from checks.method_branch_provider import BranchCounter


@ac.system
def ExerciseMethodBranches():
    owner = BranchCounter()

    @ac.autorule
    def observe_before() -> None:
        ac.log("info", "reader before", owner.read())

    @ac.autorule
    def drive() -> None:
        false_saved = owner.advance(0)
        true_saved = owner.advance(1)
        owner.snapshot()
        ac.log("info", "branch saved", false_saved, true_saved)

    @ac.autorule
    def observe_after() -> None:
        ac.log("info", "reader after", owner.read())
