CC  = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
VERSION := $(shell tr -d "[:space:]" < VERSION)
NAME = SekiroFPSUnlockAndMoreNative

CPPFLAGS = -Isrc -Ithird_party -Ithird_party/minhook/include -DPROJECT_VERSION_STRING=\"$(VERSION)\" -MMD -MP
CFLAGS   = -O3 -flto -ffunction-sections -fdata-sections -Wall
CXXFLAGS = -std=c++17 $(CFLAGS)
LDFLAGS  = -shared -static -s -flto -Wl,--gc-sections

CXX_SRC := $(wildcard src/*.cpp)
C_SRC   := $(wildcard third_party/minhook/src/*.c third_party/minhook/src/hde/*.c)
OBJ := $(CXX_SRC:%.cpp=build/obj/%.o) $(C_SRC:%.c=build/obj/%.o)

# The mod is loaded by the game as its d3dcompiler_43.dll; it forwards D3DGetBlobPart to the real one.
all: build/d3dcompiler_43.dll

build/d3dcompiler_43.dll: $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $(OBJ)

build/obj/src/main.o: VERSION

build/obj/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

build/obj/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

dist: all
	rm -f build/$(NAME).7z
	cd build && 7z a -bso0 $(NAME).7z d3dcompiler_43.dll ../LICENSE ../README.md ../CHANGELOG.md

# Copies the DLL into the game folder: make install SEKIRO_PATH=/path/to/Sekiro
install: all
	cp build/d3dcompiler_43.dll "$(SEKIRO_PATH)/"

clean:
	rm -rf build

-include $(OBJ:.o=.d)

.PHONY: all dist install clean
