# Lists imports of a PE (here: the FMOD functions sekiro.exe uses). Usage: python imports.py <exe> [all]
import pefile,sys
pe=pefile.PE(sys.argv[1],fast_load=True)
pe.parse_data_directories([pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT']])
for e in pe.DIRECTORY_ENTRY_IMPORT:
    n=e.dll.decode()
    if 'fmod' in n.lower() or len(sys.argv)>2:
        for i in e.imports: print(n, hex(i.address-pe.OPTIONAL_HEADER.ImageBase), i.name)
