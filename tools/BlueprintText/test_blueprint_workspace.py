"""Offline transaction, conflict, recovery, grouping and input safety checks."""
import copy
from contextlib import redirect_stdout
import io
import json
import subprocess
import sys
import tempfile
from pathlib import Path
import unittest
from unittest.mock import patch

import blueprint_workspace as bw
from test_asset_text_codec import fixture


class WorkspaceTests(unittest.TestCase):
    def setUp(self):
        bw.OWNER.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=bw.OWNER)
        self.root = Path(self.temp.name)
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
        self.temp.cleanup()

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
            # A previous-version engine owner holds only this legacy lock.
            with bw.lock(legacy_owner / '.engine-operation.lock'):
                with self.assertRaisesRegex(ValueError, 'engine-operation.lock'):
                    with bw.project_operation(self.project):
                        self.fail('Legacy engine gate bypassed')

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

    def test_engine_gate_refuses_other_project_before_dispatch_but_allows_preview(self):
        other = self.root / 'Other/Other.uproject'
        bw.write(other, {})
        output = io.StringIO()
        with patch.object(bw, 'OWNER', self.root), patch.object(bw, 'closed'):
            with bw.project_operation(other):
                for action, method in [('Prepare', 'prepare'), ('Export', 'export'),
                                       ('Apply', 'apply'), ('Recover', 'recover')]:
                    args = ['workspace', action, '--workspace', str(self.ws.root),
                            '--project', str(self.project)]
                    with self.subTest(action=action), patch.object(sys, 'argv', args), \
                         patch.object(bw.Workspace, method) as dispatch, redirect_stdout(output):
                        self.assertEqual(1, bw.main())
                        dispatch.assert_not_called()
                    result = json.loads(output.getvalue().splitlines()[-1])
                    self.assertIn('.engine-operation.lock', result['reason'])
                    self.assertEqual(b'original', self.file.read_bytes())
                with patch.object(sys, 'argv', ['workspace', 'Preview', '--workspace', str(self.ws.root)]), \
                     patch.object(bw.Workspace, 'preview', return_value={'status': 'previewed'}) as preview, \
                     redirect_stdout(output):
                    self.assertEqual(0, bw.main())
                    preview.assert_called_once()
            # A different workspace using this project must still honor its lock.
            with bw.lock(self.project.parent / 'Saved/BlueprintTextWorkspace.lock'):
                with self.assertRaisesRegex(ValueError, 'BlueprintTextWorkspace.lock'):
                    with bw.project_operation(self.project):
                        self.fail('Project lock was bypassed')

    def test_same_workspace_blocks_preview_before_dispatch(self):
        with bw.lock(self.ws.root.parent / (self.ws.root.name + '.workspace.lock')):
            with patch.object(sys, 'argv', ['workspace', 'Preview', '--workspace', str(self.ws.root)]), \
                 patch.object(bw.Workspace, 'preview') as preview, redirect_stdout(io.StringIO()):
                self.assertEqual(1, bw.main())
                preview.assert_not_called()


if __name__ == '__main__':
    unittest.main()
