"""Generate a hardware block architecture drawing for draft Shared-KV SparseFA."""

from __future__ import annotations

import ctypes
from html import escape
from pathlib import Path
from xml.etree import ElementTree

WIDTH, HEIGHT = 3600, 2200
HERE = Path(__file__).resolve().parent
SVG = HERE / "sparsefa-shared-kv-hardware-architecture.svg"
PNG = HERE / "sparsefa-shared-kv-hardware-architecture.png"
COLORS = {
    "shell": ("#334155", "#f8fafc"),
    "control": ("#4f46e5", "#eef2ff"),
    "fabric": ("#475569", "#f1f5f9"),
    "index": ("#1d4ed8", "#eff6ff"),
    "kv": ("#c2410c", "#fff7ed"),
    "compute": ("#0f766e", "#f0fdfa"),
    "vector": ("#be185d", "#fdf2f8"),
    "fault": ("#b91c1c", "#fef2f2"),
}
layers: dict[str, list[str]] = {
    "background": [],
    "wires": [],
    "modules": [],
    "labels": [],
}


def label(
    x: int,
    y: int,
    words: str,
    size: int = 22,
    color: str = "#334155",
    weight: int = 400,
    anchor: str = "start",
) -> str:
    return (
        f'<text x="{x}" y="{y}" text-anchor="{anchor}" '
        f'font-family="Noto Sans CJK SC, DejaVu Sans, sans-serif" '
        f'font-size="{size}" font-weight="{weight}" fill="{color}">'
        f"{escape(words)}</text>"
    )


def group(
    x: int, y: int, w: int, h: int, title: str, category: str, side: str = ""
) -> None:
    stroke, fill = COLORS[category]
    layers["background"].append(
        f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="22" '
        f'fill="{fill}" stroke="{stroke}" stroke-width="3"/>'
        f'<rect x="{x}" y="{y}" width="{w}" height="62" rx="22" fill="{stroke}"/>'
        f'<rect x="{x}" y="{y+40}" width="{w}" height="22" fill="{stroke}"/>'
    )
    layers["labels"].append(label(x + 23, y + 42, title, 28, "#ffffff", 700))
    if side:
        layers["labels"].append(
            label(x + w - 22, y + 41, side, 19, "#ffffff", 500, "end")
        )


def card(
    x: int,
    y: int,
    w: int,
    h: int,
    title: str,
    details: tuple[str, ...],
    category: str,
    font: int = 21,
) -> None:
    stroke, fill = COLORS[category]
    layers["modules"].append(
        f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="14" '
        f'fill="#ffffff" stroke="{stroke}" stroke-width="2.3"/>'
        f'<rect x="{x}" y="{y}" width="{w}" height="52" rx="14" fill="{stroke}"/>'
        f'<rect x="{x}" y="{y+37}" width="{w}" height="15" fill="{stroke}"/>'
    )
    layers["labels"].append(label(x + 17, y + 35, title, 24, "#ffffff", 700))
    for row, detail in enumerate(details):
        layers["labels"].append(
            label(x + 17, y + 85 + row * 34, detail, font, "#334155")
        )


def wire(
    points: tuple[tuple[int, int], ...],
    color: str,
    width: int = 5,
    arrow: bool = True,
    dash: bool = False,
) -> None:
    path = "M " + " L ".join(f"{x} {y}" for x, y in points)
    marker = f' marker-end="url(#arrow-{color.lstrip("#")})"' if arrow else ""
    pattern = ' stroke-dasharray="12 9"' if dash else ""
    layers["wires"].append(
        f'<path d="{path}" fill="none" stroke="{color}" stroke-width="{width}" '
        f'stroke-linejoin="round" stroke-linecap="round"{marker}{pattern}/>'
    )


def port(x: int, y: int, text_value: str, color: str, above: bool = True) -> None:
    layers["labels"].append(
        label(x, y - 12 if above else y + 26, text_value, 18, color, 600, "middle")
    )


def draw() -> str:
    # External platform and the accelerator containment boundary.
    layers["background"].append(
        '<rect x="90" y="165" width="3420" height="220" rx="22" '
        'fill="#ffffff" stroke="#94a3b8" stroke-width="3" stroke-dasharray="13 10"/>'
        '<rect x="90" y="415" width="3420" height="1670" rx="28" '
        'fill="#ffffff" stroke="#1e3a8a" stroke-width="4"/>'
    )
    layers["labels"].append(label(125, 198, "SoC / 主机边界", 22, "#64748b", 700))
    card(
        145,
        220,
        620,
        125,
        "命令 / 完成",
        ("SharedKVCommand、cancel、completion",),
        "shell",
    )
    card(
        835,
        220,
        2615,
        125,
        "片外 HBM / DDR",
        (
            "Count + 预生成 Index     |     Q[B,H,Sq,D]     |     单份共享 KV[B,Sk,D]     |     O[B,H,Sq,D]",
        ),
        "shell",
    )
    layers["labels"].append(
        label(130, 465, "SparseFA Accelerator / Shared-KV 目标架构", 31, "#1e3a8a", 700)
    )

    # Control plane: these modules issue identities and grants, not payload data.
    group(
        130,
        495,
        3340,
        235,
        "控制平面 / Control Plane",
        "control",
        "单 active operation；H,D 在接受时固定（各 1..64）",
    )
    card(
        165,
        555,
        750,
        145,
        "命令与描述符检查",
        ("形状 / stride / 地址范围 / dtype / scale", "错误描述符禁止产生访存"),
        "control",
    )
    card(
        950,
        555,
        770,
        145,
        "Operation / Batch 调度器",
        ("(b,i,list_owner,batch)、16 head/wave", "Index、KV、计算按 batch 顺序协调"),
        "control",
    )
    card(
        1755,
        555,
        770,
        145,
        "资源与身份管理",
        (
            "Index/KV slot 分配；generation / tag",
            "credit、ready/valid、反压、slot 复用",
        ),
        "control",
    )
    card(
        2560,
        555,
        875,
        145,
        "故障、取消与提交控制",
        ("跨批首错 / drain / 候选状态失效", "完整列表通过后才开放 O 写回"),
        "fault",
    )

    # Shared external transaction fabric and its four named clients.
    group(
        130,
        770,
        3340,
        285,
        "片外传输子系统 / Memory & DMA Fabric",
        "fabric",
        "请求与响应可并行在途；同一物理链路按仲裁共享",
    )
    layers["background"].append(
        '<rect x="170" y="830" width="3260" height="64" rx="12" ' 'fill="#334155"/>'
    )
    layers["labels"].append(
        label(
            1800,
            871,
            "Tagged Request / Response Fabric   •   请求仲裁   •   响应分发   •   有界队列",
            25,
            "#ffffff",
            700,
            "middle",
        )
    )
    card(
        180,
        930,
        705,
        95,
        "Count / Index DMA",
        ("连续 64 项装载 + 响应装配",),
        "index",
        20,
    )
    card(930, 930, 760, 95, "KV Gather DMA", ("scatter 行请求 / ≤8 credit",), "kv", 20)
    card(1735, 930, 760, 95, "Q DMA", ("Q head tile 载入",), "compute", 20)
    card(
        2540, 930, 890, 95, "O Writeback DMA", ("输出写确认后 completion",), "shell", 20
    )

    # Physically separate datapath subsystems with local shared buses.
    group(130, 1135, 995, 855, "Index / 地址生成子系统", "index", "本模块不计算 Index")
    group(1165, 1135, 1040, 855, "共享 KV / Buffer 子系统", "kv", "K=V，仅一份 KV tile")
    group(2245, 1135, 1225, 855, "计算与输出子系统", "compute", "QK 与 PV 复用阵列")

    # Index hierarchy.
    card(
        165,
        1225,
        430,
        215,
        "IndexSlotManager",
        (
            "2 × 64×uint32；每 slot 256 B",
            "byte 到达 / 错误位图",
            "装载、VALID、释放 owner",
        ),
        "index",
    )
    card(
        625,
        1225,
        465,
        215,
        "IndexValidator + Patricia",
        (
            "按原位置校验 memory / range / duplicate",
            "跨批精确集合：4096×64 bit",
            "发布 BATCH_VALID(t)",
        ),
        "index",
        19,
    )
    card(
        165,
        1540,
        385,
        230,
        "Count / Index Loader",
        (
            "Count: 0..min(2048,Sk)",
            "64 项连续 burst；尾批不读 padding",
            "乱序 beat 按位置装配",
        ),
        "index",
        19,
    )
    card(
        590,
        1540,
        500,
        230,
        "GatherAGU + mask",
        (
            "仅使用已校验 token ID",
            "kv_base + b·stride + token·row_stride",
            "causal mask；复制原位置 / ID 到 KV",
        ),
        "index",
        19,
    )

    # KV hierarchy: response assembly, two physically independent 16-bank tiles.
    card(
        1200,
        1225,
        430,
        215,
        "Response Assembly",
        (
            "tag → slot / position / word",
            "8 行 × 4 字 × 32 B 有界暂存",
            "完整字才写 SRAM bank",
        ),
        "kv",
        20,
    )
    card(
        1665,
        1225,
        245,
        215,
        "KV slot 0",
        ("16 bank 1R1W", "≤8192 B KV", "token / valid / tag"),
        "kv",
        19,
    )
    card(
        1930,
        1225,
        245,
        215,
        "KV slot 1",
        ("16 bank 1R1W", "≤8192 B KV", "token / valid / tag"),
        "kv",
        19,
    )
    card(
        1200,
        1540,
        430,
        230,
        "KVSlotManager",
        (
            "FREE / FILLING / READY / IN_USE",
            "两个 slot 独立仲裁与 generation",
            "所有 head wave 用完才释放",
        ),
        "kv",
        19,
    )
    card(
        1665,
        1540,
        510,
        230,
        "Bank Read / Decode / Transpose",
        (
            "16 bank 同读：512 B 原始 KV",
            "扩展为 1024 B FP32 操作数",
            "QK[token,dim] / PV[dim,token]",
        ),
        "kv",
        19,
    )

    # Compute cluster: local crossbar is intentionally shown as a shared
    # interconnect, not a linear per-stage pipeline.
    card(
        2280,
        1225,
        355,
        215,
        "Operand Feeder",
        ("Q、KV、w_p 组装", "16×16 两侧操作数", "同步读返回对齐"),
        "compute",
        19,
    )
    card(
        2665,
        1225,
        355,
        215,
        "Q Head Store",
        ("最多 64×64 FP32", "各 head 独立 Q", "尾 H / D mask"),
        "compute",
        19,
    )
    card(
        3050,
        1225,
        385,
        215,
        "3D MAC 16×16×16",
        ("4096 乘法位置", "256 个点积归约", "QK / PV；约 32 KiB 寄存数据"),
        "compute",
        19,
    )
    card(
        2280,
        1540,
        355,
        230,
        "Score / Weight SRAM",
        ("每 wave 16×64 FP32", "4 KiB 分阶段复用", "不存完整 S / P"),
        "vector",
        19,
    )
    card(
        2665,
        1540,
        355,
        230,
        "Vector + exp",
        ("普通 FP32 256 B/周期", "exp 32 FP32 元素/周期", "max / shift / w / sum"),
        "vector",
        19,
    )
    card(
        3050,
        1540,
        385,
        230,
        "HeadState",
        ("最多 64 head：m/l 512 B", "FP32 acc ≤16 KiB", "m/l/acc 每批原子提交"),
        "compute",
        19,
    )
    card(
        2280,
        1820,
        1155,
        125,
        "Normalize / O staging",
        (
            "acc/l 或空集合全零；FP16/BF16 cast；完整 Index 列表成功且全部计算完成后才可写 O",
        ),
        "compute",
        20,
    )

    # Thick local buses express shared ports and module topology.
    for x, w, color, title in (
        (165, 925, "#1d4ed8", "Index slot / validator / AGU 本地互联"),
        (1200, 975, "#c2410c", "KV tag / bank-write / bank-read 互联"),
        (2280, 1155, "#0f766e", "QK / PV operand + result fabric"),
    ):
        layers["background"].append(
            f'<rect x="{x}" y="1460" width="{w}" height="50" rx="10" fill="{color}"/>'
        )
        layers["labels"].append(
            label(x + w // 2, 1494, title, 20, "#ffffff", 700, "middle")
        )

    # External command and memory, with distinct request and response paths.
    wire(((440, 345), (440, 555)), "#4f46e5", 5)
    wire(((3420, 345), (3480, 345), (3480, 845), (3430, 845)), "#475569", 5)
    wire(((3430, 875), (3500, 875), (3500, 305), (3450, 305)), "#475569", 5)
    port(3050, 390, "片外 tagged req / resp", "#475569")

    # DMA client ports down to their owning subsystems.
    wire(((520, 1025), (520, 1090), (380, 1090), (380, 1225)), "#1d4ed8")
    wire(((1330, 1025), (1330, 1225)), "#c2410c")
    wire(((2110, 1025), (2110, 1095), (2840, 1095), (2840, 1225)), "#0f766e")
    wire(
        (
            (3020, 1820),
            (3020, 1795),
            (3500, 1795),
            (3500, 1080),
            (3000, 1080),
            (3000, 1025),
        ),
        "#475569",
    )
    port(520, 1117, "Index payload", "#1d4ed8")
    port(1330, 1108, "KV fragments", "#c2410c")

    # Three local bus taps. Each boxed module retains its own state.
    for x, top_y, bottom_y, color in (
        (380, 1440, 1460, "#1d4ed8"),
        (855, 1440, 1460, "#1d4ed8"),
        (355, 1510, 1540, "#1d4ed8"),
        (840, 1510, 1540, "#1d4ed8"),
        (1415, 1440, 1460, "#c2410c"),
        (1790, 1440, 1460, "#c2410c"),
        (2050, 1440, 1460, "#c2410c"),
        (1415, 1510, 1540, "#c2410c"),
        (1920, 1510, 1540, "#c2410c"),
        (2450, 1440, 1460, "#0f766e"),
        (2840, 1440, 1460, "#0f766e"),
        (3240, 1440, 1460, "#0f766e"),
        (2450, 1510, 1540, "#0f766e"),
        (2840, 1510, 1540, "#0f766e"),
        (3240, 1510, 1540, "#0f766e"),
    ):
        wire(((x, top_y), (x, bottom_y)), color, 4, arrow=False)

    # Typed inter-subsystem ports, independent of the panel layout.
    wire(((1090, 1630), (1200, 1630)), "#1d4ed8", 6)
    port(1145, 1606, "ID + mask", "#1d4ed8")
    wire(((2175, 1650), (2225, 1650), (2225, 1485), (2280, 1485)), "#c2410c", 6)
    port(2225, 1595, "KV tile", "#c2410c")
    wire(((2840, 1770), (2840, 1820)), "#0f766e", 5)
    wire(((3240, 1770), (3240, 1820)), "#0f766e", 5)

    # Control/tag rail goes to each subsystem, with separate fault/O gate.
    wire(
        ((133, 700), (112, 700), (112, 1080), (3330, 1080)),
        "#4f46e5",
        4,
        arrow=False,
        dash=True,
    )
    for x in (710, 1700, 2930):
        wire(((x, 1080), (x, 1135)), "#4f46e5", 4, dash=True)
    wire(
        ((3020, 700), (3475, 700), (3475, 1840), (3435, 1840)), "#b91c1c", 4, dash=True
    )
    port(1600, 1071, "控制 / tag / 反压", "#4f46e5")
    port(3370, 1825, "O 提交门", "#b91c1c")

    # Semantic notes are placed within their owning subsystem.
    layers["labels"].extend(
        (
            label(
                165,
                1840,
                "双 Index slot 只保留批载荷；Patricia 保留跨批去重状态。",
                19,
                "#1e40af",
                600,
            ),
            label(
                1200,
                1840,
                "共享列表：一批 KV 载入一次，供全部 head wave 重用。",
                19,
                "#9a3412",
                600,
            ),
            label(
                2280,
                1980,
                "head-specific Index 首版逐 head；不同列表不能直接广播共享 tile。",
                19,
                "#0f766e",
                600,
            ),
        )
    )
    defs = "".join(
        f'<marker id="arrow-{color.lstrip("#")}" viewBox="0 0 12 12" refX="10" refY="6" '
        f'markerWidth="12" markerHeight="12" orient="auto"><path '
        f'd="M 1 1 L 11 6 L 1 11 z" fill="{color}"/></marker>'
        for color in ("#4f46e5", "#475569", "#1d4ed8", "#c2410c", "#0f766e", "#b91c1c")
    )
    doc = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" '
        f'viewBox="0 0 {WIDTH} {HEIGHT}" role="img" '
        f'aria-label="SparseFA Shared-KV 硬件模块架构图">',
        f"<defs>{defs}</defs>",
        f'<rect width="{WIDTH}" height="{HEIGHT}" fill="#ffffff"/>',
        label(90, 70, "SparseFA Shared-KV / 硬件模块架构", 48, "#0f172a", 700),
        label(
            92,
            115,
            "顶层模块边界、片外互联、片上存储和计算资源；蓝色 Index、橙色共享 KV、青色计算、紫色控制。",
            24,
            "#475569",
        ),
        *layers["background"],
        *layers["wires"],
        *layers["modules"],
        *layers["labels"],
        label(
            100,
            2135,
            "目标 NDF 架构草案：不是已集成的 P0S / 三维阵列硬件实测。实线为载荷/端口，虚线为控制与提交条件。",
            22,
            "#9f1239",
            600,
        ),
        "</svg>",
    ]
    return "\n".join(doc) + "\n"


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
    gobject.g_object_unref.argtypes = [ctypes.c_void_p]
    surface = cairo.cairo_image_surface_create(0, WIDTH, HEIGHT)
    context = cairo.cairo_create(surface)
    handle = rsvg.rsvg_handle_new_from_file(str(svg).encode(), None)
    if not handle or not rsvg.rsvg_handle_render_cairo(handle, context):
        raise RuntimeError("could not render SVG")
    if cairo.cairo_surface_write_to_png(surface, str(png).encode()):
        raise RuntimeError("could not write PNG")
    gobject.g_object_unref(handle)
    cairo.cairo_destroy(context)
    cairo.cairo_surface_destroy(surface)


if __name__ == "__main__":
    SVG.write_text(draw(), encoding="utf-8")
    ElementTree.parse(SVG)
    rasterize(SVG, PNG)
    print(SVG)
    print(PNG)
