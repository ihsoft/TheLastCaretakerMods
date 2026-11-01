"""Offline transaction, conflict, recovery, grouping and input safety checks."""
import copy
from contextlib import redirect_stdout, nullcontext
import io
import json
import subprocess
import sys
import tempfile
from pathlib import Path
import unittest
import threading
import time
from unittest.mock import patch

import blueprint_workspace as bw
from test_asset_text_codec import fixture


class WorkspaceTests(unittest.TestCase):
    def setUp(self):
        bw.OWNER.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=bw.OWNER)
        self.root = Path(self.temp.name)
        self.queue_mock=patch.object(bw,'engine_slot',return_value=nullcontext())
        self.queue_mock.start();self.addCleanup(self.queue_mock.stop)
        for name, value in [('prepare_project', []), ('prepare_provider', {'built': False})]:
            mock = patch.object(bw, name, return_value=value)
            mock.start(); self.addCleanup(mock.stop)
        self.project = self.root / 'Project/Test.uproject'
        bw.write(self.project, {})
        self.ws = bw.Workspace(self.root / 'Session', self.root / 'Engine')
        self.ws.project = self.project
        self.document = bw.codec.compact(fixture())
        self.source = self.ws.root / 'blueprints/BP_Test.json'
        bw.write(self.source, self.document)
        self.file = bw.target(self.project, self.document['asset'])
        self.file.parent.mkdir(parents=True)
        self.file.write_bytes(b'original')
        self.backup = self.ws.internal / 'original.uasset'
        self.backup.parent.mkdir(parents=True, exist_ok=True)
        self.backup.write_bytes(b'original')
        bw.write(self.ws.internal / 'before.json', self.document)
        bw.write(self.ws.internal / 'baseline.json', {self.document['asset']: dict(source='blueprints/BP_Test.json', bytes='original.uasset', before='before.json', accepted='before.json')})
        bw.write(self.ws.manifest, dict(schema=1, project=str(self.project), assets=[dict(source='blueprints/BP_Test.json')]))

    def tearDown(self):
        # Windows may briefly report a directory nonempty while its deleted
        # children are still closing. Retry only that transient cleanup error;
        # never suppress test failures or persistent filesystem errors.
        for attempt in range(4):
            try:
                self.temp.cleanup()
                return
            except OSError as exc:
                if getattr(exc, 'winerror', None) != 145 or attempt == 3:
                    raise
                time.sleep(0.1)

    def changed(self):
        source = copy.deepcopy(self.document)
        source['defaults']['Amount'] = '2.000000'
        bw.write(self.source, source)
        return source

    def library(self):
        document = copy.deepcopy(self.document)
        document.pop('root')
        document['parent'] = '/Script/CoreUObject.Object'
        path = self.root / 'library.json'
        bw.write(path, document)
        return path

    def test_add_source_initializes_empty_workspace_without_engine_or_asset_writes(self):
        workspace = bw.Workspace(self.root / 'NewSession', self.root / 'Engine')
        asset = '/Game/Mods/Example/BP_Test'
        with patch.object(workspace, 'command') as engine:
            result = workspace.add_source(self.library(), asset, self.project)
        engine.assert_not_called()
        self.assertFalse(result['applied'])
        self.assertEqual(workspace.baseline(), {})
        _, entries = workspace.load()
        self.assertEqual(entries[0][2]['asset'], asset)
        self.assertFalse(bw.target(self.project, asset).exists())

    def test_add_source_preserves_existing_baseline_and_rejects_duplicate(self):
        before = self.ws.baseline()
        library = self.library()
        self.ws.add_source(library, '/Game/Mods/Example/BP_Test')
        self.assertEqual(self.ws.baseline(), before)
        self.assertEqual(len(self.ws.load()[1]), 2)
        with self.assertRaisesRegex(ValueError, 'already selected'):
            self.ws.add_source(library, '/Game/Mods/Example/BP_Test')

    def test_add_source_rejects_existing_asset_and_project_mismatch(self):
        library = self.library()
        asset = '/Game/Mods/Example/BP_Test'
        existing = bw.target(self.project, asset)
        existing.parent.mkdir(parents=True)
        existing.write_bytes(b'owned elsewhere')
        with self.assertRaisesRegex(ValueError, 'already exists'):
            self.ws.add_source(library, asset)
        self.assertEqual(existing.read_bytes(), b'owned elsewhere')
        with self.assertRaisesRegex(ValueError, 'disagrees'):
            self.ws.add_source(library, asset, self.root / 'Other.uproject')

    def test_ready_provider_prepares_project_dependencies_without_project_build(self):
        events = []
        with patch.object(bw, 'prepare_project', side_effect=lambda *args: events.append('project') or []) as dependencies, \
             patch.object(bw, 'prepare_provider', side_effect=lambda *args: events.append('bpt') or {'built': True}) as provider, \
             patch.object(self.ws, 'command') as build:
            result = self.ws.prepare(self.project)
        self.assertEqual(events, ['project', 'bpt'])
        self.assertTrue(result['binaryProvider']['built'])
        self.assertEqual(provider.call_args.args[3].name, 'BlueprintTextWorkspace.uplugin')
        build.assert_not_called()
        self.assertFalse((self.project.parent / 'Source').exists())

    def test_ready_provider_failure_prevents_export(self):
        # Export must be a complete cold-start entry point, not require Prepare first.
        self.ws = bw.Workspace(self.root / 'ColdExport', self.root / 'Engine')
        with patch.object(bw, 'prepare_provider', side_effect=RuntimeError('provider failed')), \
             patch.object(self.ws, 'export_one') as export:
            with self.assertRaisesRegex(RuntimeError, 'provider failed'):
                self.ws.export(self.project, [self.document['asset']])
        export.assert_not_called()

    def test_prepare_refresh_preserves_existing_module_and_reuses_current(self):
        module = self.project.parent / 'Source/BlueprintText'
        module.mkdir(parents=True)
        (module / 'BlueprintText.Build.cs').write_text('previous shared module')
        (module / 'RemovedOldSource.cpp').write_bytes(b'previous customized source')
        native = self.project.parent / 'Source/ExistingNative/ExistingNative.Build.cs'
        native.parent.mkdir(parents=True)
        native.write_bytes(b'preserve unrelated native mirror')
        with patch.object(self.ws, 'command'):
            result = self.ws.prepare(self.project)
        self.assertTrue(result['reusedModule'])
        self.assertTrue(result['refreshedModule'])
        backup = Path(result['moduleBackup'])
        self.assertEqual('previous shared module', (backup / 'BlueprintText.Build.cs').read_text())
        self.assertEqual(b'previous customized source', (backup / 'RemovedOldSource.cpp').read_bytes())
        self.assertFalse((module / 'RemovedOldSource.cpp').exists())
        self.assertEqual(b'preserve unrelated native mirror', native.read_bytes())
        template = bw.HERE / 'ProjectTemplate/Source/BlueprintText'
        self.assertEqual({p.relative_to(template): p.read_bytes() for p in template.rglob('*') if p.is_file()},
                         {p.relative_to(module): p.read_bytes() for p in module.rglob('*') if p.is_file()})
        with patch.object(self.ws, 'command'):
            repeated = self.ws.prepare(self.project)
        self.assertTrue(repeated['reusedModule'])
        self.assertFalse(repeated['refreshedModule'])
        self.assertIsNone(repeated['moduleBackup'])

    def test_prepare_refreshes_legacy_identity_without_duplicate_provider(self):
        identity = bw.MODULE_NAMES[1]
        module = self.project.parent / 'Source' / identity
        module.mkdir(parents=True)
        (module / (identity + '.Build.cs')).write_bytes(b'previous legacy module')
        (module / 'Old.cpp').write_bytes(b'custom old source')
        with patch.object(self.ws, 'command'):
            result = self.ws.prepare(self.project)
        self.assertEqual(identity, result['moduleIdentity'])
        self.assertTrue(result['refreshedModule'])
        self.assertEqual(b'custom old source', (Path(result['moduleBackup']) / 'Old.cpp').read_bytes())
        self.assertFalse((module.parent / 'BlueprintText').exists())
        expected = bw.module_template(identity)
        self.assertEqual(expected, {p.relative_to(module): p.read_bytes() for p in module.rglob('*') if p.is_file()})
        self.assertIn(('class ' + identity + ' : ModuleRules').encode(), expected[Path(identity + '.Build.cs')])
        self.assertIn(('IMPLEMENT_MODULE(FDefaultModuleImpl, ' + identity + ')').encode(), expected[Path(identity + '.cpp')])
        self.assertIn(b'UBlueprintTextCommandlet', expected[Path('BlueprintTextCommandlet.h')])

    def test_prepare_refuses_both_module_identities_before_mutation(self):
        for identity in bw.MODULE_NAMES:
            module = self.project.parent / 'Source' / identity
            module.mkdir(parents=True)
            (module / (identity + '.Build.cs')).write_bytes(b'preserve')
        with patch.object(self.ws, 'command') as build:
            with self.assertRaisesRegex(ValueError, 'Multiple Blueprint text module providers'):
                self.ws.prepare(self.project)
            build.assert_not_called()
        with patch.object(bw, 'OWNER', self.root / 'CurrentGate'), patch.object(bw, 'LEGACY_OWNER', self.root / 'LegacyGate'):
            with self.assertRaisesRegex(ValueError, 'Multiple Blueprint text module providers'):
                with bw.project_operation(self.project):
                    self.fail('Duplicate commandlet providers accepted for engine work')
        for identity in bw.MODULE_NAMES:
            self.assertEqual(b'preserve', (self.project.parent / 'Source' / identity / (identity + '.Build.cs')).read_bytes())

    def test_legacy_workspace_apply_recover_and_shared_engine_gate(self):
        legacy_owner, current_owner = self.root / 'PreviousOwner', self.root / 'CurrentOwner'
        legacy_root = legacy_owner / 'Session'
        bw.shutil.copytree(self.ws.root, legacy_root)
        with patch.object(bw, 'OWNER', current_owner), patch.object(bw, 'LEGACY_OWNER', legacy_owner), patch.object(bw, 'closed'):
            for action, status in [('Apply', 'unchanged'), ('Recover', 'no-recovery-needed')]:
                output = io.StringIO()
                with patch.object(sys, 'argv', ['workspace', action, '--workspace', str(legacy_root)]), redirect_stdout(output):
                    self.assertEqual(0, bw.main())
                self.assertEqual(status, json.loads(output.getvalue().splitlines()[-1])['status'])
            self.assert_waits_for_lock(legacy_owner / '.engine-operation.lock')

    def assert_waits_for_lock(self, path):
        release=self.root/'release-lock';release.unlink(missing_ok=True)
        program='from pathlib import Path; import sys,time; from blueprint_workspace import lock\nwith lock(Path(sys.argv[1])):\n print("ready",flush=True)\n while not Path(sys.argv[2]).exists():time.sleep(.02)\n'
        process=subprocess.Popen([sys.executable,'-B','-c',program,str(path),str(release)],cwd=bw.HERE,stdout=subprocess.PIPE,text=True)
        try:
            self.assertEqual('ready',process.stdout.readline().strip())
            timer=threading.Timer(.2,release.touch);timer.start()
            with bw.project_operation(self.project):self.assertTrue(release.exists())
            timer.join();self.assertEqual(0,process.wait(timeout=10))
        finally:
            if process.poll() is None:process.kill();process.wait()
            process.stdout.close()

    def test_noop_and_conflict(self):
        self.assertEqual('unchanged', self.ws.apply()['status'])
        self.file.write_bytes(b'editor modified')
        with self.assertRaisesRegex(ValueError, 'changed since'):
            self.ws.apply()
        self.assertEqual(b'editor modified', self.file.read_bytes())

    def test_conflict_after_journal_preserves_project_and_proposal(self):
        desired = self.changed()
        new = copy.deepcopy(self.document)
        new['asset'] = '/Game/BP_New'
        new_source = self.ws.root / 'blueprints/New.json'
        bw.write(new_source, new)
        manifest = bw.read(self.ws.manifest)
        manifest['assets'].append(dict(source='blueprints/New.json'))
        bw.write(self.ws.manifest, manifest)
        new_file = bw.target(self.project, new['asset'])
        baseline_path = self.ws.internal / 'baseline.json'
        baseline_bytes = baseline_path.read_bytes()
        journal = self.ws.internal / 'recovery.json'
        original_write = bw.write
        for conflict in ('replace', 'remove', 'create'):
            with self.subTest(conflict=conflict):
                self.ws = bw.Workspace(self.ws.root, self.ws.engine)
                self.ws.project = self.project
                self.file.write_bytes(b'original')
                new_file.unlink(missing_ok=True)
                def write_with_conflict(path, data):
                    original_write(path, data)
                    if path == journal:
                        # The backup and journal exist, but Apply has not written an asset.
                        if conflict == 'replace':
                            self.file.write_bytes(b'editor modified')
                        elif conflict == 'remove':
                            self.file.unlink()
                        else:
                            new_file.write_bytes(b'editor created')
                with patch.object(bw, 'write', side_effect=write_with_conflict), \
                     patch.object(self.ws, 'engine_run') as engine:
                    with self.assertRaisesRegex(ValueError, 'changed since|overwrite an unselected'):
                        self.ws.apply()
                engine.assert_not_called()
                if conflict == 'remove':
                    self.assertFalse(self.file.exists())
                else:
                    self.assertEqual(b'editor modified' if conflict == 'replace' else b'original', self.file.read_bytes())
                if conflict == 'create':
                    self.assertTrue(new_file.exists())
                    self.assertEqual(b'editor created', new_file.read_bytes())
                else:
                    self.assertFalse(new_file.exists())
                self.assertEqual(baseline_bytes, baseline_path.read_bytes())
                self.assertEqual(desired, bw.read(self.source))
                self.assertEqual(new, bw.read(new_source))
                self.assertEqual(manifest, bw.read(self.ws.manifest))
                self.assertFalse(journal.exists())
                self.assertIsNone(self.ws.recovery_candidates)
                self.assertFalse(self.ws.recover())

    def test_recover_before_mutation_preserves_external_changes(self):
        new_asset = '/Game/BP_New'
        new_file = bw.target(self.project, new_asset)
        new_file.write_bytes(b'editor created')
        self.file.write_bytes(b'editor modified')
        baseline_path = self.ws.internal / 'baseline.json'
        baseline_bytes = baseline_path.read_bytes()
        journal = self.ws.internal / 'recovery.json'
        bw.write(journal, dict(project=str(self.project), baseline=self.ws.baseline(), restoreRequired=False,
                 assets=[dict(asset=self.document['asset'], backup='original.uasset'), dict(asset=new_asset, backup=None)]))
        # A new process must see that the interrupted Apply never entered its write phase.
        workspace = bw.Workspace(self.ws.root, self.ws.engine)
        workspace.project = self.project
        self.assertFalse(workspace.recover())
        self.assertEqual(b'editor modified', self.file.read_bytes())
        self.assertEqual(b'editor created', new_file.read_bytes())
        self.assertEqual(baseline_bytes, baseline_path.read_bytes())
        self.assertFalse(journal.exists())
        self.assertIsNone(workspace.recovery_candidates)

    def test_failure_to_record_write_phase_never_starts_asset_writes(self):
        self.changed()
        baseline = self.ws.baseline()
        journal = self.ws.internal / 'recovery.json'
        original_write = bw.write
        def fail_transition(path, data):
            if path == journal and data.get('restoreRequired') is True:
                raise OSError('cannot record write phase')
            original_write(path, data)
        with patch.object(bw, 'write', side_effect=fail_transition), \
             patch.object(self.ws, 'engine_run') as engine:
            with self.assertRaisesRegex(OSError, 'cannot record write phase'):
                self.ws.apply()
        engine.assert_not_called()
        self.assertEqual(b'original', self.file.read_bytes())
        self.assertEqual(baseline, self.ws.baseline())
        self.assertFalse(journal.exists())
        self.assertIsNone(self.ws.recovery_candidates)

    def test_recovery_refuses_invalid_write_phase(self):
        self.file.write_bytes(b'preserve current')
        journal = self.ws.internal / 'recovery.json'
        bw.write(journal, dict(project=str(self.project), baseline=self.ws.baseline(), restoreRequired='false',
                 assets=[dict(asset=self.document['asset'], backup='original.uasset')]))
        with self.assertRaisesRegex(ValueError, 'Invalid recovery restoreRequired'):
            self.ws.recover()
        self.assertEqual(b'preserve current', self.file.read_bytes())
        self.assertTrue(journal.exists())

    def test_rollback_after_deletion_before_engine_launch(self):
        self.changed()
        baseline = self.ws.baseline()
        def fail(*args, **kwargs):
            self.assertFalse(self.file.exists())
            raise KeyboardInterrupt('interrupted before engine launch')
        with patch.object(self.ws, 'engine_run', side_effect=fail):
            with self.assertRaisesRegex(KeyboardInterrupt, 'before engine launch'):
                self.ws.apply()
        self.assertEqual(b'original', self.file.read_bytes())
        self.assertEqual(baseline, self.ws.baseline())
        self.assertFalse((self.ws.internal / 'recovery.json').exists())

    def test_rollback_after_partial_save(self):
        self.changed()
        def fail(*args, **kw):
            self.file.write_bytes(b'partially saved')
            raise RuntimeError('compile failed')
        with patch.object(self.ws, 'engine_run', side_effect=fail):
            with self.assertRaisesRegex(RuntimeError, 'compile failed'):
                self.ws.apply()
        self.assertEqual(b'original', self.file.read_bytes())
        self.assertFalse((self.ws.internal / 'recovery.json').exists())

    def test_compare_failure_rolls_back(self):
        self.changed()
        def engine(mode, name, **fields):
            if mode == 'ExportBatch':
                bw.write(fields['Output'] / '0.native.json', bw.codec.native(self.document))
        with patch.object(self.ws, 'engine_run', side_effect=engine):
            with self.assertRaisesRegex(ValueError, 'fields differ'):
                self.ws.apply()
        self.assertEqual(b'original', self.file.read_bytes())

    def test_success_and_second_noop(self):
        changed = self.changed()
        def engine(mode, name, **fields):
            if mode == 'ImportBatch':
                self.file.write_bytes(b'rebuilt')
            elif mode == 'ExportBatch':
                bw.write(fields['Output'] / '0.native.json', bw.codec.native(changed))
        with patch.object(self.ws, 'engine_run', side_effect=engine):
            self.assertEqual(1, self.ws.apply()['changed'])
        self.assertEqual('unchanged', self.ws.apply()['status'])

    def batch_fixture(self):
        changed = self.changed()
        child = copy.deepcopy(self.document)
        child.update(asset='/Game/BP_Child', parent=self.document['asset'] + '.BP_Test_C')
        child_source = self.ws.root / 'blueprints/Child.json'
        bw.write(child_source, child)
        child_file = bw.target(self.project, child['asset'])
        child_file.write_bytes(b'original child')
        (self.ws.internal / 'child.uasset').write_bytes(b'original child')
        bw.write(self.ws.internal / 'child.json', child)
        baseline = self.ws.baseline()
        baseline[child['asset']] = dict(source='blueprints/Child.json', bytes='child.uasset',
                                       before='child.json', accepted='child.json')
        bw.write(self.ws.internal / 'baseline.json', baseline)
        new = copy.deepcopy(self.document)
        new['asset'] = '/Game/BP_New'
        bw.write(self.ws.root / 'blueprints/New.json', new)
        manifest = bw.read(self.ws.manifest)
        # Verification order differs from parent-first import order.
        manifest['assets'] = [dict(source='blueprints/Child.json'), *manifest['assets'], dict(source='blueprints/New.json')]
        bw.write(self.ws.manifest, manifest)
        return [child, changed, new]

    def test_apply_exports_all_selected_assets_in_one_fresh_batch(self):
        desired = self.batch_fixture()
        imported, calls = [], []
        def engine(mode, name, **fields):
            calls.append(mode)
            assets = bw.read(fields['Input'])['assets']
            if mode == 'ImportBatch':
                imported.append([d['asset'] for d in assets])
                for document in assets:
                    bw.target(self.project, document['asset']).write_bytes(b'rebuilt')
            elif mode == 'ExportBatch':
                self.assertEqual([[desired[1]['asset'], desired[2]['asset']], [desired[0]['asset']]], imported)
                self.assertEqual([d['asset'] for d in desired], [d['asset'] for d in assets])
                self.assertTrue(fields['Output'].is_dir())
                for i, document in enumerate(desired):
                    bw.write(fields['Output'] / f'{i}.native.json', bw.codec.native(document))
            else:
                self.fail('Unexpected engine action: ' + mode)
        with patch.object(self.ws, 'engine_run', side_effect=engine):
            self.assertEqual(dict(status='applied', assets=3, changed=1, added=1), self.ws.apply())
        self.assertEqual(['ImportBatch', 'ImportBatch', 'ExportBatch'], calls)
        for document in desired:
            accepted = bw.read(bw.inside(self.ws.internal, self.ws.baseline()[document['asset']]['accepted']))
            self.assertEqual([], bw.codec.compare(document, accepted))
        with patch.object(self.ws, 'engine_run') as engine_mock:
            self.assertEqual('unchanged', self.ws.apply()['status'])
        engine_mock.assert_not_called()

    def test_incomplete_or_incorrect_batch_verification_restores_entire_selection(self):
        desired = self.batch_fixture()
        baseline = self.ws.baseline()
        errors = {'missing': FileNotFoundError, 'identity': ValueError, 'difference': ValueError,
                  'invalid-json': json.JSONDecodeError, 'export-failed': RuntimeError}
        for fault, error in errors.items():
            with self.subTest(fault=fault):
                self.ws = bw.Workspace(self.ws.root, self.ws.engine)
                self.ws.project = self.project
                def engine(mode, name, **fields):
                    if mode == 'ImportBatch':
                        for document in bw.read(fields['Input'])['assets']:
                            bw.target(self.project, document['asset']).write_bytes(b'rebuilt')
                    elif mode == 'ExportBatch':
                        for i, document in enumerate(desired):
                            exported = copy.deepcopy(document)
                            # The first, unchanged asset must be checked too.
                            if i == 0 and fault == 'difference':
                                exported['defaults']['Amount'] = '99.000000'
                            if i == 2:
                                if fault == 'missing':
                                    continue
                                if fault == 'identity':
                                    exported['asset'] = desired[0]['asset']
                                if fault == 'invalid-json':
                                    (fields['Output'] / f'{i}.native.json').write_text('{')
                                    continue
                                if fault == 'export-failed':
                                    raise RuntimeError('batch export interrupted')
                            bw.write(fields['Output'] / f'{i}.native.json', bw.codec.native(exported))
                    else:
                        self.fail('Unexpected engine action: ' + mode)
                with patch.object(self.ws, 'engine_run', side_effect=engine):
                    with self.assertRaises(error):
                        self.ws.apply()
                self.assertEqual(b'original', self.file.read_bytes())
                self.assertEqual(b'original child', bw.target(self.project, desired[0]['asset']).read_bytes())
                self.assertFalse(bw.target(self.project, desired[2]['asset']).exists())
                self.assertEqual(baseline, self.ws.baseline())
                self.assertEqual(desired, [d for _, _, d in self.ws.load()[1]])
                self.assertFalse((self.ws.internal / 'recovery.json').exists())

    def test_recovery_removes_new_and_restores_existing(self):
        new_asset = '/Game/BP_New'
        new_file = bw.target(self.project, new_asset)
        new_file.write_bytes(b'new partial')
        self.file.write_bytes(b'old partial')
        original_baseline = self.ws.baseline()
        bw.write(self.ws.internal / 'recovery.json', dict(project=str(self.project), baseline=original_baseline,
                 assets=[dict(asset=self.document['asset'], backup='original.uasset'), dict(asset=new_asset, backup=None)]))
        self.assertTrue(self.ws.recover())
        self.assertEqual(b'original', self.file.read_bytes())
        self.assertFalse(new_file.exists())
        candidates = self.ws.recovery_candidates
        self.assertEqual(b'old partial', (candidates / 'BP_Test.uasset').read_bytes())
        self.assertEqual(b'new partial', (candidates / 'BP_New.uasset').read_bytes())
        self.assertEqual(original_baseline, self.ws.baseline())

    def test_recovery_preservation_failure_changes_no_target(self):
        self.file.write_bytes(b'manual post-interruption edit')
        bw.write(self.ws.internal / 'recovery.json', dict(project=str(self.project), baseline=self.ws.baseline(),
                 assets=[dict(asset=self.document['asset'], backup='original.uasset')]))
        with patch.object(bw.shutil, 'copyfile', side_effect=OSError('disk full')):
            with self.assertRaisesRegex(OSError, 'disk full'):
                self.ws.recover()
        self.assertEqual(b'manual post-interruption edit', self.file.read_bytes())
        self.assertTrue((self.ws.internal / 'recovery.json').exists())

    def test_corrupt_backup_aborts_recovery(self):
        operation_backup = self.ws.internal / 'corrupted.uasset'
        operation_backup.write_bytes(b'corrupted')
        self.file.write_bytes(b'partial save')
        bw.write(self.ws.internal / 'recovery.json', dict(project=str(self.project), baseline=self.ws.baseline(),
                 assets=[dict(asset=self.document['asset'], backup='corrupted.uasset')]))
        with self.assertRaisesRegex(ValueError, 'differs from accepted original'):
            self.ws.recover()
        self.assertEqual(b'partial save', self.file.read_bytes())

    def test_new_asset_rollback(self):
        data = bw.read(self.ws.manifest)
        new = copy.deepcopy(self.document)
        new['asset'] = '/Game/BP_New'
        bw.write(self.ws.root / 'blueprints/New.json', new)
        data['assets'].append(dict(source='blueprints/New.json'))
        bw.write(self.ws.manifest, data)
        new_file = bw.target(self.project, new['asset'])
        def fail(*args, **kw):
            new_file.write_bytes(b'partial')
            raise RuntimeError('save failure')
        with patch.object(self.ws, 'engine_run', side_effect=fail):
            with self.assertRaises(RuntimeError):
                self.ws.apply()
        self.assertFalse(new_file.exists())
        self.assertEqual(b'original', self.file.read_bytes())

    def test_removal_and_missing_source_fail_before_mutation(self):
        self.source.unlink()
        with self.assertRaises(FileNotFoundError):
            self.ws.apply()
        self.assertEqual(b'original', self.file.read_bytes())

    def test_missing_manifest_entry_is_not_deletion(self):
        data = bw.read(self.ws.manifest)
        replacement = copy.deepcopy(self.document)
        replacement['asset'] = '/Game/BP_Replacement'
        bw.write(self.ws.root / 'blueprints/Replacement.json', replacement)
        data['assets'] = [dict(source='blueprints/Replacement.json')]
        bw.write(self.ws.manifest, data)
        with self.assertRaisesRegex(ValueError, 'Removing exported assets'):
            self.ws.apply()
        self.assertEqual(b'original', self.file.read_bytes())

    def test_paths_and_duplicate_identity(self):
        for identity in ['/Engine/A', '/Game/../A', '/Game/A.B', '/Game//A']:
            with self.assertRaises(ValueError):
                bw.target(self.project, identity)
        with self.assertRaises(ValueError):
            bw.inside(self.ws.root, '../outside')
        data = bw.read(self.ws.manifest)
        data['assets'].append(data['assets'][0])
        bw.write(self.ws.manifest, data)
        with self.assertRaisesRegex(ValueError, 'Duplicate'):
            self.ws.load()

    def test_inheritance_groups_and_forward_reference_diagnostic(self):
        parent = dict(asset='/Game/Parent', parent='/Script/Engine.Actor')
        child = dict(asset='/Game/Child', parent='/Game/Parent.Parent_C')
        peer = dict(asset='/Game/Peer', parent='/Script/Engine.Actor')
        self.assertEqual([['/Game/Parent', '/Game/Peer'], ['/Game/Child']], [[d['asset'] for d in group] for group in bw.groups([child, parent, peer])])
        parent['reference'] = "/Game/Child.Child_C"
        with self.assertRaisesRegex(ValueError, 'later child'):
            bw.groups([parent, child])

    def test_kernel_lock_rejects_competitor(self):
        with bw.lock(self.root / 'operation.lock'):
            with self.assertRaisesRegex(ValueError, 'Competing'):
                with bw.lock(self.root / 'operation.lock'):
                    pass

    def test_lock_between_processes_and_release_after_exit(self):
        path = self.root / 'process.lock'
        program = '''
import os, sys
from pathlib import Path
from blueprint_workspace import lock
try:
    with lock(Path(sys.argv[1])):
        if sys.argv[2] == 'crash':
            os._exit(7)
except ValueError:
    sys.exit(2)
'''
        def run(mode):
            return subprocess.run([sys.executable, '-B', '-c', program, str(path), mode],
                                  cwd=bw.HERE, capture_output=True, text=True, timeout=15)
        with bw.lock(path):
            self.assertEqual(2, run('normal').returncode)
        self.assertEqual(0, run('normal').returncode)
        self.assertEqual(7, run('crash').returncode)
        self.assertEqual(0, run('normal').returncode)

    def test_engine_actions_including_preview_use_queue(self):
        with patch.object(bw,'OWNER',self.root), patch.object(bw,'LEGACY_OWNER',self.root/'legacy'), patch.object(bw,'closed'):
            for action,method in [('Prepare','prepare'),('Export','export'),('Apply','apply'),('Recover','recover'),('Preview','preview')]:
                with patch.object(bw,'engine_slot',return_value=nullcontext()) as queue, patch.object(bw.Workspace,method,return_value={'status':'test'}) as dispatch, patch.object(sys,'argv',['workspace',action,'--workspace',str(self.ws.root),'--project',str(self.project)]),redirect_stdout(io.StringIO()):
                    self.assertEqual(0,bw.main());dispatch.assert_called_once()
                    self.assertEqual(1,queue.call_count)
            self.assert_waits_for_lock(self.project.parent/'Saved/BlueprintTextWorkspace.lock')

    def test_same_workspace_blocks_preview_before_dispatch(self):
        with bw.lock(self.ws.root.parent / (self.ws.root.name + '.workspace.lock')):
            with patch.object(sys, 'argv', ['workspace', 'Preview', '--workspace', str(self.ws.root)]), \
                 patch.object(bw.Workspace, 'preview') as preview, redirect_stdout(io.StringIO()):
                self.assertEqual(1, bw.main())
                preview.assert_not_called()

    def catalog_fixture(self):
        selected = self.document['asset']
        rows = [dict(asset=selected, role='unclassified', dependencies=[dict(asset='/Game/Parent', missing=False)]),
                dict(asset='/Game/Parent', role='stub', dependencies=[dict(asset=selected, missing=False)]),
                dict(asset='/Game/Item', role='unclassified', dependencies=[dict(asset='/Game/Generator', missing=False)]),
                dict(asset='/Game/Generator', role='stub', dependencies=[], reason='Unsupported UObject Blueprint'),
                dict(asset='/Game/Other', role='unclassified', dependencies=[dict(asset='/Game/Missing', missing=True)])]
        for row in rows:
            row['class'] = 'Blueprint' if row['asset'] != '/Game/Item' else 'DataAsset'
            row.setdefault('reason', 'Informational dependency')
        parent = copy.deepcopy(self.document)
        parent.update(asset='/Game/Parent', editorRole='stub')
        native = self.ws.run / 'catalog/parent.native.json'
        bw.write(native, bw.codec.native(parent))
        rows[1]['native'] = str(native)
        rows[1].pop('reason')
        return dict(schema=1, assets=rows)

    def select_widget(self):
        document = copy.deepcopy(self.document)
        document.pop('root')
        document['parent'] = '/Script/UMG.UserWidget'
        document['widget'] = dict(tree='BP_Test:WidgetTree', objects=[
            dict(id='BP_Test:WidgetTree', name='WidgetTree', **{'class': '/Script/UMG.WidgetTree'},
                 outer='$blueprint', flags=8, properties=dict(RootWidget='BP_Test:WidgetTree.Canvas')),
            dict(id='BP_Test:WidgetTree.Canvas', name='Canvas', **{'class': '/Script/UMG.CanvasPanel'},
                 outer='BP_Test:WidgetTree', flags=8, properties=dict(Slots='()'))])
        self.document = document
        bw.write(self.source, document)
        bw.write(self.ws.internal / 'before.json', document)
        return document

    def test_widget_apply_verifies_without_preview_resources_and_survives_preview_failure(self):
        self.select_widget()
        changed = self.changed()
        source_bundle = self.source.with_suffix('.resources') / 'manifest.json'
        bw.write(source_bundle, dict(original='export resources'))
        original_bundle = source_bundle.read_bytes()
        calls = []
        def engine(mode, name, **fields):
            calls.append(mode)
            if mode == 'ImportBatch':
                self.file.write_bytes(b'rebuilt widget')
            elif mode == 'ExportBatch':
                if 'PreviewResources' in fields:
                    raise RuntimeError('PNG export unavailable')
                bw.write(fields['Output'] / '0.native.json', bw.codec.native(changed))
            else:
                self.fail('Unexpected engine action: ' + mode)
        with patch.object(self.ws, 'prepare'), patch.object(self.ws, 'engine_run', side_effect=engine), \
             patch.object(bw.shutil, 'copytree', side_effect=OSError('preview copy unavailable')):
            self.assertEqual('applied', self.ws.apply()['status'])
        self.assertEqual(['ImportBatch', 'ExportBatch'], calls)
        self.assertEqual(b'rebuilt widget', self.file.read_bytes())
        self.assertEqual(original_bundle, source_bundle.read_bytes())
        self.assertEqual('unchanged', self.ws.apply()['status'])
        baseline = self.ws.baseline()
        with patch.object(self.ws, 'prepare'), \
             patch.object(self.ws, 'preview_catalog', side_effect=RuntimeError('PNG export unavailable')):
            with self.assertRaisesRegex(RuntimeError, 'PNG export unavailable'):
                self.ws.preview()
        self.assertEqual(b'rebuilt widget', self.file.read_bytes())
        self.assertEqual(baseline, self.ws.baseline())
        self.assertFalse((self.ws.internal / 'recovery.json').exists())
        self.assertFalse((self.ws.run / 'preview/index.html').exists())

    def test_widget_preview_uses_current_resources_without_replacing_proposal_or_before(self):
        self.select_widget()
        desired = self.changed()
        original = self.ws.manifest.read_bytes(), self.ws.baseline(), self.file.read_bytes()
        # Adjacent export resources may be stale after Apply; Preview must not use them.
        stale = self.source.with_suffix('.resources') / 'manifest.json'
        bw.write(stale, dict(stale=True))
        before = self.ws.internal / 'before.resources/manifest.json'
        bundle = dict(schema=1, asset=self.document['asset'], textures={}, unsupported=[])
        bw.write(before, bundle)
        before_bytes = before.read_bytes()
        native = self.ws.run / 'catalog/widget.native.json'
        resources = native.with_suffix('.resources') / 'manifest.json'
        def engine(mode, name, **fields):
            self.assertEqual('PreviewCatalog', mode)
            self.assertEqual([], bw.read(fields['Input'])['selected'])
            bw.write(native, bw.codec.native(self.document))
            bw.write(resources, bundle)
            bw.write(fields['Output'], dict(schema=1, assets=[dict(asset=self.document['asset'],
                     role='unclassified', **{'class': 'WidgetBlueprint'}, dependencies=[], native=str(native))]))
        with patch.object(self.ws, 'prepare'), patch.object(self.ws, 'engine_run', side_effect=engine) as engine_mock:
            result = self.ws.preview()
        self.assertEqual(1, engine_mock.call_count)
        self.assertEqual('previewed', result['status'])
        record = bw.read(self.ws.run / 'preview.json')['assets'][0]
        self.assertEqual(str(resources), record['resources'])
        self.assertEqual(str(self.source), record['source'])
        self.assertEqual('changed', record['status'])
        self.assertEqual(desired, bw.read(self.source))
        self.assertEqual(before_bytes, before.read_bytes())
        self.assertEqual(original, (self.ws.manifest.read_bytes(), self.ws.baseline(), self.file.read_bytes()))

    def test_widget_preview_rejects_missing_resources_instead_of_reusing_export_bundle(self):
        self.select_widget()
        bw.write(self.source.with_suffix('.resources') / 'manifest.json',
                 dict(schema=1, asset=self.document['asset'], textures={}, unsupported=[]))
        row = dict(asset=self.document['asset'], role='unclassified', **{'class': 'WidgetBlueprint'}, dependencies=[])
        for native in (None, str(self.ws.run / 'catalog/widget.native.json')):
            with self.subTest(native=native):
                if native:
                    row['native'] = native
                with patch.object(self.ws, 'prepare'), patch.object(self.ws, 'preview_catalog', return_value={row['asset']: row}):
                    with self.assertRaisesRegex(ValueError, 'current widget resources'):
                        self.ws.preview()
                self.assertFalse((self.ws.run / 'preview/index.html').exists())
                self.assertEqual(b'original', self.file.read_bytes())

    def test_preview_discovers_dataasset_dependency_and_preserves_edit_selection(self):
        original = self.ws.manifest.read_bytes(), self.ws.baseline(), self.file.read_bytes()
        catalog = self.catalog_fixture()
        def engine(mode, name, **fields):
            self.assertEqual(mode, 'PreviewCatalog')
            self.assertEqual(bw.read(fields['Input'])['selected'], [self.document['asset']])
            bw.write(fields['Output'], catalog)
        with patch.object(self.ws, 'prepare'), patch.object(self.ws, 'engine_run', side_effect=engine):
            result = self.ws.preview()
        summary = bw.read(Path(result['preview']).parent / 'summary.json')
        items = {r['asset']: r for r in summary['items']}
        self.assertEqual(6, result['assets'])
        self.assertEqual(2, result['stubs'])
        self.assertEqual(1, result['missingDependencies'])
        self.assertIn('/Game/Generator', result['unsupportedBlueprints'])
        self.assertTrue(items['/Game/Generator']['placeholder'])
        self.assertEqual(['/Game/Item'], items['/Game/Generator']['context']['referencedBy'])
        self.assertEqual(0, items['/Game/Generator']['graphs'])
        self.assertEqual(0, items['/Game/Parent']['graphs'])
        self.assertFalse(items['/Game/Parent']['displayed'])
        self.assertEqual(1, result['editableAssets'])
        self.assertEqual(5, result['listedAssets'])
        self.assertIn('Unsupported UObject', items['/Game/Generator']['context']['reason'])
        self.assertEqual(original, (self.ws.manifest.read_bytes(), self.ws.baseline(), self.file.read_bytes()))

    def test_preview_rejects_failed_or_incomplete_discovery(self):
        with patch.object(self.ws, 'prepare'), patch.object(self.ws, 'engine_run', side_effect=RuntimeError('catalog failed')):
            with self.assertRaisesRegex(RuntimeError, 'catalog failed'):
                self.ws.preview()
        with patch.object(self.ws, 'prepare'), patch.object(self.ws, 'preview_catalog', return_value={}):
            with self.assertRaisesRegex(ValueError, 'omitted selected asset'):
                self.ws.preview()
        self.assertFalse((self.ws.run / 'preview/index.html').exists())

    def test_preview_rejects_stale_selected_asset_before_engine(self):
        self.file.write_bytes(b'changed in editor')
        with patch.object(self.ws, 'prepare') as prepare:
            with self.assertRaisesRegex(ValueError, 'changed since export'):
                self.ws.preview()
            prepare.assert_not_called()

    def test_mark_stubs_restores_asset_after_failed_reopen(self):
        dependency = bw.target(self.project, '/Game/Dependency')
        dependency.write_bytes(b'original dependency')
        def engine(*args, **kwargs):
            dependency.write_bytes(b'partial metadata write')
        with patch.object(self.ws, 'prepare'), patch.object(self.ws, 'engine_run', side_effect=engine), \
             patch.object(self.ws, 'preview_catalog', return_value={}):
            with self.assertRaisesRegex(ValueError, 'did not survive'):
                self.ws.mark_stubs(['/Game/Dependency'])
        self.assertEqual(b'original dependency', dependency.read_bytes())
        self.assertFalse((self.ws.internal / 'recovery.json').exists())
        with self.assertRaisesRegex(ValueError, 'cannot change selected'):
            self.ws.mark_stubs([self.document['asset']])


if __name__ == '__main__':
    unittest.main()
