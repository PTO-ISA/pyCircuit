import pycircuit as ac


@ac.rule
def add_one(source: ac.u8) -> ac.u8:
    return source + 1
