#include "runtime.h"

/* ============================================================
 * Constants
 * ============================================================ */
#define ANALOG_CENTER    128
#define ANALOG_DEADZONE  16
#define ANALOG_MAX       127.0f
#define PRESSURE_MAX     255.0f

/* ============================================================
 * Helper: Normalize analog stick value with deadzone
 * ============================================================ */
static float normalize_analog(uint8_t raw)
{
    int centered = (int)raw - ANALOG_CENTER;
    float normalized;

    /* Apply deadzone */
    if (centered > -ANALOG_DEADZONE && centered < ANALOG_DEADZONE) {
        return 0.0f;
    }

    /* After deadzone check, remap remaining range */
    if (centered < 0) {
        centered += ANALOG_DEADZONE;
    } else {
        centered -= ANALOG_DEADZONE;
    }
    normalized = (float)centered / (ANALOG_MAX - ANALOG_DEADZONE);

    /* Clamp */
    if (normalized < -1.0f) normalized = -1.0f;
    if (normalized > 1.0f) normalized = 1.0f;

    return normalized;
}

/* ============================================================
 * Helper: Normalize pressure value (0-255 to 0.0-1.0)
 * ============================================================ */
static float normalize_pressure(uint8_t raw)
{
    return (float)raw / PRESSURE_MAX;
}

/* ============================================================
 * Initialize Runtime Context
 * ============================================================ */
void runtime_init(RuntimeContext *ctx)
{
    if (ctx == NULL) {
        return;
    }

    ctx->time = 0.0f;
    ctx->dt = 0.0f;
    ctx->frame = 0;

    ctx->pad_lx = 0.0f;
    ctx->pad_ly = 0.0f;
    ctx->pad_rx = 0.0f;
    ctx->pad_ry = 0.0f;
    ctx->pad_l2 = 0.0f;
    ctx->pad_r2 = 0.0f;

    ctx->buttons_held = 0;
    ctx->buttons_pressed = 0;
    ctx->buttons_released = 0;
}

/* ============================================================
 * Reset Runtime Context
 * ============================================================ */
void runtime_reset(RuntimeContext *ctx)
{
    if (ctx == NULL) {
        return;
    }

    ctx->time = 0.0f;
    ctx->frame = 0;
    /* Keep dt and pad state intact for continuity */
}

/* ============================================================
 * Update Timing
 * ============================================================ */
void runtime_update_timing(RuntimeContext *ctx, float dt)
{
    if (ctx == NULL) {
        return;
    }

    /* Clamp dt to reasonable bounds (prevent huge jumps) */
    if (dt < 0.0f) {
        dt = 0.0f;
    }
    if (dt > 0.1f) {
        dt = 0.1f;  /* Cap at 100ms (10 FPS minimum) */
    }

    ctx->dt = dt;
    ctx->time += dt;
    ctx->frame++;
}

/* ============================================================
 * Update Pad State
 * ============================================================ */
void runtime_update_pad(RuntimeContext *ctx,
                        uint8_t lx, uint8_t ly,
                        uint8_t rx, uint8_t ry,
                        uint8_t l2, uint8_t r2,
                        uint16_t buttons)
{
    uint16_t prev_buttons;

    if (ctx == NULL) {
        return;
    }

    /* Store previous button state for edge detection */
    prev_buttons = ctx->buttons_held;

    /* Normalize analog sticks */
    ctx->pad_lx = normalize_analog(lx);
    ctx->pad_ly = normalize_analog(ly);
    ctx->pad_rx = normalize_analog(rx);
    ctx->pad_ry = normalize_analog(ry);

    /* Normalize triggers */
    ctx->pad_l2 = normalize_pressure(l2);
    ctx->pad_r2 = normalize_pressure(r2);

    /* Update button state */
    ctx->buttons_held = buttons;

    /* Edge detection */
    ctx->buttons_pressed = buttons & ~prev_buttons;   /* Now pressed, wasn't before */
    ctx->buttons_released = ~buttons & prev_buttons;  /* Now released, was pressed */
}

/* ============================================================
 * Button Query Helpers
 * ============================================================ */
int runtime_button_held(const RuntimeContext *ctx, uint16_t btn)
{
    if (ctx == NULL) {
        return 0;
    }
    return (ctx->buttons_held & btn) != 0;
}

int runtime_button_pressed(const RuntimeContext *ctx, uint16_t btn)
{
    if (ctx == NULL) {
        return 0;
    }
    return (ctx->buttons_pressed & btn) != 0;
}

int runtime_button_released(const RuntimeContext *ctx, uint16_t btn)
{
    if (ctx == NULL) {
        return 0;
    }
    return (ctx->buttons_released & btn) != 0;
}
