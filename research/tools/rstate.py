# Render system sizes of a running game (window target, picture, render resolution, aspect ratio pairs). Usage: python rstate.py <pid>
import ctypes,sys,struct
k=ctypes.WinDLL('kernel32'); u=ctypes.WinDLL('user32')
h=k.OpenProcess(0x0410,False,int(sys.argv[1]))
def rd(a,n):
    b=ctypes.create_string_buffer(n); k.ReadProcessMemory(h,ctypes.c_void_p(a),b,n,None); return b.raw
q=lambda a: struct.unpack('<Q',rd(a,8))[0]
sys_=q(0x143F3B400)
print('sys+0x200 target', struct.unpack('<II',rd(sys_+0x200,8)))
o=q(sys_+160); print('output+272 picture', struct.unpack('<II',rd(o+272,8)))
print('render_resolution', struct.unpack('<II',rd(0x143B1D100,8)))
for i in range(4):
    print(' pair',i, struct.unpack('<II',rd(sys_+0x218+40*i,8)))
