import pycircuit as ac
from checks.multi_output_provider import Header, Payload, StatefulTuple


@ac.struct
class WrappedResult:
    header: Header
    payload: Payload
    valid: ac.u1
    repeated_header: Header
    repeated_payload: Payload
    repeated_valid: ac.u1


@ac.module
def Wrapped(payload: ac.u8, enable: ac.u1) -> WrappedResult:
    header, data, valid = StatefulTuple(payload, enable)
    return WrappedResult(
        header=header,
        payload=data,
        valid=valid,
        repeated_header=header,
        repeated_payload=data,
        repeated_valid=valid,
    )


@ac.rule
def advance_phase(phase):
    phase = phase + 1


@ac.system
def ExerciseMultiOutput():
    phase: ac.u8 = 0
    header, payload, valid = StatefulTuple(phase + 10, 1)
    wrapped = Wrapped(phase + 10, 1)

    @ac.rule
    def drive():
        assert header.sequence == wrapped.header.sequence
        assert header.sequence == wrapped.repeated_header.sequence
        assert payload.data == wrapped.payload.data
        assert payload.data == wrapped.repeated_payload.data
        assert valid == wrapped.valid
        assert valid == wrapped.repeated_valid
        ac.log(
            "info",
            "tuple outputs",
            header.sequence,
            payload.data,
            valid,
        )

    advance_phase(phase)
