"""Resource request tests; --with-unreal also exercises real PNG/WAV authoring.

Wrong modes, destinations or settings can silently replace the wrong resource.
These cases protect that input boundary and content replacement at stable paths.
"""
import copy
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import wave
import zlib

from request import prepare

REPO = Path(__file__).resolve().parents[2]
WITH_UNREAL = '--with-unreal' in sys.argv
if WITH_UNREAL:
    sys.argv.remove('--with-unreal')


def png(path, width, height):
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    pixels = (b'\0' + bytes((30, 80, 150, 180)) * width) * height
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
                     + chunk(b'IDAT', zlib.compress(pixels)) + chunk(b'IEND', b''))


def wav(path, frames):
    with wave.open(str(path), 'wb') as stream:
        stream.setparams((1, 2, 8000, frames, 'NONE', 'not compressed'))
        stream.writeframes(b'\x10\x00' * frames)


class Requests(unittest.TestCase):
    def setUp(self):
        parent = REPO / 'Tmp/tests/UnrealResources'
        parent.mkdir(parents=True, exist_ok=True)
        self.root = Path(tempfile.mkdtemp(prefix='test-', dir=parent))
        self.project = self.root / 'Project/Resources.uproject'
        self.project.parent.mkdir()
        self.project.write_text('{"FileVersion":3,"Plugins":[{"Name":"PythonScriptPlugin","Enabled":true},{"Name":"EditorScriptingUtilities","Enabled":true}]}')
        self.request = self.root / 'resources.json'
        png(self.root / 'input.png', 4, 3)
        wav(self.root / 'input.wav', 1600)
        self.entries = [
            {'asset': '/Game/Fixture/T_Image', 'type': 'Texture2D', 'mode': 'create', 'source': 'input.png',
             'settings': {'srgb': False, 'compression_settings': 'TC_EDITOR_ICON', 'lod_group': 'TEXTUREGROUP_UI', 'mip_gen_settings': 'TMGS_NO_MIPMAPS'}},
            {'asset': '/Game/Fixture/S_Sound', 'type': 'SoundWave', 'mode': 'create', 'source': 'input.wav',
             'settings': {'volume': 0.3}},
        ]

    def write(self, entries):
        self.request.write_text(json.dumps({'schemaVersion': 1, 'resources': entries}))

    def prepared(self, entries):
        self.write(entries)
        return prepare('Import', self.project, self.request, self.root / 'output')

    def test_reject_wrong_request_without_creating_output(self):
        mutations = [
            ('asset', '/Game/../Elsewhere'), ('mode', 'automatic'), ('type', 'Blueprint'),
            ('source', 'absent.png'), ('settings', {'srgb': 'false'}),
            ('settings', {'s_rgb': False}),
        ]
        for field, value in mutations:
            with self.subTest(field=field, value=value):
                entries = copy.deepcopy(self.entries)
                entries[0][field] = value
                with self.assertRaises((ValueError, FileNotFoundError)):
                    self.prepared(entries)
                self.assertFalse((self.root / 'output').exists())
        with self.assertRaises(ValueError):
            self.prepared([self.entries[0], self.entries[0]])

    def test_reject_maintained_project_and_existing_output(self):
        self.write(self.entries)
        with self.assertRaises(ValueError):
            prepare('Import', REPO / 'mods/Example/Project/Example.uproject', self.request, self.root / 'output')
        (self.root / 'output').mkdir()
        with self.assertRaises(ValueError):
            prepare('Import', self.project, self.request, self.root / 'output')

    def test_read_actions_reject_write_fields(self):
        self.write(self.entries)
        for action in ['Inspect', 'Export']:
            with self.assertRaises(ValueError):
                prepare(action, self.project, self.request, self.root / 'output')

    @unittest.skipUnless(WITH_UNREAL, 'Use --with-unreal for editor integration')
    def test_editor_create_replace_export_and_rejection(self):
        def invoke(action, entries, label, success=True):
            self.write(entries)
            output = self.root / label
            result = subprocess.run([sys.executable, '-B', str(REPO / 'tools/run_powershell.py'),
                str(REPO / 'tools/Invoke-UnrealResource.ps1'), '-Action', action,
                '-ProjectFile', str(self.project), '-RequestPath', str(self.request), '-OutputRoot', str(output)],
                cwd=REPO, capture_output=True, text=True)
            (self.root / (label + '-wrapper.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
            self.assertEqual(result.returncode == 0, success, f'{label}: {self.root}\n{result.stdout}\n{result.stderr}')
            if success:
                report = 'reopen.json' if action == 'Import' else 'execute.json'
                return json.loads((output / report).read_text())['resources']

        created = invoke('Import', self.entries, 'create')
        self.assertFalse(created[0]['settings']['srgb'])
        self.assertAlmostEqual(created[1]['settings']['volume'], 0.3, places=5)
        png(self.root / 'input.png', 6, 2)
        wav(self.root / 'input.wav', 3200)
        replacement = [{**entry, 'mode': 'replace', 'settings': {}} for entry in self.entries]
        replaced = invoke('Import', replacement, 'replace')
        self.assertEqual(replaced[0]['settings'], created[0]['settings'])
        self.assertEqual(replaced[1]['settings'], created[1]['settings'])
        # A later bad entry must reject the entire request before the first write.
        png(self.root / 'input.png', 8, 8)
        invalid = [replacement[0], {**replacement[1], 'mode': 'create'}]
        invoke('Import', invalid, 'reject-existing', success=False)
        read = [{'asset': e['asset'], 'type': e['type']} for e in self.entries]
        exported = invoke('Export', read, 'export')
        self.assertEqual((exported[0]['width'], exported[0]['height']), (6, 2))
        with wave.open(exported[1]['export'], 'rb') as stream:
            self.assertEqual(stream.getnframes(), 3200)
        print(f'Editor integration evidence: {self.root}')


if __name__ == '__main__':
    unittest.main()
