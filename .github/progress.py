#!/usr/bin/env python3
"""Print a Discord message (JSON payload) with per-version Game progress from objdiff reports.

Usage: progress.py CUR_DIR [PREV_DIR]
Prints nothing when no displayed number changed since PREV_DIR.
Each directory holds <VERSION>_report/report.json, as downloaded from the Build workflow's artifacts.
"""
import json
import sys
from pathlib import Path

VERSIONS = ["GICE78"]
USERNAME = "Mr. Incredible"
CATEGORY = "game"
MEASURES = [("Code", "matched_code", "total_code"), ("Data", "matched_data", "total_data")]
PCT_WIDTH = len("100.00%")
FILES_WIDTH = len("104/490")


def load(d, ver):
    p = Path(d) / f"{ver}_report" / "report.json"
    if not p.exists():
        return None
    r = json.loads(p.read_text())
    m = next((c["measures"] for c in r.get("categories", []) if c["id"] == CATEGORY), None)
    if m is None:
        return None
    return {k: int(v) for k, v in m.items() if not k.endswith("_percent")}


def pct(m, key, total):
    return 100.0 * m.get(key, 0) / m[total] if m.get(total) else 0.0


def main():
    args = sys.argv[1:]
    cur_dir, prev_dir = args[0], (args[1] if len(args) > 1 else None)
    rows = []
    changed = False
    for ver in VERSIONS:
        cur = load(cur_dir, ver)
        if cur is None:
            continue
        prev = load(prev_dir, ver) if prev_dir else None
        cells = [f"{pct(cur, key, total):.2f}%" for _, key, total in MEASURES]
        files = f"{cur.get('complete_units', 0)}/{cur.get('total_units', 0)}"
        if prev is None:
            changed = True
        else:
            changed |= cells != [f"{pct(prev, key, total):.2f}%" for _, key, total in MEASURES]
            changed |= files != f"{prev.get('complete_units', 0)}/{prev.get('total_units', 0)}"
        rows.append((ver, cells, files))
    if not changed:
        return
    build_width = max(len(ver) for ver, _, _ in rows) + 2
    header = f"{'Build':<{build_width}}" + " ".join(f"  {name:<{PCT_WIDTH - 2}}" for name, _, _ in MEASURES)
    table = [header + "   Files"]
    for ver, cells, files in rows:
        table.append(f"{ver:<{build_width}}" + " ".join(f"{c:>{PCT_WIDTH}}" for c in cells) + "   " + files)
    rule_width = build_width + (PCT_WIDTH + 1) * len(MEASURES) + 2 + FILES_WIDTH
    table.insert(1, "=" * max([rule_width] + [len(line) for line in table]))
    lines = ["**Progress**", "```"] + table + ["```"]
    print(json.dumps({"username": USERNAME, "content": "\n".join(lines)}))


if __name__ == "__main__":
    main()
