# Breadth-first search for pointers to targets from an object in a running game. Usage: python findptr.py <pid> <start hex> <target hex>...
import ctypes,sys,struct
k=ctypes.WinDLL('kernel32')
h=k.OpenProcess(0x0410,False,int(sys.argv[1]))
def rd(a,n):
    b=ctypes.create_string_buffer(n); k.ReadProcessMemory(h,ctypes.c_void_p(a),b,n,None); return b.raw
start=int(sys.argv[2],16); targets=[int(x,16) for x in sys.argv[3:]]
# BFS up to depth 3 from start, offsets 0..0x400
from collections import deque
q=deque([(start,[])]); seen=set()
while q:
    a,path=q.popleft()
    if a in seen or len(path)>3: continue
    seen.add(a)
    data=rd(a,0x400)
    for off in range(0,0x400,8):
        v=struct.unpack_from('<Q',data,off)[0]
        for t in targets:
            if v==t or (t<=v<t+0x100):
                print('found', '->'.join(path+[hex(off)]), hex(v), 'target', hex(t))
        if 0x7ff400000000<v<0x7fff00000000 and len(path)<3:
            q.append((v,path+[hex(off)]))
