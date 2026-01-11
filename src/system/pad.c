#include "pad.h"
#include <libpad.h>
#include <string.h>
#include <sifrpc.h>
#include <loadfile.h>

/* ============================================================
 * Static State
 * ============================================================
 * Memory usage:
 *   s_pad_buffer: 512 bytes (2 ports * 256 bytes, 64-byte aligned for DMA)
 *   s_prev_buttons: 4 bytes
 *   Total: ~520 bytes static allocation
 * ============================================================ */
static uint8_t s_pad_buffer[2][256] __attribute__((aligned(64)));
static uint16_t s_prev_buttons[2];
static int s_deadzone = 10;
static int s_initialized = 0;
static int s_sif_initialized = 0;

/* ============================================================
 * Initialize SIF and Load IOP Modules
 * ============================================================
 * libpad requires sio2man.irx and padman.irx to be loaded on
 * the IOP before padInit/padPortOpen will work.
 * ============================================================ */
static int pad_init_sif(void)
{
    int ret;

    if (s_sif_initialized) {
        return 0;
    }

    /* Initialize SIF RPC system */
    SifInitRpc(0);

    /* Load SIO2 manager (required for pad communication) */
    ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    if (ret < 0) {
        return -1;
    }

    /* Load pad manager */
    ret = SifLoadModule("rom0:PADMAN", 0, NULL);
    if (ret < 0) {
        return -1;
    }

    s_sif_initialized = 1;
    return 0;
}

/* ============================================================
 * Initialize Pad Subsystem
 * ============================================================ */
int pad_init(void)
{
    int ret;
    int pad_state;
    int wait_count;

    if (s_initialized) {
        return 0;
    }

    /* Initialize SIF and load IOP modules first */
    if (pad_init_sif() != 0) {
        return -1;
    }

    /* Clear buffers */
    memset(s_pad_buffer, 0, sizeof(s_pad_buffer));
    s_prev_buttons[0] = 0;
    s_prev_buttons[1] = 0;

    /* Initialize libpad */
    ret = padInit(0);
    if (ret != 1) {
        return -1;
    }

    /* Open port 0 only - port 1 can cause issues */
    ret = padPortOpen(0, 0, s_pad_buffer[0]);
    if (ret == 0) {
        padEnd();
        return -1;
    }

    /* Wait for pad to reach stable state before continuing.
     * This is CRITICAL - calling padSetMainMode or other pad functions
     * before the pad is stable will cause an infinite hang.
     * Use a timeout to avoid hanging if no controller is connected. */
    wait_count = 0;
    while (wait_count < 1000) {  /* ~1000 iterations timeout */
        pad_state = padGetState(0, 0);

        if (pad_state == PAD_STATE_STABLE || pad_state == PAD_STATE_FINDCTP1) {
            /* Pad is ready */
            break;
        }

        if (pad_state == PAD_STATE_DISCONN) {
            /* No controller connected - that's okay, continue anyway */
            break;
        }

        wait_count++;
    }

    /* Note: Skip padSetMainMode - it can hang waiting for controller response
     * even after pad is stable if the controller doesn't support the mode.
     * The pad will work in digital mode by default, analog will be available
     * if the user presses the analog button on their controller. */

    s_initialized = 1;
    return 0;
}

/* ============================================================
 * Shutdown Pad Subsystem
 * ============================================================ */
void pad_shutdown(void)
{
    if (!s_initialized) {
        return;
    }

    padPortClose(0, 0);
    /* Port 1 was not opened */
    padEnd();

    s_initialized = 0;
}

/* ============================================================
 * Normalize Analog Value
 * ============================================================
 * Converts 0-255 range to -1.0 to 1.0 with deadzone
 * ============================================================ */
static float normalize_analog(uint8_t raw, int deadzone)
{
    int centered;
    float normalized;

    /* Center around 0 (-128 to 127) */
    centered = (int)raw - 128;

    /* Apply deadzone */
    if (centered > -deadzone && centered < deadzone) {
        return 0.0f;
    }

    /* Remove deadzone from range */
    if (centered < 0) {
        centered += deadzone;
        normalized = (float)centered / (128.0f - (float)deadzone);
    } else {
        centered -= deadzone;
        normalized = (float)centered / (127.0f - (float)deadzone);
    }

    /* Clamp to -1.0 to 1.0 */
    if (normalized < -1.0f) normalized = -1.0f;
    if (normalized > 1.0f) normalized = 1.0f;

    return normalized;
}

/* ============================================================
 * Normalize Trigger Value
 * ============================================================
 * Converts 0-255 range to 0.0 to 1.0
 * ============================================================ */
static float normalize_trigger(uint8_t raw)
{
    return (float)raw / 255.0f;
}

/* ============================================================
 * Update Pad State
 * ============================================================ */
void pad_update(int port, PadState *state)
{
    struct padButtonStatus buttons;
    int pad_state;
    int pad_mode;
    uint16_t btns;
    int ret;

    if (!state) {
        return;
    }

    /* Default to disconnected state */
    memset(state, 0, sizeof(PadState));

    if (!s_initialized) {
        return;
    }

    if (port < 0 || port > 1) {
        return;
    }

    /* Check pad state */
    pad_state = padGetState(port, 0);
    if (pad_state != PAD_STATE_STABLE && pad_state != PAD_STATE_FINDCTP1) {
        s_prev_buttons[port] = 0;
        return;
    }

    /* Read button data */
    ret = padRead(port, 0, &buttons);
    if (ret == 0) {
        s_prev_buttons[port] = 0;
        return;
    }

    state->connected = 1;

    /* Invert button bits (libpad returns active-low) */
    btns = buttons.btns ^ 0xFFFF;

    /* Edge detection */
    state->held = btns;
    state->pressed = btns & ~s_prev_buttons[port];
    state->released = ~btns & s_prev_buttons[port];
    s_prev_buttons[port] = btns;

    /* Normalize analog sticks */
    state->lx = normalize_analog(buttons.ljoy_h, s_deadzone);
    state->ly = normalize_analog(buttons.ljoy_v, s_deadzone);
    state->rx = normalize_analog(buttons.rjoy_h, s_deadzone);
    state->ry = normalize_analog(buttons.rjoy_v, s_deadzone);

    /* Normalize triggers (pressure sensitive, DualShock2 only) */
    pad_mode = padInfoMode(port, 0, PAD_MODETABLE, -1);
    if (pad_mode == PAD_TYPE_DUALSHOCK) {
        state->l2 = normalize_trigger(buttons.l2_p);
        state->r2 = normalize_trigger(buttons.r2_p);
    } else {
        /* Digital-only controller: use button state instead */
        state->l2 = (btns & (1 << 8)) ? 1.0f : 0.0f;  /* L2 button */
        state->r2 = (btns & (1 << 9)) ? 1.0f : 0.0f;  /* R2 button */
    }
}

/* ============================================================
 * Check Connection Status
 * ============================================================ */
int pad_is_connected(int port)
{
    int pad_state;

    if (!s_initialized) {
        return 0;
    }

    if (port < 0 || port > 1) {
        return 0;
    }

    pad_state = padGetState(port, 0);
    return (pad_state == PAD_STATE_STABLE || pad_state == PAD_STATE_FINDCTP1);
}

/* ============================================================
 * Set Deadzone
 * ============================================================ */
void pad_set_deadzone(int deadzone)
{
    if (deadzone < 0) {
        deadzone = 0;
    }
    if (deadzone > 64) {
        deadzone = 64;
    }
    s_deadzone = deadzone;
}

/* ============================================================
 * Get Deadzone
 * ============================================================ */
int pad_get_deadzone(void)
{
    return s_deadzone;
}

/* ============================================================
 * Wait for Pad Ready
 * ============================================================
 * Blocks until pad reaches stable state or timeout.
 * timeout_frames: max frames to wait (0 = no timeout)
 * Returns 1 if ready, 0 if timeout or error.
 * ============================================================ */
int pad_wait_ready(int port, int timeout_frames)
{
    int pad_state;
    int frames = 0;

    if (!s_initialized) {
        return 0;
    }

    if (port < 0 || port > 1) {
        return 0;
    }

    while (1) {
        pad_state = padGetState(port, 0);

        if (pad_state == PAD_STATE_STABLE || pad_state == PAD_STATE_FINDCTP1) {
            return 1;
        }

        if (pad_state == PAD_STATE_DISCONN) {
            return 0;
        }

        frames++;
        if (timeout_frames > 0 && frames >= timeout_frames) {
            return 0;
        }

        /* Small delay - in real use, caller should integrate with vsync */
    }
}
