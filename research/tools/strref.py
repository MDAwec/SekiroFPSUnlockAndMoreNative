# Code referencing a string (ascii and utf-16, via rip-relative lea). Usage: python strref.py <text>...   (env SEKIRO_DUMP)
import os
import re,struct,sys
img=open(os.environ.get('SEKIRO_DUMP', 'sekiro_dump.exe'),'rb').read()
for w in sys.argv[1:]:
    tg=[]
    for enc in ('ascii','utf-16-le'):
        for m in re.finditer(re.escape((w+'\0').encode(enc)),img[0x2929000:0x3af9000]):
            tg.append((m.start()+0x2929000,enc))
    for t,enc in tg:
        refs=[]
        for m in re.finditer(rb'[\x48\x4c][\x8d][\x05\x0d\x15\x1d\x25\x2d\x35\x3d]',img[0x1000:0x291d000]):
            a=m.start()+0x1000
            if a+7+struct.unpack_from('<i',img,a+3)[0]==t: refs.append(hex(a))
        print(w,enc,hex(t),refs[:10])
