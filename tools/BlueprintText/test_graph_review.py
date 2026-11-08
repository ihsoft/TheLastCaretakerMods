"""Automatic review marks without agent-authored annotation lists."""
import copy
import unittest

from graph_review import review_marks, graph_signature


def fixture():
    return dict(schema=2, classes={'get': '/Script/BlueprintGraph.K2Node_VariableGet'},
                pinTemplates={'vector': {'PinType.PinCategory': 'struct'}}, graphs=[dict(
                    name='Main', body=dict(nodes=[
                        dict(id='a', kind='get', pins={'out:Value': {'type': 'vector', 'id': 'pin'}},
                             defaults={'out:Value': '0,0,0'}),
                        dict(id='b', kind='call', pins={}), dict(id='c', kind='call', pins={})],
                        links=[]))])


class GraphReviewTests(unittest.TestCase):
    def setUp(self):
        self.old = fixture()
        self.old['graphs'][0]['body']['links'] = [{'from': 'a.out:Value', 'to': 'b.in:Value'}]

    def test_payload_and_link_changes_mark_only_affected_nodes(self):
        new = copy.deepcopy(self.old)
        new['graphs'][0]['body']['nodes'][0]['defaults']['out:Value'] = '1,0,0'
        self.assertEqual(review_marks(self.old, new)['Main']['changed'], ['a'])
        new = copy.deepcopy(self.old)
        new['graphs'][0]['body']['links'][0]['to'] = 'c.in:Value'
        self.assertEqual(review_marks(self.old, new)['Main']['changed'], ['a', 'b', 'c'])

    def test_table_alias_and_order_changes_are_not_edits(self):
        new = copy.deepcopy(self.old)
        new['classes']['renamed'] = new['classes'].pop('get')
        new['pinTemplates']['renamed'] = new['pinTemplates'].pop('vector')
        node = new['graphs'][0]['body']['nodes'][0]
        node['kind'] = 'renamed'
        node['pins']['out:Value']['type'] = 'renamed'
        new['graphs'][0]['body']['nodes'].reverse()
        self.assertEqual(review_marks(self.old, new), {})
        self.assertEqual(graph_signature(self.old, self.old['graphs'][0]),
                         graph_signature(new, new['graphs'][0]))

    def test_changed_pin_definition_is_detected_without_node_payload_edit(self):
        new = copy.deepcopy(self.old)
        new['pinTemplates']['vector']['PinType.PinCategory'] = 'object'
        self.assertEqual(review_marks(self.old, new)['Main']['changed'], ['a'])

    def test_added_removed_and_reused_identity(self):
        new = copy.deepcopy(self.old)
        new['graphs'][0]['body']['nodes'][1]['id'] = 'new'
        new['graphs'][0]['body']['links'][0]['to'] = 'new.in:Value'
        self.assertEqual(review_marks(self.old, new)['Main'],
                         dict(added=['new'], removed=['b'], changed=['a']))


if __name__ == '__main__':
    unittest.main()
