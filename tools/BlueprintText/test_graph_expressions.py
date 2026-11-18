"""Expression presentation preserves operand identity, structure and review boundaries."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

from graph_expressions import fold_expressions, format_expression, flat_expression
from render_graph import load_graph, render, edge_pin_details


TYPES = {'double': {'PinType.PinCategory': '"real"', 'PinType.PinSubCategory': '"double"'},
         'float': {'PinType.PinCategory': '"real"', 'PinType.PinSubCategory': '"float"'},
         'Vector': {'PinType.PinCategory': '"struct"',
                    'PinType.PinSubCategoryObject': '"/Script/CoreUObject.ScriptStruct\'/Script/CoreUObject.Vector\'"'},
         'bool': {'PinType.PinCategory': '"bool"'}, 'exec': {'PinType.PinCategory': '"exec"'}}


def read(nid, name, kind='double'):
    return dict(id=nid, kind='getVariable', variable=name, pins={'out:result': {'type': kind}})


def call(nid, member, args, result='double', defaults=None):
    return dict(id=nid, kind='call', properties={
        'FunctionReference': '(MemberParent="/Script/CoreUObject.Class\'/Script/Engine.KismetMathLibrary\'",MemberName="'+member+'")',
        'bDefaultsToPureFunc': 'True'}, pins={**{'in:'+name: {'type': kind} for name,kind in args},
                                            'out:result': {'type': result}}, defaults=defaults or {})


def fixture():
    return dict(schema=2, pinTemplates=copy.deepcopy(TYPES), nodes=[
        read('rate', 'Deceleration'), read('dt', 'DeltaSeconds'),
        call('sub', 'Subtract_DoubleDouble', [('A','double'),('B','double')], defaults={'in:B':'0.5'}),
        call('multiply', 'Multiply_DoubleDouble', [('A','double'),('B','double')]),
        call('compare', 'Greater_DoubleDouble', [('A','double'),('B','double')], 'bool', {'in:B':'0.1'}),
        dict(id='exit', kind='Tunnel', properties={'bCanHaveInputs':'True'}, pins={'in:Ready': {'type':'bool'}})],
        links=[{'from': a, 'to': b} for a,b in [
            ('rate.out:result','sub.in:A'), ('sub.out:result','multiply.in:A'),
            ('dt.out:result','multiply.in:B'), ('multiply.out:result','compare.in:A'),
            ('compare.out:result','exit.in:Ready')]])


def folded(doc, **kwargs):
    nodes,edges,_,_ = load_graph(doc)
    return fold_expressions(nodes, edges, edges, doc['pinTemplates'], **kwargs)


def visible_text(element):
    # SVG labels contain styled tspans and optional non-visible title tooltips.
    return (element.text or '') + ''.join(
        (''.join(child.itertext()) if child.tag.rsplit('}', 1)[-1] != 'title' else '')
        + (child.tail or '') for child in element)


class ExpressionTests(unittest.TestCase):
    def test_vector_operations_preserve_types_operand_order_and_literals(self):
        cases = [
            ('MakeVector', [('X','double'),('Y','double'),('Z','double')], 'Vector',
             {'in:X':'0.0','in:Y':'-2','in:Z':'3e-1'}, 'Vector(0.0, -2, 3e-1)'),
            ('SelectVector', [('A','Vector'),('B','Vector'),('bPickA','bool')], 'Vector',
             {'in:A':'1, 2, 3','in:B':'-4, 5, 6','in:bPickA':'true'},
             'select(true, true: Vector(1, 2, 3), false: Vector(-4, 5, 6))'),
            ('Dot_VectorVector', [('A','Vector'),('B','Vector')], 'double',
             {'in:A':'1, 2, 3','in:B':'4, 5, 6'}, 'dot(Vector(1, 2, 3), Vector(4, 5, 6))'),
            ('Add_VectorVector', [('A','Vector'),('B','Vector')], 'Vector',
             {'in:A':'1, 2, 3','in:B':'4, 5, 6'}, '(Vector(1, 2, 3) + Vector(4, 5, 6))'),
            ('Subtract_VectorVector', [('A','Vector'),('B','Vector')], 'Vector',
             {'in:A':'1, 2, 3','in:B':'4, 5, 6'}, '(Vector(1, 2, 3) - Vector(4, 5, 6))'),
            ('Multiply_VectorFloat', [('A','Vector'),('B','double')], 'Vector',
             {'in:A':'1, 2, 3','in:B':'2.0'}, '(Vector(1, 2, 3) * 2.0)'),
            ('VInterpTo_Constant', [('Current','Vector'),('Target','Vector'),('DeltaTime','float'),('InterpSpeed','float')],
             'Vector', {'in:Current':'1, 2, 3','in:Target':'0, 0, 0','in:DeltaTime':'0.1','in:InterpSpeed':'2'},
             'VInterpTo_Constant(current: Vector(1, 2, 3), target: Vector(0, 0, 0), deltaTime: 0.1, interpSpeed: 2)'),
        ]
        for name, args, result, defaults, expected in cases:
            with self.subTest(name=name):
                doc=dict(schema=2,pinTemplates=copy.deepcopy(TYPES),
                         nodes=[call('op',name,args,result,defaults)],links=[])
                original=copy.deepcopy(doc)
                group=folded(doc)[0][0]
                self.assertEqual(group['_formula'],expected)
                self.assertEqual(doc,original)
                lines=format_expression(group['_formulaTree'],width=32)
                self.assertEqual(''.join(''.join(lines).removeprefix('Output:').split()),
                                 ''.join(expected.split()))
                for direction in ('TB','LR'):
                    self.assertEqual(render(doc,view='expressions',direction=direction)[2],1)

    @staticmethod
    def shared_vector_fixture():
        return dict(schema=2,pinTemplates=copy.deepcopy(TYPES),nodes=[
            call('zero','MakeVector',[('X','double'),('Y','double'),('Z','double')],
                 'Vector',{'in:X':'0','in:Y':'0','in:Z':'0'}),
            read('velocity','Velocity','Vector'),
            call('dot','Dot_VectorVector',[('A','Vector'),('B','Vector')]),
            call('add','Add_VectorVector',[('A','Vector'),('B','Vector')],'Vector')],
            links=[{'from':a,'to':b} for a,b in [('zero.out:result','dot.in:A'),
                ('zero.out:result','add.in:A'),('velocity.out:result','dot.in:B'),
                ('velocity.out:result','add.in:B')]])

    def test_shared_literal_constructor_inlines_per_use_without_losing_details(self):
        doc=self.shared_vector_fixture();original=copy.deepcopy(doc)
        nodes,edges,count=folded(doc)
        self.assertEqual(count,1)
        self.assertEqual({n['id'] for n in nodes},{'velocity','dot','add'})
        self.assertEqual(len(edges),2)
        for node in nodes:
            if '_formula' not in node:continue
            self.assertIn('Vector(0, 0, 0)',node['_formula'])
            self.assertEqual([p['name'] for p in node['_formulaInputs']],['Velocity'])
            self.assertEqual(next(p for p in node['_foldedNodes'] if p['id']=='zero'),doc['nodes'][0])
            self.assertEqual(len(node['_foldedLinks']),1)
            self.assertEqual(len(node['_boundaryLinks']),1)
        self.assertEqual(doc,original)
        self.assertEqual(render(doc,view='full')[2:4],(4,4))
        for direction in ('TB','LR'):
            svg=ET.fromstring(render(doc,view='expressions',direction=direction)[0])
            self.assertIsNone(svg.find('.//{*}g[@data-node-id="zero"]'))
            for nid in ('dot','add'):
                card=svg.find('.//{*}g[@data-node-id="'+nid+'"]')
                self.assertIn('zero',json.loads(card.get('data-node-ids')))
                self.assertEqual(json.loads(card.find('{*}title').text)['_foldedNodes'][-1]['id'],'zero')
        # A remaining unsupported consumer still needs the original constructor card.
        doc['nodes'].append(dict(id='exit',kind='FunctionResult',pins={'in:V':{'type':'Vector'}}))
        doc['links'].append({'from':'zero.out:result','to':'exit.in:V'})
        nodes,edges,_=folded(doc)
        self.assertIn('zero',{n['id'] for n in nodes})
        self.assertEqual([e[2] for e in edges if e[0]=='zero'],['exit'])

    def test_computed_vectors_review_boundaries_and_invalid_vectors_stay_explicit(self):
        doc=self.shared_vector_fixture()
        doc['nodes'].append(read('x','PositionX'))
        doc['links'].append({'from':'x.out:result','to':'zero.in:X'})
        nodes,edges,_=folded(doc)
        self.assertIn('zero',{n['id'] for n in nodes})
        self.assertEqual(sum(e[0]=='zero' for e in edges),2)
        doc=self.shared_vector_fixture();doc['nodes'][0]['_reviewStatus']='added'
        self.assertEqual(sum(e[0]=='zero' for e in folded(doc)[1]),2)
        doc=self.shared_vector_fixture()
        edge=('zero','result','dot','A',False)
        self.assertNotIn('_formula',next(n for n in folded(doc,removed_links=[edge])[0] if n['id']=='zero'))
        for metadata in [
            {'PinType.PinSubCategoryObject':'"/Script/CoreUObject.ScriptStruct\'/Script/CoreUObject.Rotator\'"'},
            {'PinType.ContainerType':'Array'}, {'PinType.bIsReference':'True'}]:
            doc=self.shared_vector_fixture();doc['pinTemplates']['Vector'].update(metadata)
            self.assertFalse(any('_formula' in n for n in folded(doc)[0]))
        for value in ('0', '1, 2', '1, 2, 3, 4', '(X=1,Y=2,Z=3)', 'nan, 0, 0'):
            doc=dict(schema=2,pinTemplates=TYPES,nodes=[call('op','Dot_VectorVector',
                [('A','Vector'),('B','Vector')],defaults={'in:A':value,'in:B':'0, 0, 0'})],links=[])
            self.assertNotIn('_formula',folded(doc)[0][0])

    def test_vector_interpolation_keeps_scalar_conversion_and_shared_constant(self):
        doc=self.shared_vector_fixture()
        doc['nodes'].extend([read('dt','DeltaSeconds','float'),read('speed','Decay','double'),
            call('interp','VInterpTo_Constant',[('Current','Vector'),('Target','Vector'),
                 ('DeltaTime','float'),('InterpSpeed','float')],'Vector')])
        doc['links'].extend({'from':a,'to':b} for a,b in [('add.out:result','interp.in:Current'),
            ('zero.out:result','interp.in:Target'),('dt.out:result','interp.in:DeltaTime'),
            ('speed.out:result','interp.in:InterpSpeed')])
        original=copy.deepcopy(doc)
        nodes,edges,_=folded(doc)
        self.assertNotIn('zero',{n['id'] for n in nodes})
        interp=next(n for n in nodes if n['id']=='interp')
        self.assertEqual(interp['_formula'], 'VInterpTo_Constant(current: (Vector(0, 0, 0) + Velocity), '
                         'target: Vector(0, 0, 0), deltaTime: DeltaSeconds, interpSpeed: float(Decay))')
        self.assertEqual([p['id'] for p in interp['_foldedNodes']].count('zero'),1)
        self.assertFalse(any(e[0]=='zero' for e in edges))
        self.assertEqual(doc,original)

    def test_fallback_names_are_local_ordered_and_independent_of_graph_order(self):
        doc = fixture()
        # Separate expressions feed a third one; first use determines local names.
        doc['nodes'].insert(0, call('extra', 'FMax', [('A','double'),('B','double')],
                                    defaults={'in:A':'2','in:B':'3'}))
        doc['links'] = [e for e in doc['links'] if e['to'] != 'multiply.in:B']
        doc['links'].append({'from':'extra.out:result','to':'multiply.in:B'})
        baseline = copy.deepcopy(doc)
        for reverse in (False, True):
            if reverse:
                doc['nodes'].reverse()
                doc['links'].reverse()
            nodes, _, _ = folded(doc, max_nodes=1)
            groups = {n['id']: n for n in nodes if '_formula' in n}
            self.assertEqual(groups['multiply']['_formula'], '(value1 * value2)')
            self.assertEqual([(p['name'], p['source']) for p in groups['multiply']['_formulaInputs']],
                             [('value1','sub'), ('value2','extra')])
            self.assertEqual(groups['compare']['_formula'], '(value1 > 0.1)')
            self.assertEqual(groups['sub']['_formula'], '(Deceleration - 0.5)')
        doc['nodes'].reverse(); doc['links'].reverse()
        self.assertEqual(doc, baseline)

    def test_local_aliases_preserve_named_inputs_and_repeated_uses(self):
        doc = fixture()
        doc['nodes'][1]['variable'] = 'value1'  # Real name must not alias a generated input.
        nodes, _, _ = folded(doc, max_nodes=1)
        multiply = next(n for n in nodes if n['id'] == 'multiply')
        self.assertEqual(multiply['_formula'], '(value2 * value1)')
        self.assertEqual([(p['name'], p['source']) for p in multiply['_formulaInputs']],
                         [('value2','sub'), ('value1','dt')])
        doc['links'] = [e for e in doc['links'] if e['to'] != 'multiply.in:B']
        doc['links'].append({'from':'sub.out:result','to':'multiply.in:B'})
        nodes, edges, _ = folded(doc)
        result = next(n for n in nodes if n['id'] == 'compare')
        self.assertEqual(result['_formula'], '((value1 * value1) > 0.1)')
        self.assertEqual([p['name'] for p in result['_formulaInputs']], ['value1'])
        self.assertEqual(sum(e[0] == 'sub' and e[2] == 'compare' for e in edges), 1)

    def test_input_sections_use_card_width_and_precede_bold_output_with_gap(self):
        doc = fixture()
        doc['nodes'][0]['variable'] = 'PreviousCorrectedVerticalVelocity'
        for direction in ('TB', 'LR'):
            svg = ET.fromstring(render(doc, view='expressions', direction=direction)[0])
            card = svg.find('.//{*}g[@data-node-id="compare"]')
            rows = card.findall('{*}text')
            inputs = [r for r in rows if visible_text(r).startswith('Input:')]
            outputs = [r for r in rows if visible_text(r).startswith('Output (')]
            self.assertEqual(len(inputs), 2)
            self.assertIn('Input: PreviousCorrectedVerticalVelocity (double)', [visible_text(r) for r in inputs])
            self.assertLess(rows.index(inputs[-1]), rows.index(outputs[0]))
            self.assertEqual(float(outputs[0].get('y')) - float(inputs[-1].get('y')), 27)
            for row in inputs + outputs:
                label = row.find('{*}tspan')
                self.assertEqual(label.get('font-weight'), '600')
                self.assertEqual(label.get('style'), 'font-family:Segoe UI,Arial,sans-serif')
                self.assertTrue(label.text.startswith(('Input:', 'Output (')))
            for row in rows:
                if row.get('font-family'):
                    self.assertIn('font-family:Consolas,monospace', row.get('style'))

    def test_repeated_expression_inputs_share_one_edge_and_keep_all_uses(self):
        doc = fixture()
        doc['links'].append({'from':'rate.out:result','to':'sub.in:B'})
        original = copy.deepcopy(doc)
        nodes, edges, _ = folded(doc)
        group = next(n for n in nodes if n['id'] == 'compare')
        self.assertEqual(sum(e[0] == 'rate' for e in edges), 1)
        self.assertEqual(group['_formula'].count('Deceleration'), 2)
        self.assertEqual(len(group['_boundaryLinks']), 4)
        self.assertEqual(doc, original)
        for direction in ('TB','LR'):
            svg = ET.fromstring(render(doc, view='expressions', direction=direction)[0])
            wires = svg.findall('.//{*}g[@data-source="rate"]')
            self.assertEqual(len(wires), 1)
            hint = wires[0].find('{*}title').text
            self.assertIn('sub: A (double)', hint)
            self.assertIn('sub: B (double)', hint)
            full = ET.fromstring(render(doc, view='full', direction=direction)[0])
            self.assertEqual(len(full.findall('.//{*}g[@data-source="rate"]')), 2)
        # Different pins of the same source are distinct values, not duplicates.
        doc['nodes'][0]['pins']['out:other'] = {'type':'double'}
        doc['links'][-1]['from'] = 'rate.out:other'
        self.assertEqual(sum(e[0] == 'rate' for e in folded(doc)[1]), 2)

    def test_structural_wrapping_preserves_tokens_groups_and_compact_arguments(self):
        tree = {'op':'AND','args':[{'op':'<=','args':['Deceleration','0.0']},
            {'op':'OR','args':[{'op':'AND','args':['Descending',{'op':'<','args':['VerticalVelocity','0.0']}]},
                              {'op':'AND','args':[{'op':'NOT','args':['Descending']},
                                                 {'op':'>','args':['VerticalVelocity','0.0']}]}]}]}
        lines = format_expression(tree, width=60)
        self.assertEqual(lines[0], 'Output:')
        self.assertTrue(all(len(line) <= 60 for line in lines))
        self.assertTrue(any(line.lstrip().startswith('AND ') for line in lines))
        self.assertTrue(any(line.lstrip().startswith('OR ') for line in lines))
        self.assertFalse(any(line.rstrip().endswith(('AND','OR')) for line in lines))
        self.assertEqual(''.join(''.join(lines[1:]).split()), ''.join(flat_expression(tree).split()))
        short = {'op':'min','args':['A','B']}
        self.assertEqual(format_expression(short), ['Output: min(A, B)'])
        call = {'op':'select','args':['SomeLongCondition','A','B']}
        self.assertEqual(format_expression(call, width=40),
                         ['Output:', '  select(', '    SomeLongCondition, true: A,', '    false: B)'])

    def test_formula_output_has_no_temporary_assignment_and_preserves_indentation(self):
        doc=fixture()
        for direction in ('TB','LR'):
            svg=ET.fromstring(render(doc,view='expressions',direction=direction)[0])
            card=svg.find('.//{*}g[@data-node-id="compare"]')
            rows=card.findall('{*}text')
            code=[r for r in rows if r.get('font-family')]
            self.assertTrue(code)
            self.assertTrue(visible_text(code[0]).startswith('Output (bool):'))
            self.assertTrue(all(r.get('{http://www.w3.org/XML/1998/namespace}space')=='preserve' for r in code))
            self.assertTrue(all('white-space:pre' in r.get('style') for r in code))
            self.assertFalse(any('Ready =' in (visible_text(r) or '') or 'Output: Ready' in (visible_text(r) or '') for r in rows))
        # A long identifier is indivisible; widen the card rather than corrupt it.
        identifier='Unbroken'+('LongVariableName'*5)
        doc['nodes'][0]['variable']=identifier
        svg=ET.fromstring(render(doc,view='expressions')[0])
        card=next(c for c in svg.findall('.//{*}g[@class="graph-node"]')
                  if any(identifier in (visible_text(r) or '') for r in c.findall('{*}text') if r.get('font-family')))
        code=[visible_text(r) for r in card.findall('{*}text') if r.get('font-family')]
        self.assertTrue(any(identifier in line for line in code))
        self.assertGreater(float(card.find('{*}rect').get('width')),330)

    def test_formula_keeps_order_parentheses_variables_and_original_endpoints(self):
        doc=fixture(); original=copy.deepcopy(doc)
        nodes,edges,count=folded(doc)
        self.assertEqual(count,2)
        group=next(n for n in nodes if '_formula' in n)
        self.assertEqual(group['_formula'],'(((Deceleration - 0.5) * DeltaSeconds) > 0.1)')
        self.assertEqual(group['_formulaResult'],'Ready')
        self.assertEqual({n['id'] for n in group['_foldedNodes']},{'sub','multiply','compare'})
        self.assertEqual({n['id'] for n in nodes},{'rate','dt','compare','exit'})
        edge=next(e for e in edges if e[0]=='rate')
        self.assertEqual(edge_pin_details(group,edge[3],'in',TYPES),('sub','A','double'))
        self.assertEqual(doc,original)
        for direction in ('TB','LR'):
            svg=ET.fromstring(render(doc,view='expressions',direction=direction)[0])
            self.assertIsNotNone(svg.find('.//{*}g[@data-node-id="rate"]'))
            card=svg.find('.//{*}g[@data-node-id="compare"]')
            details=json.loads(card.find('{*}title').text)
            self.assertEqual(details['_foldedNodes'],group['_foldedNodes'])
            self.assertEqual(set(json.loads(card.get('data-node-ids'))),{'compare','multiply','sub'})
        self.assertEqual(render(doc,view='full')[2],6)

    def test_shared_intermediate_stays_a_separate_expression(self):
        doc=fixture()
        doc['nodes'].append(dict(id='watch',kind='FunctionResult',pins={'in:Value':{'type':'double'}}))
        doc['links'].append({'from':'sub.out:result','to':'watch.in:Value'})
        nodes,edges,_=folded(doc)
        self.assertEqual({n['id'] for n in nodes if '_formula' in n},{'sub','compare'})
        self.assertEqual(sum(e[0]=='sub' for e in edges),2)

    def test_unknown_owner_impure_extra_inputs_and_missing_literals_stay_explicit(self):
        for mutation in [
            lambda n: n['properties'].update(FunctionReference=n['properties']['FunctionReference'].replace('KismetMathLibrary','CustomLibrary')),
            lambda n: n['properties'].update(bDefaultsToPureFunc='False'),
            lambda n: n['pins'].update({'in:exec':{'type':'exec'}}),
            lambda n: n['defaults'].clear(),
            lambda n: n.update(_graphLink={'href':'function.html'}),
            lambda n: n['properties'].update(EnabledState='DevelopmentOnly'),
            lambda n: n['properties'].update(NodeComment='"Keep this explanation"'),
        ]:
            doc=fixture();mutation(doc['nodes'][2])
            nodes,_,_=folded(doc)
            self.assertNotIn('_formula',next(n for n in nodes if n['id']=='sub'))

    def test_select_keeps_true_false_order_and_numeric_conversion(self):
        doc=dict(schema=2,pinTemplates=TYPES,nodes=[read('condition','Descending','bool'),read('value','Velocity','float'),
            call('choose','SelectFloat',[('A','double'),('B','double'),('bPickA','bool')],defaults={'in:B':'-0.25'})],
            links=[{'from':'condition.out:result','to':'choose.in:bPickA'}, {'from':'value.out:result','to':'choose.in:A'}])
        group=next(n for n in folded(doc)[0] if '_formula' in n)
        self.assertEqual(group['_formula'],'select(Descending, true: double(Velocity), false: -0.25)')

    def test_different_reads_with_same_name_have_distinct_symbols(self):
        doc=fixture();doc['nodes'][1]['variable']='Deceleration'
        group=next(n for n in folded(doc)[0] if '_formula' in n)
        self.assertIn('Deceleration_rate',group['_formula'])
        self.assertIn('Deceleration_dt',group['_formula'])

    def test_limits_and_cycles_do_not_duplicate_or_drop_nodes(self):
        doc=fixture()
        for limits in ({'max_nodes':1},{'max_chars':12}):
            nodes,_,_=folded(doc,**limits)
            members=[p['id'] for n in nodes for p in n.get('_foldedNodes',[n])]
            self.assertEqual(sorted(members),sorted(n['id'] for n in doc['nodes']))
            self.assertEqual(len([n for n in nodes if '_formula' in n]),3)
        doc['links']=[l for l in doc['links'] if l['from']!='rate.out:result']
        doc['links'].append({'from':'multiply.out:result','to':'sub.in:A'})
        nodes,_,count=folded(doc)
        self.assertEqual(count,0)
        self.assertFalse(any('_formula' in n for n in nodes))

    def test_review_statuses_and_removed_edges_do_not_merge(self):
        doc=fixture()
        doc['nodes'][2]['_reviewStatus']='added'
        nodes,_,_=folded(doc)
        self.assertEqual({n['id'] for n in nodes if '_formula' in n},{'sub','compare'})
        edge=('sub','result','multiply','A',False)
        nodes,_,_=folded(fixture(),removed_links=[edge])
        self.assertNotIn('_formula',next(n for n in nodes if n['id']=='sub'))
        self.assertNotIn('_formula',next(n for n in nodes if n['id']=='multiply'))
        old=fixture();new=copy.deepcopy(old);new['nodes'][2]['defaults']['in:B']='0.75'
        svg=ET.fromstring(render(new,view='expressions',review={'changed':['sub']},before=old)[0])
        card=svg.find('.//{*}g[@data-node-id="sub"]')
        details=json.loads(card.find('{*}title').text)
        self.assertEqual(details['Before']['Nodes']['sub']['node']['defaults']['in:B'],'0.5')
        self.assertEqual(details['After']['Nodes']['sub']['node']['defaults']['in:B'],'0.75')

    def test_expressions_and_chain_grouping_compose_without_losing_source_nodes(self):
        doc = fixture()
        doc['nodes'] += [
            dict(id='self', kind='self', pins={'out:result': {'type':'double'}}),
            read('owner', 'Owner'), read('field', 'Field'),
            *[dict(id=n, kind='call', operation=n, pins={
                'in:exec': {'type':'exec'}, 'out:then': {'type':'exec'}}) for n in ('first','second','off')]]
        doc['nodes'][-1]['enabled'] = False
        doc['links'] += [{'from':a, 'to':b} for a,b in (
            ('self.out:result','owner.in:target'), ('owner.out:result','field.in:target'),
            ('first.out:then','second.in:exec'), ('second.out:then','off.in:exec'))]
        original = copy.deepcopy(doc)
        for grouping in (False,True):
            for expressions in (False,True):
                for direction in ('TB','LR'):
                    with self.subTest(grouping=grouping, expressions=expressions, direction=direction):
                        svg=ET.fromstring(render(doc,group_chains=grouping,expressions=expressions,direction=direction)[0])
                        cards=[json.loads(g.find('{*}title').text) for g in svg.findall('.//{*}g[@class="graph-node"]')]
                        parts=[p for c in cards for p in c.get('_foldedNodes',[c])]
                        self.assertCountEqual([p['id'] for p in parts],[n['id'] for n in doc['nodes']])
                        self.assertTrue(all('_foldedNodes' not in p for p in parts))
                        self.assertEqual(any(c.get('_chainKind')=='action' for c in cards),grouping)
                        self.assertEqual(any(c.get('_chainKind')=='data' for c in cards),grouping)
                        self.assertEqual(any('_formula' in c for c in cards),expressions)
                        self.assertIn('self',[c['id'] for c in cards])
                        self.assertIn('off',[c['id'] for c in cards])
        self.assertEqual(doc,original)

    def test_html_contains_options_in_all_review_variants(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);source=root/'source.json';old=root/'old.json';review=root/'review.json'
            doc=fixture();old.write_text(json.dumps(doc),encoding='utf-8')
            doc['nodes'][2]['defaults']['in:B']='0.75';source.write_text(json.dumps(doc),encoding='utf-8')
            review.write_text(json.dumps({'changed':['sub']}),encoding='utf-8')
            subprocess.run([sys.executable,'-B',str(Path(__file__).with_name('render_graph.py')),str(source),
                            '--output',str(root/'graph.svg'),'--before',str(old),'--review',str(review)],check=True,capture_output=True)
            page=(root/'graph.html').read_text(encoding='utf-8')
            self.assertTrue('id="expressions" type="checkbox"' in page)
            self.assertTrue('id="group-chains" type="checkbox"' in page)
            self.assertFalse('id="view"' in page)
            for prefix in ('','final-'):
                for direction in ('TB','LR'):
                    for key in ('full','simplified','expressions','combined'):
                        self.assertTrue('id="'+prefix+key+'-'+direction+'"' in page)


if __name__ == '__main__':
    unittest.main()
