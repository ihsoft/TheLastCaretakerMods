"""Parameter and representation boundaries that can silently select or preserve the wrong data."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from pak_files import ROOT, prepare, verify


class PackagingTests(unittest.TestCase):
    def setUp(self):
        parent = ROOT / 'Tmp/tests/asset-packaging'
        parent.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=parent)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def manifest(self, files):
        path = self.root / 'files.json'
        path.write_text(json.dumps({'schemaVersion': 1, 'files': files}), encoding='utf-8')
        return path

    def test_mount_paths_reject_aliases_before_creating_output(self):
        (self.root / 'a.bin').write_bytes(b'one')
        base = {'source': 'a.bin', 'mountPath': '../../../Voyage/Mods/Example/a.bin'}
        invalid = ['../../escape', '../../../Voyage/../escape', '../../../Voyage/a\n.bin',
                   '../../../Voyage/CON', '../../../Voyage/a\\b.bin']
        cases = [[dict(base, mountPath=path)] for path in invalid]
        cases += [[base, dict(base, mountPath=base['mountPath'].upper())],
                  [base, dict(base, mountPath=base['mountPath'] + '/child')],
                  [dict(base, source='missing')]]
        for files in cases:
            output = self.root / 'rejected'
            with self.assertRaises(ValueError):
                prepare(self.manifest(files), output)
            self.assertFalse(output.exists())

    def test_mount_and_bytes_both_matter(self):
        (self.root / 'a.bin').write_bytes(b'one')
        (self.root / 'b.bin').write_bytes(b'two')
        files = [{'source': 'a.bin', 'mountPath': '../../../Voyage/A/same.bin'},
                 {'source': 'b.bin', 'mountPath': '../../../Voyage/B/same.bin'}]
        for case in ('valid', 'wrong-mount', 'wrong-bytes'):
            output = self.root / case
            prepare(self.manifest(files), output)
            readback = output / 'readback'
            for name, content in [('A', b'one'), ('B', b'two')]:
                (readback / name).mkdir(parents=True)
                (readback / name / 'same.bin').write_bytes(content)
            log = output / 'list.log'
            mount = '../../../Wrong/' if case == 'wrong-mount' else '../../../Voyage/'
            log.write_text('LogPakFile: Display: Listing files.pak with mount point "' + mount + '"\n'
                           'LogPakFile: Display: "A/same.bin" offset: 0\n'
                           'LogPakFile: Display: "B/same.bin" offset: 3\n', encoding='utf-8')
            if case == 'wrong-bytes':
                (readback / 'B/same.bin').write_bytes(b'bad')
            if case == 'valid':
                self.assertEqual(verify(output, log)['fileCount'], 2)
                self.assertFalse(readback.exists())
            else:
                with self.assertRaises(ValueError):
                    verify(output, log)
                self.assertTrue(readback.exists())

    def codec(self, mode, source, **options):
        args = [sys.executable, '-B', str(Path(__file__).with_name('data_asset.py')), mode,
                '--package', '/Game/Example/Item', '--class', '/Script/Example.Item', '--source']
        path = self.root / 'source.json'
        path.write_text(json.dumps(source), encoding='utf-8')
        output = self.root / ('result-' + str(len(list(self.root.glob('result-*')))) + '.json')
        args += [str(path), '--output', str(output)]
        for name, data in options.items():
            path = self.root / (name + '.json')
            path.write_text(json.dumps(data), encoding='utf-8')
            args += ['--' + name, str(path)]
        result = subprocess.run(args, capture_output=True, text=True)
        return result, output

    def test_conversion_preserves_payload_and_rejects_wrong_identity(self):
        asset = {'FolderName': '/Game/Example/Item', 'HasUnversionedProperties': False, 'PackageFlags': 'PKG_Cooked',
                 'Imports': [{'OuterIndex': 0, 'ObjectName': '/Script/Example', 'PackageName': 'original'},
                             {'OuterIndex': -1, 'ObjectName': 'Item', 'PackageName': 'original'}],
                 'Exports': [{'OuterIndex': 0, 'ClassIndex': -2, 'ObjectName': 'Item',
                              'Data': [{'$type': 'UAssetAPI.EnumPropertyData', 'Name': 'Mode',
                                        'EnumType': 'EMode', 'Value': 'EMode::Ready'}]}]}
        contract = {'IsUnversioned': True, 'LegacyFileVersion': -9, 'ObjectVersion': 'version',
                    'ObjectVersionUE5': 'version', 'FileVersionLicenseeUE': 0, 'CustomVersionContainer': [],
                    'PackageFlags': 'PKG_Cooked', 'HasUnversionedProperties': True, 'CustomSerializationFlags': 'None'}
        result, output = self.codec('convert', asset, contract=contract)
        self.assertEqual(result.returncode, 0, result.stderr)
        converted = json.loads(output.read_text())
        self.assertEqual(converted['Exports'][0]['Data'][0]['Value'], 'Ready')
        self.assertEqual(converted['Imports'][1]['PackageName'], 'None')
        self.assertEqual(converted['FolderName'], asset['FolderName'])
        wrong = copy.deepcopy(asset)
        wrong['Imports'][1]['ObjectName'] = 'OtherClass'
        result, output = self.codec('convert', wrong, contract=contract)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(output.exists())
        wrong = copy.deepcopy(asset)
        wrong['FolderName'] = '/Game/Other/Item'
        result, output = self.codec('convert', wrong, contract=contract)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(output.exists())


if __name__ == '__main__':
    unittest.main()
