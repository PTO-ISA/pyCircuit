import agentic_circuit as ac

from designs.davincioo.contracts.spe import (
    IssueAttemptKey,
    OperandSourceDescriptor,
    valid_operand_source,
)


@ac.struct
class ContractRequest:
    left: IssueAttemptKey
    right: IssueAttemptKey
    operand: OperandSourceDescriptor
    same_attempt: bool
    valid_operand: bool


@ac.rule
def check_contracts(request) -> ContractRequest:
    return request.with_fields(
        same_attempt=request.left == request.right,
        valid_operand=valid_operand_source(request.operand),
    )


@ac.system
def contract_probe(
    request: ContractRequest,
) -> ContractRequest:
    verdict = check_contracts(request)
    return verdict
