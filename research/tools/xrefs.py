# Direct call/jmp xrefs to functions (rel32), cached in calls.pkl. Usage: python xrefs.py <rva hex>...   (env SEKIRO_DUMP)
import struct,sys,pickle,os
img=open(os.environ.get('SEKIRO_DUMP', 'sekiro_dump.exe'),'rb').read()
cache='calls.pkl'
if os.path.exists(cache): calls=pickle.load(open(cache,'rb'))
else:
    import numpy as np
    a=np.frombuffer(img,dtype=np.uint8)
    idx=np.nonzero((a[0x1000:0x291d000]==0xE8)|(a[0x1000:0x291d000]==0xE9))[0]+0x1000
    rel=np.frombuffer(img,dtype=np.uint8)
    r=(a[idx+1].astype(np.int64)|(a[idx+2].astype(np.int64)<<8)|(a[idx+3].astype(np.int64)<<16)|(a[idx+4].astype(np.int64)<<24))
    r=np.where(r>=2**31,r-2**32,r)
    tgt=idx+5+r
    calls=(idx,tgt)
    pickle.dump(calls,open(cache,'wb'))
idx,tgt=calls
for a in sys.argv[1:]:
    t=int(a,16)
    src=idx[tgt==t]
    print(a, len(src), ' '.join(hex(x) for x in src[:40]))
