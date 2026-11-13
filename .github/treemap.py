#!/usr/bin/env python3
"""Draw a treemap of the Game units as a PNG, for the Discord progress post.

Usage: treemap.py CUR_DIR OUT.png [PREV_DIR]
Every Game unit of the first version, grouped by folder and sized by code bytes (tiny units raised to a minimum box):
green is complete, grey has nothing matched, and blue is partly matched, brighter the more is matched. With PREV_DIR,
units whose match changed in any version get an outline, or a yellow fill when too small for one. Wide and short, with
folder headings only, to stay readable on a phone without filling the screen.
Each directory holds <VERSION>_report/report.json, as for progress.py. Needs Pillow; progress.py doesn't, so a failure
here only costs the post its image.
"""
import json
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

from progress import CATEGORY, VERSIONS

WIDTH, HEIGHT = 1000, 420
MARGIN = 4
GAP = 2
FOLDER_GAP = 8
FOLDER_FONT = 26
HEADING = FOLDER_FONT + 6
OUTLINE = 4
MIN_BOX = 1  # the smallest unit box side, so that every unit shows as its own box
PHI = (1 + 5 ** 0.5) / 2
SURFACE = (0x1a, 0x1a, 0x19)
INK = (0xf0, 0xef, 0xec)
MATCHED = (0x1f, 0xa8, 0x4f)
PARTIAL = (0x39, 0x87, 0xe5)
UNMATCHED = (0x3c, 0x3c, 0x39)
CHANGED = (0xff, 0xd3, 0x3d)


def load(d, ver):
    """{unit path without the build's prefix: its measures and metadata} of the Game units, or None without a report."""
    path = Path(d) / f"{ver}_report" / "report.json"
    if not path.exists():
        return None
    units = {}
    for unit in json.loads(path.read_text())["units"]:
        if CATEGORY in unit.get("metadata", {}).get("progress_categories", []):
            units[unit["name"].split("/", 1)[-1]] = unit
    return units


def match_state(unit):
    """What progress.py's list of changed units compares."""
    if unit is None:
        return "0.00", False
    return f"{float(unit.get('measures', {}).get('fuzzy_match_percent', 0.0)):.2f}", bool(unit["metadata"].get("complete"))


def changed_units(cur_dir, prev_dir):
    changed = set()
    for ver in VERSIONS if prev_dir else []:
        cur, prev = load(cur_dir, ver), load(prev_dir, ver)
        if cur and prev:
            changed |= {key for key, unit in cur.items() if match_state(unit) != match_state(prev.get(key))}
    return changed


def squarify(values, x0, y0, x1, y1):
    """Rectangles for values sorted large to small: the squarified layout of d3.treemap."""
    boxes, total, i0, n = [], sum(values), 0, len(values)
    while i0 < n:
        dx, dy = x1 - x0, y1 - y0
        i1 = i0 + 1
        row = low = high = values[i0]
        alpha = max(dy / dx, dx / dy) / (total * PHI)
        beta = row * row * alpha
        best = max(high / beta, beta / low)
        while i1 < n:
            value = values[i1]
            beta = (row + value) * (row + value) * alpha
            ratio = max(max(high, value) / beta, beta / min(low, value))
            if ratio > best:
                break
            row, low, high, best, i1 = row + value, min(low, value), max(high, value), ratio, i1 + 1
        if dx < dy:  # a horizontal row across the top
            bottom = y0 + dy * row / total
            x = x0
            for value in values[i0:i1]:
                boxes.append((x, y0, x + dx * value / row, bottom))
                x += dx * value / row
            y0 = bottom
        else:  # a vertical column on the left
            right = x0 + dx * row / total
            y = y0
            for value in values[i0:i1]:
                boxes.append((x0, y, right, y + dy * value / row))
                y += dy * value / row
            x0 = right
        total -= row
        i0 = i1
    return boxes


def folder_boxes(sizes):
    """{folder: rectangle}: a column per top folder, its folders stacked, so that a byte takes the same area everywhere."""
    columns = {}
    for name, size in sizes.items():
        columns.setdefault(name.split("/")[0], {})[name] = size
    width, height = WIDTH - 2 * MARGIN, HEIGHT - 2 * MARGIN
    weights = {top: sum(c.values()) / (height - len(c) * HEADING) for top, c in columns.items()}
    boxes, x = {}, MARGIN
    for top, column in columns.items():
        right = x + width * weights[top] / sum(weights.values())
        y, room = MARGIN, height - len(column) * HEADING
        for name, size in column.items():
            bottom = y + HEADING + room * size / sum(column.values())
            boxes[name] = (x, y, right, bottom)
            y = bottom
        x = right
    return boxes


def unit_boxes(values, x0, y0, x1, y1):
    """Boxes for values sorted large to small, tiny values raised until every box is at least MIN_BOX on a side."""
    width, height = x1 - x0, y1 - y0
    if width <= 0 or height <= 0:
        return []
    cell = (MIN_BOX + GAP) ** 2
    for _ in range(12):  # squarify's last cells can be thin: raise the floor until they fit
        floor = 0
        for _ in range(8):  # the floor grows the total, which lowers the bytes per pixel
            floor = cell * sum(max(v, floor) for v in values) / (width * height)
        boxes = [(round(bx0), round(by0), round(bx1) - GAP - 1, round(by1) - GAP - 1)
                 for bx0, by0, bx1, by1 in squarify([max(v, floor) for v in values], x0, y0, x1, y1)]
        if all(min(r - l, b - u) + 1 >= MIN_BOX for l, u, r, b in boxes):
            break
        cell *= 2
    return [(l, u, max(l, r), max(u, b)) for l, u, r, b in boxes]


def color(unit):
    """Green when complete, else grey to blue by how much is matched, square-rooted so that a small share still shows."""
    if unit["metadata"].get("complete"):
        return MATCHED
    measures = unit["measures"]
    share = min(1, int(measures.get("matched_code", 0)) / int(measures["total_code"])) ** 0.5
    return tuple(round(u + (p - u) * share) for u, p in zip(UNMATCHED, PARTIAL))


def main():
    cur_dir, out, prev_dir = sys.argv[1], sys.argv[2], (sys.argv[3] if len(sys.argv) > 3 else None)
    changed = changed_units(cur_dir, prev_dir)
    groups = {}
    for key, unit in load(cur_dir, VERSIONS[0]).items():
        code = int(unit.get("measures", {}).get("total_code", 0))
        if code:
            parts = key.split("/")[1:]  # without the game's source folder
            folder = "/".join([p for p in parts[:-1] if p != "Engine"][:2]) or "(top)"
            groups.setdefault(folder, []).append((code, key, unit))
    groups = sorted(groups.items(), key=lambda g: -sum(u[0] for u in g[1]))

    image = Image.new("RGB", (WIDTH, HEIGHT), SURFACE)
    draw = ImageDraw.Draw(image)
    font = ImageFont.load_default(size=FOLDER_FONT)

    outlines = []
    folders = folder_boxes({name: sum(u[0] for u in units) for name, units in groups})
    for name, units in groups:
        gx0, gy0, gx1, gy1 = folders[name]
        draw.text((gx0, gy0), name, font=font, fill=INK)
        top, right, bottom = gy0 + HEADING, gx1 - FOLDER_GAP, gy1 - FOLDER_GAP
        units.sort(key=lambda u: -u[0])
        for (_, key, unit), box in zip(units, unit_boxes([u[0] for u in units], gx0, top, right, bottom)):
            draw.rectangle(box, fill=color(unit))
            if key in changed:
                outlines.append(box)
    for box in outlines:  # last, so that no neighbour covers one
        if min(box[2] - box[0], box[3] - box[1]) < 2 * OUTLINE + 1:  # too small for an outline around a 2 px hole
            draw.rectangle(box, fill=CHANGED)
        else:
            draw.rectangle(box, outline=CHANGED, width=OUTLINE)
    image.save(out)


if __name__ == "__main__":
    main()
