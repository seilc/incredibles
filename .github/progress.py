#!/usr/bin/env python3
"""Print a Discord message (JSON payload) with per-version Game progress from objdiff reports.

Usage: progress.py CUR_DIR [PREV_DIR]
One line per version (fuzzy-matched code, objdiff's fuzzy_match_percent, and its change since PREV_DIR; a 🎉 when
it passed a multiple of 10% since PREV_DIR), then, with PREV_DIR, the Game files whose match changed, with the
changed functions of the files not yet complete.
Files come newly matched first, then rising, then falling, each group by bytes changed (biggest first); more than
COLLAPSE functions of one file now at 100.00% become one count line. The message stops after the last whole file
that fits in MAX_CONTENT, with a count of the files left out.
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
VERSIONS = ["GICE78", "GICF78", "GICJ78_20040913", "GICE78_20040912", "GICE78_20040719"]
USERNAME = "Mr. Incredible"
CATEGORY = "game"
ABBREVS = {"DEBUG": "D", "RELEASE": "R", "MASTERDEBUG": "MD", "MASTER": "M"}
MAX_CONTENT = 1000  # kept short (Discord's own limit is 2000)
COLLAPSE = 3  # more functions of one file now at 100.00% than this become one count line


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
    measures = {k: int(v) for k, v in m.items() if not k.endswith("_percent")}
    measures["fuzzy_code"] = round(measures.get("total_code", 0) * float(m.get("fuzzy_match_percent", 0.0)) / 100)
    return measures, units


def pct(m, key, total):
    return 100.0 * m.get(key, 0) / m[total] if m.get(total) else 0.0


def step(name, before, after, prev_size, size):
    """OK's `name -> (+pct%, +bytes) -> pct%`, bytes as the fuzzy-matched share of the code."""
    gained = round(size * after / 100) - round(prev_size * before / 100)
    return f"`{name} -> ({after - before:+.2f}%, {gained:+,}) -> {after:.2f}%`"


def unit_change(p, c):
    """(rank, bytes changed, line, function lines) for a unit whose match changed, else None."""
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
    done = sum(1 for f in funcs if f"{f[2]:.2f}" == "100.00")
    if done > COLLAPSE:
        funcs = [f for f in funcs if f"{f[2]:.2f}" != "100.00"]
    short = [func_name(f[0]) for f in funcs]
    funcs = ["- " + step(s if short.count(s) == 1 else f[0], *f[1:]) for s, f in zip(short, funcs)]
    if done > COLLAPSE:
        funcs.append(f"- {'and ' if funcs else ''}{done} functions now at 100.00%")
    rank = 0 if c["complete"] and not p["complete"] else 2 if p["complete"] else 1 if c["fuzzy"] >= p["fuzzy"] else 2
    gained = round(c["size"] * c["fuzzy"] / 100) - round(p["size"] * p["fuzzy"] / 100)
    return rank, gained, step(c["name"], p["fuzzy"], c["fuzzy"], p["size"], c["size"]) + label, funcs


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
            changed |= cur.get("fuzzy_code", 0) != prev.get("fuzzy_code", 0)
            for key, u in cur_units.items():
                change = unit_change(prev_units.get(key), u)
                if change:
                    changes.setdefault(key, []).append((ver, change))
        rows.append((ver, cur, prev))
    if not changed and not changes:
        return

    lines = []
    for ver, cur, prev in rows:
        stats = f"{cur.get('fuzzy_code', 0):,} / {cur.get('total_code', 0):,}"
        cur_pct, end = pct(cur, "fuzzy_code", "total_code"), "."
        if prev:
            prev_pct = pct(prev, "fuzzy_code", "total_code")
            gained = cur.get("fuzzy_code", 0) - prev.get("fuzzy_code", 0)
            stats = f"({cur_pct - prev_pct:+.2f}% {gained:+,}), " + stats
            if round(cur_pct, 2) // 10 > round(prev_pct, 2) // 10:
                end = " 🎉"
        who = f"{GAME} {ver}" if len(rows) > 1 else GAME
        lines.append(f"{who} is **{cur_pct:.2f}%** decompiled{end} `{stats}`")

    vers = [ver for ver, _, prev in rows if prev]
    items = []
    for key, per_ver in changes.items():
        rank, gained, line, funcs = per_ver[-1][1]  # the latest build with the change
        builds = [ver for ver, _ in per_ver]
        if builds != vers:
            line += f" ({', '.join(ABBREVS.get(b, b) for b in builds)})"
        items.append((rank, -abs(gained), line.lower(), [line] + funcs))
    groups = [group for *_, group in sorted(items)]
    if not groups:
        print(json.dumps({"username": USERNAME, "content": "\n".join(lines)}))
        return
    lines += ["", contributor()]

    def message(shown, more):
        return "\n".join(lines + shown + ([f"... and {more} more file{'s' if more != 1 else ''}"] if more else []))

    shown = []
    for i, group in enumerate(groups):
        more = len(groups) - i - 1
        if len(message(shown + group, more)) <= MAX_CONTENT:
            shown += group
            continue
        if not shown:  # the first file alone is too long: as many of its lines as fit
            for k in range(len(group) - 1, 0, -1):
                part = group[:k] + [f"- and {len(group) - k} more line{'s' if len(group) - k != 1 else ''}"]
                if len(message(part, more)) <= MAX_CONTENT:
                    shown = part
                    break
            if shown:
                break
        more += 1
        break
    else:
        more = 0
    print(json.dumps({"username": USERNAME, "content": message(shown, more)}))


if __name__ == "__main__":
    main()
