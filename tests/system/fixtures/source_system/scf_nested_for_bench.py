# ruff: noqa: B007, F821, F823, F841 -- capture-only dead names and state proposals.
import pycircuit as ac
from checks.scf_nested_for_provider import NestedValues


@ac.system
def ExerciseNestedValues():
    phase: ac.u2 = 0
    seed: ac.u8 = 250
    helper, module_sum, binding = NestedValues(seed)

    @ac.rule
    def inspect_bindings() -> None:
        # Dead outer and inner bodies preserve incoming typed binders.
        outer: ac.u8 = 7
        inner: ac.u8 = 9
        for outer in range(0):
            for inner in range(2):
                dead = missing_dead_name
        else:
            for inner in range(0):
                dead = missing_dead_name
            else:
                preserved = outer + inner
        for outer in range(1):
            for inner in range(0):
                dead = missing_dead_name
        preserved = preserved + outer + inner

        # Loops remain ordinary statements within an outer else suite.
        completed: ac.u8 = 0
        for outer in range(0):
            pass
        else:
            if phase == 0:
                for inner in range(2):
                    completed = completed + 1
            else:
                for inner in range(1):
                    completed = completed + 3
            match phase:
                case 0:
                    for inner in range(1):
                        completed = completed + 1
                case _:
                    completed = completed + 2
        # Control in the inner else targets the still-active outer loop.
        for outer in range(3):
            for inner in range(1):
                completed = completed + 1
            else:
                match outer:
                    case 1:
                        continue
                    case 2:
                        break
                    case _:
                        pass
            completed = completed + 10
        else:
            completed = completed + 100
        ac.log(
            "info",
            "nested values",
            helper,
            module_sum,
            binding,
            preserved,
            completed,
        )

    @ac.rule
    def tick() -> None:
        seed = seed + 1
        phase = phase + 1
