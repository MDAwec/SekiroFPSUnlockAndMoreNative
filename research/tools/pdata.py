# Finds the function (from .pdata) containing each rva. Usage: python pdata.py <rva hex>...   (env SEKIRO_DUMP)
import os
import struct,sys,bisect
img=open(os.environ.get('SEKIRO_DUMP', 'sekiro_dump.exe'),'rb').read()
e=struct.unpack_from('<I',img,0x3c)[0]
rva,size=struct.unpack_from('<II',img,e+0x18+0x70+3*8)
ents=[struct.unpack_from('<III',img,rva+i*12) for i in range(size//12)]
for a in sys.argv[1:]:
    t=int(a,16)
    for s,en,u in ents:
        if s<=t<en: print(a,'-> func',hex(s),hex(en),'unwind',hex(u)); break
    else: print(a,'not found')
