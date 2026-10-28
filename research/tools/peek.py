# Option data of a running game: GameDataMan (0x143D5AAC0) -> +0x50; bytes +4..+6 are the volumes. Usage: python peek.py <pid>
import ctypes,sys,struct
k=ctypes.WinDLL('kernel32')
h=k.OpenProcess(0x0410,False,int(sys.argv[1]))
def rd(a,n):
    b=ctypes.create_string_buffer(n); k.ReadProcessMemory(h,ctypes.c_void_p(a),b,n,None); return b.raw
gdm=struct.unpack('<Q',rd(0x143D5AAC0,8))[0]
opt=struct.unpack('<Q',rd(gdm+0x50,8))[0]
print(hex(gdm),hex(opt), rd(opt,0x30).hex(' '))
