#ifndef COMMAND_PALETTE_H
#define COMMAND_PALETTE_H

#include "../common.h"
#include "../graph/graph_types.h"
#include "../system/pad.h"
#include <stdint.h>

/* ============================================================
 * Command Palette Configuration
 * ============================================================ */
#define CMD_PALETTE_MAX_COMMANDS   32
#define CMD_PALETTE_MAX_VISIBLE    10
#define CMD_PALETTE_WIDTH          180
#define CMD_PALETTE_ITEM_HEIGHT    16

/* ============================================================
 * Forward Declarations
 * ============================================================ */
struct EditorState;
struct CommitApi;

/* ============================================================
 * Palette Context (passed to open/update/execute)
 * ============================================================
 * Provides all external dependencies the palette needs without
 * requiring access to editor-private symbols.
 * ============================================================ */
typedef struct CmdPaletteContext {
    struct EditorState *state;         /* Editor state: edit_graph, ui, ui_meta */
    const Graph        *active_graph;  /* Live/active graph (may be NULL) */
    const struct CommitApi *commit_api;/* Commit hooks from editor (may be NULL) */
} CmdPaletteContext;

/* ============================================================
 * Command Definition (context-based signatures)
 * ============================================================ */
typedef int (*CmdIsEnabledFunc)(const CmdPaletteContext *ctx);
typedef void (*CmdExecuteFunc)(CmdPaletteContext *ctx);

typedef struct CommandDef {
    const char       *label;
    CmdIsEnabledFunc  is_enabled;
    CmdExecuteFunc    execute;
} CommandDef;

/* ============================================================
 * Command Palette State
 * ============================================================ */
typedef struct CommandPalette {
    uint8_t  is_open;
    uint8_t  selected_index;    /* Index into filtered list */
    uint8_t  scroll_offset;
    uint8_t  filtered_count;
    uint8_t  filtered[CMD_PALETTE_MAX_COMMANDS];  /* Indices of enabled commands */
} CommandPalette;

/* ============================================================
 * Command Palette API (context-based)
 * ============================================================ */

/* Initialize palette state (call once at startup) */
void cmd_palette_init(CommandPalette *palette);

/* Open the command palette (rebuilds filtered list from context) */
void cmd_palette_open(CommandPalette *palette, const CmdPaletteContext *ctx);

/* Close the command palette */
void cmd_palette_close(CommandPalette *palette);

/* Check if palette is currently open */
int cmd_palette_is_open(const CommandPalette *palette);

/* Update palette input handling.
 * ctx: palette context for command execution
 * now/prev: current and previous pad state
 * Returns 1 if a command was executed, 0 otherwise.
 * Caller should suppress normal editor input while palette is open. */
int cmd_palette_update(CommandPalette *palette,
                       CmdPaletteContext *ctx,
                       const PadState *now,
                       const PadState *prev);

/* Render the command palette overlay */
void cmd_palette_draw(const CommandPalette *palette,
                      void (*rect_filled)(int x, int y, int w, int h, uint32_t color),
                      void (*draw_text)(int x, int y, uint32_t color, const char *text));

/* ============================================================
 * Command Registration (internal use)
 * ============================================================ */

/* Get total number of registered commands */
uint8_t cmd_palette_get_command_count(void);

/* Get command definition by index */
const CommandDef *cmd_palette_get_command(uint8_t index);

#endif /* COMMAND_PALETTE_H */
