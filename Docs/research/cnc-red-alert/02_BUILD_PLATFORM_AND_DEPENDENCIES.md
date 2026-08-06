# C&C Red Alert - Build System, Platform Dependencies & File Formats

> **Source**: `<home>/cnc-red-alert-original/CODE/`
> **Analysis Date**: July 2026
> **Confidence**: High (direct source analysis)

---

## 1. Build System Overview

### 1.1 Primary Build Tool: Watcom Make (WMAKE)

**Main Makefile**: `CODE/MAKEFILE` (21 KB, 755 lines)

The build system uses **Watcom Make (wmake)** with Watcom C/C++ compiler (`wpp386`) and **TASM** (Turbo Assembler) for assembly modules.

#### Key Build Variables (MAKEFILE:45-67)
```makefile
!ifdef WIN32
    WWFLAT=..\win32lib          # Win32 library path
    WWOBJ=obj\win32\$(LANGUAGE) # Object output directory
    LINKFILE=win95.lnk          # Linker response file
    CC=..\watcom\binnt\wpp386   # Watcom C++ compiler (NT-hosted)
    LIB=..\watcom\binnt\wlib    # Watcom librarian
!else
    WWFLAT=..\wwflat32          # DOS/DPMI library path
    WWOBJ=obj\dos               # Object output directory
    LINKFILE=conquer.lnk        # DOS linker response file
    CC=..\watcom\binnt\wpp386   # Same compiler, different target
    LIB=..\watcom\binnt\wlib
!endif
```

#### Compiler Flags (MAKEFILE:85-121)
**DOS/DPMI Target** (non-WIN32):
```makefile
CC_CFG = /d1                            # Partial debug (line numbers only)
CC_CFG += /i=..\watcom\H                # Watcom include directory
CC_CFG += /i=$(WWFLAT)\INCLUDE          # Westwood library includes
CC_CFG += /i=..\vq\include              # VQ video player includes
CC_CFG += /DDOS4G                       # Required for Greenleaf runtime
CC_CFG += /5s                           # Pentium-optimized stack calling
CC_CFG += /DGF_WATCOM_S                 # Greenleaf with /3s calling convention
CC_CFG += /bt=DOS                       # DOS target
```

**Win32 Target** (WIN32=1):
```makefile
CC_CFG = /d0                            # No debugging info
CC_CFG += /DWIN32=1 /D_WIN32            # Win32 defines
CC_CFG += /DWOLAPI_INTEGRATION          # Westwood Online API
CC_CFG += /DWINSOCK_IPX                 # IPX over Winsock
CC_CFG += /i=..\dxsdk\inc               # DirectX SDK includes
CC_CFG += /i=..\watcom\h\nt             # Watcom NT headers
CC_CFG += /i=..\winvq\include           # WinVQ video includes
CC_CFG += /bt=NT                        # Windows NT target
CC_CFG += /otxan                        # Optimizations: time, no alias, no FP
CC_CFG += /5r                           # Pentium register calling convention
```

#### Common Flags (both targets):
```makefile
CC_CFG += /i=..\gcl510\H                # Greenleaf Comm Library v5.10
CC_CFG += /of+                          # Traceable stack frames
CC_CFG += /zp1                          # Pack structs on 1-byte boundary
CC_CFG += /s                            # Remove stack check calls
CC_CFG += /j                            # char is signed
CC_CFG += /fh=$(WWOBJ)\conquer.pch      # Precompiled headers
CC_CFG += /fhq                          # Quiet PCH mode
CC_CFG += /we                           # Warnings as errors
CC_CFG += /w8                           # Most warnings enabled
CC_CFG += /ri                           # char/short returned as int
CC_CFG += /zq                           # Quiet operation
```

### 1.2 Linker: Watcom Linker (WLINK)

**DOS Linker Response** (`conquer.lnk` generated at MAKEFILE:523-540):
```makefile
system dos4g                    # DOS/4GW extender
option stack=128k
option redefsok
option quiet
option map
option eliminate
option caseexact
debug all
library $(WWOBJ)\jshell.lib
library $(WWOBJ)\tech.lib
library $(WWFLAT)\lib\wwflat32.lib
library ..\vq\lib\vqa32wp.lib
library ..\vq\lib\vqm32wp.lib
library ..\gcl510\w10\gclfr3s.lib
```

**Win32 Linker Response** (`win95.lnk` at MAKEFILE:544-571):
```makefile
system win95
option redefsok
option quiet
option map
option eliminate
option caseexact
option stack=128k
library $(WWFLAT)\lib\win32lib.lib
library ..\winvq\lib\vqa32wp.lib
library ..\winvq\lib\vqm32wp.lib
library ipx\wwipx32.lib
library ..\dxsdk\lib\dxguid.lib
library ..\dxsdk\lib\ddraw.lib
library ..\dxsdk\lib\dsound.lib
library $(WWFLAT)\lib\keyboard.lib
library mpgdll.lib                # MPEG video
library ..\dxmedia\lib\amstrmid.lib
library ..\dxmedia\lib\strmbasd.lib
library uuid.lib
```

### 1.3 Project Makefiles (CodeWright IDE)

| File | Size | Target | Description |
|------|------|--------|-------------|
| `ADAMTEMP.MAK` | 30 KB | Build asset pipeline | Lists all art/audio assets for MIX packaging |
| `BFILE.MAK` | 34 KB | DOS build | Main asset list (SHP, PCX, AUD, VQA, TEM, INT, etc.) |
| `BFILE2.MAK` | 34 KB | Alternate | Duplicate of BFILE with minor differences |
| `RA-HDOS.PJT` | 14 KB | DOS | CodeWright project for DOS executable |
| `RA-HOME.PJT` | 16 KB | DOS | Home edition project |
| `RA95.PJT` | 22 KB | Win95 | CodeWright project for Windows 95 |
| `RADOS.PJT` | 15 KB | DOS | Alternative DOS project |
| `REDALERT.IDE` | 66 KB | Binary | CodeWright IDE workspace |

### 1.4 Build Targets (MAKEFILE:461-465)
```makefile
!ifdef WIN32
all: ra95.exe       # Windows 95 executable
!else
all: game.dat       # DOS executable (bound with DOS/4GW)
!endif
```

### 1.5 External Dependencies

| Dependency | Version | Purpose | Location |
|------------|---------|---------|----------|
| **Watcom C/C++** | 10.6/11.0 | Compiler, linker, librarian | `..\watcom\binnt\` |
| **TASM** | 5.0 | 16/32-bit assembly | `utils\tasm` (invoked via MAKEFILE:152) |
| **DOS/4GW** | 1.97 | DOS extender (DPMI) | `..\watcom\4gwpro.exe` bound at MAKEFILE:502 |
| **Greenleaf Comm Lib** | 5.10 | Serial/modem/IPX comms | `..\gcl510\` |
| **DirectX SDK** | 3.0/5.0 | DirectDraw, DirectSound | `..\dxsdk\` |
| **VQ Video** | Custom | VQA video playback | `..\vq\` (DOS), `..\winvq\` (Win32) |
| **MPEG** | MPG | MPEG video (Win95) | `mpgdll.lib` |

---

## 2. Platform Targets

### 2.1 DOS/DPMI (Primary Target: `game.dat`)

**Architecture**: 32-bit protected mode via DOS/4GW extender
- **Memory Model**: Watcom flat model (`/bt=DOS`), selector = segment << 4
- **DPMI Interface**: `CODE/DPMI.CPP` & `CODE/DPMI.H` (DOSSegmentClass)
- **Entry Point**: `main()` in `STARTUP.CPP:111`
- **Minimum RAM**: 13 MB (checked at STARTUP.CPP:146-149)
- **Executable**: `game.dat` (DOS/4GW bound to `dos.exe` at MAKEFILE:502)

**Key Characteristics**:
- Custom DPMI segment management (DOSSegmentClass at DPMI.H:49-103)
- Real-mode stub (`CWSTUB.EXE` built from `CWSTUB.C` at MAKEFILE:488-492)
- Greenleaf Comm Library for serial/IPX (`/DDOS4G`, `/DGF_WATCOM_S`)
- Direct VGA access via linear framebuffer (0xA0000)

### 2.2 Windows 95 (Target: `ra95.exe`)

**Architecture**: Win32 native (Windows 95/NT)
- **Entry Point**: `WinMain()` in `STARTUP.CPP:109`
- **Windowing**: Custom main window (`Create_Main_Window` at STARTUP.CPP:63)
- **DirectX**: DirectDraw (ddraw.lib) + DirectSound (dsound.lib)
- **Networking**: Winsock IPX (`IPX95.CPP`, `IPX95.H` - loads DLL dynamically)
- **MPEG**: `mpgdll.lib` for video playback
- **MCI**: `MCIMPEG` support for movie playback (STARTUP.CPP:49-51)

**Win32-Specific Objects** (MAKEFILE:402-419):
```
2KEYFBUF.OBJ, CPUID.OBJ, GETCPU.OBJ, INTERPAL.OBJ,
WINASM.OBJ, WINSTUB.OBJ, 2TXTPRNT.OBJ, WRITEPCX.OBJ,
IPX95.OBJ, 2KEYFRAM.OBJ, TCPIP.OBJ, INTERNET.OBJ,
DDE.OBJ, CCDDE.OBJ, STATS.OBJ, PACKET.OBJ,
KEY.OBJ, FIELD.OBJ
```

### 2.3 Windows NT Support

The Win32 build targets both Windows 95 and NT:
- Watcom `/bt=NT` flag generates NT-compatible PE executables
- `IPX95.H` has `WindowsNT` global flag
- Thread-safe design for NT kernel

---

## 3. Memory Model & DPMI Details

### 3.1 Watcom Flat Memory Model

```c
// DPMI.H:54-56 - Selector arithmetic
// In Watcom flat: Selector == Segment << 4 (e.g., 0xA0000 for VGA)
inline void DOSSegmentClass::Assign(unsigned short segment, long) {
    Selector = (long)(segment) << 4L;  // DPMI.H:130-133
}
```

### 3.2 DOSSegmentClass (DPMI.H:49-103)

Provides unified real/protected mode memory access:
- **Selector-based**: Works in both real and protected mode
- **Inline operations**: `Copy_To`, `Copy_From`, `Copy_Word_To`, etc.
- **Swap support**: `Swap()` uses `#pragma aux` for inline assembly (DPMI.CPP:84-100)
- **Screen access**: VGA at selector 0xB0000 (DPMI.CPP:108)

### 3.3 DPMI Assembly Stubs (DPMI.CPP:47-81)

```cpp
#ifndef __FLAT__
void DOSSegmentClass::Swap(DOSSegmentClass &src, int soffset,
                           DOSSegmentClass &dest, int doffset, int size) {
    asm {
        push es; push ds
        mov si,soffset; mov di,doffset; mov cx,size
        mov ax,ssel; mov dx,dsel; mov ds,ax; mov es,dx
    again:
        mov al,ds:[si]; mov ah,es:[di]
        mov ds:[si],ah; mov es:[di],al
        inc di; inc si; dec cx; jnz again
        pop ds; pop es
    }
}
#endif
```

---

## 4. DirectX / DirectDraw Usage

### 4.1 Win95 DirectDraw (RA95)

From `WIN95.LNK` and `STARTUP.CPP`:
- **DirectDraw**: `ddraw.lib` for hardware-accelerated blitting
- **DirectSound**: `dsound.lib` for audio mixing
- **Video Back Buffer**: `VideoBackBufferAllowed` flag (STARTUP.CPP:68)
- **Surface Caps**: `DDSCAPS` used (STARTUP.CPP:118)

### 4.2 DOS Video (VQ/VQA)

Custom video system in `VQ/` and `WINVQ/`:
- **VQA Format**: Vector Quantized Animation (VQAFILE.H)
- **Codebooks**: 2x2, 4x2, 4x4 block VQ (VQAPLAY.H:58-61)
- **Compression**: Run-Skip-Dump (RSD) + LCW (VQAFILE.H:145-153)
- **Audio**: PCM, ADPCM, LCW compressed (VQAFILE.H:179-187)

---

## 5. File Formats

### 5.1 MIX Archive Format (`MIXFILE.CPP`, `MIXFILE.H`)

**Purpose**: Game asset container (similar to PAK/WAD)

**Structure** (MIXFILE.CPP:199-231):
```cpp
// Extended format detection
struct {
    short First;     // Always 0 for extended format
    short Second;    // Bitfield: 0x01=digest, 0x02=encrypted
} alternate;

FileHeader fileheader;  // { long count; long size; }
SubBlock header[count]; // { long Offset; long Size; long CRC; }
```

**Features**:
- CRC32-based indexing (MIXFILE.CPP:538)
- Optional Blowfish encryption (MIXFILE.CPP:221-225)
- SHA-1 message digest (MIXFILE.CPP:408-450)
- On-demand caching (MIXFILE.CPP:345-456)

**MIX Files in Game** (BFILE.MAK:1630-1643):
| File | Contents |
|------|----------|
| `CONQUER.MIX` | Core UI shapes, cursors, fonts (cached) |
| `GENERAL.MIX` | Mission INI, map PKT, UI sounds |
| `TEMPERAT.MIX` | Temperate theater terrain (.TEM) |
| `SNOW.MIX` | Snow theater terrain (.SNO) |
| `INTERIOR.MIX` | Interior building tiles (.INT) |
| `LORES.MIX` | Low-res shapes/fonts (320x200) |
| `HIRES.MIX` | Hi-res shapes/fonts (640x400) |
| `LORES1.MIX` / `HIRES1.MIX` | Expansion units |
| `SCORES.MIX` | Music tracks (AUD) |
| `SPEECH.MIX` | Voice overs (AUD) |
| `MOVIES1.MIX` | VQA cinematics |
| `NCHIRES.MIX` | Non-cached hi-res (VQP captions) |

### 5.2 SHP (Shape) Format

**Used for**: All 2D sprites (units, buildings, UI, cursors)

**References**:
- `SHAPEBTN.CPP:116` - `Get_Build_Frame_Width/Height(ShapeData)`
- `BFILE.MAK:127-268` - 200+ .SHP files listed
- `SHAPIPE.H/CPP` - SHA hash pipe for shapes (integrity)

**Format** (inferred from Westwood legacy):
- Multiple frames per file
- Per-frame palette indices
- RLE compression (LCW)
- Transparent color index 0

### 5.3 VQA Video Format (`VQAFILE.H`, `VQAPLAY.H`)

**Header** (VQAFILE.H:75-97):
```c
typedef struct _VQAHeader {
    unsigned short Version;       // 1 or 2
    unsigned short Flags;         // Audio, AltAudio
    unsigned short Frames;        // Total frames
    unsigned short ImageWidth;    // 320/640
    unsigned short ImageHeight;   // 200/400
    unsigned char  BlockWidth;    // 2, 4
    unsigned char  BlockHeight;   // 2, 4
    unsigned char  FPS;           // 15, 30
    unsigned char  Groupsize;     // Frames per codebook
    unsigned short Num1Colors;    // Single-color blocks
    unsigned short CBentries;     // Codebook size
    unsigned short Xpos, Ypos;    // Draw position (-1=center)
    unsigned short MaxFramesize;  // Largest frame bytes
    unsigned short SampleRate;    // Audio sample rate
    unsigned char  Channels;      // 1=mono, 2=stereo
    unsigned char  BitsPerSample; // 8 or 16
    // Alternate audio track info...
} VQAHeader;
```

**Chunk Types** (VQAFILE.H:161-187):
| Chunk ID | Description |
|----------|-------------|
| `VQHD` | Header |
| `FINF` | Frame info (keyframe, palette, sync flags + offset) |
| `VQFR` | Delta frame |
| `VQFK` | Keyframe |
| `CBF0/CBFZ` | Full codebook (Z=compressed) |
| `CBP0/CBPZ` | Partial codebook |
| `VPT0/VPTZ/VPTK/VPTD/VPTR/VPRZ` | Vector pointers (various compression) |
| `CPL0/CPLZ` | Color palette |
| `SND0/SND1/SND2/SNDZ` | Audio (PCM, ADPCM, LCW) |
| `SNA0/SNA1/SNA2/SNAZ` | Alt audio track |
| `CAP0` | Captions |
| `EVA0` | EVA text (C&C) |

**Compression** (VQAFILE.H:134-153):
- **VPC codes**: Single color, semi-transparent, short/long dump, short/long run
- **Run-Skip-Dump (RSD)**: Variable-length encoding
- **LCW**: Lempel-Castle-Welch (LCWPIPE.CPP)

### 5.4 PCX Format

**Usage**: Hi-res background images (BFILE.MAK:302-323)
- `ALIPAPER.PCX`, `PROLOG.PCX`, `SOVPAPER.PCX`
- Mission briefing images: `AFTR_HI.PCX`, `ALY1.PCX`, etc.
- Win95 only (HIRESFILES at BFILE.MAK:302)

**Loader**: `DIBFILE.CPP`, `LOADBMP.CPP` (Win95), `WRITEPCX.CPP`

### 5.5 TEM/SNO (Theater Terrain)

**Format**: Terrain tile definitions (BFILE.MAK:791-1123)
- `.TEM` = Temperate, `.SNO` = Snow (SNOWFILES = TEM→SNO at BFILE.MAK:1125)
- 1100+ terrain templates per theater
- Used by map editor and game engine

### 5.6 INT (Interior Tiles)

**Format**: Building interior tiles (BFILE.MAK:636-789)
- 150+ interior tile types
- `CLEAR1.INT`, `WALLxxxx.INT`, `FLORxxxx.INT`, etc.

### 5.7 AUD Audio Format

**Format**: Raw PCM with ADPCM variants
- Extension `.AUD` (BFILE.MAK:1131-1273)
- 200+ sound effects
- ADPCM decompression in `ADPCM.CPP` (SOS/Studies Weekly codec)
- Sample rates vary (11kHz, 22kHz)

**ADPCM** (ADPCM.CPP:34-81):
```c
// Nibble-based differential coding
token = *inbuff++;
fastindex += token & 0x0f;      // First nibble
sample += DiffTable[fastindex];
fastindex = IndexTable[fastindex];
*outbuff++ = (short)sample;
// Second nibble (high 4 bits)
fastindex += token >> 4;
...
```

### 5.8 FNT (Font) Format

**Format**: Bitmap fonts with gradient support
- `3POINT.FNT`, `6POINT.FNT`, `8POINT.FNT`, `12METFNT.FNT`
- `HELP.FNT`, `VCR.FNT`, `TYPE.FNT`, `SCOREFNT.FNT`
- `GRAD6FNT.FNT`, `EDITFNT.FNT`, `LED.FNT`
- Palette: `EGOPAL.PAL`

### 5.9 WSA (Westwood Sound Archive)

**Format**: Streaming audio (BFILE.MAK:607-634)
- `MSAA.WSA` through `MSSN.WSA` (14 files)
- `MSSA.WSA` through `MSSN.WSA` (14 files)
- Used for music tracks

### 5.10 PAL (Palette)

**Format**: 256-color RGB palette (768 bytes)
- `TEMPERAT.PAL`, `SNOW.PAL`, `INTERIOR.PAL`
- `EGOPAL.PAL` (UI palette)

---

## 6. Asset Pipeline

### 6.1 Build Process (BFILE.MAK / ADAMTEMP.MAK)

```makefile
# MIX file generation (BFILE.MAK:1695-1808)
CONQUER.MIX: $(CONQUERFILES) $(CACHEMAP) .\key.ini
    UTILS\MIXFILE -k -h -I$(.path.cps) &&!
    $(CONQUERFILES) $(CACHEMAP)
! $(.path.mix)$&.mix

HIRES.MIX: $(HIRESFILES:.SHP=.HI) $(HIHILORES) .\key.ini
    UTILS\MIXFILE -h -k -E.HI=.SHP -E.HNT=.FNT -I$(.path.cps) &&!
    $(HIRESFILES:.SHP=.HI) $(HIHILORES)
! $(.path.mix)$&.mix
```

### 6.2 Art Tools (TOOLS/)

| Tool | Purpose |
|------|---------|
| `MIXFILE.EXE` | Create/extract MIX archives |
| `WWCOMP.EXE` | Compress files (LCW) |
| `WWPACK.EXE` | Package assets |
| `AUDIOMAK.EXE` | Build audio MIX files |
| `FONTMAKE.EXE` | Generate FNT from bitmaps |
| `ICONMAP.EXE` / `ICONCOMP.EXE` | Icon processing |
| `KEYFRAME.EXE` | Animation keyframe extraction |
| `ANIMATE.EXE` | Animation preview |
| `MAP2MAP.EXE` | Map format conversion |
| `GETREG.EXE` | Registry access (Win95) |

### 6.3 Asset Naming Conventions

| Extension | Content | Resolution Variants |
|-----------|---------|---------------------|
| `.SHP` | Shapes/sprites | `.HI`/`.HNT` (hi-res), `.LOW`/`.LNT` (lo-res) |
| `.PCX` | Backgrounds | Hi-res only |
| `.TEM`/`.SNO` | Terrain | Theater-specific |
| `.INT` | Interiors | Shared |
| `.AUD` | Sound effects | Mono/stereo |
| `.WSA` | Music streams | ADPCM |
| `.VQA` | Video | VQA format |
| `.VQP` | Video palette | Caption overlays |
| `.FNT`/`.HNT`/`.LNT` | Fonts | Multi-res |
| `.PAL` | Palettes | 256-color |

---

## 7. Modern Build Challenges

### 7.1 Toolchain Availability

| Component | Status | Alternative |
|-----------|--------|-------------|
| Watcom C/C++ 10.6/11.0 | **Unavailable** (abandonware) | Open Watcom 1.9/2.0 (partial compatibility) |
| TASM 5.0 | **Unavailable** | NASM (syntax differences), JWASM/UASM |
| DOS/4GW | **Unavailable** | DOS/32A, HX DOS Extender |
| DirectX 3/5 SDK | **Unavailable** | DX7+ SDK (API changes), SDL2 |
| Greenleaf Comm Lib | **Commercial** | Custom replacement |

### 7.2 Code Portability Issues

1. **Watcom-Specific Pragmas**:
   - `#pragma aux` for inline assembly (DPMI.CPP:88-96, LCW.H:46)
   - `#pragma pack(1)` (VQAFILE.H:47)
   - `#pragma warning` (WATCOM.H:42-75)

2. **Calling Conventions**:
   - `/5r` (register) (Pentium register) / `/5s` (stack) - Watcom specific
   - `__stdcall` for Win32 DLL imports (IPX95.H)

3. **Memory Model Assumptions**:
   - Flat selector = segment << 4 (DPMI.H:54, 132)
   - Direct linear framebuffer access (0xA0000)

4. **Assembly Dependencies**:
   - 16 `.ASM` files in CODE/ (KEYFBUFF, SUPPORT, TXTPRNT, COORDA, CPUID, LCWCOMP, LCWUNCMP, WINASM, IPXPROT, IPXREAL, PAGFAULT, MEM_COPY, MOUSE, KEYIPROT, KEYIREAL, 2SUPPORT, 2KEYFBUF, 2TXTPRNT)
   - TASM syntax (not MASM/NASM compatible)

### 7.3 Required Source Modifications for Modern Build

| File | Issue | Fix |
|------|-------|-----|
| `MAKEFILE` | Hardcoded paths, Watcom flags | Convert to CMake/Meson; update compiler flags |
| `DPMI.H/.CPP` | DOS-specific | Replace with SDL2/virtual memory |
| `IPX*.CPP` | IPX/SPX networking | Replace with UDP/ENet |
| `VQA*/*.CPP` | Custom video | Replace with FFmpeg/Theora |
| `ADPCM.CPP` | SOS codec | Replace with FAAD2/opus |
| `LCW*.CPP/.ASM` | Custom compression | Replace with zstd/lz4 |
| `MIXFILE.CPP` | Custom archive | Replace with zip/7z |
| `STARTUP.CPP` | Dual DOS/WinMain | Platform abstraction layer |

---

## 8. Summary: Key Technical Specifications

| Aspect | Specification |
|--------|---------------|
| **Language** | C++98 (Watcom dialect), x86 Assembly (TASM) |
| **Compiler** | Watcom C/C++ 10.6/11.0 (`wpp386`) |
| **Assembler** | Turbo Assembler 5.0 (`tasm`) |
| **Linker** | Watcom Linker (`wlink`) |
| **DOS Extender** | DOS/4GW 1.97 (DPMI) |
| **Win32 SDK** | DirectX 3/5, Winsock 1.1 |
| **Memory Model** | Flat 32-bit (Watcom `/bt=DOS` or `/bt=NT`) |
| **Calling Convention** | Register-optimized (`/5r` Pentium) / Stack (`/5s`) |
| **Struct Packing** | 1-byte (`/zp1`) |
| **Debug Format** | Watcom (`/d1` line numbers, `/d0` none) |
| **Archive Format** | MIX (custom, CRC32 indexed, optional Blowfish/SHA1) |
| **Sprite Format** | SHP (RLE+LCW compressed, multi-frame) |
| **Video Format** | VQA (Vector Quantized Animation, custom codec) |
| **Audio Format** | Raw PCM, ADPCM (SOS), LCW compressed |
| **Terrain Format** | TEM/SNO (theater-specific tile templates) |
| **Font Format** | FNT (bitmap + gradient) |
| **Network** | IPX (DOS), IPX-over-Winsock (Win95), TCP/IP (Win95) |

---

## 9. Evidence Index

| Section | File | Lines | Confidence |
|---------|------|-------|------------|
| Build System | `CODE/MAKEFILE` | 1-755 | 100% |
| DOS/DPMI | `CODE/DPMI.H`, `CODE/DPMI.CPP` | 1-175 / 1-169 | 100% |
| Watcom Config | `CODE/WATCOM.H` | 1-90 | 100% |
| Platform Targets | `CODE/MAKEFILE` | 55-67, 402-419 | 100% |
| Linker Scripts | `CODE/MAKEFILE` | 523-571 | 100% |
| Win95 Entry | `CODE/STARTUP.CPP` | 107-138 | 100% |
| DirectX Link | `CODE/MAKEFILE` | 562-565 | 100% |
| MIX Format | `CODE/MIXFILE.CPP` | 1-580 | 100% |
| VQA Format | `VQ/VQA32/VQAFILE.H` | 1-197 | 100% |
| VQA Player | `VQ/VQA32/VQAPLAY.H` | 1-321 | 100% |
| Asset Lists | `CODE/BFILE.MAK` | 1-1847 | 100% |
| ADPCM Audio | `CODE/ADPCM.CPP` | 1-83 | 100% |
| LCW Compression | `CODE/LCWPIPE.CPP`, `CODE/LCW.H` | 1-313 / 1-49 | 100% |
| Tools | `TOOLS/` directory listing | - | 100% |
| Assembly Files | `CODE/*.ASM` | 16 files | 100% |

---

*Document generated from primary source analysis of the original EA/Westwood C&C Red Alert source code repository.*