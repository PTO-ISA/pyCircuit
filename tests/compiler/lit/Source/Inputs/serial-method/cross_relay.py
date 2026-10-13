# ruff: noqa: F841 -- retain a same-name local to test lexical binding.
from pycircuit import method, module
from serial_methods.cross_provider import Counter


@module
def Relay():
    counter = Counter()
    alias = counter

    @method
    def advance() -> None:
        bias = 11
        first = counter.read()
        next_value = counter.add(1)
        nested_saved = alias.chain(2)
        final_value = counter.add(4)
        lexical_old = counter.snapshot()
        counter.record(lexical_old, next_value, nested_saved, final_value,
                       counter.copy_payload())
