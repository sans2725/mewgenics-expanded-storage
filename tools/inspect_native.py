"""Read-only exact-build disassembly. Reports go only to ignored work/."""
import argparse
import bisect
import hashlib
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'work' / 'python'))
import capstone
import pefile

EXPECTED = '4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea'

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--game', required=True)
    parser.add_argument('--function', action='append', default=[])
    parser.add_argument('--range', nargs=2, metavar=('START', 'END'))
    parser.add_argument('--target', action='append', default=[])
    parser.add_argument('--operand', help='Find instructions containing this operand text')
    parser.add_argument('--string', action='append', default=[])
    parser.add_argument('--calls', action='append', default=[])
    args = parser.parse_args()
    raw = pathlib.Path(args.game).read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED:
        raise SystemExit('Unsupported executable')
    pe = pefile.PE(data=raw)
    image = pe.get_memory_mapped_image()
    base = pe.OPTIONAL_HEADER.ImageBase
    entries = [(x.struct.BeginAddress, x.struct.EndAddress) for x in pe.DIRECTORY_ENTRY_EXCEPTION]
    starts = [x[0] for x in entries]
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    md.skipdata = True
    if args.range:
        start, end = (int(value, 16) for value in args.range)
        if not 0 <= start < end <= len(image) or end-start > 65536:
            raise SystemExit('Invalid or excessive analysis range')
        for a, size, op, operands in md.disasm_lite(image[start:end], base+start):
            print(f'{a-base:08x} {op:10} {operands}')
        return
    def enclosing(rva):
        i = bisect.bisect_right(starts, rva) - 1
        return entries[i] if i >= 0 and rva < entries[i][1] else (rva, rva + 256)
    if args.function:
        for addr in args.function:
            start, end = enclosing(int(addr, 16))
            print(f'FUNCTION {start:x}-{end:x}')
            for a, size, op, operands in md.disasm_lite(image[start:end], base + start):
                print(f'{a-base:08x} {op:10} {operands}')
        return
    needles = args.string or ['house_storage_upgrades', 'storage_expansion', 'rep_reward_count',
               'steamcampaign', 'campaign', 'npc_favor', 'inventory_storage']
    strings = {}
    for target in args.target:
        strings[base + int(target, 16)] = 'target_' + target
    for needle in needles:
        at = 0
        while (at := image.find(needle.encode(), at)) >= 0:
            strings[base + at] = needle
            at += 1
    print('STRINGS', json.dumps({hex(k-base): v for k,v in strings.items()}))
    refs = []
    for section in pe.sections:
        if not section.Characteristics & 0x20000000:
            continue
        start = section.VirtualAddress
        end = start + section.Misc_VirtualSize
        for a, size, op, operands in md.disasm_lite(image[start:end], base + start):
            if op in ('call', 'jmp') and operands in [hex(base + int(v, 16)) for v in args.calls]:
                fn = enclosing(a-base)
                print(f'{a-base:08x} fn={fn[0]:x}-{fn[1]:x} {op} {operands}')
            if args.operand and args.operand in operands:
                fn = enclosing(a-base)
                print(f'{a-base:08x} fn={fn[0]:x}-{fn[1]:x} {op} {operands}')
            match = re.search(r'\[rip ([+-]) (0x[0-9a-f]+)\]', operands)
            if match:
                delta = int(match[2],16) * (1 if match[1] == '+' else -1)
                target = a + size + delta
                if target in strings:
                    fn = enclosing(a-base)
                    refs.append({'key':strings[target], 'rva':hex(a-base),
                                 'function':hex(fn[0]), 'end':hex(fn[1]),
                                 'instruction':op+' '+operands})
    (ROOT/'work'/'native-xrefs.json').write_text(json.dumps(refs,indent=2))
    print(json.dumps(refs,indent=2))

if __name__ == '__main__':
    main()
