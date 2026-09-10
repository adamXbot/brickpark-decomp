"""Differential execution of SchoolCarBlockedAhead, including the original __ftol.

Compares return value, car memory, non-stack writes, helper calls, callee-saved
registers, stack balance and x87 control state. An independent arithmetic model
checks finite and exceptional headings under all four x87 rounding modes.
"""
import argparse
import hashlib
import json
import math
import random
import tempfile

from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

from common import *
from emulate import *

FUNCTION = "SchoolCarBlockedAhead"
ADDRESS = 0x00402490
HEAD = 0x004c10d4
FTOL = 0x00458930
FTOL_SCRATCH = 0x00667c3c
CAR_SIZE = 0xd0
REGISTERS = (UC_X86_REG_EBX, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI)


def i32(value):
    value &= 0xffffffff
    return value - 0x100000000 if value & 0x80000000 else value


def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def convert(value, mode):
    value = f32(value) * -65536.0
    if not math.isfinite(value):
        return -0x80000000
    result = (round, math.floor, math.ceil, math.trunc)[mode](value)
    return result if -0x80000000 <= result < 0x80000000 else -0x80000000


def model(case):
    hit, calls = 0, 0
    cars = case["cars"]
    current = cars[case["current"]]
    for index in case["chain"]:
        if index == case["current"]:
            continue
        other = cars[index]
        dx = i32(current[0] - other[0]) >> 8
        dy = i32(current[1] - other[1]) >> 8
        if i32(dx * dx + dy * dy) > 0x40000:
            continue
        calls += 2
        x = i32(current[0] - convert(current[2], case["rounding"]))
        y = i32(current[1] - convert(current[3], case["rounding"]))
        dx = i32(x - other[0]) >> 8
        dy = i32(y - other[1]) >> 8
        if i32(dx * dx + dy * dy) <= 0x10000:
            hit = HEAP_BASE + index * 0x100
    return hit, calls


class Runner:
    def __init__(self, entry, image=None):
        self.machine = Machine(image)
        self.entry = entry
        self.calls = 0
        self.writes = []
        self.machine.uc.hook_add(UC_HOOK_CODE, self.on_helper, begin=FTOL, end=FTOL)
        self.machine.uc.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def on_helper(self, uc, address, size, user_data):
        self.calls += 1

    def on_write(self, uc, access, address, size, value, user_data):
        if not STACK_BASE <= address < STACK_BASE + STACK_SIZE:
            self.writes.append((address, size, value & ((1 << (size * 8)) - 1)))

    def run(self, case):
        uc = self.machine.uc
        current = HEAP_BASE + case["current"] * 0x100
        stack = self.machine.reset([current])
        random_regs = random.Random(case["register_seed"])
        for reg in (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, *REGISTERS):
            uc.reg_write(reg, random_regs.getrandbits(32))
        saved = [uc.reg_read(reg) for reg in REGISTERS]
        control = 0x037f | (case["rounding"] << 10)
        uc.reg_write(UC_X86_REG_FPCW, control)
        uc.reg_write(UC_X86_REG_FPSW, 0)
        uc.reg_write(UC_X86_REG_FPTAG, 0xffff)
        uc.mem_write(FTOL_SCRATCH, b"\x5a" * 4)
        head = HEAP_BASE + case["chain"][0] * 0x100 if case["chain"] else 0
        uc.mem_write(HEAD, struct.pack("<I", head))
        next_ids = dict(zip(case["chain"], case["chain"][1:] + [None]))
        for index, (x, y, ux, uy) in enumerate(case["cars"]):
            record = bytearray(random_regs.randbytes(CAR_SIZE))
            nxt = next_ids.get(index)
            struct.pack_into("<I", record, 0, HEAP_BASE + nxt * 0x100 if nxt is not None else 0)
            struct.pack_into("<ii", record, 0x10, x, y)
            struct.pack_into("<ff", record, 0xb0, ux, uy)
            uc.mem_write(HEAP_BASE + index * 0x100, bytes(record))
        before = bytes(uc.mem_read(HEAP_BASE, HEAP_SIZE))
        self.calls, self.writes = 0, []
        uc.emu_start(self.entry, STOP, count=20000)
        if uc.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError("Execution did not reach its return sentinel")
        if uc.reg_read(UC_X86_REG_ESP) != stack + 4:
            raise RuntimeError("Unbalanced stack")
        if [uc.reg_read(reg) for reg in REGISTERS] != saved:
            raise RuntimeError("Callee-saved register corruption")
        if uc.reg_read(UC_X86_REG_FPCW) != control:
            raise RuntimeError("Changed x87 rounding control")
        after = bytes(uc.mem_read(HEAP_BASE, HEAP_SIZE))
        if before != after:
            raise RuntimeError("Read-only car query changed car memory")
        if any(address != FTOL_SCRATCH or size != 4 for address, size, _ in self.writes):
            raise RuntimeError(f"Unexpected non-stack write: {self.writes}")
        return dict(result=uc.reg_read(UC_X86_REG_EAX), calls=self.calls,
                    writes=list(self.writes), car_memory=hashlib.sha256(after).hexdigest(),
                    fp_status=uc.reg_read(UC_X86_REG_FPSW), fp_tag=uc.reg_read(UC_X86_REG_FPTAG))


def cases(count):
    rng = random.Random(0xf6a2026)
    fixed = [
        ("empty", [(0, 0, 0., 0.)], []),
        ("self_only", [(0, 0, 0., 0.)], [0]),
        ("overlap", [(0, 0, 0., 0.), (0, 0, 0., 0.)], [0, 1]),
        ("last_hit", [(0, 0, 0., 0.), (0, 0, 0., 0.), (256, 0, 0., 0.)], [1, 0, 2]),
        ("current_not_linked", [(0, 0, 0., 0.), (0, 0, 0., 0.)], [1]),
    ]
    for shift in (-1, 0, 1):
        for direction in (-1, 1):
            for axis in (0, 1):
                for radius in (65536, 131072):
                    point = [0, 0, 0., 0.]
                    point[axis] = direction * (radius + shift)
                    fixed.append((f"boundary_{radius}_{direction}_{axis}_{shift}",
                                  [(0, 0, float(axis == 0), float(axis == 1)), tuple(point)], [0, 1]))
    for heading in (0., -0., .5 / 65536, -.5 / 65536, 1.5 / 65536,
                    -1.5 / 65536, 1., -1., 32768., float("inf"), float("nan")):
        fixed.append(("heading", [(0, 0, heading, heading), (65536, 0, 0., 0.)], [0, 1]))
    for name, cars, chain in fixed:
        for rounding in range(4):
            yield dict(name=name, cars=cars, chain=chain, current=0, rounding=rounding,
                       register_seed=rng.getrandbits(32))
    for index in range(count):
        n = rng.randrange(1, 13)
        current = rng.randrange(n)
        center = [rng.randrange(-0x10000000, 0x10000000) for _ in range(2)]
        if index % 10 == 0:
            center = [rng.choice((-0x80000000, 0x7fffffff, 0)), rng.choice((-1, 0, 1))]
        cars = []
        for j in range(n):
            x, y = [i32(center[k] + rng.randrange(-180000, 180001)) for k in range(2)]
            angle = rng.uniform(-math.pi, math.pi)
            cars.append((x, y, f32(math.cos(angle)), f32(math.sin(angle))))
        chain = list(range(n))
        rng.shuffle(chain)
        chain = chain[:rng.randrange(n + 1)]
        yield dict(name=f"random_{index}", cars=cars, chain=chain, current=current,
                   rounding=index % 4, register_seed=rng.getrandbits(32))


def linked(source, obj):
    compile_source(source, obj)
    externals = relocs.source_addresses(source)
    # Verified against the two original CALL operands and the helper body:
    # fistp [0x667c3c]; mov eax,[0x667c3c]; ret. No host-side conversion stub.
    externals["_ftol"] = {FTOL}
    return CoffImage(relocs.Coff(obj), FUNCTION, externals)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--random-cases", type=int, default=2000)
    parser.add_argument("--source", type=Path, default=ROOT / "LEGOLAND/schoolcar4.c",
                        help="candidate translation unit to compare with the original")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--self-test", action="store_true", help="require intentional errors to be detected")
    args = parser.parse_args()
    if args.random_cases < 0:
        parser.error("--random-cases must be nonnegative")
    source = args.source.resolve()
    all_cases = list(cases(args.random_cases))
    report = dict(function=FUNCTION, cases=len(all_cases), seed=hex(0xf6a2026),
                  source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                  original_sha256=hashlib.sha256((ROOT / "original/legoland.exe").read_bytes()).hexdigest(),
                  failures=[], controls=[])
    with tempfile.TemporaryDirectory(prefix="fgh_execution_") as tmp:
        tmp = Path(tmp)
        image = linked(source, tmp / "query.obj")
        original, compiled = Runner(ADDRESS), Runner(image.entry, image)
        observations = []
        for index, case in enumerate(all_cases):
            expected, expected_calls = model(case)
            retail = original.run(case)
            rebuilt = compiled.run(case)
            observations.append(retail)
            if retail != rebuilt or (retail["result"], retail["calls"]) != (expected, expected_calls):
                report["failures"].append(dict(index=index, case=case, original=retail, compiled=rebuilt,
                                                model_result=expected, model_calls=expected_calls))
                if len(report["failures"]) >= 10:
                    break
        if args.self_test and not report["failures"]:
            variants = {
                "first_hit_instead_of_last": ("hit = p;", "return p;"),
                "exclusive_inner_boundary": ("if (dy * dy + dx * dx <= 0x10000)",
                                             "if (dy * dy + dx * dx < 0x10000)"),
            }
            text = source.read_text()
            for name, (old, new) in variants.items():
                if text.count(old) != 1:
                    raise RuntimeError("Mutation site is not unique")
                candidate = tmp / (name + ".c")
                candidate.write_text(text.replace(old, new))
                mutation = linked(candidate, tmp / (name + ".obj"))
                runner = Runner(mutation.entry, mutation)
                found = next((i for i, case in enumerate(all_cases) if runner.run(case) != observations[i]), None)
                report["controls"].append(dict(name=name, detected=found is not None, case_index=found))
    report["passed"] = not report["failures"] and all(x["detected"] for x in report["controls"])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "failures"}, indent=2))
    if report["failures"]:
        print(json.dumps(report["failures"][0], indent=2))
    return int(not report["passed"])


if __name__ == "__main__":
    raise SystemExit(main())
