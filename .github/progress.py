#!/usr/bin/env python3
"""Print a Discord message (JSON payload) with the Game and SDK progress from objdiff reports.

Usage: progress.py CUR_DIR [PREV_DIR]
Prints nothing when no displayed number changed since PREV_DIR.
Each directory holds GICE78_report/report.json, as downloaded from the Build workflow's artifacts.
"""
import json
import sys
from pathlib import Path

VERSION = "GICE78"
MEASURES = [("Code", "matched_code", "total_code"), ("Data", "matched_data", "total_data")]


CATEGORIES = [("game", "Game"), ("sdk", "SDK")]


def load(d, ver, cat):
    p = Path(d) / f"{ver}_report" / "report.json"
    if not p.exists():
        return None
    r = json.loads(p.read_text())
    m = next((c["measures"] for c in r.get("categories", []) if c["id"] == cat), None)
    if m is None:
        return None
    return {k: int(v) for k, v in m.items() if not k.endswith("_percent")}


def pct(m, key, total):
    return 100.0 * m.get(key, 0) / m[total] if m.get(total) else 0.0


def main():
    args = sys.argv[1:]
    cur_dir, prev_dir = args[0], (args[1] if len(args) > 1 else None)
    lines = ["**Progress**"]
    changed = False
    have_prev = False
    for cat, title in CATEGORIES:
        cur = load(cur_dir, VERSION, cat)
        if cur is None:
            continue
        prev = load(prev_dir, VERSION, cat) if prev_dir else None
        have_prev |= prev is not None
        changed |= prev is None
        parts = []
        for name, key, total in MEASURES:
            part = f"{name} **{pct(cur, key, total):.2f}%**"
            if prev:
                delta = pct(cur, key, total) - pct(prev, key, total)
                changed |= round(delta, 2) != 0
                part += f" ({delta:+.2f}%)"
            parts.append(part)
        units = cur.get("complete_units", 0)
        part = f"Files **{units}/{cur.get('total_units', 0)}**"
        if prev:
            delta = units - prev.get("complete_units", 0)
            changed |= delta != 0
            part += f" ({delta:+d})"
        parts.append(part)
        lines.append(f"- {title}: " + ", ".join(parts))
    if not have_prev:
        lines.append("(no previous report to compare with)")
    if changed:
        print(json.dumps({"username": "Mr. Incredible", "content": "\n".join(lines)}))


if __name__ == "__main__":
    main()
