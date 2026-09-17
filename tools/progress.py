#!/usr/bin/env python3
"""Generate the public LEGOLAND decompilation progress report.

The report uses committed FUNCTION/WIP-FUNCTION annotations as its source of
truth, so GitHub Pages needs neither the original binary nor the VC6 toolchain.

Those annotations give function counts, not code size, and a function count
badly overstates completion: the export figure alone reads ~98% while less
than half the game's code is reproduced. So the headline figure is byte
coverage, read from `docs/coverage.json` — a checkpoint written by
`tools/coverage.py --write`, which does have the binary. If that file is
missing the report falls back to function counts and says so.
"""

from __future__ import annotations

import argparse
import html
import json
import re
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "LEGOLAND"
EXPORTS = ROOT / "symbols" / "legoland.exports.txt"
HTML_OUT = ROOT / "docs" / "LEGOLANDPROGRESS.HTML"
SVG_OUT = ROOT / "docs" / "LEGOLANDPROGRESS.SVG"
COVERAGE = ROOT / "docs" / "coverage.json"
REPO_URL = "https://github.com/adamxbot/legoland"
# Executable range from this binary's PE .text section (VA 0x401000, size 0xa9d46).
TEXT_START = 0x401000
TEXT_END = 0x4AAD46

MARKER = re.compile(r"//\s*(WIP-)?FUNCTION:\s*LEGOLAND\s+(0x[0-9a-fA-F]+)")
NAME = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(")


@dataclass(frozen=True)
class Function:
    address: int
    name: str
    status: str
    source: str = ""
    line: int = 0
    exported: bool = True


def read_exports() -> dict[int, str]:
    exports: dict[int, str] = {}
    for line in EXPORTS.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        name, _ordinal, rva = line.split("\t")
        address = 0x400000 + int(rva, 16)
        if address in exports:
            raise ValueError(f"duplicate export address: 0x{address:08x}")
        exports[address] = name
    return exports


def read_annotations() -> dict[int, Function]:
    annotations: dict[int, Function] = {}
    for path in sorted(SOURCE.glob("*.c")):
        lines = path.read_text(encoding="utf-8").splitlines()
        for index, line in enumerate(lines):
            marker = MARKER.search(line)
            if not marker:
                continue
            address = int(marker.group(2), 16)
            if address in annotations:
                raise ValueError(f"duplicate annotation: 0x{address:08x}")
            name = f"sub_{address:08x}"
            for candidate in lines[index + 1 : index + 9]:
                match = NAME.search(candidate)
                if match and not candidate.lstrip().startswith("//"):
                    name = match.group(1)
                    break
            annotations[address] = Function(
                address=address,
                name=name,
                status="wip" if marker.group(1) else "matched",
                source=path.relative_to(ROOT).as_posix(),
                line=index + 1,
                exported=False,
            )
    return annotations


def collect() -> tuple[list[Function], dict[str, int]]:
    all_exports = read_exports()
    exports = {
        address: name
        for address, name in all_exports.items()
        if TEXT_START <= address < TEXT_END
    }
    annotations = read_annotations()
    functions: list[Function] = []

    for address, name in sorted(exports.items()):
        annotation = annotations.get(address)
        functions.append(
            Function(
                address=address,
                name=name,
                status=annotation.status if annotation else "not-started",
                source=annotation.source if annotation else "",
                line=annotation.line if annotation else 0,
                exported=True,
            )
        )

    functions.extend(
        Function(
            address=item.address,
            name=item.name,
            status=item.status,
            source=item.source,
            line=item.line,
            exported=False,
        )
        for address, item in sorted(annotations.items())
        if address not in exports
    )

    stats = {
        "exports": len(exports),
        "data_exports": len(all_exports) - len(exports),
        "matched_exports": sum(f.exported and f.status == "matched" for f in functions),
        "wip_exports": sum(f.exported and f.status == "wip" for f in functions),
        "matched_internal": sum(not f.exported and f.status == "matched" for f in functions),
        "wip_internal": sum(not f.exported and f.status == "wip" for f in functions),
    }
    stats["matched_total"] = stats["matched_exports"] + stats["matched_internal"]
    stats["wip_total"] = stats["wip_exports"] + stats["wip_internal"]
    return functions, stats


def read_coverage() -> dict[str, int] | None:
    """Byte-coverage checkpoint from `tools/coverage.py --write`, if committed."""
    if not COVERAGE.exists():
        return None
    data = json.loads(COVERAGE.read_text(encoding="utf-8"))
    game = data["game_bytes"]
    data["exact_percent"] = data["exact_bytes"] / game * 100
    data["partial_percent"] = (data["exact_bytes"] + data["wip_bytes"]) / game * 100
    return data


def render_svg(stats: dict[str, int], cov: dict[str, int] | None) -> str:
    export_percent = stats["matched_exports"] / stats["exports"] * 100
    if cov:
        headline = cov["exact_percent"]
        caption = f'{cov["exact_percent"]:.1f}% of game code exact · {cov["partial_percent"]:.1f}% with partials'
        footer = "Bytes of game code · exports and function counts run far ahead"
        desc = (
            f'{cov["exact_percent"]:.1f}% of the game\'s {cov["game_bytes"] / 1024:.0f} KB of code is '
            f'reproduced exactly, {cov["partial_percent"]:.1f}% counting partials, across '
            f'{stats["matched_total"]} exact functions.'
        )
    else:
        headline = export_percent
        caption = f'{stats["matched_exports"]} / {stats["exports"]} exported functions exact'
        footer = "Function-count progress · byte coverage unavailable"
        desc = (
            f'{stats["matched_exports"]} of {stats["exports"]} exported functions match exactly; '
            f'{stats["matched_internal"]} internal functions also match.'
        )
    bar_width = 664 * headline / 100
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="720" height="150" viewBox="0 0 720 150" role="img" aria-labelledby="title desc">
  <title id="title">LEGOLAND decompilation progress: {headline:.1f}%</title>
  <desc id="desc">{desc}</desc>
  <rect width="720" height="150" rx="14" fill="#202124"/>
  <text x="28" y="40" fill="#f5f5f5" font-family="system-ui, sans-serif" font-size="22" font-weight="700">LEGOLAND decompilation</text>
  <text x="692" y="40" fill="#ffd500" text-anchor="end" font-family="system-ui, sans-serif" font-size="22" font-weight="700">{headline:.1f}%</text>
  <rect x="28" y="61" width="664" height="20" rx="10" fill="#3a3d40"/>
  <rect x="28" y="61" width="{bar_width:.2f}" height="20" rx="10" fill="#31c86a"/>
  <text x="28" y="113" fill="#f5f5f5" font-family="system-ui, sans-serif" font-size="17">{caption}</text>
  <text x="692" y="113" fill="#b8bdc3" text-anchor="end" font-family="system-ui, sans-serif" font-size="15">{stats["matched_exports"]}/{stats["exports"]} exports · {stats["matched_total"]} fns · {stats["wip_total"]} WIP</text>
  <text x="28" y="137" fill="#92979d" font-family="system-ui, sans-serif" font-size="13">{footer} · click for the searchable report</text>
</svg>
'''


def render_html(functions: list[Function], stats: dict[str, int],
                cov: dict[str, int] | None) -> str:
    export_percent = stats["matched_exports"] / stats["exports"] * 100
    percent = cov["exact_percent"] if cov else export_percent
    if cov:
        cards = (
            f'<div class="card"><strong>{cov["exact_percent"]:.1f}%</strong>'
            f'<span>game code exact &middot; {cov["partial_percent"]:.1f}% with partials</span></div>'
            f'<div class="card"><strong>{cov["exact_functions"]}</strong>'
            f'<span>exact functions &middot; {cov["wip_functions"]} partial</span></div>'
            f'<div class="card"><strong>{export_percent:.1f}%</strong>'
            f'<span>{stats["matched_exports"]} / {stats["exports"]} exported functions</span></div>'
            f'<div class="card"><strong>{cov["exact_instructions"]:,}</strong>'
            f'<span>instructions reproduced</span></div>'
        )
        note = (
            f'The headline is <strong>bytes of game code</strong>: '
            f'{cov["exact_bytes"]:,} of {cov["game_bytes"]:,} bytes '
            f'({cov["game_bytes"] / 1024:.0f}&nbsp;KB of <code>.text</code>, excluding the '
            f'statically-linked C runtime, which is not a decompilation target). It is the only '
            f'measure here with a fixed denominator, so it moves only by doing work. '
            f'The export figure runs far ahead of it — exports are just the symbols the linker '
            f'exposed, and {stats["matched_total"]} matched functions sit behind only '
            f'{stats["matched_exports"]} of them. The binary\'s other {stats["data_exports"]} '
            f'named exports are data symbols. Byte figures are from the '
            f'{cov["generated"]} <code>tools/coverage.py</code> checkpoint; function counts are live.'
        )
        bar_label = f'{cov["exact_percent"]:.1f}% of game code exact'
    else:
        cards = (
            f'<div class="card"><strong>{export_percent:.1f}%</strong><span>named exports exact</span></div>'
            f'<div class="card"><strong>{stats["matched_exports"]} / {stats["exports"]}</strong><span>exported functions</span></div>'
            f'<div class="card"><strong>{stats["matched_total"]}</strong><span>exact functions total</span></div>'
            f'<div class="card"><strong>{stats["matched_internal"]}</strong><span>internal exact &middot; {stats["wip_total"]} WIP</span></div>'
        )
        note = (
            f'Byte coverage is unavailable (no <code>docs/coverage.json</code>), so this page '
            f'counts functions. That overstates completion: exports are only the symbols the '
            f'linker exposed. Internal functions are reported separately so they do not distort '
            f'the {stats["exports"]}-function baseline; the binary\'s other '
            f'{stats["data_exports"]} named exports are data symbols.'
        )
        bar_label = f'{export_percent:.1f}% of named exports exact'
    labels = {"matched": "Exact", "wip": "WIP", "not-started": "Not started"}
    rows = []
    for function in functions:
        source = ""
        if function.source:
            href = f"{REPO_URL}/blob/main/{function.source}#L{function.line}"
            source = f'<a href="{html.escape(href)}">{html.escape(function.source)}:{function.line}</a>'
        scope = "Export" if function.exported else "Internal"
        rows.append(
            f'<tr data-status="{function.status}">'
            f'<td><code>0x{function.address:08x}</code></td>'
            f'<td>{html.escape(function.name)}</td>'
            f'<td>{scope}</td>'
            f'<td><span class="status {function.status}">{labels[function.status]}</span></td>'
            f'<td>{source}</td></tr>'
        )

    return f'''<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LEGOLAND decompilation status</title>
  <style>
    :root {{ color-scheme: dark; font-family: system-ui, sans-serif; background: #171819; color: #f5f5f5; }}
    body {{ margin: 0; }}
    main {{ width: min(1100px, calc(100% - 32px)); margin: 40px auto; }}
    a {{ color: #76b7ff; }}
    h1 {{ margin-bottom: 8px; }}
    .lede {{ color: #b8bdc3; margin-top: 0; }}
    .cards {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 12px; margin: 28px 0; }}
    .card {{ background: #242628; border: 1px solid #383b3e; border-radius: 10px; padding: 16px; }}
    .card strong {{ display: block; color: #ffd500; font-size: 26px; }}
    .card span {{ color: #b8bdc3; }}
    .bar {{ height: 16px; overflow: hidden; background: #3a3d40; border-radius: 8px; }}
    .bar span {{ display: block; width: {percent:.4f}%; height: 100%; background: #31c86a; }}
    .controls {{ display: flex; gap: 10px; margin: 28px 0 12px; }}
    input, select {{ box-sizing: border-box; border: 1px solid #555b60; border-radius: 7px; background: #242628; color: inherit; padding: 10px 12px; font: inherit; }}
    input {{ flex: 1; min-width: 0; }}
    table {{ width: 100%; border-collapse: collapse; font-size: 14px; }}
    th, td {{ padding: 10px; border-bottom: 1px solid #383b3e; text-align: left; }}
    th {{ position: sticky; top: 0; background: #171819; }}
    tbody tr:hover {{ background: #242628; }}
    td:last-child {{ word-break: break-word; }}
    .status {{ display: inline-block; border-radius: 999px; padding: 3px 8px; white-space: nowrap; }}
    .status.matched {{ background: #174f2c; color: #8cf0ac; }}
    .status.wip {{ background: #5b4713; color: #ffe08a; }}
    .status.not-started {{ background: #34373a; color: #c4c8cc; }}
    .note, #count {{ color: #92979d; }}
    @media (max-width: 720px) {{
      main {{ width: min(100% - 20px, 1100px); margin-top: 20px; }}
      .controls {{ flex-direction: column; }}
      th:nth-child(3), td:nth-child(3), th:nth-child(5), td:nth-child(5) {{ display: none; }}
    }}
  </style>
</head>
<body>
<main>
  <h1>LEGOLAND decompilation status</h1>
  <p class="lede">Exact, full-body VC6 matches recovered from the original 2000 Windows release.</p>
  <div class="cards">{cards}</div>
  <div class="bar" aria-label="{bar_label}"><span></span></div>
  <p class="note">{note}</p>
  <div class="controls">
    <input id="search" type="search" placeholder="Search by function, address, or source…" aria-label="Search functions">
    <select id="status" aria-label="Filter by status">
      <option value="all">All statuses</option>
      <option value="matched">Exact</option>
      <option value="wip">WIP</option>
      <option value="not-started">Not started</option>
    </select>
  </div>
  <p id="count"></p>
  <table>
    <thead><tr><th>Address</th><th>Function</th><th>Scope</th><th>Status</th><th>Source</th></tr></thead>
    <tbody>{''.join(rows)}</tbody>
  </table>
</main>
<script>
  const search = document.querySelector('#search');
  const status = document.querySelector('#status');
  const rows = [...document.querySelectorAll('tbody tr')];
  const count = document.querySelector('#count');
  function filter() {{
    const query = search.value.trim().toLowerCase();
    let visible = 0;
    for (const row of rows) {{
      const show = (status.value === 'all' || row.dataset.status === status.value) &&
        (!query || row.textContent.toLowerCase().includes(query));
      row.hidden = !show;
      visible += show;
    }}
    count.textContent = `${{visible}} function${{visible === 1 ? '' : 's'}} shown`;
  }}
  search.addEventListener('input', filter);
  status.addEventListener('change', filter);
  filter();
</script>
</body>
</html>
'''


def update(path: Path, content: str, check: bool) -> bool:
    if check:
        return path.exists() and path.read_text(encoding="utf-8") == content
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="fail if generated reports are stale")
    args = parser.parse_args()

    functions, stats = collect()
    cov = read_coverage()
    outputs = {
        HTML_OUT: render_html(functions, stats, cov),
        SVG_OUT: render_svg(stats, cov),
    }
    stale = [path for path, content in outputs.items() if not update(path, content, args.check)]
    if stale:
        print("stale progress report: " + ", ".join(str(path.relative_to(ROOT)) for path in stale))
        return 1
    if cov:
        print(
            f'{cov["exact_percent"]:.1f}% of game code exact '
            f'({cov["partial_percent"]:.1f}% with partials), from the {cov["generated"]} checkpoint'
        )
    else:
        print("no docs/coverage.json; reporting function counts only")
    print(
        f'{stats["matched_exports"]}/{stats["exports"]} exports exact '
        f'({stats["matched_exports"] / stats["exports"] * 100:.1f}%); '
        f'{stats["matched_total"]} exact functions total; {stats["wip_total"]} WIP'
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
