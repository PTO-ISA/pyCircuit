"""The single capture, link and final-codegen driver."""

from __future__ import annotations

import argparse
import sys


def _cmd_emit(args: argparse.Namespace) -> int:
    from ._driver import _DriverError
    from ._emit import emit_command
    from ._publication import _PublicationError
    from ._publication_fs import _PublicationFileSystemError

    try:
        emit_command(
            final=args.final,
            target=args.target,
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
        print(f"pycircuit link: {error}", file=sys.stderr)
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
        nargs="+",
        help="Published source unit directories: the complete closure",
    )
    link_program.add_argument(
        "--top", required=True, help="Qualified module of the source graph root"
    )
    link_program.add_argument(
        "--parameters",
        default=None,
        help="Ordered JSON bindings for the root's static parameters",
    )
    link_program.add_argument(
        "-o",
        dest="output",
        required=True,
        help="Final design artifact path (for example design_top.ac)",
    )
    link_program.add_argument(
        "--replace",
        action="store_true",
        help="Republish the target under the same owner and definition",
    )
    link_program.set_defaults(fn=_cmd_link)

    emit = sub.add_parser("emit", help="Emit C++ or Verilog from a saved final design.")
    emit.add_argument("final")
    emit.add_argument("--target", choices=("cpp", "verilog"), required=True)
    emit.add_argument("-o", dest="output", required=True)
    emit.add_argument("--replace", action="store_true")
    emit.set_defaults(fn=_cmd_emit)
    args = p.parse_args(argv)
    try:
        return args.fn(args)
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    raise SystemExit(main())
