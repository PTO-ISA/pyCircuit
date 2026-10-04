"""Record verified evidence, physical source counts and the measured timing table."""
import argparse
import json
import re
from pathlib import Path
import xml.etree.ElementTree as ET
from verify import HERE, ROOT, sha, fingerprint


def lines(paths):
    files = {}
    for path in paths:
        rows = path.read_text().splitlines()
        label = str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else path.name
        files[label] = dict(lines=len(rows), nonblank=sum(bool(x.strip()) for x in rows), sha256=sha(path))
    return dict(lines=sum(f['lines'] for f in files.values()),
                nonblank=sum(f['nonblank'] for f in files.values()), files=files)


def tests(path):
    root = ET.parse(path).getroot()
    if root.get('failures') != '0' or root.get('skipped') != '0':
        raise AssertionError(f'incomplete regression: {path}')
    return dict(root.attrib, cases=[dict(t.attrib) for t in root.findall('testcase')], xml_sha256=sha(path))


def benchmark_report(path):
    timing = json.loads(path.read_text())
    if timing['schema'] != 2 or not timing['acceptance']['full_acceptance'] or timing['acceptance']['configurations'] != 52:
        raise AssertionError('requires fixed-window timing and full acceptance')
    if not timing['acceptance']['fixed_runner']['passed']:
        raise AssertionError('fixed-window checks failed')
    names = dict(generated='ACPy 生成版', handwritten='手写 GFSim', native='原生 Ripes5')
    parts = ['# Ripes5 三方同批性能对比', '',
             f'采样时间：{timing["timestamp_utc"]}。CPU affinity：{timing["affinity"]["selected"]}。', '',
             '13 程序 × 四配置逐拍验收通过；五个长程序逐拍比较生成版、手写版和原生版，且原生通知开／关轨迹（包括 raw 诊断）完全一致。各长程序另核对独立计算的完整寄存器和内存结果。每次预热与正式采样均核对总周期、退休增量和最终状态。未通过门槛的程序不进入汇总。', '',
             f'统一 GCC {timing["build_configuration"]["compiler_version"].splitlines()[0]}，C++20，'
             f'`{timing["build_configuration"]["flags"]}`，关闭 LTO。GFSim 使用缓存开、Module 正序。'
             '原生版通过 `setEnableSignals(false)`、`setEnableClockedSignals(false)` 关闭观察通知，反向历史为 0；上游模型源码未修改。', '',
             f'每个进程从相同输入重新构造，先执行 K 拍；仅计时随后 N=T−K 次 `step()`／`clockUnguarded()`，截至结束 marker 提交。'
             f'每版完整预热一次，三方轮换顺序串行采样 {timing["repeats"]} 次。构造、装载、K 拍预热、宿主地址检查、结束判断和快照不计入核心耗时。模型内部必要检查和统计保留。', '',
             '下表为中位数 [Q1, Q3]，四分位数使用 inclusive 线性插值。提交指令/秒按计时窗口退休增量计算。进程耗时包含启动、输入、构造、预热、输出和退出，独立于核心耗时。', '',
             '| 程序 | K / N | 版本 | ns/周期 | 提交 M指令/秒 | 核心 ms | 进程 ms |',
             '| --- | ---: | --- | ---: | ---: | ---: | ---: |']

    def cell(values, scale=1):
        return f'{values["median"]/scale:.2f} [{values["q1"]/scale:.2f}, {values["q3"]/scale:.2f}]'

    ratios = ['| 程序 | 原生/生成耗时 | 原生/手写耗时 | 生成/手写耗时 |', '| --- | ---: | ---: | ---: |']
    for program in timing['programs']:
        if not program['verification']['passed'] or not program['verification']['observation_toggle_identical']:
            raise AssertionError('unverified program')
        for measurement in program['measurements']:
            if len(measurement['samples']) != timing['repeats']:
                raise AssertionError('incomplete timing samples')
            s = measurement['summary']
            parts.append(f'| {program["name"]} | {program["warmup_cycles"]} / {program["measured_cycles"]} | '
                         f'{names[measurement["model"]]} | {cell(s["ns_per_cycle"])} | '
                         f'{cell(s["instructions_per_second"], 1e6)} | {cell(s["run_ns"], 1e6)} | {cell(s["process_ns"], 1e6)} |')
        r = program['ratios_of_medians']
        ratios.append(f'| {program["name"]} | {r["native_over_generated"]:.3f} | '
                      f'{r["native_over_handwritten"]:.3f} | {r["generated_over_handwritten"]:.3f} |')
    parts += ['', '同批中位耗时之比（分子/分母；原生/生成大于 1 表示生成版更快）：', '', *ratios, '',
              '结果描述各自完整模型的执行效率，不能用于归因调度器各部分成本；该归因需后续剖析。工作负载限定于有界 RV32I 子集循环，不能外推为所有电路的通用速度比。', '',
              f'原始样本、输入、顺序、预热、验证摘要和源码／二进制散列见 [{path.name}]({path.name})。'
              '有效逐文件编译及链接命令见 `output/fair-build/build-manifest.json`；长程序验证只存流式摘要及首个差异附近记录。', '',
              '本测量取代跨批次估算作为当前三方对比依据。历史 [timing.json](timing.json) 保留旧短程序、七次采样及含结束检查／首次 Signal 初始化的口径；不与本批数据计算速度比。', '',
              '统一构建、验收与测速命令见 [README](README.md)。']
    (HERE / 'benchmark-report.md').write_text('\n'.join(parts) + '\n')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build', type=Path)
    p.add_argument('--asan-build', type=Path)
    p.add_argument('--benchmark-only', action='store_true')
    p.add_argument('--timing', type=Path, default=HERE / 'timing-fixed.json')
    a = p.parse_args()
    if a.benchmark_only:
        benchmark_report(a.timing)
        return
    if a.build is None or a.asan_build is None:
        p.error('--build and --asan-build are required for the compiler regression report')
    comparison = json.loads((HERE / 'output/Release/comparison.json').read_text())
    if not comparison['full_acceptance'] or comparison['configurations'] != 52:
        raise AssertionError('requires full native acceptance')
    timing = json.loads((HERE / 'timing.json').read_text())
    package = ROOT / 'pycircuit'
    counts = dict(compiler=lines(sorted(package.glob('*.py'))),
                  value_support=lines([package / 'support.hpp']),
                  acpy_ripes5=lines([HERE / 'logic.py', HERE / 'model.py']),
                  generated_model=lines([a.build / 'compiled/model.hpp', a.build / 'compiled/model.cpp']),
                  tests=lines(sorted((package / 'tests').glob('*.py'))),
                  pipeline_example=lines([package / 'examples/pipeline/model.py']),
                  evidence_tools=lines([HERE / name for name in ('verify.py', 'bench.py', 'report.py')]),
                  shared_host_runner=lines([ROOT / 'gfsim/cpp/examples/ripes5/runner.cpp']))
    equality = {name: sha(a.build / 'compiled' / name) == sha(a.build / 'emitted' / name)
                for name in ('model.hpp', 'model.cpp', 'ac_support.hpp')}
    if not all(equality.values()):
        raise AssertionError('ACIR emit changed generated code')
    python_log = a.build / 'python-regression.log'
    python_text = python_log.read_text()
    match = re.search(r'Ran (\d+) tests in ([0-9.]+)s\n\nOK\s*$', python_text)
    if not match:
        raise AssertionError('Python GFSim regressions did not pass')
    result = dict(comparison, counts=counts, generated_and_reloaded_identical=equality,
                  tests=dict(release=tests(a.build / 'ctest.xml'), asan_ubsan_lsan=tests(a.asan_build / 'ctest.xml'),
                             python_gfsim=dict(tests=int(match[1]), seconds=float(match[2]), log_sha256=sha(python_log))),
                  final_source_sha256=fingerprint(), timing_sha256=sha(HERE / 'timing.json'))
    (HERE / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
    table = ['| 程序 | 周期 | 生成版 ns/周期 | 手写版 ns/周期 | 生成/手写 |', '| --- | ---: | ---: | ---: | ---: |']
    for program in timing['programs']:
        values = {m['model']: m['median_ns_per_cycle'] for m in program['measurements']}
        generated, manual = values['generated-cache1-reverse0'], values['handwritten-cache1-reverse0']
        table.append(f'| {program["name"]} | {program["cycles"]} | {generated:.1f} | {manual:.1f} | {generated/manual:.3f} |')
    parts = ['# ACPy 编译链验收报告', '',
             '当前三方性能结果见 [同批固定周期报告](benchmark-report.md)。下面的七次短程序计时保留历史口径。', '',
             '生成版、仅 ACIR 重载生成版、手写 C++、当前 Python、固定原生 Ripes：13 程序 × 四配置，共 52 配置逐拍通过。使用同一输入和周期边界，不移动轨迹。原生版本验证为强制门槛。', '',
             'Release 与 Clang ASan/UBSan/LeakSanitizer 均通过全部 8 项 CTest，其中包含 12 项编译器测试及原有 GFSim 回归。现有 Python GFSim 的 28 项测试也在强制原生参考模式下全部通过。LeakSanitizer 在受 ptrace 限制的沙箱内无法运行，正式内存验收在获准的沙箱外执行。', '',
             '编译器测试覆盖完整消息序列、分支消费、别名去重、必要读失败原子清理、背压与候选保留、共享 RuleId／输出、资源身份和普通参数缓存、整值／嵌套字段 revise、动态资源阵列、Signal 过滤与共享读取、事件、短路／提前返回、定宽运算和固定数组。临时源码删除后，保存的 ACIR 可独立生成并运行。Ripes5 两种入口生成的三个 C++ 文件逐字相同。', '',
             '## 代码量', '', '| 类别 | 物理行数 | 非空行数 |', '| --- | ---: | ---: |']
    labels = dict(compiler='独立 Python 编译器', value_support='通用 C++ 值／存储支持', acpy_ripes5='ACPy Ripes5',
                  generated_model='生成模型头文件及实现', tests='编译器测试及补充电路', pipeline_example='ACPy 消息流水示例',
                  evidence_tools='验收／计时／报告工具', shared_host_runner='复用的宿主 runner')
    for name, count in counts.items():
        parts.append(f'| {labels[name]} | {count["lines"]} | {count["nonblank"]} |')
    parts += ['', '计数包含注释，生成模型不重复计入通用支持头。既有 GFSim runtime 与 Python／原生参考没有计入编译器。逐文件计数及散列保存在 [results.json](results.json)。', '',
              '## 历史短程序循环耗时', '',
              '下表为缓存开、Module 正序配置的七次采样中位数。两模型各四配置的全部样本、预热、轮换次序、CPU affinity 和二进制指纹保存在 [timing.json](timing.json)。数字描述完整模型，不代表单独调度器成本。', '', *table, '',
              '计时包含逐拍循环、结束 marker 检查、runtime 计数与首次 Signal 初始化；不含构造、轨迹观察及 JSON。无性能专用编译路径。', '',
              '## 重现', '', '构建、两阶段编译、验收与计时命令见 [编译器 README](../../README.md)。原始证据位于本目录 `output/Release`、`output/Debug`。随后执行：', '',
              '```bash', 'RIPES5_REQUIRE_NATIVE=1 python3 -m unittest discover -s gfsim/experiment -v > /tmp/acpy-build/python-regression.log 2>&1',
              'python3 pycircuit/examples/ripes5/report.py --build /tmp/acpy-build --asan-build /tmp/acpy-asan', '```', '',
              '语言仍限于 README 列出的首版子集；运行时资源声明、运行时循环、跨 Signal 依赖和动态 payload 字段 revise 尚不支持。']
    (HERE / 'report.md').write_text('\n'.join(parts) + '\n')


if __name__ == '__main__':
    main()
