"""Regenerate presentation from existing, functionally verified local traces."""

import hashlib
import json
import sys
from pathlib import Path

from circuit_flow_viewer.cli import render

root = Path.cwd()
out = root / ".pycircuit_out/davincioo-rob/20260908-table-tree"
sources = {
    **{
        f"single-{name}": f".pycircuit_out/davincioo-rob/20260908-single/{name}/execution.pyctrace"
        for name in ("capacity", "backpressure", "invalid", "recovery")
    },
    "dual-recovery": ".pycircuit_out/davincioo-rob/20260908-source-names/recovery/execution.pyctrace",
    "legacy": ".pycircuit_out/replay/main-migration/latest-verified/single-ss2nqoym/execution.pyctrace",
}
artifacts = {}
for name, source in sources.items():
    trace = root / source
    page = render(trace, out / name / "replay.html")
    artifacts[name] = {
        "trace": source,
        "trace_sha256": hashlib.sha256(trace.read_bytes()).hexdigest(),
        "html": str(page.relative_to(root)),
        "html_sha256": hashlib.sha256(page.read_bytes()).hexdigest(),
    }
    sys.stdout.write(f"{name} {page}\n")
render(
    root / "third_party/circuit-flow-viewer/tests/fixtures/latency.pyctrace",
    out / "legacy/queue-latency.html",
)
links = "\n".join(
    f'<li><a href="{name}/replay.html">{name}</a></li>'
    for name in sources
    if name != "legacy"
)
(out / "index.html").write_text(
    '<!doctype html><html lang="zh"><meta charset="utf-8"><title>ROB Table tree replay</title>'
    "<style>body{font:16px system-ui;max-width:900px;margin:50px auto;line-height:1.8}a{color:#126694}</style>"
    "<h1>ROB 树状 Table 回放</h1><p>Fields 选择字段，Rows 选择槽位；点击表头箭头展开结构。"
    "展开和筛选在播放与跳转时保留，刷新页面恢复默认。</p><ul>" + links + "</ul>"
    "<p>使用已有 trace 重新生成；原始 IR、模型和驱动仍在各自原始生成目录。</p></html>"
)
(root / "docs/gates/logs/20260908-table-tree/artifacts.json").write_text(
    json.dumps(artifacts, indent=2) + "\n"
)
