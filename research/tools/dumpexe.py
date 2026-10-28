# Dumps the decrypted sekiro.exe from a running game (the file on disk is SteamStub encrypted).
# Usage: python dumpexe.py <pid> sekiro_dump.exe   -> a PE with sections laid out as in memory, base 0x140000000
import ctypes, ctypes.wintypes as w, struct, sys, subprocess
pid=int(sys.argv[1]); out=sys.argv[2]
k=ctypes.WinDLL('kernel32',use_last_error=True); psapi=ctypes.WinDLL('psapi')
h=k.OpenProcess(0x0410,False,pid)
mods=(ctypes.c_void_p*1024)(); need=w.DWORD()
psapi.EnumProcessModulesEx(h,mods,ctypes.sizeof(mods),ctypes.byref(need),3)
base=mods[0]
def rd(a,n):
    buf=ctypes.create_string_buffer(n); got=ctypes.c_size_t()
    if not k.ReadProcessMemory(h,ctypes.c_void_p(a),buf,n,ctypes.byref(got)):
        # page-wise
        out=bytearray(n)
        for off in range(0,n,0x1000):
            b=ctypes.create_string_buffer(0x1000)
            if k.ReadProcessMemory(h,ctypes.c_void_p(a+off),b,0x1000,ctypes.byref(got)): out[off:off+0x1000]=b.raw[:min(0x1000,n-off)]
        return bytes(out)
    return buf.raw
hdr=rd(base,0x1000)
e=struct.unpack_from('<I',hdr,0x3c)[0]
size=struct.unpack_from('<I',hdr,e+0x50)[0]
img=bytearray(rd(base,size))
nsec=struct.unpack_from('<H',img,e+6)[0]; opt=struct.unpack_from('<H',img,e+0x14)[0]
st=e+0x18+opt
for i in range(nsec):
    o=st+i*40
    vs,va=struct.unpack_from('<II',img,o+8)
    struct.pack_into('<II',img,o+16,vs,va)  # SizeOfRawData, PointerToRawData = virtual layout
struct.pack_into('<Q',img,e+0x18+0x18,base)  # ImageBase = runtime base
open(out,'wb').write(img); print(hex(base),hex(size))
