#!/usr/bin/env python3
"""PORT-M10 -- sweep LEGOLAND/*.c for the silent wasm32 by-value-struct ABI class.

A parameter that one TU spells as a BY-VALUE STRUCT (more than one member,
total size <= 4 bytes) and another TU spells as a SCALAR occupies the same
dword on x86 cdecl but is passed INDIRECTLY on wasm32 while the scalar goes
direct.  Both lower to one i32 parameter, so wasm-ld reports no signature
mismatch and nothing traps -- the callee reads a shadow-stack pointer.

Larger aggregates (> 4 bytes) change the wasm ARITY, so wasm-ld already warns
about those and the existing gates catch them; they are reported separately.

The sweep is source-level and deliberately over-reports: every hit is meant to
be read by eye.
"""
import os, re, sys, collections

ROOT = sys.argv[1] if len(sys.argv) > 1 else 'LEGOLAND'

SCALAR_WORDS = {
    'void', 'char', 'short', 'int', 'long', 'float', 'double',
    'signed', 'unsigned', '_Bool', 'const', 'volatile', 'struct', 'union',
    'enum', '__stdcall', '__cdecl', '__declspec', 'dllimport', 'size_t',
    'unsigned int', 'unsigned char', 'unsigned short', 'unsigned long',
}

ADDR = re.compile(r'/\*\s*(0x[0-9a-fA-F]{6,8})')
# an extern function declaration on one line, with a trailing address comment
DECL = re.compile(
    r'^\s*extern\s+(?:__declspec\(dllimport\)\s+)?(?:__stdcall\s+)?'
    r'(?P<ret>[A-Za-z_][A-Za-z0-9_ ]*?[\s*]+)'
    r'(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\((?P<args>[^;]*)\)\s*;'
    r'.*?/\*\s*(?P<addr>0x[0-9a-fA-F]{6,8})')
MARKER = re.compile(r'^//\s*(?:WIP-)?FUNCTION:\s*LEGOLAND\s+(0x[0-9a-fA-F]{8})')


def split_args(s):
    out, depth, cur = [], 0, ''
    for ch in s:
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur.strip()); cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def classify(arg):
    """'ptr', 'scalar', 'aggregate:<Type>', 'varargs', 'void', 'unknown'."""
    a = arg.strip()
    if a in ('', 'void'):
        return 'void'
    if a == '...':
        return 'varargs'
    if '*' in a or '[' in a:
        return 'ptr'
    # drop the parameter NAME (last identifier) when the type has >= 2 words
    toks = re.findall(r'[A-Za-z_][A-Za-z0-9_]*', a)
    if not toks:
        return 'unknown'
    words = [t for t in toks if t not in ('const', 'volatile', 'struct', 'union', 'enum')]
    base = words[:-1] if len(words) > 1 else words
    # a declaration may omit the name: `extern void f(BPos);`
    if len(words) == 1:
        base = words
    joined = ' '.join(base)
    if all(w in SCALAR_WORDS for w in base) or joined in SCALAR_WORDS:
        return 'scalar'
    # a single non-scalar word that is the whole type -> an aggregate or a typedef
    return 'aggregate:' + base[-1]


def struct_info(text, name):
    """(size, members) for a struct/union typedef'd in `text`, or None."""
    m = re.search(r'typedef\s+(struct|union)\s+(?:' + re.escape(name) +
                  r'\s*)?\{(.*?)\}\s*' + re.escape(name) + r'\s*;', text, re.S)
    if not m:
        return None
    kind, body = m.group(1), m.group(2)
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
    size, members = 0, 0
    for decl in body.split(';'):
        decl = decl.strip()
        if not decl:
            continue
        if '*' in decl:
            w, n = 4, len(re.findall(r'\*', decl))
        elif re.search(r'\b(char|_Bool)\b', decl):
            w = 1
        elif re.search(r'\bshort\b', decl):
            w = 2
        elif re.search(r'\b(double)\b', decl):
            w = 8
        elif re.search(r'\b(int|long|float|unsigned|signed)\b', decl):
            w = 4
        else:
            return None          # a nested aggregate: give up, report anyway
        names = [d for d in decl.split(',')]
        for nm in names:
            cnt = 1
            arr = re.search(r'\[\s*(0x[0-9a-fA-F]+|\d+)\s*\]', nm)
            if arr:
                cnt = int(arr.group(1), 0)
            members += 1
            if kind == 'struct':
                size += w * cnt
            else:
                size = max(size, w * cnt)
    return size, members


def main():
    files = sorted(f for f in os.listdir(ROOT) if f.endswith('.c'))
    text = {f: open(os.path.join(ROOT, f), errors='replace').read() for f in files}

    # address -> list of (file, line, name, [classified args], raw)
    sites = collections.defaultdict(list)
    defs = {}

    for f in files:
        lines = text[f].splitlines()
        for i, ln in enumerate(lines, 1):
            m = DECL.match(ln)
            if m:
                args = [classify(a) for a in split_args(m.group('args'))]
                sites[m.group('addr').lower().replace('0x00', '0x')].append(
                    (f, i, m.group('name'), args, ln.strip(), 'decl'))
                continue
            mk = MARKER.match(ln)
            if mk:
                # the signature is the next non-blank, non-comment line(s)
                j, sig = i, ''
                while j < len(lines) and len(sig) < 400:
                    sig += ' ' + lines[j].strip()
                    if '(' in sig and sig.count('(') <= sig.count(')'):
                        break
                    j += 1
                m2 = re.search(r'(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\((?P<args>[^)]*)\)', sig)
                if m2:
                    args = [classify(a) for a in split_args(m2.group('args'))]
                    key = mk.group(1).lower().replace('0x00', '0x')
                    defs[key] = (f, i + 1, m2.group('name'), args, sig.strip())

    silent, noisy = [], []
    for addr, rows in sites.items():
        allrows = list(rows)
        if addr in defs:
            f, i, n, a, raw = defs[addr]
            allrows.append((f, i, n, a, raw, 'def'))
        # per position, collect the distinct classes
        width = max(len(r[3]) for r in allrows)
        for pos in range(width):
            kinds = {}
            for r in allrows:
                if pos < len(r[3]):
                    kinds.setdefault(r[3][pos], []).append(r)
            aggs = [k for k in kinds if k.startswith('aggregate:')]
            scal = [k for k in kinds if k in ('scalar', 'ptr')]
            if not (aggs and scal):
                continue
            for k in aggs:
                tname = k.split(':', 1)[1]
                info = None
                for r in kinds[k]:
                    info = struct_info(text[r[0]], tname)
                    if info:
                        break
                row = (addr, pos, tname, info, kinds[k], [x for s in scal for x in kinds[s]])
                if info and info[0] <= 4 and info[1] > 1:
                    silent.append(row)
                elif info and info[1] == 1:
                    pass                      # single-element struct: passed direct, safe
                else:
                    noisy.append(row)

    def show(title, rows):
        print('=' * 78)
        print(title, f'-- {len(rows)} site(s)')
        print('=' * 78)
        for addr, pos, tname, info, aggs, scals in sorted(rows):
            sz = f'{info[0]}B/{info[1]} members' if info else 'size unknown'
            print(f'\n{addr}  arg {pos}  aggregate `{tname}` ({sz})')
            for f, i, n, a, raw, kind in aggs:
                print(f'    AGG  {kind:4s} {f}:{i}  {raw[:110]}')
            for f, i, n, a, raw, kind in scals:
                print(f'    SCA  {kind:4s} {f}:{i}  {raw[:110]}')

    show('SILENT on wasm32 (multi-member struct <= 4 bytes: indirect vs direct, '
         'same i32 arity, NO wasm-ld warning)', silent)
    show('NOISY (wasm-ld already warns: the arity differs) or size unknown', noisy)


main()
