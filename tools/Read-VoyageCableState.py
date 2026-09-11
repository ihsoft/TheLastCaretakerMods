"""Read-only cable diagnostic for reviewed fingerprints. No writes or injected code.

Pass fresh process/chunk identity from Find-VoyageUObjectArray.ps1. Offsets are
derived from current native registration and decoded creation code; never
reuse against another executable fingerprint. Output remains in artifacts/.
"""
import argparse, ctypes, hashlib, json, struct, time
from pathlib import Path
from ctypes import wintypes
p=argparse.ArgumentParser()
p.add_argument('--pid', type=int, required=True)
p.add_argument('--base', type=lambda x:int(x,0), required=True)
p.add_argument('--objects', type=lambda x:int(x,0), required=True)
p.add_argument('--count', type=int, required=True)
p.add_argument('--exe', required=True)
p.add_argument('--output', required=True)
a=p.parse_args()
expected=hashlib.sha256(Path(a.exe).read_bytes()).hexdigest()
reviewed={
    'ca84428cf4562c703bedff053db727d14cc70c593451c09be75a92828efd9933': '25056839',
    # Native class registration, reflected offsets and connected-force code
    # revalidated; evidence owned by docs/mooring-cable-length-research.md.
    '747dc2553f7e68d8ea7ed0b2e0cac6d08943ea3f50dd6ed822e9293e0b45f58b': '25191271',
}
if expected not in reviewed: raise SystemExit('Fingerprint mismatch')
out=Path(a.output)
if out.exists() or not 10000<=a.count<=5000000: raise SystemExit('Invalid output/count')
k=ctypes.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[wintypes.DWORD,wintypes.BOOL,wintypes.DWORD]; k.OpenProcess.restype=wintypes.HANDLE
k.ReadProcessMemory.argtypes=[wintypes.HANDLE,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_size_t)]
k.CloseHandle.argtypes=[wintypes.HANDLE]
h=k.OpenProcess(0x1010,False,a.pid)
if not h: raise ctypes.WinError(ctypes.get_last_error())
def read(addr,n):
    buf=ctypes.create_string_buffer(n); got=ctypes.c_size_t()
    if not k.ReadProcessMemory(h,addr,buf,n,ctypes.byref(got)) or got.value!=n: raise OSError('Unreadable address '+hex(addr))
    return buf.raw
def u64(b,o=0): return struct.unpack_from('<Q',b,o)[0]
def f32(b,o): return struct.unpack_from('<f',b,o)[0]
records=[]; socket_records=[]; errors=0; cache={}; begin=time.monotonic()
try:
    native=u64(read(a.base+0xb5df208,8))
    native_header=read(native,0x40)
    slot=struct.unpack_from('<i',native_header,0x38)[0]
    if not 0<=slot<=64: raise RuntimeError('Invalid native class chain')
    socket_class=u64(read(a.base+0xb5dea40,8))
    socket_slot=struct.unpack_from('<i',read(socket_class,0x40),0x38)[0]
    if not 0<=socket_slot<=64: raise RuntimeError('Invalid socket class chain')
    def object_kind(cls):
        if cls not in cache:
            b=read(cls,0x40); count=struct.unpack_from('<i',b,0x38)[0]
            cache[cls]=None
            for kind, target, index in (('cable',native,slot),('socket',socket_class,socket_slot)):
                if cls==target or (index<=count<=128 and u64(read(u64(b,0x30)+8*index,8))==target+0x30):
                    cache[cls]=kind
                    break
        return cache[cls]
    for chunk_index in range((a.count+65535)//65536):
        chunk=u64(read(a.objects+chunk_index*8,8))
        n=min(65536,a.count-chunk_index*65536)
        items=read(chunk,n*24)
        for i in range(n):
            if time.monotonic()-begin>60: raise RuntimeError('60-second read limit')
            obj=u64(items,i*24+8)
            if not obj: continue
            try:
                header=read(obj,0x28); cls=u64(header,0x10)
                kind=object_kind(cls) if cls else None
                if not kind: continue
                if kind=='socket':
                    b=read(obj,0x780); item=u64(b,0x770); length=f32(b,0x778)
                    if item or length in (2000,6000):
                        socket_records.append({'address':hex(obj),'class':hex(cls),'flags':hex(struct.unpack_from('<I',b,8)[0]),
                            'nameIds':struct.unpack_from('<II',b,0x18),'length':length,'item':hex(item),'outer':hex(u64(b,0x20))})
                    continue
                b=read(obj,0x698)
                flags=struct.unpack_from('<I',b,8)[0]
                physics=None
                # Additional diagnostics were reviewed on 25191271 only.
                if reviewed[expected]=='25191271':
                    component=u64(b,0x2c8)
                    if component:
                        cb=read(component,0x6c0)
                        segments=struct.unpack_from('<i',cb,0x628)[0]
                        width=f32(cb,0x68c); tightness=f32(b,0x410)
                        physics={'component':hex(component),'length':f32(cb,0x620),
                            'currentLength':f32(cb,0x624),'segments':segments,'width':width,
                            'tightness':tightness,'minSegments':struct.unpack_from('<i',b,0x404)[0],
                            'forceThreshold':(1+tightness)*segments*width,
                            'strength':f32(b,0x3ec),'strengthMax':f32(b,0x3e8),
                            'stretchMultiplier':f32(b,0x3f0),'stretchOffset':f32(b,0x3f4)}
                sockets=[]
                for offset in (0x620,0x628):
                    ptr=u64(b,offset)
                    if ptr:
                        sb=read(ptr,0x780)
                        sockets.append({'address':hex(ptr),'integratedLength':f32(sb,0x778),'item':hex(u64(sb,0x770))})
                records.append({'address':hex(obj),'class':hex(cls),'flags':hex(flags),'isDefault':bool(flags&0x10),
                    'nameIds':struct.unpack_from('<II',b,0x18),'maxLength':f32(b,0x3fc),'maxLengthBase':f32(b,0x400),
                    'integrated':b[0x414],'item':hex(u64(b,0x500)),'sockets':sockets,'physics':physics})
            except OSError: errors+=1
    out.parent.mkdir(parents=True,exist_ok=True)
    out.write_text(json.dumps({'pid':a.pid,'sha256':expected,'nativeClass':hex(native),'classSlot':slot,'readErrors':errors,'cables':records,'integratedSockets':socket_records},indent=2))
    print(json.dumps({'output':str(out.resolve()),'cables':len(records),'integratedSockets':len(socket_records),'readErrors':errors,'seconds':round(time.monotonic()-begin,2)}))
finally: k.CloseHandle(h)
