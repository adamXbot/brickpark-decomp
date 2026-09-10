"""Execute both Jungle Cruise seat switches against the original machine code.

This is a block-level regression test, not a whole rendering-function test.
The normalized instruction sequence must agree before block boundaries can be
mapped. Each block runs from its switch dispatch up to SetPersonRotation's
argument setup, without replacing any arithmetic or internal branch.
"""
import argparse
import hashlib
import itertools
import json
import tempfile

from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

from common import *
from emulate import *

FUNCTION = "JungleCruise_UpdateRiverAnim"
ADDRESS = 0x00432d00
BLOCKS = ((0x00432f83, 0x00433000, 0x30),
          (0x0043311d, 0x0043319a, 0x38))


def linked(source, obj):
    compile_source(source, obj)
    data, sections = match.load_exe()
    original, compiled, escapes = bodies(obj, FUNCTION, ADDRESS, data, sections)
    _, _, exact, _ = match.compare(original, compiled, len(original),
                                   sum(x.size for x in original), escapes)
    if not exact:
        raise ValueError("Block correspondence requires a normalized exact body")
    coff = relocs.Coff(obj)
    issues = branch_issues(coff, FUNCTION, original, compiled, data, sections)
    local = relocs.source_addresses(source)
    identities = relocs.inspect_function(coff, FUNCTION, ADDRESS, data, sections, local, {})
    literals = literal_checks(coff, original, identities, data, sections)
    image = CoffImage(coff, FUNCTION, local)
    mapping = {a.address: image.entry + b.address for a, b in zip(original, compiled)}
    return image, [(mapping[start], mapping[end], slot) for start, end, slot in BLOCKS], issues, literals


class Runner:
    def __init__(self, blocks, image=None):
        self.machine = Machine(image)
        self.blocks = blocks
        self.writes = []
        self.machine.uc.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def on_write(self, uc, access, address, size, value, user_data):
        self.writes.append((address, size, value & ((1 << (8 * size)) - 1)))

    def run(self, direction, heading, seat, tick, y):
        uc = self.machine.uc
        stack = self.machine.reset([])
        boat, person = HEAP_BASE, HEAP_BASE + 0x2000
        start, end, slot = self.blocks[direction]
        for reg, value in ((UC_X86_REG_EBX, seat), (UC_X86_REG_ECX, tick),
                           (UC_X86_REG_ESI, boat), (UC_X86_REG_EDI, person),
                           (UC_X86_REG_FPCW, 0x037f), (UC_X86_REG_FPSW, 0),
                           (UC_X86_REG_FPTAG, 0xffff)):
            uc.reg_write(reg, value)
        uc.mem_write(boat + 0x29c + tick * 4, struct.pack("<i", heading))
        uc.mem_write(stack + slot, struct.pack("<i", y))
        self.writes = []
        uc.emu_start(start, end, count=100)
        if uc.reg_read(UC_X86_REG_EIP) != end or uc.reg_read(UC_X86_REG_ESP) != stack:
            raise RuntimeError("Seat switch did not reach its expected boundary")
        permitted = {stack + 0x20, stack + slot, person + 0x44}
        if any(address not in permitted or size != 4 for address, size, _ in self.writes):
            raise RuntimeError(f"Unexpected seat-switch write: {self.writes}")
        return dict(y=struct.unpack("<i", uc.mem_read(stack + slot, 4))[0],
                    rotation_bits=bytes(uc.mem_read(person + 0x44, 4)).hex(),
                    writes=list(self.writes), status=uc.reg_read(UC_X86_REG_FPSW),
                    tag=uc.reg_read(UC_X86_REG_FPTAG))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source = ROOT / "LEGOLAND/junglecruise.c"
    cases = list(itertools.product(range(2), range(16), range(3), (0, 7, 63), (-100, 24, 100)))
    report = dict(function=FUNCTION, test_level="seat-switch blocks", cases=len(cases),
                  source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                  original_sha256=hashlib.sha256((ROOT / "original/legoland.exe").read_bytes()).hexdigest(),
                  failures=[])
    with tempfile.TemporaryDirectory(prefix="fgh_seats_") as tmp:
        tmp = Path(tmp)
        image, blocks, report["branch_issues"], report["literals"] = linked(source, tmp / "river.obj")
        original, compiled = Runner(BLOCKS), Runner(blocks, image)
        observations = []
        for case in cases:
            retail, rebuilt = original.run(*case), compiled.run(*case)
            observations.append(retail)
            expected_y = case[4] - (16 if case[2] in (1, 2) else 0)
            if retail != rebuilt or retail["y"] != expected_y:
                report["failures"].append(dict(case=case, original=retail, compiled=rebuilt))
        # Recreate the pre-integration source bug in both directions. This
        # compiles to the same normalized body, but must fail the execution test.
        mutant = source.read_text()
        for offset in ("soA", "soB"):
            pattern = r"(case 1:\s*\{[^{}]*\})\s*" + offset + r"\.y -= 0x10;"
            mutant, count = re.subn(pattern, r"\1", mutant)
            if count != 1:
                raise ValueError("Missing or ambiguous historical regression site")
        candidate = tmp / "historical_missing_seat1.c"
        candidate.write_text(mutant)
        image, blocks, issues, _ = linked(candidate, tmp / "historical.obj")
        runner = Runner(blocks, image)
        failures = [case for case, observation in zip(cases, observations)
                    if runner.run(*case) != observation]
        report["historical_control"] = dict(normalized_exact=True, failing_cases=len(failures),
                                             branch_issues=issues,
                                             directions=sorted({c[0] for c in failures}),
                                             seats=sorted({c[2] for c in failures}))
        report["passed"] = (not report["failures"] and len(failures) == len(cases) // 3
                             and {c[2] for c in failures} == {1} and not report["branch_issues"]
                             and not report["literals"]["issues"]
                             and issues == [dict(index=i, kind="branch_destination") for i in (216, 329)])
        # A wrong numeric constant also passes normalized instruction and
        # branch checks. Require literal-content and execution checks to catch it.
        text = source.read_text()
        if text.count("* 22.5f") != 6:
            raise ValueError("Unexpected number of seat angle constants")
        candidate = tmp / "wrong_angle_constant.c"
        candidate.write_text(text.replace("* 22.5f", "* 22.25f"))
        image, blocks, branches, literals = linked(candidate, tmp / "wrong_angle.obj")
        runner = Runner(blocks, image)
        detected = [case for case, observation in zip(cases, observations)
                    if runner.run(*case) != observation]
        report["literal_control"] = dict(normalized_exact=True, branch_issues=branches,
                                          literals=literals, failing_cases=len(detected))
        report["passed"] &= (not branches and bool(literals["issues"]) and bool(detected))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return int(not report["passed"])


if __name__ == "__main__":
    raise SystemExit(main())
