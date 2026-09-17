"""Stable Queue frontend facade and source-lowering orchestration."""

from __future__ import annotations

from collections.abc import Mapping

from ._queue_compiler.errors import QueueFrontendError
from ._queue_compiler.expressions import _ExpressionEmitter  # noqa: F401
from ._queue_compiler.lower_acir import lower_queue_program
from ._queue_compiler.model import Payload  # noqa: F401
from ._queue_compiler.modules import _lower_simple_module_source
from ._queue_compiler.normalize import (  # noqa: F401
    _normalize_rule_field_assignments,
)
from ._queue_compiler.parser import (  # noqa: F401
    RULE_LOWERING_PIPELINE,
    parse_queue_program,
)
from ._queue_compiler.provenance import build_queue_acpy  # noqa: F401
from ._queue_compiler.source import (
    _DEFAULT_QUEUE_SOURCE_PATH,  # noqa: F401 - compatibility re-export
)
from ._queue_compiler.static_types import (
    MAX_PACKED_VALUE_WIDTH,  # noqa: F401 - compatibility re-export
    StaticParameterAlias,  # noqa: F401 - compatibility re-export
    _config_schema_document,  # noqa: F401 - compatibility re-export
    _project_static_config_value,  # noqa: F401 - compatibility re-export
    _static_constraint,  # noqa: F401 - compatibility re-export
    _static_parameter_value,  # noqa: F401 - compatibility re-export
)
from ._source_map import SourceNodeLocations
from ._static_eval import StaticValue


def lower_queue_source(
    text: str,
    system: str,
    static_arguments: Mapping[str, StaticValue] | None = None,
    specialization_fingerprint: str | None = None,
    *,
    host_results: bool = False,
    source_path: str | None = None,
    definition_locations: Mapping[str, tuple[str, int, int]] | None = None,
    static_assert_locations: (
        Mapping[str, tuple[tuple[str, int, int], ...]] | None
    ) = None,
    source_node_locations: SourceNodeLocations | None = None,
) -> str:
    if lowered := _lower_simple_module_source(
        text,
        system,
        static_arguments=static_arguments,
        specialization_fingerprint=specialization_fingerprint,
        host_results=host_results,
        source_path=source_path,
        definition_locations=definition_locations,
        static_assert_locations=static_assert_locations,
        source_node_locations=source_node_locations,
    ):
        return lowered
    if host_results:
        raise QueueFrontendError(
            "ACPY-MODULE-006: host result boundaries require structured modules"
        )
    return lower_queue_program(
        parse_queue_program(
            text,
            system,
            static_arguments=static_arguments,
            specialization_fingerprint=specialization_fingerprint,
            source_path=source_path,
            definition_locations=definition_locations,
            static_assert_locations=static_assert_locations,
            source_node_locations=source_node_locations,
        )
    )
