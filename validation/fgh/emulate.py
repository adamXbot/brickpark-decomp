"""Load original PE code and relocated, independently compiled COFF code in Unicorn.

Only explicitly resolved externals are accepted. This is a function-test
environment, not a Windows emulator or a replacement for the exactness gate.
"""
import struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP

from common import ROOT, relocs

PAGE = 4096
COMPILED_BASE = 0x02000000
STACK_BASE = 0x03000000
STACK_SIZE = 0x10000
HEAP_BASE = 0x04000000
HEAP_SIZE = 0x10000
STOP = 0x05000000


def page_size(size):
    return (size + PAGE - 1) & -PAGE


class CoffImage:
    def __init__(self, coff, function, externals):
        self.coff = coff
        self.externals = externals
        self.addresses = {}
        self.images = {}
        self.next_address = COMPILED_BASE
        symbol = coff.function(function)
        self.entry = self.load_section(symbol["section"]) + symbol["value"]

    def resolve(self, symbol):
        if symbol["section"] > 0:
            return self.load_section(symbol["section"]) + symbol["value"]
        if symbol["section"] == -1:
            return symbol["value"]
        name = relocs.c_name(symbol["name"])
        values = self.externals.get(name, set())
        if len(values) != 1:
            raise ValueError(f"Unresolved or ambiguous external: {symbol['name']}: {values}")
        return next(iter(values))

    def load_section(self, index):
        if index in self.addresses:
            return self.addresses[index]
        section = self.coff.sections[index - 1]
        code = bytearray(section["code"])
        if not code:
            raise ValueError("Referenced empty/BSS COFF section is not supported")
        address = self.next_address
        self.next_address += page_size(len(code))
        self.addresses[index] = address
        for offset, symbol_index, kind in section["relocs"]:
            symbol = self.coff.symbols[symbol_index]
            target = self.resolve(symbol)
            addend = struct.unpack_from("<I", code, offset)[0]
            if kind == relocs.DIR32:
                value = target + addend
            elif kind == relocs.REL32:
                value = target + addend - (address + offset + 4)
            else:
                raise ValueError(f"Unsupported execution relocation type {kind}")
            struct.pack_into("<I", code, offset, value & 0xffffffff)
        self.images[index] = bytes(code)
        return address


class Machine:
    def __init__(self, image=None):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        pe = pefile.PE(str(ROOT / "original/legoland.exe"))
        self.image_base = pe.OPTIONAL_HEADER.ImageBase
        self.uc.mem_map(self.image_base, page_size(pe.OPTIONAL_HEADER.SizeOfImage))
        self.uc.mem_write(self.image_base, pe.get_memory_mapped_image())
        pe.close()
        if image:
            for index, code in image.images.items():
                address = image.addresses[index]
                self.uc.mem_map(address, page_size(len(code)))
                self.uc.mem_write(address, code)
        self.uc.mem_map(STACK_BASE, STACK_SIZE)
        self.uc.mem_map(HEAP_BASE, HEAP_SIZE)
        self.uc.mem_map(STOP, PAGE)
        self.initial_context = self.uc.context_save()

    def reset(self, arguments):
        self.uc.context_restore(self.initial_context)
        self.uc.mem_write(STACK_BASE, b"\xa5" * STACK_SIZE)
        self.uc.mem_write(HEAP_BASE, b"\x00" * HEAP_SIZE)
        stack = STACK_BASE + STACK_SIZE - 0x100
        words = [STOP, *arguments]
        self.uc.mem_write(stack, struct.pack("<" + "I" * len(words), *words))
        self.uc.reg_write(UC_X86_REG_ESP, stack)
        return stack
