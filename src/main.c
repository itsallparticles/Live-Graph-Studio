/* PS2 Live Graph Studio - Main Entry Point */

#include "common.h"
#include "graph/graph_types.h"
#include "graph/graph_core.h"
#include "graph/graph_validate.h"
#include "graph/graph_eval.h"
#include "graph/graph_publish.h"
#include "nodes/node_registry.h"
#include "runtime/runtime.h"
#include "system/pad.h"
#include "system/timing.h"
#include "render/render.h"
#include "render/font.h"
#include "ui/editor.h"
#include "io/graph_io.h"
#include "io/assets.h"

#include <stdio.h>
#include <debug.h>
#include <kernel.h>

/* ============================================================
 * Static Application State
 * ============================================================ */
static Graph        s_active_graph;    /* Live graph being evaluated */
static EvalPlan     s_eval_plan;       /* Current evaluation order */
static OutputBank   s_output_bank;     /* Node output storage */
static RuntimeContext s_runtime;       /* Runtime context (time, pad) */
static EditorState  s_editor;          /* Editor UI state */
static PadState     s_pad;             /* Controller state */
static int          s_running = 1;     /* Main loop flag */
static int          s_assets_ok = 0;   /* Asset preload success flag */
static const char  *s_asset_error = NULL; /* Asset error message for banner */
static int          s_editor_visible = 1; /* Editor visibility (R3 toggle) */
static PadState     s_pad_prev;          /* Previous frame pad for edge detect */

/* ============================================================
 * Default Graph Setup
 * ============================================================ */
static void create_default_graph(void)
{
    NodeId time_id, sin_id, colorize_id, render_id;

    graph_init(&s_active_graph);

    /* Create a simple demo graph: Time -> Sin -> Colorize -> Render2D */

    /* Time source */
    if (graph_alloc_node(&s_active_graph, NODE_TYPE_TIME, &time_id) != STATUS_OK) {
        return;
    }

    /* Sin oscillator */
    if (graph_alloc_node(&s_active_graph, NODE_TYPE_SIN, &sin_id) != STATUS_OK) {
        return;
    }
    graph_connect(&s_active_graph, time_id, 0, sin_id, 0);
    graph_set_param(&s_active_graph, sin_id, 0, 1.0f);  /* Frequency */
    graph_set_param(&s_active_graph, sin_id, 1, 1.0f);  /* Amplitude */
    graph_set_param(&s_active_graph, sin_id, 2, 0.0f);  /* Offset */

    /* Colorize */
    if (graph_alloc_node(&s_active_graph, NODE_TYPE_COLORIZE, &colorize_id) != STATUS_OK) {
        return;
    }
    graph_connect(&s_active_graph, sin_id, 0, colorize_id, 0);

    /* Render2D sink */
    if (graph_alloc_node(&s_active_graph, NODE_TYPE_RENDER2D, &render_id) != STATUS_OK) {
        return;
    }
    graph_connect(&s_active_graph, colorize_id, 0, render_id, 0);  /* R */
    graph_connect(&s_active_graph, colorize_id, 1, render_id, 1);  /* G */
    graph_connect(&s_active_graph, colorize_id, 2, render_id, 2);  /* B */
    graph_set_param(&s_active_graph, render_id, 0, 0.3f);  /* X */
    graph_set_param(&s_active_graph, render_id, 1, 0.3f);  /* Y */
    graph_set_param(&s_active_graph, render_id, 2, 0.4f);  /* W */
    graph_set_param(&s_active_graph, render_id, 3, 0.4f);  /* H */

    /* Build evaluation plan */
    if (graph_build_eval_plan(&s_active_graph, &s_eval_plan) != STATUS_OK) {
        printf("Warning: Failed to build eval plan for default graph\n");
    }
}

static int graph_has_render_sink(const Graph *g)
{
    NodeId i;

    if (!g) {
        return 0;
    }

    for (i = 0; i < MAX_NODES; i++) {
        if (g->nodes[i].type == NODE_TYPE_RENDER2D) {
            return 1;
        }
    }

    return 0;
}

/* ============================================================
 * Initialization
 * ============================================================ */
static int app_init(void)
{
    scr_printf("  timing_init...\n");
    /* Initialize timing (NTSC 60Hz) */
    timing_init(TIMING_MODE_NTSC);

    scr_printf("  pad_init...\n");
    /* Initialize controller */
    if (pad_init() != 0) {
        scr_printf("Error: Failed to initialize pad\n");
        return -1;
    }

    scr_printf("  render_init...\n");
    /* Initialize rendering */
    if (render_init() != 0) {
        scr_printf("Error: Failed to initialize render\n");
        return -1;
    }

    scr_printf("  font_init...\n");
    /* Initialize font */
    if (font_init() != 0) {
        scr_printf("Error: Failed to initialize font\n");
        return -1;
    }

    scr_printf("  node_registry_init...\n");
    /* Initialize node registry */
    node_registry_init();

    scr_printf("  assets_init...\n");
    /* Initialize assets system */
    if (assets_init() != ASSETS_OK) {
        scr_printf("Warning: Failed to initialize asset system\n");
        s_asset_error = "Asset system init failed";
    } else {
        /* Preload assets from embedded data (no file I/O) */
        static const char* preload_list[] = {
            "ui/cursor.png",
            "ui/port_dot.png",
            "ui/node_icons.png",
            "visuals/shapes.png",
            "palettes/pal_256.rgb",
            "graphs/default.gph",
            NULL
        };
        const void* data;
        size_t size;
        int preload_ok = 1;
        int i;

        scr_printf("  preloading assets...\n");
        for (i = 0; preload_list[i] != NULL; i++) {
            int result = assets_get(preload_list[i], &data, &size);
            if (result != ASSETS_OK) {
                scr_printf("  Warning: '%s': %s\n",
                       preload_list[i], assets_strerror(result));
                if (s_asset_error == NULL) {
                    s_asset_error = preload_list[i];
                }
                preload_ok = 0;
            } else {
                scr_printf("  OK: %s (%d bytes)\n", preload_list[i], (int)size);
            }
        }

        if (preload_ok) {
            s_assets_ok = 1;
            scr_printf("  All assets OK (%d cached)\n", assets_get_cached_count());
        }
    }

    scr_printf("  runtime_init...\n");
    /* Initialize runtime context */
    runtime_init(&s_runtime);

    /* Try to load saved graph, or create default */
    if (graph_io_load("host:assets/graphs/default.gph", &s_active_graph, NULL) != GRAPH_IO_OK) {
        printf("No saved graph found, creating default\n");
        create_default_graph();
    } else {
        printf("Loaded graph from file\n");
        if (!graph_has_render_sink(&s_active_graph)) {
            printf("Warning: Loaded graph has no RENDER2D sink, creating default\n");
            create_default_graph();
        } else if (graph_build_eval_plan(&s_active_graph, &s_eval_plan) != STATUS_OK) {
            printf("Warning: Loaded graph invalid, creating default\n");
            create_default_graph();
        }
    }
#if 0
    /* TODO: Re-enable graph_io_load when file I/O is confirmed working */
    scr_printf("  create_default_graph...\n");
    create_default_graph();
#endif

    scr_printf("  editor_init...\n");
    /* Initialize editor with active graph */
    editor_init(&s_editor, &s_active_graph);

    scr_printf("  graph_eval_init_outputs...\n");
    /* Initialize output bank */
    graph_eval_init_outputs(&s_output_bank);

    printf("PS2 Live Graph Studio initialized\n");
    return 0;
}

/* ============================================================
 * Shutdown
 * ============================================================ */
static void app_shutdown(void)
{
    assets_shutdown();
    font_shutdown();
    render_shutdown();
    pad_shutdown();

    printf("PS2 Live Graph Studio shutdown\n");
}

/* ============================================================
 * Update
 * ============================================================ */
static void app_update(void)
{
    float dt;

    /* Update timing */
    dt = timing_update();

    /* Poll controller */
    pad_update(0, &s_pad);

    /* Update runtime context */
    runtime_update_timing(&s_runtime, dt);
    runtime_update_pad(&s_runtime,
                       (uint8_t)((s_pad.lx + 1.0f) * 127.5f),
                       (uint8_t)((s_pad.ly + 1.0f) * 127.5f),
                       (uint8_t)((s_pad.rx + 1.0f) * 127.5f),
                       (uint8_t)((s_pad.ry + 1.0f) * 127.5f),
                       (uint8_t)(s_pad.l2 * 255.0f),
                       (uint8_t)(s_pad.r2 * 255.0f),
                       s_pad.held);

    /* Update editor (handles input, mode transitions) */
    editor_update(&s_editor, &s_runtime, &s_active_graph);

    /* Rebuild eval plan if graph was committed */
    if (s_editor.commit_result == COMMIT_SUCCESS) {
        if (graph_build_eval_plan(&s_active_graph, &s_eval_plan) != STATUS_OK) {
            printf("Warning: Failed to rebuild eval plan after commit\n");
        }
    }

    /* Evaluate active graph */
    graph_eval(&s_active_graph, &s_eval_plan, &s_output_bank, &s_runtime);

    /* Check for exit (Select + Start) */
    if ((s_pad.held & 0x0001) && (s_pad.held & 0x0008)) {  /* SELECT + START */
        s_running = 0;
    }

    /* Check for save (L1 + R1 + Start) */
    if ((s_pad.pressed & 0x0008) && (s_pad.held & 0x0400) && (s_pad.held & 0x0800)) {
        GraphIoResult result = graph_io_save("host:assets/graphs/default.gph",
                                              &s_editor.edit_graph,
                                              &s_editor.ui_meta);
        if (result == GRAPH_IO_OK) {
            printf("Graph saved successfully\n");
        } else {
            printf("Failed to save graph: %s\n", graph_io_result_str(result));
        }
    }

    /* Toggle editor visibility on R3 */
    if ((s_pad.held & BTN_R3) && !(s_pad_prev.held & BTN_R3)) {
        s_editor_visible = !s_editor_visible;
    }

    /* Store previous pad state for edge detection */
    s_pad_prev = s_pad;
}

/* ============================================================
 * Render Graph Output (reads sink nodes and draws in preview area)
 * Preview area: top 65% of screen (0 to PREVIEW_HEIGHT)
 * ============================================================ */
static void render_graph_output(void)
{
    NodeId i;
    float x, y, w, h;
    float r, g, b, a;
    uint64_t color;

    /* Find and render all RENDER2D sink nodes (full screen) */
    for (i = 0; i < MAX_NODES; i++) {
        const Node *node = &s_active_graph.nodes[i];
        if (node->type != NODE_TYPE_RENDER2D) {
            continue;
        }

        /* Read geometry from sink outputs (set by node_eval_render2d) */
        x = graph_eval_get_output(&s_output_bank, i, 0);
        y = graph_eval_get_output(&s_output_bank, i, 1);
        w = graph_eval_get_output(&s_output_bank, i, 2);
        h = graph_eval_get_output(&s_output_bank, i, 3);

        /* Skip if dimensions are too small (node not properly set up) */
        if (w < 0.001f || h < 0.001f) {
            continue;
        }

        /* Read color from connected inputs */
        r = 1.0f;
        g = 1.0f;
        b = 1.0f;
        a = 1.0f;

        /* Gather color from connected nodes */
        if (node->inputs[0].src_node != INVALID_NODE_ID) {
            r = graph_eval_get_output(&s_output_bank,
                                       node->inputs[0].src_node,
                                       node->inputs[0].src_port);
        }
        if (node->inputs[1].src_node != INVALID_NODE_ID) {
            g = graph_eval_get_output(&s_output_bank,
                                       node->inputs[1].src_node,
                                       node->inputs[1].src_port);
        }
        if (node->inputs[2].src_node != INVALID_NODE_ID) {
            b = graph_eval_get_output(&s_output_bank,
                                       node->inputs[2].src_node,
                                       node->inputs[2].src_port);
        }
        if (node->inputs[3].src_node != INVALID_NODE_ID) {
            a = graph_eval_get_output(&s_output_bank,
                                       node->inputs[3].src_node,
                                       node->inputs[3].src_port);
        }

        /* Clamp color values to 0-1 and convert to 0-255 */
        r = LGS_CLAMP(r, 0.0f, 1.0f);
        g = LGS_CLAMP(g, 0.0f, 1.0f);
        b = LGS_CLAMP(b, 0.0f, 1.0f);
        a = LGS_CLAMP(a, 0.0f, 1.0f);

        color = RENDER_COLOR((uint8_t)(r * 255.0f),
                             (uint8_t)(g * 255.0f),
                             (uint8_t)(b * 255.0f),
                             (uint8_t)(a * 128.0f));  /* Alpha 0-128 for gsKit */

        /* Draw rectangle at normalized coordinates (full screen) */
        render_rect(x, y, w, h, color);
    }
}

/* ============================================================
 * Render
 * ============================================================ */
static void app_render(void)
{
    /* Begin frame */
    render_begin_frame();
    render_clear(RENDER_COLOR(20, 20, 30, 128));

    /* Render graph output first (background layer - full screen) */
    render_graph_output();

    /* Draw editor UI overlay on top (if visible) */
    if (s_editor_visible) {
        editor_draw(&s_editor);
    }

    /* Draw FPS counter */
    font_printf_screen(RENDER_SCREEN_WIDTH - 80, 10, RENDER_COLOR_WHITE, 1,
                       "FPS: %d", timing_get_target_fps());
    font_printf_screen(RENDER_SCREEN_WIDTH - 80, 22, RENDER_COLOR_WHITE, 1,
                       "dt: %.3f", (double)timing_get_dt());

    /* Draw frame counter */
    font_printf_screen(RENDER_SCREEN_WIDTH - 80, 34, RENDER_COLOR_GRAY, 1,
                       "F: %u", timing_get_frame());

    /* Draw editor toggle hint */
    if (!s_editor_visible) {
        font_printf_screen(10, SCREEN_H - 16, RENDER_COLOR_GRAY, 1,
                           "R3: Show Editor");
    }

    /* Draw asset error banner if preload failed */
    if (!s_assets_ok && s_asset_error != NULL) {
        uint64_t banner_color = RENDER_COLOR(180, 50, 50, 100);
        uint64_t text_color = RENDER_COLOR(255, 255, 100, 128);
        render_rect_screen(0, 0, SCREEN_W, 28, banner_color);
        font_printf_screen(10, 8, text_color, 1,
                           "Asset load failed: %s (running fallback)", s_asset_error);
    }

    /* End frame */
    render_end_frame();
}

/* ============================================================
 * Main Entry Point
 * ============================================================ */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    /* Initialize debug screen first for printf output in PCSX2 */
    init_scr();
    scr_printf("PS2 Live Graph Studio starting...\n");

    /* Initialize application */
    scr_printf("Calling app_init...\n");
    if (app_init() != 0) {
        scr_printf("Failed to initialize application\n");
        SleepThread();  /* Halt so we can see the error */
        return 1;
    }
    scr_printf("app_init complete, entering main loop\n");

    /* Main loop */
    while (s_running) {
        app_update();
        app_render();
    }

    /* Shutdown */
    app_shutdown();

    return 0;
}
