# Disassembles the dump with capstone. Usage: python disasm.py <rva hex> <length>   (env SEKIRO_DUMP = dump path)
import os,pefile,sys,capstone
pe=pefile.PE(os.environ.get("SEKIRO_DUMP", "sekiro_dump.exe"),fast_load=True)
img=pe.get_memory_mapped_image()
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
start=int(sys.argv[1],16); n=int(sys.argv[2],0)
for i in md.disasm(img[start:start+n],start):
    print(f"{i.address:8X}: {i.bytes.hex():24} {i.mnemonic} {i.op_str}")
