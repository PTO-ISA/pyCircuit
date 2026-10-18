import pycircuit as ac


def reverse_bits(value: ac.bits[100]) -> ac.bits[100]:
    reversed_value: ac.bits[100] = 0
    for _index in range(100):
        reversed_value = (reversed_value << 1) | (value & 1)
        value = value >> 1
    return reversed_value


@ac.module
def Top(incoming: ac.bits[100]) -> ac.bits[100]:  # noqa: N802
    return reverse_bits(incoming)
