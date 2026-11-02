"""Verify the generated starter against its independently supplied vectors."""

import argparse
import subprocess
from pathlib import Path


# ndf: verifies=MODEL-VER-001
def verify_project(build_dir):
    project = Path(__file__).resolve().parents[1]
    subprocess.run(
        ["pycircuit", "verify", str(project), "--build-dir", str(build_dir), "--rtl"],
        check=True,
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    verify_project(parser.parse_args().build_dir.resolve())
