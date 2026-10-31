"""Source composition, rebuild selection and transaction tests without Unreal."""
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from UnrealEditorSupport import editor_support as support
from UnrealEditorSupport import project_support as projects


class ProjectSupportTests(unittest.TestCase):
    def setUp(self):
        scratch = support.ROOT / 'Tmp/UnrealEditorSupport/tests'
        scratch.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.project = self.root / 'consumer/Test.uproject'
        support.write(self.project, {'FileVersion': 3})
        self.engine = self.root / 'engine'
        support.write(self.engine / 'Engine/Binaries/Win64/UnrealEditor.modules', {'BuildId': 'one'})
        self.source = self.root / 'owner/Support.uplugin'
        support.write(self.source, {'FileVersion': 3, 'Modules': [{'Name': 'Native', 'Type': 'Runtime'}]})
        self.header = self.source.parent / 'Source/Native/Native.h'
        self.header.parent.mkdir(parents=True)
        self.header.write_bytes(b'owned definition')
        self.calls = []
        self.sequence = 0

    def build(self, source, engine, output):
        self.calls.append(support.tree(source.parent))
        package = output / 'package'
        package.mkdir()
        shutil.copyfile(source, package / source.name)
        identity = support.engine_identity(engine)
        support.write(package / 'Binaries/Win64/UnrealEditor.modules',
                      {'BuildId': identity, 'Modules': {'Native': 'UnrealEditor-Native.dll'}})
        (package / 'Binaries/Win64/UnrealEditor-Native.dll').write_bytes(self.header.read_bytes())
        support.write(package / support.MANIFEST, dict(kind=support.KIND, schema=1, plugin='Support',
                      modules=['Native'], engineBuildId=identity,
                      files={name: len(value) for name, value in support.tree(package).items()}))
        return package

    def prepare(self, **kwargs):
        self.sequence += 1
        return projects.prepare_provider(self.project, self.engine, self.root / ('run-' + str(self.sequence)),
                                         self.source, builder=self.build, loader=kwargs.pop('loader', lambda *args: None), **kwargs)

    def test_first_clean_attach_repeat_and_changed_source(self):
        self.assertTrue(self.prepare()['built'])
        self.assertFalse(self.prepare()['built'])
        self.assertEqual(len(self.calls), 1)
        self.header.write_bytes(b'updated definition')
        self.assertTrue(self.prepare()['built'])
        self.assertEqual(len(self.calls), 2)
        installed = support.tree(self.project.parent / 'Plugins/Support')
        self.assertFalse(any(Path(name).suffix in ('.h', '.cpp', '.cs') for name in installed))
        self.assertEqual(len(list((self.project.parent / 'Plugins').glob('*.uplugin'))), 0)
        self.assertEqual(len(support.read(self.project)['Plugins']), 1)

    def test_engine_change_rebuilds(self):
        self.prepare()
        support.write(self.engine / 'Engine/Binaries/Win64/UnrealEditor.modules', {'BuildId': 'two'})
        self.assertTrue(self.prepare()['built'])
        self.assertEqual(len(self.calls), 2)

    def test_owned_overlay_is_composed_without_changing_inputs(self):
        other = self.root / 'shared'
        other.mkdir(); (other / 'Shared.h').write_bytes(b'shared definition')
        before = support.tree(self.source.parent)
        self.prepare(overlays=[dict(source=str(other), destination='Source/Native')])
        self.assertEqual(self.calls[0]['Source/Native/Shared.h'], b'shared definition')
        self.assertEqual(support.tree(self.source.parent), before)

    def test_overlay_collision_and_escape_fail_before_installation(self):
        for destination in ('Source/Native', '../Source/Native'):
            with self.subTest(destination=destination), self.assertRaises(ValueError):
                self.prepare(overlays=[dict(source=str(self.header.parent), destination=destination)])
        self.assertFalse(self.calls)
        self.assertFalse((self.project.parent / 'Plugins').exists())

    def test_failed_update_preserves_old_provider_and_input_cache(self):
        self.prepare()
        before = support.tree(self.project.parent)
        self.header.write_bytes(b'bad update')
        def fail(*args):
            raise RuntimeError('module load failure')
        with self.assertRaisesRegex(RuntimeError, 'module load failure'):
            self.prepare(loader=fail)
        self.assertEqual(support.tree(self.project.parent), before)
        self.assertTrue(self.prepare()['built'])

    def test_missing_cached_installation_rebuilds(self):
        self.prepare()
        support.remove_owned(self.project.parent / 'Plugins/Support', self.project.parent)
        self.assertTrue(self.prepare()['built'])

    def test_project_declaration_resolves_owners_and_rejects_duplicate_modules(self):
        support.write(self.project.parent / 'EditorSupport.json', {'schema': 1, 'plugins': [{'source': str(self.source)}]})
        with patch.object(projects, 'prepare_provider', return_value={'status': 'prepared'}) as call:
            self.assertEqual(len(projects.prepare_project(self.project, self.engine, self.root / 'declared')), 1)
            self.assertEqual(call.call_args.args[3], self.source)
        support.write(self.project.parent / 'EditorSupport.json', {'schema': 1, 'plugins': [{'source': str(self.source)}] * 2})
        with patch.object(projects, 'prepare_provider') as call, self.assertRaises(ValueError):
            projects.prepare_project(self.project, self.engine, self.root / 'invalid')
        call.assert_not_called()


if __name__ == '__main__':
    unittest.main()
