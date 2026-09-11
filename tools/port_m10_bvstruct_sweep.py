#!/usr/bin/env python3
"""PORT-M10/M11 -- sweep LEGOLAND/*.c for the silent wasm32 by-value-struct ABI class.

A parameter that one TU spells as a BY-VALUE AGGREGATE (more than one member,
total size <= 4 bytes) and another TU spells as a SCALAR occupies the same
dword on x86 cdecl but is passed INDIRECTLY on wasm32 while the scalar goes
direct.  Both lower to one i32 parameter, so wasm-ld reports no signature
mismatch and nothing traps -- the callee reads a shadow-stack pointer.

Larger aggregates (> 4 bytes) change the wasm ARITY, so wasm-ld already warns
about those and the existing gates catch them; they are reported separately.

PORT-M11 added three things:

  * The sweep is PREPROCESSOR-AWARE for `LEGOLAND_PORTABLE`.  The class only
    exists in the portable build, so only the lines the portable build COMPILES
    count: an `#ifndef LEGOLAND_PORTABLE` arm is the VC6 text and is skipped.
    Without this, every fix of the class -- which is always a portable arm next
    to the matched spelling -- reads as a fresh hit in the same file, and the
    sweep can never reach 0.  It also follows the `#define X X_vc6_body` rename
    (PORT-M3's trick): when the matched body is renamed for the portable build,
    the signature that matters is the WRAPPER's.

  * Nested aggregates are resolved, so `union BPosW { unsigned short w; BPos b; }`
    is 2 bytes / 2 members -- IN the silent window -- instead of "size unknown"
    in the noisy bucket.  Every `BPosW` spelling of a map square was invisible
    before this.

  * A third section: the SLOT direction.  A function-pointer field whose own
    parameter list carries a by-value aggregate <= 4 bytes is called
    indirectly with a POINTER on wasm32; a body installed in that slot which
    spells the parameter as a scalar reads the pointer as a value, and neither
    wasm-ld nor `portable/tests/test_callback_types.c` (which type-checks the
    slots as `void*`) can see it.  The decl/def sweep cannot see it either:
    the body and its declarations can agree perfectly and still disagree with
    the slot they are stored in.

The sweep is source-level and deliberately over-reports: every hit is meant to
be read by eye.  Exit status is 1 when the silent window or the slot section is
non-empty, so it can be used as a gate.
"""
import os, re, sys, collections

ROOT = 'LEGOLAND'
for a in sys.argv[1:]:
    if not a.startswith('-'):
        ROOT = a

SCALAR_WORDS = {
    'void', 'char', 'short', 'int', 'long', 'float', 'double',
    'signed', 'unsigned', '_Bool', 'const', 'volatile', 'struct', 'union',
    'enum', '__stdcall', '__cdecl', '__declspec', 'dllimport', 'size_t',
    'unsigned int', 'unsigned char', 'unsigned short', 'unsigned long',
}

# an extern function declaration on one line, with a trailing address comment
DECL = re.compile(
    r'^\s*extern\s+(?:__declspec\(dllimport\)\s+)?(?:__stdcall\s+)?'
    r'(?P<ret>[A-Za-z_][A-Za-z0-9_]*(?:\s+[A-Za-z_][A-Za-z0-9_]*)*[\s*]+)'
    r'(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\((?P<args>[^;]*)\)\s*;'
    r'.*?/\*\s*(?P<addr>0x[0-9a-fA-F]{6,8})')
MARKER = re.compile(r'^//\s*(?:WIP-)?FUNCTION:\s*LEGOLAND\s+(0x[0-9a-fA-F]{8})')
RENAME = re.compile(r'^\s*#\s*define\s+([A-Za-z_]\w*)\s+(\1_vc6_body)\s*$')

# a function-pointer FIELD or typedef:  void (*remove)(void* obj, BPos bp, void* c);
FPFIELD = re.compile(
    r'^\s*(?:const\s+)?[A-Za-z_]\w*(?:\s+[A-Za-z_]\w*)*\s*\*?\s*'
    r'\(\s*\*\s*(?P<field>[A-Za-z_]\w*)\s*\)\s*\((?P<args>[^;]*)\)\s*;'
    r'(?:[^\n]*?/\*\s*\+(?P<off>0x[0-9a-fA-F]+))?')
# any struct field with a trailing offset comment:  void* cb_remove;  /* +0x9c */
OFFFIELD = re.compile(
    r'^\s*[^;{}]*?\b(?P<field>[A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*;'
    r'[^\n]*?/\*\s*(?:\[\d*\]\s*)?(?:->\s*)?\+(?P<off>0x[0-9a-fA-F]+)')
# an assignment of a bare function name into a slot:  p->f4 = (void*)Standard...;
# any extern declaration that carries the callee's address: the bridge from a
# stale alias NAME to the address its body is marked with (PORT-M12).
DECLADDR = re.compile(
    r'^\s*extern\s+.*?(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\([^;]*\)\s*;'
    r'.*?/\*\s*(?P<addr>0x[0-9a-fA-F]{6,8})')

SLOTSET = re.compile(
    r'(?:->|\.)(?P<field>[A-Za-z_]\w*)\s*=\s*(?:\(\s*[A-Za-z_][\w\s*]*\)\s*)?'
    r'(?P<fn>[A-Za-z_]\w*)\s*;')


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


def live_lines(text):
    """The lines of `text` the PORTABLE build compiles, 1-based.

    Only `LEGOLAND_PORTABLE` is evaluated; every other condition is treated as
    taken (and so is its #else), because the sweep would rather over-report
    than lose a site.
    """
    live = set()
    # stack entries: None = not about LEGOLAND_PORTABLE, True/False = this arm is live
    stack = []
    for i, ln in enumerate(text.splitlines(), 1):
        s = ln.strip()
        m = re.match(r'#\s*(ifdef|ifndef|if|else|elif|endif)\b(.*)', s)
        if m:
            kw, rest = m.group(1), m.group(2)
            if kw in ('ifdef', 'ifndef', 'if'):
                about = 'LEGOLAND_PORTABLE' in rest
                if not about:
                    stack.append(None)
                elif kw == 'ifdef':
                    stack.append(True)
                elif kw == 'ifndef':
                    stack.append(False)
                else:
                    neg = '!' in rest.split('LEGOLAND_PORTABLE')[0]
                    stack.append(not neg)
            elif kw in ('else', 'elif'):
                if stack and stack[-1] is not None:
                    stack[-1] = not stack[-1]
            elif kw == 'endif':
                if stack:
                    stack.pop()
            continue
        if all(v is not False for v in stack):
            live.add(i)
    return live


def struct_info(text, name, _depth=0):
    """(size, members) for a struct/union typedef'd in `text`, or None.

    Nested aggregates are resolved recursively -- `union BPosW { unsigned short
    w; BPos b; }` is (2, 2), which is inside the silent window.
    """
    if _depth > 4:
        return None
    m = re.search(r'typedef\s+(struct|union)\s+(?:' + re.escape(name) +
                  r'\s*)?\{(.*?)\}\s*' + re.escape(name) + r'\s*;', text, re.S)
    if not m:
        return None
    kind, body = m.group(1), m.group(2)
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
    body = re.sub(r'//[^\n]*', '', body)
    size, members = 0, 0
    for decl in body.split(';'):
        decl = decl.strip()
        if not decl:
            continue
        nested = None
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
            # a nested aggregate: resolve its own typedef
            words = [t for t in re.findall(r'[A-Za-z_]\w*', decl)
                     if t not in ('const', 'volatile', 'struct', 'union', 'enum')]
            if not words:
                return None
            nested = struct_info(text, words[0], _depth + 1)
            if not nested:
                return None
            w = nested[0]
        names = [d for d in decl.split(',')]
        for nm in names:
            cnt = 1
            arr = re.search(r'\[\s*(0x[0-9a-fA-F]+|\d+)\s*\]', nm)
            if arr:
                cnt = int(arr.group(1), 0)
            members += nested[1] * cnt if nested else 1
            if kind == 'struct':
                size += w * cnt
            else:
                size = max(size, w * cnt)
    return size, members


def scan_file_bodies(f, text, live):
    """Every marked definition in one file as the PORTABLE build sees it.

    Yields (address, line, name, raw arg strings, signature).  A body renamed
    with `#define X X_vc6_body` is reported with the WRAPPER's signature, since
    that is the X the rest of the portable link sees.
    """
    lines = text[f].splitlines()
    renamed, offs = set(), []
    acc = 0
    for ln in lines:
        offs.append(acc)
        acc += len(ln) + 1
    for i, ln in enumerate(lines, 1):
        if i not in live[f]:
            continue
        r = RENAME.match(ln)
        if r:
            renamed.add(r.group(1))
            continue
        mk = MARKER.match(ln)
        if not mk:
            continue
        j, sig = i, ''
        while j < len(lines) and len(sig) < 400:
            sig += ' ' + lines[j].strip()
            if '(' in sig and sig.count('(') <= sig.count(')'):
                break
            j += 1
        m2 = re.search(r'(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\((?P<args>[^)]*)\)', sig)
        if not m2:
            continue
        name, args = m2.group('name'), split_args(m2.group('args'))
        sig = sig.strip()
        if name in renamed:
            # PORT-M3's rename: the portable build exports the WRAPPER
            w = re.search(r'#\s*undef\s+' + re.escape(name) + r'\s*\n'
                          r'(?:[^\n]*\n){0,6}?[^\n]*?\b' + re.escape(name) +
                          r'\s*\((?P<args>[^)]*)\)', text[f][offs[i - 1]:])
            if w:
                args = split_args(w.group('args'))
                sig = name + '(' + w.group('args') + ')   [portable wrapper]'
        yield mk.group(1).lower().replace('0x00', '0x'), i + 1, name, args, sig


def collect(files, text, live):
    """address -> declarations, and address -> the portable-visible definition."""
    sites = collections.defaultdict(list)
    defs = {}
    for f in files:
        for i, ln in enumerate(text[f].splitlines(), 1):
            if i not in live[f]:
                continue
            m = DECL.match(ln)
            if m:
                args = [classify(a) for a in split_args(m.group('args'))]
                sites[m.group('addr').lower().replace('0x00', '0x')].append(
                    (f, i, m.group('name'), args, ln.strip(), 'decl'))
        for addr, lno, name, args, sig in scan_file_bodies(f, text, live):
            defs[addr] = (f, lno, name, [classify(a) for a in args], sig)
    return sites, defs


def find_slot_hazards(files, text, live):
    """Bodies whose parameter disagrees with the aggregate <= 4B slot they sit in.

    A field `void (*remove)(void*, BPos, void*)` is called indirectly; on
    wasm32 the BPos argument is a POINTER.  A body stored in that field which
    spells the same parameter as a scalar reads the pointer as a value.
    """
    # Every struct field carries its own offset comment in this tree, and the
    # SAME slot is spelled with a different field name in every file that types
    # the record (ObjDef +0x9c is `remove` in objmap2.c, `cb_remove` in
    # interfaces.c and castleobj.c, `cb_9c` in loaders.c, `f4` in sweep3.c).
    # So slots are keyed by OFFSET, and every field name seen at that offset is
    # an alias for it.
    alias = collections.defaultdict(set)      # offset -> {field names}
    for f in files:
        for i, ln in enumerate(text[f].splitlines(), 1):
            if i not in live[f]:
                continue
            m = OFFFIELD.match(ln)
            if m:
                alias[m.group('off').lower()].add(m.group('field'))

    # offset (or field name, when the field carries no offset comment) -> shape
    slots = {}
    for f in files:
        for i, ln in enumerate(text[f].splitlines(), 1):
            if i not in live[f]:
                continue
            m = FPFIELD.match(ln)
            if not m:
                continue
            for pos, a in enumerate(split_args(m.group('args'))):
                k = classify(a)
                if not k.startswith('aggregate:'):
                    continue
                tname = k.split(':', 1)[1]
                info = struct_info(text[f], tname)
                if info and info[0] <= 4 and info[1] > 1:
                    off = (m.group('off') or '').lower()
                    key = off if off else m.group('field')
                    slots.setdefault(key, []).append((f, i, pos, tname, info))
    if not slots:
        return []
    names = {}                                 # field name -> slot key
    for key, decls in slots.items():
        for n in alias.get(key, ()):
            names[n] = key
        for (_f, _i, _p, _t, _inf) in decls:
            pass
        names.setdefault(key, key)
    # the field name the function-pointer declaration itself used is an alias
    for f in files:
        for i, ln in enumerate(text[f].splitlines(), 1):
            if i not in live[f]:
                continue
            m = FPFIELD.match(ln)
            if m:
                off = (m.group('off') or '').lower()
                if off and off in slots:
                    names[m.group('field')] = off
    # every function name stored into one of those fields
    stored = collections.defaultdict(set)
    for f in files:
        for i, ln in enumerate(text[f].splitlines(), 1):
            if i not in live[f]:
                continue
            for m in SLOTSET.finditer(ln):
                key = names.get(m.group('field'))
                if key:
                    stored[key].add((m.group('fn'), f, i))
    # every definition in the tree, by name AND by address, as the PORTABLE
    # build sees it.
    #
    # PORT-M12: by name was not enough.  262 functions in this tree are
    # DECLARED under a name that is not the name of their definition
    # (gen_link's "function aliases (stale extern names)"), and a slot is
    # filled with the declaration's name: `def->cb_draw = SpaceTower_Draw;`
    # against `RideDrawDesc* SpaceTower_GetDrawDesc(...)` in mechrides.c.  For
    # every one of those `bodies.get(fn)` returned None and the row was DROPPED
    # -- silently, which is the one thing a gate may never do.  Eighteen stores
    # of ten +0xa0 draw bodies were hiding behind exactly that, including the
    # Space Tower's and all nine western-town shopfronts'.  The stored name is
    # now resolved through its own declaration's /* 0xADDR */ comment and the
    # body looked up by ADDRESS; the name path stays first so an unaliased
    # store costs nothing.
    bodies, bodies_addr = {}, {}
    for f in files:
        for addr, lno, name, args, sig in scan_file_bodies(f, text, live):
            bodies[name] = (f, lno, args, sig)
            bodies_addr[addr.lower().replace('0x00', '0x')] = (f, lno, args,
                                                               sig)
    name_addr = collections.defaultdict(set)
    for f in files:
        for i, ln in enumerate(text[f].splitlines(), 1):
            if i not in live[f]:
                continue
            m = DECLADDR.match(ln)
            if m:
                name_addr[m.group('name')].add(
                    m.group('addr').lower().replace('0x00', '0x'))
    out = {}
    for field, decls in sorted(slots.items()):
        f0, i0, pos, tname, info = decls[0]
        for fn, sf, sl in sorted(stored.get(field, ())):
            b = bodies.get(fn)
            if not b:
                cand = [bodies_addr[a] for a in sorted(name_addr.get(fn, ()))
                        if a in bodies_addr]
                b = cand[0] if len(cand) == 1 else None
            if not b:
                continue
            bf, bl, bargs, bsig = b
            if pos >= len(bargs):
                continue
            k = classify(bargs[pos])
            if k.startswith('aggregate:'):
                bi = struct_info(text[bf], k.split(':', 1)[1])
                if bi is None or (bi[0] <= 4 and bi[1] > 1):
                    continue        # an aggregate of the same shape: fine
            elif k in ('void', 'varargs', 'unknown'):
                continue
            elif k == 'ptr':
                continue            # a pointer receives the pointer: harmless
            key = (field, fn)
            row = out.setdefault(key, [field, f0, i0, pos, tname, info, fn, bf,
                                      bl, bargs[pos], []])
            row[10].append(f'{sf}:{sl}')
    return [tuple(v) for v in out.values()]


def main():
    files = sorted(f for f in os.listdir(ROOT) if f.endswith('.c'))
    text = {f: open(os.path.join(ROOT, f), errors='replace').read() for f in files}
    live = {f: live_lines(text[f]) for f in files}

    sites, defs = collect(files, text, live)

    silent, noisy = [], []
    for addr, rows in sites.items():
        allrows = list(rows)
        if addr in defs:
            f, i, n, a, raw = defs[addr]
            allrows.append((f, i, n, a, raw, 'def'))
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

    hazards = find_slot_hazards(files, text, live)

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

    show('SILENT on wasm32 (multi-member aggregate <= 4 bytes: indirect vs direct, '
         'same i32 arity, NO wasm-ld warning)', silent)
    show('NOISY (wasm-ld already warns: the arity differs) or size unknown', noisy)

    print('=' * 78)
    print('SLOT vs BODY: an aggregate <= 4B in a function-POINTER field, and a '
          f'body stored in it that spells it a scalar -- {len(hazards)} site(s)')
    print('=' * 78)
    for (field, f0, i0, pos, tname, info, fn, bf, bl, barg, at) in sorted(hazards):
        print(f'\nslot `{field}` arg {pos} is `{tname}` ({info[0]}B/{info[1]} members)'
              f'  {f0}:{i0}')
        print(f'    BODY {fn} takes `{barg}`  {bf}:{bl}')
        print(f'         stored at {", ".join(at[:6])}'
              + (f' (+{len(at) - 6} more)' if len(at) > 6 else ''))

    if silent or hazards:
        sys.exit(1)


SELFTEST = {
    # a FIXED pair: VC6 sees the scalar, the portable build sees the aggregate.
    # Must NOT be reported -- without preprocessor awareness it always was, and
    # the sweep could never reach 0 however many sites were fixed.
    'a.c': """typedef struct BPos { unsigned char x, y; } BPos;
typedef union BPosW { unsigned short w; BPos b; } BPosW;
#ifndef LEGOLAND_PORTABLE
extern void F(void* o, unsigned int t);   /* 0x00401000 */
#else
extern void F(void* o, BPosW t);          /* 0x00401000 */
#endif
""",
    'b.c': """typedef struct BPos { unsigned char x, y; } BPos;
// FUNCTION: LEGOLAND 0x00401000
void F(void* o, BPos t) { (void)o; (void)t; }
""",
    # an UNFIXED pair: must be reported.
    'c.c': """extern void G(void* o, unsigned int t);   /* 0x00402000 */
""",
    'd.c': """typedef struct BPos { unsigned char x, y; } BPos;
// FUNCTION: LEGOLAND 0x00402000
void G(void* o, BPos t) { (void)o; (void)t; }
""",
    # a body renamed for the portable build, exporting the SLOT's own shape:
    # the wrapper is what the link sees, so this must NOT be reported either.
    'e.c': """typedef struct BPos { unsigned char x, y; } BPos;
typedef struct Cls {
    void (*remove)(void* o, BPos sq);   /* +0x9c */
} Cls;
#ifdef LEGOLAND_PORTABLE
#define H H_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00403000
void H(void* o, unsigned int sq) { (void)o; (void)sq; }
#ifdef LEGOLAND_PORTABLE
#undef H
void H(void* o, BPos sq) { H_vc6_body(o, sq.x | (sq.y << 8)); }
#endif
void Install(Cls* c) { c->remove = H; }
""",
    # the same slot, a body that was NOT fixed: must be reported.
    'f.c': """typedef struct BPos { unsigned char x, y; } BPos;
typedef struct Cls2 {
    void (*remove)(void* o, BPos sq);   /* +0x9c */
} Cls2;
// FUNCTION: LEGOLAND 0x00404000
void J(void* o, unsigned int sq) { (void)o; (void)sq; }
void Install2(Cls2* c) { c->remove = J; }
""",
}


def selftest():
    """Prove the three things this sweep claims it can do, on a tiny tree."""
    import tempfile, io, contextlib
    global ROOT
    d = tempfile.mkdtemp(prefix='bvstruct-selftest-')
    for name, body in SELFTEST.items():
        open(os.path.join(d, name), 'w').write(body)
    ROOT = d
    buf = io.StringIO()
    try:
        with contextlib.redirect_stdout(buf):
            main()
    except SystemExit:
        pass
    out = buf.getvalue()
    ok = True

    def want(cond, what):
        nonlocal ok
        print('%-4s %s' % ('ok' if cond else 'FAIL', what))
        ok = ok and cond

    want('0x402000' in out, 'an unfixed decl/def pair is reported')
    want('0x401000' not in out,
         'a pair fixed with a portable ARM is not (preprocessor awareness)')
    want('BODY J takes' in out, 'an unfixed body in an aggregate SLOT is reported')
    want('BODY H takes' not in out,
         'a body fixed with the _vc6_body rename is not (the wrapper is what links)')
    want('`BPosW` (2B/' in out or 'BPosW' not in out,
         'a nested union resolves to a size, not "size unknown"')
    print()
    print('selftest', 'PASSED' if ok else 'FAILED')
    sys.exit(0 if ok else 1)


if '--selftest' in sys.argv:
    selftest()
main()
