"""Read-only gaze-cache observer, Steam 25191271 / UE5.8 only.

Native cache offsets are object-relative (not interface-relative). Revalidate
all native layouts on fingerprint change. No game calls, injection or writes.
"""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
from pathlib import Path
import struct
import time

EXE_HASH = '747dc2553f7e68d8ea7ed0b2e0cac6d08943ea3f50dd6ed822e9293e0b45f58b'
NAME_POOL_RVA = 0xb32db80
DETECTOR_VTABLE_RVA = 0x995d1d0
CACHE_OFFSETS = {'interaction': 0x360, 'hitComponent': 0x370}
ITEM_SIZE, ITEM_POINTER, ITEM_SERIAL = 24, 8, 16
MARKERS = ('HarpoonEntryReady', 'HarpoonEntryInteractBlocks', 'HarpoonEntryProviderSeen', 'HarpoonEntryCallbackSeen')


def integer(s): return int(s, 0)
def u64(b, offset=0): return struct.unpack_from('<Q', b, offset)[0]
def i32(b, offset=0): return struct.unpack_from('<i', b, offset)[0]


class Reader:
    def __init__(self, args):
        self.a = args
        self.deadline = time.monotonic() + 60
        self.names, self.schemas = {}, {}
        self.k = ctypes.WinDLL('kernel32', use_last_error=True)
        self.k.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
        self.k.OpenProcess.restype = wintypes.HANDLE
        self.k.ReadProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p,
            ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
        self.k.CloseHandle.argtypes = [wintypes.HANDLE]
        self.k.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE, wintypes.DWORD,
            wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD)]
        self.h = self.k.OpenProcess(0x1010, False, args.pid)
        if not self.h: raise ctypes.WinError(ctypes.get_last_error())

    def read(self, at, size):
        if time.monotonic() > self.deadline: raise TimeoutError('60-second observation limit')
        if not at or not 0 < size <= 1600000: raise ValueError('Invalid read range')
        buf, got = ctypes.create_string_buffer(size), ctypes.c_size_t()
        if not self.k.ReadProcessMemory(self.h, at, buf, size, ctypes.byref(got)) or got.value != size:
            raise OSError('Unreadable address ' + hex(at))
        return buf.raw

    def ptr(self, at): return u64(self.read(at, 8))

    def fname(self, b, offset=0):
        index, number = struct.unpack_from('<II', b, offset)
        if index not in self.names:
            if index >> 16 > 8191: raise ValueError('Invalid name index')
            block = self.ptr(self.a.base + NAME_POOL_RVA + 0x10 + (index >> 16) * 8)
            entry = block + (index & 65535) * 2
            header = struct.unpack('<H', self.read(entry, 2))[0]
            length = header >> 6
            if not 1 <= length <= 1024: raise ValueError('Invalid name length')
            self.names[index] = self.read(entry + 2, length * (2 if header & 1 else 1)).decode(
                'utf-16-le' if header & 1 else 'utf-8')
        return self.names[index] + ('_' + str(number - 1) if number else '')

    def name(self, obj): return self.fname(self.read(obj + 0x18, 8)) if obj else None

    def item(self, index):
        if not 0 <= index < self.a.count: raise ValueError('Index outside discovery snapshot')
        chunk = self.ptr(self.a.objects + (index // 65536) * 8)
        return self.read(chunk + index % 65536 * ITEM_SIZE, ITEM_SIZE)

    def identity(self, obj):
        if not obj: return None
        b = self.read(obj, 0x28)
        index = i32(b, 0xc)
        if u64(self.item(index), ITEM_POINTER) != obj: raise ValueError('Reused UObject index')
        return {'address': hex(obj), 'index': index, 'name': self.fname(b, 0x18),
            'class': self.name(u64(b, 0x10)), 'outer': self.name(u64(b, 0x20))}

    def weak(self, data):
        index, serial = struct.unpack('<ii', data)
        result = {'index': index, 'serial': serial, 'target': None}
        if serial == 0:
            result['status'] = 'null'
            return result
        item = self.item(index)
        if serial < 0 or i32(item, ITEM_SERIAL) != serial:
            result['status'] = 'stale'
            return result
        obj = u64(item, ITEM_POINTER)
        if not obj: raise ValueError('Serial-valid item has null object')
        result['target'] = self.identity(obj)
        again = self.item(index)
        if i32(again, ITEM_SERIAL) != serial or u64(again, ITEM_POINTER) != obj:
            raise ValueError('Weak target changed during read')
        result['status'] = 'serial-validated'
        return result

    def schema(self, cls):
        if cls in self.schemas: return self.schemas[cls]
        result, current = {}, cls
        for _ in range(64):
            if not current: break
            b = self.read(current, 0x58)
            field, seen = u64(b, 0x50), set()
            while field:
                if field in seen or len(seen) >= 2048: raise ValueError('Invalid field chain')
                seen.add(field)
                f = self.read(field, 0x78)
                label = self.fname(f, 0x20)
                offset = i32(f, 0x44)
                if not 0 <= offset < 1048576: raise ValueError('Invalid property offset')
                result.setdefault(label, {'type': self.fname(self.read(u64(f, 8) + 8, 8)),
                    'offset': offset, 'size': i32(f, 0x34), 'byte': f[0x71], 'mask': f[0x73]})
                field = u64(f, 0x18)
            current = u64(b, 0x40)
        else: raise ValueError('Invalid class chain')
        self.schemas[cls] = result
        return result

    def properties(self, obj, labels):
        sc, values = self.schema(self.ptr(obj + 0x10)), {}
        for label in labels:
            if label not in sc: continue
            p = sc[label]
            at, kind = obj + p['offset'], p['type']
            if kind == 'ObjectProperty': val = self.identity(self.ptr(at))
            elif kind == 'BoolProperty': val = bool(self.read(at + p['byte'], 1)[0] & p['mask'])
            elif kind == 'FloatProperty': val = struct.unpack('<f', self.read(at, 4))[0]
            elif kind == 'IntProperty': val = i32(self.read(at, 4))
            elif kind == 'StructProperty' and p['size'] == 24:
                val = list(struct.unpack('<ddd', self.read(at, 24)))
            else: val = {'unsupportedType': kind}
            values[label] = val
        return values

    def chain(self, obj):
        result, seen = [], set()
        for _ in range(32):
            if not obj: return result
            if obj in seen: raise ValueError('Cyclic attachment')
            seen.add(obj)
            rec = self.identity(obj)
            rec['properties'] = self.properties(obj, ('AttachParent', 'Owner',
                'RelativeLocation', 'RelativeRotation', 'RelativeScale3D',
                'bAbsoluteLocation', 'bAbsoluteRotation', 'bAbsoluteScale'))
            result.append(rec)
            parent = rec['properties'].get('AttachParent')
            obj = int(parent['address'], 16) if parent else 0
        raise ValueError('Attachment chain exceeds bound')

    def capture(self):
        image = ctypes.create_unicode_buffer(32768)
        size = wintypes.DWORD(len(image))
        if not self.k.QueryFullProcessImageNameW(self.h, 0, image, ctypes.byref(size)):
            raise ctypes.WinError(ctypes.get_last_error())
        if Path(image.value).resolve() != Path(self.a.exe).resolve(): raise ValueError('Wrong process image')
        if self.fname(bytes(8)) != 'None': raise ValueError('Wrong name pool')
        detectors, targets, classes = [], [], {}
        for chunk_index in range((self.a.count + 65535) // 65536):
            count = min(65536, self.a.count - chunk_index * 65536)
            data = self.read(self.ptr(self.a.objects + chunk_index * 8), count * ITEM_SIZE)
            for n in range(count):
                obj = u64(data, n * ITEM_SIZE + ITEM_POINTER)
                if not obj: continue
                b = self.read(obj, 0x28)
                if i32(b, 0xc) != chunk_index * 65536 + n: raise ValueError('Unstable object scan')
                if i32(b, 8) & 0x30: continue  # CDO/archetype
                cls = u64(b, 0x10)
                if cls not in classes: classes[cls] = self.name(cls)
                cn = classes[cls]
                if cn == 'InteractiveDetectorPointerComponent': detectors.append(obj)
                if cn == 'BoxComponent' and self.fname(b, 0x18).startswith('HarpoonEntryQuery'):
                    targets.append(obj)
                if cn == 'BP_HarpoonOperator_C' and self.name(self.ptr(cls + 0x20)) == '/Game/Mods/HarpoonCannon/Station/BP_HarpoonOperator':
                    targets.append(obj)
        if not detectors: raise ValueError('No live detector; load a save first')
        samples = []
        for detector in detectors:
            if self.ptr(detector) != self.a.base + DETECTOR_VTABLE_RVA:
                raise ValueError('Unreviewed detector vtable')
            rec = self.identity(detector)
            rec['outerIdentity'] = self.identity(self.ptr(detector + 0x20))
            rec['settings'] = self.properties(detector, ('MaxReach', 'RayShapeRadius'))
            before = self.read(detector + 0x360, 0x18)
            rec['caches'] = {key: self.weak(before[offset - 0x360:offset - 0x360 + 8])
                            for key, offset in CACHE_OFFSETS.items()}
            rec['cacheStableDuringRead'] = before == self.read(detector + 0x360, 0x18)
            for value in rec['caches'].values():
                if value['target']:
                    value['attachmentChain'] = self.chain(int(value['target']['address'], 16))
            samples.append(rec)
        probes = []
        for obj in targets:
            rec = self.identity(obj)
            rec['properties'] = self.properties(obj, MARKERS + ('BoxExtent', 'bActorEnableCollision'))
            rec['attachmentChain'] = self.chain(obj)
            probes.append(rec)
        return {'detectors': samples, 'probeObjects': probes,
            'schemas': {self.name(cls): schema for cls, schema in self.schemas.items()}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('pid', 'base', 'objects', 'count'):
        parser.add_argument('--' + key, type=integer, required=True)
    for key in ('exe', 'output', 'label'): parser.add_argument('--' + key, required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    if out.exists() or 'artifacts' not in out.parts: parser.error('Use a NEW output below artifacts')
    if not 10000 <= args.count <= 5000000: parser.error('Invalid object count')
    if hashlib.sha256(Path(args.exe).read_bytes()).hexdigest() != EXE_HASH:
        parser.error('Unreviewed executable fingerprint')
    started = time.monotonic()
    reader = Reader(args)
    payload = {'label': args.label, 'pid': args.pid, 'sha256': EXE_HASH,
        'capturedAtUnix': time.time(), 'boundary': 'Non-atomic read only; no calls or writes. '
        'Cache state is NOT a record of dispatched provider calls. Relative transforms only, not world bounds.'}
    code = 0
    try:
        payload.update(reader.capture())
        payload['status'] = 'captured'
    except (OSError, ValueError, TimeoutError, struct.error) as error:
        payload.update(status='invalid', error=str(error))
        code = 2
    finally:
        reader.k.CloseHandle(reader.h)
    payload['seconds'] = round(time.monotonic() - started, 2)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open('x', encoding='utf-8') as stream: json.dump(payload, stream, indent=2)
    print(json.dumps({'status': payload['status'], 'output': str(out),
        'detectors': len(payload.get('detectors', [])), 'seconds': payload['seconds'],
        'error': payload.get('error')}))
    return code


if __name__ == '__main__': raise SystemExit(main())
