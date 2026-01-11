# PS2 Live Graph Studio

<p align="center">
  <img src="https://img.shields.io/badge/Platform-PlayStation%202-blue" alt="Platform: PlayStation 2">
  <img src="https://img.shields.io/badge/Language-C99-green" alt="Language: C99">
  <img src="https://img.shields.io/badge/License-MIT-yellow" alt="License: MIT">
</p>

A **controller-driven, node-based live visual programming environment** for the Sony PlayStation 2. Create real-time 2D visuals by connecting nodes in a dataflow graph—all controlled with a DualShock 2 controller.

## Features

- **Live Visual Programming** — Edit your graph while it runs, see changes instantly
- **Controller-Driven UI** — No keyboard needed; full editing with DualShock 2
- **Node-Based Dataflow** — Connect nodes to build visual patches
- **Real-Time 2D Output** — Smooth 60 FPS rendering with gsKit
- **Command Palette** — Quick access to all editing commands
- **Save/Load** — Persist graphs via ps2client host filesystem

## Controls Overview

| Button | Action |
|--------|--------|
| **Left Stick** | Move cursor |
| **✕** | Select / Confirm |
| **○** | Cancel / Back to NAV |
| **△** | Add node / Edit params |
| **□** | Enter Wire mode |
| **Start** | Commit changes to live |
| **L2 (hold)** | Pan canvas |
| **R2 + △** | Command Palette |
| **R3** | Toggle editor visibility |

See [HELP.md](HELP.md) for complete control reference.

## Node Types

### Sources
- **CONST** — Constant value output
- **TIME** — Elapsed time (seconds)
- **PAD** — Controller input (sticks, buttons)
- **LFO** — Low-frequency oscillator

### Math
- **ADD** — Add two values
- **MUL** — Multiply two values
- **SIN** — Sine function
- **LERP** — Linear interpolation
- **SMOOTH** — Smoothed value transition
- **CLAMP** — Clamp value to range

### Output
- **COLORIZE** — Convert value to color
- **TRANSFORM2D** — Position, rotation, scale
- **RENDER2D** — Draw to screen (sink node)
- **DEBUG** — Display value on HUD

## Building

### Requirements

- [ps2toolchain](https://github.com/ps2dev/ps2toolchain) installed
- [ps2sdk](https://github.com/ps2dev/ps2sdk) configured
- [gsKit](https://github.com/ps2dev/gsKit) installed
- `PS2SDK` environment variable set
make and load the ELF like you do any other homebrew

## Project Structure

```
ps2-live-graph-studio/
├── src/
│   ├── main.c              # Entry point, main loop
│   ├── graph/              # Graph core, evaluation, validation
│   ├── nodes/              # Node type implementations
│   ├── render/             # gsKit rendering, fonts
│   ├── runtime/            # Runtime state management
│   ├── system/             # Controller input, timing
│   ├── ui/                 # Editor UI, command palette
│   └── io/                 # Graph I/O, asset loading
├── assets/                 # Fonts, palettes, default graphs
├── Makefile
└── HELP.md                 # User manual
```


## Workflow

1. **Add nodes** — Press △ to open the node menu
2. **Position nodes** — Select with ✕, move with stick
3. **Connect nodes** — Press □ for wire mode, click output then input
4. **Tweak parameters** — Select node, press △ for param mode
5. **Go live** — Press Start to commit changes

### Quick Example: Animated Circle

1. Add **TIME** node (provides animation)
2. Add **SIN** node (creates oscillation)
3. Add **RENDER2D** node (draws output)
4. Wire: TIME → SIN → RENDER2D.x
5. Press **Start** — watch it move!

## Documentation

- [HELP.md](HELP.md) — Complete user guide and control reference

## Contributing

This was a failed experiment on my end and I will not be improving this node based system, but if you like that sort of thing you can go for it. 

## 📄 License

MIT License — See [LICENSE](LICENSE) for details.
