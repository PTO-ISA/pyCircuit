from pycircuit import bits, module


@module
def scalar_parameterized_types(
    value: bits[width], *, width: int = 17
) -> {"result": bits[width]}:
    return {"result": value}
