"""Shared F/G/H validation helpers; never modify the retail image or match tools."""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import audit
import match
import relocs


def targets():
    result = []
    for scope, suffix in (("F", "rides"), ("G", "coaster"), ("H", "flume_jungle")):
        brief = (ROOT / "docs" / f"SCOPE_{scope}_partials_{suffix}.md").read_text()
        for file, address, name in re.findall(
            r"^\|.*?`([\w]+\.c)`.*?(0x[0-9a-fA-F]{8}).*?`([\w]+)`", brief, re.M
        ):
            result.append(dict(scope=scope, file=file, address=int(address, 16), name=name))
    return result


def compile_source(source, obj):
    env = dict(os.environ, ALPHATEAM_VC6_ROOT=str(ROOT / "toolchain"))
    proc = subprocess.run(
        [match.CL_WRAPPER, "/nologo", "/c", "/W3", "/O2", "/Gy", "/Gd",
         "/I" + str(ROOT / "LEGOLAND"), "/Fo" + str(obj), str(source)],
        cwd=ROOT, env=env, capture_output=True, text=True,
    )
    if proc.returncode or "warning" in (proc.stdout + proc.stderr).lower():
        raise RuntimeError(proc.stdout + proc.stderr)


def bodies(obj, name, address, data, sections):
    count, size = match.true_extent(data, sections, address - match.IMAGE_BASE)
    offset = match.rva2off(sections, address - match.IMAGE_BASE)
    original = list(match.md.disasm(data[offset:offset + size], address))
    compiled, escapes = match.compiled_body(
        list(match.md.disasm(match.obj_function_code(str(obj), name), 0)), count
    )
    return original, compiled, escapes


def branch_issues(coff, name, original, compiled, data, sections):
    """Check widths and branch destinations that address normalization hides.

    Only call after the normalized sequence and total byte size agree.
    Jump tables use the original independently bounded entry count and actual
    compiled COFF relocations, including local-label addends.
    """
    function = coff.function(name)
    section = coff.sections[function["section"] - 1]
    oi = {x.address: i for i, x in enumerate(original)}
    ci = {x.address: i for i, x in enumerate(compiled)}
    problems = []
    for index, (x, y) in enumerate(zip(original, compiled)):
        if x.size != y.size:
            problems.append(dict(index=index, kind="instruction_width"))
        if not x.mnemonic.startswith("j"):
            continue
        target = match._branch_target(x)
        if target in oi:
            if oi[target] != ci.get(match._branch_target(y)):
                problems.append(dict(index=index, kind="branch_destination"))
        elif x.mnemonic == "jmp" and "*4" in x.op_str:
            original_targets = match._table_targets(
                data, sections, x.op_str, original[0].address,
                original[-1].address + original[-1].size,
            )
            candidates = [r for r in section["relocs"]
                          if function["value"] + y.address <= r[0]
                          < function["value"] + y.address + y.size]
            try:
                if len(candidates) != 1 or not original_targets:
                    raise ValueError("ambiguous table")
                offset, symbol_index, _ = candidates[0]
                symbol = coff.symbols[symbol_index]
                table = coff.sections[symbol["section"] - 1]
                start = symbol["value"] + struct.unpack_from("<I", section["code"], offset)[0]
                relocations = {r[0]: r for r in table["relocs"]}
                compiled_targets = []
                for n in range(len(original_targets)):
                    at = start + n * 4
                    _, symbol_index, _ = relocations[at]
                    symbol = coff.symbols[symbol_index]
                    if symbol["section"] != function["section"]:
                        raise ValueError("table target outside function section")
                    value = symbol["value"] + struct.unpack_from("<I", table["code"], at)[0]
                    compiled_targets.append(value - function["value"])
                if [oi.get(t) for t in original_targets] != [ci.get(t) for t in compiled_targets]:
                    raise ValueError("different table targets")
            except (KeyError, ValueError, IndexError, struct.error) as exc:
                problems.append(dict(index=index, kind="jump_table", reason=str(exc)))
    return problems


def literal_checks(coff, original, identities, data, sections):
    """Compare floating and string literal contents hidden by address normalization.

    These prove literal values, not their linker placement. Unsupported symbols
    stay unresolved in the ordinary relocation report.
    """
    checked, problems = 0, []
    for row in identities.get("rows", []):
        if row.get("status") != "unresolved" or row.get("reason") not in (
                "floating-point literal", "string literal"):
            continue
        index = row["index"]
        try:
            symbols = [s for s in coff.symbols.values() if s["name"] == row["symbol"]]
            if len(symbols) != 1 or symbols[0]["section"] <= 0:
                raise ValueError("literal is not uniquely defined in COFF")
            symbol = symbols[0]
            code = coff.sections[symbol["section"] - 1]["code"]
            offset = symbol["value"] + row["addend"]
            if row["reason"] == "string literal":
                if not 0 <= offset < len(code):
                    raise ValueError("literal outside COFF section")
                end = code.find(b"\x00", offset)
                if end < 0:
                    raise ValueError("unterminated string literal")
                size = end - offset + 1
            else:
                ins = next(relocs.MD.disasm(bytes(original[index].bytes), original[index].address))
                operands = [o for o in ins.operands if o.type == 3
                            and o.mem.disp == row["original"]]
                if len(operands) != 1 or not operands[0].size:
                    raise ValueError("floating operand width is ambiguous")
                size = operands[0].size
            expected_offset = match.rva2off(sections, row["original"] - match.IMAGE_BASE)
            if (expected_offset is None or offset < 0 or offset + size > len(code)
                    or expected_offset + size > len(data)):
                raise ValueError("literal outside image")
            if code[offset:offset + size] != data[expected_offset:expected_offset + size]:
                raise ValueError("literal contents differ")
            checked += 1
        except (ValueError, KeyError, IndexError, StopIteration) as exc:
            problems.append(dict(index=index, symbol=row["symbol"], reason=str(exc)))
    return dict(checked=checked, issues=problems)
