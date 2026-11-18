"""Execution-only card avoidance; mirrored in graph_layout.js. Coordinates are SVG units."""
from heapq import heappush, heappop
from itertools import count

CLEARANCE = 14
STUB = 24
BEND_COST = 18


def segment_clear(a, b, rects):
    if a[0] == b[0]:
        return not any(l < a[0] < r and max(min(a[1], b[1]), t) < min(max(a[1], b[1]), d)
                       for l, t, r, d in rects)
    return not any(t < a[1] < d and max(min(a[0], b[0]), l) < min(max(a[0], b[0]), r)
                   for l, t, r, d in rects)


def curve_hits(points, rect):
    """Conservative adaptive Bezier/rectangle interior test."""
    l, t, r, b = rect
    xs, ys = zip(*points)
    if max(xs) <= l or min(xs) >= r or max(ys) <= t or min(ys) >= b:
        return False
    if max(max(xs)-min(xs), max(ys)-min(ys)) < .5:
        return True
    a, b, c, d = points
    mid = lambda p, q: ((p[0]+q[0])/2, (p[1]+q[1])/2)
    ab, bc, cd = mid(a, b), mid(b, c), mid(c, d)
    abc, bcd = mid(ab, bc), mid(bc, cd)
    center = mid(abc, bcd)
    return curve_hits((a, ab, abc, center), rect) or curve_hits((center, bcd, cd, d), rect)


def simplify(points):
    result = []
    for point in points:
        if result and point == result[-1]:
            continue
        while len(result) >= 2 and ((result[-2][0] == result[-1][0] == point[0]) or
                                    (result[-2][1] == result[-1][1] == point[1])):
            result.pop()
        result.append(point)
    return result


def orthogonal_route(start, end, rects):
    """Try short local corridors first, then a rectilinear visibility grid."""
    xs = sorted({start[0], end[0], *(v for r in rects for v in (r[0], r[2]))})
    ys = sorted({start[1], end[1], *(v for r in rects for v in (r[1], r[3]))})
    candidates = [[start, (start[0], end[1]), end], [start, (end[0], start[1]), end]]
    candidates += [[start, (start[0], y), (end[0], y), end] for y in ys]
    candidates += [[start, (x, start[1]), (x, end[1]), end] for x in xs]
    # Equal-length corridors must tie identically in Python and JavaScript.
    score = lambda p: round(sum(abs(a[0]-b[0])+abs(a[1]-b[1]) for a, b in zip(p, p[1:])) + BEND_COST*(len(p)-2), 6)
    candidates = [simplify(p) for p in candidates]
    candidates.sort(key=score)
    for points in candidates:
        if all(segment_clear(a, b, rects) for a, b in zip(points, points[1:])):
            return points
    # A* states retain incoming direction so bends have an explicit cost.
    origin = (xs.index(start[0]), ys.index(start[1]), -1)
    target = (xs.index(end[0]), ys.index(end[1]))
    serial, queue, costs, parents = count(), [], {origin: 0}, {}
    heappush(queue, (0, next(serial), 0, origin))
    while queue:
        _, _, cost, state = heappop(queue)
        if cost != costs.get(state):
            continue
        x, y, direction = state
        if (x, y) == target:
            points = []
            while state is not None:
                points.append((xs[state[0]], ys[state[1]]))
                state = parents.get(state)
            return simplify(points[::-1])
        for nx, ny, axis in ((x+1, y, 0), (x, y+1, 1), (x-1, y, 0), (x, y-1, 1)):
            if not (0 <= nx < len(xs) and 0 <= ny < len(ys)):
                continue
            a, b = (xs[x], ys[y]), (xs[nx], ys[ny])
            if not segment_clear(a, b, rects):
                continue
            value = round(cost + abs(a[0]-b[0])+abs(a[1]-b[1]) + (BEND_COST if direction not in (-1, axis) else 0), 6)
            other = (nx, ny, axis)
            if value < costs.get(other, float('inf')):
                costs[other], parents[other] = value, state
                heuristic = abs(b[0]-end[0])+abs(b[1]-end[1])
                heappush(queue, (round(value+heuristic, 6), next(serial), value, other))
    raise ValueError('No free corridor between graph ports')


def rounded_path(points):
    path = f'M{points[0][0]},{points[0][1]}'
    for a, b, c in zip(points, points[1:], points[2:]):
        incoming, outgoing = abs(a[0]-b[0])+abs(a[1]-b[1]), abs(b[0]-c[0])+abs(b[1]-c[1])
        radius = min(6, incoming/2, outgoing/2)
        p = (b[0]+(a[0]-b[0])*radius/incoming, b[1]+(a[1]-b[1])*radius/incoming)
        q = (b[0]+(c[0]-b[0])*radius/outgoing, b[1]+(c[1]-b[1])*radius/outgoing)
        path += f' L{p[0]},{p[1]} Q{b[0]},{b[1]} {q[0]},{q[1]}'
    return path + f' L{points[-1][0]},{points[-1][1]}'


def edge_path(start, end, boxes, source, target, horizontal, execution):
    sx, sy = start
    ex, ey = end
    bend = max(35, ((ex-sx) if horizontal else (ey-sy))/2)
    controls = (start, (sx+bend, sy), (ex-bend, ey), end) if horizontal else (
        start, (sx, sy+bend), (ex, ey-bend), end)
    if not execution:
        if source == target:
            x, y, w, h = boxes[source]
            controls = (start, (sx+40, y+h+55), (ex-40, y+h+55), end) if horizontal else (
                start, (x+w+55, sy+40), (x+w+55, ey-40), end)
        return f'M{sx},{sy} C{controls[1][0]},{controls[1][1]} {controls[2][0]},{controls[2][1]} {ex},{ey}'
    raw = {n: (x, y, x+w, y+h) for n, (x, y, w, h) in boxes.items()}
    rects = {n: (l-CLEARANCE, t-CLEARANCE, r+CLEARANCE, b+CLEARANCE) for n, (l, t, r, b) in raw.items()}
    if source != target and not any(curve_hits(controls, raw[n] if n in (source, target) else r)
                                    for n, r in rects.items()):
        return f'M{sx},{sy} C{controls[1][0]},{controls[1][1]} {controls[2][0]},{controls[2][1]} {ex},{ey}'
    leave = (sx+STUB, sy) if horizontal else (sx, sy+STUB)
    enter = (ex-STUB, ey) if horizontal else (ex, ey-STUB)
    middle = orthogonal_route(leave, enter, list(rects.values()))
    # Preserve port stubs: a back edge must leave/enter on the correct side.
    return rounded_path([start, *middle, end])
