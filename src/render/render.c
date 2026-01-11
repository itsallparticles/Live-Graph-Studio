#include "render.h"
#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>
#include <math.h>
#include <kernel.h>

/* PI constant for circle calculations */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define RENDER_PI ((float)M_PI)

/* ============================================================
 * Static State
 * ============================================================
 * gsKit context allocated once at init time.
 * NOTE: gsKit_init_global() uses malloc internally. This is
 * acceptable as it happens only at init time, not per-frame.
 * ============================================================ */
static GSGLOBAL *s_gs = NULL;
static int s_initialized = 0;

/* ============================================================
 * Initialize Rendering
 * ============================================================ */
int render_init(void)
{
    if (s_initialized) {
        return 0;
    }

    /* Initialize DMA - required before gsKit */
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    /* Allocate gsKit global context */
    s_gs = gsKit_init_global();
    if (!s_gs) {
        return -1;
    }

    /* Configure display mode: 640x480 interlaced NTSC */
    s_gs->Mode = GS_MODE_NTSC;
    s_gs->Interlace = GS_INTERLACED;
    s_gs->Field = GS_FIELD;
    s_gs->Width = RENDER_SCREEN_WIDTH;
    s_gs->Height = RENDER_SCREEN_HEIGHT;
    s_gs->PSM = GS_PSM_CT32;     /* 32-bit for proper alpha support */
    s_gs->PSMZ = GS_PSMZ_16S;
    s_gs->DoubleBuffering = GS_SETTING_ON;
    s_gs->ZBuffering = GS_SETTING_OFF;
    s_gs->PrimAlphaEnable = GS_SETTING_OFF;  /* Disable alpha blending for now */

    /* Initialize screen - this sets up the GS hardware */
    gsKit_init_screen(s_gs);

    /* Use ONESHOT mode for manual queue control */
    gsKit_mode_switch(s_gs, GS_ONESHOT);

    /* Do an initial frame to ensure display is active */
    gsKit_queue_reset(s_gs->Os_Queue);
    gsKit_clear(s_gs, GS_SETREG_RGBAQ(0x40, 0x00, 0x00, 0x80, 0x00)); /* Red to confirm rendering */
    gsKit_queue_exec(s_gs);
    gsKit_sync_flip(s_gs);

    s_initialized = 1;
    return 0;
}

/* ============================================================
 * Shutdown Rendering
 * ============================================================ */
void render_shutdown(void)
{
    if (!s_initialized) {
        return;
    }

    if (s_gs) {
        gsKit_deinit_global(s_gs);
        s_gs = NULL;
    }

    s_initialized = 0;
}

/* ============================================================
 * Begin Frame
 * ============================================================ */
void render_begin_frame(void)
{
    if (!s_initialized || !s_gs) {
        return;
    }

    gsKit_queue_reset(s_gs->Os_Queue);
}

/* ============================================================
 * End Frame
 * ============================================================ */
void render_end_frame(void)
{
    if (!s_initialized || !s_gs) {
        return;
    }

    gsKit_queue_exec(s_gs);
    gsKit_sync_flip(s_gs);
}

/* ============================================================
 * Clear Screen
 * ============================================================ */
void render_clear(uint64_t color)
{
    uint8_t r, g, b;

    if (!s_initialized || !s_gs) {
        return;
    }

    r = (uint8_t)(color & 0xFF);
    g = (uint8_t)((color >> 8) & 0xFF);
    b = (uint8_t)((color >> 16) & 0xFF);

    /* Use 0x80 alpha for opaque clear */
    gsKit_clear(s_gs, GS_SETREG_RGBAQ(r, g, b, 0x80, 0x00));
}

/* ============================================================
 * Coordinate Conversion
 * ============================================================ */
int render_norm_to_screen_x(float nx)
{
    int sx = (int)(nx * (float)RENDER_SCREEN_WIDTH);
    if (sx < 0) sx = 0;
    if (sx >= RENDER_SCREEN_WIDTH) sx = RENDER_SCREEN_WIDTH - 1;
    return sx;
}

int render_norm_to_screen_y(float ny)
{
    int sy = (int)(ny * (float)RENDER_SCREEN_HEIGHT);
    if (sy < 0) sy = 0;
    if (sy >= RENDER_SCREEN_HEIGHT) sy = RENDER_SCREEN_HEIGHT - 1;
    return sy;
}

float render_screen_to_norm_x(int sx)
{
    return (float)sx / (float)RENDER_SCREEN_WIDTH;
}

float render_screen_to_norm_y(int sy)
{
    return (float)sy / (float)RENDER_SCREEN_HEIGHT;
}

int render_get_width(void)
{
    return RENDER_SCREEN_WIDTH;
}

int render_get_height(void)
{
    return RENDER_SCREEN_HEIGHT;
}

/* ============================================================
 * Draw Filled Rectangle (screen coords)
 * ============================================================ */
void render_rect_screen(int x, int y, int w, int h, uint64_t color)
{
    uint8_t r, g, b;
    int x2, y2;

    if (!s_initialized || !s_gs) {
        return;
    }

    /* Clamp to screen bounds */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    x2 = x + w;
    y2 = y + h;
    if (x2 > RENDER_SCREEN_WIDTH) x2 = RENDER_SCREEN_WIDTH;
    if (y2 > RENDER_SCREEN_HEIGHT) y2 = RENDER_SCREEN_HEIGHT;
    if (x >= x2 || y >= y2) return;

    r = (uint8_t)(color & 0xFF);
    g = (uint8_t)((color >> 8) & 0xFF);
    b = (uint8_t)((color >> 16) & 0xFF);
    /* Alpha disabled - just use 0x80 for all primitives */

    gsKit_prim_sprite(s_gs, (float)x, (float)y,
                      (float)x2, (float)y2,
                      0, GS_SETREG_RGBAQ(r, g, b, 0x80, 0x00));
}

/* ============================================================
 * Draw Filled Rectangle (normalized coords)
 * ============================================================ */
void render_rect(float x, float y, float w, float h, uint64_t color)
{
    int sx, sy, sw, sh;

    sx = render_norm_to_screen_x(x);
    sy = render_norm_to_screen_y(y);
    sw = (int)(w * (float)RENDER_SCREEN_WIDTH);
    sh = (int)(h * (float)RENDER_SCREEN_HEIGHT);

    if (sw < 1) sw = 1;
    if (sh < 1) sh = 1;

    render_rect_screen(sx, sy, sw, sh, color);
}

/* ============================================================
 * Draw Line (screen coords)
 * ============================================================ */
void render_line_screen(int x1, int y1, int x2, int y2, uint64_t color)
{
    uint8_t r, g, b;

    if (!s_initialized || !s_gs) {
        return;
    }

    /* Clamp endpoints to screen bounds */
    if (x1 < 0) x1 = 0;
    if (x1 >= RENDER_SCREEN_WIDTH) x1 = RENDER_SCREEN_WIDTH - 1;
    if (y1 < 0) y1 = 0;
    if (y1 >= RENDER_SCREEN_HEIGHT) y1 = RENDER_SCREEN_HEIGHT - 1;
    if (x2 < 0) x2 = 0;
    if (x2 >= RENDER_SCREEN_WIDTH) x2 = RENDER_SCREEN_WIDTH - 1;
    if (y2 < 0) y2 = 0;
    if (y2 >= RENDER_SCREEN_HEIGHT) y2 = RENDER_SCREEN_HEIGHT - 1;

    r = (uint8_t)(color & 0xFF);
    g = (uint8_t)((color >> 8) & 0xFF);
    b = (uint8_t)((color >> 16) & 0xFF);
    /* Alpha disabled - just use 0x80 for all primitives */

    gsKit_prim_line(s_gs, (float)x1, (float)y1, (float)x2, (float)y2,
                    0, GS_SETREG_RGBAQ(r, g, b, 0x80, 0x00));
}

/* ============================================================
 * Draw Line (normalized coords)
 * ============================================================ */
void render_line(float x1, float y1, float x2, float y2, uint64_t color)
{
    int sx1, sy1, sx2, sy2;

    sx1 = render_norm_to_screen_x(x1);
    sy1 = render_norm_to_screen_y(y1);
    sx2 = render_norm_to_screen_x(x2);
    sy2 = render_norm_to_screen_y(y2);

    render_line_screen(sx1, sy1, sx2, sy2, color);
}

/* ============================================================
 * Draw Rectangle Outline (normalized coords)
 * ============================================================ */
void render_rect_outline(float x, float y, float w, float h, uint64_t color)
{
    float x2 = x + w;
    float y2 = y + h;

    /* Top edge */
    render_line(x, y, x2, y, color);
    /* Bottom edge */
    render_line(x, y2, x2, y2, color);
    /* Left edge */
    render_line(x, y, x, y2, color);
    /* Right edge */
    render_line(x2, y, x2, y2, color);
}

/* ============================================================
 * Draw Rectangle Outline (screen coords)
 * ============================================================ */
void render_rect_outline_screen(int x, int y, int w, int h, uint64_t color)
{
    int x2 = x + w;
    int y2 = y + h;

    /* Top edge */
    render_line_screen(x, y, x2, y, color);
    /* Bottom edge */
    render_line_screen(x, y2, x2, y2, color);
    /* Left edge */
    render_line_screen(x, y, x, y2, color);
    /* Right edge */
    render_line_screen(x2, y, x2, y2, color);
}

/* ============================================================
 * Draw Circle (line segments, normalized coords)
 * ============================================================ */
void render_circle(float cx, float cy, float r, uint64_t color, int segments)
{
    float angle_step;
    float angle;
    float x1, y1, x2, y2;
    float rx, ry;
    int i;

    if (!s_initialized || !s_gs) {
        return;
    }

    if (segments < 3) segments = 3;
    if (segments > 64) segments = 64;

    angle_step = (2.0f * RENDER_PI) / (float)segments;

    /* Adjust radius for aspect ratio */
    rx = r;
    ry = r * ((float)RENDER_SCREEN_WIDTH / (float)RENDER_SCREEN_HEIGHT);

    x1 = cx + rx;
    y1 = cy;

    for (i = 1; i <= segments; i++) {
        angle = (float)i * angle_step;
        x2 = cx + rx * cosf(angle);
        y2 = cy + ry * sinf(angle);
        render_line(x1, y1, x2, y2, color);
        x1 = x2;
        y1 = y2;
    }
}

/* ============================================================
 * Draw Filled Circle (triangle fan, normalized coords)
 * ============================================================ */
void render_circle_filled(float cx, float cy, float r, uint64_t color, int segments)
{
    int i;
    uint8_t cr, cg, cb, ca;
    int scx, scy;
    int screen_rx, screen_ry;
    float angle_step, angle;
    int prev_x, prev_y, cur_x, cur_y;

    if (!s_initialized || !s_gs) {
        return;
    }

    if (segments < 3) segments = 3;
    if (segments > 64) segments = 64;

    cr = (uint8_t)(color & 0xFF);
    cg = (uint8_t)((color >> 8) & 0xFF);
    cb = (uint8_t)((color >> 16) & 0xFF);
    ca = (uint8_t)((color >> 24) & 0xFF);

    /* Convert center to screen coords */
    scx = render_norm_to_screen_x(cx);
    scy = render_norm_to_screen_y(cy);
    
    /* Convert radius to screen pixels (use X for circular appearance) */
    screen_rx = (int)(r * (float)RENDER_SCREEN_WIDTH);
    screen_ry = (int)(r * (float)RENDER_SCREEN_HEIGHT);
    
    /* Ensure minimum size */
    if (screen_rx < 2) screen_rx = 2;
    if (screen_ry < 2) screen_ry = 2;

    angle_step = (2.0f * RENDER_PI) / (float)segments;

    /* First point on circle */
    prev_x = scx + screen_rx;
    prev_y = scy;

    for (i = 1; i <= segments; i++) {
        angle = (float)i * angle_step;
        cur_x = scx + (int)(screen_rx * cosf(angle));
        cur_y = scy + (int)(screen_ry * sinf(angle));

        gsKit_prim_triangle(s_gs,
                            (float)scx, (float)scy,
                            (float)prev_x, (float)prev_y,
                            (float)cur_x, (float)cur_y,
                            0, GS_SETREG_RGBAQ(cr, cg, cb, ca, 0x00));

        prev_x = cur_x;
        prev_y = cur_y;
    }
}

/* ============================================================
 * Check if Initialized
 * ============================================================ */
int render_is_initialized(void)
{
    return s_initialized;
}

/* ============================================================
 * Get gsKit Context (for font/texture modules)
 * ============================================================ */
GSGLOBAL *render_get_gs_context(void)
{
    return s_gs;
}
