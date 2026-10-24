"""Ordinary public state provider, independently published before consumers."""

# ruff: noqa: F823, F841 -- assignments propose writes to captured hardware state.
import pycircuit as ac


@ac.module
def Counter():
    count: ac.u8 = 13
    payload: ac.u8 = ac.four_state("10xz0011")

    @ac.method
    def read() -> ac.u8:
        return count

    @ac.method
    def write(incoming: ac.u8) -> None:
        count = incoming

    @ac.method
    def read_payload() -> ac.u8:
        return payload
