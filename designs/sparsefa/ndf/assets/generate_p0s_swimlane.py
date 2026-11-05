"""Render the P0S data-flow swimlane as SVG and PNG, using system librsvg."""

from __future__ import annotations

import ctypes
import html
from pathlib import Path


HERE = Path(__file__).resolve().parent
SVG = HERE / "fa2-p0s-dataflow-swimlane.svg"
PNG = HERE / "fa2-p0s-dataflow-swimlane.png"
WIDTH, HEIGHT = 2460, 1900
CENTERS = [185, 485, 785, 1085, 1385, 1685, 1985, 2285]
LANES = [
    ("外部 / Host", "预生成的 Index 与张量", "#eaf3ff", "#315f98"),
    ("调度 / 控制", "一个 query；阶段 FSM", "#fff4d6", "#9b6b12"),
    ("Index / Sparse", "校验完整前缀；保序", "#e7f7ed", "#32845e"),
    ("外部访存", "单 outstanding 请求", "#f0eaff", "#7052a5"),
    ("QK / Score", "逐维乘加；D ≤ 64", "#ffece4", "#af633d"),
    ("Online Softmax", "m、l、α、β 标量状态", "#fce7f1", "#ae4e7c"),
    ("PV 累加", "逐维写 shadow bank", "#e5f6f5", "#23827d"),
    ("归一化 / 写回", "逐维输出；等写确认", "#eaf2ff", "#416cab"),
]


def esc(value: str) -> str:
    return html.escape(value, quote=True)


parts = [
    f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" '
    f'viewBox="0 0 {WIDTH} {HEIGHT}" role="img" '
    'aria-label="FlashAttention2 P0S 数据流泳道图">',
    '<defs>',
    '<marker id="arrow" markerWidth="10" markerHeight="8" refX="9" refY="4" '
    'orient="auto" markerUnits="strokeWidth"><path d="M0 0 L10 4 L0 8 Z" fill="#34445e"/></marker>',
    '<marker id="arrow-muted" markerWidth="10" markerHeight="8" refX="9" refY="4" '
    'orient="auto" markerUnits="strokeWidth"><path d="M0 0 L10 4 L0 8 Z" fill="#8593a9"/></marker>',
    '</defs>',
    '<rect width="2460" height="1900" fill="#f7f9fc"/>',
    '<text x="78" y="67" font-size="39" font-weight="700" fill="#152338">'
    'FlashAttention2 · P0S 数据流泳道图</text>',
    '<text x="80" y="106" font-size="21" fill="#516276">'
    '时间向下 · 当前串行行引擎 · indexed 路径示例</text>',
]

for index, ((name, detail, fill, stroke), x) in enumerate(zip(LANES, CENTERS)):
    left = x - 137
    parts += [
        f'<rect x="{left}" y="137" width="274" height="1612" rx="15" '
        f'fill="{fill}" stroke="{stroke}" stroke-opacity=".34"/>',
        f'<rect x="{left}" y="137" width="274" height="86" rx="15" '
        f'fill="{stroke}"/>',
        f'<text x="{x}" y="174" text-anchor="middle" font-size="23" '
        f'font-weight="700" fill="white">{esc(name)}</text>',
        f'<text x="{x}" y="204" text-anchor="middle" font-size="16" '
        f'fill="white">{esc(detail)}</text>',
        f'<line x1="{x}" y1="230" x2="{x}" y2="1735" '
        'stroke="#9ba9bc" stroke-width="2" stroke-dasharray="7 9"/>',
    ]


def box(lane: int, y: int, lines: list[str], height: int = 72) -> None:
    x = CENTERS[lane]
    top = y - height // 2
    fill = "#ffffff"
    stroke = LANES[lane][3]
    parts.append(
        f'<rect x="{x - 124}" y="{top}" width="248" height="{height}" rx="13" '
        f'fill="{fill}" stroke="{stroke}" stroke-width="2.4"/>'
    )
    step = 24
    first = y - (len(lines) - 1) * step / 2 + 7
    for row, line in enumerate(lines):
        parts.append(
            f'<text x="{x}" y="{first + row * step}" text-anchor="middle" '
            f'font-size="19" font-weight="{650 if row == 0 else 450}" '
            f'fill="#253248">{esc(line)}</text>'
        )


def arrow(src: int, dst: int, y: int, label: str, muted: bool = False) -> None:
    start = CENTERS[src] + (126 if dst > src else -126)
    end = CENTERS[dst] - (134 if dst > src else -134)
    marker = "arrow-muted" if muted else "arrow"
    color = "#8593a9" if muted else "#34445e"
    parts.append(
        f'<line x1="{start}" y1="{y}" x2="{end}" y2="{y}" '
        f'stroke="{color}" stroke-width="3.1" marker-end="url(#{marker})"/>'
    )
    if label:
        parts.append(
            f'<text x="{(start + end) / 2}" y="{y - 10}" text-anchor="middle" '
            f'font-size="17" font-weight="600" fill="{color}">{esc(label)}</text>'
        )


def separator(y: int, caption: str) -> None:
    parts.extend([
        f'<line x1="50" y1="{y}" x2="2410" y2="{y}" '
        'stroke="#d8e0ea" stroke-width="2"/>',
        f'<rect x="66" y="{y - 18}" width="{max(225, 22 * len(caption))}" '
        'height="36" rx="18" fill="#34445e"/>',
        f'<text x="86" y="{y + 7}" font-size="18" font-weight="700" '
        f'fill="white">{esc(caption)}</text>',
    ])


box(0, 282, ["TensorCommand", "Index/Count 已由上游生成"])
arrow(0, 1, 282, "提交")
box(1, 282, ["捕获描述符", "检查布局与维度"])

separator(353, "① 先验证完整 Index 前缀")
box(1, 428, ["选择 b / h / query", "indexed: 先读 count"])
arrow(1, 3, 428, "Count 读")
box(3, 428, ["REQ → WAIT", "外部内存响应"])
arrow(3, 2, 500, "count")
box(2, 500, ["count ≤ 2048", "准备读有效前缀"])
box(0, 573, ["Index[p]", "原 token ID"])
arrow(0, 3, 573, "内存输入")
box(3, 573, ["逐项读 Index[p]", "每次只挂起 1 请求"])
arrow(3, 2, 645, "index")
box(2, 645, ["范围 + 重复校验", "Patricia 树；保留顺序"])
parts.append('<text x="785" y="711" text-anchor="middle" font-size="18" '
             'fill="#39624f">循环至完整前缀通过 · dense 模式跳过此段</text>')

separator(753, "② 选 token，计算 QK score")
box(1, 826, ["装载当前 Q[d]", "逐维 D = 1..64"])
arrow(1, 3, 826, "Q 读")
arrow(3, 4, 899, "Q[d]")
box(2, 899, ["取 Index[p]", "causal 用原 token ID"])
arrow(2, 4, 947, "有效 token")
box(4, 1000, ["逐维 K 读取", "dot += Q[d] × K[j,d]"])
arrow(4, 3, 1000, "K[j,d]")
arrow(3, 4, 1051, "K 响应")
box(4, 1104, ["score = dot × scale", "完成 D 个维度后"])
arrow(4, 5, 1104, "score")

separator(1177, "③ 在线 softmax + PV，逐 token 更新")
box(5, 1247, ["m′ = max(m, score)", "α、β：两次 exp"])
box(5, 1330, ["l′ = l×α + β", "保留候选行状态"])
arrow(5, 6, 1330, "α / β")
box(6, 1330, ["逐维 V 读取", "active / shadow 双 bank"])
arrow(6, 3, 1401, "V[j,d]")
arrow(3, 6, 1452, "V 响应")
box(6, 1506, ["shadow[d] =", "active[d]×α + β×V[d]"])
box(6, 1602, ["最后一个 d：", "切换 bank，提交 m′/l′"])
parts.append('<text x="1985" y="1655" text-anchor="middle" font-size="17" '
             'font-weight="700" fill="#23827d">还有 token：返回 ②，重做 QK → softmax → PV</text>')

separator(1671, "④ 全部 token 完成后归一化、写回")
arrow(6, 7, 1722, "acc[d] / l")
box(7, 1766, ["FP16 / BF16 编码", "O[d] 写入并等待 ack"], 82)
arrow(7, 3, 1823, "O 写 / ack")
parts.append('<text x="1220" y="1874" text-anchor="middle" font-size="20" '
             'font-weight="700" fill="#354861">最后一个写确认后发布 completion</text>')
parts.append('</svg>')
SVG.write_text("\n".join(parts) + "\n", encoding="utf-8")


def render_png() -> None:
    cairo = ctypes.CDLL("libcairo.so.2")
    rsvg = ctypes.CDLL("librsvg-2.so.2")
    cairo.cairo_image_surface_create.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
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
    gobject = ctypes.CDLL("libgobject-2.0.so.0")
    gobject.g_object_unref.argtypes = [ctypes.c_void_p]
    surface = cairo.cairo_image_surface_create(0, WIDTH, HEIGHT)
    context = cairo.cairo_create(surface)
    handle = rsvg.rsvg_handle_new_from_file(str(SVG).encode(), None)
    if not handle or not rsvg.rsvg_handle_render_cairo(handle, context):
        raise RuntimeError("librsvg could not render the swimlane")
    if cairo.cairo_surface_write_to_png(surface, str(PNG).encode()):
        raise RuntimeError("cairo could not write the swimlane PNG")
    gobject.g_object_unref(handle)
    cairo.cairo_destroy(context)
    cairo.cairo_surface_destroy(surface)


if __name__ == "__main__":
    render_png()
    print(SVG)
    print(PNG)
