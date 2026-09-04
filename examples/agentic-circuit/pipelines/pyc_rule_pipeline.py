import agentic_circuit as ac


@ac.struct
class RuleToken:
    value: ac.u16


@ac.rule
def increment(token):
    return token.with_fields(value=token.value + 1)


@ac.system
def pyc_rule_pipeline() -> None:
    incoming = ac.source(RuleToken, depth=2, latency=1)
    outgoing = increment(incoming)
    ac.sink(outgoing)
