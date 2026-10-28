import unittest
from review_details import render_details


class DetailReviewTests(unittest.TestCase):
    def test_fields_nested_markers_and_deleted_position(self):
        before = {'node': {'defaults': {'Min': -1, 'Max': 1, 'Old': True}, 'name': 'Clamp'}}
        after = {'node': {'defaults': {'Min': -1, 'Max': .8, 'New': False}, 'name': 'Clamp'}}
        text = render_details(before, after)
        self.assertIn('Previously: 1', text)
        self.assertIn('detail-removed', text)
        self.assertIn('detail-added', text)
        self.assertLess(text.index('&quot;Old&quot;'), text.index('&quot;New&quot;'))
        self.assertIn('detail-row " title=""><span class="detail-mark" aria-hidden="true"> </span>      &quot;Min&quot;', text)

    def test_key_and_link_order_are_not_changes(self):
        a = {'from': 'a.out', 'to': 'b.in'}
        b = {'from': 'c.out', 'to': 'd.in'}
        text = render_details({'node': {'a': 1, 'b': 2}, 'links': [a,b]},
                              {'links': [b,a], 'node': {'b': 2, 'a': 1}})
        self.assertNotIn('detail-changed', text)
        self.assertNotIn('detail-added', text)
        self.assertNotIn('detail-removed', text)
        text = render_details({'links': [a]}, {'links': [b]})
        self.assertEqual(text.count('detail-removed'), 1)
        self.assertEqual(text.count('detail-added'), 1)

    def test_arrays_atomic_and_technical_fields_neutral(self):
        text = render_details({'node': {'values': [1,2], 'properties': {'NodeGuid': 'a', 'NodePosX': 0}}},
                              {'node': {'values': [3,2], 'properties': {'NodeGuid': 'b', 'NodePosX': 1}}})
        self.assertEqual(text.count('&quot;values&quot;'), 1)
        self.assertIn('[3, 2]', text)
        self.assertEqual(text.count('detail-technical'), 3)  # Properties heading + its fields.

    def test_untrusted_values_and_keys_are_escaped(self):
        text = render_details({'<script>': '<img onerror="x">'}, {'<script>': '<svg onload="x">'})
        self.assertNotIn('<script>', text)
        self.assertNotIn('<img', text)
        self.assertNotIn('<svg', text)
        self.assertIn('&lt;svg', text)
        self.assertIn('&lt;img', text)


if __name__ == '__main__':
    unittest.main()
