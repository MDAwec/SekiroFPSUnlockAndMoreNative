# Searches the writable memory of a running game for utf-16 texts; also prints the game language. Usage: python memsearch.py <pid> <text>...
import ctypes,ctypes.wintypes as w,sys,struct
k=ctypes.WinDLL('kernel32')
h=k.OpenProcess(0x0410,False,int(sys.argv[1]))
def rd(a,n):
    b=ctypes.create_string_buffer(n); g=ctypes.c_size_t()
    return b.raw[:g.value] if k.ReadProcessMemory(h,ctypes.c_void_p(a),b,n,ctypes.byref(g)) else b''
print('lang var', struct.unpack('<i',rd(0x143D9C868,4))[0])
class MBI(ctypes.Structure):
    _fields_=[('BaseAddress',ctypes.c_void_p),('AllocationBase',ctypes.c_void_p),('AllocationProtect',w.DWORD),('PartitionId',w.WORD),('RegionSize',ctypes.c_size_t),('State',w.DWORD),('Protect',w.DWORD),('Type',w.DWORD)]
pats=[p.encode('utf-16-le') for p in sys.argv[2:]]
found={}
a=0;m=MBI()
while a<0x7fffffffffff and k.VirtualQueryEx(h,ctypes.c_void_p(a),ctypes.byref(m),ctypes.sizeof(m)):
    if m.State==0x1000 and m.Protect in (0x04,0x02) and m.RegionSize<0x40000000:
        data=rd(m.BaseAddress,m.RegionSize)
        for p in pats:
            i=data.find(p)
            while i!=-1 and found.get(p,0)<4:
                s=data[max(0,i-60):i+120].decode('utf-16-le','replace').split('\x00')
                print(p.decode('utf-16-le'),'|',[x for x in s if x][:3]); found[p]=found.get(p,0)+1
                i=data.find(p,i+2)
    a=(m.BaseAddress or 0)+m.RegionSize
