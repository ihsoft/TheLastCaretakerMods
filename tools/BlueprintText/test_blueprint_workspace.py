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
        with patch.object(self.ws, 'engine_run'), patch.object(self.ws, 'export_one', return_value=self.document):
            with self.assertRaisesRegex(ValueError, 'fields differ'):
                self.ws.apply()
        self.assertEqual(b'original', self.file.read_bytes())

    def test_success_and_second_noop(self):
        changed = self.changed()
        def rebuild(*args, **kw):
            self.file.write_bytes(b'rebuilt')
        def export(asset, output, name, **kwargs):
            bw.write(output, changed)
            return changed
        with patch.object(self.ws, 'engine_run', side_effect=rebuild), patch.object(self.ws, 'export_one', side_effect=export):
            self.assertEqual(1, self.ws.apply()['changed'])
        self.assertEqual('unchanged', self.ws.apply()['status'])

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
