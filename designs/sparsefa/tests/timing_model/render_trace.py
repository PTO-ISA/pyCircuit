"""Render the executed Shared-KV timing-model CSV as a cycle-axis SVG/PNG."""

from __future__ import annotations

import csv
import ctypes
import html
import sys
from collections import Counter
from pathlib import Path


def read(path: Path) -> list[dict[str, int | str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    if not rows:
        raise ValueError(f"empty trace: {path}")
    result: list[dict[str, int | str]] = []
    for row in rows:
        result.append(
            {
                "cycle": int(row["cycle"]),
                "kind": row["kind"],
                "batch": int(row["batch"]),
                "slot": int(row["slot"]),
                "wave": int(row["wave"]),
                "position": int(row["position"]),
            }
        )
    if any(a["cycle"] > b["cycle"] for a, b in zip(result, result[1:], strict=False)):
        raise ValueError("trace cycles are not monotonic")
    return result


def at(
    rows: list[dict[str, int | str]],
    kind: str,
    batch: int = -1,
    slot: int = -1,
    wave: int = -1,
) -> int:
    hits = [
        int(row["cycle"])
        for row in rows
        if row["kind"] == kind
        and (batch < 0 or row["batch"] == batch)
        and (slot < 0 or row["slot"] == slot)
        and (wave < 0 or row["wave"] == wave)
    ]
    if len(hits) != 1:
        raise ValueError(f"expected one {kind} event for {batch=}, {slot=}, {wave=}")
    return hits[0]


def render(prefix: Path) -> tuple[Path, Path]:
    single = read(Path(str(prefix) + "-1slot.csv"))
    double = read(Path(str(prefix) + "-2slot.csv"))
    one_cycles = at(single, "completion")
    two_cycles = at(double, "completion")
    barrier = at(double, "index_valid_barrier")
    q_start = at(double, "q_load_start")
    q_ready = at(double, "q_load_end")
    counts = Counter(str(row["kind"]) for row in double)
    if counts["kv_row_request"] != 128 or counts["kv_row_complete"] != 128:
        raise ValueError("incomplete KV row traffic")
    if any(
        int(row["cycle"]) < q_ready for row in double if row["kind"] == "kv_row_request"
    ):
        raise ValueError("KV request preceded Q load")
    if not (barrier <= q_start < q_ready):
        raise ValueError("Q load did not follow the Index timing barrier")
    if not (
        at(double, "kv_fill_end", batch=1) < at(double, "batch_compute_end", batch=0)
    ):
        raise ValueError("second slot did not overlap first batch computation")

    width, height = 1820, 940
    left, right = 210, 75
    scale = (width - left - right) / one_cycles

    def x(cycle: int) -> float:
        return left + cycle * scale

    lanes = [
        ("Index DMA", 185, "#4c78a8"),
        ("校验时延", 248, "#54a24b"),
        ("Q 载入", 311, "#6f9c46"),
        ("KV slot 0", 374, "#f58518"),
        ("KV slot 1", 437, "#e45756"),
        ("QK 阵列", 500, "#7a5195"),
        ("Softmax", 563, "#bc5090"),
        ("PV 阵列", 626, "#2a9d8f"),
        ("状态提交", 689, "#9b9b1f"),
        ("O 写回", 752, "#6c757d"),
    ]
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" role="img" '
        'aria-label="SparseFA Shared-KV C++ 周期性能模型实测泳道图">',
        '<rect width="100%" height="100%" fill="#fbfcfe"/>',
        '<text x="48" y="48" font-size="29" font-family="sans-serif" '
        'font-weight="700" fill="#182739">SparseFA Shared-KV · C++ 周期模型执行泳道图</text>',
        '<text x="49" y="77" font-size="16" font-family="sans-serif" fill="#536277">'
        "H=64, D=64, Count=128 · 每批 64 Index · 8 个 KV 行 credit · 32 B/cycle 总线</text>",
        f'<text x="49" y="106" font-size="16" font-family="sans-serif" fill="#7b3f00">'
        f"双 slot {two_cycles} 周期；单 slot {one_cycles} 周期；模型节省 "
        f"{one_cycles - two_cycles} 周期（{(one_cycles - two_cycles) / one_cycles:.1%}）</text>",
        '<text x="49" y="131" font-size="14" font-family="sans-serif" fill="#a03030">'
        "这是实际执行的独立 C++ 时序模型输出；共享 KV 硬件尚未实现，非 RTL/C++ DUT 实测。</text>",
    ]
    for tick in range(0, one_cycles + 1, 200):
        parts.append(
            f'<line x1="{x(tick):.1f}" y1="160" x2="{x(tick):.1f}" y2="793" '
            'stroke="#e1e6ef" stroke-width="1"/>'
        )
        parts.append(
            f'<text x="{x(tick):.1f}" y="160" text-anchor="middle" '
            'font-family="sans-serif" font-size="13" fill="#66758a">'
            f"{tick}</text>"
        )
    for name, y, color in lanes:
        parts.extend(
            [
                f'<text x="185" y="{y + 7}" text-anchor="end" '
                'font-family="sans-serif" font-size="18" fill="#26374a">'
                f"{html.escape(name)}</text>",
                f'<line x1="{left}" y1="{y + 20}" x2="{width - right}" '
                f'y2="{y + 20}" stroke="{color}" stroke-opacity=".19"/>',
            ]
        )

    def bar(
        start: int,
        end: int,
        lane: int,
        label: str,
        color: str | None = None,
        opacity: float = 1.0,
    ) -> None:
        if end <= start:
            raise ValueError("nonpositive measured span")
        _, y, base = lanes[lane]
        fill = color or base
        bx = x(start)
        bw = max(1.0, x(end) - bx)
        parts.append(
            f'<rect x="{bx:.1f}" y="{y - 16}" width="{bw:.1f}" height="32" '
            f'rx="5" fill="{fill}" opacity="{opacity}"/>'
        )
        if bw >= 42:
            parts.append(
                f'<text x="{bx + bw / 2:.1f}" y="{y + 5}" text-anchor="middle" '
                'font-family="sans-serif" font-size="13" font-weight="600" '
                f'fill="white">{html.escape(label)}</text>'
            )
        parts.append(f"<title>{html.escape(label)}: cycle {start}–{end}</title>")

    bar(at(double, "count_read_start"), at(double, "count_read_end"), 0, "Count")
    for batch in range(2):
        bar(
            at(double, "index_dma_start", batch),
            at(double, "index_dma_end", batch),
            0,
            f"B{batch} DMA",
        )
        bar(
            at(double, "index_validate_start", batch),
            at(double, "index_validate_end", batch),
            1,
            f"B{batch} 时延",
        )
    bar(at(double, "q_load_start"), q_ready, 2, "8192 B Q 载入")
    for batch, slot in ((0, 0), (1, 1)):
        fill_start = at(double, "kv_fill_start", batch, slot)
        fill_end = at(double, "kv_fill_end", batch, slot)
        compute_start = at(double, "batch_compute_start", batch, slot)
        compute_end = at(double, "batch_compute_end", batch, slot)
        bar(fill_start, fill_end, 3 + slot, f"B{batch} gather 64 行")
        if fill_end < compute_start:
            bar(fill_end, compute_start, 3 + slot, "READY 等待", "#b5bdc9")
        bar(
            compute_start,
            compute_end,
            3 + slot,
            f"B{batch} 供 4 个 head wave",
            "#c97718" if slot == 0 else "#b93d40",
            0.62,
        )
        for wave in range(4):
            for start_kind, end_kind, lane, label in (
                ("qk_start", "qk_end", 5, "QK"),
                ("softmax_start", "softmax_end", 6, "SM"),
                ("pv_start", "pv_end", 7, "PV"),
                ("state_commit_start", "state_commit_end", 8, "commit"),
            ):
                bar(
                    at(double, start_kind, batch, slot, wave),
                    at(double, end_kind, batch, slot, wave),
                    lane,
                    f"B{batch} W{wave} {label}",
                )
    bar(
        at(double, "output_write_start"),
        at(double, "output_write_ack"),
        9,
        "8192 B O 写 + ack",
    )

    for key, label in (
        (barrier, "Index 假定有效"),
        (q_ready, "Q 载入完成"),
        (at(double, "kv_fill_end", 0), "slot 0 READY"),
        (at(double, "kv_fill_end", 1), "slot 1 READY"),
        (two_cycles, "完成"),
    ):
        parts.append(
            f'<line x1="{x(key):.1f}" y1="169" x2="{x(key):.1f}" y2="798" '
            'stroke="#34455c" stroke-width="1.3" stroke-dasharray="4 5" '
            'opacity=".6"/>'
        )
        parts.append(
            f'<text x="{x(key) + 4:.1f}" y="818" font-family="sans-serif" '
            f'font-size="12" fill="#34455c">{key}: {html.escape(label)}</text>'
        )
    parts.extend(
        [
            '<text x="49" y="870" font-family="sans-serif" font-size="14" '
            'fill="#536277">每条彩色区间来自 CSV 事件；底层 trace 含 128 次 KV 请求与返回、'
            "四个 head wave/批。</text>",
            '<text x="49" y="895" font-family="sans-serif" font-size="14" '
            'fill="#536277">Count 4 B 读 13 周期，Q 8192 B 读 272 周期；'
            "Index 内容假定有效、互异，模型未计算或校验实际 ID。</text>",
            '<text x="49" y="920" font-family="sans-serif" font-size="14" '
            'fill="#536277">模型参数：Index 启动 12 周期，KV 行延迟 16 周期，'
            "阵列 5 周期排空，softmax 每 wave 16 周期。</text>",
            "</svg>",
        ]
    )
    svg = Path(str(prefix) + "-2slot.svg")
    png = Path(str(prefix) + "-2slot.png")
    svg.write_text("\n".join(parts) + "\n", encoding="utf-8")

    cairo = ctypes.CDLL("libcairo.so.2")
    rsvg = ctypes.CDLL("librsvg-2.so.2")
    gobject = ctypes.CDLL("libgobject-2.0.so.0")
    cairo.cairo_image_surface_create.argtypes = [
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_int,
    ]
    cairo.cairo_image_surface_create.restype = ctypes.c_void_p
    cairo.cairo_create.argtypes = [ctypes.c_void_p]
    cairo.cairo_create.restype = ctypes.c_void_p
    cairo.cairo_surface_write_to_png.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    cairo.cairo_surface_write_to_png.restype = ctypes.c_int
    cairo.cairo_destroy.argtypes = [ctypes.c_void_p]
    cairo.cairo_surface_destroy.argtypes = [ctypes.c_void_p]
    rsvg.rsvg_handle_new_from_file.argtypes = [ctypes.c_char_p, ctypes.c_void_p]
    rsvg.rsvg_handle_new_from_file.restype = ctypes.c_void_p
    rsvg.rsvg_handle_render_cairo.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    rsvg.rsvg_handle_render_cairo.restype = ctypes.c_int
    gobject.g_object_unref.argtypes = [ctypes.c_void_p]
    surface = cairo.cairo_image_surface_create(0, width, height)
    context = cairo.cairo_create(surface)
    handle = rsvg.rsvg_handle_new_from_file(str(svg).encode(), None)
    if not handle or not rsvg.rsvg_handle_render_cairo(handle, context):
        raise RuntimeError("librsvg could not render model trace")
    if cairo.cairo_surface_write_to_png(surface, str(png).encode()):
        raise RuntimeError("cairo could not write model PNG")
    gobject.g_object_unref(handle)
    cairo.cairo_destroy(context)
    cairo.cairo_surface_destroy(surface)
    return svg, png


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: render_trace.py OUTPUT_PREFIX")
    for path in render(Path(sys.argv[1])):
        print(path)
