"""Shared semantic-primitive input range and result-width rules."""

PRIMITIVE_MINIMUM_INPUT_WIDTH = 1
PRIMITIVE_MAXIMUM_INPUT_WIDTH = 64


def is_primitive_input_width(width: object) -> bool:
    return (
        type(width) is int
        and PRIMITIVE_MINIMUM_INPUT_WIDTH <= width <= PRIMITIVE_MAXIMUM_INPUT_WIDTH
    )


def _checked_width(width: object) -> int:
    if not is_primitive_input_width(width):
        raise ValueError("semantic primitive input width must be in [1, 64]")
    assert type(width) is int
    return width


def primitive_priority_index_width(input_width: object) -> int:
    width = _checked_width(input_width)
    return max(1, (width - 1).bit_length())


def primitive_count_width(input_width: object) -> int:
    width = _checked_width(input_width)
    return max(1, width.bit_length())
