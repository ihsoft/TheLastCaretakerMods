"""Transaction/capability tests with synthetic DLL bytes; engine tests are separate."""
import json
import contextlib
import io
from pathlib import Path
import sys
import tempfile
import unittest
import shutil
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import editor_support as support


class SupportTests(unittest.TestCase):
    def setUp(self):
        scratch = support.ROOT / 'Tmp/UnrealEditorSupport/tests'
        scratch.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=scratch)
        self.root = Path(self.temp.name)
        self.engine = self.root / 'engine'
        support.write(self.engine / 'Engine/Binaries/Win64/UnrealEditor.modules', {'BuildId': 'test-editor'})
        self.project = self.root / 'consumer/Example.uproject'
        support.write(self.project, {'FileVersion': 3, 'Description': 'Preserve this', 'Plugins': []})
        self.original = self.project.read_bytes()
        self.package = self.root / 'package'
        self.package.mkdir()
        support.write(self.package / 'ExampleSupport.uplugin', {'FileVersion': 3, 'Modules': [
            {'Name': 'ExampleNative', 'Type': 'Runtime', 'LoadingPhase': 'Default'}]})
        support.write(self.package / 'Binaries/Win64/UnrealEditor.modules',
                      {'BuildId': 'test-editor', 'Modules': {'ExampleNative': 'UnrealEditor-ExampleNative.dll'}})
        (self.package / 'Binaries/Win64/UnrealEditor-ExampleNative.dll').write_bytes(b'fake binary, only for transaction tests')
        self.inventory()
        self.output = self.root / 'output'
        self.output.mkdir()

    def tearDown(self):
        self.temp.cleanup()

    def inventory(self):
        support.write(self.package / support.MANIFEST, dict(kind=support.KIND, schema=1, plugin='ExampleSupport',
            modules=['ExampleNative'], engineBuildId='test-editor',
            files={n: len(b) for n, b in support.tree(self.package).items() if n != support.MANIFEST}))

    def attach(self, loader=lambda *args: None):
        return support.attach(self.project, self.package, self.engine, self.output, loader)

    def test_first_attach_and_idempotence(self):
        self.assertEqual(self.attach(), 'prepared')
        before = support.tree(self.project.parent)
        self.assertEqual(self.attach(), 'unchanged')
        self.assertEqual(support.tree(self.project.parent), before)
        self.assertEqual(support.read(self.project)['Description'], 'Preserve this')

    def test_wrong_editor_build_before_mutation(self):
        support.write(self.engine / 'Engine/Binaries/Win64/UnrealEditor.modules', {'BuildId': 'other'})
        with self.assertRaisesRegex(ValueError, 'another editor build'):
            self.attach()
        self.assertEqual(self.project.read_bytes(), self.original)
        self.assertFalse((self.project.parent / 'Plugins').exists())

    def test_missing_dll(self):
        (self.package / 'Binaries/Win64/UnrealEditor-ExampleNative.dll').unlink()
        with self.assertRaises(ValueError):
            self.attach()

    def test_unexpected_source(self):
        (self.package / 'unexpected.cpp').write_text('source')
        self.inventory()
        with self.assertRaisesRegex(ValueError, 'unsupported source'):
            self.attach()

    def test_module_manifest_disagreement(self):
        support.write(self.package / 'Binaries/Win64/UnrealEditor.modules', {'BuildId': 'test-editor', 'Modules': {}})
        self.inventory()
        with self.assertRaisesRegex(ValueError, 'disagrees'):
            self.attach()

    def test_unmanaged_plugin_preserved(self):
        existing = self.project.parent / 'Plugins/ExampleSupport/Keep.txt'
        existing.parent.mkdir(parents=True)
        existing.write_text('user-owned')
        with self.assertRaisesRegex(ValueError, 'unmanaged'):
            self.attach()
        self.assertEqual(existing.read_text(), 'user-owned')

    def test_project_module_conflict(self):
        support.write(self.project, {'Modules': [{'Name': 'ExampleNative'}]})
        with self.assertRaisesRegex(ValueError, 'Project already owns'):
            self.attach()

    def test_other_plugin_conflict(self):
        support.write(self.project.parent / 'Plugins/Other/Other.uplugin', {'Modules': [{'Name': 'ExampleNative'}]})
        with self.assertRaisesRegex(ValueError, 'Another plugin'):
            self.attach()

    def test_disabled_plugin_not_silently_enabled(self):
        support.write(self.project, {'Plugins': [{'Name': 'ExampleSupport', 'Enabled': False}]})
        with self.assertRaisesRegex(ValueError, 'explicitly disables'):
            self.attach()

    def test_editor_failure_rolls_back_first_install(self):
        def fail(*args):
            raise RuntimeError('injected editor failure')
        with self.assertRaisesRegex(RuntimeError, 'injected'):
            self.attach(fail)
        self.assertEqual(self.project.read_bytes(), self.original)
        self.assertFalse((self.project.parent / 'Plugins/ExampleSupport').exists())
        self.assertFalse(support.recovery_path(self.project).exists())

    def test_editor_failure_restores_previous_plugin(self):
        self.attach()
        previous = support.tree(self.project.parent / 'Plugins')
        descriptor = self.project.read_bytes()
        (self.package / 'Binaries/Win64/UnrealEditor-ExampleNative.dll').write_bytes(b'replacement')
        self.inventory()
        def fail(*args):
            raise RuntimeError('injected update failure')
        with self.assertRaises(RuntimeError):
            self.attach(fail)
        self.assertEqual(support.tree(self.project.parent / 'Plugins'), previous)
        self.assertEqual(self.project.read_bytes(), descriptor)

    def test_interrupted_install_recovery(self):
        owner = support.recovery_path(self.project).parent
        stage = owner / 'transaction-test'
        stage.mkdir(parents=True)
        (stage / 'project-before.json').write_bytes(self.original)
        support.write(self.project, {'broken': 'partial preparation'})
        support.write(self.project.parent / 'Plugins/ExampleSupport/partial.json', {})
        support.write(support.recovery_path(self.project), dict(project=str(self.project), plugin='ExampleSupport', stage=str(stage), existed=False))
        with self.assertRaisesRegex(ValueError, 'Interrupted'):
            self.attach()
        self.assertTrue(support.recover(self.project))
        self.assertEqual(self.project.read_bytes(), self.original)
        self.assertFalse((self.project.parent / 'Plugins/ExampleSupport').exists())

    def test_recovery_rejects_outside_stage(self):
        support.write(support.recovery_path(self.project), dict(project=str(self.project), plugin='ExampleSupport', stage=str(self.root), existed=False))
        with self.assertRaisesRegex(ValueError, 'Invalid recovery'):
            support.recover(self.project)

    def test_existing_output_is_not_overwritten_by_error_report(self):
        (self.output / 'keep.txt').write_text('user result')
        before = support.tree(self.output)
        arguments = ['tool', '--project', str(self.project), '--engine', str(self.engine),
                     '--package', str(self.package), '--output', str(self.output)]
        with patch.object(sys, 'argv', arguments), contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(support.main(), 1)
        self.assertEqual(support.tree(self.output), before)

    def test_managed_plugin_with_new_user_files_is_not_erased(self):
        self.attach()
        added = self.project.parent / 'Plugins/ExampleSupport/Notes.txt'
        added.write_text('keep this')
        with self.assertRaisesRegex(ValueError, 'Unexpected or missing'):
            self.attach()
        self.assertEqual(added.read_text(), 'keep this')

    def test_uat_cannot_write_filter_file_into_owner_sources(self):
        source = self.root / 'source-plugin'
        source.mkdir()
        shutil.copy2(self.package / 'ExampleSupport.uplugin', source / 'ExampleSupport.uplugin')
        (source / 'Source').mkdir()
        (source / 'Source/native.cpp').write_text('// synthetic input')
        original = support.tree(source)
        def fake_uat(command, log):
            supplied = Path(next(arg[8:] for arg in command if isinstance(arg, str) and arg.startswith('-Plugin=')))
            packaged = Path(next(arg[9:] for arg in command if isinstance(arg, str) and arg.startswith('-Package=')))
            support.write(supplied.parent / 'Config/FilterPlugin.ini', {'uat': 'creates this'})
            shutil.copytree(self.package, packaged)
        package = support.build_package(source / 'ExampleSupport.uplugin', self.engine, self.output, fake_uat)
        self.assertEqual(support.tree(source), original)
        self.assertTrue((package / support.MANIFEST).is_file())
        self.assertFalse((self.output / 'input-plugin').exists())


if __name__ == '__main__':
    unittest.main()
