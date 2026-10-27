#!/usr/bin/env python3
"""Print a Discord message (JSON payload) with per-version Game progress from objdiff reports.

Usage: progress.py CUR_DIR [PREV_DIR]
One line per version (matched code, and its change since PREV_DIR), then, with PREV_DIR, the Game files whose
match changed, with the changed functions of the files not yet complete.
Optional environment, for the line naming who pushed what: PUSHER (GitHub login), REPO (owner/name), HEAD_SHA,
PREV_SHA, COMMITS (the number of commits pushed).
Prints nothing when nothing changed since PREV_DIR.
Each directory holds <VERSION>_report/report.json, as downloaded from the Build workflow's artifacts.
"""
import json
import os
import sys
from pathlib import Path

GAME = "Incredibles"
VERSIONS = ["GICE78"]
USERNAME = "Mr. Incredible"
CATEGORY = "game"
ABBREVS = {"DEBUG": "D", "RELEASE": "R", "MASTERDEBUG": "MD", "MASTER": "M"}
MAX_CONTENT = 2000  # Discord's message limit


def func_name(name):
    """A demangled name without its argument list."""
    if name.endswith(" const"):
        name = name[:-len(" const")]
    if not name.endswith(")"):
        return name
    depth = 0
    for i in range(len(name) - 1, -1, -1):
        depth += {")": 1, "(": -1}.get(name[i], 0)
        if depth == 0:
            return name[:i]
    return name


def load(d, ver):
    p = Path(d) / f"{ver}_report" / "report.json"
    if not p.exists():
        return None
    r = json.loads(p.read_text())
    m = next((c["measures"] for c in r.get("categories", []) if c["id"] == CATEGORY), None)
    if m is None:
        return None
    units = {}
    for u in r.get("units", []):
        meta = u.get("metadata", {})
        if CATEGORY not in meta.get("progress_categories", []):
            continue
        key = u["name"].split("/", 1)[-1]  # without the build's prefix (ratsgc_m/)
        um = u.get("measures", {})
        units[key] = {"name": Path(meta.get("source_path") or key).name, "complete": bool(meta.get("complete")),
                      "fuzzy": float(um.get("fuzzy_match_percent", 0.0)), "size": int(um.get("total_code", 0)),
                      "functions": int(um.get("total_functions", 0)),
                      "funcs": {f["name"]: (f.get("metadata", {}).get("demangled_name") or f["name"],
                                            float(f.get("fuzzy_match_percent", 0.0)), int(f["size"]))
                                for f in u.get("functions", [])}}
    return {k: int(v) for k, v in m.items() if not k.endswith("_percent")}, units


def pct(m, key, total):
    return 100.0 * m.get(key, 0) / m[total] if m.get(total) else 0.0


def step(name, before, after, prev_size, size):
    """OK's `name -> (+pct%, +bytes) -> pct%`, bytes as the fuzzy-matched share of the code."""
    gained = round(size * after / 100) - round(prev_size * before / 100)
    return f"`{name} -> ({after - before:+.2f}%, {gained:+,}) -> {after:.2f}%`"


def unit_change(p, c):
    """(rank, line, function lines) for a unit whose match changed, else None."""
    p = p or {"fuzzy": 0.0, "size": c["size"], "complete": False, "funcs": {}}
    label = f" matched, {c['functions']} function{'s' if c['functions'] != 1 else ''}" if c["complete"] and not p["complete"] else \
        " unmatched" if p["complete"] and not c["complete"] else ""
    if f"{p['fuzzy']:.2f}" == f"{c['fuzzy']:.2f}" and not label:
        return None
    funcs = []
    if not c["complete"]:
        for key, (name, after, size) in c["funcs"].items():
            _, before, prev_size = p["funcs"].get(key, (name, 0.0, size))
            if f"{before:.2f}" != f"{after:.2f}":
                funcs.append((name, before, after, prev_size, size))
    short = [func_name(f[0]) for f in funcs]
    funcs = ["- " + step(s if short.count(s) == 1 else f[0], *f[1:]) for s, f in zip(short, funcs)]
    rank = 0 if c["complete"] and not p["complete"] else 2 if p["complete"] else 1 if c["fuzzy"] >= p["fuzzy"] else 2
    return rank, step(c["name"], p["fuzzy"], c["fuzzy"], p["size"], c["size"]) + label, funcs


def contributor():
    env = os.environ.get
    repo, head, prev, pusher = env("REPO"), env("HEAD_SHA"), env("PREV_SHA"), env("PUSHER")
    if not (repo and head):
        return "Changes:"
    url = f"https://github.com/{repo}"
    commits = int(env("COMMITS")) if (env("COMMITS") or "").isdigit() else 0
    if prev and commits != 1:
        ref = f"[{prev[:7]}..{head[:7]}](<{url}/compare/{prev}...{head}>)" + (f" ({commits} commits)" if commits else "")
    else:
        ref = f"[{head[:7]}](<{url}/commit/{head}>)"
    if not pusher:
        return f"Changes in {ref}:"
    return f"[{pusher}](<https://github.com/{pusher}>) just contributed the following in {ref}:"


def main():
    args = sys.argv[1:]
    cur_dir, prev_dir = args[0], (args[1] if len(args) > 1 else None)
    rows = []
    changes = {}
    changed = False
    for ver in VERSIONS:
        cur = load(cur_dir, ver)
        if cur is None:
            continue
        cur, cur_units = cur
        prev = load(prev_dir, ver) if prev_dir else None
        if prev is None:
            changed = True
        else:
            prev, prev_units = prev
            changed |= cur.get("matched_code", 0) != prev.get("matched_code", 0)
            for key, u in cur_units.items():
                change = unit_change(prev_units.get(key), u)
                if change:
                    changes.setdefault(key, []).append((ver, change))
        rows.append((ver, cur, prev))
    if not changed and not changes:
        return

    lines = []
    for ver, cur, prev in rows:
        stats = f"{cur.get('matched_code', 0):,} / {cur.get('total_code', 0):,}"
        if prev:
            gained = cur.get("matched_code", 0) - prev.get("matched_code", 0)
            stats = f"({pct(cur, 'matched_code', 'total_code') - pct(prev, 'matched_code', 'total_code'):+.2f}% {gained:+,}), " + stats
        who = f"{GAME} {ver}" if len(rows) > 1 else GAME
        lines.append(f"{who} is **{pct(cur, 'matched_code', 'total_code'):.2f}%** decompiled. `{stats}`")

    vers = [ver for ver, _, prev in rows if prev]
    items = []
    for key, per_ver in changes.items():
        rank, line, funcs = per_ver[-1][1]  # the latest build with the change
        builds = [ver for ver, _ in per_ver]
        if builds != vers:
            line += f" ({', '.join(ABBREVS.get(b, b) for b in builds)})"
        items.append((rank, line.lower(), [line] + funcs))
    item_lines = [line for _, _, group in sorted(items) for line in group]
    if not item_lines:
        print(json.dumps({"username": USERNAME, "content": "\n".join(lines)}))
        return
    lines += ["", contributor()]
    shown, more = item_lines, 0
    while True:
        content = "\n".join(lines + shown + ([f"... and {more} more"] if more else []))
        if len(content) <= MAX_CONTENT or not shown:
            break
        shown, more = shown[:-1], more + 1
    print(json.dumps({"username": USERNAME, "content": content}))


if __name__ == "__main__":
    main()
