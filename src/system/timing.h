#ifndef TIMING_H
#define TIMING_H

/* ============================================================
 * Timing Module
 * ============================================================
 * VBlank-based timing for consistent frame pacing.
 * Provides delta time calculation with clamping to prevent
 * physics explosions from large dt spikes.
 * ============================================================ */

/* Default timing constants */
#define TIMING_TARGET_FPS_NTSC  60
#define TIMING_TARGET_FPS_PAL   50
#define TIMING_DT_MIN           0.001f    /* 1ms minimum */
#define TIMING_DT_MAX           0.1f      /* 100ms maximum (10 FPS floor) */

/* Video mode enumeration */
typedef enum {
    TIMING_MODE_NTSC = 0,
    TIMING_MODE_PAL
} TimingMode;

/* ============================================================
 * Timing API
 * ============================================================ */

/* Initialize timing subsystem.
 * mode: NTSC (60Hz) or PAL (50Hz) */
void timing_init(TimingMode mode);

/* Wait for next VBlank and update timing.
 * Returns delta time in seconds (clamped). */
float timing_update(void);

/* Get current delta time (from last update) */
float timing_get_dt(void);

/* Get total elapsed time since init */
float timing_get_time(void);

/* Get current frame number */
unsigned int timing_get_frame(void);

/* Get target FPS for current mode */
int timing_get_target_fps(void);

/* Set dt clamping bounds */
void timing_set_dt_bounds(float dt_min, float dt_max);

/* Reset timing (e.g., after pause/unpause) */
void timing_reset(void);

#endif /* TIMING_H */
