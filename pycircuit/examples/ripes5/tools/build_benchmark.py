"""Build all three runners with the same GCC 14 / C++20 release settings."""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
from ..tests.verify import ROOT, sha, v, fingerprint

REFERENCE = ROOT / 'gfsim/experiment/examples/ripes5/reference'
FLAGS = '-O3 -DNDEBUG -march=armv8-a -mtune=generic -fno-lto'


def run(*args):
    subprocess.run([str(x) for x in args], check=True, cwd=ROOT)


def build_record(directory, compiler):
    commands = json.loads((directory / 'compile_commands.json').read_text())
    # Store effective commands, not just cache flags (upstream may override them).
    for entry in commands:
        if Path(shlex.split(entry['command'])[0]).resolve() != compiler.resolve():
            raise AssertionError(f'unexpected compiler: {entry}')
    links = {str(p.relative_to(directory)): p.read_text()
             for p in sorted(directory.rglob('link.txt')) if '_deps' not in p.parts}
    return dict(directory=str(directory), commands=commands, links=links,
                translation_units_sha256={e['file']: sha(e['file']) for e in commands if Path(e['file']).is_file()},
                unbuilt_generated_units=[e['file'] for e in commands if not Path(e['file']).is_file()],
                cmake_cache=(directory / 'CMakeCache.txt').read_text())


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build', type=Path, default=ROOT / 'reference/builds/ripes5-benchmark')
    p.add_argument('--native-build', type=Path, default=ROOT / 'reference/builds/ripes-reference')
    p.add_argument('--cxx', type=Path, default=Path('/home/lc/opt/gcc14/bin/aarch64-conda-linux-gnu-g++'))
    p.add_argument('--qt-prefix', type=Path, default=Path('/tmp/gfsim-ripes-qt'))
    p.add_argument('-j', type=int, default=4)
    a = p.parse_args()
    a.build, a.native_build, a.cxx = a.build.resolve(), a.native_build.resolve(), a.cxx.resolve()
    os.environ['RIPES_QT_PREFIX'] = str(a.qt_prefix.resolve())
    version = subprocess.check_output([str(a.cxx), '-dumpfullversion'], text=True).strip()
    if not version.startswith('14.'):
        p.error('this comparison requires GCC 14')
    run(sys.executable, REFERENCE / 'build.py', '--build', a.native_build, '--cxx', a.cxx,
        '--qt-prefix', a.qt_prefix, '--release-flags', FLAGS, '--reference-only', '-j', a.j)
    native = a.native_build / 'ripes5-reference'
    run('cmake', '-S', ROOT / 'pycircuit', '-B', a.build, '-DCMAKE_BUILD_TYPE=Release',
        f'-DCMAKE_CXX_COMPILER={a.cxx}', f'-DCMAKE_CXX_FLAGS_RELEASE={FLAGS}',
        '-DCMAKE_CXX_STANDARD=20', '-DCMAKE_CXX_EXTENSIONS=OFF',
        '-DCMAKE_POSITION_INDEPENDENT_CODE=ON', '-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF',
        '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', f'-DGFSIM_RIPES_REFERENCE={native}')
    run('cmake', '--build', a.build, '-j', a.j)
    binaries = dict(generated=a.build / 'acpy-ripes5-compiled',
                    handwritten=a.build / 'gfsim/gfsim-ripes5', native=native,
                    emitted=a.build / 'acpy-ripes5-emitted')
    report = dict(compiler=str(a.cxx), compiler_sha256=sha(a.cxx), version=version,
                  compiler_version=subprocess.check_output([str(a.cxx), '--version'], text=True),
                  flags=FLAGS, standard='c++20', lto=False,
                  qt_prefix=str(a.qt_prefix.resolve()),
                  upstream_lock=json.loads((REFERENCE / 'version.json').read_text()),
                  local_source_sha256={**fingerprint(), **{'gfsim/' + k: value for k, value in v.fingerprint().items()}},
                  reference=v.verify_runner(native),
                  binaries={k: dict(path=str(b), sha256=sha(b)) for k, b in binaries.items()},
                  builds={k: build_record(d, a.cxx) for k, d in
                          (('gfsim', a.build), ('native', a.native_build))})
    path = a.build / 'build-manifest.json'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(f'Build identity: {path}', flush=True)


if __name__ == '__main__':
    main()
