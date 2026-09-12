#!/usr/bin/env python3
"""Check COFF relocation identities that normalized instruction matching hides.

    python3 tools/relocs.py LEGOLAND/pathmisc2.c NewMechanicOrder 0x004995d0
    python3 tools/relocs.py LEGOLAND/pathmisc2.c          # every body in one file
    python3 tools/relocs.py --all --json /tmp/sl_results.json
    python3 tools/relocs.py --all --no-wip      # exact bodies only, as before
    python3 tools/relocs.py --self-test

Compiles once per source with audit.py's wrapper and /O2 /Gy /Gd.

An exact (`// FUNCTION:`) body is checked POSITION BY POSITION: its bytes are
known to match, so relocation i of ours must name the same address as operand i
of the original, and a disagreement is a `MISMATCH` line. That is the gate.

A WIP (`// WIP-FUNCTION:`) body cannot be checked that way -- its instructions
do not line up with the original's -- but it can still be checked as a SET:
every address the original body names must be named somewhere in ours, and vice
versa. Those differences print as `WIPRELOC ... MISSING/EXTRA` lines, never as
`MISMATCH`, so the integration gate (`relocs.py --all | grep MISMATCH`) is
unchanged by them; they are a lead for a matching lane, not a failure. P5-1 is
why: `CheckWorkerOnMouseStatus` read `g_input.mouse_a.MASK` (0x00813a4c) where
the original reads the cursor Y (0x00813a48), a same-sized global in a WIP body,
which every byte gate passed and no relocation check even looked at. The
original has no relocation table, so its side is read out of the operands (a
32-bit displacement or immediate inside the image, and every direct branch that
leaves the body); a literal that happens to look like an address is therefore
reported as MISSING, and `--no-wip` turns the whole pass off.

Exit status: 0 = all resolved positions agree, 1 = mismatches, 2 = incomplete
(unresolved positions or skipped functions), 3 = both mismatch and incomplete.
An unresolved position is never counted as an address mismatch, and a WIP set
difference is counted in neither: it cannot change the exit status.
"""
import argparse
from collections import Counter
import contextlib
import io
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

import capstone

from audit import annotated
from match import (ROOT, IMAGE_BASE, CL_WRAPPER, load_exe, rva2off,
                   obj_function_code, true_extent, compiled_body, compare,
                   _branch_target)

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
MD.detail = True
DIR32, DIR32NB, REL32 = 6, 7, 20


_ADDR = re.compile(r'0x[0-9a-fA-F]{6,8}\b')
_MARK = re.compile(r'^[ \t]*//\s*(?:WIP-)?FUNCTION:\s*LEGOLAND\s+(0x[0-9a-fA-F]+)', re.M)
_COMMENTS = re.compile(r'/\*.*?\*/|//[^\n]*', re.S)
_MASK = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|^[ \t]*\#(?:\\\n|[^\n])*', re.S | re.M)
_IDENT = r'[A-Za-z_][A-Za-z_0-9]*'


def _blank(match):
    return ''.join('\n' if c == '\n' else ' ' for c in match.group())


def _statements(text):
    """Top-level declaration/signature spans; function bodies never inspected."""
    clean = _MASK.sub(_blank, text)
    start = depth = 0
    function = False
    for i, char in enumerate(clean):
        if char == '{':
            if depth == 0:
                prefix = clean[start:i].strip()
                function = ('(' in prefix and '=' not in prefix
                            and not re.match(r'typedef\b', prefix))
                if function:
                    yield start, i, True, clean
            depth += 1
        elif char == '}':
            depth -= 1
            if depth == 0 and function:
                start, function = i + 1, False
        elif char == ';' and depth == 0:
            yield start, i + 1, False, clean
            start = i + 1


def _names(declaration):
    declaration = re.sub(r'__declspec\s*\([^()]*\)', '', declaration).strip()
    if not declaration or re.match(r'typedef\b', declaration):
        return []
    # Inline record declarations can contain their own named fields.
    declaration = re.sub(r'\{[^{}]*\}', ' ', declaration)
    pointer = re.search(r'\(\s*(?:(?:__cdecl|__stdcall|__fastcall)\s+)?\*\s*(' + _IDENT + r')\s*\)', declaration)
    calls = re.finditer(r'\b(' + _IDENT + r')\s*\(', declaration)
    call = next((m for m in calls if m.group(1) not in {'void', 'int', 'long', 'short', 'char', 'float', 'double', '__cdecl', '__stdcall', '__fastcall'}), None)
    if call and pointer and call.end() - 1 == pointer.start():
        call = None
    if call and (not pointer or call.start() < pointer.start()):
        return [call.group(1)]
    if pointer:
        return [pointer.group(1)]
    parts, start, depth = [], 0, 0
    for i, char in enumerate(declaration):
        if char in '([':
            depth += 1
        elif char in ')]':
            depth -= 1
        elif char == ',' and depth == 0:
            parts.append(declaration[start:i]); start = i + 1
    parts.append(declaration[start:])
    names = []
    for part in parts:
        part = re.sub(r'\[[^\]]*\]', '', part.split('=', 1)[0]).rstrip(' ;\t\r\n')
        found = re.search(r'\b(' + _IDENT + r')$', part)
        if not found:
            return []
        names.append(found.group(1))
    return names


def _primary_addresses(comment):
    """First address and an immediately following comma/slash address list.

    A prose suffix such as 'built by 0x004860f0' is not another binding.
    """
    found = list(_ADDR.finditer(comment))
    if not found:
        return []
    out = [int(found[0].group(), 16)]
    previous = found[0]
    for item in found[1:]:
        if not re.fullmatch(r'\s*[,/]\s*', comment[previous.end():item.start()]):
            break
        out.append(int(item.group(), 16)); previous = item
    return out


def _add(out, names, addresses):
    if len(names) == len(addresses):
        for name, address in zip(names, addresses):
            out.setdefault(name, set()).add(address)
    elif len(names) == 1 and addresses:
        out.setdefault(names[0], set()).update(addresses)


def _text_addresses(text, markers_only=False):
    out = {}
    comments = list(_COMMENTS.finditer(text))
    consumed_comment_end = 0
    for start, end, function, clean in _statements(text):
        names = _names(clean[start:end])
        if not names:
            continue
        code = re.search(r'\S', clean[start:end])
        code_start = start + code.start()
        if function:
            marks = list(_MARK.finditer(text[start:end]))
            if marks:
                _add(out, names, [int(marks[-1].group(1), 16)])
            continue
        if markers_only:
            continue
        # Prefer a same-line trailing comment over a preceding explanatory one.
        trailing = re.match(r'[ \t]*(/\*.*?\*/|//[^\n]*)', text[end:], re.S)
        if not trailing:
            # Long prototypes put their address-only trailing comment on the
            # next indented line. It belongs to THIS declaration, not the next.
            trailing = re.match(
                r'[ \t]*\n[ \t]+(/\*\s*(?:\[|@)?\s*0x[0-9a-fA-F]+.*?\*/)',
                text[end:], re.S)
        comment_text = ''
        if trailing:
            comment_text = trailing.group(1)
            consumed_comment_end = end + trailing.end()
            addresses = _primary_addresses(comment_text)
        else:
            previous = [c for c in comments if max(start, consumed_comment_end) <= c.start()
                        and c.end() <= code_start]
            addresses = []
            if previous:
                comment = previous[-1]
                comment_text = comment.group()
                # Do not borrow the preceding declaration's trailing comment.
                after_prior_statement = text[start:comment.start()]
                gap = text[comment.end():code_start]
                describes_name = any(re.search(r'\b' + re.escape(n) + r'\b', comment_text)
                                     for n in names)
                if ((start == 0 or '\n' in after_prior_statement)
                        and gap.isspace() and gap.count('\n') <= 1
                        and ('\n' not in comment_text or describes_name)
                        and not _MARK.search(comment_text)):
                    values = [int(x, 16) for x in _ADDR.findall(comment.group())]
                    if len(set(values)) == 1:
                        addresses = values[:1]
        if re.search(r'__declspec\s*\(\s*dllimport\s*\)', clean[start:end]):
            if not re.search(r'\bIAT\b|\[\s*0x[0-9a-fA-F]+\s*\]', comment_text):
                continue
            names = ['__imp__' + name for name in names]
        _add(out, names, addresses)
    return out


def source_addresses(path):
    """File-local annotations plus recursively included quoted local headers."""
    out, seen = {}, set()
    def visit(item):
        item = Path(item).resolve()
        if item in seen or not item.is_file():
            return
        seen.add(item)
        text = item.read_text(errors='replace')
        for name, values in _text_addresses(text).items():
            out.setdefault(name, set()).update(values)
        for header in re.findall(r'^\s*#\s*include\s*"([^"]+)"', text, re.M):
            visit(item.parent / header)
    visit(path)
    return out


def reference_addresses(root):
    """Export names, DECOMP global aliases and function-definition markers."""
    root, out = Path(root), {}
    for line in (root / 'symbols/legoland.exports.txt').read_text().splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[-1].startswith('0x'):
            _add(out, [parts[0]], [int(parts[-1], 16) + 0x400000])
    docs = (root / 'docs/DECOMP.md').read_text()
    section = docs.split('## Global names from the export table', 1)[-1].split('\n## ', 1)[0]
    for line in section.splitlines():
        columns = line.split('|')
        if len(columns) < 5:
            continue
        addresses = [int(x, 16) for x in _ADDR.findall(columns[1])]
        names = re.findall(r'`(' + _IDENT + r')`', columns[2])
        _add(out, names, addresses)
        if len(addresses) == 1:
            aliases = re.findall(r'`(g_[A-Za-z_0-9]+)`', columns[3])
            if 'calls it `GameInput`' in columns[3]:
                aliases.append('GameInput')
            for alias in aliases:
                _add(out, [alias], addresses)
    for path in sorted((root / 'LEGOLAND').glob('*.c')):
        for name, values in _text_addresses(path.read_text(errors='replace'), markers_only=True).items():
            out.setdefault(name, set()).update(values)
    return out



def c_name(name):
    """Remove exactly one calling-convention decoration, not C underscores."""
    if name.startswith('__imp_'):
        return re.sub(r'@\d+$', '', name)  # Keep import slots distinct from callees.
    if name.startswith(('_', '@')):
        name = name[1:]
    return re.sub(r'@\d+$', '', name)


class Coff:
    """The i386 sections, primary symbols and relocations needed for this audit."""

    def __init__(self, path):
        self.path = str(path)
        data = Path(path).read_bytes()
        machine, count, _, symptr, nsym, optsize, _ = struct.unpack_from('<HHIIIHH', data)
        if machine != 0x14c:
            raise ValueError('expected an i386 COFF object')
        self.sections = []
        for i in range(count):
            off = 20 + optsize + 40 * i
            size, raw, rel = struct.unpack_from('<III', data, off + 16)
            nrel = struct.unpack_from('<H', data, off + 32)[0]
            flags = struct.unpack_from('<I', data, off + 36)[0]
            if flags & 0x01000000:
                raise ValueError('COFF relocation overflow is not supported')
            self.sections.append({
                'name': data[off:off + 8].split(b'\0')[0].decode('latin1'),
                'code': data[raw:raw + size] if raw else b'',
                'relocs': [struct.unpack_from('<IIH', data, rel + 10 * j)
                           for j in range(nrel)],
            })
        self.symbols = {}
        strings = symptr + nsym * 18
        i = 0
        while i < nsym:
            off = symptr + i * 18
            rec = data[off:off + 18]
            if rec[:4] == b'\0' * 4:
                start = strings + struct.unpack_from('<I', rec, 4)[0]
                end = data.find(b'\0', start)
                if end < start:
                    raise ValueError('unterminated COFF symbol')
                name = data[start:end].decode('latin1')
            else:
                name = rec[:8].split(b'\0')[0].decode('latin1')
            value, section, typ, storage, aux = struct.unpack_from('<IhHBB', rec, 8)
            self.symbols[i] = dict(name=name, value=value, section=section,
                                   type=typ, storage=storage)
            i += 1 + aux  # Auxiliary records are not symbols.

    def function(self, name):
        found = [s for s in self.symbols.values()
                 if s['section'] > 0 and s['type'] & 0x20
                 and (s['name'] == name or c_name(s['name']) == name)]
        if len(found) != 1:
            raise ValueError(f'expected one COFF function {name}, found {len(found)}')
        return found[0]


def symbol_address(symbol, local, reference, function, address, byte_count, addend):
    name = symbol['name']
    key = c_name(name)
    # Static helpers may collide with unrelated exported globals (PathSprite
    # in misc3.c does). Only external linkage can use cross-file references.
    values = local.get(key, reference.get(key, set()) if symbol['storage'] == 2 else set())
    if len(values) == 1:
        return next(iter(values)), 'address annotation/reference'
    if values:
        return None, 'conflicting address annotations: ' + ', '.join(
            f'0x{v:08x}' for v in sorted(values))
    # Section-relative code labels are only meaningful inside the checked body.
    # Do not extrapolate this mapping into a switch table or an adjacent body.
    delta = symbol['value'] - function['value']
    if symbol['section'] == function['section'] and 0 <= delta + addend < byte_count:
        return address + delta, 'label in aligned function'
    if name.startswith(('$SG', '??_C')):
        reason = 'string literal'
    elif name.startswith(('__real@', '$F', '__xmm@')):
        reason = 'floating-point literal'
    elif name.startswith(('$L', '$T', '.text')):
        reason = 'jump table/local code symbol'
    elif name.startswith('__imp_'):
        reason = 'import slot without address annotation'
    elif name.startswith('.'):
        reason = 'section symbol without address annotation'
    else:
        reason = 'symbol without address annotation'
    return None, reason


def original_target(insn, offset, typ):
    """Decode the original field at the relocation's operand position."""
    field = offset - insn.address
    if (field, 4) not in ((insn.disp_offset, insn.disp_size),
                          (insn.imm_offset, insn.imm_size)):
        raise ValueError('relocation does not select a 32-bit operand')
    value = struct.unpack_from('<I', insn.bytes, field)[0]
    if typ == REL32:
        if field != insn.imm_offset or not (insn.mnemonic == 'call'
                                           or insn.mnemonic.startswith('j')):
            raise ValueError('REL32 is not a direct branch operand')
        value = (value + offset + 4) & 0xffffffff
    elif typ == DIR32NB:
        value = (value + IMAGE_BASE) & 0xffffffff
    elif typ != DIR32:
        raise ValueError(f'unsupported relocation type 0x{typ:04x}')
    return value


def check_relocations(coff, function, address, original, compiled, local, reference):
    section = coff.sections[function['section'] - 1]
    start = function['value']
    end = compiled[-1].address + compiled[-1].size
    rows = []
    counts = Counter()
    for offset, symidx, typ in section['relocs']:
        if typ == 0 or not start <= offset < start + end:
            continue
        counts['relocations'] += 1
        rel = offset - start
        index = next((i for i, ins in enumerate(compiled)
                      if ins.address <= rel < ins.address + ins.size), None)
        symbol = coff.symbols.get(symidx)
        row = dict(index=index, offset=rel, type=typ,
                   symbol=symbol['name'] if symbol else f'<symbol {symidx}>',
                   original=None, ours=None, addend=None)
        try:
            if index is None or symbol is None:
                raise ValueError('relocation outside decoded instructions or invalid symbol')
            ins, orig = compiled[index], original[index]
            if ins.size != orig.size:
                raise ValueError('instruction sizes differ')
            within = rel - ins.address
            if within + 4 > ins.size:
                raise ValueError('relocation crosses instruction boundary')
            row['original'] = original_target(orig, orig.address + within, typ)
            row['operand'] = f'{orig.mnemonic} {orig.op_str}'
            addend = struct.unpack_from('<i', section['code'], offset)[0]
            row['addend'] = addend
            base, reason = symbol_address(symbol, local, reference, function,
                                          address, end, addend)
            if base is None:
                raise ValueError(reason)
            row['ours'] = (base + addend) & 0xffffffff
            row['basis'] = reason
            if row['ours'] == row['original']:
                counts['matched'] += 1
                continue
            row.update(status='mismatch', reason='callee target' if typ == REL32
                       else 'address operand')
            counts['mismatches'] += 1
        except (ValueError, struct.error) as exc:
            row.update(status='unresolved', reason=str(exc))
            counts['unresolved'] += 1
        rows.append(row)
    return dict(counts), rows


def image_span(sections):
    """(first mapped virtual address, one past the last) of this image.

    The floor matters: a bit-mask immediate of 0x00400000 is the image BASE, not
    an address, and the PE header below the first section is never referenced.
    """
    return (IMAGE_BASE + min(va for va, _, _, _ in sections),
            IMAGE_BASE + max(va + max(vsz, raw) for va, vsz, _, raw in sections))


# An immediate operand of a bitwise instruction is a mask, not an address
# (`or dword ptr [esp + 0x48], 0x800000`). Displacements are still read.
_MASKING = ('and', 'or', 'xor', 'test', 'shl', 'shr', 'sar', 'rol', 'ror', 'bt')
# Switch dispatch: VC6 emits the jump table and the byte case-index table into
# the function's own COMDAT, just past the code, so the original's copies sit
# immediately after the original body and ours are local $L symbols.
_TABLE_WINDOW = 512


def original_references(body, address, size, span):
    """{address: (operand, kind)} for what the ORIGINAL body names outside itself.

    A linked body carries no relocation table, so the references are recovered
    from the operands: a 32-bit displacement or immediate whose value lands
    inside the image, and every direct call/branch whose target leaves the
    body. Branches within the body are labels, not references.

    `kind` is what the operand says the target is, which is how the unresolvable
    classes are told apart from a real name: `float` (an x87 instruction's
    absolute operand), `table` (an indexed read just past the body -- a switch
    table in the same COMDAT), `code` (a direct call/jmp) or `data`.
    """
    floor, limit = span
    end = address + size
    out = {}
    for insn in body:
        where = f'0x{insn.address:08x}: {insn.mnemonic} {insn.op_str}'
        target = _branch_target(insn)
        if target is not None:
            if not address <= target < end:
                out.setdefault(target, (where, 'code'))
            continue
        indexed = any(op.type == capstone.x86.X86_OP_MEM
                      and (op.mem.base or op.mem.index) for op in insn.operands)
        for offset, width, immediate in (
                (insn.disp_offset, insn.disp_size, False),
                (insn.imm_offset, insn.imm_size, True)):
            if width != 4 or offset <= 0 or offset + 4 > insn.size:
                continue
            if immediate and insn.mnemonic.startswith(_MASKING):
                continue
            value = struct.unpack_from('<I', insn.bytes, offset)[0]
            if not floor <= value < limit:
                continue
            if insn.mnemonic.startswith('f'):
                kind = 'float'
            elif indexed and not immediate and end <= value < end + _TABLE_WINDOW:
                kind = 'table'
            else:
                kind = 'data'
            out.setdefault(value, (where, kind))
    return out


def literal_bytes(coff, symbol):
    """The bytes of the string or float literal a symbol points at, or None.

    The linker places a literal wherever it likes, so its address cannot be
    derived -- but its CONTENT can be compared against the original's, which is
    what makes a literal reference checkable at all here.
    """
    index = symbol['section']
    if index <= 0 or index > len(coff.sections):
        return None
    data = coff.sections[index - 1]['code']
    start = symbol['value']
    if start >= len(data):
        return None
    if symbol['name'].startswith(('$SG', '??_C')):
        end = data.find(b'\0', start)
        return data[start:end + 1] if end >= start else None
    digits = symbol['name'].rsplit('@', 1)[-1]
    if re.fullmatch(r'[0-9a-fA-F]+', digits) and not len(digits) % 2:
        return data[start:start + len(digits) // 2] or None
    return None


def our_references(coff, function, address, compiled, local, reference, size=0):
    """What OUR body names: ({address: symbol}, unresolved, literals, tables).

    Position is not used: a WIP body's instructions do not line up with the
    original's. A relocation against a label inside the body -- or against
    anything else that resolves inside the original's extent, such as a
    recursive call -- is our own code, not a reference. An unresolved relocation
    is not claimed as an address either, but a string or float literal's CONTENT
    is returned so the original's copy can still be recognised, and a local
    code/jump-table symbol is counted so the original's switch tables can be.
    """
    section = coff.sections[function['section'] - 1]
    start = function['value']
    end = compiled[-1].address + compiled[-1].size if compiled else 0
    out, unresolved, literals, tables = {}, 0, [], 0
    for offset, symidx, typ in section['relocs']:
        if typ == 0 or not start <= offset < start + end:
            continue
        symbol = coff.symbols.get(symidx)
        if symbol is None:
            unresolved += 1
            continue
        addend = struct.unpack_from('<i', section['code'], offset)[0]
        base, reason = symbol_address(symbol, local, reference, function,
                                      address, end, addend)
        if base is None:
            unresolved += 1
            content = literal_bytes(coff, symbol)
            if content:
                literals.append(content)
            elif symbol['name'].startswith(('$L', '$T', '.text')):
                tables += 1
            continue
        if reason == 'label in aligned function':
            continue
        value = (base + addend) & 0xffffffff
        if address <= value < address + size:
            continue
        out.setdefault(value, symbol['name'])
    return out, unresolved, literals, tables


def exe_bytes(data, sections, address, count):
    off = rva2off(sections, address - IMAGE_BASE)
    return None if off is None else data[off:off + count]


def account_for(missing, theirs, literals, tables, data, sections):
    """The subset of `missing` that our body's UNRESOLVABLE references explain.

    A literal is matched by content: if the original holds our exact string or
    float bytes at the address it names, that reference is the same literal at a
    different place, not a different object. A switch table is matched by class
    and counted, because its contents are code addresses that cannot agree.
    Each of ours explains at most one of the original's.
    """
    pool, left, out = list(literals), tables, set()
    for value in missing:
        kind = theirs[value][1]
        if kind == 'table' and left:
            left -= 1
            out.add(value)
            continue
        for i, content in enumerate(pool):
            if exe_bytes(data, sections, value, len(content)) == content:
                pool.pop(i)
                out.add(value)
                break
    return out


def inspect_wip_function(coff, name, address, data, sections, local, reference):
    """Set-compare the addresses a WIP body names against the original's.

    The bytes of a WIP body are known NOT to match, so there is nothing to gate
    here; the output is a lead. Both extents are the same ones the byte gates
    use: the original's by control flow (`true_extent`), ours by our own first
    unjumped `ret` (`compiled_body` with no target count).
    """
    result = dict(name=name, address=address, status='wip_checked', counts={},
                  rows=[], wip=True)
    try:
        function = coff.function(name)
        code = obj_function_code(coff.path, function['name'])
        count, size = true_extent(data, sections, address - IMAGE_BASE)
        if count is None:
            raise ValueError('original extent is unknown')
        off = rva2off(sections, address - IMAGE_BASE)
        original = list(MD.disasm(data[off:off + size], address))
        compiled, _ = compiled_body(list(MD.disasm(code, 0)), None)
        theirs = original_references(original, address, size, image_span(sections))
        ours, unresolved, literals, tables = our_references(
            coff, function, address, compiled, local, reference, size)
        missing = sorted(set(theirs) - set(ours))
        accounted = account_for(missing, theirs, literals, tables, data, sections)
        result.update(instructions=count, bytes=size)
        for value in missing:
            if value in accounted:
                continue
            where, kind = theirs[value]
            result['rows'].append(dict(status='wipreloc', kind='MISSING',
                                       original=value, ours=None, symbol=None,
                                       reason=f'{where} [{kind}]'))
        for value in sorted(set(ours) - set(theirs)):
            result['rows'].append(dict(status='wipreloc', kind='EXTRA',
                                       original=None, ours=value,
                                       symbol=ours[value],
                                       reason='named by our body only'))
        result['counts'] = {
            'wip_references_ours': len(ours),
            'wip_references_original': len(theirs),
            'wip_references_shared': len(set(ours) & set(theirs)),
            'wip_references_missing': len(missing) - len(accounted),
            'wip_references_accounted': len(accounted),
            'wip_references_extra': len(set(ours) - set(theirs)),
            'wip_unresolved': unresolved,
        }
    except (ValueError, struct.error, IndexError) as exc:
        result.update(status='wip_skipped', reason=str(exc))
    return result


def inspect_function(coff, name, address, data, sections, local, reference):
    result = dict(name=name, address=address, status='checked', counts={}, rows=[])
    try:
        function = coff.function(name)
        # Pass the exact decorated symbol: match.py's fallback must never select
        # the first .text section for an unrecognized stdcall/fastcall name.
        code = obj_function_code(coff.path, function['name'])
        count, size = true_extent(data, sections, address - IMAGE_BASE)
        if count is None:
            raise ValueError('original extent is unknown')
        off = rva2off(sections, address - IMAGE_BASE)
        original = list(MD.disasm(data[off:off + size], address))
        compiled, escapes = compiled_body(list(MD.disasm(code, 0)), count)
        matched, total, ok, reasons = compare(original, compiled, count, size, escapes)
        result.update(instructions=count, bytes=size)
        if not ok:
            raise ValueError('normalization/extent gate: ' + '; '.join(
                reasons + [f'{total - matched} normalized mismatches']))
        result['counts'], result['rows'] = check_relocations(
            coff, function, address, original, compiled, local, reference)
    except (ValueError, struct.error, IndexError) as exc:
        result.update(status='skipped', reason=str(exc))
    return result


def print_result(path, result):
    prefix = f'{path} {result["name"]} 0x{result["address"]:08x}'
    if result.get('wip'):
        # Never the word MISMATCH: the integration gate greps for it and a WIP
        # set difference is a lead, not a failed gate.
        if result['status'] == 'wip_skipped':
            print(f'WIPSKIPPED {prefix}: {result["reason"]}', flush=True)
        for row in result['rows']:
            if row['kind'] == 'MISSING':
                print(f'WIPRELOC {prefix} MISSING original=0x{row["original"]:08x} '
                      f'(the original reads it at {row["reason"]})', flush=True)
            else:
                print(f'WIPRELOC {prefix} EXTRA ours=0x{row["ours"]:08x} '
                      f'symbol={row["symbol"]}: {row["reason"]}', flush=True)
        return
    if result['status'] != 'checked':
        print(f'SKIPPED {prefix}: {result["reason"]}', flush=True)
    for row in result['rows']:
        original = '?' if row['original'] is None else f'0x{row["original"]:08x}'
        ours = '?' if row['ours'] is None else f'0x{row["ours"]:08x}'
        addend = '?' if row['addend'] is None else f'{row["addend"]:+#x}'
        print(f'{row["status"].upper()} {prefix} i={row["index"]} '
              f'original={original} ours={ours} symbol={row["symbol"]} '
              f'addend={addend}: {row["reason"]}', flush=True)


def annotation_self_test():
    sample = '''
extern int a, b; /* 0x00668000, 0x00668004 */
extern int c; /* 0x00668008 built by 0x00444000 */
extern void SixDigits(void); /* 0x47b330 */
__declspec(dllimport) long __stdcall WinFn(int a,
    void (*callback)(int)); /* [0x4ab290] */
extern CallbackReturn (*hook)(void); /* 0x00829a58 */
__declspec(dllimport) int UnknownImport(void); /* 0x00490000 */
extern struct FortArea { int x, y; } fort; /* 0x004b4580 */
/* A directly described global (@ 0x00667ca4). */
extern int tile;
/* Several nearby globals: 0x00600000 and 0x00600004. */
extern int uncertain;
extern int no_address;
/* An unrelated function LLSAuto at 0x0047d630.
 * Do not attach its description to the string below. */

static const char kBadSprite[] = "message";
// FUNCTION: LEGOLAND 0x00412340
void __stdcall Body(int x) {
  extern int body_only; /* 0x00666000 */
  suspicious(0x00444555);
}
extern int conflict; /* 0x00666000 */
extern int conflict; /* 0x00666004 */
extern void Previous(void);
    /* 0x0045f220 */
extern void Next(void);
    /* 0x0048a2e0 */
extern void NoAnnotation(void);
'''
    sample += '#define EXAMPLE(x) \\\n+{ x; }\n'
    sample += '__declspec(naked)\n// FUNCTION: LEGOLAND 0x00412360\nvoid Naked(void) {}\n'
    got = _text_addresses(sample)
    expected = {'a': {0x668000}, 'b': {0x668004}, 'c': {0x668008},
                'SixDigits': {0x47b330}, '__imp__WinFn': {0x4ab290}, 'hook': {0x829a58},
                'fort': {0x4b4580}, 'tile': {0x667ca4}, 'Body': {0x412340},
                'conflict': {0x666000, 0x666004}, 'Naked': {0x412360},
                'Previous': {0x45f220}, 'Next': {0x48a2e0}}
    assert got == expected, (got, expected)
    assert _text_addresses(sample, True) == {'Body': {0x412340}, 'Naked': {0x412360}}
    print('source annotation checks passed')


def self_test():
    """Small synthetic COFF, with independently linked operand expectations."""
    # Includes a nonzero function offset, an auxiliary symbol record, long
    # symbol names, stdcall decoration, negative addends, and two relocations
    # in ONE instruction. No game binary or compiler is needed for this test.
    raw = bytes.fromhex('a1 08000000 a3 fcffffff e8 04000000 '
                        'c705 00000000 0c000000 68 00000000 ff15 00000000 c3')
    relocs = [(1, 2, DIR32), (6, 2, DIR32), (11, 3, REL32),
              (17, 2, DIR32), (21, 4, DIR32), (26, 5, DIR32), (32, 6, DIR32)]
    symbols = [('_Test@0', 2, 1, 0x20, 1), ('_g_base', 0, 0, 0, 0),
               ('_Callee', 0, 0, 0x20, 0), ('_g_other', 0, 0, 0, 0),
               ('$SG1', 0, 0, 0, 0), ('__imp__Api@0', 0, 0, 0, 0)]
    strings = bytearray(b'\0' * 4)
    table = bytearray()
    for name, value, section, typ, aux in symbols:
        encoded = name.encode()
        if len(encoded) > 8:
            field = struct.pack('<II', 0, len(strings))
            strings.extend(encoded + b'\0')
        else:
            field = encoded.ljust(8, b'\0')
        table.extend(field + struct.pack('<IhHBB', value, section, typ, 2, aux))
        table.extend(b'\0' * 18 * aux)
    struct.pack_into('<I', strings, 0, len(strings))
    code = b'\x90\x90' + raw
    reloc_bytes = b''.join(struct.pack('<IIH', pos + 2, sym, typ)
                           for pos, sym, typ in relocs)
    symptr = 60 + len(code) + len(reloc_bytes)
    header = struct.pack('<HHIIIHH', 0x14c, 1, 0, symptr, len(table) // 18, 0, 0)
    section = b'.text\0\0\0' + struct.pack('<IIIIIIHHI', 0, 0, len(code), 60,
                                          60 + len(code), 0, len(relocs), 0, 0x60000020)
    address = 0x401000
    linked = bytearray(raw)
    for offset, value in ((1, 0x500008), (6, 0x4ffffc),
                          (11, 0x401104 - (address + 15)), (17, 0x500000),
                          (21, 0x60000c), (26, 0x4b0000), (32, 0x4ab000)):
        struct.pack_into('<I', linked, offset, value)
    local = dict(g_base={0x500000}, Callee={0x401100}, g_other={0x600000})
    local['__imp__Api'] = {0x4ab000}
    with tempfile.TemporaryDirectory(prefix='sl_test_', dir='/tmp') as temp:
        path = Path(temp) / 'test.obj'
        path.write_bytes(header + section + code + reloc_bytes + table + strings)
        coff = Coff(path)
        function = coff.function('Test')
        assert function['name'] == '_Test@0' and function['value'] == 2
        try:
            coff.function('Missing')
        except ValueError:
            pass
        else:
            raise AssertionError('missing function selected another .text')
        original = list(MD.disasm(linked, address))
        compiled = list(MD.disasm(obj_function_code(path, function['name']), 0))
        assert compare(original, compiled, len(original), len(linked), False)[2]
        counts, rows = check_relocations(coff, function, address, original, compiled, local, {})
        assert counts == dict(relocations=7, matched=6, unresolved=1), (counts, rows)
        assert rows[0]['symbol'] == '$SG1' and rows[0]['reason'] == 'string literal'
        wrong = dict(local, g_base={0x500004}, Callee={0x401200})
        counts, rows = check_relocations(coff, function, address, original, compiled, wrong, {})
        assert counts == dict(relocations=7, matched=2, mismatches=4, unresolved=1), counts
        assert [(r['index'], r['original'], r['ours']) for r in rows
                if r['status'] == 'mismatch'] == [
                    (0, 0x500008, 0x50000c), (1, 0x4ffffc, 0x500000),
                    (2, 0x401104, 0x401204), (3, 0x500000, 0x500004)]
        conflict = dict(local, g_base={0x500000, 0x500004})
        counts, rows = check_relocations(coff, function, address, original, compiled, conflict, {})
        assert counts.get('mismatches', 0) == 0 and counts['unresolved'] == 4
        static = dict(name='_PathSprite', storage=3, section=2, value=0)
        assert symbol_address(static, {}, {'PathSprite': {0x832bf0}},
                              function, address, len(raw), 0)[0] is None
        assert symbol_address(static, {'PathSprite': {0x401800}}, {},
                              function, address, len(raw), 0)[0] == 0x401800
        assert c_name('__imp__Api@0') == '__imp__Api' and c_name('__open') == '_open'
        # DIR32NB contains an RVA, whereas an import slot remains an address.
        push = next(MD.disasm(bytes.fromhex('68 00100000'), address))
        assert original_target(push, address + 1, DIR32NB) == 0x401000
        try:
            original_target(push, address + 1, 0xb)
        except ValueError:
            pass
        else:
            raise AssertionError('unsupported relocation accepted')
        wip_self_test(coff, function, address, linked, original, compiled, local)
    annotation_self_test()
    print('PASS: COFF identity/addend/operand/decoration/unresolved regression checks')


def wip_self_test(coff, function, address, linked, original, compiled, local):
    """The WIP set comparison, on the same synthetic body (no binary needed)."""
    span = (0x401000, 0x600010)   # Covers every operand this body names.
    theirs = original_references(original, address, len(linked), span)
    # Absolute displacements, a 32-bit immediate, an indirect call's import slot
    # and the direct call's external target; the `0xc` immediate is below the
    # image and the `ret` names nothing.
    assert set(theirs) == {0x500008, 0x4ffffc, 0x401104, 0x500000, 0x60000c,
                           0x4b0000, 0x4ab000}, sorted(theirs)
    # A branch that stays inside the body is a label, not a reference.
    inner = list(MD.disasm(bytes.fromhex('eb 00 c3'), address))
    assert original_references(inner, address, 3, span) == {}
    # A bitwise immediate is a mask, and the image base is not an address.
    mask = list(MD.disasm(bytes.fromhex('81 4c 24 48 00004000'), address))
    assert original_references(mask, address, 8, span) == {}
    # A switch table just past the body, reached through an index register.
    table = list(MD.disasm(bytes.fromhex('ff 24 85 30104000'), address))
    assert original_references(table, address, 8, span)[0x401030][1] == 'table'
    ours, unresolved, literals, tables = our_references(
        coff, function, address, compiled, local, {})
    assert unresolved == 1 and set(ours) == set(theirs) - {0x4b0000}, sorted(ours)
    assert literals == [] and tables == 0

    wrong = dict(local, g_base={0x500004})
    ours, _, _, _ = our_references(coff, function, address, compiled, wrong, {})
    assert set(theirs) - set(ours) == {0x500008, 0x4ffffc, 0x4b0000}
    assert set(ours) - set(theirs) == {0x500004, 0x50000c}
    # 0x500000 is in both sets by coincidence (g_base-4 == the wrong g_base):
    # the set test is strictly weaker than the positional one, which reports
    # all four of this object's wrong positions.
    assert 0x500000 in set(ours) & set(theirs)

    # A reference that resolves inside the original's own extent is our code --
    # a recursive call -- and is not a reference. Without this a self-call reads
    # back as EXTRA (the original's is a label we dropped on its side).
    inside, _, _, _ = our_references(coff, function, address, compiled, local, {},
                                    0x200)
    assert 0x401104 not in inside and 0x401104 in ours

    # A literal is accounted for by CONTENT, at whatever address the linker
    # happened to give the original's copy; a switch table by class and count.
    fake = bytes(0x200) + b'hello\0'
    parts = [(0x1000, 0x100, 0x200, 0x100)]
    theirs = {0x401000: ('push 0x401000', 'data'),
              0x401040: ('jmp dword ptr [eax*4 + 0x401040]', 'table')}
    assert account_for([0x401000, 0x401040], theirs, [b'hello\0'], 1,
                       fake, parts) == {0x401000, 0x401040}
    assert account_for([0x401000, 0x401040], theirs, [b'world\0'], 0,
                       fake, parts) == set()
    assert literal_bytes(coff, dict(name='$SG1', section=0, value=0)) is None

    result = dict(name='Test', address=address, status='wip_checked', wip=True,
                  counts={}, rows=[
                      dict(status='wipreloc', kind='MISSING', original=0x813a48,
                           ours=None, symbol=None, reason='0x004706dd: mov eax, x'),
                      dict(status='wipreloc', kind='EXTRA', original=None,
                           ours=0x813a4c, symbol='_g_cursor',
                           reason='named by our body only')])
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        print_result('LEGOLAND/test.c', result)
    printed = out.getvalue()
    # The integration gate greps for MISMATCH. A WIP lead must never trip it.
    assert 'MISMATCH' not in printed and printed.count('WIPRELOC') == 2, printed
    assert '0x00813a48' in printed and '0x00813a4c' in printed


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('src', nargs='?')
    ap.add_argument('func', nargs='?')
    ap.add_argument('addr', nargs='?')
    ap.add_argument('--all', action='store_true', help='sweep every marker')
    ap.add_argument('--json', metavar='PATH', help='also save full results as JSON')
    ap.add_argument('--no-wip', action='store_true',
                    help='skip the WIP set comparison (exact bodies only)')
    ap.add_argument('--self-test', action='store_true', help='run synthetic regression checks')
    args = ap.parse_args()
    if args.self_test:
        self_test()
        return 0
    if args.all and any((args.src, args.func, args.addr)):
        ap.error('--all cannot be combined with a single function')
    if not args.all and not args.src:
        ap.error('supply SRC [FUNC VA], or --all')
    if bool(args.func) != bool(args.addr):
        ap.error('FUNC and VA go together')
    data, sections = load_exe()
    root = Path(ROOT)
    reference = reference_addresses(root)
    def markers(path):
        """(name, VA, is_wip) for every bound marker we are asked to check."""
        return [(name, int(va, 16), bool(wip)) for name, va, wip in annotated(path)
                if not (wip and args.no_wip)]

    if args.all:
        sources = {p: markers(p) for p in sorted((root / 'LEGOLAND').glob('*.c'))}
    elif args.func:
        address = int(args.addr, 16)
        if address < IMAGE_BASE:
            address += IMAGE_BASE
        src = Path(args.src).resolve()
        # The marker decides which check applies, so that naming one function
        # behaves exactly as the sweep does on it.
        wip = any(name == args.func and flag for name, _, flag in markers(src))
        sources = {src: [(args.func, address, wip)]}
    else:
        # One source file: every marker in it (the integration gate's per-file
        # form; compiles the file once).
        src = Path(args.src).resolve()
        sources = {src: markers(src)}
    env = dict(os.environ, ALPHATEAM_VC6_ROOT=str(root / 'toolchain'))
    results, totals = [], Counter()
    # Unique per process AND per invocation, safe alongside audit/matching jobs.
    with tempfile.TemporaryDirectory(prefix=f'sl_{os.getpid()}_', dir='/tmp') as temp:
        obj = str(Path(temp) / 'functions.obj')
        for source, functions in sources.items():
            if not functions:
                continue
            label = os.path.relpath(source, root)
            totals['files_attempted'] += 1
            totals['functions_attempted'] += len(functions)
            print(f'Checking {label} ({len(functions)} functions)', file=sys.stderr, flush=True)
            Path(obj).unlink(missing_ok=True)
            try:
                proc = subprocess.run([CL_WRAPPER, '/nologo', '/c', '/Fo' + obj,
                                       '/O2', '/Gy', '/Gd', str(source)],
                                      capture_output=True, text=True, env=env, cwd=ROOT)
                if proc.returncode:
                    raise ValueError('compile failed: ' + (proc.stdout + proc.stderr).strip())
                coff = Coff(obj)
                local = source_addresses(source)
            except (OSError, ValueError, struct.error) as exc:
                totals['files_failed'] += 1
                current = [dict(name=n, address=a, wip=w, counts={}, rows=[],
                                status='wip_skipped' if w else 'skipped',
                                reason=str(exc)) for n, a, w in functions]
            else:
                current = [(inspect_wip_function if w else inspect_function)(
                    coff, n, a, data, sections, local, reference)
                    for n, a, w in functions]
            for result in current:
                result['file'] = label
                results.append(result)
                totals['functions_' + result['status']] += 1
                totals.update(result['counts'])
                if result['counts'].get('mismatches'):
                    totals['functions_with_hits'] += 1
                if result['counts'].get('unresolved'):
                    totals['functions_with_unresolved'] += 1
                if (result['counts'].get('wip_references_missing')
                        or result['counts'].get('wip_references_extra')):
                    totals['wip_functions_with_differences'] += 1
                print_result(label, result)
    for key in ('files_failed', 'functions_checked', 'functions_skipped',
                'functions_with_hits', 'functions_with_unresolved', 'relocations',
                'matched', 'mismatches', 'unresolved', 'functions_wip_checked',
                'functions_wip_skipped', 'wip_functions_with_differences',
                'wip_references_missing', 'wip_references_extra'):
        totals.setdefault(key, 0)
    print('SUMMARY ' + json.dumps(dict(totals), sort_keys=True))
    if args.json:
        Path(args.json).write_text(json.dumps(dict(summary=dict(totals), functions=results),
                                              indent=2) + '\n')
    # WIP counts are deliberately absent from both bits: the set comparison is
    # a lead for a matching lane, and a round must not start failing its gate
    # because a WIP body it never touched names one address more than the
    # original does.
    return int(bool(totals['mismatches'])) | (2 * int(bool(
        totals['unresolved'] or totals['functions_skipped'])))


if __name__ == '__main__':
    sys.exit(main())
