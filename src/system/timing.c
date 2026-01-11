#include "timing.h"
#include <kernel.h>
#include <graph.h>

/* ============================================================
 * Static State
 * ============================================================
 * Memory usage: ~32 bytes
 * ============================================================ */
static TimingMode s_mode = TIMING_MODE_NTSC;
static float s_dt = 0.0f;
static float s_time = 0.0f;
static unsigned int s_frame = 0;
static float s_dt_min = TIMING_DT_MIN;
static float s_dt_max = TIMING_DT_MAX;
static int s_target_fps = TIMING_TARGET_FPS_NTSC;
static int s_initialized = 0;

/* ============================================================
 * Initialize Timing
 * ============================================================ */
void timing_init(TimingMode mode)
{
    s_mode = mode;
    s_dt = 0.0f;
    s_time = 0.0f;
    s_frame = 0;
    s_dt_min = TIMING_DT_MIN;
    s_dt_max = TIMING_DT_MAX;

    if (mode == TIMING_MODE_PAL) {
        s_target_fps = TIMING_TARGET_FPS_PAL;
    } else {
        s_target_fps = TIMING_TARGET_FPS_NTSC;
    }

    s_initialized = 1;
}

/* ============================================================
 * Update Timing
 * ============================================================
 * Calculates dt based on target framerate.
 * VSync is handled by gsKit (gsKit_sync_flip), so we don't
 * call graph_wait_vsync here to avoid double-waiting.
 * ============================================================ */
float timing_update(void)
{
    float frame_time;

    if (!s_initialized) {
        timing_init(TIMING_MODE_NTSC);
    }

    /* Note: VSync wait is handled by gsKit_sync_flip in render_end_frame */
    /* We just calculate dt based on target fps */

    /* Each frame should be 1/fps */
    frame_time = 1.0f / (float)s_target_fps;

    /* Clamp dt to prevent physics explosions */
    if (frame_time < s_dt_min) {
        frame_time = s_dt_min;
    }
    if (frame_time > s_dt_max) {
        frame_time = s_dt_max;
    }

    s_dt = frame_time;
    s_time += s_dt;
    s_frame++;

    return s_dt;
}

/* ============================================================
 * Get Delta Time
 * ============================================================ */
float timing_get_dt(void)
{
    return s_dt;
}

/* ============================================================
 * Get Total Time
 * ============================================================ */
float timing_get_time(void)
{
    return s_time;
}

/* ============================================================
 * Get Frame Number
 * ============================================================ */
unsigned int timing_get_frame(void)
{
    return s_frame;
}

/* ============================================================
 * Get Target FPS
 * ============================================================ */
int timing_get_target_fps(void)
{
    return s_target_fps;
}

/* ============================================================
 * Set DT Bounds
 * ============================================================ */
void timing_set_dt_bounds(float dt_min, float dt_max)
{
    if (dt_min > 0.0f && dt_min < dt_max) {
        s_dt_min = dt_min;
    }
    if (dt_max > dt_min && dt_max <= 1.0f) {
        s_dt_max = dt_max;
    }
}

/* ============================================================
 * Reset Timing
 * ============================================================
 * Call after pause/unpause to prevent large dt spike.
 * ============================================================ */
void timing_reset(void)
{
    s_dt = 1.0f / (float)s_target_fps;
}
