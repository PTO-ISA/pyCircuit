# ruff: noqa: F821, F842 -- Annotated Module bindings allocate compiler-owned instances.
import pycircuit as ac
from example_sibling_method_calls.Receiver import Receiver
from example_sibling_method_calls.Sender import Sender


@ac.system
def Top():  # noqa: N802
    receiver: Receiver
    unused: Receiver
    sender: Sender
    sender(receiver)
