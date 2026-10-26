import pycircuit as ac


@ac.struct
class LoopItem:
    value: ac.u8
    remaining: ac.u2


@ac.module
def BranchJoinedFeedback():
    ingress = ac.queue[LoopItem](depth=2, latency=1)
    feedback = ac.queue[LoopItem](depth=1, latency=1)
    completed = ac.queue[LoopItem](depth=1, latency=1)

    @ac.method
    def send(value: ac.u8) -> None:
        ingress.push(LoopItem(value=value, remaining=2))

    @ac.method
    def step(cycle: ac.u8) -> None:
        if feedback.valid:
            item = feedback.pop()
        else:
            item = ingress.pop()
        if item.remaining > 0:
            feedback.push(LoopItem(value=item.value + 1, remaining=item.remaining - 1))
        else:
            completed.push(item)
        ac.log("info", "queue feedback visited", cycle, item.value)

    @ac.method
    def take() -> ac.u8:
        item = completed.pop()
        return item.value
