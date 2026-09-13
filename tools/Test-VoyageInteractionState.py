"""Offline tests: no process access, fixture bytes are synthetic."""
import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location('observer', Path(__file__).with_name('Read-VoyageInteractionState.py'))
observer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(observer)


def item(pointer=0x10000, serial=42):
    return struct.pack('<QQii', 0, pointer, serial, 0)


class WeakReferenceTests(unittest.TestCase):
    def reader(self, items):
        reader = observer.Reader.__new__(observer.Reader)
        iterator = iter(items)
        reader.item = lambda index: next(iterator)
        reader.identity = lambda obj: {'address': hex(obj)}
        return reader

    def test_null_does_not_access_array(self):
        self.assertEqual(self.reader([]).weak(bytes(8))['status'], 'null')

    def test_valid_serial(self):
        result = self.reader([item(), item()]).weak(struct.pack('<ii', 3, 42))
        self.assertEqual(result['status'], 'serial-validated')
        self.assertEqual(result['target']['address'], '0x10000')

    def test_stale_serial_has_no_target(self):
        result = self.reader([item(serial=43)]).weak(struct.pack('<ii', 3, 42))
        self.assertEqual(result['status'], 'stale')
        self.assertIsNone(result['target'])

    def test_reuse_during_read_rejected(self):
        with self.assertRaisesRegex(ValueError, 'changed'):
            self.reader([item(), item(serial=43)]).weak(struct.pack('<ii', 3, 42))

    def test_null_object_with_matching_serial_rejected(self):
        with self.assertRaisesRegex(ValueError, 'null object'):
            self.reader([item(pointer=0)]).weak(struct.pack('<ii', 3, 42))

    def test_correct_object_relative_caches(self):
        self.assertEqual(observer.CACHE_OFFSETS, {'interaction': 0x360, 'hitComponent': 0x370})


if __name__ == '__main__': unittest.main()
