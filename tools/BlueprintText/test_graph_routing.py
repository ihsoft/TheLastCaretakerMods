"""Independent sampled-path checks for obstacle routing, without Unreal."""
import math
import re
import unittest

from graph_routing import edge_path, orthogonal_route, rounded_path


def sample_path(path, spacing=2):
    tokens = re.findall(r'[MLCQ]|-?\d+(?:\.\d+)?(?:e[+-]?\d+)?', path, re.I)
    cursor, point = 0, (0, 0)
    while cursor < len(tokens):
        command = tokens[cursor]
        cursor += 1
        count = {'M': 1, 'L': 1, 'Q': 2, 'C': 3}[command]
        controls = [point] + [tuple(map(float, tokens[i:i+2])) for i in range(cursor, cursor+2*count, 2)]
        cursor += 2*count
        if command != 'M':
            length = sum(math.dist(a, b) for a, b in zip(controls, controls[1:]))
            steps = max(1, math.ceil(length/spacing))
            for i in range(steps+1):
                t, points = i/steps, controls
                while len(points) > 1:
                    points = [tuple((1-t)*a[j]+t*b[j] for j in range(2)) for a, b in zip(points, points[1:])]
                yield points[0]
        point = controls[-1]


def crossings(path, boxes):
    return {name for x, y in sample_path(path) for name, (l, t, w, h) in boxes.items()
            if l+1e-6 < x < l+w-1e-6 and t+1e-6 < y < t+h-1e-6}


class RoutingTests(unittest.TestCase):
    def test_branch_bypass_preserves_ports_and_avoids_middle_card(self):
        for horizontal in (False, True):
            boxes = {'branch': (40, 240, 100, 80), 'set': (240, 240, 100, 80), 'join': (440, 240, 100, 80)}
            start, end = (140, 285), (440, 280)
            if not horizontal:
                boxes = {n: (y, x, h, w) for n, (x, y, w, h) in boxes.items()}
                start, end = start[::-1], end[::-1]
            path = edge_path(start, end, boxes, 'branch', 'join', horizontal, True)
            self.assertIn(' Q', path)
            self.assertEqual(crossings(path, boxes), set())
            points = list(sample_path(path))
            self.assertEqual(points[0], start)
            self.assertEqual(points[-1], end)
            data_path = edge_path(start, end, boxes, 'branch', 'join', horizontal, False)
            self.assertIn(' C', data_path)
            self.assertNotIn(' Q', data_path)
            self.assertIn('set', crossings(data_path, boxes), 'Data curves are exempt from card avoidance')

    def test_clear_forward_connection_keeps_original_curve(self):
        boxes = {'a': (40, 240, 100, 80), 'b': (240, 240, 100, 80)}
        self.assertEqual(edge_path((140, 280), (240, 280), boxes, 'a', 'b', True, True),
                         'M140,280 C190.0,280 190.0,280 240,280')

    def test_self_and_back_edges_go_around_endpoint_cards(self):
        for horizontal in (True, False):
            boxes = {'a': (40, 240, 100, 80), 'b': (240, 240, 100, 80)}
            for source, target in [('a', 'a'), ('b', 'a')]:
                a, b = boxes[source], boxes[target]
                start, end = (a[0]+a[2], a[1]+a[3]/2), (b[0], b[1]+b[3]/2)
                transformed = boxes
                if not horizontal:
                    transformed = {n: (y, x, h, w) for n, (x, y, w, h) in boxes.items()}
                    start, end = start[::-1], end[::-1]
                path = edge_path(start, end, transformed, source, target, horizontal, True)
                self.assertEqual(crossings(path, transformed), set())
                data_path = edge_path(start, end, transformed, source, target, horizontal, False)
                self.assertIn(' C', data_path)
                self.assertNotIn(' Q', data_path)

    def test_visibility_grid_routes_between_opposing_pockets(self):
        rects = [(-20,-20,-10,20), (-10,-20,40,-10), (-10,10,40,20),
                 (110,-20,120,20), (60,-20,110,-10), (60,10,110,20), (45,-30,55,30)]
        points = orthogonal_route((0, 0), (100, 0), rects)
        self.assertGreaterEqual(len(points), 6)
        # Grid paths hug the clearance boundary; actual cards are inset by 14.
        cards = {str(i): (l+2,t+2,r-l-4,b-t-4) for i,(l,t,r,b) in enumerate(rects)}
        self.assertEqual(crossings(rounded_path(points), cards), set())


if __name__ == '__main__':
    unittest.main()
