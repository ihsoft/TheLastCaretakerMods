"""Offline regression checks; no Unreal, catalogue, or external packages."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

from render_graph import render

ROOT = Path(__file__).resolve().parent


class GraphRenderingTests(unittest.TestCase):
    def setUp(self):
        self.example = json.loads((ROOT / 'Examples/division.json').read_text())

    def test_four_views_are_deterministic_and_do_not_modify_source(self):
        original = copy.deepcopy(self.example)
        for view in ('simplified', 'full'):
            for direction in ('TB', 'LR'):
                first = render(self.example, view=view, direction=direction)
                self.assertEqual(first, render(self.example, view=view, direction=direction))
                ET.fromstring(first[0])
                self.assertEqual(first[2:], (4, 3))
        self.assertEqual(self.example, original)

    def test_hidden_nodes_do_not_create_bypass_links(self):
        document = {'schema': 1, 'nodes': [
            {'id': 'start', 'kind': 'event'},
            {'id': 'off', 'kind': 'sequence', 'enabled': False},
            {'id': 'self', 'kind': 'self'},
            {'id': 'self.result', 'kind': 'getVariable', 'variable': 'Result'},
        ], 'links': [
            {'from': 'start.then', 'to': 'off.exec'},
            {'from': 'off.then', 'to': 'self.result.exec'},
            {'from': 'self.result', 'to': 'self.result.target'},
        ]}
        simplified = render(document)
        self.assertEqual(simplified[2:], (2, 0))
        self.assertIn('Self (hidden)', simplified[0])
        self.assertIn('disabled off (hidden)', simplified[0])
        self.assertEqual(render(document, view='full')[2:], (4, 3))
        document['links'].append({'from': 'off.then', 'to': 'missing.exec'})
        with self.assertRaisesRegex(ValueError, 'Unresolved'):
            render(document)

    def test_cycles_and_empty_simplified_graph(self):
        document = {'schema': 1, 'nodes': [{'id': 'loop', 'kind': 'sequence'}],
                    'links': [{'from': 'loop.then', 'to': 'loop.exec'}]}
        for direction in ('TB', 'LR'):
            result = render(document, direction=direction)
            ET.fromstring(result[0])
            self.assertEqual(result[2:], (1, 1))
        document['nodes'][0]['enabled'] = False
        self.assertEqual(render(document)[2:], (0, 0))

    def test_labels_are_escaped(self):
        self.example['graphs'][0]['body']['nodes'][0]['comment'] = '<script>alert("x")</script>'
        svg = render(self.example)[0]
        self.assertNotIn('<script>', svg)
        ET.fromstring(svg)

    def test_selection_metadata_preserves_exact_ids(self):
        special_id = 'node."<&'
        document = {'schema': 1, 'nodes': [
            {'id': special_id, 'kind': 'event'}, {'id': 'next', 'kind': 'sequence'}],
            'links': [{'from': special_id + '.then', 'to': 'next.exec'}]}
        svg = ET.fromstring(render(document)[0])
        nodes = svg.findall('.//{*}g[@class="graph-node"]')
        edges = svg.findall('.//{*}g[@class="graph-edge"]')
        self.assertEqual({n.attrib['data-node-id'] for n in nodes}, {special_id, 'next'})
        self.assertEqual([(e.attrib['data-source'], e.attrib['data-target']) for e in edges],
                         [(special_id, 'next')])

    def test_cli_outputs_all_variants_and_preserves_input(self):
        owner = ROOT.parents[1] / 'Tmp/BlueprintTextPrototype/render-tests'
        owner.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='case-', dir=owner) as directory:
            self.assertEqual(Path(directory).resolve().parent, owner.resolve())
            source, output = Path(directory) / 'input.json', Path(directory) / 'graph.svg'
            source.write_text(json.dumps(self.example))
            before = source.read_bytes()
            result = subprocess.run([sys.executable, '-B', str(ROOT / 'render_graph.py'),
                                     str(source), '--output', str(output)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual((report['nodes'], report['links']), (4, 3))
            page = output.with_suffix('.html').read_text(encoding='utf-8')
            self.assertIn('<html lang="en">', page)
            self.assertNotRegex(page, r'[\u0400-\u04ff]')
            for view in ('simplified', 'full'):
                for direction in ('TB', 'LR'):
                    self.assertIn(f'id="{view}-{direction}"', page)
            self.assertNotIn('<script src=', page)
            ET.parse(output)
            ET.parse(output.with_name('graph.full.svg'))
            self.assertEqual(source.read_bytes(), before)
            collision_source = Path(directory) / 'collision.html'
            collision_source.write_bytes(before)
            rejected = subprocess.run([sys.executable, '-B', str(ROOT / 'render_graph.py'),
                                       str(collision_source), '--output', str(collision_source.with_suffix('.svg'))],
                                      capture_output=True, text=True)
            self.assertEqual(rejected.returncode, 2)
            self.assertIn('must not overwrite', rejected.stderr)
            self.assertEqual(collision_source.read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
