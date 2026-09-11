"""Stable process exit codes shared by Agentic Circuit CLI commands."""

from enum import IntEnum


class ExitCode(IntEnum):
    SUCCESS = 0
    USER_INPUT = 2
    INTERNAL = 3
    BUILD = 4
    PREFLIGHT = 5
    SIMULATION = 6
    INCOMPLETE = 7
    INTERRUPTED = 130
