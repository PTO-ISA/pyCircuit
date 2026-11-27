"""Validate and render executed Index-stream C++ timing-model traces."""

from __future__ import annotations

import csv
import ctypes
import html
import sys
from collections import Counter
from pathlib import Path

SOFTMAX_CYCLES = 4 * 16 + 33
LANES = (
    ("Count / Q 预载", "#64748b"),
    ("Index slot 0", "#2563eb"),
    ("Index slot 1", "#3b82f6"),
    ("KV slot 0", "#ea580c"),
    ("KV slot 1", "#ef4444"),
    ("QK 阵列", "#7c3aed"),
    ("Softmax", "#db2777"),
    ("PV 阵列", "#0d9488"),
    ("状态提交", "#a16207"),
    ("O 写回", "#475569"),
)


def read(path: Path) -> list[dict[str, int | str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    if not rows:
        raise ValueError(f"empty trace: {path}")
    result = [
        {key: value if key == "kind" else int(value) for key, value in row.items()}
        for row in rows
    ]
    if any(
        int(a["cycle"]) > int(b["cycle"])
        for a, b in zip(result, result[1:], strict=False)
    ):
        raise ValueError("trace cycles are not monotonic")
    return result


def at(
    rows: list[dict[str, int | str]],
    kind: str,
    batch: int = -1,
    slot: int = -1,
    wave: int = -1,
    position: int = -1,
) -> int:
    hits = [
        int(row["cycle"])
        for row in rows
        if row["kind"] == kind
        and (batch < 0 or row["batch"] == batch)
        and (slot < 0 or row["slot"] == slot)
        and (wave < 0 or row["wave"] == wave)
        and (position < 0 or row["position"] == position)
    ]
    if len(hits) != 1:
        raise ValueError(
            f"expected one {kind} for {batch=}, {slot=}, {wave=}, {position=}; got {len(hits)}"
        )
    return hits[0]


def slot_for(rows: list[dict[str, int | str]], kind: str, batch: int) -> int:
    hits = [
        int(row["slot"])
        for row in rows
        if row["kind"] == kind and row["batch"] == batch
    ]
    if len(hits) != 1:
        raise ValueError(f"expected one {kind} for batch {batch}")
    return hits[0]


def validate(one: list[dict[str, int | str]], two: list[dict[str, int | str]]) -> int:
    batches = sum(row["kind"] == "index_batch_valid" for row in two)
    if not 1 <= batches <= 32:
        raise ValueError("invalid batch count")
    for name, rows, kv_slots in (("one", one, 1), ("two", two, 2)):
        counts = Counter(str(row["kind"]) for row in rows)
        for kind in (
            "index_slot_acquire",
            "index_dma_start",
            "index_dma_end",
            "index_validate_start",
            "index_validate_end",
            "index_batch_valid",
            "index_dispatch_start",
            "index_slot_release",
            "kv_fill_start",
            "kv_fill_end",
            "batch_compute_start",
            "batch_compute_end",
        ):
            if counts[kind] != batches:
                raise ValueError(f"{name}: {kind} count {counts[kind]} != {batches}")
        if (
            counts["kv_row_request"] != batches * 64
            or counts["kv_row_complete"] != batches * 64
        ):
            raise ValueError(f"{name}: incomplete KV row traffic")
        intervals = []
        for row in rows:
            if row["kind"] == "index_response_start":
                batch = int(row["batch"])
                intervals.append(
                    (int(row["cycle"]), at(rows, "index_dma_end", batch), "Index")
                )
            elif row["kind"] == "kv_response_start":
                intervals.append((int(row["cycle"]), int(row["cycle"]) + 4, "KV"))
        intervals.sort()
        if any(
            left[1] > right[0]
            for left, right in zip(intervals, intervals[1:], strict=False)
        ):
            raise ValueError(f"{name}: shared response bus overlap")
        outstanding_rows = 0
        for row in rows:
            if row["kind"] == "kv_row_request":
                outstanding_rows += 1
            elif row["kind"] == "kv_row_complete":
                outstanding_rows -= 1
            if not 0 <= outstanding_rows <= 8:
                raise ValueError(f"{name}: KV credit violation")
        if outstanding_rows:
            raise ValueError(f"{name}: KV requests not drained")
        index_free = [-1, -1]
        kv_free = [-1] * kv_slots
        prior_valid = -1
        prior_compute_end = -1
        for batch in range(batches):
            i = slot_for(rows, "index_slot_acquire", batch)
            k = slot_for(rows, "kv_fill_start", batch)
            if i not in (0, 1) or k not in range(kv_slots):
                raise ValueError("invalid slot identifier")
            acquire = at(rows, "index_slot_acquire", batch, i)
            dma_start = at(rows, "index_dma_start", batch, i)
            dma_end = at(rows, "index_dma_end", batch, i)
            check_start = at(rows, "index_validate_start", batch, i)
            check_end = at(rows, "index_validate_end", batch, i)
            valid = at(rows, "index_batch_valid", batch, i)
            dispatch = at(rows, "index_dispatch_start", batch, i)
            release = at(rows, "index_slot_release", batch, i)
            fill_start = at(rows, "kv_fill_start", batch, k)
            fill_end = at(rows, "kv_fill_end", batch, k)
            compute_start = at(rows, "batch_compute_start", batch, k)
            compute_end = at(rows, "batch_compute_end", batch, k)
            if not (
                index_free[i]
                <= acquire
                == dma_start
                < dma_end
                <= check_start
                < check_end
                == valid
                <= dispatch
                == fill_start
                <= release
            ):
                raise ValueError(f"{name}: Index batch {batch} order/overlap")
            if check_end - check_start != 64:
                raise ValueError("Index validation budget")
            if not (
                kv_free[k] <= fill_start < fill_end <= compute_start < compute_end
                and prior_compute_end <= compute_start
            ):
                raise ValueError(f"{name}: KV batch {batch} order/overlap")
            if valid <= prior_valid:
                raise ValueError("Index validation reordered")
            index_free[i], kv_free[k] = release, compute_end
            prior_valid, prior_compute_end = valid, compute_end
            requests = {
                int(r["position"]): int(r["cycle"])
                for r in rows
                if r["kind"] == "kv_row_request" and r["batch"] == batch
            }
            completions = {
                int(r["position"]): int(r["cycle"])
                for r in rows
                if r["kind"] == "kv_row_complete" and r["batch"] == batch
            }
            if set(requests) != set(range(64)) or set(completions) != set(range(64)):
                raise ValueError(f"{name}: KV row positions missing or repeated")
            if any(
                not (
                    valid <= requests[p] <= release
                    and requests[p] < completions[p] <= fill_end
                )
                for p in range(64)
            ):
                raise ValueError(f"{name}: KV row timing relative to Index")
            for wave in range(4):
                for stage, expected in (
                    ("qk", 44),
                    ("softmax", SOFTMAX_CYCLES),
                    ("pv", 44),
                    ("state_commit", 1),
                ):
                    if (
                        at(rows, f"{stage}_end", batch, k, wave)
                        - at(rows, f"{stage}_start", batch, k, wave)
                    ) != expected:
                        raise ValueError(f"{name}: {stage} wave budget")
                if at(rows, "qk_end", batch, k, wave) != at(
                    rows, "softmax_start", batch, k, wave
                ) or at(rows, "softmax_end", batch, k, wave) != at(
                    rows, "pv_start", batch, k, wave
                ):
                    raise ValueError("compute stages out of order")
        if at(rows, "output_write_start") < max(prior_valid, prior_compute_end):
            raise ValueError("O write before full Index validation or compute")
        if at(rows, "completion") != at(rows, "output_write_ack"):
            raise ValueError("completion before O acknowledgement")
    if batches > 1:
        if not (
            at(two, "index_batch_valid", 0)
            <= at(two, "kv_row_request", 0, position=0)
            < at(two, "index_batch_valid", batches - 1)
        ):
            raise ValueError("KV waited for full Index list")
        if at(two, "index_dma_start", 1) >= at(two, "kv_fill_end", 0):
            raise ValueError("next Index load did not overlap KV gather")
    return batches


def rasterize(svg: Path, png: Path, width: int, height: int) -> None:
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
    gobject.g_object_unref.argtypes = [ctypes.c_void_p]
    surface = cairo.cairo_image_surface_create(0, width, height)
    context = cairo.cairo_create(surface)
    handle = rsvg.rsvg_handle_new_from_file(str(svg).encode(), None)
    if not handle or not rsvg.rsvg_handle_render_cairo(handle, context):
        raise RuntimeError("librsvg render failed")
    if cairo.cairo_surface_write_to_png(surface, str(png).encode()):
        raise RuntimeError("cairo PNG write failed")
    gobject.g_object_unref(handle)
    cairo.cairo_destroy(context)
    cairo.cairo_surface_destroy(surface)


def draw(
    rows: list[dict[str, int | str]],
    batches: int,
    one_cycles: int,
    two_cycles: int,
    prefix: Path,
    suffix: str,
    start: int,
    end: int,
    width: int = 2600,
) -> tuple[Path, Path]:
    if not 0 <= start < end <= two_cycles:
        raise ValueError("invalid view window")
    height, left, right, top, pitch = 1010, 220, 55, 245, 62
    scale = (width - left - right) / (end - start)

    def x(cycle: int) -> float:
        return left + (cycle - start) * scale

    def esc(value: object) -> str:
        return html.escape(str(value))

    count = batches * 64
    title = "启动及前五批放大" if suffix == "detail" else "全程"
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#fbfdff"/>',
        f'<text x="45" y="46" font-family="sans-serif" font-size="30" font-weight="700" fill="#172033">SparseFA · {count} Index · {title}泳道图</text>',
        '<text x="46" y="78" font-family="sans-serif" font-size="17" fill="#46556b">独立 C++ 时序模型实际执行 trace；64 Index/批，两个 Index slot，两个 KV slot；H=64，D=64</text>',
        f'<text x="46" y="110" font-family="sans-serif" font-size="18" fill="#854d0e">双 KV slot {two_cycles:,} 周期；单 KV slot {one_cycles:,} 周期；节省 {one_cycles - two_cycles:,} 周期（{(one_cycles - two_cycles) / one_cycles:.1%}）</text>',
        '<text x="46" y="141" font-family="sans-serif" font-size="15" fill="#a11b1b">模型时延，不是已生成硬件 DUT 或 RTL 的实测数据；Index 值假定有效且互异。</text>',
        f'<text x="{left}" y="185" font-family="sans-serif" font-size="15" fill="#475569">模型周期 {start:,}–{end:,}</text>',
    ]
    tick = 2000 if end - start > 10000 else 250 if end - start > 2000 else 100
    for cycle in range(((start + tick - 1) // tick) * tick, end + 1, tick):
        parts.append(
            f'<line x1="{x(cycle):.1f}" y1="204" x2="{x(cycle):.1f}" y2="859" stroke="#e2e8f0"/>'
        )
        parts.append(
            f'<text x="{x(cycle):.1f}" y="220" text-anchor="middle" font-family="sans-serif" font-size="12" fill="#64748b">{cycle:,}</text>'
        )
    for lane, (name, color) in enumerate(LANES):
        y = top + lane * pitch
        parts.append(
            f'<text x="{left - 15}" y="{y + 5}" text-anchor="end" font-family="sans-serif" font-size="17" fill="#26364a">{esc(name)}</text>'
        )
        parts.append(
            f'<line x1="{left}" y1="{y + 24}" x2="{width - right}" y2="{y + 24}" stroke="{color}" stroke-opacity=".22"/>'
        )

    def bar(a: int, b: int, lane: int, label: str, color: str | None = None) -> None:
        if b <= a:
            raise ValueError(f"nonpositive span: {label}")
        if b <= start or a >= end:
            return
        begin, finish = max(a, start), min(b, end)
        px, pw = x(begin), x(finish) - x(begin)
        y = top + lane * pitch
        parts.append(
            f'<rect x="{px:.1f}" y="{y - 17}" width="{max(1., pw):.1f}" height="34" rx="4" fill="{color or LANES[lane][1]}"><title>{esc(label)}: {a:,}–{b:,}</title></rect>'
        )
        if pw >= 50:
            caption = label if pw >= 90 else label.split(" ")[0]
            parts.append(
                f'<text x="{px + pw/2:.1f}" y="{y + 5}" text-anchor="middle" font-family="sans-serif" font-size="12" font-weight="600" fill="white">{esc(caption)}</text>'
            )

    bar(at(rows, "count_read_start"), at(rows, "count_read_end"), 0, "Count")
    bar(at(rows, "q_load_start"), at(rows, "q_load_end"), 0, "Q 8192 B")
    for batch in range(batches):
        i, k = slot_for(rows, "index_slot_acquire", batch), slot_for(
            rows, "kv_fill_start", batch
        )
        dma_start, dma_end = at(rows, "index_dma_start", batch, i), at(
            rows, "index_dma_end", batch, i
        )
        check_start, valid = at(rows, "index_validate_start", batch, i), at(
            rows, "index_batch_valid", batch, i
        )
        dispatch, release = at(rows, "index_dispatch_start", batch, i), at(
            rows, "index_slot_release", batch, i
        )
        bar(dma_start, dma_end, 1 + i, f"B{batch} DMA")
        if dma_end < check_start:
            bar(dma_end, check_start, 1 + i, f"B{batch} wait", "#94a3b8")
        bar(check_start, valid, 1 + i, f"B{batch} check", "#16a34a")
        if valid < dispatch:
            bar(valid, dispatch, 1 + i, f"B{batch} valid", "#94a3b8")
        if dispatch < release:
            bar(dispatch, release, 1 + i, f"B{batch} issue", "#0891b2")
        fill_end = at(rows, "kv_fill_end", batch, k)
        compute_start = at(rows, "batch_compute_start", batch, k)
        compute_end = at(rows, "batch_compute_end", batch, k)
        bar(at(rows, "kv_fill_start", batch, k), fill_end, 3 + k, f"B{batch} gather")
        if fill_end < compute_start:
            bar(fill_end, compute_start, 3 + k, f"B{batch} ready", "#94a3b8")
        bar(
            compute_start,
            compute_end,
            3 + k,
            f"B{batch} compute",
            "#c2410c" if k == 0 else "#be123c",
        )
        for wave in range(4):
            for stage, lane in (
                ("qk", 5),
                ("softmax", 6),
                ("pv", 7),
                ("state_commit", 8),
            ):
                bar(
                    at(rows, f"{stage}_start", batch, k, wave),
                    at(rows, f"{stage}_end", batch, k, wave),
                    lane,
                    f"B{batch} W{wave}",
                )
    bar(
        at(rows, "output_write_start"),
        at(rows, "output_write_ack"),
        9,
        "O 8192 B + ack",
    )
    for cycle, label in (
        (at(rows, "index_batch_valid", 0), "首批 Index 放行"),
        (at(rows, "kv_row_request", 0, position=0), "首笔 KV 请求"),
        (two_cycles, "完成"),
    ):
        if start <= cycle <= end:
            parts.append(
                f'<line x1="{x(cycle):.1f}" y1="225" x2="{x(cycle):.1f}" y2="849" stroke="#334155" stroke-dasharray="4 5" opacity=".5"/>'
            )
            if suffix != "overview" or cycle == two_cycles:
                anchor, offset = ("end", -4) if cycle == end else ("start", 4)
                parts.append(
                    f'<text x="{x(cycle)+offset:.1f}" y="883" text-anchor="{anchor}" font-family="sans-serif" font-size="13" fill="#334155">{cycle:,}: {esc(label)}</text>'
                )
    parts.extend(
        [
            f'<text x="46" y="925" font-family="sans-serif" font-size="15" fill="#475569">trace 含 {count:,} 笔 KV 请求和完成；蓝色 Index DMA、绿色校验预算、青色请求发射与 KV gather 交叠。</text>',
            f'<text x="46" y="951" font-family="sans-serif" font-size="15" fill="#475569">softmax 每 wave {SOFTMAX_CYCLES} 周期：普通 FP32 64 元素/周期 × 4 遍，exp 32 元素/周期；QK/PV 各 44 周期。</text>',
            '<text x="46" y="977" font-family="sans-serif" font-size="15" fill="#475569">Index 的 64 周期仅为校验调度预算；模型没有读取真实 ID、执行 Patricia 校验或计算 Attention 数值。</text>',
            "</svg>",
        ]
    )
    svg, png = Path(f"{prefix}-{suffix}.svg"), Path(f"{prefix}-{suffix}.png")
    svg.write_text("\n".join(parts) + "\n", encoding="utf-8")
    rasterize(svg, png, width, height)
    return svg, png


def render(prefix: Path) -> tuple[Path, Path]:
    one = read(Path(f"{prefix}-1slot.csv"))
    two = read(Path(f"{prefix}-2slot.csv"))
    batches = validate(one, two)
    return draw(
        two,
        batches,
        at(one, "completion"),
        at(two, "completion"),
        prefix,
        "overview",
        0,
        at(two, "completion"),
        width=2100,
    )


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: render_trace.py OUTPUT_PREFIX")
    for path in render(Path(sys.argv[1])):
        print(path)
