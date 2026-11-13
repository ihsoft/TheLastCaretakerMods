"""Source composition, rebuild selection and transaction tests without Unreal."""
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from UnrealEditorSupport import editor_support as support
from UnrealEditorSupport import project_support as projects
from UnrealEditorSupport import provider_cache


class ProjectSupportTests(unittest.TestCase):
    def setUp(self):
        scratch = support.ROOT / 'Tmp/UnrealEditorSupport/tests'
        scratch.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.cache_root = self.root / 'shared-cache'
        cache_patch = patch.object(provider_cache, 'CACHE_ROOT', self.cache_root)
        cache_patch.start(); self.addCleanup(cache_patch.stop)
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
        rule = package / 'Source/Native/Native.Build.cs'
        rule.parent.mkdir(parents=True)
        rule.write_bytes(support.precompiled_rules('Native').encode('utf-8'))
        support.write(package / support.MANIFEST, dict(kind=support.KIND, schema=support.PACKAGE_SCHEMA, plugin='Support',
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
        self.assertFalse(any(Path(name).suffix in ('.h', '.cpp') for name in installed))
        self.assertEqual(installed['Source/Native/Native.Build.cs'], support.precompiled_rules('Native').encode('utf-8'))
        self.assertEqual(len(list((self.project.parent / 'Plugins').glob('*.uplugin'))), 0)
        self.assertEqual(len(support.read(self.project)['Plugins']), 1)

    def test_engine_change_rebuilds(self):
        self.prepare()
        support.write(self.engine / 'Engine/Binaries/Win64/UnrealEditor.modules', {'BuildId': 'two'})
        self.assertTrue(self.prepare()['built'])
        self.assertEqual(len(self.calls), 2)

    def test_old_local_snapshot_recovers_from_shared_cache(self):
        self.prepare()
        cache = self.project.parent / 'Saved/UnrealEditorSupport/inputs/Support.json'
        old = support.read(cache)
        old.pop('packageSchema')
        support.write(cache, old)
        self.assertEqual(self.prepare()['reuse'], 'shared')
        self.assertFalse(self.prepare()['built'])
        self.assertEqual(len(self.calls), 1)

    def test_legacy_package_reinstalled_under_current_cache_is_upgraded(self):
        self.prepare()
        installed = self.project.parent / 'Plugins/Support'
        support.remove_owned(installed / 'Source', installed)
        manifest = support.read(installed / support.MANIFEST)
        manifest['schema'] = 1
        manifest['files'] = {n: len(b) for n, b in support.tree(installed).items() if n != support.MANIFEST}
        support.write(installed / support.MANIFEST, manifest)
        self.assertEqual(self.prepare()['reuse'], 'shared')
        self.assertFalse(self.prepare()['built'])

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

    def test_missing_installation_uses_shared_cache(self):
        self.prepare()
        support.remove_owned(self.project.parent / 'Plugins/Support', self.project.parent)
        self.assertEqual(self.prepare()['reuse'], 'shared')

    def fresh_project(self, name):
        self.project = self.root / name / 'Test.uproject'
        support.write(self.project, {'FileVersion': 3})

    def test_fresh_copy_and_relocated_sources_reuse_but_still_load(self):
        self.prepare()
        before = support.tree(self.source.parent)
        shutil.copytree(self.source.parent, self.root / 'relocated')
        self.source = self.root / 'relocated/Support.uplugin'
        self.fresh_project('second')
        with patch.object(projects.support, 'probe') as load:
            result = self.prepare(loader=load)
        self.assertFalse(result['built'])
        self.assertEqual(result['reuse'], 'shared')
        self.assertEqual(Path(result['package']), self.project.parent / 'Plugins/Support')
        load.assert_called_once()
        self.assertEqual(len(self.calls), 1)
        self.assertEqual(support.tree(self.source.parent), before)

    def test_overlay_variants_reuse_independently(self):
        overlay = self.root / 'overlay'
        overlay.mkdir(); extra = overlay / 'Extra.cpp'
        options = [dict(source=str(overlay), destination='Source/Native')]
        extra.write_bytes(b'int Extra() { return 1; }')
        self.prepare(overlays=options)
        extra.write_bytes(b'int Extra() { return 2; }')
        self.fresh_project('variant-two')
        self.assertTrue(self.prepare(overlays=options)['built'])
        extra.write_bytes(b'int Extra() { return 1; }')
        self.fresh_project('variant-one-again')
        self.assertEqual(self.prepare(overlays=options)['reuse'], 'shared')
        self.assertEqual(len(self.calls), 2)

    def test_build_recipe_change_invalidates_reuse(self):
        self.prepare()
        with patch.object(support, 'BUILD_RECIPE', support.BUILD_RECIPE + 1):
            self.assertTrue(self.prepare()['built'])
        self.fresh_project('old-recipe')
        self.assertEqual(self.prepare()['reuse'], 'shared')

    def test_same_size_corrupt_shared_binary_is_rebuilt(self):
        self.prepare()
        binary = next(self.cache_root.glob('*/package/Binaries/Win64/*.dll'))
        original = binary.read_bytes()
        binary.write_bytes(b'x' * len(original))
        self.fresh_project('after-corruption')
        self.assertTrue(self.prepare()['built'])
        self.assertEqual(binary.read_bytes(), original)

    def test_local_snapshot_cannot_authorize_a_replaced_package(self):
        self.prepare()
        installed = self.project.parent / 'Plugins/Support'
        original = support.tree(installed)
        binary = installed / 'Binaries/Win64/UnrealEditor-Native.dll'
        binary.write_bytes(b'x' * len(binary.read_bytes()))
        self.assertEqual(self.prepare()['reuse'], 'shared')
        self.assertEqual(support.tree(installed), original)
        self.assertEqual(len(self.calls), 1)

    def test_interrupted_or_incomplete_entry_is_rebuilt(self):
        self.prepare()
        next(self.cache_root.glob('*/entry.json')).unlink()
        self.fresh_project('after-incomplete-cache')
        self.assertTrue(self.prepare()['built'])
        self.assertFalse(list(self.root.glob('run-*/cache-entry')))

    def test_failed_load_does_not_publish_or_mark_inputs_current(self):
        def fail(*args):
            raise RuntimeError('failed load')
        with self.assertRaisesRegex(RuntimeError, 'failed load'):
            self.prepare(loader=fail)
        self.assertFalse(self.cache_root.exists())
        self.assertFalse((self.project.parent / 'Saved/UnrealEditorSupport/inputs/Support.json').exists())
        self.assertFalse((self.project.parent / 'Plugins/Support').exists())

    def test_shared_hit_load_failure_preserves_cache_and_rolls_back_consumer(self):
        self.prepare()
        before = support.tree(self.cache_root)
        self.fresh_project('bad-consumer')
        original = self.project.read_bytes()
        def fail(*args):
            raise RuntimeError('consumer missing dependency')
        with self.assertRaisesRegex(RuntimeError, 'consumer missing dependency'):
            self.prepare(loader=fail)
        self.assertEqual(support.tree(self.cache_root), before)
        self.assertEqual(self.project.read_bytes(), original)
        self.assertFalse((self.project.parent / 'Plugins/Support').exists())
        self.assertFalse((self.project.parent / 'Saved/UnrealEditorSupport/inputs/Support.json').exists())
        self.assertEqual(len(self.calls), 1)

    def test_failed_build_is_not_published(self):
        with patch.object(self, 'build', side_effect=RuntimeError('compiler failed')), self.assertRaises(RuntimeError):
            self.prepare()
        self.assertFalse(self.cache_root.exists())

    def test_entry_limit_evicts_least_recently_used_variant(self):
        with patch.object(provider_cache, 'MAX_ENTRIES', 2):
            self.prepare()
            first = next(self.cache_root.iterdir())
            self.header.write_bytes(b'second variant')
            self.prepare()
            second = next(p for p in self.cache_root.iterdir() if p != first)
            os.utime(first, (1, 1)); os.utime(second, (2, 2))
            self.header.write_bytes(b'owned definition')
            self.fresh_project('reuse-first')
            self.assertEqual(self.prepare()['reuse'], 'shared')
            self.header.write_bytes(b'third variant')
            self.prepare()
            self.assertTrue(first.exists())
            self.assertFalse(second.exists())
            self.assertEqual(len(list(self.cache_root.iterdir())), 2)

    def test_byte_limit_and_unknown_names(self):
        self.prepare()
        unknown = self.cache_root / 'owner-notes.txt'
        unknown.write_text('keep')
        installed = support.tree(self.project.parent / 'Plugins/Support')
        with patch.object(provider_cache, 'MAX_BYTES', 1):
            self.header.write_bytes(b'oversized variant')
            self.assertTrue(self.prepare()['built'])
        self.assertEqual(list(self.cache_root.iterdir()), [unknown])
        self.assertTrue(installed)
        self.assertTrue((self.project.parent / 'Plugins/Support/Support.uplugin').is_file())

    def test_shared_hit_survives_eviction_after_installation(self):
        self.prepare()
        self.fresh_project('eviction-consumer')
        with patch.object(provider_cache, 'MAX_BYTES', 1):
            result = self.prepare()
        self.assertEqual(result['reuse'], 'shared')
        self.assertEqual(result['loadedModules'], ['Native'])
        self.assertTrue(Path(result['package']).is_dir())
        self.assertFalse(list(self.cache_root.iterdir()))

    def test_cache_copy_failure_leaves_no_visible_entry(self):
        original = provider_cache.shutil.copytree
        def copy(source, target, *args, **kwargs):
            if 'cache-entry' in Path(target).parts:
                Path(target).mkdir(parents=True)
                (Path(target) / 'partial').write_bytes(b'partial')
                raise OSError('cache copy failed')
            return original(source, target, *args, **kwargs)
        with patch.object(provider_cache.shutil, 'copytree', side_effect=copy), self.assertRaisesRegex(OSError, 'cache copy failed'):
            self.prepare()
        self.assertFalse(list(self.cache_root.iterdir()))
        self.assertFalse(list(self.root.glob('run-*/cache-entry')))

    def test_intermediate_probe_exclusion_preserves_required_reference_and_rollback(self):
        support.write(self.project, {'Plugins': [{'Name': 'PendingProvider', 'Enabled': True}]})
        def load(project, engine, names, output, **options):
            self.assertEqual(names, ['Native'])
            self.assertEqual(options, {'disabled_plugins': ('PendingProvider',)})
            self.assertIn({'Name': 'PendingProvider', 'Enabled': True}, support.read(project)['Plugins'])
        self.prepare(loader=load, disabled_plugins=('PendingProvider',))
        before = support.tree(self.project.parent)
        self.header.write_bytes(b'changed')
        def fail(*args, **options):
            load(*args, **options)
            raise RuntimeError('unrelated missing module')
        with self.assertRaisesRegex(RuntimeError, 'unrelated missing module'):
            self.prepare(loader=fail, disabled_plugins=('PendingProvider',))
        self.assertEqual(support.tree(self.project.parent), before)

    def test_project_declaration_resolves_owners_and_rejects_duplicate_modules(self):
        support.write(self.project.parent / 'EditorSupport.json', {'schema': 1, 'plugins': [{'source': str(self.source)}]})
        with patch.object(projects, 'prepare_provider', return_value={'status': 'prepared'}) as call:
            self.assertEqual(len(projects.prepare_project(self.project, self.engine, self.root / 'declared',
                                                        disabled_plugins=('PendingProvider',))), 1)
            self.assertEqual(call.call_args.args[3], self.source)
            self.assertEqual(call.call_args.kwargs, {'disabled_plugins': ('PendingProvider',)})
        support.write(self.project.parent / 'EditorSupport.json', {'schema': 1, 'plugins': [{'source': str(self.source)}] * 2})
        with patch.object(projects, 'prepare_provider') as call, self.assertRaises(ValueError):
            projects.prepare_project(self.project, self.engine, self.root / 'invalid')
        call.assert_not_called()


if __name__ == '__main__':
    unittest.main()
