"""Bounded member-use search and optional source context; no runtime claims."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from graph_context import component_detail
from graph_hierarchy import graph_key
from graph_inspection import inspect_source
from graph_references import find_references
from graph_symbols import member_identity
from preview_graph_links import resolve_calls
from test_graph_inspection import fixture
from test_nested_graphs import fixture as nested_fixture


def sources():
    caller = fixture()
    caller['asset'] = '/Game/Caller'
    caller['classes'].update(delegate='/Script/BlueprintGraph.K2Node_CreateDelegate',
        bind='/Script/BlueprintGraph.K2Node_AddDelegate', self='/Script/BlueprintGraph.K2Node_Self',
        entry='/Script/BlueprintGraph.K2Node_FunctionEntry')
    caller['pinTemplates']['object'] = {'PinType.PinCategory': '"object"',
        'PinType.PinSubCategoryObject': '"/Script/CoreUObject.Class\'/Script/CoreUObject.Object\'"'}
    caller['pinTemplates']['delegate'] = {'PinType.PinCategory': '"delegate"'}
    callee = copy.deepcopy(caller)
    callee['asset'] = '/Game/Callee'
    callee['graphs'] = [dict(name='Reset', kind='function', properties={'GraphGuid': '2' * 32},
        body=dict(nodes=[dict(id='Entry', kind='entry', pins={})], links=[]))]
    graph = caller['graphs'][0]
    graph['body']['nodes'] = [
        dict(id='Call', kind='call', properties={'FunctionReference':
            '(MemberParent="/Game/Callee.Callee_C",MemberName="Reset",MemberGuid=' + '2' * 32 + ')'}, pins={}),
        dict(id='Read', kind='getVariable', properties={'VariableReference':
            '(MemberParent="/Game/Callee.Callee_C",MemberName="Value")'}, pins={}),
        dict(id='Callback', kind='delegate', properties={'SelectedFunctionName': '"Reset"'},
             pins={'in:target': {'type': 'object', 'name': 'self'}, 'out:delegate': {'type': 'delegate'}}),
        dict(id='Bind', kind='bind', properties={'DelegateReference': '(MemberName="Changed",bSelfContext=True)'},
             pins={'in:Delegate': {'type': 'delegate'}}),
        dict(id='Self', kind='self', pins={'out:target': {'type': 'object'}}),
        dict(id='Timer', kind='call', properties={'FunctionReference':
             '(MemberParent="/Script/Engine.KismetSystemLibrary",MemberName="K2_SetTimerForNextTick")'},
             pins={'in:Object': {'type': 'object'}, 'in:FunctionName': {'type': 'text'}},
             defaults={'in:FunctionName': 'Reset'})]
    graph['body']['links'] = [{'from': 'Callback.out:delegate', 'to': 'Bind.in:Delegate'},
                             {'from': 'Self.out:target', 'to': 'Timer.in:Object'}]
    caller['graphs'].append(copy.deepcopy(callee['graphs'][0]))
    return caller, callee


class ReferenceTests(unittest.TestCase):
    def test_direct_delegate_and_timer_remain_distinct_and_owner_scoped(self):
        docs = sources()
        original = json.dumps(docs)
        result = find_references(docs, function='Reset')
        self.assertEqual([r['kind'] for r in result['uses']], ['direct-call', 'delegate-reference', 'timer'])
        self.assertTrue(all(r['resolution']['status'] == 'resolved' for r in result['uses']))
        self.assertEqual(result['uses'][1]['bindings'][0]['action'], 'bind')
        self.assertEqual(result['uses'][1]['bindings'][0]['node'], 'Bind')
        self.assertEqual([r['node'] for r in find_references(docs, function='Reset', owner='/Game/Callee')['uses']], ['Call'])
        self.assertEqual([r['node'] for r in find_references(docs, function='Reset', owner='/Game/Caller')['uses']], ['Callback', 'Timer'])
        result['uses'][1]['bindings'].clear()
        self.assertEqual(json.dumps(docs), original)

    def test_variable_reads_writes_are_not_confused_with_functions_or_external_names(self):
        caller, callee = sources()
        write = dict(id='Write', kind='set', properties={'VariableReference': '(MemberName="Value",bSelfContext=True)'}, pins={})
        caller['classes']['set'] = '/Script/BlueprintGraph.K2Node_VariableSet'
        caller['graphs'][0]['body']['nodes'].append(write)
        result = find_references([caller, callee], variable='Value')
        self.assertEqual([r['kind'] for r in result['uses']], ['read', 'write'])
        self.assertEqual([r['target']['owner'] for r in result['uses']], ['/Game/Callee', '/Game/Caller'])
        self.assertEqual(len(find_references([caller, callee], variable='Value', owner='/Game/Callee')['uses']), 1)

    def test_dynamic_name_is_not_replaced_by_stored_default(self):
        caller, callee = sources()
        graph = caller['graphs'][0]
        graph['body']['nodes'].append(dict(id='Name', kind='getVariable', properties={}, pins={'out:Name': {'type': 'text'}}))
        graph['body']['links'].append({'from': 'Name.out:Name', 'to': 'Timer.in:FunctionName'})
        result = find_references([caller, callee], function='Reset')
        self.assertNotIn('Timer', [r['node'] for r in result['uses']])
        self.assertEqual(result['unresolvedCandidates'][0]['node'], 'Timer')
        self.assertIsNone(result['unresolvedCandidates'][0]['target']['name'])

    def test_missing_source_guid_mismatch_and_ambiguous_definition_do_not_guess(self):
        caller, callee = sources()
        only = find_references([caller], function='Reset')['uses'][0]
        self.assertEqual(only['resolution']['status'], 'unresolved')
        call = caller['graphs'][0]['body']['nodes'][0]
        call['properties']['FunctionReference'] = '(MemberParent="/Game/Callee.Callee_C",MemberName="Reset",MemberGuid=' + '9'*32 + ')'
        result = find_references([caller, callee], function='Reset')['uses'][0]
        self.assertEqual(result['resolution']['status'], 'unresolved')
        identity = member_identity(caller, call)
        self.assertEqual(resolve_calls(caller, 'EventGraph', {'/Game/Callee': [{'id': '2'*32, 'name': 'Reset'}]}), {})
        self.assertEqual(identity['guid'], '9'*32)
        call['properties']['FunctionReference'] = '(MemberParent="/Game/Callee.Callee_C",MemberName="Reset")'
        duplicate = copy.deepcopy(callee['graphs'][0]); duplicate['properties']['GraphGuid'] = '3'*32
        callee['graphs'].append(duplicate)
        # The source hierarchy itself rejects duplicate root names.
        with self.assertRaises(ValueError):
            find_references([caller, callee], function='Reset')

    def test_unknown_owner_does_not_claim_a_filtered_match(self):
        caller, callee = sources()
        caller['graphs'][0]['body']['nodes'][0]['properties']['FunctionReference'] = '(MemberName="Reset")'
        result = find_references([caller, callee], function='Reset', owner='/Game/Callee')
        self.assertFalse(result['uses'])
        self.assertEqual(result['unresolvedCandidates'][0]['node'], 'Call')

    def test_only_supported_native_timer_api_is_classified(self):
        caller, callee = sources()
        node = caller['graphs'][0]['body']['nodes'][-1]
        node['properties']['FunctionReference'] = '(MemberParent="/Game/Custom.Custom_C",MemberName="K2_SetTimerForNextTick")'
        result = find_references([caller, callee], function='Reset')
        self.assertNotIn('timer', [r['kind'] for r in result['uses']])

    def test_guid_resolution_agrees_with_preview_and_retains_disabled_sites(self):
        caller, callee = sources()
        callee['graphs'][0]['name'] = 'Renamed'
        call = caller['graphs'][0]['body']['nodes'][0]
        call['properties']['EnabledState'] = 'Disabled'
        result = find_references([caller, callee], function='Renamed')['uses']
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['enabledState'], 'Disabled')
        navigation = resolve_calls(caller, 'EventGraph', {'/Game/Callee': [dict(id='2'*32, name='Renamed', href='target.html')]})
        self.assertEqual(navigation['Call']['id'], result[0]['resolution']['graph'])
        self.assertEqual(navigation['Call']['name'], result[0]['resolution']['name'])

    def test_connected_callback_target_uses_declared_type_or_remains_unknown(self):
        caller, callee = sources()
        graph = caller['graphs'][0]
        caller['pinTemplates']['callee'] = {'PinType.PinCategory': '"object"',
            'PinType.PinSubCategoryObject': '"/Game/Callee.Callee_C"'}
        source = dict(id='Object', kind='getVariable', properties={}, pins={'out:Object': {'type': 'callee'}})
        graph['body']['nodes'].append(source)
        graph['body']['links'].append({'from': 'Object.out:Object', 'to': 'Callback.in:target'})
        result = find_references([caller, callee], function='Reset', owner='/Game/Callee')
        self.assertEqual([r['node'] for r in result['uses']], ['Call', 'Callback'])
        source['pins']['out:Object']['type'] = 'object'
        result = find_references([caller, callee], function='Reset', owner='/Game/Callee')
        self.assertEqual([r['node'] for r in result['uses']], ['Call'])
        self.assertEqual([r['node'] for r in result['unresolvedCandidates']], ['Callback'])

    def test_invalid_queries_and_duplicate_assets_reject(self):
        docs = sources()
        for kwargs in ({}, dict(function='Reset', variable='Value'), dict(function=''), dict(function='Reset', scope='Scope')):
            with self.subTest(kwargs=kwargs), self.assertRaises(ValueError):
                find_references(docs, **kwargs)
        with self.assertRaisesRegex(ValueError, 'one source per asset'):
            find_references([docs[0], docs[0]], function='Reset')


def context_source():
    document = nested_fixture()
    parent, child = document['graphs'][0], document['graphs'][2]
    parent['kind'] = 'function'
    document['classes'].update(entry='/Script/BlueprintGraph.K2Node_FunctionEntry',
                               get='/Script/BlueprintGraph.K2Node_VariableGet')
    declaration = '(VarName="Available",VarGuid=' + '8'*32 + ',VarType=(PinCategory="bool"),DefaultValue="false")'
    parent['body']['nodes'].append(dict(id='Entry', kind='entry', pins={}, properties={'LocalVariables(0)': declaration}))
    child['body']['nodes'].append(dict(id='ReadLocal', kind='get', pins={}, properties={
        'VariableReference': '(MemberName="Available",MemberScope="Root",MemberGuid=' + '8'*32 + ')'}))
    document['components'] = dict(scs=None, templates=[], generatedTemplates=['BP_C:Plate'], objects=[
        dict(id='BP_C:Plate', name='Plate', outer='$class', **{'class': '/Script/Engine.StaticMeshComponent'}, flags=33,
             properties={'BodyInstance': '(CollisionEnabled=NoCollision)'}),
        dict(id='BP_C:Plate:Child', name='Child', outer='BP_C:Plate', **{'class': '/Script/CoreUObject.Object'}, properties={}),
        dict(id='BP_C:Other', name='Other', outer='$class', **{'class': '/Script/Engine.StaticMeshComponent'}, properties={})])
    document['classes']['call'] = '/Script/BlueprintGraph.K2Node_CallFunction'
    document['pinTemplates']['text'] = {'PinType.PinCategory': '"name"'}
    child['body']['nodes'].append(dict(id='Add', kind='call', properties={
        'FunctionReference': '(MemberName="AddComponent",bSelfContext=True)'},
        pins={'in:TemplateName': {'type': 'text'}}, defaults={'in:TemplateName': 'Plate'}))
    return document


class ContextTests(unittest.TestCase):
    def test_nested_local_and_template_context_is_optional_scoped_and_exact(self):
        document = context_source(); original = json.dumps(document)
        self.assertNotIn('context', inspect_source(document, 'C'*32))
        view = inspect_source(document, 'C'*32, context='all')['context']
        local = view['locals'][0]
        self.assertEqual(local['status'], 'resolved')
        self.assertEqual(local['graph'], 'A'*32)
        self.assertEqual(local['source'], document['graphs'][0]['body']['nodes'][-1]['properties']['LocalVariables(0)'])
        self.assertEqual(local['declaration']['DefaultValue'], 'false')
        component = view['components'][0]
        self.assertEqual(component['status'], 'source-template')
        self.assertEqual(component['detail']['object'], document['components']['objects'][0])
        self.assertEqual(len(component['detail']['ownedObjects']), 1)
        focused = inspect_source(document, 'C'*32, node_ids=['ReadLocal'], context='all')['context']
        self.assertEqual(focused['components'], [])
        view['components'][0]['detail']['object']['properties'].clear()
        self.assertEqual(json.dumps(document), original)

    def test_local_search_keeps_owner_scope_and_nested_path(self):
        document = context_source()
        result = find_references([document], variable='Available', scope='Root')['uses']
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['path'], ['Root', 'Details', 'Details'])
        self.assertEqual(result[0]['resolution']['graph'], 'A'*32)
        self.assertFalse(find_references([document], variable='Available', scope='Other')['uses'])
        document['graphs'][2]['body']['nodes'][-2]['properties']['VariableReference'] = '(MemberName="Available",MemberScope="Other")'
        self.assertEqual(inspect_source(document, 'C'*32, context='locals')['context']['locals'][0]['status'], 'unresolved')

    def test_component_selection_and_dynamic_template_boundaries(self):
        document = context_source()
        self.assertEqual(component_detail(document, 'Plate')['object']['id'], 'BP_C:Plate')
        graph = document['graphs'][2]; node = graph['body']['nodes'][-1]
        graph['body']['links'].append({'from': 'Entry.out:Value', 'to': 'Add.in:TemplateName'})
        self.assertEqual(inspect_source(document, 'C'*32, context='components')['context']['components'][0]['status'], 'unresolved')
        graph['body']['links'].pop()
        node['pins']['in:ComponentTemplateContext'] = dict(type='text', DefaultObject='/Game/Other.Other_C')
        self.assertEqual(inspect_source(document, 'C'*32, context='components')['context']['components'][0]['status'], 'unresolved')
        document['components']['objects'].append(dict(document['components']['objects'][0], id='Duplicate'))
        with self.assertRaises(ValueError):
            component_detail(document, 'Plate')
        with self.assertRaises(ValueError):
            component_detail(document, 'Missing')

    def test_context_selectors_do_not_change_full_compatibility(self):
        document = context_source()
        for kwargs in (dict(context='locals'), dict(selection='C'*32, view='full', context='locals'),
                       dict(selection='C'*32, context='unknown')):
            with self.subTest(kwargs=kwargs), self.assertRaises(ValueError):
                inspect_source(document, **kwargs)

    def test_cli_reads_explicit_files_and_never_writes_sources(self):
        root = Path(__file__).resolve().parents[2] / 'Tmp/BlueprintText'
        root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=root) as folder:
            paths = [Path(folder) / (str(i)+'.json') for i in range(3)]
            for path, doc in zip(paths, [*sources(), context_source()]):
                path.write_text(json.dumps(doc), encoding='utf-8')
            before = [p.read_bytes() for p in paths]
            def run(script, *args):
                return subprocess.run([sys.executable, '-B', str(Path(__file__).with_name(script)), *map(str, args)], capture_output=True)
            result = run('graph_references.py', *paths[:2], '--function', 'Reset')
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(len(json.loads(result.stdout)['uses']), 3)
            result = run('graph_hierarchy.py', paths[2], '--graph', 'C'*32, '--context', 'all')
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(json.loads(result.stdout)['context']['locals'][0]['name'], 'Available')
            result = run('graph_hierarchy.py', paths[2], '--component', 'Plate')
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(json.loads(result.stdout)['object']['name'], 'Plate')
            for script, args in [('graph_references.py', [paths[0], paths[0], '--function', 'Reset']),
                                 ('graph_hierarchy.py', [paths[2], '--component', 'Plate', '--graph', 'Root'])]:
                failed = run(script, *args)
                self.assertEqual(failed.returncode, 2)
                self.assertEqual(failed.stdout, b'')
            self.assertEqual([p.read_bytes() for p in paths], before)
            self.assertEqual(set(Path(folder).iterdir()), set(paths))


if __name__ == '__main__':
    unittest.main()
