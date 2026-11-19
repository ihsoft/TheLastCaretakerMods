"""Fragment packing and pin alignment, independent of card text and Unreal."""
import copy
import unittest

from graph_layout import layout_boxes, port_fractions


class FragmentLayoutTests(unittest.TestCase):
    def test_sibling_inputs_follow_ports_without_crossing_execution(self):
        # Both consumers have two inputs beside the preceding execution step.
        # IDs deliberately oppose input order; card widths/heights vary as they
        # do when expressions or hidden rows change the browser geometry.
        nodes = [dict(id=n, rank=r, w=w, h=h) for n,r,w,h in [
            ('entry',0,330,70),('first',1,330,100),('last',2,330,70),
            ('aTarget',0,350,100),('zValue',0,330,80),
            ('bTarget',1,330,80),('yValue',1,390,100)]]
        edges = [('entry','then','first','exec',True),('first','then','last','exec',True),
                 ('aTarget','value','first','target',False),('zValue','value','first','Value',False),
                 ('bTarget','value','last','target',False),('yValue','value','last','Value',False)]
        original = copy.deepcopy((nodes, edges))
        for direction in ('TB','LR'):
            axis = int(direction == 'LR')
            for columns in (1,8,12):
                for reverse in (False,True):
                    boxes=layout_boxes(nodes[::-1] if reverse else nodes,
                                       edges[::-1] if reverse else edges,direction,columns)
                    for value,target,action in [('zValue','aTarget','entry'),('yValue','bTarget','first')]:
                        self.assertLessEqual(boxes[value][axis]+boxes[value][axis+2], boxes[target][axis])
                        self.assertLessEqual(boxes[target][axis]+boxes[target][axis+2], boxes[action][axis])
        self.assertEqual((nodes,edges),original)

    def test_entry_arguments_do_not_pull_computation_across_execution(self):
        nodes = [dict(id=n, rank=r, w=330, h=100) for n, r in
                 [('entry',0),('first',1),('second',2),('branch',3),('result',4),('length',1),('compare',2)]]
        edges = [('entry','then','first','exec',True),('first','then','second','exec',True),
                 ('second','then','branch','exec',True),('branch','then','result','exec',True),
                 ('entry','Start','length','left',False),('entry','End','length','right',False),
                 ('length','value','compare','value',False),('compare','value','branch','condition',False),
                 ('length','value','result','distance',False)]
        original = copy.deepcopy((nodes, edges))
        for direction in ('TB','LR'):
            axis = int(direction == 'LR')
            for width in (1,8,12):
                boxes = layout_boxes(nodes,edges,direction,width)
                self.assertLessEqual(boxes['length'][axis]+boxes['length'][axis+2], boxes['first'][axis])
                self.assertLessEqual(boxes['compare'][axis]+boxes['compare'][axis+2], boxes['second'][axis])
                self.assertEqual(len(boxes),len(nodes),'Shared length remains one card')
        self.assertEqual((nodes, edges),original)

    def test_linear_pure_computation_moves_as_one_pin_aligned_group(self):
        nodes = [dict(id=n, rank=r, w=330, h=100) for n, r in
                 [('entry',0),('first',1),('second',2),('branch',3),('subtract',1),('length',2)]]
        edges = [('entry','then','first','exec',True),('first','then','second','exec',True),
                 ('second','then','branch','exec',True),('entry','Start','subtract','left',False),
                 ('entry','End','subtract','right',False),('subtract','value','length','value',False),
                 ('length','value','branch','condition',False)]
        outputs, inputs = port_fractions(nodes,edges)
        for direction in ('TB','LR'):
            axis = int(direction == 'LR')
            boxes = layout_boxes(nodes,edges,direction)
            a,b = boxes['subtract'],boxes['length']
            self.assertAlmostEqual(a[axis]+a[axis+2]*outputs['subtract']['value'],
                                   b[axis]+b[axis+2]*inputs['length']['value'])
            self.assertLessEqual(a[axis]+a[axis+2],boxes['first'][axis])
            self.assertLessEqual(b[axis]+b[axis+2],boxes['second'][axis])

    def test_disconnected_pairs_remain_adjacent_at_every_width(self):
        nodes, edges = [], []
        for i in range(13):
            nodes += [dict(id=f'event{i}', rank=0, w=330, h=70+i),
                      dict(id=f'call{i}', rank=1, w=330, h=170-i)]
            edges.append((f'event{i}', 'then', f'call{i}', 'exec', True))
        original = copy.deepcopy((nodes, edges))
        for direction in ('TB', 'LR'):
            narrow = None
            for width in (1, 3, 8, 12):
                boxes = layout_boxes(nodes, edges, direction, width)
                for a, _, b, _, _ in edges:
                    ax, ay, aw, ah = boxes[a]
                    bx, by, bw, bh = boxes[b]
                    if direction == 'TB':
                        self.assertAlmostEqual(ax+aw/2, bx+bw/2)
                        self.assertEqual(by-ay-ah, 110)
                    else:
                        self.assertAlmostEqual(ay+ah/2, by+bh/2)
                        self.assertEqual(bx-ax-aw, 170)
                if width == 1:
                    narrow = boxes
                if width == 12:
                    self.assertNotEqual(narrow, boxes)
        self.assertEqual((nodes, edges), original)

    def test_data_ports_and_unequal_sizes_preserve_execution_spine(self):
        nodes = [dict(id='event', rank=0, w=330, h=160),
                 dict(id='action', rank=1, w=460, h=90),
                 dict(id='result', rank=2, w=330, h=110)]
        edges = [('event', 'value', 'action', 'value', False),
                 ('event', 'then', 'action', 'exec', True),
                 ('action', 'then', 'result', 'exec', True)]
        outputs, inputs = port_fractions(nodes, edges)
        for direction in ('TB', 'LR'):
            boxes = layout_boxes(nodes, edges, direction, 8)
            for a, out, b, inp, execution in edges:
                if execution:
                    axis = int(direction == 'LR')
                    self.assertAlmostEqual(boxes[a][axis] + boxes[a][axis+2]*outputs[a][out],
                                           boxes[b][axis] + boxes[b][axis+2]*inputs[b][inp])

    def test_shared_input_pin_has_one_endpoint_and_cycles_keep_all_nodes(self):
        nodes = [dict(id=n, rank=rank, w=330, h=100) for n, rank in
                 [('root', 0), ('true', 1), ('false', 1), ('join', 2), ('loop', 2)]]
        edges = [('root', 'then', 'true', 'exec', True), ('root', 'else', 'false', 'exec', True),
                 ('true', 'then', 'join', 'exec', True), ('false', 'then', 'join', 'exec', True),
                 ('join', 'then', 'loop', 'exec', True), ('loop', 'then', 'join', 'exec', True)]
        outputs, inputs = port_fractions(nodes, edges)
        self.assertEqual(inputs['join'], {'exec': 0.5})
        for direction in ('TB', 'LR'):
            boxes = layout_boxes(nodes, edges, direction, 1)
            self.assertEqual(set(boxes), {n['id'] for n in nodes})
            self.assertLess(boxes['true'][int(direction == 'LR')], boxes['false'][int(direction == 'LR')])
            for i, a in enumerate(boxes.values()):
                for b in list(boxes.values())[i+1:]:
                    self.assertTrue(a[0]+a[2] <= b[0] or b[0]+b[2] <= a[0] or
                                    a[1]+a[3] <= b[1] or b[1]+b[3] <= a[1])


if __name__ == '__main__':
    unittest.main()
