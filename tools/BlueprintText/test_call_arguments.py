"""Intent preservation, legacy transition, review and rejection at the codec boundary."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import call_arguments as args
import parameter_defaults
from asset_text_codec import compact, native, add_call, compare, compare_materialized
from test_asset_text_codec import fixture
from test_call_templates import catalog, OPERATION
from graph_review import review_marks
from render_graph import node_lines
from preview_references import with_type_reference_details
from graph_inspection import compact_pin


class ArgumentTests(unittest.TestCase):
    def setUp(self):
        self.doc = add_call(compact(fixture()), catalog(), OPERATION, 'Call', 'Calculate')
        self.node = next(n for n in self.doc['graphs'][0]['body']['nodes'] if n['id'] == 'Call')
        self.key = 'in:bUpdateOverlaps'

    def test_new_factory_omits_optional_literals_and_roundtrips_intent(self):
        self.assertEqual(self.node['arguments'], {})
        self.assertNotIn(self.key, self.node.get('defaults', {}))
        self.assertEqual(compare(self.doc, compact(native(self.doc))), [])
        self.assertNotIn(args.CALL_CLASS, native(self.doc)['graphs'][0]['text'])
        self.assertIn(args.LEGACY_CLASS, native(self.doc)['graphs'][0]['text'])
        self.assertNotIn(args.STATE, native(self.doc)['graphs'][0]['text'])

    def test_explicit_equal_is_not_inheritance(self):
        before = copy.deepcopy(self.doc)
        self.node['arguments'][self.key] = 'true'
        self.assertTrue(compare(before, self.doc))
        result = compact(native(self.doc))
        self.assertEqual(next(n for n in result['graphs'][0]['body']['nodes'] if n['id']=='Call')['arguments'], {self.key:'true'})

    def test_import_comparison_cli_accepts_native_equal_default_normalization(self):
        from blueprint_workspace import OWNER, write
        actual = copy.deepcopy(self.doc)
        self.node['arguments'][self.key] = 'true'
        with tempfile.TemporaryDirectory(dir=OWNER) as directory:
            root = Path(directory)
            write(root/'source.json', self.doc)
            write(root/'actual.json', actual)
            write(root/'batch.json', {'assets':['source.json']})
            write(root/'reopened/0.native.json', native(actual))
            for mode, source, saved in (('compare','source.json','actual.json'),
                                        ('batch-compare','batch.json','reopened')):
                command = [sys.executable, '-B', str(Path(__file__).with_name('asset_text_codec.py')),
                           mode, str(root/source), '--actual', str(root/saved), '--output', str(root/'result.json')]
                strict = subprocess.run(command, capture_output=True, text=True)
                self.assertEqual(strict.returncode, 1, strict.stderr)
                materialized = subprocess.run(command + ['--materialized'], capture_output=True, text=True)
                self.assertEqual(materialized.returncode, 0, materialized.stderr)
                self.assertTrue(json.loads((root/'result.json').read_text())['equal'])

    def test_materialized_comparison_resolves_omission_from_declaration_not_actual_pin(self):
        actual = copy.deepcopy(self.doc)
        saved = next(n for n in actual['graphs'][0]['body']['nodes'] if n['id']=='Call')
        saved['arguments'][self.key] = 'true'
        self.assertEqual(compare_materialized(self.doc, actual), [])
        saved['arguments'][self.key] = 'false'
        self.assertTrue(compare_materialized(self.doc, actual))
        self.node['arguments'][self.key] = 'false'
        self.assertEqual(compare_materialized(self.doc, actual), [])
        saved['arguments'][self.key] = 'true'
        self.assertTrue(compare_materialized(self.doc, actual))

    def test_materialized_comparison_rejects_unwired_missing_required(self):
        actual = copy.deepcopy(self.doc)
        saved = next(n for n in actual['graphs'][0]['body']['nodes'] if n['id']=='Call')
        saved['declarationDefaults'].clear()
        saved['arguments'][self.key] = 'false'
        with self.assertRaisesRegex(ValueError, 'Missing required argument'):
            compare_materialized(self.doc, actual)

    def test_legacy_node_migration_uses_stored_value_not_serialized_intent(self):
        source = native(self.doc)
        graph = source['graphs'][0]
        graph.pop('callArguments')
        graph['text'] = graph['text'].replace(args.LEGACY_CLASS, args.CALL_CLASS)
        result = compact(source)
        node = next(n for n in result['graphs'][0]['body']['nodes'] if n['id']=='Call')
        self.assertEqual(node['arguments'][self.key], 'true')
        rebuilt = native(result)['graphs'][0]['text']
        self.assertNotIn(args.CALL_CLASS, rebuilt)
        self.assertNotIn(args.STATE, rebuilt)

    def test_legacy_migration_preserves_literals(self):
        self.node.pop('arguments')
        self.node.pop('declarationDefaults')
        self.node['defaults'] = {self.key:'true'}
        legacy = copy.deepcopy(self.doc)
        args.enable(self.node, self.doc['pinTemplates'])
        self.assertEqual(self.node['arguments'], {self.key:'true'})
        self.assertTrue(compare(legacy, self.doc))
        self.assertEqual(compact(native(legacy))['graphs'][0]['body']['nodes'][0].get('arguments'), None)

    def test_unsupported_and_ambiguous_arguments_rejected(self):
        for key in ('out:result', 'in:Missing', 'in:Required'):
            with self.subTest(key=key):
                self.node['arguments'] = {key:'0'}
                with self.assertRaisesRegex(ValueError, 'Unsupported argument'):
                    native(self.doc)
        self.node['arguments'] = {}
        self.node['defaults'] = {self.key:'false'}
        with self.assertRaisesRegex(ValueError, 'Use arguments'):
            native(self.doc)

    def test_review_shows_override_and_details_resolve_default(self):
        self.assertFalse(any('UpdateOverlaps =' in line for line in node_lines(self.node,set(),pin_templates=self.doc['pinTemplates'])))
        details = with_type_reference_details(self.node,self.doc['pinTemplates'])['_argumentDetails'][self.key]
        self.assertEqual(details['mode'], 'declaration-default')
        self.assertEqual(details['declarationDefault'], 'true')
        read = compact_pin(self.doc,self.node,self.key,set())
        self.assertEqual(read['argumentMode'], 'declaration-default')
        self.node['arguments'][self.key] = 'true'
        self.assertTrue(any('UpdateOverlaps =' in line for line in node_lines(self.node,set(),pin_templates=self.doc['pinTemplates'])))

    def test_declaration_snapshot_changes_mark_review_but_not_override_intent(self):
        before = copy.deepcopy(self.doc)
        self.doc['pinTemplates'][self.node['pins'][self.key]['type']]['AutogeneratedDefaultValue'] = '"false"'
        self.assertEqual(compare(before,self.doc), [])
        self.assertIn('Call', next(iter(review_marks(before,self.doc).values()))['changed'])

    def test_arithmetic_templates_keep_all_operands_explicit(self):
        source=catalog()
        source['entries'][0]['nativeInsertion']=source['entries'][0]['nativeInsertion'].replace('Engine.BoxComponent','Engine.KismetMathLibrary')
        doc=add_call(compact(fixture()),source,OPERATION,'Math','Calculate')
        node=next(n for n in doc['graphs'][0]['body']['nodes'] if n['id']=='Math')
        self.assertEqual(node['arguments'],{self.key:'true'})

    def test_unused_declaration_change_does_not_mark_fixed_override(self):
        self.node['arguments'][self.key]='true'
        before=copy.deepcopy(self.doc)
        self.node['declarationDefaults'][self.key]='false'
        self.doc['pinTemplates'][self.node['pins'][self.key]['type']]['AutogeneratedDefaultValue']='"false"'
        self.assertEqual(review_marks(before,self.doc),{})

    def test_declaration_distinguishes_empty_default_from_required(self):
        doc=compact(fixture())
        entry=next(n for n in doc['graphs'][0]['body']['nodes'] if n['kind']=='FunctionEntry')
        key=entry['pins']['out:Result']['type']
        doc['pinTemplates'][key]['PinType.PinCategory']='"string"'
        doc['pinTemplates'][key]['PinType.PinSubCategory']='""'
        parameter_defaults.enable(entry,doc['pinTemplates'])
        entry['parameterDefaults']={'out:Result':''}
        empty=compact(native(doc))
        self.assertEqual(empty['graphs'][0]['body']['nodes'][0]['parameterDefaults'],{'out:Result':''})
        entry['parameterDefaults']={}
        required=compact(native(doc))
        self.assertEqual(required['graphs'][0]['body']['nodes'][0]['parameterDefaults'],{})
        self.assertTrue(compare(empty,required))

    def test_proposed_default_change_updates_only_review_copy(self):
        self.node['properties']['FunctionReference']='(MemberParent="/Script/Engine.BlueprintGeneratedClass\'/Game/Defaults.Defaults_C\'",MemberName="Calculate")'
        callee=compact(fixture());callee['asset']='/Game/Defaults'
        entry=next(n for n in callee['graphs'][0]['body']['nodes'] if n['kind']=='FunctionEntry')
        entry['pins']={'out:bUpdateOverlaps':{'id':'A'*32,'type':'bool'}}
        callee['pinTemplates']['bool']={'PinType.PinCategory':'"bool"'}
        entry['defaults']={'out:bUpdateOverlaps':'true'}
        parameter_defaults.enable(entry,callee['pinTemplates'])
        entry['parameterDefaults']['out:bUpdateOverlaps']='false'
        original=copy.deepcopy(self.doc)
        view=args.preview_documents([self.doc,callee])[0]
        node=next(n for n in view['graphs'][0]['body']['nodes'] if n['id']=='Call')
        self.assertEqual(node['declarationDefaults'][self.key],'false')
        self.assertEqual(self.doc,original)


if __name__ == '__main__':
    unittest.main()
