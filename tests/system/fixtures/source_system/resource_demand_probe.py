# ruff: noqa: F823, F841 -- captured hardware locals and state proposals.
import pycircuit as ac


@ac.system
def ResourceDemandProbe():
    selector: ac.u1 = 0
    early: ac.u1 = 1
    later: ac.u1 = 1
    send_left: ac.u1 = 0
    send_right: ac.u1 = 0
    take: ac.u1 = 0
    count: ac.u8 = 0
    left = ac.queue[ac.u8](in_valid=send_left, in_data=11, depth=1, latency=1)
    right = ac.queue[ac.u8](in_valid=send_right, in_data=11, depth=1, latency=1)
    output = ac.queue[ac.u8](out_ready=take, depth=1, latency=1)

    @ac.method(guard=lambda enabled: enabled)
    def require(enabled: ac.u1) -> None:
        pass

    @ac.rule
    def drive() -> None:
        require(early)
        require(later)
        if selector:
            value = left.pop()
        else:
            value = right.pop()
        output.push(value)

    @ac.rule
    def tick() -> None:
        count = count + 1
