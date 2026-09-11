"""Read-only diagnostic: decode PE functions containing requested member offsets.

Usage: python Inspect-VoyageNativeMemberAccess.py EXE OUTPUT --offset 0x778
Requires pefile and capstone. Output is game-derived and must stay in artifacts/.
Byte-pattern candidates are bounded by PE exception function ranges, then
filtered by decoded memory operands. Matches are evidence, not owner proof.
"""
import argparse
import bisect
import hashlib
import json
from pathlib import Path
import struct
import capstone
import pefile

p = argparse.ArgumentParser()
p.add_argument('exe')
p.add_argument('output')
p.add_argument('--offset', action='append', type=lambda x: int(x, 0), default=[])
p.add_argument('--start-va', type=lambda x: int(x, 0))
p.add_argument('--size', type=lambda x: int(x, 0), default=512)
a = p.parse_args()
out = Path(a.output)
if out.exists():
    raise SystemExit('Output already exists')
raw = Path(a.exe).read_bytes()
pe = pefile.PE(data=raw, fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
directory = pe.OPTIONAL_HEADER.DATA_DIRECTORY[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXCEPTION']]
table = pe.get_data(directory.VirtualAddress, directory.Size)
funcs = sorted((start, end) for start, end, unwind in struct.iter_unpack('<III', table) if start < end)
starts = [x[0] for x in funcs]
candidates = set()
if a.start_va is not None:
    if not 1 <= a.size <= 65536:
        raise SystemExit('Exact window size must be 1..65536')
    candidates.add((a.start_va-base, a.start_va-base+a.size))
elif not a.offset:
    raise SystemExit('Supply --offset or --start-va')
for s in pe.sections:
    if not s.Characteristics & 0x20000000:
        continue
    data = s.get_data()
    for offset in a.offset:
        pat = struct.pack('<I', offset)
        pos = data.find(pat)
        while pos >= 0:
            rva = s.VirtualAddress + pos
            ix = bisect.bisect_right(starts, rva) - 1
            if ix >= 0 and funcs[ix][0] <= rva < funcs[ix][1]:
                candidates.add(funcs[ix])
            pos = data.find(pat, pos + 1)
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
md.detail = True
records = []
for start, end in sorted(candidates):
    if end - start > 200000:
        continue
    instructions = list(md.disasm(pe.get_data(start, end-start), base+start))
    matches = []
    for i in instructions:
        for op in i.operands:
            if op.type == capstone.x86.X86_OP_MEM and op.mem.disp in a.offset:
                matches.append({'va': hex(i.address), 'offset': hex(op.mem.disp), 'instruction': i.mnemonic+' '+i.op_str})
    if matches or a.start_va is not None:
        records.append({'start': hex(base+start), 'end': hex(base+end), 'matches': matches,
                        'assembly': [hex(i.address)+' '+i.mnemonic+' '+i.op_str for i in instructions]})
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps({'sha256': hashlib.sha256(raw).hexdigest(), 'functions': records}, indent=2))
print(json.dumps({'output': str(out.resolve()), 'functions': len(records), 'candidates':len(candidates)}))
