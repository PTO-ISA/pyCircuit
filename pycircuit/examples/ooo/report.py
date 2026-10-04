"""Produce bounded, fingerprinted acceptance and timing evidence."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import shutil
import sys
import xml.etree.ElementTree as ET
sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from pycircuit.examples.ooo.verify import HERE, ROOT, sha


def lines(path):
    return len(path.read_text().splitlines())


def gate(path):
    tree = ET.parse(path).getroot()
    assert tree.attrib['failures'] == '0'
    assert tree.attrib.get('skipped', '0') == '0'
    return dict(tests=int(tree.attrib['tests']), failures=0,
                cases=[dict(name=e.attrib['name'], seconds=float(e.attrib['time'])) for e in tree.findall('testcase')])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build', type=Path, default=Path('/tmp/acpy-ooo-build'))
    p.add_argument('--asan-build', type=Path, default=Path('/tmp/acpy-ooo-asan'))
    args = p.parse_args()
    acceptance = json.loads((HERE / 'output/Release/comparison.json').read_text())
    sanitizer = json.loads((HERE / 'output/Debug/comparison.json').read_text())
    timing = json.loads((HERE / 'timing.json').read_text())
    for release_case, sanitized_case in zip(acceptance['results'], sanitizer['results']):
        assert [c['trace_sha256'] for c in release_case['configurations']] == [
            c['trace_sha256'] for c in sanitized_case['configurations']], 'cross-compiler cycle trace mismatch'
    assert acceptance['full_acceptance'] and sanitizer['full_acceptance']
    assert timing['acceptance_sha256'] == sha(HERE / 'output/Release/comparison.json')
    assert [(r['name'], r['cycles'], r['coverage']) for r in acceptance['results']] == [
        (r['name'], r['cycles'], r['coverage']) for r in sanitizer['results']]
    hardware = [HERE / name for name in ('model.py', 'types.py', 'frontend.py', 'issue.py', 'execute.py', 'commit.py')]
    host = [HERE / name for name in ('runner.cpp', 'reference.py', 'verify.py', 'run.py', 'programs.py',
                                    'bench.py', 'report.py', 'test_tools.py', 'input_gates.py')]
    generated = args.build / 'examples/ooo/compiled'
    counts = dict(hardware_acpy=sum(map(lines, hardware)), host_tools_tests=sum(map(lines, host)),
                  runner_cpp=lines(HERE / 'runner.cpp'),
                  repro_sources=sum(lines(p) for p in (HERE / 'repro').iterdir() if p.suffix in ('.py', '.cpp')),
                  assembly=sum(lines(p) for p in (HERE / 'programs').glob('*.s')),
                  generated_cpp=lines(generated / 'model.cpp'), generated_hpp=lines(generated / 'model.hpp'),
                  acir_bytes=(generated / 'model.acir.mlir').stat().st_size)
    evidence = HERE / 'output/gates'
    evidence.mkdir(exist_ok=True)
    for name, source in [('release.xml', args.build / 'ctest.xml'), ('sanitizers.xml', args.asan_build / 'ctest.xml')]:
        shutil.copyfile(source, evidence / name)
    source_paths = sorted(p for p in HERE.rglob('*') if p.is_file() and 'output' not in p.parts
                          and p.suffix in ('.py', '.cpp', '.s', '.md', '.txt') and p.name != 'report.md')
    compiler_paths = sorted((ROOT / 'pycircuit').glob('*.py')) + [ROOT / 'pycircuit/support.hpp']
    runtime_paths = sorted((ROOT / 'gfsim/cpp/include').rglob('*.hpp')) + sorted((ROOT / 'gfsim/cpp/src').glob('*.cpp'))
    result = dict(date_utc=datetime.now(timezone.utc).isoformat(), acceptance=acceptance,
                  sanitizer_runs=sanitizer['runs'], sanitizer_binaries=sanitizer['binaries'],
                  gates=dict(release=gate(args.build / 'ctest.xml'), sanitizers=gate(args.asan_build / 'ctest.xml')),
                  line_counts=counts, timing_sha256=sha(HERE / 'timing.json'),
                  example_sources_sha256={str(f.relative_to(ROOT)): sha(f) for f in source_paths},
                  compiler_and_runtime_sha256={str(f.relative_to(ROOT)): sha(f) for f in compiler_paths + runtime_paths},
                  generated_sha256={name: sha(generated / name) for name in ('model.cpp', 'model.hpp', 'model.acir.mlir')})
    (HERE / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
    text = ['# 乱序 CPU 验收报告', '',
        f"记录时间：{result['date_utc']}。详细指纹和配置见 [results.json](results.json)。", '',
        f"20 场景 × 4 个调度配置 × 2 条生成路径 = {acceptance['runs']} 次 Release 运行，"
        f"另有 {sanitizer['runs']} 次 ASan/UBSan 运行。每次均逐条对照独立顺序解释器，"
        '同一构建的八份完整轨迹逐拍一致；两个编译器构建的完整轨迹另行逐拍比对。', '',
        f"现有工程完整 CTest：{result['gates']['release']['tests']}/"
        f"{result['gates']['release']['tests']} 通过。Sanitizer 下新示例 CTest："
        f"{result['gates']['sanitizers']['tests']}/{result['gates']['sanitizers']['tests']} 通过。", '',
        '## 程序结果', '', '| 场景 | 周期 | 提交数 | IPC |', '| --- | ---: | ---: | ---: |']
    for row in acceptance['results']:
        text.append(f"| {row['name']} | {row['cycles']} | {row['retired']} | {row['ipc']:.3f} |")
    text += ['', '提交数包含 halt 或导致停止的精确错误事件。', '', '## 实际触发的行为', '',
             '以下计数只汇总每个场景的一份基准轨迹，未将八种配置重复累加。', '',
             '| 轨迹证据 | 次数／周期数 |', '| --- | ---: |']
    for name, count in acceptance['coverage'].items():
        text.append(f'| {name} | {count} |')
    text += ['', '## 宿主计时', '',
             f"机器 {timing['machine']}，固定 CPU {timing['cpu']}，一次预热、七次串行轮换采样。"
             '只计 tick 循环（含结束检查和首次 Signal 初始化），排除构造、输入、快照和 JSON。', '',
             '下表选取直接生成、Module 正序的中位数；完整 24 组及全部样本见 [timing.json](timing.json)。', '',
             '| 程序 | 缓存 | ns / cycle |', '| --- | --- | ---: |']
    for row in timing['results']:
        if row['binary'] == 'compiled' and not row['reverse']:
            text.append(f"| {row['program']} | {'开' if row['cache'] else '关'} | {row['median_ns_per_cycle']:.1f} |")
    text += ['', '短程序的每周期开销包含首次初始化，不能将这组数值视为硬件周期时间。', '',
             '## 代码量与限制', '', '| 项目 | 行／字节 |', '| --- | ---: |']
    for name, count in counts.items():
        text.append(f'| {name} | {count} |')
    text += ['', '行数包含注释和空行。硬件行数不重复计算复用的 Ripes5 译码文件；'
             '生成行数不包括 GFSim 和 ac_support.hpp。', '',
             '编译器和 GFSim 保持原样。F1 构造常量在条件区域错误复用的缺陷仍然存在，'
             '本模型通过循环前显式转换表达相同组合逻辑，最小复现及影响见 [findings.md](findings.md)。', '',
             '没有未完成的计划内功能；不包含 trap/CSR/中断、缓存、访存推测和 Store 转发。', '',
             '复现命令见 [README.md](README.md)。测试日志及首个差异上下文保存在 `output/`，'
             '该目录不纳入版本管理。']
    (HERE / 'report.md').write_text('\n'.join(text) + '\n')


if __name__ == '__main__':
    main()
