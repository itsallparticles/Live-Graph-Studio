#ifndef PAD_H
#define PAD_H

#include <stdint.h>

/* ============================================================
 * Pad Module
 * ============================================================
 * libpad wrapper providing:
 * - Initialization and polling
 * - Normalized analog stick values (-1.0 to 1.0)
 * - Normalized trigger values (0.0 to 1.0)
 * - Button edge detection (pressed/released)
 * ============================================================ */

/* Pad state structure */
typedef struct {
    /* Normalized analog sticks (-1.0 to 1.0) */
    float lx;
    float ly;
    float rx;
    float ry;

    /* Normalized triggers (0.0 to 1.0) */
    float l2;
    float r2;

    /* Button state */
    uint16_t held;      /* Currently held buttons */
    uint16_t pressed;   /* Just pressed this frame */
    uint16_t released;  /* Just released this frame */

    /* Connection status */
    int connected;
} PadState;

/* ============================================================
 * Pad API
 * ============================================================ */

/* Initialize pad subsystem. Call once at startup.
 * Returns 0 on success, -1 on failure. */
int pad_init(void);

/* Shutdown pad subsystem. Call at exit. */
void pad_shutdown(void);

/* Poll pad and update state. Call once per frame.
 * port: controller port (0 or 1)
 * state: output state structure */
void pad_update(int port, PadState *state);

/* Check if pad is connected on given port */
int pad_is_connected(int port);

/* Deadzone threshold for analog sticks (0-127 range, default 10) */
void pad_set_deadzone(int deadzone);

/* Get current deadzone value */
int pad_get_deadzone(void);

/* Wait for pad to reach stable state.
 * timeout_frames: max frames to wait (0 = no timeout)
 * Returns 1 if ready, 0 if timeout or disconnected. */
int pad_wait_ready(int port, int timeout_frames);

#endif /* PAD_H */
