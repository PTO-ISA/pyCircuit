"""One execution stage. Its sole Rule owns both input pop and output push."""
from pycircuit import ac
from .logic import calculate


@ac.module
def Integer(requests, control):
    @ac.rule(capacity=1)
    def execute(message):
        request = message.value
        if request.tag.epoch == control.value.epoch and not control.value.stopped:
            return calculate(request)
        # The same consumer discards old-path messages, without producing output.
        return None

    completed = execute(requests)

    return completed
