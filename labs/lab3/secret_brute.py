import pwn
import pwnlib.asm as pasm
import unicorn as uc
import unicorn.x86_const as uc86
from pwnlib.elf import ELF
from tqdm import tqdm

pwn.context.arch = "x86_64"

BASE = 0x400000

elf = ELF("./bomb17/bomb")
fun7_addr = elf.symbols["fun7"]
fun7_offset = int(elf.vaddr_to_offset(fun7_addr))  # type: ignore
secret_phase_addr = elf.symbols["secret_phase"]
secret_phase_offset = int(elf.vaddr_to_offset(secret_phase_addr))  # type: ignore
data_start_addr = elf.symbols["n1"]
data_start_offset = int(elf.vaddr_to_offset(data_start_addr))  # type: ignore
data_end_addr = elf.symbols["node1"]
data_end_offset = int(elf.vaddr_to_offset(data_end_addr))  # type: ignore

with open("./bomb17/bomb", "rb") as f:
    bomb_raw = f.read()
    fun7_code = bomb_raw[fun7_offset:secret_phase_offset]
    data = bomb_raw[data_start_offset:data_end_offset]

shellcode = pasm.asm("call 0x4011e1", vma=BASE)

emu = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_64)
emu.mem_map(0, 1 << 20, uc.UC_PROT_ALL)
emu.mem_map(BASE, 4 << 20, uc.UC_PROT_ALL)
emu.mem_write(fun7_addr, fun7_code)
emu.mem_write(data_start_addr, data)
emu.mem_write(BASE, shellcode)

for i in tqdm(range(1, 1001)):
    emu.reg_write(uc86.UC_X86_REG_RSP, (1 << 20) - 1)
    emu.reg_write(uc86.UC_X86_REG_RIP, BASE)
    emu.reg_write(uc86.UC_X86_REG_EDI, 0x604110)
    emu.reg_write(uc86.UC_X86_REG_ESI, i)
    emu.emu_start(BASE, BASE + len(shellcode))
    reg = emu.reg_read(uc86.UC_X86_REG_RAX)
    if reg == 2:
        print(f"Solution found: {i}")
