#!/usr/bin/env python3
"""Parse kscreen-doctor output into dynamic output rectangles."""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from typing import Iterable


ANSI_ESCAPE = re.compile(r"\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])")


@dataclass(frozen=True)
class OutputGeometry:
    name: str
    x: int
    y: int
    width: int
    height: int

    @property
    def right(self) -> int:
        return self.x + self.width

    @property
    def bottom(self) -> int:
        return self.y + self.height


def parse_outputs(text: str) -> list[OutputGeometry]:
    outputs: list[OutputGeometry] = []
    current_name: str | None = None
    for raw_line in text.splitlines():
        line = ANSI_ESCAPE.sub("", raw_line)
        output_match = re.match(r"^Output:\s+\d+\s+(\S+)", line)
        if output_match:
            current_name = output_match.group(1)
            continue
        geometry_match = re.search(
            r"Geometry:\s*(-?\d+),(-?\d+)\s+(\d+)x(\d+)", line
        )
        if geometry_match and current_name:
            x, y, width, height = map(int, geometry_match.groups())
            if width > 0 and height > 0:
                outputs.append(OutputGeometry(current_name, x, y, width, height))
            current_name = None
    return outputs


def virtual_bounds(outputs: Iterable[OutputGeometry]) -> tuple[int, int, int, int]:
    items = list(outputs)
    if not items:
        raise ValueError("no enabled output geometries found")
    left = min(item.x for item in items)
    top = min(item.y for item in items)
    right = max(item.right for item in items)
    bottom = max(item.bottom for item in items)
    return left, top, right - left, bottom - top


def main() -> int:
    outputs = parse_outputs(sys.stdin.read())
    if not outputs:
        print("No enabled output geometries found.", file=sys.stderr)
        return 1
    left, top, width, height = virtual_bounds(outputs)
    print("=== KDE output topology ===")
    for output in outputs:
        print(
            f"{output.name}: x={output.x}, y={output.y}, "
            f"size={output.width}x{output.height}, "
            f"right={output.right}, bottom={output.bottom}"
        )
    print(f"Virtual bounds: x={left}, y={top}, size={width}x{height}")
    print(f"Output count: {len(outputs)}")
    print("Reference geometry: runtime-derived from the active Plasma outputs")
    print("Enable the Multiscreen and Diagnostics settings to inspect global seams.")
    print("The diagnostic grid must remain continuous across every output boundary.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
