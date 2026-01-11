#ifndef UI_EDITOR_H
#define UI_EDITOR_H

#include "../common.h"
#include "../graph/graph_types.h"
#include "../system/pad.h"
#include "../runtime/runtime.h"
#include <stdint.h>

/* ============================================================
 * Fixed UI Constants (locked)
 * ============================================================ */
#define SCREEN_W 640
#define SCREEN_H 480

/* Editor overlay mode - draws on top of preview */
#define EDITOR_Y0      0        /* Editor starts at top */
#define EDITOR_HEIGHT  480      /* Full screen height */
#define PREVIEW_HEIGHT 480      /* Preview is full screen */

#define CANVAS_Y1 399           /* Node canvas area */
#define HUD_Y0    400           /* HUD at bottom */

#define UI_MARGIN_X 16
#define UI_MARGIN_Y 16

#define NODE_W 85
#define NODE_H 46
#define NODE_HEADER_H 10
#define NODE_PAD_X 3
#define NODE_PAD_Y 2

#define PORT_R 2
#define PORT_HIT_R 6
#define PORT_GAP_Y 9
#define PORT_TOP_Y (NODE_HEADER_H + 7)

#define CURSOR_SPEED_BASE 3.2f
#define CURSOR_SPEED_FINE 1.2f
#define CURSOR_SPEED_COARSE 6.0f

#define DPAD_STEP 1

#define PARAM_STEP_FINE 0.01f
#define PARAM_STEP_NORMAL 0.05f
#define PARAM_STEP_COARSE 0.10f

#define BANNER_H 28
#define BANNER_Y 12
#define BANNER_TIMEOUT_SEC 2.0f

/* ============================================================
 * Optional API Adapters (define macros to override)
 * ============================================================ */
#ifndef UI_EDITOR_RENDER_API_DEFINED
#define UI_EDITOR_RENDER_API_DEFINED
/* Render API: screen-space primitives */
typedef struct RenderApi {
    void (*rect_filled)(int x, int y, int w, int h, uint32_t color);
    void (*rect_outline)(int x, int y, int w, int h, uint32_t color);
    void (*line)(int x1, int y1, int x2, int y2, uint32_t color);
} RenderApi;
#endif

#ifndef UI_EDITOR_FONT_API_DEFINED
#define UI_EDITOR_FONT_API_DEFINED
/* Font API: screen-space text */
typedef struct FontApi {
    void (*draw_text)(int x, int y, uint32_t color, const char *text);
} FontApi;
#endif

#ifndef UI_EDITOR_COMMIT_API_DEFINED
#define UI_EDITOR_COMMIT_API_DEFINED
/* Commit API: publish attempt */
typedef struct CommitApi {
    int (*publish_try_commit)(Graph *edit, const Graph *active, void *plan, void *err);
    void *plan;
    void *err;
} CommitApi;
#endif

/* ============================================================
 * GraphUi Adapter
 * ============================================================ */
typedef UiMetaBank GraphUi;

/* ============================================================
 * UI Modes
 * ============================================================ */
typedef enum {
    UI_EDITOR_MODE_NAV = 0,
    UI_EDITOR_MODE_WIRE,
    UI_EDITOR_MODE_PARAM,
    UI_EDITOR_MODE_ADD
} UiEditorMode;

/* ============================================================
 * UI State
 * ============================================================ */
typedef struct UiEditor {
    UiEditorMode mode;

    float cursor_x;
    float cursor_y;

    float pan_x;
    float pan_y;

    NodeId selected_node;
    uint8_t selected_param;

    NodeId wire_src_node;
    uint8_t wire_src_port;

    uint8_t add_index;
    uint8_t add_scroll;

    int edit_dirty;

    float banner_timer;
    int banner_error;
    char banner_text[64];
} UiEditor;

/* ============================================================
 * Legacy Editor API (compatibility layer)
 * ============================================================ */
typedef enum {
    COMMIT_NONE = 0,
    COMMIT_SUCCESS,
    COMMIT_FAIL_CYCLE,
    COMMIT_FAIL_NO_SINK,
    COMMIT_FAIL_VALIDATION
} CommitResult;

typedef struct EditorState {
    UiEditor ui;
    Graph edit_graph;
    UiMetaBank ui_meta;
    CommitResult commit_result;
} EditorState;

/* ============================================================
 * Public API
 * ============================================================ */
void ui_editor_init(UiEditor *ui);

void ui_editor_update(
    UiEditor *ui,
    const PadState *now,
    const PadState *prev,
    Graph *edit,
    GraphUi *edit_ui,
    const Graph *active,
    const CommitApi *commit_api,
    float dt_sec
);

void ui_editor_draw(
    const UiEditor *ui,
    const Graph *edit,
    const GraphUi *edit_ui,
    const Graph *active,
    const GraphUi *active_ui,
    const RenderApi *r,
    const FontApi *f
);

/* Legacy entry points expected by existing main.c */
void editor_init(EditorState *state, const Graph *live_graph);
Status editor_update(EditorState *state, const RuntimeContext *ctx, Graph *live_graph);
void editor_draw(const EditorState *state);

#endif /* UI_EDITOR_H */
