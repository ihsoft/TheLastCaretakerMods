"""DataAsset value edits, protected-state rejection and transaction rollback."""
import copy
from contextlib import nullcontext, redirect_stdout
import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import blueprint_workspace as bw
from data_asset_workspace import DataAssetWorkspace, changes
from data_asset_preview import compare, render


def source():
    return dict(schema=2, kind='dataAsset', asset='/Game/Test/Item', objectName='Item',
                **{'class': '/Script/Example.Item'}, objects={
                    '.': {'class': '/Script/Example.Item', 'properties': {
                        'DroppedActor': dict(type='TSoftObjectPtr<UObject>', editable=True, value='/Game/Original.Original_C'),
                        'Icon': dict(type='TSoftObjectPtr<UTexture2D>', editable=True, value='/Game/Icon.Icon'),
                        'Name': dict(type='FText', editable=True, value='NSLOCTEXT("Items","Name","Item")'),
                        'Count': dict(type='int32', editable=True, value='1'),
                        'Bundle': dict(type='FAssetBundleData', editable=False, value='()')}},
                    'Generator': {'class': '/Script/Example.Generator', 'properties': {
                        'Rates': dict(type='TMap<FName,float>', editable=True, value='(("Fuel",1.000000))')}}})


class DataAssetTests(unittest.TestCase):
    def setUp(self):
        bw.OWNER.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=bw.OWNER); self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.project = self.root / 'Project/Test.uproject'; bw.write(self.project, {})
        self.original = source()
        self.file = bw.target(self.project, self.original['asset'])
        self.file.parent.mkdir(parents=True); self.file.write_bytes(b'original package')
        self.state = dict(source=self.original, preserved={'.': dict(bytes='custom serialization'), 'Generator': dict(bytes='owned identity')},
                          propertyState={key: {name: 'serialized ' + str(row['value']) for name, row in obj['properties'].items()} for key, obj in self.original['objects'].items()})
        self.ws = DataAssetWorkspace(self.root / 'workspace', self.root / 'Engine')
        self.ws.project = self.project
        self.prepare = patch.object(DataAssetWorkspace, 'prepare'); self.prepare.start(); self.addCleanup(self.prepare.stop)
        def export(asset, name):
            path = self.ws.run / (name + '.json'); bw.write(path, self.state)
            return copy.deepcopy(self.state), path
        self.export = patch.object(self.ws, 'export_state', side_effect=export); self.export.start(); self.addCleanup(self.export.stop)
        self.ws.export(self.project, [self.original['asset']])
        self.path = self.ws.root / 'data-assets/Test/Item.json'

    def proposal(self):
        doc = copy.deepcopy(self.original); doc['objects']['.']['properties']['DroppedActor']['value'] = '/Game/Child.Child_C'
        bw.write(self.path, doc); return doc

    def engine(self, mode, name, **fields):
        self.assertEqual(mode, 'DataAssetApply')
        self.assertTrue(self.file.is_file(), 'Existing DataAsset must not be deleted before editing')
        request = bw.read(fields['Input'])
        self.assertEqual(request['before']['preserved'], self.state['preserved'])
        self.state['source'] = request['after']; self.file.write_bytes(b'changed package')
        bw.write(fields['Output'], self.state)

    def test_value_changes_and_review_survives_apply(self):
        initial = self.ws.preview()
        page = (self.ws.run / 'preview/index.html').read_text(encoding='utf-8')
        self.assertEqual(initial['changedProperties'], 0)
        self.assertIn('/Game/Original.Original_C', page)
        desired = self.proposal()
        with patch.object(self.ws, 'engine_run', side_effect=self.engine):
            result = self.ws.apply()
            self.assertEqual(result['changes'][0]['properties'], [dict(object='.', property='DroppedActor', before='/Game/Original.Original_C', after='/Game/Child.Child_C')])
            self.assertEqual(self.ws.apply()['status'], 'unchanged')
        self.assertEqual(bw.read(self.path), desired)
        self.assertEqual(self.ws.preview()['changedProperties'], 1)
        self.assertFalse((self.ws.internal / 'recovery.json').exists())

    def test_protected_owned_object_change_rolls_back_package_and_baseline(self):
        self.proposal(); baseline = self.ws.baseline()
        def corrupt(*args, **kwargs):
            self.engine(*args, **kwargs); self.state['preserved']['Generator']['bytes'] = 'changed custom state'
        with patch.object(self.ws, 'engine_run', side_effect=corrupt):
            with self.assertRaisesRegex(ValueError, 'beyond requested properties'):
                self.ws.apply()
        self.assertEqual(self.file.read_bytes(), b'original package')
        self.assertEqual(self.ws.baseline(), baseline)
        self.assertEqual(bw.read(self.path)['objects']['.']['properties']['DroppedActor']['value'], '/Game/Child.Child_C')
        self.assertTrue(self.ws.recovery_candidates.is_dir())

    def test_concurrent_project_change_rejected_before_engine(self):
        self.proposal(); self.file.write_bytes(b'external edit')
        with patch.object(self.ws, 'engine_run') as engine:
            with self.assertRaisesRegex(ValueError, 'changed since export'):
                self.ws.apply()
            engine.assert_not_called()
        self.assertEqual(self.file.read_bytes(), b'external edit')

    def test_conflict_after_backup_does_not_restore_over_external_edit(self):
        self.proposal(); original_check = self.ws.check; calls = 0
        def check(entries):
            nonlocal calls
            calls += 1
            if calls == 3:
                self.file.write_bytes(b'late external edit')
            return original_check(entries)
        with patch.object(self.ws, 'check', side_effect=check), patch.object(self.ws, 'engine_run') as engine:
            with self.assertRaisesRegex(ValueError, 'changed since export'):
                self.ws.apply()
            engine.assert_not_called()
        self.assertEqual(self.file.read_bytes(), b'late external edit')
        self.assertFalse((self.ws.internal / 'recovery.json').exists())

    def test_incomplete_or_expanded_proposal_cannot_reset_existing_fields(self):
        for mutation in ('omit', 'new', 'class', 'identity', 'extra', 'readonly', 'object'):
            doc = copy.deepcopy(self.original)
            props = doc['objects']['.']['properties']
            if mutation == 'omit': del props['Icon']
            if mutation == 'new': props['Unknown'] = dict(type='int32', editable=True, value='1')
            if mutation == 'class': props['Icon']['type'] = 'int32'
            if mutation == 'identity': doc['objectName'] = 'Different'
            if mutation == 'extra': doc['properties'] = {'ReadOnly': 'replacement'}
            if mutation == 'readonly': props['Bundle']['value'] = 'changed'
            if mutation == 'object': del doc['objects']['Generator']
            with self.subTest(mutation=mutation), self.assertRaises(ValueError): changes(self.original, doc)

    def test_explicit_clear_is_a_change_not_an_omission(self):
        doc = copy.deepcopy(self.original); doc['objects']['.']['properties']['Icon']['value'] = 'None'
        self.assertEqual(changes(self.original, doc)[0]['after'], 'None')

    def test_public_preview_dispatch_uses_dataasset_manifest(self):
        self.proposal(); output = io.StringIO()
        with patch('sys.argv', ['bpt', 'Preview', '--workspace', str(self.ws.root)]), \
             patch.object(bw, 'project_operation', return_value=nullcontext()) as engine_gate, redirect_stdout(output):
            self.assertEqual(bw.main(), 0)
            engine_gate.assert_not_called()
        result = json.loads(output.getvalue())
        self.assertEqual(result['changedProperties'], 1)
        self.assertEqual(self.file.read_bytes(), b'original package')

    def test_source_change_during_apply_restores_package_and_preserves_new_proposal(self):
        self.proposal()
        def concurrent(*args, **kwargs):
            self.engine(*args, **kwargs)
            doc = bw.read(self.path); doc['objects']['.']['properties']['DroppedActor']['value'] = '/Game/Other.Other_C'; bw.write(self.path, doc)
        with patch.object(self.ws, 'engine_run', side_effect=concurrent):
            with self.assertRaisesRegex(ValueError, 'proposal changed'):
                self.ws.apply()
        self.assertEqual(self.file.read_bytes(), b'original package')
        self.assertEqual(bw.read(self.path)['objects']['.']['properties']['DroppedActor']['value'], '/Game/Other.Other_C')

    def test_root_and_owned_collection_values_visible_in_pending_preview(self):
        doc = copy.deepcopy(self.original)
        doc['objects']['.']['properties']['Count']['value'] = '3'
        doc['objects']['Generator']['properties']['Rates']['value'] = '(("Fuel",2.000000),("Water",4.000000))'
        bw.write(self.path, doc)
        self.assertEqual(self.ws.preview()['changedProperties'], 2)
        page = (self.ws.run / 'preview/index.html').read_text(encoding='utf-8')
        self.assertIn('Generator / Rates', page)
        self.assertIn('Water', page)
        self.assertIn('read-only', page)
        with patch.object(self.ws, 'engine_run', side_effect=self.engine):
            self.assertEqual(self.ws.apply()['changed'], 1)

    def test_untouched_text_hidden_state_loss_is_rejected(self):
        self.proposal()
        def corrupt(*args, **kwargs):
            self.engine(*args, **kwargs); self.state['propertyState']['.']['Name'] = 'lost text flags'
        with patch.object(self.ws, 'engine_run', side_effect=corrupt), self.assertRaisesRegex(ValueError, 'Untouched property serialization'):
            self.ws.apply()
        self.assertEqual(self.file.read_bytes(), b'original package')

    def test_changed_property_must_reopen_with_exact_expected_serialization(self):
        self.proposal()
        def corrupt(*args, **kwargs):
            self.engine(*args, **kwargs); self.state['propertyState']['.']['DroppedActor'] = 'different after save'
        with patch.object(self.ws, 'engine_run', side_effect=corrupt), self.assertRaisesRegex(ValueError, 'serialization changed after fresh reopen'):
            self.ws.apply()
        self.assertEqual(self.file.read_bytes(), b'original package')

    def test_external_review_baseline_does_not_become_apply_baseline(self):
        reference = self.root / 'stock'
        stock = copy.deepcopy(self.original)
        stock['objects']['.']['properties']['DroppedActor']['value'] = '/Game/Stock.Stock_C'
        bw.write(reference / 'workspace.json', dict(schema=1, kind='dataAssets'))
        bw.write(reference / '.workspace/before/0.json', stock)
        bw.write(reference / '.workspace/baseline.json', {stock['asset']: dict(before='before/0.json', accepted='missing.json')})
        # Later proposals in the reference workspace are not the exported baseline.
        bw.write(reference / 'data-assets/Test/Item.json', self.original)
        baseline = self.ws.baseline()
        output = io.StringIO()
        with patch('sys.argv', ['bpt', 'Preview', '--workspace', str(self.ws.root), '--comparison-workspace', str(reference), '--comparison-label', 'Stock <item>']), \
             patch.object(bw, 'project_operation') as gate, redirect_stdout(output):
            self.assertEqual(bw.main(), 0)
            gate.assert_not_called()
        result = json.loads(output.getvalue())
        self.assertEqual(result['changedProperties'], 1)
        page = (Path(result['evidence']) / 'preview/index.html').read_text(encoding='utf-8')
        self.assertIn('Compared with: Stock &lt;item&gt;', page)
        self.assertIn('/Game/Stock.Stock_C', page)
        self.assertIn('class="changed"', page)
        self.assertIn('Full asset</button>', page)
        self.assertEqual(self.ws.baseline(), baseline)
        self.assertEqual(bw.read(self.path), self.original)
        with patch.object(self.ws, 'engine_run') as engine:
            self.assertEqual(self.ws.apply()['status'], 'unchanged')
            engine.assert_not_called()

    def test_comparison_includes_read_only_values_and_rejects_incompatible_shape(self):
        before = copy.deepcopy(self.original)
        before['objects']['.']['properties']['Bundle']['value'] = '(old)'
        self.assertEqual(compare(before, self.original), [dict(object='.', property='Bundle', before='(old)', after='()')])
        before['objects']['Generator']['properties']['Rates']['type'] = 'float'
        with self.assertRaisesRegex(ValueError, 'type differs'):
            compare(before, self.original)
        del before['objects']['Generator']
        with self.assertRaisesRegex(ValueError, 'inventory differs'):
            compare(before, self.original)

    def test_missing_baseline_is_not_reported_as_no_differences(self):
        page, count = render([dict(document=self.original, before=None, pending=False)], None)
        self.assertEqual(count, 0)
        self.assertIn('No comparison baseline', page)
        self.assertNotIn('No differences from the comparison baseline', page)
        self.assertIn('data-view="full" aria-pressed="true"', page)
        self.assertIn('data-mode="changes" hidden', page)
        self.assertIn('data-mode="full">', page)
        baseline = self.ws.baseline()
        baseline[self.original['asset']].pop('before')
        bw.write(self.ws.internal / 'baseline.json', baseline)
        self.assertIsNone(self.ws.preview()['comparison'])

    def test_comparison_label_requires_a_real_reference_selection(self):
        with self.assertRaisesRegex(ValueError, 'supplied together'):
            self.ws.preview(comparison_label='Stock game')
        with self.assertRaisesRegex(ValueError, 'supplied together'):
            self.ws.preview(comparison_workspace=self.ws.root)


if __name__ == '__main__':
    unittest.main()
