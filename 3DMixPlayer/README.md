# 3DMixPlayer

A standalone player and 3D visualizer for **BeOS `.3dmix` projects**, running
natively on **Haiku OS**. Extracted from
[VeniceDAW](https://github.com/atomozero/venicedaw) as a self-contained
application.

3DMixPlayer loads the historic BeOS R5 (1995–2001) `.3dmix` project format,
resolves and decodes the referenced audio tracks, and plays them back with
real-time 3D spatial positioning, an OpenGL scene view and a timeline with
loop support.

## Features

- **3dmix import** — full BMessage parser for BeOS `.3dmix` files, with
  intelligent multi-strategy audio path resolution and RAW format detection.
- **Real-time spatial playback** — `BSoundPlayer`-based mixing of all tracks
  with per-track 3D positioning (front/back depth, left/right, up/down).
- **OpenGL 3D scene** — interactive view of every track's position in space.
- **Timeline & transport** — playhead, loop in/out points, fit-to-window.
- **Per-track biquad filters** — low/high/band-pass, notch, peaking and shelf EQ.
- **C64-style about box** — a Commodore 64 demo-scene tribute.

## Requirements

Native **Haiku OS** only. The application uses the BeAPI (Interface Kit,
Media Kit, `BSoundPlayer`, OpenGL) and cannot be built or run on Linux/WSL.

Required system libraries: `be`, `root`, `media`, `tracker`, `translation`,
`GL`, `GLU`.

## Build

```bash
make            # build ./3DMixPlayer
make debug      # build with symbols and verbose logging
make clean      # remove objects and binary
make install    # copy the binary to the Desktop
```

## Usage

```bash
./3DMixPlayer /path/to/project.3dmix
```

You can also launch it without arguments and open a project via
**File → Open…**. Recently opened projects are listed under
**File → Open Recent**.

### Keyboard controls (3D view)

| Key            | Action                          |
|----------------|---------------------------------|
| `Space`        | Play / pause                    |
| `R` / `Home`   | Reset playhead to start         |
| `End`          | Jump to end                     |
| `+` / `=`      | Zoom in                         |
| `-` / `_`      | Zoom out                        |
| `F`            | Fit scene to window             |
| `I`            | Set loop **in** point           |
| `O`            | Set loop **out** point          |
| `L`            | Toggle loop on / off            |

Drag with the mouse to rotate the scene and reposition tracks.

## Project layout

```
3DMixPlayer/
├── Makefile
├── README.md
├── LICENSE
└── src/
    ├── 3DMixPlayer.cpp            # application, GUI, audio playback
    ├── audio/
    │   ├── AudioLogging.{h,cpp}   # conditional logging
    │   ├── BiquadFilter.{h,cpp}   # per-track EQ / filters
    │   └── 3dmix/                 # BeOS .3dmix import system
    │       ├── 3DMixFormat.{h,cpp}
    │       ├── 3DMixParser.{h,cpp}
    │       ├── CoordinateSystemMapper.{h,cpp}
    │       └── AudioPathResolver.{h,cpp}
    └── testing/
        └── HaikuMockHeaders.h     # syntax-check stubs (non-Haiku only)
```

## License

See [LICENSE](LICENSE). Originally part of the VeniceDAW project.
