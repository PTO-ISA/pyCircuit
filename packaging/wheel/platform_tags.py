from __future__ import annotations

import os
import platform


def _normalize_system(system: str | None = None) -> str:
    raw = (system or platform.system()).strip().lower()
    aliases = {
        "darwin": "macos",
        "macos": "macos",
        "mac": "macos",
        "linux": "linux",
        "windows": "windows",
        "win32": "windows",
        "cygwin": "windows",
        "msys": "windows",
    }
    return aliases.get(raw, raw)


def _normalize_arch(system: str, arch: str | None = None) -> str:
    raw = (arch or platform.machine()).strip().lower()
    if system == "macos":
        aliases = {
            "aarch64": "arm64",
            "arm64": "arm64",
            "x86_64": "x86_64",
            "amd64": "x86_64",
        }
        return aliases.get(raw, raw)
    if system == "windows":
        aliases = {
            "amd64": "amd64",
            "x86_64": "amd64",
            "arm64": "arm64",
            "aarch64": "arm64",
        }
        return aliases.get(raw, raw)
    aliases = {
        "amd64": "x86_64",
        "x64": "x86_64",
        "x86_64": "x86_64",
        "arm64": "aarch64",
        "aarch64": "aarch64",
    }
    return aliases.get(raw, raw)


def _macos_target(arch: str, deployment_target: str | None = None) -> tuple[str, str]:
    target = (
        deployment_target or os.environ.get("MACOSX_DEPLOYMENT_TARGET", "")
    ).strip()
    if not target:
        target = "11.0" if arch == "arm64" else "10.13"
    parts = (target.split(".") + ["0", "0"])[:2]
    return parts[0], parts[1]


def wheel_plat_name(
    *,
    system: str | None = None,
    arch: str | None = None,
    deployment_target: str | None = None,
) -> str:
    normalized_system = _normalize_system(system)
    normalized_arch = _normalize_arch(normalized_system, arch)

    if normalized_system == "macos":
        major, minor = _macos_target(normalized_arch, deployment_target)
        return f"macosx_{major}_{minor}_{normalized_arch}"
    if normalized_system == "linux":
        return f"linux_{normalized_arch}"
    if normalized_system == "windows":
        return f"win_{normalized_arch}"
    raise SystemExit(f"unsupported platform for wheel packaging: {normalized_system}")
