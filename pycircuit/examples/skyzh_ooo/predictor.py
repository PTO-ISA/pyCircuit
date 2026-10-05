"""Branch retirement trains the native history and saturating counters."""
from .types import ac


@ac.module
def Predictor(predictor, retirement):
    @ac.rule
    def update():
        commit = retirement.value
        if commit.branch:
            address = commit.entry.dest
            history = predictor[address >> 8].value.history[address & 255]
            selected = address + (ac.u32(history) & 3)
            counter = predictor[selected >> 8].value.counters[selected & 255]
            if commit.taken:
                if counter < 3:
                    counter = counter + 1
            elif counter > 0:
                counter = counter - 1
            predictor[selected >> 8].value.counters[selected & 255] = counter
            predictor[address >> 8].value.history[address & 255] = ac.u8((ac.u32(history) << 1) | ac.u32(commit.taken))
    update()
