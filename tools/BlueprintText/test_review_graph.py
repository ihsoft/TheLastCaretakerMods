"""Review overlays are explicit, non-destructive and retain deleted source state."""
import copy
import json
import unittest
import xml.etree.ElementTree as ET

from render_graph import render


class ReviewTests(unittest.TestCase):
    def setUp(self):
        self.before = {'schema': 2, 'graph': 'Example', 'asset': '/Game/Example',
                       'pinTemplates': {'value': {'PinType.PinCategory': '"bool"'}},
                       'nodes': [
                           {'id': 'old', 'kind': 'getVariable', 'variable': 'Flag',
                            'pins': {'out:result': {'type': 'value'}}},
                           {'id': 'sink', 'kind': 'call', 'operation': 'Consume',
                            'pins': {'in:value': {'type': 'value'}}}],
                       'links': [{'from': 'old.out:result', 'to': 'sink.in:value'}]}
        self.current = copy.deepcopy(self.before)
        self.current['nodes'][0]['id'] = 'new'
        self.current['nodes'][0]['variable'] = 'OtherFlag'
        self.current['links'][0]['from'] = 'new.out:result'
        # The two exports may independently assign the same template key.
        self.current['pinTemplates']['value'] = {'PinType.PinCategory': '"int"'}
        self.marks = {'added': ['new'], 'changed': ['sink'], 'removed': ['old']}

    def test_overlay_preserves_sources_deleted_types_and_before_after_links(self):
        originals = copy.deepcopy((self.current, self.before, self.marks))
        for view in ('full', 'simplified'):
            for direction in ('TB', 'LR'):
                svg = ET.fromstring(render(self.current, view=view, direction=direction,
                                           review=self.marks, before=self.before)[0])
                nodes = {n.attrib['data-node-id']: n for n in svg.findall('.//{*}g[@class="graph-node"]')}
                self.assertEqual(set(nodes), {'old', 'new', 'sink'})
                self.assertEqual(nodes['old'].attrib['data-review-status'], 'removed')
                details = json.loads(nodes['sink'].find('{*}title').text)
                self.assertEqual(details['Before']['links'][0]['from'], 'old.result')
                self.assertEqual(details['After']['links'][0]['from'], 'new.result')
                old_edge = next(e for e in svg.findall('.//{*}g[@class="graph-edge"]')
                                if e.attrib['data-source'] == 'old')
                self.assertIn('Removed connection (Before)', old_edge.find('{*}title').text)
                self.assertIn('result (bool)', old_edge.find('{*}title').text)
                self.assertIn('stroke-dasharray', old_edge.find('{*}path').attrib)
        self.assertEqual((self.current, self.before, self.marks), originals)

    def test_bad_marks_fail_instead_of_silently_hiding_review(self):
        for marks in ({'added': ['typo']}, {'removed': ['sink']}, {'changed': ['new']},
                      {'added': ['new', 'new']}, {'added': ['new'], 'changed': ['new']},
                      {'changed': 'sink'}, {'unexpected': []}):
            with self.subTest(marks=marks), self.assertRaises(ValueError):
                render(self.current, review=marks, before=self.before)
        with self.assertRaisesRegex(ValueError, 'require --before'):
            render(self.current, review={'changed': ['sink']})
        wrong = dict(self.before, asset='/Game/Other')
        with self.assertRaisesRegex(ValueError, 'same asset and graph'):
            render(self.current, review=self.marks, before=wrong)

    def test_marked_disabled_self_stays_visible_and_unfolded(self):
        document = {'schema': 1, 'nodes': [{'id': 'self', 'kind': 'self', 'enabled': False}], 'links': []}
        self.assertEqual(render(document)[2], 0)
        result = render(document, review={'added': ['self']})
        self.assertEqual(result[2], 1)
        self.assertIn('+ Added', result[0])

    def test_marked_data_node_breaks_folding(self):
        document = {'schema': 2, 'pinTemplates': {'b': {'PinType.PinCategory': '"bool"'}},
                    'nodes': [{'id': 'get', 'kind': 'getVariable', 'variable': 'Flag',
                               'pins': {'out:result': {'type': 'b'}}},
                              {'id': 'not', 'kind': 'call', 'properties': {'bDefaultsToPureFunc': 'True',
                               'FunctionReference': '(MemberName="Not_PreBool")'},
                               'pins': {'in:A': {'type': 'b'}, 'out:result': {'type': 'b'}}}],
                    'links': [{'from': 'get.out:result', 'to': 'not.in:A'}]}
        self.assertEqual(render(document)[2], 1)
        self.assertEqual(render(document, review={'changed': ['get']}, before=document)[2], 2)

    def test_changed_field_highlight_is_local_and_keeps_previous_value(self):
        before = {'schema': 1, 'nodes': [{'id': 'clamp', 'kind': 'call', 'operation': 'Clamp',
                  'defaults': {'in:Min': '-1', 'in:Max': '0.9999'}}], 'links': []}
        after = copy.deepcopy(before)
        after['nodes'][0]['defaults']['in:Max'] = '0.8'
        for view in ('simplified', 'full'):
            for direction in ('TB', 'LR'):
                svg = ET.fromstring(render(after, view=view, direction=direction,
                    review={'changed': ['clamp']}, before=before)[0])
                changed = svg.findall('.//{*}text[@class="review-changed-line"]')
                self.assertEqual([n.text for n in changed], ['Input: Max = "0.8"'])
                self.assertEqual(changed[0].find('{*}title').text, 'Previously: Input: Max = "0.9999"')

    def test_wrapping_and_field_order_do_not_create_false_changes(self):
        before = {'schema': 1, 'nodes': [{'id': 'n', 'kind': 'call', 'operation': 'Example',
                  'defaults': {'in:LongParameterNameWithWrapping': 'old', 'in:Z': 'unchanged'}}], 'links': []}
        after = copy.deepcopy(before)
        after['nodes'][0]['defaults']['in:LongParameterNameWithWrapping'] = 'a longer value ' * 8
        after['nodes'][0]['defaults']['in:A'] = 'new'
        svg = ET.fromstring(render(after, review={'changed': ['n']}, before=before)[0])
        changed = svg.findall('.//{*}text[@class="review-changed-line"]')
        self.assertGreater(len(changed), 3)
        self.assertFalse(any('unchanged' in n.text for n in changed))
        self.assertTrue(all(n.find('{*}title').text == 'Previously: Input: LongParameterNameWithWrapping = "old"'
                            for n in changed if n.text != 'Input: A = "new"'))

    def test_link_only_change_does_not_highlight_newly_visible_default(self):
        before = copy.deepcopy(self.before)
        before['nodes'][1]['defaults'] = {'in:value': 'false'}
        after = copy.deepcopy(before)
        after['links'] = []
        svg = ET.fromstring(render(after, review={'changed': ['old', 'sink']}, before=before)[0])
        self.assertFalse(svg.findall('.//{*}text[@class="review-changed-line"]'))

    def test_output_type_change_uses_each_versions_pin_templates(self):
        after = copy.deepcopy(self.before)
        for d in (after, self.before):
            d['nodes'][0].update(kind='call', operation='ReadFlag')
        after['pinTemplates']['value'] = {'PinType.PinCategory': '"int"'}
        svg = ET.fromstring(render(after, review={'changed': ['old']}, before=self.before)[0])
        changed = svg.findall('.//{*}text[@class="review-changed-line"]')
        self.assertEqual([n.text for n in changed], ['Output: int'])
        self.assertEqual(changed[0].find('{*}title').text, 'Previously: Output: bool')


if __name__ == '__main__':
    unittest.main()
