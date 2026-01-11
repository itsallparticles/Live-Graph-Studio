#include "command_palette.h"
#include "editor.h"
#include "../graph/graph_core.h"
#include "../graph/graph_validate.h"
#include "../graph/graph_publish.h"
#include "../io/graph_io.h"
#include "../nodes/node_registry.h"
#include <string.h>
#include <stdio.h>

/* ============================================================
 * Colors
 * ============================================================ */
#define CMD_COLOR_BG          0xE0202020u
#define CMD_COLOR_BG_HEADER   0xE0404060u
#define CMD_COLOR_ITEM_SEL    0xE0606090u
#define CMD_COLOR_ITEM_HOVER  0xE0505070u
#define CMD_COLOR_TEXT        0xFFFFFFFFu
#define CMD_COLOR_TEXT_DIM    0xFF808080u
#define CMD_COLOR_BORDER      0xFF606060u

/* ============================================================
 * Palette Position
 * ============================================================ */
#define CMD_PALETTE_X         ((SCREEN_W - CMD_PALETTE_WIDTH) / 2)
#define CMD_PALETTE_Y         60
#define CMD_PALETTE_HEADER_H  18
#define CMD_PALETTE_PAD       4

/* ============================================================
 * Command Implementations - Forward Declarations
 * All commands now use CmdPaletteContext* instead of separate args
 * ============================================================ */
static int cmd_always_enabled(const CmdPaletteContext *ctx);
static int cmd_has_selection(const CmdPaletteContext *ctx);
static int cmd_has_selection_with_params(const CmdPaletteContext *ctx);
static int cmd_has_active_graph(const CmdPaletteContext *ctx);

static void cmd_add_node(CmdPaletteContext *ctx);
static void cmd_edit_params(CmdPaletteContext *ctx);
static void cmd_wire_connect(CmdPaletteContext *ctx);
static void cmd_delete_node(CmdPaletteContext *ctx);
static void cmd_duplicate_node(CmdPaletteContext *ctx);
static void cmd_commit(CmdPaletteContext *ctx);
static void cmd_revert(CmdPaletteContext *ctx);
static void cmd_validate(CmdPaletteContext *ctx);
static void cmd_save_graph(CmdPaletteContext *ctx);
static void cmd_load_graph(CmdPaletteContext *ctx);
static void cmd_clear_selection(CmdPaletteContext *ctx);

/* ============================================================
 * Static Command Table
 * ============================================================ */
static const CommandDef s_commands[] = {
    /* Mode Routing */
    { "Add Node",         cmd_always_enabled,              cmd_add_node },
    { "Edit Parameters",  cmd_has_selection_with_params,   cmd_edit_params },
    { "Wire Connect",     cmd_has_selection,               cmd_wire_connect },

    /* Node Operations */
    { "Delete Node",      cmd_has_selection,               cmd_delete_node },
    { "Duplicate Node",   cmd_has_selection,               cmd_duplicate_node },
    { "Clear Selection",  cmd_has_selection,               cmd_clear_selection },

    /* Graph Operations */
    { "Commit Edits",     cmd_always_enabled,              cmd_commit },
    { "Revert Edits",     cmd_has_active_graph,            cmd_revert },
    { "Validate Graph",   cmd_always_enabled,              cmd_validate },

    /* Session */
    { "Save Graph",       cmd_always_enabled,              cmd_save_graph },
    { "Load Graph",       cmd_always_enabled,              cmd_load_graph },
};

#define CMD_COUNT (sizeof(s_commands) / sizeof(s_commands[0]))

/* ============================================================
 * Enable Condition Implementations (context-based)
 * ============================================================ */
static int cmd_always_enabled(const CmdPaletteContext *ctx)
{
    (void)ctx;
    return 1;
}

static int cmd_has_selection(const CmdPaletteContext *ctx)
{
    NodeId sel;
    if (!ctx || !ctx->state) return 0;
    sel = ctx->state->ui.selected_node;
    if (sel == INVALID_NODE_ID || sel >= MAX_NODES) return 0;
    return ctx->state->edit_graph.nodes[sel].type != NODE_TYPE_NONE;
}

static int cmd_has_selection_with_params(const CmdPaletteContext *ctx)
{
    const NodeMeta *meta;
    if (!cmd_has_selection(ctx)) return 0;
    meta = node_registry_get_meta(ctx->state->edit_graph.nodes[ctx->state->ui.selected_node].type);
    return meta && meta->num_params > 0;
}

static int cmd_has_active_graph(const CmdPaletteContext *ctx)
{
    /* Revert requires an active graph to copy from */
    return ctx && ctx->active_graph != NULL;
}

/* ============================================================
 * Command Execute Implementations (context-based)
 * ============================================================ */
static void cmd_add_node(CmdPaletteContext *ctx)
{
    if (!ctx || !ctx->state) return;
    ctx->state->ui.mode = UI_EDITOR_MODE_ADD;
    ctx->state->ui.add_index = 0;
    ctx->state->ui.add_scroll = 0;
}

static void cmd_edit_params(CmdPaletteContext *ctx)
{
    if (!ctx || !ctx->state) return;
    if (!cmd_has_selection_with_params(ctx)) return;
    ctx->state->ui.mode = UI_EDITOR_MODE_PARAM;
    ctx->state->ui.selected_param = 0;
}

static void cmd_wire_connect(CmdPaletteContext *ctx)
{
    if (!ctx || !ctx->state) return;
    ctx->state->ui.mode = UI_EDITOR_MODE_WIRE;
    ctx->state->ui.wire_src_node = INVALID_NODE_ID;
}

static void cmd_delete_node(CmdPaletteContext *ctx)
{
    NodeId sel;
    EditorState *state;

    if (!ctx || !ctx->state) return;
    state = ctx->state;
    sel = state->ui.selected_node;
    if (!cmd_has_selection(ctx)) return;

    /* Delete the node */
    graph_free_node(&state->edit_graph, sel);
    state->ui.selected_node = INVALID_NODE_ID;
    state->ui.edit_dirty = 1;

    /* Show banner */
    snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "NODE DELETED");
    state->ui.banner_timer = BANNER_TIMEOUT_SEC;
    state->ui.banner_error = 0;
}

static void cmd_duplicate_node(CmdPaletteContext *ctx)
{
    NodeId src_id, new_id;
    NodeType type;
    const Node *src;
    Node *dst;
    uint8_t i;
    EditorState *state;

    if (!ctx || !ctx->state) return;
    state = ctx->state;
    src_id = state->ui.selected_node;
    if (!cmd_has_selection(ctx)) return;

    src = &state->edit_graph.nodes[src_id];
    type = src->type;

    /* Allocate new node of same type */
    if (graph_alloc_node(&state->edit_graph, type, &new_id) != STATUS_OK) {
        snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "DUPLICATE FAIL: GRAPH FULL");
        state->ui.banner_timer = BANNER_TIMEOUT_SEC;
        state->ui.banner_error = 1;
        return;
    }

    dst = &state->edit_graph.nodes[new_id];

    /* Copy parameters (but not connections) */
    for (i = 0; i < MAX_PARAMS; i++) {
        dst->params[i] = src->params[i];
    }

    /* Position offset from source */
    state->ui_meta.meta[new_id].x = state->ui_meta.meta[src_id].x + 30.0f;
    state->ui_meta.meta[new_id].y = state->ui_meta.meta[src_id].y + 30.0f;

    /* Select the new node */
    state->ui.selected_node = new_id;
    state->ui.edit_dirty = 1;

    snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "NODE DUPLICATED");
    state->ui.banner_timer = BANNER_TIMEOUT_SEC;
    state->ui.banner_error = 0;
}

static void cmd_clear_selection(CmdPaletteContext *ctx)
{
    if (!ctx || !ctx->state) return;
    ctx->state->ui.selected_node = INVALID_NODE_ID;
}

/* ============================================================
 * cmd_commit: Uses CommitApi if available, else graph_publish directly
 * ============================================================ */
static void cmd_commit(CmdPaletteContext *ctx)
{
    EditorState *state;
    char err_buf[64];
    int ok;

    if (!ctx || !ctx->state) return;
    state = ctx->state;

    /* Use CommitApi callback if provided (preferred path) */
    if (ctx->commit_api && ctx->commit_api->publish_try_commit) {
        err_buf[0] = '\0';
        ok = ctx->commit_api->publish_try_commit(
            &state->edit_graph,
            ctx->active_graph,
            ctx->commit_api->plan,
            err_buf
        );
        if (ok) {
            snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "COMMIT OK");
            state->ui.edit_dirty = 0;
            state->ui.banner_error = 0;
        } else {
            snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "COMMIT FAIL: %s",
                     err_buf[0] ? err_buf : "UNKNOWN");
            state->ui.banner_error = 1;
        }
    } else {
        /* Fallback: use graph_publish directly (validation only, no active copy) */
        PublishResult result = graph_publish(&state->edit_graph, NULL, NULL);
        switch (result) {
            case PUBLISH_OK:
                state->commit_result = COMMIT_SUCCESS;
                snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "COMMIT OK");
                state->ui.edit_dirty = 0;
                state->ui.banner_error = 0;
                break;
            case PUBLISH_ERR_CYCLE:
                state->commit_result = COMMIT_FAIL_CYCLE;
                snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "COMMIT FAIL: CYCLE");
                state->ui.banner_error = 1;
                break;
            case PUBLISH_ERR_NO_SINK:
                state->commit_result = COMMIT_FAIL_NO_SINK;
                snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "COMMIT FAIL: NO SINK");
                state->ui.banner_error = 1;
                break;
            default:
                state->commit_result = COMMIT_FAIL_VALIDATION;
                snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "COMMIT FAIL: VALIDATION");
                state->ui.banner_error = 1;
                break;
        }
    }
    state->ui.banner_timer = BANNER_TIMEOUT_SEC;
}

/* ============================================================
 * cmd_revert: Copy active_graph back to edit_graph
 * ============================================================ */
static void cmd_revert(CmdPaletteContext *ctx)
{
    EditorState *state;

    if (!ctx || !ctx->state) return;
    state = ctx->state;

    /* Require active_graph to revert from */
    if (!ctx->active_graph) {
        snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "REVERT FAIL: NO ACTIVE GRAPH");
        state->ui.banner_timer = BANNER_TIMEOUT_SEC;
        state->ui.banner_error = 1;
        return;
    }

    /* Copy active graph to edit graph (same as editor Select/Revert behavior) */
    graph_copy(&state->edit_graph, ctx->active_graph);

    /* Clear selection and mark dirty to force UI refresh */
    state->ui.selected_node = INVALID_NODE_ID;
    state->ui.edit_dirty = 0;  /* Not dirty since we just synced with active */
    state->ui.mode = UI_EDITOR_MODE_NAV;

    snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "REVERTED TO ACTIVE");
    state->ui.banner_timer = BANNER_TIMEOUT_SEC;
    state->ui.banner_error = 0;
}

static void cmd_validate(CmdPaletteContext *ctx)
{
    EvalPlan plan;
    Status status;
    EditorState *state;

    if (!ctx || !ctx->state) return;
    state = ctx->state;

    status = graph_build_eval_plan(&state->edit_graph, &plan);

    if (status == STATUS_OK) {
        snprintf(state->ui.banner_text, sizeof(state->ui.banner_text),
                 "VALID: %d nodes", plan.count);
        state->ui.banner_error = 0;
    } else if (status == STATUS_ERR_CYCLE_DETECTED) {
        snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "INVALID: CYCLE DETECTED");
        state->ui.banner_error = 1;
    } else if (status == STATUS_ERR_NO_SINK) {
        snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "INVALID: NO SINK NODE");
        state->ui.banner_error = 1;
    } else {
        snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "INVALID: ERROR %d", (int)status);
        state->ui.banner_error = 1;
    }
    state->ui.banner_timer = BANNER_TIMEOUT_SEC;
}

static void cmd_save_graph(CmdPaletteContext *ctx)
{
    GraphIoResult result;
    EditorState *state;

    if (!ctx || !ctx->state) return;
    state = ctx->state;

    result = graph_io_save("host:graph.gph", &state->edit_graph, &state->ui_meta);

    if (result == GRAPH_IO_OK) {
        snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "SAVED: host:graph.gph");
        state->ui.banner_error = 0;
    } else {
        snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "SAVE FAIL: %s",
                 graph_io_result_str(result));
        state->ui.banner_error = 1;
    }
    state->ui.banner_timer = BANNER_TIMEOUT_SEC;
}

static void cmd_load_graph(CmdPaletteContext *ctx)
{
    GraphIoResult result;
    EditorState *state;

    if (!ctx || !ctx->state) return;
    state = ctx->state;

    result = graph_io_load("host:graph.gph", &state->edit_graph, &state->ui_meta);

    if (result == GRAPH_IO_OK) {
        snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "LOADED: host:graph.gph");
        state->ui.banner_error = 0;
        state->ui.selected_node = INVALID_NODE_ID;
        state->ui.edit_dirty = 1;
    } else {
        snprintf(state->ui.banner_text, sizeof(state->ui.banner_text), "LOAD FAIL: %s",
                 graph_io_result_str(result));
        state->ui.banner_error = 1;
    }
    state->ui.banner_timer = BANNER_TIMEOUT_SEC;
}

/* ============================================================
 * Palette API Implementation (context-based)
 * ============================================================ */
void cmd_palette_init(CommandPalette *palette)
{
    if (!palette) return;
    memset(palette, 0, sizeof(*palette));
    palette->is_open = 0;
    palette->selected_index = 0;
    palette->scroll_offset = 0;
    palette->filtered_count = 0;
}

void cmd_palette_open(CommandPalette *palette, const CmdPaletteContext *ctx)
{
    uint8_t i;

    if (!palette) return;

    /* Rebuild filtered list of enabled commands using context */
    palette->filtered_count = 0;
    for (i = 0; i < CMD_COUNT && palette->filtered_count < CMD_PALETTE_MAX_COMMANDS; i++) {
        if (s_commands[i].is_enabled(ctx)) {
            palette->filtered[palette->filtered_count++] = i;
        }
    }

    palette->selected_index = 0;
    palette->scroll_offset = 0;
    palette->is_open = 1;
}

void cmd_palette_close(CommandPalette *palette)
{
    if (!palette) return;
    palette->is_open = 0;
}

int cmd_palette_is_open(const CommandPalette *palette)
{
    if (!palette) return 0;
    return palette->is_open != 0;
}

/* Helper: check for button press */
static int cmd_btn_pressed(const PadState *now, const PadState *prev, uint16_t mask)
{
    uint16_t now_held = now ? now->held : 0;
    uint16_t prev_held = prev ? prev->held : 0;
    return ((now_held & mask) != 0) && ((prev_held & mask) == 0);
}

int cmd_palette_update(CommandPalette *palette,
                       CmdPaletteContext *ctx,
                       const PadState *now,
                       const PadState *prev)
{
    uint8_t cmd_idx;

    if (!palette || !palette->is_open) return 0;

    /* Circle or Triangle: close palette */
    if (cmd_btn_pressed(now, prev, BTN_CIRCLE) || cmd_btn_pressed(now, prev, BTN_TRIANGLE)) {
        cmd_palette_close(palette);
        return 0;
    }

    /* Up/Down: navigate */
    if (cmd_btn_pressed(now, prev, BTN_UP)) {
        if (palette->selected_index > 0) {
            palette->selected_index--;
            if (palette->selected_index < palette->scroll_offset) {
                palette->scroll_offset = palette->selected_index;
            }
        }
    }
    if (cmd_btn_pressed(now, prev, BTN_DOWN)) {
        if (palette->selected_index + 1 < palette->filtered_count) {
            palette->selected_index++;
            if (palette->selected_index >= palette->scroll_offset + CMD_PALETTE_MAX_VISIBLE) {
                palette->scroll_offset = palette->selected_index - CMD_PALETTE_MAX_VISIBLE + 1;
            }
        }
    }

    /* Cross: execute selected command */
    if (cmd_btn_pressed(now, prev, BTN_CROSS)) {
        if (palette->selected_index < palette->filtered_count) {
            cmd_idx = palette->filtered[palette->selected_index];
            if (cmd_idx < CMD_COUNT) {
                cmd_palette_close(palette);
                s_commands[cmd_idx].execute(ctx);
                return 1;
            }
        }
        cmd_palette_close(palette);
        return 0;
    }

    return 0;
}

void cmd_palette_draw(const CommandPalette *palette,
                      void (*rect_filled)(int x, int y, int w, int h, uint32_t color),
                      void (*draw_text)(int x, int y, uint32_t color, const char *text))
{
    int x, y, w, h;
    uint8_t i, visible_count, cmd_idx;
    int item_y;

    if (!palette || !palette->is_open) return;
    if (!rect_filled || !draw_text) return;

    /* Calculate dimensions */
    visible_count = palette->filtered_count;
    if (visible_count > CMD_PALETTE_MAX_VISIBLE) {
        visible_count = CMD_PALETTE_MAX_VISIBLE;
    }

    x = CMD_PALETTE_X;
    y = CMD_PALETTE_Y;
    w = CMD_PALETTE_WIDTH;
    h = CMD_PALETTE_HEADER_H + (visible_count * CMD_PALETTE_ITEM_HEIGHT) + (CMD_PALETTE_PAD * 2);

    /* Background */
    rect_filled(x, y, w, h, CMD_COLOR_BG);

    /* Header */
    rect_filled(x, y, w, CMD_PALETTE_HEADER_H, CMD_COLOR_BG_HEADER);
    draw_text(x + CMD_PALETTE_PAD + 2, y + 4, CMD_COLOR_TEXT, "Commands");

    /* Items */
    for (i = 0; i < visible_count; i++) {
        uint8_t list_idx = palette->scroll_offset + i;
        if (list_idx >= palette->filtered_count) break;

        cmd_idx = palette->filtered[list_idx];
        if (cmd_idx >= CMD_COUNT) continue;

        item_y = y + CMD_PALETTE_HEADER_H + CMD_PALETTE_PAD + (i * CMD_PALETTE_ITEM_HEIGHT);

        /* Highlight selected */
        if (list_idx == palette->selected_index) {
            rect_filled(x + 2, item_y, w - 4, CMD_PALETTE_ITEM_HEIGHT - 2, CMD_COLOR_ITEM_SEL);
        }

        draw_text(x + CMD_PALETTE_PAD + 4, item_y + 2, CMD_COLOR_TEXT, s_commands[cmd_idx].label);
    }

    /* Scroll indicators */
    if (palette->scroll_offset > 0) {
        draw_text(x + w - 16, y + CMD_PALETTE_HEADER_H + 2, CMD_COLOR_TEXT_DIM, "^");
    }
    if (palette->scroll_offset + visible_count < palette->filtered_count) {
        draw_text(x + w - 16, y + h - 14, CMD_COLOR_TEXT_DIM, "v");
    }
}

uint8_t cmd_palette_get_command_count(void)
{
    return (uint8_t)CMD_COUNT;
}

const CommandDef *cmd_palette_get_command(uint8_t index)
{
    if (index >= CMD_COUNT) return NULL;
    return &s_commands[index];
}
