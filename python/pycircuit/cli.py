"""The single capture, link and final-codegen driver."""

from __future__ import annotations

import argparse
import sys


class _UniqueCycles(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        if getattr(namespace, self.dest, None) is not None:
            parser.error("duplicate --cycles")
        setattr(namespace, self.dest, values)


def _cmd_emit(args: argparse.Namespace) -> int:
    from ._driver import _DriverError
    from ._emit import emit_command
    from ._publication import _PublicationError
    from ._publication_fs import _PublicationFileSystemError

    try:
        emit_command(
            final=args.final,
            target=args.target,
            rtl_mode=args.rtl_mode,
            output=args.output,
            replace=args.replace,
        )
    except (
        _DriverError,
        _PublicationError,
        _PublicationFileSystemError,
        OSError,
        SyntaxError,
        ValueError,
        KeyError,
        TypeError,
    ) as error:
        print(f"pycircuit emit: {error}", file=sys.stderr)
        return 1
    return 0


def _cmd_compile(args: argparse.Namespace) -> int:
    from ._driver import _DriverError, compile_command
    from ._publication import _PublicationError
    from ._publication_fs import _PublicationFileSystemError

    try:
        compile_command(
            source=args.source,
            source_root=args.source_root,
            output=args.output,
            package_prefix=args.package_prefix,
            interface_units=args.interface_units,
            replace=args.replace,
        )
    except (
        _DriverError,
        _PublicationError,
        _PublicationFileSystemError,
        OSError,
        SyntaxError,
        ValueError,
    ) as error:
        print(f"pycircuit compile: {error}", file=sys.stderr)
        return 1
    return 0


def _cmd_link(args: argparse.Namespace) -> int:
    from ._driver import _DriverError, link_command
    from ._publication import _PublicationError
    from ._publication_fs import _PublicationFileSystemError

    try:
        link_command(
            units=args.units,
            top=args.top,
            output=args.output,
            parameters=args.parameters,
            reset_semantics=args.reset_semantics,
            replace=args.replace,
            target=args.target,
            from_anchor=args.from_anchor,
            verify_only=args.verify_only,
        )
    except (
        _DriverError,
        _PublicationError,
        _PublicationFileSystemError,
        OSError,
        SyntaxError,
        ValueError,
    ) as error:
        print(f"pycircuit link: {error}", file=sys.stderr)
        return 1
    return 0


def _cmd_run(args: argparse.Namespace) -> int:
    import subprocess

    from ._driver import _DriverError
    from ._run import run_command

    try:
        run_command(
            source=args.source,
            target=args.target,
            build_dir=args.build_dir,
            toolchain=args.toolchain,
            cycles=args.cycles,
            workers=args.workers,
            timeout=args.timeout,
        )
    except (
        _DriverError,
        OSError,
        ValueError,
        KeyError,
        TypeError,
        subprocess.TimeoutExpired,
    ) as error:
        print(f"pycircuit run: {error}", file=sys.stderr)
        return 1
    return 0


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(prog="pycircuit", allow_abbrev=False)
    sub = p.add_subparsers(dest="cmd", required=True)

    compile_units = sub.add_parser(
        "compile",
        help="Compile one Python source into one published source unit.",
    )
    compile_units.add_argument(
        "-c",
        dest="source",
        required=True,
        help="The one Python source file to compile",
    )
    compile_units.add_argument(
        "--source-root", required=True, help="Capture confinement root for the source"
    )
    compile_units.add_argument(
        "--package-prefix",
        default="",
        help="Dotted source-unit package prefix (default: empty)",
    )
    compile_units.add_argument(
        "-I",
        dest="interface_units",
        action="append",
        default=[],
        help="A published managed interface unit directory (repeatable)",
    )
    compile_units.add_argument(
        "-o", dest="output", required=True, help="Published source unit directory"
    )
    compile_units.add_argument(
        "--replace",
        action="store_true",
        help="Republish the target under the same owner and artifact kind",
    )
    compile_units.set_defaults(fn=_cmd_compile)

    link_program = sub.add_parser(
        "link",
        help="Link an explicitly listed unit closure into one verified final design.",
    )
    link_program.add_argument(
        "units",
        nargs="*",
        help="Published source unit directories: the complete closure",
    )
    link_program.add_argument("--top", help="Qualified module of the source graph root")
    link_program.add_argument(
        "--parameters",
        default=None,
        help='JSON file of {"name": "W", "value": "8"} Integer overrides; omitted names use defaults',
    )
    link_program.add_argument(
        "--reset-semantics",
        choices=("strict", "if_one"),
        default=None,
        help="Root reset interpretation (source default: strict)",
    )
    link_program.add_argument(
        "-o",
        dest="output",
        help="Final artifact file or saved anchor directory",
    )
    link_program.add_argument(
        "--replace",
        action="store_true",
        help="Republish the target under the same owner and definition",
    )
    link_program.add_argument("--target", choices=("final", "anchor"), default=None)
    link_program.add_argument(
        "--from-anchor", help="Reload a self-contained saved anchor"
    )
    link_program.add_argument("--verify-only", action="store_true")
    link_program.set_defaults(fn=_cmd_link)

    emit = sub.add_parser("emit", help="Emit C++ or Verilog from a saved final design.")
    emit.add_argument("final")
    emit.add_argument("--target", choices=("cpp", "verilog"), required=True)
    emit.add_argument("--rtl-mode", choices=("managed", "physical"), default="managed")
    emit.add_argument("-o", dest="output", required=True)
    emit.add_argument("--replace", action="store_true")
    emit.set_defaults(fn=_cmd_emit)
    run = sub.add_parser(
        "run", help="Build and simulate a closed source system.", allow_abbrev=False
    )
    run.add_argument("source", help="Independent example directory")
    run.add_argument("--target", choices=("cpp", "verilog"), default="cpp")
    run.add_argument("--build-dir")
    run.add_argument("--toolchain", help="Installed pyCircuit prefix")
    run.add_argument("--cycles", type=int, default=None, action=_UniqueCycles)
    run.add_argument("--workers", type=int, default=1)
    run.add_argument(
        "--timeout", type=int, default=180, help="Seconds per build/run stage"
    )
    run.set_defaults(fn=_cmd_run)
    args = p.parse_args(argv)
    if args.cmd == "link":
        if args.from_anchor is None:
            missing = []
            if not args.units:
                missing.append("units")
            if args.top is None:
                missing.append("--top")
            if args.output is None:
                missing.append("-o")
            if missing:
                link_program.error(
                    "the following arguments are required: " + ", ".join(missing)
                )
        elif (
            args.units
            or args.top is not None
            or args.parameters is not None
            or args.reset_semantics is not None
        ):
            link_program.error(
                "--from-anchor forbids units, --top, --parameters and --reset-semantics"
            )
        if args.verify_only:
            if (
                args.from_anchor is None
                or args.output is not None
                or args.target is not None
                or args.replace
            ):
                link_program.error(
                    "--verify-only requires --from-anchor and forbids output and transformation options"
                )
        elif args.from_anchor is not None and (
            args.target != "final" or args.output is None
        ):
            link_program.error(
                "--from-anchor refinement requires --target final and -o"
            )
    try:
        return args.fn(args)
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    raise SystemExit(main())
