# Display modes and refresh rates of the primary display; "current" prints the mode now in use. Usage: python modes.py [current]
import ctypes, ctypes.wintypes as w, sys
class DEVMODEW(ctypes.Structure):
    _fields_=[('dmDeviceName',w.WCHAR*32),('dmSpecVersion',w.WORD),('dmDriverVersion',w.WORD),('dmSize',w.WORD),('dmDriverExtra',w.WORD),('dmFields',w.DWORD),
              ('dmPositionX',ctypes.c_long),('dmPositionY',ctypes.c_long),('dmDisplayOrientation',w.DWORD),('dmDisplayFixedOutput',w.DWORD),
              ('dmColor',ctypes.c_short),('dmDuplex',ctypes.c_short),('dmYResolution',ctypes.c_short),('dmTTOption',ctypes.c_short),('dmCollate',ctypes.c_short),
              ('dmFormName',w.WCHAR*32),('dmLogPixels',w.WORD),('dmBitsPerPel',w.DWORD),('dmPelsWidth',w.DWORD),('dmPelsHeight',w.DWORD),
              ('dmDisplayFlags',w.DWORD),('dmDisplayFrequency',w.DWORD)]
u=ctypes.WinDLL('user32'); dm=DEVMODEW(); dm.dmSize=ctypes.sizeof(dm)
if sys.argv[1:]==['current']:
    u.EnumDisplaySettingsW(None,-1,ctypes.byref(dm)); print('current',dm.dmPelsWidth,dm.dmPelsHeight,dm.dmDisplayFrequency)
else:
    s=set(); i=0
    while u.EnumDisplaySettingsW(None,i,ctypes.byref(dm)):
        s.add((dm.dmPelsWidth,dm.dmPelsHeight,dm.dmDisplayFrequency)); i+=1
    print(sorted(f for f in {x[2] for x in s})); print(sorted({x[2] for x in s if x[:2]==(2560,1080)}))
