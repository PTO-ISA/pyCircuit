"""Render full and zoomed swimlanes from an executed long Shared-KV CSV trace."""

from __future__ import annotations

import ctypes
import html
import sys
from collections import Counter
from pathlib import Path

from render_trace import at, read

WIDTH = 2600
HEIGHT = 930
LEFT = 205
RIGHT = 75
LANES = (
    ("Index DMA", 233, "#4c78a8"),
    ("校验时延", 295, "#54a24b"),
    ("Q 载入", 357, "#6f9c46"),
    ("KV slot 0", 419, "#f58518"),
    ("KV slot 1", 481, "#e45756"),
    ("QK 阵列", 543, "#7a5195"),
    ("Softmax", 605, "#bc5090"),
    ("PV 阵列", 667, "#2a9d8f"),
    ("O 写回", 729, "#6c757d"),
)


def validate(
    single: list[dict[str, int | str]], double: list[dict[str, int | str]]
) -> int:
    batches = Counter(
        int(row["batch"]) for row in double if row["kind"] == "index_dma_start"
    )
    if set(batches) != set(range(len(batches))) or any(
        value != 1 for value in batches.values()
    ):
        raise ValueError("Index batches are incomplete or out of order")
    batch_count = len(batches)
    if batch_count < 2:
        raise ValueError("long trace needs at least two batches")
    count = batch_count * 64
    for name, rows in (("one", single), ("two", double)):
        events = Counter(str(row["kind"]) for row in rows)
        if events["kv_row_request"] != count or events["kv_row_complete"] != count:
            raise ValueError(f"{name}-slot KV traffic differs from {count} rows")
        if events["batch_compute_end"] != batch_count:
            raise ValueError(f"{name}-slot batch count mismatch")
        if any(
            sum(
                row["kind"] == "kv_row_request" and row["batch"] == batch
                for row in rows
            )
            != 64
            for batch in range(batch_count)
        ):
            raise ValueError(f"{name}-slot batch missing KV rows")
    q_ready = at(double, "q_load_end")
    if not (at(double, "index_valid_barrier") == at(double, "q_load_start") < q_ready):
        raise ValueError("Index/Q ordering mismatch")
    for batch in range(batch_count):
        slot = batch % 2
        fill_start = at(double, "kv_fill_start", batch, slot)
        fill_end = at(double, "kv_fill_end", batch, slot)
        compute_start = at(double, "batch_compute_start", batch, slot)
        compute_end = at(double, "batch_compute_end", batch, slot)
        if not (q_ready <= fill_start < fill_end <= compute_start < compute_end):
            raise ValueError(f"batch {batch} fill/compute order mismatch")
        if batch > 0:
            previous_start = at(double, "batch_compute_start", batch - 1)
            previous_end = at(double, "batch_compute_end", batch - 1)
            if not (previous_start <= fill_start < fill_end < previous_end):
                raise ValueError(f"batch {batch} did not overlap prior compute")
        for wave in range(4):
            if not (
                at(double, "qk_start", batch, slot, wave)
                < at(double, "qk_end", batch, slot, wave)
                == at(double, "softmax_start", batch, slot, wave)
                < at(double, "softmax_end", batch, slot, wave)
                == at(double, "pv_start", batch, slot, wave)
                < at(double, "pv_end", batch, slot, wave)
            ):
                raise ValueError(f"batch {batch} wave {wave} stage order mismatch")
            for first, last, expected in (
                ("qk_start", "qk_end", 44),
                ("softmax_start", "softmax_end", 16),
                ("pv_start", "pv_end", 44),
                ("state_commit_start", "state_commit_end", 1),
            ):
                if (
                    at(double, last, batch, slot, wave)
                    - at(double, first, batch, slot, wave)
                    != expected
                ):
                    raise ValueError(
                        f"batch {batch} wave {wave} {first} timing mismatch"
                    )
    if any(
        int(row["cycle"]) < q_ready for row in double if row["kind"] == "kv_row_request"
    ):
        raise ValueError("KV request precedes Q load")
    return batch_count


def rasterize(svg: Path, png: Path) -> None:
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
    surface = cairo.cairo_image_surface_create(0, WIDTH, HEIGHT)
    context = cairo.cairo_create(surface)
    handle = rsvg.rsvg_handle_new_from_file(str(svg).encode(), None)
    if not handle or not rsvg.rsvg_handle_render_cairo(handle, context):
        raise RuntimeError("librsvg could not render long trace")
    if cairo.cairo_surface_write_to_png(surface, str(png).encode()):
        raise RuntimeError("cairo could not write long trace PNG")
    gobject.g_object_unref(handle)
    cairo.cairo_destroy(context)
    cairo.cairo_surface_destroy(surface)


def draw(
    rows: list[dict[str, int | str]],
    batch_count: int,
    one_cycles: int,
    two_cycles: int,
    prefix: Path,
    name: str,
    start: int,
    end: int,
) -> tuple[Path, Path]:
    count = batch_count * 64
    scale = (WIDTH - LEFT - RIGHT) / (end - start)

    def x(cycle: int) -> float:
        return LEFT + (cycle - start) * scale

    overview = name == "overview"
    title = "全程" if overview else "启动与前 5 批计算放大"
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" '
        f'viewBox="0 0 {WIDTH} {HEIGHT}" role="img" '
        f'aria-label="SparseFA Count={count} C++ 周期模型{title}泳道图">',
        '<rect width="100%" height="100%" fill="#fbfcfe"/>',
        f'<text x="48" y="48" font-size="30" font-family="sans-serif" '
        f'font-weight="700" fill="#182739">SparseFA Shared-KV · {count} Index · {title}泳道图</text>',
        f'<text x="49" y="80" font-size="17" font-family="sans-serif" fill="#536277">'
        f"H=64，D=64，{batch_count} 批 × 64 Index，8 个 KV 行 credit，32 B/cycle 总线</text>",
        f'<text x="49" y="109" font-size="18" font-family="sans-serif" fill="#7b3f00">'
        f"双 slot {two_cycles:,} 周期；单 slot {one_cycles:,} 周期；模型节省 "
        f"{one_cycles - two_cycles:,} 周期（{(one_cycles - two_cycles) / one_cycles:.1%}）</text>",
        '<text x="49" y="138" font-size="15" font-family="sans-serif" fill="#a03030">'
        "来自实际执行的独立 C++ 时序模型；Shared-KV 硬件尚未实现，非 RTL/C++ DUT 实测。</text>",
        f'<text x="{LEFT}" y="185" font-size="14" font-family="sans-serif" '
        f'fill="#536277">周期范围 {start:,}–{end:,}；横轴为模型周期</text>',
    ]

    tick = 2000 if overview else 250
    first_tick = ((start + tick - 1) // tick) * tick
    for cycle in range(first_tick, end + 1, tick):
        parts.append(
            f'<line x1="{x(cycle):.1f}" y1="198" x2="{x(cycle):.1f}" y2="765" '
            'stroke="#e1e6ef" stroke-width="1"/>'
        )
        parts.append(
            f'<text x="{x(cycle):.1f}" y="210" text-anchor="middle" '
            f'font-family="sans-serif" font-size="13" fill="#66758a">{cycle:,}</text>'
        )
    for label, y, color in LANES:
        parts.append(
            f'<text x="185" y="{y + 7}" text-anchor="end" font-family="sans-serif" '
            f'font-size="18" fill="#26374a">{html.escape(label)}</text>'
        )
        parts.append(
            f'<line x1="{LEFT}" y1="{y + 19}" x2="{WIDTH - RIGHT}" y2="{y + 19}" '
            f'stroke="{color}" stroke-opacity=".22"/>'
        )

    def bar(
        begin: int, finish: int, lane: int, label: str, color: str | None = None
    ) -> None:
        if finish <= begin:
            raise ValueError(f"nonpositive event span: {label}")
        if finish <= start or begin >= end:
            return
        _, y, base = LANES[lane]
        clipped_begin, clipped_end = max(begin, start), min(finish, end)
        bx, bw = x(clipped_begin), x(clipped_end) - x(clipped_begin)
        parts.append(
            f'<rect x="{bx:.1f}" y="{y - 15}" width="{max(1.0, bw):.1f}" '
            f'height="30" rx="4" fill="{color or base}">'
            f"<title>{html.escape(label)}: cycle {begin:,}–{finish:,}</title></rect>"
        )
        if bw >= 33:
            text = label if bw >= 64 else label.split(" ")[0]
            parts.append(
                f'<text x="{bx + bw / 2:.1f}" y="{y + 5}" text-anchor="middle" '
                'font-family="sans-serif" font-size="12" font-weight="600" '
                f'fill="white">{html.escape(text)}</text>'
            )

    bar(at(rows, "count_read_start"), at(rows, "count_read_end"), 0, "Count")
    for batch in range(batch_count):
        bar(
            at(rows, "index_dma_start", batch),
            at(rows, "index_dma_end", batch),
            0,
            f"B{batch} DMA",
        )
        bar(
            at(rows, "index_validate_start", batch),
            at(rows, "index_validate_end", batch),
            1,
            f"B{batch} 时延",
        )
    bar(at(rows, "q_load_start"), at(rows, "q_load_end"), 2, "8192 B Q 载入")
    for batch in range(batch_count):
        slot = batch % 2
        fill_start = at(rows, "kv_fill_start", batch, slot)
        fill_end = at(rows, "kv_fill_end", batch, slot)
        compute_start = at(rows, "batch_compute_start", batch, slot)
        compute_end = at(rows, "batch_compute_end", batch, slot)
        bar(fill_start, fill_end, 3 + slot, f"B{batch} gather")
        if fill_end < compute_start:
            bar(fill_end, compute_start, 3 + slot, f"B{batch} READY", "#aeb9c8")
        bar(
            compute_start,
            compute_end,
            3 + slot,
            f"B{batch} compute",
            "#c47719" if slot == 0 else "#b94848",
        )
        for wave in range(4):
            for first, last, lane in (
                ("qk_start", "qk_end", 5),
                ("softmax_start", "softmax_end", 6),
                ("pv_start", "pv_end", 7),
            ):
                bar(
                    at(rows, first, batch, slot, wave),
                    at(rows, last, batch, slot, wave),
                    lane,
                    f"B{batch} W{wave}",
                )
    bar(
        at(rows, "output_write_start"),
        at(rows, "output_write_ack"),
        8,
        "8192 B O 写 + ack",
    )

    for key, label in (
        (at(rows, "index_valid_barrier"), "Index 屏障"),
        (at(rows, "q_load_end"), "Q READY"),
        (two_cycles, "完成"),
    ):
        if start <= key <= end:
            parts.append(
                f'<line x1="{x(key):.1f}" y1="220" x2="{x(key):.1f}" y2="762" '
                'stroke="#31495f" stroke-width="1.4" stroke-dasharray="5 5"/>'
            )
            if not overview or key == two_cycles:
                label_x = x(key) - 5 if key == end else x(key) + 5
                anchor = "end" if key == end else "start"
                parts.append(
                    f'<text x="{label_x:.1f}" y="790" text-anchor="{anchor}" '
                    f'font-family="sans-serif" font-size="13" fill="#31495f">'
                    f"{key:,}: {label}</text>"
                )
    parts.extend(
        [
            '<text x="49" y="845" font-family="sans-serif" font-size="15" '
            f'fill="#536277">CSV 记录 {count:,} 次 KV 行请求/完成；橙/红槽交替供给，每批 4 个 16-head wave。</text>',
            '<text x="49" y="874" font-family="sans-serif" font-size="15" '
            'fill="#536277">模型先完成全部 Index DMA/校验时延预算，再加载 Q 并执行 KV gather；'
            "Index 内容假定有效，模型未检查实际 ID。</text>",
            '<text x="49" y="903" font-family="sans-serif" font-size="15" '
            'fill="#536277">Count 4 B；Q/O 各 8192 B；KV 行延迟 16 周期；'
            "QK/PV 每 tile 11 周期，softmax 每 wave 16 周期。非硬件实测。</text>",
            "</svg>",
        ]
    )
    svg = Path(f"{prefix}-{name}.svg")
    png = Path(f"{prefix}-{name}.png")
    svg.write_text("\n".join(parts) + "\n", encoding="utf-8")
    rasterize(svg, png)
    return svg, png


def render(prefix: Path) -> list[Path]:
    single = read(Path(f"{prefix}-1slot.csv"))
    double = read(Path(f"{prefix}-2slot.csv"))
    batch_count = validate(single, double)
    one_cycles, two_cycles = at(single, "completion"), at(double, "completion")
    q_ready = at(double, "q_load_end")
    detail_end = at(double, "batch_compute_end", min(4, batch_count - 1))
    result: list[Path] = []
    for name, start, end in (
        ("overview", 0, two_cycles),
        ("detail", max(0, q_ready - 350), detail_end),
    ):
        result.extend(
            draw(double, batch_count, one_cycles, two_cycles, prefix, name, start, end)
        )
    return result


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: render_long_trace.py OUTPUT_PREFIX")
    for path in render(Path(sys.argv[1])):
        print(path)
