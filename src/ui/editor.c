#include "editor.h"
#include "command_palette.h"
#include "../graph/graph_core.h"
#include "../graph/graph_publish.h"
#include "../nodes/node_registry.h"
#include "../runtime/runtime.h"
#include "../render/render.h"
#include "../render/font.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================
 * Colors (RGBA 0xAABBGGRR style not assumed; use simple RGBA)
 * ============================================================ */
#define UI_COLOR_BG           0x80202020u
#define UI_COLOR_NODE         0x80404080u
#define UI_COLOR_NODE_SEL     0x806060C0u
#define UI_COLOR_NODE_HDR     0x805050A0u
#define UI_COLOR_NODE_BORDER  0x80FFFFFFu
#define UI_COLOR_PORT_IN      0x80FF4040u
#define UI_COLOR_PORT_OUT     0x8040FF40u
#define UI_COLOR_WIRE         0x80C0C0C0u
#define UI_COLOR_WIRE_PREVIEW 0x80FFFF00u
#define UI_COLOR_CURSOR       0x80FFFF00u
#define UI_COLOR_HUD_BG       0x80303030u
#define UI_COLOR_TEXT         0x80FFFFFFu
#define UI_COLOR_BANNER_OK    0x8040FF40u
#define UI_COLOR_BANNER_ERR   0x80FF4040u
#define UI_COLOR_MENU_BG      0x80404040u
#define UI_COLOR_MENU_SEL     0x806060C0u

#define CURSOR_HALF 6

/* ============================================================
 * Command Palette State (static, editor-local)
 * ============================================================ */
static CommandPalette s_cmdpal;
static int s_cmdpal_initialized = 0;

/* ============================================================
 * Local Helpers
 * ============================================================ */
static float ui_frame_scale(float dt_sec)
{
    if (dt_sec <= 0.0f) {
        return 1.0f;
    }
    return dt_sec * 60.0f;
}

static int ui_btn_held(const PadState *now, uint16_t mask)
{
    if (!now) {
        return 0;
    }
    return (now->held & mask) != 0;
}

static int ui_btn_pressed(const PadState *now, const PadState *prev, uint16_t mask)
{
    uint16_t now_held = now ? now->held : 0;
    uint16_t prev_held = prev ? prev->held : 0;
    return ((now_held & mask) != 0) && ((prev_held & mask) == 0);
}

static const char *ui_node_type_to_string(NodeType type)
{
    switch (type) {
        case NODE_TYPE_NONE: return "NONE";
        case NODE_TYPE_CONST: return "CONST";
        case NODE_TYPE_TIME: return "TIME";
        case NODE_TYPE_PAD: return "PAD";
        case NODE_TYPE_NOISE: return "NOISE";
        case NODE_TYPE_LFO: return "LFO";
        case NODE_TYPE_ADD: return "ADD";
        case NODE_TYPE_MUL: return "MUL";
        case NODE_TYPE_SUB: return "SUB";
        case NODE_TYPE_DIV: return "DIV";
        case NODE_TYPE_MOD: return "MOD";
        case NODE_TYPE_ABS: return "ABS";
        case NODE_TYPE_NEG: return "NEG";
        case NODE_TYPE_MIN: return "MIN";
        case NODE_TYPE_MAX: return "MAX";
        case NODE_TYPE_CLAMP: return "CLAMP";
        case NODE_TYPE_MAP: return "MAP";
        case NODE_TYPE_SIN: return "SIN";
        case NODE_TYPE_COS: return "COS";
        case NODE_TYPE_TAN: return "TAN";
        case NODE_TYPE_ATAN2: return "ATAN2";
        case NODE_TYPE_LERP: return "LERP";
        case NODE_TYPE_SMOOTH: return "SMOOTH";
        case NODE_TYPE_STEP: return "STEP";
        case NODE_TYPE_PULSE: return "PULSE";
        case NODE_TYPE_HOLD: return "HOLD";
        case NODE_TYPE_DELAY: return "DELAY";
        case NODE_TYPE_COMPARE: return "COMPARE";
        case NODE_TYPE_SELECT: return "SELECT";
        case NODE_TYPE_GATE: return "GATE";
        case NODE_TYPE_SPLIT: return "SPLIT";
        case NODE_TYPE_COMBINE: return "COMBINE";
        case NODE_TYPE_COLORIZE: return "COLORIZE";
        case NODE_TYPE_HSV: return "HSV";
        case NODE_TYPE_GRADIENT: return "GRADIENT";
        case NODE_TYPE_TRANSFORM2D: return "TRANSFORM2D";
        case NODE_TYPE_RENDER2D: return "RENDER2D";
        case NODE_TYPE_RENDER_CIRCLE: return "RENDER_CIRCLE";
        case NODE_TYPE_RENDER_LINE: return "RENDER_LINE";
        case NODE_TYPE_DEBUG: return "DEBUG";
        default: return "NODE";
    }
}

static int ui_node_valid(const Graph *g, NodeId id)
{
    if (!g) {
        return 0;
    }
    if (id == INVALID_NODE_ID || id >= MAX_NODES) {
        return 0;
    }
    return g->nodes[id].type != NODE_TYPE_NONE;
}

static void ui_node_screen_pos(const UiEditor *ui, const GraphUi *edit_ui,
                               NodeId id, int *out_x, int *out_y)
{
    if (!ui || !edit_ui || !out_x || !out_y || id >= MAX_NODES) {
        return;
    }
    float sx = edit_ui->meta[id].x - ui->pan_x + (SCREEN_W / 2.0f);
    float sy = edit_ui->meta[id].y - ui->pan_y + (CANVAS_Y1 / 2.0f);
    *out_x = (int)(sx + 0.5f);
    *out_y = (int)(sy + 0.5f);
}

static void ui_port_center(const UiEditor *ui, const GraphUi *edit_ui,
                           NodeId id, int is_output, uint8_t port,
                           int *out_x, int *out_y)
{
    int node_x = 0;
    int node_y = 0;
    ui_node_screen_pos(ui, edit_ui, id, &node_x, &node_y);

    if (out_x) {
        *out_x = is_output ? (node_x + NODE_W) : node_x;
    }
    if (out_y) {
        *out_y = node_y + PORT_TOP_Y + (int)port * PORT_GAP_Y;
    }
}

static NodeId ui_node_hit_test(const UiEditor *ui, const Graph *g, const GraphUi *edit_ui)
{
    int i;
    if (!ui || !g || !edit_ui) {
        return INVALID_NODE_ID;
    }

    for (i = MAX_NODES - 1; i >= 0; --i) {
        NodeId id = (NodeId)i;
        int node_x = 0;
        int node_y = 0;
        if (!ui_node_valid(g, id)) {
            continue;
        }
        ui_node_screen_pos(ui, edit_ui, id, &node_x, &node_y);
        if (ui->cursor_x >= node_x && ui->cursor_x < node_x + NODE_W &&
            ui->cursor_y >= node_y && ui->cursor_y < node_y + NODE_H) {
            return id;
        }
    }

    return INVALID_NODE_ID;
}

static int ui_port_hit_test(const UiEditor *ui, const Graph *g, const GraphUi *edit_ui,
                            NodeId *out_node, int *out_is_output, uint8_t *out_port)
{
    int i;
    if (!ui || !g || !edit_ui) {
        return 0;
    }

    for (i = MAX_NODES - 1; i >= 0; --i) {
        NodeId id = (NodeId)i;
        const NodeMeta *meta;
        int p;

        if (!ui_node_valid(g, id)) {
            continue;
        }

        meta = node_registry_get_meta(g->nodes[id].type);
        if (!meta) {
            continue;
        }

        for (p = (int)meta->num_inputs - 1; p >= 0; --p) {
            int px = 0;
            int py = 0;
            int dx;
            int dy;
            ui_port_center(ui, edit_ui, id, 0, (uint8_t)p, &px, &py);
            dx = (int)(ui->cursor_x - px);
            dy = (int)(ui->cursor_y - py);
            if (dx * dx + dy * dy <= PORT_HIT_R * PORT_HIT_R) {
                if (out_node) {
                    *out_node = id;
                }
                if (out_is_output) {
                    *out_is_output = 0;
                }
                if (out_port) {
                    *out_port = (uint8_t)p;
                }
                return 1;
            }
        }

        for (p = (int)meta->num_outputs - 1; p >= 0; --p) {
            int px = 0;
            int py = 0;
            int dx;
            int dy;
            ui_port_center(ui, edit_ui, id, 1, (uint8_t)p, &px, &py);
            dx = (int)(ui->cursor_x - px);
            dy = (int)(ui->cursor_y - py);
            if (dx * dx + dy * dy <= PORT_HIT_R * PORT_HIT_R) {
                if (out_node) {
                    *out_node = id;
                }
                if (out_is_output) {
                    *out_is_output = 1;
                }
                if (out_port) {
                    *out_port = (uint8_t)p;
                }
                return 1;
            }
        }
    }

    return 0;
}

static float ui_get_param_step(const PadState *now)
{
    if (ui_btn_held(now, BTN_L1)) {
        return PARAM_STEP_FINE;
    }
    if (ui_btn_held(now, BTN_R1)) {
        return PARAM_STEP_COARSE;
    }
    return PARAM_STEP_NORMAL;
}

/* ============================================================
 * Compatibility Adapter (legacy EditorState API)
 * ============================================================ */
static void editor_pad_from_runtime(const RuntimeContext *ctx, PadState *out_now, PadState *out_prev)
{
    uint16_t held = 0;
    uint16_t pressed = 0;

    if (!ctx || !out_now || !out_prev) {
        return;
    }

    held = ctx->buttons_held;
    pressed = ctx->buttons_pressed;

    memset(out_now, 0, sizeof(*out_now));
    memset(out_prev, 0, sizeof(*out_prev));

    out_now->lx = ctx->pad_lx;
    out_now->ly = ctx->pad_ly;
    out_now->rx = ctx->pad_rx;
    out_now->ry = ctx->pad_ry;
    out_now->l2 = ctx->pad_l2;
    out_now->r2 = ctx->pad_r2;
    out_now->held = held;
    out_now->pressed = pressed;
    out_now->released = ctx->buttons_released;
    out_now->connected = 1;

    out_prev->held = (uint16_t)(held & (uint16_t)(~pressed));
    out_prev->connected = 1;
}

static int editor_commit_adapter(Graph *edit, const Graph *active, void *plan, void *err)
{
    EditorState *state = (EditorState *)plan;
    char *err_buf = (char *)err;
    PublishResult result;
    Graph *active_mut = (Graph *)active;

    result = graph_publish(edit, active_mut, NULL);

    if (state) {
        switch (result) {
            case PUBLISH_OK:
                state->commit_result = COMMIT_SUCCESS;
                break;
            case PUBLISH_ERR_CYCLE:
                state->commit_result = COMMIT_FAIL_CYCLE;
                break;
            case PUBLISH_ERR_NO_SINK:
                state->commit_result = COMMIT_FAIL_NO_SINK;
                break;
            default:
                state->commit_result = COMMIT_FAIL_VALIDATION;
                break;
        }
    }

    if (err_buf) {
        const char *msg = graph_publish_result_str(result);
        snprintf(err_buf, 63, "%s", msg ? msg : "UNKNOWN");
        err_buf[63] = '\0';
    }

    if (result == PUBLISH_OK) {
        if (edit && active_mut) {
            edit->version = active_mut->version;
        }
        return 1;
    }
    return 0;
}

static void editor_render_rect_filled(int x, int y, int w, int h, uint32_t color)
{
    render_rect_screen(x, y, w, h, (uint64_t)color);
}

static void editor_render_rect_outline(int x, int y, int w, int h, uint32_t color)
{
    render_rect_outline_screen(x, y, w, h, (uint64_t)color);
}

static void editor_render_line(int x1, int y1, int x2, int y2, uint32_t color)
{
    render_line_screen(x1, y1, x2, y2, (uint64_t)color);
}

static void editor_font_draw_text(int x, int y, uint32_t color, const char *text)
{
    font_draw_string_screen(text, x, y, (uint64_t)color, 1);
}

/* ============================================================
 * Initialization
 * ============================================================ */
void ui_editor_init(UiEditor *ui)
{
    if (!ui) {
        return;
    }
    memset(ui, 0, sizeof(*ui));
    ui->mode = UI_EDITOR_MODE_NAV;
    ui->cursor_x = SCREEN_W / 2.0f;
    ui->cursor_y = CANVAS_Y1 / 2.0f;
    ui->pan_x = 0.0f;
    ui->pan_y = 0.0f;
    ui->selected_node = INVALID_NODE_ID;
    ui->wire_src_node = INVALID_NODE_ID;
    ui->banner_text[0] = '\0';
}

/* ============================================================
 * Update
 * ============================================================ */
void ui_editor_update(
    UiEditor *ui,
    const PadState *now,
    const PadState *prev,
    Graph *edit,
    GraphUi *edit_ui,
    const Graph *active,
    const CommitApi *commit_api,
    float dt_sec)
{
    float frame_scale;
    float speed;
    int move_pan;

    if (!ui || !now || !edit || !edit_ui) {
        return;
    }

    frame_scale = ui_frame_scale(dt_sec);

    if (ui->banner_timer > 0.0f) {
        ui->banner_timer -= dt_sec;
        if (ui->banner_timer <= 0.0f) {
            ui->banner_timer = 0.0f;
            ui->banner_error = 0;
            ui->banner_text[0] = '\0';
        }
    }

    if (ui_btn_pressed(now, prev, BTN_START) && commit_api && commit_api->publish_try_commit) {
        int ok = commit_api->publish_try_commit(edit, active, commit_api->plan, commit_api->err);
        if (ok) {
            snprintf(ui->banner_text, sizeof(ui->banner_text), "COMMIT OK");
            ui->edit_dirty = 0;
            ui->banner_error = 0;
        } else {
            const char *reason = commit_api->err ? (const char *)commit_api->err : "UNKNOWN";
            snprintf(ui->banner_text, sizeof(ui->banner_text), "COMMIT FAIL: %s", reason);
            ui->banner_error = 1;
        }
        ui->banner_timer = BANNER_TIMEOUT_SEC;
    }

    speed = CURSOR_SPEED_BASE;
    if (ui_btn_held(now, BTN_L1)) {
        speed = CURSOR_SPEED_FINE;
    } else if (ui_btn_held(now, BTN_R1)) {
        speed = CURSOR_SPEED_COARSE;
    }

    move_pan = ui_btn_held(now, BTN_L2);

    if (move_pan) {
        ui->pan_x += now->lx * speed * frame_scale;
        ui->pan_y += now->ly * speed * frame_scale;
    } else {
        ui->cursor_x += now->lx * speed * frame_scale;
        ui->cursor_y += now->ly * speed * frame_scale;
    }

    if (ui->mode == UI_EDITOR_MODE_NAV || ui->mode == UI_EDITOR_MODE_WIRE) {
        /* D-Pad moves cursor */
        if (ui_btn_pressed(now, prev, BTN_LEFT)) {
            ui->cursor_x -= DPAD_STEP;
        }
        if (ui_btn_pressed(now, prev, BTN_RIGHT)) {
            ui->cursor_x += DPAD_STEP;
        }
        if (ui_btn_pressed(now, prev, BTN_UP)) {
            ui->cursor_y -= DPAD_STEP;
        }
        if (ui_btn_pressed(now, prev, BTN_DOWN)) {
            ui->cursor_y += DPAD_STEP;
        }
    }

    if (ui->cursor_x < 0.0f) {
        ui->cursor_x = 0.0f;
    }
    if (ui->cursor_x > (float)(SCREEN_W - 1)) {
        ui->cursor_x = (float)(SCREEN_W - 1);
    }
    if (ui->cursor_y < 0.0f) {
        ui->cursor_y = 0.0f;
    }
    if (ui->cursor_y > (float)CANVAS_Y1) {
        ui->cursor_y = (float)CANVAS_Y1;
    }

    if (ui_btn_pressed(now, prev, BTN_CIRCLE)) {
        ui->mode = UI_EDITOR_MODE_NAV;
        ui->wire_src_node = INVALID_NODE_ID;
    }

    if (ui_btn_pressed(now, prev, BTN_SQUARE)) {
        if (ui->mode == UI_EDITOR_MODE_WIRE) {
            ui->mode = UI_EDITOR_MODE_NAV;
            ui->wire_src_node = INVALID_NODE_ID;
        } else if (ui->mode == UI_EDITOR_MODE_PARAM) {
            ui->mode = UI_EDITOR_MODE_NAV;
        } else {
            ui->mode = UI_EDITOR_MODE_WIRE;
            ui->wire_src_node = INVALID_NODE_ID;
        }
    }

    if (ui_btn_pressed(now, prev, BTN_TRIANGLE)) {
        /* Triangle: Enter param mode if node selected, otherwise open add menu */
        if (ui->mode == UI_EDITOR_MODE_NAV && ui_node_valid(edit, ui->selected_node)) {
            const NodeMeta *meta = node_registry_get_meta(edit->nodes[ui->selected_node].type);
            if (meta && meta->num_params > 0) {
                ui->mode = UI_EDITOR_MODE_PARAM;
                ui->selected_param = 0;
            } else {
                /* Node has no params, open add menu instead */
                ui->mode = UI_EDITOR_MODE_ADD;
                ui->add_index = 0;
                ui->add_scroll = 0;
            }
        } else if (ui->mode == UI_EDITOR_MODE_PARAM) {
            /* Exit param mode */
            ui->mode = UI_EDITOR_MODE_NAV;
        } else {
            ui->mode = UI_EDITOR_MODE_ADD;
            ui->add_index = 0;
            ui->add_scroll = 0;
        }
    }

    if (ui->mode == UI_EDITOR_MODE_NAV) {
        if (ui_btn_pressed(now, prev, BTN_CROSS)) {
            NodeId hit = ui_node_hit_test(ui, edit, edit_ui);
            if (hit != INVALID_NODE_ID) {
                ui->selected_node = hit;
            }
        }
    }

    if (ui->mode == UI_EDITOR_MODE_WIRE) {
        if (ui_btn_pressed(now, prev, BTN_CROSS)) {
            NodeId port_node = INVALID_NODE_ID;
            int is_output = 0;
            uint8_t port_idx = 0;
            if (ui_port_hit_test(ui, edit, edit_ui, &port_node, &is_output, &port_idx)) {
                if (ui->wire_src_node == INVALID_NODE_ID) {
                    if (is_output) {
                        ui->wire_src_node = port_node;
                        ui->wire_src_port = port_idx;
                        ui->selected_node = port_node;
                    }
                } else {
                    if (!is_output) {
                        const NodeMeta *meta_dst = node_registry_get_meta(edit->nodes[port_node].type);
                        const NodeMeta *meta_src = node_registry_get_meta(edit->nodes[ui->wire_src_node].type);
                        if (meta_dst && meta_src &&
                            port_idx < meta_dst->num_inputs &&
                            ui->wire_src_port < meta_src->num_outputs) {
                            graph_connect(edit, ui->wire_src_node, ui->wire_src_port, port_node, port_idx);
                            ui->edit_dirty = 1;
                        }
                        ui->wire_src_node = INVALID_NODE_ID;
                    }
                }
            }
        }
    }

    if (ui->mode == UI_EDITOR_MODE_ADD) {
        uint8_t total_types = (uint8_t)(NODE_TYPE_COUNT - 1);
        int visible = (220 - 16) / 14;
        if (ui_btn_pressed(now, prev, BTN_UP)) {
            if (ui->add_index > 0) {
                ui->add_index--;
            }
        }
        if (ui_btn_pressed(now, prev, BTN_DOWN)) {
            if (ui->add_index + 1 < total_types) {
                ui->add_index++;
            }
        }

        if (ui->add_index < ui->add_scroll) {
            ui->add_scroll = ui->add_index;
        } else if (ui->add_index >= (uint8_t)(ui->add_scroll + visible)) {
            ui->add_scroll = ui->add_index - (uint8_t)(visible - 1);
        }

        if (ui_btn_pressed(now, prev, BTN_CROSS)) {
            NodeId new_id = INVALID_NODE_ID;
            NodeType type = (NodeType)(ui->add_index + 1);
            if (graph_alloc_node(edit, type, &new_id) == STATUS_OK) {
                float gx = (ui->cursor_x - (SCREEN_W / 2.0f)) + ui->pan_x;
                float gy = (ui->cursor_y - (CANVAS_Y1 / 2.0f)) + ui->pan_y;
                edit_ui->meta[new_id].x = gx;
                edit_ui->meta[new_id].y = gy;
                ui->selected_node = new_id;
                ui->edit_dirty = 1;
            }
            ui->mode = UI_EDITOR_MODE_NAV;
        }
    }

    if (ui->mode == UI_EDITOR_MODE_PARAM) {
        if (!ui_node_valid(edit, ui->selected_node)) {
            ui->mode = UI_EDITOR_MODE_NAV;
        } else {
            const NodeMeta *meta = node_registry_get_meta(edit->nodes[ui->selected_node].type);
            if (!meta || meta->num_params == 0) {
                ui->mode = UI_EDITOR_MODE_NAV;
            } else {
                if (ui_btn_pressed(now, prev, BTN_UP) && ui->selected_param > 0) {
                    ui->selected_param--;
                }
                if (ui_btn_pressed(now, prev, BTN_DOWN) && ui->selected_param + 1 < meta->num_params) {
                    ui->selected_param++;
                }
                if (ui_btn_pressed(now, prev, BTN_LEFT) || ui_btn_pressed(now, prev, BTN_RIGHT)) {
                    float current = 0.0f;
                    float step = ui_get_param_step(now);
                    graph_get_param(edit, ui->selected_node, ui->selected_param, &current);
                    if (ui_btn_pressed(now, prev, BTN_LEFT)) {
                        current -= step;
                    }
                    if (ui_btn_pressed(now, prev, BTN_RIGHT)) {
                        current += step;
                    }
                    graph_set_param(edit, ui->selected_node, ui->selected_param, current);
                    ui->edit_dirty = 1;
                }
            }
        }
    }
}

/* ============================================================
 * Draw
 * ============================================================ */
void ui_editor_draw(
    const UiEditor *ui,
    const Graph *edit,
    const GraphUi *edit_ui,
    const Graph *active,
    const GraphUi *active_ui,
    const RenderApi *r,
    const FontApi *f)
{
    int i;

    (void)active;
    (void)active_ui;

    if (!ui || !edit || !edit_ui || !r || !f ||
        !r->rect_filled || !r->rect_outline || !r->line || !f->draw_text) {
        return;
    }

    r->rect_filled(0, 0, SCREEN_W, CANVAS_Y1 + 1, UI_COLOR_BG);

    for (i = 0; i < MAX_NODES; ++i) {
        NodeId dst = (NodeId)i;
        const Node *dst_node;
        const NodeMeta *meta_dst;
        int in_port;

        if (!ui_node_valid(edit, dst)) {
            continue;
        }

        dst_node = &edit->nodes[dst];
        meta_dst = node_registry_get_meta(dst_node->type);
        if (!meta_dst) {
            continue;
        }

        for (in_port = 0; in_port < (int)meta_dst->num_inputs; ++in_port) {
            const Connection *conn = &dst_node->inputs[in_port];
            NodeId src = conn->src_node;
            uint8_t src_port = conn->src_port;
            int sx, sy, dx, dy;
            int mx;
            if (!ui_node_valid(edit, src)) {
                continue;
            }

            ui_port_center(ui, edit_ui, src, 1, src_port, &sx, &sy);
            ui_port_center(ui, edit_ui, dst, 0, (uint8_t)in_port, &dx, &dy);
            mx = (sx + dx) / 2;
            r->line(sx, sy, mx, sy, UI_COLOR_WIRE);
            r->line(mx, sy, mx, dy, UI_COLOR_WIRE);
            r->line(mx, dy, dx, dy, UI_COLOR_WIRE);
        }
    }

    for (i = 0; i < MAX_NODES; ++i) {
        NodeId id = (NodeId)i;
        int node_x, node_y;
        int p;
        const NodeMeta *meta;
        uint32_t fill = UI_COLOR_NODE;
        const char *name;

        if (!ui_node_valid(edit, id)) {
            continue;
        }

        ui_node_screen_pos(ui, edit_ui, id, &node_x, &node_y);
        if (id == ui->selected_node) {
            fill = UI_COLOR_NODE_SEL;
        }
        r->rect_filled(node_x, node_y, NODE_W, NODE_H, fill);
        r->rect_outline(node_x, node_y, NODE_W, NODE_H, UI_COLOR_NODE_BORDER);
        r->rect_filled(node_x, node_y, NODE_W, NODE_HEADER_H, UI_COLOR_NODE_HDR);

        name = ui_node_type_to_string(edit->nodes[id].type);
        f->draw_text(node_x + NODE_PAD_X, node_y + NODE_PAD_Y, UI_COLOR_TEXT, name);

        meta = node_registry_get_meta(edit->nodes[id].type);
        if (!meta) {
            continue;
        }

        for (p = 0; p < (int)meta->num_inputs; ++p) {
            int px, py;
            ui_port_center(ui, edit_ui, id, 0, (uint8_t)p, &px, &py);
            r->rect_filled(px - PORT_R, py - PORT_R, PORT_R * 2, PORT_R * 2, UI_COLOR_PORT_IN);
        }

        for (p = 0; p < (int)meta->num_outputs; ++p) {
            int px, py;
            ui_port_center(ui, edit_ui, id, 1, (uint8_t)p, &px, &py);
            r->rect_filled(px - PORT_R, py - PORT_R, PORT_R * 2, PORT_R * 2, UI_COLOR_PORT_OUT);
        }
    }

    if (ui->mode == UI_EDITOR_MODE_WIRE && ui->wire_src_node != INVALID_NODE_ID) {
        int sx, sy;
        ui_port_center(ui, edit_ui, ui->wire_src_node, 1, ui->wire_src_port, &sx, &sy);
        r->line(sx, sy, (int)ui->cursor_x, (int)ui->cursor_y, UI_COLOR_WIRE_PREVIEW);
    }

    if (ui->mode == UI_EDITOR_MODE_ADD) {
        int panel_x = UI_MARGIN_X;
        int panel_y = UI_MARGIN_Y;
        int panel_w = 240;
        int panel_h = 220;
        int line_h = 14;
        int visible = (panel_h - 16) / line_h;
        int start = ui->add_scroll;
        int total = (int)(NODE_TYPE_COUNT - 1);
        int idx;

        if (ui->add_index < ui->add_scroll) {
            start = ui->add_index;
        } else if (ui->add_index >= (uint8_t)(ui->add_scroll + visible)) {
            start = ui->add_index - (visible - 1);
        }

        r->rect_filled(panel_x, panel_y, panel_w, panel_h, UI_COLOR_MENU_BG);
        r->rect_outline(panel_x, panel_y, panel_w, panel_h, UI_COLOR_NODE_BORDER);
        f->draw_text(panel_x + 8, panel_y + 6, UI_COLOR_TEXT, "ADD NODE");

        for (idx = 0; idx < visible; ++idx) {
            int type_index = start + idx;
            int y = panel_y + 20 + idx * line_h;
            const char *name;
            if (type_index >= total) {
                break;
            }
            name = ui_node_type_to_string((NodeType)(type_index + 1));
            if ((uint8_t)type_index == ui->add_index) {
                r->rect_filled(panel_x + 4, y - 2, panel_w - 8, line_h, UI_COLOR_MENU_SEL);
            }
            f->draw_text(panel_x + 8, y, UI_COLOR_TEXT, name);
        }
    }

    r->rect_filled((int)ui->cursor_x - CURSOR_HALF, (int)ui->cursor_y, CURSOR_HALF * 2, 1, UI_COLOR_CURSOR);
    r->rect_filled((int)ui->cursor_x, (int)ui->cursor_y - CURSOR_HALF, 1, CURSOR_HALF * 2, UI_COLOR_CURSOR);

    if (ui->banner_timer > 0.0f && ui->banner_text[0] != '\0') {
        uint32_t banner_color = ui->banner_error ? UI_COLOR_BANNER_ERR : UI_COLOR_BANNER_OK;
        r->rect_filled(UI_MARGIN_X, BANNER_Y, SCREEN_W - UI_MARGIN_X * 2, BANNER_H, banner_color);
        f->draw_text(UI_MARGIN_X + 8, BANNER_Y + 8, UI_COLOR_TEXT, ui->banner_text);
    }

    r->rect_filled(0, HUD_Y0, SCREEN_W, SCREEN_H - HUD_Y0, UI_COLOR_HUD_BG);

    {
        char line1[128];
        char line2[128];
        const char *mode_str = "NAV";
        const char *status_str = "LIVE";
        char sel_buf[64];

        if (ui->mode == UI_EDITOR_MODE_WIRE) {
            mode_str = "WIRE";
        } else if (ui->mode == UI_EDITOR_MODE_PARAM) {
            mode_str = "PARAM";
        } else if (ui->mode == UI_EDITOR_MODE_ADD) {
            mode_str = "ADD";
        }

        if (ui->banner_error && ui->banner_timer > 0.0f) {
            status_str = "ERROR";
        } else if (ui->edit_dirty) {
            status_str = "PENDING";
        }

        if (ui_node_valid(edit, ui->selected_node)) {
            snprintf(sel_buf, sizeof(sel_buf), "#%u %s", (unsigned)ui->selected_node,
                     ui_node_type_to_string(edit->nodes[ui->selected_node].type));
        } else {
            snprintf(sel_buf, sizeof(sel_buf), "NONE");
        }

        snprintf(line1, sizeof(line1), "MODE %s  SEL %s  %s", mode_str, sel_buf, status_str);
        f->draw_text(UI_MARGIN_X, 404, UI_COLOR_TEXT, line1);

        if (ui->mode == UI_EDITOR_MODE_PARAM && ui_node_valid(edit, ui->selected_node)) {
            float val = 0.0f;
            graph_get_param(edit, ui->selected_node, ui->selected_param, &val);
            snprintf(line2, sizeof(line2), "Param %u = %.3f  (L/R adjust)",
                     (unsigned)ui->selected_param, (double)val);
        } else {
            snprintf(line2, sizeof(line2), "X select  O back  Square wire  Triangle add  Start commit");
        }
        f->draw_text(UI_MARGIN_X, 420, UI_COLOR_TEXT, line2);
    }
}

/* ============================================================
 * Legacy Editor API
 * ============================================================ */
void editor_init(EditorState *state, const Graph *live_graph)
{
    int col = 0;
    int row = 0;

    if (!state) {
        return;
    }

    memset(state, 0, sizeof(*state));
    ui_editor_init(&state->ui);
    state->commit_result = COMMIT_NONE;

    /* Command Palette: initialize once */
    if (!s_cmdpal_initialized) {
        cmd_palette_init(&s_cmdpal);
        s_cmdpal_initialized = 1;
    }

    if (live_graph) {
        graph_copy(&state->edit_graph, live_graph);
    } else {
        graph_init(&state->edit_graph);
    }

    memset(&state->ui_meta, 0, sizeof(state->ui_meta));

    for (NodeId i = 0; i < MAX_NODES; ++i) {
        if (state->edit_graph.nodes[i].type != NODE_TYPE_NONE) {
            state->ui_meta.meta[i].x = (float)(-200 + col * 220);
            state->ui_meta.meta[i].y = (float)(-100 + row * 140);
            col++;
            if (col >= 3) {
                col = 0;
                row++;
            }
        }
    }
}

Status editor_update(EditorState *state, const RuntimeContext *ctx, Graph *live_graph)
{
    PadState now;
    PadState prev;
    CommitApi commit_api;
    char err_buf[64];

    if (!state || !ctx) {
        return STATUS_ERR_INVALID_NODE;
    }

    editor_pad_from_runtime(ctx, &now, &prev);

    commit_api.publish_try_commit = editor_commit_adapter;
    commit_api.plan = state;
    commit_api.err = err_buf;
    err_buf[0] = '\0';

    state->commit_result = COMMIT_NONE;

    /* Command Palette: toggle on R2 + Triangle in NAV mode only */
    if (state->ui.mode == UI_EDITOR_MODE_NAV &&
        ui_btn_held(&now, BTN_R2) &&
        ui_btn_pressed(&now, &prev, BTN_TRIANGLE)) {
        if (cmd_palette_is_open(&s_cmdpal)) {
            cmd_palette_close(&s_cmdpal);
        } else {
            /* Build context for palette open */
            CmdPaletteContext pal_ctx;
            pal_ctx.state = state;
            pal_ctx.active_graph = live_graph;
            pal_ctx.commit_api = &commit_api;
            cmd_palette_open(&s_cmdpal, &pal_ctx);
        }
    }

    /* Command Palette: suppress editor input when open */
    if (cmd_palette_is_open(&s_cmdpal)) {
        CmdPaletteContext pal_ctx;
        pal_ctx.state = state;
        pal_ctx.active_graph = live_graph;
        pal_ctx.commit_api = &commit_api;

        /* Keep banner timer aging even while palette open */
        if (state->ui.banner_timer > 0.0f) {
            state->ui.banner_timer -= ctx->dt;
            if (state->ui.banner_timer <= 0.0f) {
                state->ui.banner_timer = 0.0f;
                state->ui.banner_error = 0;
                state->ui.banner_text[0] = '\0';
            }
        }

        cmd_palette_update(&s_cmdpal, &pal_ctx, &now, &prev);
        return STATUS_OK;
    }

    ui_editor_update(&state->ui,
                     &now,
                     &prev,
                     &state->edit_graph,
                     &state->ui_meta,
                     live_graph,
                     &commit_api,
                     ctx->dt);

    return STATUS_OK;
}

void editor_draw(const EditorState *state)
{
    RenderApi r;
    FontApi f;

    if (!state) {
        return;
    }

    r.rect_filled = editor_render_rect_filled;
    r.rect_outline = editor_render_rect_outline;
    r.line = editor_render_line;

    f.draw_text = editor_font_draw_text;

    ui_editor_draw(&state->ui,
                   &state->edit_graph,
                   &state->ui_meta,
                   NULL,
                   NULL,
                   &r,
                   &f);

    /* Command Palette: render overlay */
    if (cmd_palette_is_open(&s_cmdpal)) {
        cmd_palette_draw(&s_cmdpal, editor_render_rect_filled, editor_font_draw_text);
    }
}
