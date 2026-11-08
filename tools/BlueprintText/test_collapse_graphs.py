"""Collapse safety, identity preservation and offline workspace transaction contracts."""
import copy
from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

import asset_text_codec as codec
import blueprint_workspace as bw
from collapse_graph import collapse, review_marks, uid, verify_inverse
from graph_hierarchy import select_graph


def fixture():
    def node(name, kind, **pins):
        return dict(id=name, kind=kind, properties={'NodeGuid': uid()},
                    pins={alias.replace('_', ':', 1): dict(id=uid(), type=ptype) for alias, ptype in pins.items()})
    nodes = [node('Entry', 'FunctionEntry', out_then='exec', out_Input='int', out_Choose='bool'),
             node('Branch', 'IfThenElse', in_exec='exec', in_Condition='bool', out_then='exec', out_else='exec'),
             node('Math', 'CallFunction', in_A='int', out_Result='int'),
             node('Set', 'VariableSet', in_exec='exec', in_Value='int', out_then='exec'),
             node('Terminal', 'VariableSet', in_exec='exec', in_Value='int', out_then='exec'),
             node('Result', 'FunctionResult', in_exec='exec', in_Value='int')]
    edges = [('Entry.out:then', 'Branch.in:exec'), ('Entry.out:Choose', 'Branch.in:Condition'),
             ('Entry.out:Input', 'Math.in:A'), ('Math.out:Result', 'Set.in:Value'),
             ('Math.out:Result', 'Terminal.in:Value'), ('Math.out:Result', 'Result.in:Value'),
             ('Branch.out:then', 'Set.in:exec'), ('Branch.out:else', 'Terminal.in:exec'),
             ('Set.out:then', 'Result.in:exec')]
    return dict(schema=2, format='compact-asset', asset='/Game/Test', parent='/Script/CoreUObject.Object',
                properties={}, variables=[], defaults={}, dependencies=[],
                classes={n['kind']: '/Script/BlueprintGraph.K2Node_' + n['kind'] for n in nodes},
                pinTemplates={t: {'PinType.PinCategory': '"'+t+'"'} for t in ('int', 'bool', 'exec')},
                graphs=[dict(name='Run', kind='function', properties={'GraphGuid': 'A'*32}, format='compact',
                             body=dict(nodes=nodes, links=[dict(zip(('from', 'to'), e)) for e in edges]))])


class CollapseTests(unittest.TestCase):
    def test_branch_terminal_and_shared_pure_value_preserved(self):
        before = fixture()
        unchanged = copy.deepcopy(before)
        after, result = collapse(before, 'Run', ['Branch', 'Math', 'Set', 'Terminal'], 'Operation')
        self.assertEqual(before, unchanged)
        self.assertEqual(result['parentNodesAfter'], 3)
        child = select_graph(after, result['childGraph'])
        self.assertFalse(any(e['from'] == 'Terminal.out:then' for e in child['body']['links']))
        verify_inverse(before, after, result['childGraph'])
        self.assertEqual(codec.compare(after, codec.compact(codec.native(after))), [])

    def test_pure_fanout_and_explicit_names(self):
        before = fixture()
        after, result = collapse(before, 'Run', ['Math'], 'Calculate', {'Entry.out:Input': 'Amount', 'Math.out:Result': 'Computed'})
        child = select_graph(after, result['childGraph'])
        self.assertEqual(set(child['body']['nodes'][0]['pins']), {'out:Amount'})
        self.assertEqual(set(child['body']['nodes'][-1]['pins']), {'in:Computed'})
        self.assertEqual(result['boundaryPins'], 2)
        self.assertEqual(len([e for e in select_graph(after, 'Run')['body']['links'] if e['from'] == 'K2Node_Composite_Calculate.out:Computed']), 3)

    def test_nested_composite_reparenting(self):
        source, inner = collapse(fixture(), 'Run', ['Math'], 'Calculate')
        after, outer = collapse(source, 'Run', ['K2Node_Composite_Calculate'], 'Calculation')
        self.assertEqual(select_graph(after, inner['childGraph'])['owner']['graph'], outer['childGraph'])
        verify_inverse(source, after, outer['childGraph'])

    def test_multiple_callers_to_same_execution_entry_are_not_duplicated(self):
        source = fixture()
        source['graphs'][0]['body']['links'] += [dict(zip(('from', 'to'), ('Terminal.out:then', 'Set.in:exec')))]
        after, result = collapse(source, 'Run', ['Set'], 'Assign')
        verify_inverse(source, after, result['childGraph'])

    def test_rejections_leave_input_unchanged(self):
        scenarios = [(['Entry'], 'Operation', {}, 'Unsupported selected'),
                     (['Result'], 'Operation', {}, 'Unsupported selected'),
                     (['Set', 'Terminal'], 'Operation', {}, 'connected operation'),
                     (['Math', 'Set'], 'Operation', {}, 'leaving and re-entering'),
                     (['Math'], 'Run', {}, 'already exists'),
                     (['Math'], 'Operation', {'Math.out:Missing': 'X'}, 'outside'),
                     (['Math'], 'Operation', {'Entry.out:Input': 'X', 'Math.out:Result': 'X'}, 'Duplicate boundary'),
                     (['Missing'], 'Operation', {}, 'existing, distinct'),
                     (['Math', 'Math'], 'Operation', {}, 'existing, distinct')]
        # Add an external path from Math through Terminal and back into Set for the cycle case.
        for selected, name, names, error in scenarios:
            with self.subTest(selected=selected, name=name, names=names):
                source = fixture()
                if error == 'leaving and re-entering':
                    source['graphs'][0]['body']['links'].append(dict(zip(('from', 'to'), ('Terminal.out:then', 'Set.in:exec'))))
                snapshot = copy.deepcopy(source)
                with self.assertRaisesRegex(ValueError, error):
                    collapse(source, 'Run', selected, name, names)
                self.assertEqual(source, snapshot)

    def test_multi_entry_and_continuation_rejected(self):
        source = fixture()
        source['graphs'][0]['body']['links'].append(dict(zip(('from', 'to'), ('Entry.out:then', 'Set.in:exec'))))
        with self.assertRaisesRegex(ValueError, 'one execution entry'):
            collapse(source, 'Run', ['Branch', 'Set', 'Terminal', 'Math'], 'Operation')
        source = fixture()
        with self.assertRaisesRegex(ValueError, 'one continuation'):
            collapse(source, 'Run', ['Branch'], 'Operation')

    def test_unsupported_type_rejected_and_typed_object_and_array_preserved(self):
        for metadata, accepted in [({'PinType.PinCategory': '"wildcard"'}, False),
                                   ({'PinType.PinCategory': '"int"', 'PinType.bIsReference': 'True'}, False),
                                   ({'PinType.PinCategory': '"int"', 'PinType.ContainerType': 'Array'}, True),
                                   ({'PinType.PinCategory': '"object"', 'PinType.PinSubCategoryObject': '"/Script/CoreUObject.Class\'/Script/Engine.StaticMesh\'"'}, True)]:
            source = fixture()
            source['pinTemplates']['int'] = metadata
            if accepted:
                after, result = collapse(source, 'Run', ['Math'], 'Typed')
                verify_inverse(source, after, result['childGraph'])
            else:
                with self.assertRaises(ValueError):
                    collapse(source, 'Run', ['Math'], 'Typed')

    def test_inverse_check_detects_payload_and_edge_changes(self):
        source = fixture()
        after, result = collapse(source, 'Run', ['Math'], 'Calculate')
        child = select_graph(after, result['childGraph'])
        child['body']['nodes'][1]['properties']['NodeComment'] = '"mutation"'
        with self.assertRaisesRegex(ValueError, 'inverse check'):
            verify_inverse(source, after, result['childGraph'])

    def test_existing_derived_object_edge_carries_source_type(self):
        source = fixture()
        source['pinTemplates']['int'] = {'PinType.PinCategory': '"object"',
            'PinType.PinSubCategoryObject': '"/Script/CoreUObject.Class\'/Script/Engine.Actor\'"'}
        source['pinTemplates']['derived'] = dict(source['pinTemplates']['int'], **{
            'PinType.PinSubCategoryObject': '"/Script/CoreUObject.Class\'/Script/Engine.Pawn\'"'})
        source['graphs'][0]['body']['nodes'][0]['pins']['out:Input']['type'] = 'derived'
        after, result = collapse(source, 'Run', ['Math'], 'ObjectOperation')
        entry = select_graph(after, result['childGraph'])['body']['nodes'][0]
        pin = next(iter(entry['pins'].values()))
        self.assertEqual(after['pinTemplates'][pin['type']]['PinType.PinSubCategoryObject'],
                         source['pinTemplates']['derived']['PinType.PinSubCategoryObject'])
        verify_inverse(source, after, result['childGraph'])

    def test_review_retains_prior_pin_metadata_edits(self):
        source = fixture()
        edited = copy.deepcopy(source)
        edited['pinTemplates']['bool']['bAdvancedView'] = 'True'
        after, _ = collapse(edited, 'Run', ['Math'], 'Calculate')
        marks = review_marks(source, after)['A'*32]
        self.assertIn('Branch', marks['changed'])


class CollapseWorkspaceTests(unittest.TestCase):
    def setUp(self):
        bw.OWNER.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=bw.OWNER)
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.project = root/'Project/Test.uproject'
        bw.write(self.project, {})
        self.ws = bw.Workspace(root/'Workspace', root/'NoEngine')
        self.source = self.ws.root/'blueprints/Test.json'
        self.original = fixture()
        bw.write(self.source, self.original)
        bw.write(self.ws.manifest, dict(schema=1, project=str(self.project), assets=[dict(source='blueprints/Test.json')]))
        bw.write(self.ws.internal/'baseline.json', {'/Game/Test': dict(before='before.json')})
        bw.write(self.ws.internal/'before.json', self.original)
        self.asset = bw.target(self.project, '/Game/Test')
        self.asset.parent.mkdir(parents=True)
        self.asset.write_bytes(b'accepted project bytes')

    def test_public_cli_offline_and_cumulative_review(self):
        with patch.object(bw, 'project_operation', side_effect=AssertionError('Unreal must not launch')), redirect_stdout(io.StringIO()) as stdout:
            for selected, name in [('Math', 'Calculate'), ('Set', 'Assign')]:
                with patch.object(sys, 'argv', ['tool', 'Collapse', '--workspace', str(self.ws.root), '--graph', 'Run', '--node', selected, '--name', name]):
                    self.assertEqual(bw.main(), 0)
        results = [json.loads(line) for line in stdout.getvalue().splitlines()]
        self.assertEqual([r['status'] for r in results], ['collapsed', 'collapsed'])
        marks = bw.read(self.ws.manifest)['assets'][0]['reviews']['A'*32]
        self.assertEqual(marks['removed'], ['Math', 'Set'])
        self.assertEqual(marks['added'], ['K2Node_Composite_Assign', 'K2Node_Composite_Calculate'])
        self.assertEqual(self.asset.read_bytes(), b'accepted project bytes')
        source_bytes = self.source.read_bytes()
        with self.assertRaises(ValueError):
            self.ws.collapse(None, 'Run', ['Set'], 'Assign')
        self.assertEqual(self.source.read_bytes(), source_bytes)

    def test_failed_pair_write_restores_exact_bytes(self):
        files = {p: p.read_bytes() for p in (self.source, self.ws.manifest, self.asset)}
        original_write = bw.write
        def fail(path, value):
            if path == self.ws.manifest:
                raise OSError('injected manifest failure')
            original_write(path, value)
        with patch.object(bw, 'write', side_effect=fail), self.assertRaisesRegex(OSError, 'injected'):
            self.ws.collapse(None, 'Run', ['Math'], 'Calculate')
        for path, data in files.items():
            self.assertEqual(path.read_bytes(), data)
        self.assertFalse((self.ws.internal/'edit-recovery.json').exists())

    def test_crashed_pair_recovery_and_conflict(self):
        previous = self.source.read_bytes().decode('utf-8')
        row = dict(path='blueprints/Test.json', before=previous, after='{}\n')
        bw.write(self.ws.internal/'edit-recovery.json', dict(files=[row]))
        self.source.write_bytes(b'{}\n')
        self.ws.recover_edit()
        self.assertEqual(self.source.read_bytes(), previous.encode('utf-8'))
        bw.write(self.ws.internal/'edit-recovery.json', dict(files=[row]))
        self.source.write_text('new unrelated work', encoding='utf-8')
        with self.assertRaisesRegex(ValueError, 'conflicts'):
            self.ws.recover_edit()
        self.assertEqual(self.source.read_text(encoding='utf-8'), 'new unrelated work')

    def test_added_asset_after_apply_has_no_before_marks(self):
        bw.write(self.ws.internal/'baseline.json', {'/Game/Test': dict(before=None)})
        self.ws.collapse(None, 'Run', ['Math'], 'Calculate')
        self.assertNotIn('reviews', bw.read(self.ws.manifest)['assets'][0])


if __name__ == '__main__':
    unittest.main()
