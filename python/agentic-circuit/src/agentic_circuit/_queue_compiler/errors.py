"""Queue frontend diagnostics."""

from .._diagnostics import DiagnosticError


class QueueFrontendError(DiagnosticError):
    """A stable rejection from the queue frontend."""
