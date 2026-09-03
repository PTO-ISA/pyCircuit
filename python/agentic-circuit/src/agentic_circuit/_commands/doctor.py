"""Read-only installed-toolchain checks."""

from __future__ import annotations

import shutil
import sys
from dataclasses import dataclass
from typing import Literal

from .._canonical_json import canonical_json_bytes
from .._capabilities import standard_library_catalog
from .._native_api import capabilities
from .._output import OutputSink


@dataclass(frozen=True, slots=True)
class DoctorCheck:
    name: str
    status: Literal["passed", "failed"]
    observed: str
    required: str
    diagnostic_code: str | None

    def to_json(self) -> dict[str, str | None]:
        return {
            "name": self.name,
            "status": self.status,
            "observed": self.observed,
            "required": self.required,
            "diagnostic_code": self.diagnostic_code,
        }


def _check(name: str, passed: bool, observed: str, required: str) -> DoctorCheck:
    return DoctorCheck(
        name=name,
        status="passed" if passed else "failed",
        observed=observed,
        required=required,
        diagnostic_code=None if passed else "ACPY-DOCTOR-001",
    )


def run(arguments: object, sink: OutputSink) -> int:
    checks: list[DoctorCheck] = []
    python_version = (
        f"{sys.version_info.major}.{sys.version_info.minor}."
        f"{sys.version_info.micro}"
    )
    checks.append(
        _check("python", sys.version_info >= (3, 11), python_version, ">=3.11")
    )
    checks.append(_check("contract_epoch", True, "0.4", "0.4"))

    try:
        native = capabilities()
    except (ImportError, RuntimeError, TypeError, ValueError) as error:
        checks.append(_check("native_extension", False, str(error), "available"))
        compiler_build_id = "unavailable"
        runtime_build_id = "unavailable"
    else:
        compiler_build_id = native.compiler_build_id
        runtime_build_id = native.runtime_build_id
        checks.append(_check("native_extension", True, "available", "available"))
    checks.append(
        _check(
            "llvm_mlir",
            "llvm-22.1.8" in compiler_build_id,
            compiler_build_id,
            "LLVM/MLIR 22.1.8",
        )
    )
    checks.append(
        _check(
            "gfsim_source_contract",
            "cxx20" in runtime_build_id,
            runtime_build_id,
            "gfsim-cxx20@0.1",
        )
    )

    compiler = shutil.which("c++")
    checks.append(
        _check("cxx_compiler", compiler is not None, compiler or "missing", "C++20")
    )
    try:
        catalog = standard_library_catalog()
        catalog_identity = f"{catalog.get('catalog')}@{catalog.get('version')}"
    except (OSError, TypeError, ValueError) as error:
        catalog_identity = str(error)
    checks.append(
        _check(
            "standard_library",
            catalog_identity == "ac@0.1",
            catalog_identity,
            "ac@0.1",
        )
    )
    canonical = canonical_json_bytes({"epoch": "0.4"})
    checks.append(
        _check(
            "canonical_json",
            canonical == b'{"epoch":"0.4"}',
            canonical.decode("utf-8"),
            'RFC 8785 {"epoch":"0.4"}',
        )
    )
    passed = all(check.status == "passed" for check in checks)
    document = {
        "schema": "agentic-circuit-doctor-result",
        "version": "0.1",
        "contract_epoch": "0.4",
        "status": "passed" if passed else "failed",
        "checks": [check.to_json() for check in checks],
    }
    sink.result(document, human=f"doctor: {document['status']}")
    return 0 if passed else 3
