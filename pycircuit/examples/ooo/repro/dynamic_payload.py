from pycircuit import ac

class Cell:
    done: bool = False

@ac.module
def Probe():
    rows = ac.queue[ac.array[Cell, 2]](initial=[Cell(), Cell()])
    index = ac.queue[ac.u32](initial=1)
    @ac.rule
    def update():
        rows.value[index.value].done = True
    update()
