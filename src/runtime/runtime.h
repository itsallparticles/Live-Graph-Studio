#ifndef RUNTIME_H
#define RUNTIME_H

#include "../common.h"

/* ============================================================
 * Button Mask Definitions
 * ============================================================ */
#define BTN_SELECT    (1 << 0)
#define BTN_L3        (1 << 1)
#define BTN_R3        (1 << 2)
#define BTN_START     (1 << 3)
#define BTN_UP        (1 << 4)
#define BTN_RIGHT     (1 << 5)
#define BTN_DOWN      (1 << 6)
#define BTN_LEFT      (1 << 7)
#define BTN_L2        (1 << 8)
#define BTN_R2        (1 << 9)
#define BTN_L1        (1 << 10)
#define BTN_R1        (1 << 11)
#define BTN_TRIANGLE  (1 << 12)
#define BTN_CIRCLE    (1 << 13)
#define BTN_CROSS     (1 << 14)
#define BTN_SQUARE    (1 << 15)
#define BTN_DPAD (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)

/* NOTE: These button masks expect pre-inverted input.
 * libpad returns buttons as active-low; caller must XOR with 0xFFFF.
 * Bit order matches DualShock2 after inversion.
 */

/* ============================================================
 * Runtime Context
 * ============================================================
 * Central state passed to node evaluation and rendering.
 * Updated once per frame before graph evaluation.
 * ============================================================ */
typedef struct {
    /* Timing */
    float    time;           /* Total elapsed time in seconds */
    float    dt;             /* Delta time since last frame (seconds) */
    uint32_t frame;          /* Frame counter */

    /* Controller (normalized values) */
    float    pad_lx;         /* Left stick X: -1.0 to 1.0 */
    float    pad_ly;         /* Left stick Y: -1.0 to 1.0 */
    float    pad_rx;         /* Right stick X: -1.0 to 1.0 */
    float    pad_ry;         /* Right stick Y: -1.0 to 1.0 */
    float    pad_l2;         /* L2 trigger: 0.0 to 1.0 */
    float    pad_r2;         /* R2 trigger: 0.0 to 1.0 */

    /* Button state */
    uint16_t buttons_held;   /* Currently held buttons (bitmask) */
    uint16_t buttons_pressed;/* Just pressed this frame (bitmask) */
    uint16_t buttons_released;/* Just released this frame (bitmask) */
} RuntimeContext;

/* ============================================================
 * Runtime API
 * ============================================================ */

/* Initialize runtime context to defaults */
void runtime_init(RuntimeContext *ctx);

/* Reset runtime context (e.g., on graph reload) */
void runtime_reset(RuntimeContext *ctx);

/* Update timing values; call once per frame with measured dt */
void runtime_update_timing(RuntimeContext *ctx, float dt);

/* Update pad values from raw controller state
 * lx, ly, rx, ry: raw analog values (0-255, 128 = center)
 * l2, r2: raw pressure values (0-255)
 * buttons: raw button bitmask
 */
void runtime_update_pad(RuntimeContext *ctx,
                        uint8_t lx, uint8_t ly,
                        uint8_t rx, uint8_t ry,
                        uint8_t l2, uint8_t r2,
                        uint16_t buttons);

/* Helper: check if button is currently held */
int runtime_button_held(const RuntimeContext *ctx, uint16_t btn);

/* Helper: check if button was just pressed this frame */
int runtime_button_pressed(const RuntimeContext *ctx, uint16_t btn);

/* Helper: check if button was just released this frame */
int runtime_button_released(const RuntimeContext *ctx, uint16_t btn);

#endif /* RUNTIME_H */
