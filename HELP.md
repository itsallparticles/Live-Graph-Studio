# PS2 Live Graph Studio - Help Guide

## Screen Layout

The editor UI overlays on top of the live preview output.

Press **R3** (click right stick) to toggle the editor on/off for clean preview.

## Controller Layout

```
        L1                              R1
        L2                              R2
    
    [D-Pad]     [Select] [Start]     [△] [○]
                                     [✕] [□]
    
      [Left Stick]            [Right Stick]
         (L3)                    (R3)
```

---

## UI Modes

The editor has four modes, shown in the top-left HUD:

| Mode   | Description                              |
|--------|------------------------------------------|
| NAV    | Navigate canvas, select nodes            |
| WIRE   | Connect node outputs to inputs           |
| PARAM  | Edit selected node's parameters          |
| ADD    | Add new nodes from the menu              |

---

## Global Controls

These controls work in most modes, unless the Command Palette is open.

| Button          | Action                                      |
|-----------------|---------------------------------------------|
| Left Stick      | Move cursor                                 |
| L1 + Left Stick | Fine cursor movement (slower)               |
| R1 + Left Stick | Fast cursor movement (faster)               |
| L2 (hold)       | Pan canvas instead of moving cursor         |
| Start           | **Commit** - publish edit graph to live     |
| ○ (Circle)      | Return to NAV mode / cancel current action  |
| R2 + △          | Open Command Palette (NAV mode only)        |
| R3              | Toggle editor visibility (full-screen preview) |

---

## Controls by Mode

### Navigation Mode (NAV)

| Button        | Action                                    |
|---------------|-------------------------------------------|
| D-Pad         | Nudge cursor in steps                     |
| ✕ (Cross)     | Select node under cursor                  |
| □ (Square)    | Enter Wire mode                           |
| △ (Triangle)  | Enter Param mode (if node selected with params) or Add menu |
| R2 + △        | Open Command Palette                      |

### Wire Mode (WIRE)

| Button        | Action                                    |
|---------------|-------------------------------------------|
| D-Pad         | Nudge cursor in steps                     |
| ✕ (Cross)     | Select output port (first click) or connect to input port (second click) |
| ○ (Circle)    | Cancel wire mode, return to NAV           |
| □ (Square)    | Cancel wire mode, return to NAV           |

### Parameter Edit Mode (PARAM)

| Button        | Action                                    |
|---------------|-------------------------------------------|
| D-Pad Up/Down | Select parameter                          |
| D-Pad Left    | Decrease parameter value                  |
| D-Pad Right   | Increase parameter value                  |
| L1 + D-Pad    | Fine adjustment (smaller step)            |
| R1 + D-Pad    | Coarse adjustment (larger step)           |
| ○ (Circle)    | Exit parameter mode                       |
| △ (Triangle)  | Exit parameter mode                       |
| □ (Square)    | Exit parameter mode                       |

### Add Node Mode (ADD)

| Button        | Action                                    |
|---------------|-------------------------------------------|
| D-Pad Up/Down | Scroll through node list                  |
| ✕ (Cross)     | Create selected node at cursor            |
| ○ (Circle)    | Cancel and return to NAV mode             |

---

## Command Palette

The Command Palette provides access to additional commands.

### Opening the Palette

Press **R2 + △** while in NAV mode to open the Command Palette.

### Palette Navigation

| Button        | Action                                    |
|---------------|-------------------------------------------|
| D-Pad Up/Down | Navigate command list                     |
| ✕ (Cross)     | Execute selected command                  |
| ○ (Circle)    | Close palette                             |
| △ (Triangle)  | Close palette                             |

### Available Commands

| Command          | Condition              | Description                          |
|------------------|------------------------|--------------------------------------|
| Add Node         | Always                 | Enter ADD mode                       |
| Edit Parameters  | Node with params selected | Enter PARAM mode                  |
| Wire Connect     | Node selected          | Enter WIRE mode                      |
| Delete Node      | Node selected          | Delete the selected node             |
| Duplicate Node   | Node selected          | Create copy of selected node         |
| Clear Selection  | Node selected          | Deselect current node                |
| Commit Edits     | Always                 | Publish edit graph to live           |
| Revert Edits     | Has active graph       | Reset edit graph to live state       |
| Validate Graph   | Always                 | Check graph for errors               |
| Save Graph       | Always                 | Save to host:graph.gph               |
| Load Graph       | Always                 | Load from host:graph.gph             |

**Note:** While Command Palette is open, all other editor input is suppressed.

---

## Workflow

### Basic Steps

1. **Add nodes** using △ (when no node selected) to open the menu
2. **Position nodes** by selecting (✕) and moving cursor
3. **Connect nodes** using □ to enter wire mode
4. **Adjust parameters** using △ (when node selected) to enter param mode
5. **Commit changes** with Start to see live output

### Creating a Simple Patch

1. Press △ (no selection), select "CONST", press ✕ (creates constant value node)
2. Press △ (no selection), select "RENDER2D", press ✕ (creates render output)
3. Select the CONST node with ✕, press □ to enter wire mode
4. Move cursor to CONST's output port, press ✕ to select it
5. Move cursor to RENDER2D's input port, press ✕ to connect
6. Press Start to commit - you should see a colored rectangle

### Deleting a Node

Use the Command Palette:
1. Select a node with ✕
2. Press R2 + △ to open Command Palette
3. Select "Delete Node" and press ✕

### Reverting Changes

To undo all edits and reset to the live graph state:
1. Press R2 + △ to open Command Palette
2. Select "Revert Edits" and press ✕

---

## Node Reference

### Source Nodes (Generate Values)

| Node    | Outputs      | Params           | Description                          |
|---------|--------------|------------------|--------------------------------------|
| CONST   | value        | value            | Outputs a constant value             |
| TIME    | sec, ms, norm| -                | Current time (seconds, ms, 0-1 loop) |
| PAD     | lx,ly,rx,ry,btn| -              | Controller input values              |
| NOISE   | raw,smooth,bi| speed            | Random noise generator               |
| LFO     | sin,tri,saw  | freq, amp, wave  | Low-frequency oscillator             |

### Math Nodes (Arithmetic)

| Node    | Inputs  | Outputs | Params        | Description                    |
|---------|---------|---------|---------------|--------------------------------|
| ADD     | a, b    | sum     | -             | a + b                          |
| SUB     | a, b    | diff    | -             | a - b                          |
| MUL     | a, b    | product | -             | a × b                          |
| DIV     | a, b    | quot    | -             | a ÷ b (safe, no div-by-zero)   |
| MOD     | a, b    | rem     | -             | a mod b                        |
| ABS     | in      | out     | -             | Absolute value                 |
| NEG     | in      | out     | -             | Negate (-in)                   |
| MIN     | a, b    | min     | -             | Smaller of a, b                |
| MAX     | a, b    | max     | -             | Larger of a, b                 |
| CLAMP   | in      | out     | min, max      | Clamp to [min, max]            |
| MAP     | in      | out,norm| in_min/max, out_min/max | Remap value range |

### Trigonometry Nodes

| Node    | Inputs  | Outputs     | Params     | Description                    |
|---------|---------|-------------|------------|--------------------------------|
| SIN     | angle   | value       | freq, amp  | Sine function                  |
| COS     | angle   | value       | freq, amp  | Cosine function                |
| TAN     | angle   | value       | -          | Tangent function               |
| ATAN2   | y, x    | angle,sin,cos| -         | Angle from coordinates         |

### Filter/Utility Nodes

| Node    | Inputs     | Outputs | Params           | Description                   |
|---------|------------|---------|------------------|-------------------------------|
| LERP    | a, b, t    | out     | -                | Linear interpolation          |
| SMOOTH  | in         | out     | speed            | Smoothed/filtered value       |
| STEP    | in         | out     | edge, smooth     | Step function (hard/soft)     |
| PULSE   | trigger    | out     | duration         | Generates pulse on trigger    |
| HOLD    | in, trigger| out     | -                | Sample and hold               |
| DELAY   | in         | out     | frames           | Delay by N frames             |

### Logic Nodes

| Node    | Inputs     | Outputs | Params      | Description                      |
|---------|------------|---------|-------------|----------------------------------|
| COMPARE | a, b       | out     | op          | Compare: <, <=, ==, !=, >=, >    |
| SELECT  | a, b, sel  | out     | threshold   | Select a or b based on sel       |
| GATE    | in, gate   | out     | -           | Pass through when gate > 0       |

### Color Nodes

| Node     | Inputs      | Outputs      | Params     | Description                   |
|----------|-------------|--------------|------------|-------------------------------|
| SPLIT    | color       | r, g, b, a   | -          | Split color into components   |
| COMBINE  | r, g, b, a  | color        | -          | Combine components to color   |
| COLORIZE | value       | r, g, b, a   | hue, sat   | Map value to color            |
| HSV      | h, s, v     | r, g, b      | -          | HSV to RGB conversion         |
| GRADIENT | t           | r, g, b, a   | colors     | Sample color gradient         |

### Render Nodes (Sinks)

| Node          | Inputs      | Params       | Description                    |
|---------------|-------------|--------------|--------------------------------|
| RENDER2D      | r, g, b, a  | X, Y, W, H   | Draw filled rectangle          |
| RENDER_CIRCLE | r, g, b, a  | X, Y, radius | Draw filled circle             |
| RENDER_LINE   | r, g, b, a  | x1,y1,x2,y2  | Draw line                      |
| DEBUG         | in          | -            | Pass-through for debugging     |

---

## Parameter Ranges

All coordinates (X, Y) are **normalized 0.0 to 1.0**:
- 0.0 = left/top edge of screen
- 1.0 = right/bottom edge of screen
- 0.5 = center

Colors are also **normalized 0.0 to 1.0**:
- 0.0 = black/transparent
- 1.0 = full intensity/opaque

---

## Tips

- **Commit often** (Start) to see your changes live
- Use **TIME** node outputs to animate values
- Connect **PAD** node to control visuals with the controller
- Use **SMOOTH** to make jerky values more fluid
- **LFO** is great for repeating animations
- Chain **MUL** and **ADD** to scale and offset values
- **CLAMP** prevents values from going out of range
- **L2 (hold)** lets you pan the canvas to see more of your graph
- Press **R3** to hide editor and see clean preview output

---

## Troubleshooting

| Problem                    | Solution                                      |
|----------------------------|-----------------------------------------------|
| Nothing renders            | Make sure you have a RENDER2D node and committed |
| Rectangle is off-screen    | Check X, Y params are in 0.0-1.0 range        |
| Colors look wrong          | Color inputs expect 0.0-1.0, not 0-255        |
| Node has no ports visible  | Ports are on the left (in) and right (out) edges |
| Changes don't take effect  | Press Start to commit the edit graph          |
| Commit fails (cycle)       | You have a circular connection - break the loop |
| Commit fails (no sink)     | Add at least one RENDER2D node                |
| Need to revert changes     | Open Command Palette (R2+△), select "Revert Edits" |
