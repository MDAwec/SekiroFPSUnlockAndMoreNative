#pragma once

// HDR in windowed and borderless mode. The game only has HDR in exclusive fullscreen (NvAPI / AMD AGS on a
// blt model swap chain); in a window its swap chain is made a flip model one showing HDR10 through DXGI.
// Must be installed before the game creates its swap chain.
void InstallWindowedHdr();
