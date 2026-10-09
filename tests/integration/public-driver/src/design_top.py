from pycircuit import bits, rule, system
from source_public.counter import Counter


@rule
def transfer_outputs(left_output, right_output, left_count, right_count):
    left_output = left_count  # noqa: F841
    right_output = right_count  # noqa: F841


@system
def DesignTop():  # noqa: N802
    left_input: bits[8] = 3
    right_input: bits[8] = 10
    left_output: bits[8] = 100
    right_output: bits[8] = 200

    left = Counter(left_input, left_output)
    right = Counter(right_input, right_output)
    transfer_outputs(left_output, right_output, left.count, right.count)
