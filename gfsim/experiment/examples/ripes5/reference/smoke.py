"""Verify original Ripes CLI register, cycle and pipeline reporting."""
import argparse
import json
from pathlib import Path
import subprocess
from ..run import DEFAULT_RUNNER, ROOT, native_env


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ripes', type=Path, default=DEFAULT_RUNNER.with_name('Ripes'))
    parser.add_argument('--output', type=Path,
                        default=ROOT / 'reference/benchmarks/ripes5-native/cli-smoke.json')
    args = parser.parse_args()
    env = native_env()
    # QApplication/QSettings must not write to the user's desktop configuration.
    env['XDG_CONFIG_HOME'] = '/tmp/gfsim-ripes-config'
    env['XDG_CACHE_HOME'] = '/tmp/gfsim-ripes-cache'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    result = subprocess.run([str(args.ripes), '--mode', 'cli', '--src',
                             str(Path(__file__).with_name('smoke.s')), '-t', 'asm',
                             '--proc', 'RV32_5S', '--cycles', '--iret', '--regs',
                             '--pipeline', '--json', '--timeout', '10000', '--output', str(args.output)],
                            env=env, text=True, capture_output=True, check=True, timeout=30)
    args.output.with_suffix('.stdout.txt').write_text(result.stdout)
    args.output.with_suffix('.stderr.txt').write_text(result.stderr)
    report = json.loads(args.output.read_text())
    assert int(report['registers']['x3']) == 12, report
    assert report['cycles'] > 0 and report['# instructions retired'] > 0 and report['pipeline'], report
    print(f'Original CLI: x3=12, {report["cycles"]} cycles, {report["# instructions retired"]} retired; pipeline report saved')


if __name__ == '__main__':
    main()
