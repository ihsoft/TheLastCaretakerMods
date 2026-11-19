"""A failed native copy must not replace a consumer or leave staged library files."""
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from blueprint_workspace import OWNER, Workspace, target, write
from native_library import copy_asset


class NativeLibraryTests(unittest.TestCase):
    def test_copy_removes_staging_before_reopen_and_preserves_original(self):
        for failed in (False, True):
            with self.subTest(failed=failed), tempfile.TemporaryDirectory(dir=OWNER) as directory:
                root = Path(directory)
                source_project = root / 'Library/Library.uproject'
                project = root / 'Consumer/Consumer.uproject'
                write(source_project, {}); write(project, {})
                source = target(source_project, '/Game/Library/Parser')
                source.parent.mkdir(parents=True); source.write_bytes(b'original')
                workspace = Workspace(root / 'workspace', root / 'Engine'); workspace.project = project
                staged = target(project, '/Game/Library/Parser')
                destination = target(project, '/Game/Mods/Example/Parser')
                def engine(*args, **kwargs):
                    self.assertEqual(staged.read_bytes(), b'original')
                    destination.parent.mkdir(parents=True); destination.write_bytes(b'copy')
                    if failed:
                        raise RuntimeError('copy failed')
                def export(*args, **kwargs):
                    self.assertFalse(staged.exists())
                    write(workspace.internal / 'baseline.json', {'/Game/Mods/Example/Parser': dict(before='before.json')})
                with patch.object(workspace, 'prepare'), patch.object(workspace, 'engine_run', side_effect=engine), patch.object(workspace, 'export', side_effect=export):
                    if failed:
                        with self.assertRaisesRegex(RuntimeError, 'copy failed'):
                            copy_asset(workspace, source, '/Game/Mods/Example/Parser')
                        self.assertFalse(destination.exists())
                    else:
                        self.assertEqual(copy_asset(workspace, source, '/Game/Mods/Example/Parser')['status'], 'asset-copied')
                        with self.assertRaisesRegex(ValueError, 'absent'):
                            copy_asset(workspace, source, '/Game/Mods/Example/Parser')
                        self.assertEqual(destination.read_bytes(), b'copy')
                self.assertFalse(staged.exists())
                self.assertEqual(source.read_bytes(), b'original')


if __name__ == '__main__':
    unittest.main()
