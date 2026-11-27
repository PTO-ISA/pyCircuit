# Independent sparse lifecycle acceptance

Run `python designs/sparsefa/tests/integration/run.py` from this checkout.
The runner recompiles every explicit provider and consumer into isolated output
units, links three generated Systems, then emits both targets and runs C++ workers
1 and 2 when linking succeeds. It records each command and exit status under
ignored `docs/gates/logs/fa2-p0-completion/integration-*`.

The System executes eight commands: successful unsorted sparse causal attention,
duplicate rejection, out-of-range token rejection, count overflow rejection,
cancellation with an outstanding request, successful restart, empty indexed
attention, and accepted count2048 followed by an invalid first token. Its memory model delays responses and stalls requests. Assertions
check stable request fields, original masked token identity, distinct K/V strides,
validation before Q access, bounded output count, completion after drain, held
completion identity, and no duplicate completion.

Q=1, K=[0,1,7], V=[2,6,8], and indices=[2,0,1] with causal position1. The independent
binary64 expectation is `(2 + 6*exp(1))/(1 + exp(1)) = 4.92423431452002`; FP16
round-to-nearest bits are17645. The accepted hardware range is one FP16 step
around that value, allowing the separately declared truncation profile. The
memory model supplies original operands and metadata, never computed DUT results.

`vectors.py` adds BF16 D2 attention with unequal scores and signed outputs,
using the declared BF16 atol=.07/rtol=.02; and a FP16 D64 sparse singleton
with 64 distinct output addresses and an exact repeating signed V oracle.
The count2048 scenario intentionally fails its first token range check, proving
the capacity boundary is accepted without a long duplicate-validation run.

Current status: all three Systems pass independent compile, default-budget link,
same-final C++/Verilog emission, and generated C++ workers 1 and 2. Lifecycle,
vectors and faults terminate at epochs 4000, 10000 and 6000 with their required
pass events. Inputs and installed tools remained unchanged throughout the run.
See [VALIDATION.md](../../VALIDATION.md) for exact candidate evidence and remaining
coverage. These bounded cases do not establish full-domain precision or execution
of a valid 2048-element index list.

`faults.py` adds ten focused commands with a two-lane singleton whose exact
output is V=[2,-3]. It injects memory errors at COUNT, INDEX, Q, K, the second V
lane, and the first O response; then invalid Q/K/V operands (FP16 infinity,
finite16, NaN); then a clean successful restart. The V fault happens after a
partial shadow-vector write, and must publish no output. An O-response error
may leave its already-issued write in memory, but must return code5 and issue no
second lane; the entire result is invalid. The responder holds each pending
transaction until `response_ready` accepts its response. All expected values
are independent constants; no numeric helper is used by the memory oracle.

Verilog simulation is explicitly excluded by the user. The runner retains both
C++ and managed Verilog emission from each successfully linked final artifact.
