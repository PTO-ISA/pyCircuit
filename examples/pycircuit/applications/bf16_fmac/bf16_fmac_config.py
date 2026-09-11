from __future__ import annotations

DEFAULT_PARAMS: dict[str, int] = {}

TB_PRESETS = {
    "smoke": {"timeout": 32, "finish": 6},
    "nightly": {"timeout": 128, "finish": 32},
}

SIM_TIER = "heavy"
