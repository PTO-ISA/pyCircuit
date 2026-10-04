"""Build pinned upstream sources and the observation-only adapter without patches."""
import argparse
import json
from pathlib import Path
import os
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]


def command(*args, **kwargs):
    return subprocess.run([str(a) for a in args], check=True, **kwargs)


def git(path, *args):
    return subprocess.check_output(['git', '-C', str(path), *args], text=True).strip()


def verify(path, sha):
    if git(path, 'rev-parse', 'HEAD') != sha:
        raise RuntimeError(f'{path}: checkout must be {sha}; existing checkout was not changed')
    if git(path, 'status', '--porcelain', '--untracked-files=no'):
        raise RuntimeError(f'{path}: modified third-party source cannot certify original Ripes')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'reference/ripes-reference')
    parser.add_argument('--build', type=Path, default=ROOT / 'reference/builds/ripes-reference')
    parser.add_argument('--qt-prefix', type=Path, default=Path('/tmp/gfsim-ripes-qt'))
    parser.add_argument('--cxx', default='/home/lc/opt/gcc14/bin/aarch64-conda-linux-gnu-g++')
    parser.add_argument('-j', type=int, default=8)
    parser.add_argument('--release-flags', default='-O3 -DNDEBUG')
    parser.add_argument('--reference-only', action='store_true')
    parser.add_argument('--fresh', action='store_true', help='discard CMake configuration cache before configuring')
    args = parser.parse_args()
    lock = json.loads((HERE / 'version.json').read_text())
    if not args.source.exists():
        args.source.parent.mkdir(parents=True, exist_ok=True)
        command('git', 'clone', '--recursive', lock['repository'], args.source)
        command('git', '-C', args.source, 'checkout', '--detach', lock['ripes'])
        command('git', '-C', args.source, 'submodule', 'update', '--init', '--recursive')
    verify(args.source, lock['ripes'])
    for path, sha in lock['submodules'].items():
        verify(args.source / path, sha)
    # Reuse pinned dependency checkouts after relocating the build tree.
    # FetchContent's old population sub-builds contain absolute CMake paths.
    dependencies = []
    for name, sha in lock['fetchcontent'].items():
        checkout = args.build.resolve() / '_deps' / f'{name}-src'
        if checkout.exists():
            verify(checkout, sha)
            dependencies.append(f'-DFETCHCONTENT_SOURCE_DIR_{name.upper()}={checkout}')
    command('cmake', *(['--fresh'] if args.fresh else []), '-S', args.source, '-B', args.build,
            '-DCMAKE_BUILD_TYPE=Release', f'-DCMAKE_PREFIX_PATH={args.qt_prefix}',
            f'-DCMAKE_CXX_COMPILER={args.cxx}',
            f'-DCMAKE_CXX_FLAGS_RELEASE={args.release_flags}',
            '-DCMAKE_CXX_EXTENSIONS=OFF', '-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF',
            f'-DCMAKE_PROJECT_Ripes_INCLUDE={HERE / "inject.cmake"}', *dependencies)
    for name, sha in lock['fetchcontent'].items():
        verify(args.build / '_deps' / f'{name}-src', sha)
    env = os.environ.copy()
    env.setdefault('CCACHE_DIR', '/tmp/gfsim-ripes-ccache')
    targets = ['ripes5-reference'] if args.reference_only else ['Ripes', 'ripes5-reference']
    command('cmake', '--build', args.build, '--target', *targets, '-j', args.j, env=env)
    if not args.reference_only:
        print(f'Built original CLI: {args.build / "Ripes"}')
    print(f'Built reference: {args.build / "ripes5-reference"}')


if __name__ == '__main__':
    main()
