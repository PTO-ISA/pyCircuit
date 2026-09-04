from __future__ import annotations

import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple

IDENT = r"[A-Za-z_][A-Za-z0-9_$]*"
BLOCK_COMMENT_RE = re.compile(r"/\*.*?\*/", flags=re.S)
LINE_COMMENT_RE = re.compile(r"//.*?$", flags=re.M)
STRING_RE = re.compile(r'"(?:\\.|[^"\\])*"')
INCLUDE_RE = re.compile(r'(?m)^\s*`include\s+"([^"]+)"')
IMPORT_RE = re.compile(r"\bimport\s+([A-Za-z_][A-Za-z0-9_$]*)::(?:\*|[A-Za-z_][A-Za-z0-9_$]*)\s*;")
PACKAGE_SCOPE_RE = re.compile(r"\b(" + IDENT + r")::" + IDENT + r"\b")
PACKAGE_DECL_RE = re.compile(r"\bpackage\s+(" + IDENT + r")\b")
MODULE_DECL_RE = re.compile(r"\bmodule\s+(?:automatic\s+)?(" + IDENT + r")\b")
INTERFACE_DECL_RE = re.compile(r"\binterface\s+(?:automatic\s+)?(" + IDENT + r")\b")

KEYWORD_BLACKLIST = {
    "if", "else", "for", "foreach", "while", "case", "casex", "casez",
    # Case qualifiers and other statement keywords can match the lightweight
    # ``module instance`` recognizer (for example ``unique casez (...)``).
    # They are language syntax, not unresolved RTL modules.
    "unique", "unique0", "priority", "randcase", "inside", "matches",
    "always", "always_ff", "always_comb", "always_latch", "initial",
    "assign", "assert", "assume", "cover", "function", "task",
    "module", "interface", "program", "generate", "begin", "end",
    "return", "typedef", "class", "property", "sequence", "clocking",
    "endfunction", "endtask", "endcase", "endgenerate", "endclass",
    "endproperty", "endsequence", "endinterface", "endprogram",
    "package", "endpackage",
}


def strip_comments(text: str) -> str:
    return LINE_COMMENT_RE.sub("", BLOCK_COMMENT_RE.sub("", text))


def split_top_level(text: str, delimiter: str = ",") -> List[str]:
    parts, start = [], 0
    dp = db = dc = 0
    in_string = escaped = False
    for i, ch in enumerate(text):
        if in_string:
            if escaped:
                escaped = False
            elif ch == "\\":
                escaped = True
            elif ch == '"':
                in_string = False
            continue
        if ch == '"':
            in_string = True
        elif ch == "(": dp += 1
        elif ch == ")": dp = max(0, dp - 1)
        elif ch == "[": db += 1
        elif ch == "]": db = max(0, db - 1)
        elif ch == "{": dc += 1
        elif ch == "}": dc = max(0, dc - 1)
        elif ch == delimiter and dp == 0 and db == 0 and dc == 0:
            parts.append(text[start:i].strip())
            start = i + 1
    parts.append(text[start:].strip())
    return [p for p in parts if p]


def extract_balanced(text: str, open_idx: int, open_ch="(", close_ch=")") -> Tuple[str, int]:
    depth = 0
    in_string = escaped = False
    for i in range(open_idx, len(text)):
        ch = text[i]
        if in_string:
            if escaped:
                escaped = False
            elif ch == "\\":
                escaped = True
            elif ch == '"':
                in_string = False
            continue
        if ch == '"':
            in_string = True
        elif ch == open_ch:
            depth += 1
        elif ch == close_ch:
            depth -= 1
            if depth == 0:
                return text[open_idx + 1:i], i
    raise ValueError("unbalanced delimiters")


def _find_module_end(text: str, start: int) -> int:
    m = re.search(r"\bendmodule\b", text[start:])
    return len(text) if not m else start + m.end()


def _extract_module_header(clean: str, decl_match: re.Match) -> Tuple[str, str, int]:
    pos, n = decl_match.end(), len(clean)
    while pos < n and clean[pos].isspace(): pos += 1
    # SystemVerilog permits a package import between the module name and its
    # parameter/port lists (``module m import pkg::*; #( ... )``).  Skip that
    # clause so the normal balanced-delimiter extraction can see the header.
    if re.match(r"import\b", clean[pos:]):
        semi = clean.find(";", pos)
        if semi >= 0:
            pos = semi + 1
            while pos < n and clean[pos].isspace(): pos += 1
    param_block = ""
    if pos < n and clean[pos] == "#":
        pos += 1
        while pos < n and clean[pos].isspace(): pos += 1
        if pos < n and clean[pos] == "(":
            param_block, end = extract_balanced(clean, pos)
            pos = end + 1
    while pos < n and clean[pos].isspace(): pos += 1
    port_block = ""
    if pos < n and clean[pos] == "(":
        port_block, end = extract_balanced(clean, pos)
        pos = end + 1
    semi = clean.find(";", pos)
    return param_block, port_block, (pos if semi == -1 else semi + 1)


def parse_parameter_block(block: str) -> List[Dict]:
    out = []
    if not block.strip(): return out
    for chunk in split_top_level(block):
        c = " ".join(chunk.replace("\n", " ").split())
        c = re.sub(r"^(parameter|localparam)\b\s*", "", c)
        left, _, default = c.partition("=")
        ids = re.findall(IDENT, left)
        if not ids: continue
        name = ids[-1]
        out.append({
            "name": name,
            "type": left[:left.rfind(name)].strip(),
            "default": default.strip(),
            "raw": chunk.strip(),
        })
    return out


def parse_port_block(block: str) -> List[Dict]:
    out = []
    if not block.strip(): return out
    inherited_dir: Optional[str] = None
    inherited_type = inherited_width = ""
    for chunk in split_top_level(block):
        raw = " ".join(chunk.replace("\n", " ").split())
        if not raw: continue
        m = re.match(r"^(input|output|inout|ref)\b", raw)
        body = raw
        if m:
            direction = inherited_dir = m.group(1)
            body = raw[m.end():].strip()
        else:
            direction = inherited_dir
        # Interface/modport style.
        im = re.match(r"^(" + IDENT + r"(?:\." + IDENT + r")?)\s+(" + IDENT + r")$", body)
        # An interface/modport port may follow a preceding ANSI port with an
        # inherited direction (e.g. ``input wire reset,`` then
        # ``my_if.master bus``).  Check the explicit-token flag ``m`` rather
        # than the inherited direction so the interface is not misclassified
        # as an ordinary input.
        if m is None and im:
            out.append({"name": im.group(2), "direction": "interface", "type": im.group(1), "width": "", "raw": chunk.strip()})
            continue
        ids = re.findall(IDENT, body)
        if not ids: continue
        name = ids[-1]
        before = body[:body.rfind(name)].strip()
        width = "".join(re.findall(r"\[[^\]]+\]", before))
        type_text = " ".join(re.sub(r"\[[^\]]+\]", "", before).split())
        if m:
            inherited_type, inherited_width = type_text, width
        elif direction and not type_text:
            type_text, width = inherited_type, inherited_width
        out.append({"name": name, "direction": direction or "unknown", "type": type_text, "width": width, "raw": chunk.strip()})
    return out


def detect_clock_reset(ports: List[Dict], body: str) -> Tuple[List[Dict], List[Dict]]:
    clocks, resets = [], []
    for p in ports:
        name, low = p["name"], p["name"].lower()
        if p["direction"] not in ("input", "inout", "unknown"): continue
        if low in {"clk", "clock"} or re.search(r"(^|_)(clk|clock)($|_)", low):
            clocks.append({"name": name})
        if "reset" in low or low.startswith("rst") or re.search(r"(^|_)rst($|_)", low):
            polarity = "active_low" if (low.endswith("_n") or low.endswith("_ni") or "rst_n" in low or "reset_n" in low) else "active_high"
            async_pat = re.compile(r"@\s*\([^)]*(?:posedge|negedge)\s+" + re.escape(name) + r"\b", re.I | re.S)
            resets.append({"name": name, "polarity": polarity, "style": "async" if async_pat.search(body) else "unknown"})
    return clocks, resets


def detect_handshake(ports: List[Dict]) -> List[str]:
    names = [p["name"].lower() for p in ports]
    out = []
    if any("valid" in n for n in names) and any("ready" in n for n in names): out.append("valid_ready")
    if any("req" in n or "request" in n for n in names) and any("gnt" in n or "grant" in n for n in names): out.append("req_gnt")
    return out


def parse_instances(body: str) -> List[Dict]:
    # Practical line-oriented recognizer for direct module instances.
    # Macro-generated/very exotic instantiations are intentionally left for a real AST later.
    text = STRING_RE.sub('""', body)
    pat = re.compile(r"(?ms)^[ \t]*(" + IDENT + r")\b\s*(?:#\s*\((.{0,4000}?)\)\s*)?(" + IDENT + r")\b\s*\(")
    out, seen = [], set()
    for m in pat.finditer(text):
        mod_type, inst_name = m.group(1), m.group(3)
        if mod_type in KEYWORD_BLACKLIST: continue
        key = (mod_type, inst_name)
        if key in seen: continue
        seen.add(key)
        out.append({"module_type": mod_type, "instance_name": inst_name})
    return out


def parse_sv_file(path: Path, repo_root: Path) -> Dict:
    raw = path.read_text(encoding="utf-8", errors="ignore")
    clean = strip_comments(raw)
    includes = INCLUDE_RE.findall(clean)
    imports = sorted(
        set(IMPORT_RE.findall(clean))
        | set(PACKAGE_SCOPE_RE.findall(clean))
    )
    packages_declared = sorted(set(PACKAGE_DECL_RE.findall(clean)))
    interfaces_declared = sorted(set(INTERFACE_DECL_RE.findall(clean)))
    modules = []
    for m in MODULE_DECL_RE.finditer(clean):
        name = m.group(1)
        param_block, port_block, header_end = _extract_module_header(clean, m)
        body_end = _find_module_end(clean, header_end)
        body = clean[header_end:body_end]
        params = parse_parameter_block(param_block)
        ports = parse_port_block(port_block)
        clocks, resets = detect_clock_reset(ports, body)
        modules.append({
            "module": name,
            "file": path.relative_to(repo_root).as_posix(),
            "parameters": params,
            "ports": ports,
            "clocks": clocks,
            "resets": resets,
            "handshakes": detect_handshake(ports),
            "instances": parse_instances(body),
            "interfaces": sorted({
                p.get("type", "").split(".", 1)[0]
                for p in ports if p.get("direction") == "interface"
            }),
            "includes": includes,
            "imports": imports,
        })
    return {"file": path.relative_to(repo_root).as_posix(), "includes": includes, "imports": imports, "packages_declared": packages_declared, "interfaces_declared": interfaces_declared, "modules": modules}
