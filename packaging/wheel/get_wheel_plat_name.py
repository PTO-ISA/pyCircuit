#!/usr/bin/env python3
from __future__ import annotations

import argparse

from platform_tags import wheel_plat_name


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="Print the wheel platform tag for the current or requested host."
    )
    ap.add_argument(
        "--system",
        default=None,
        help="Override platform.system() for cross-platform checks",
    )
    ap.add_argument(
        "--arch",
        default=None,
        help="Override platform.machine() for cross-platform checks",
    )
    ap.add_argument(
        "--deployment-target",
        default=None,
        help="Override MACOSX_DEPLOYMENT_TARGET when computing macOS wheel tags",
    )
    args = ap.parse_args(argv)
    print(
        wheel_plat_name(
            system=args.system, arch=args.arch, deployment_target=args.deployment_target
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
