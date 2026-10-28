from typing import Annotated

from pycircuit import dff, module


@module
def Word(a: Annotated[int, range(1 << 8)]) -> Annotated[int, range(1 << 8)]:
    return a


@module
def WordState(clk: bool, rst: bool, data: Annotated[int, range(1 << 8)]) -> Annotated[int, range(1 << 8)]:
    state = dff(T=Annotated[int, range(1 << 8)])
    state(clk=clk, rst=rst, d=data, init=0)
    return state.q


@module
def BitState(clk: bool, rst: bool, data: Annotated[int, range(1 << 1)]) -> Annotated[int, range(1 << 1)]:
    state = dff(T=Annotated[int, range(1 << 1)])
    state(clk=clk, rst=rst, d=data, init=0)
    return state.q
