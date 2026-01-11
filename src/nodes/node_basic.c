#include "node_registry.h"
#include <math.h>

/* ============================================================
 * NODE_TYPE_CONST: Output a constant value from params[0]
 * ============================================================ */
void node_eval_const(const Node *node, const float inputs[MAX_IN_PORTS],
                     float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)inputs;
    (void)ctx;

    outputs[0] = node->params[0];
    outputs[1] = 0.0f;
    outputs[2] = 0.0f;
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_TIME: Output time and dt
 * ============================================================ */
void node_eval_time(const Node *node, const float inputs[MAX_IN_PORTS],
                    float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float scale;
    (void)inputs;

    scale = node->params[0];
    if (scale == 0.0f) scale = 1.0f;

    outputs[0] = ctx->time * scale;
    outputs[1] = ctx->dt * scale;
    outputs[2] = 0.0f;
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_PAD: Output controller analog values
 * ============================================================ */
void node_eval_pad(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    int channel;
    (void)inputs;

    channel = (int)node->params[0];

    /* Channel 0: left stick, Channel 1: right stick, etc. */
    if (channel == 0) {
        outputs[0] = ctx->pad_lx;
        outputs[1] = ctx->pad_ly;
        outputs[2] = ctx->pad_rx;
        outputs[3] = ctx->pad_ry;
    } else if (channel == 1) {
        outputs[0] = ctx->pad_rx;
        outputs[1] = ctx->pad_ry;
        outputs[2] = ctx->pad_l2;
        outputs[3] = ctx->pad_r2;
    } else {
        outputs[0] = ctx->pad_lx;
        outputs[1] = ctx->pad_ly;
        outputs[2] = ctx->pad_rx;
        outputs[3] = ctx->pad_ry;
    }
}

/* ============================================================
 * NODE_TYPE_ADD: Add two inputs
 * ============================================================ */
void node_eval_add(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)node;
    (void)ctx;

    outputs[0] = inputs[0] + inputs[1];
    outputs[1] = 0.0f;
    outputs[2] = 0.0f;
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_MUL: Multiply two inputs
 * ============================================================ */
void node_eval_mul(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)node;
    (void)ctx;

    outputs[0] = inputs[0] * inputs[1];
    outputs[1] = 0.0f;
    outputs[2] = 0.0f;
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_SIN: Sine function with frequency and amplitude
 * ============================================================ */
void node_eval_sin(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float freq, amp, angle;
    (void)ctx;

    freq = node->params[0];
    amp = node->params[1];
    if (freq == 0.0f) freq = 1.0f;
    if (amp == 0.0f) amp = 1.0f;

    angle = inputs[0] * freq;
    outputs[0] = sinf(angle) * amp;
    outputs[1] = 0.0f;
    outputs[2] = 0.0f;
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_LERP: Linear interpolation between a and b by t
 * ============================================================ */
void node_eval_lerp(const Node *node, const float inputs[MAX_IN_PORTS],
                    float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float a, b, t;
    (void)node;
    (void)ctx;

    a = inputs[0];
    b = inputs[1];
    t = inputs[2];

    /* Clamp t to [0, 1] */
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    outputs[0] = a + (b - a) * t;
    outputs[1] = 0.0f;
    outputs[2] = 0.0f;
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_SMOOTH: Exponential smoothing using state
 * ============================================================ */
void node_eval_smooth(const Node *node, const float inputs[MAX_IN_PORTS],
                      float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float speed, target, current, blend;
    float *state;

    /* Use state_u32[0] as float storage for smoothed value */
    state = (float *)&((Node *)node)->state_u32[0];

    target = inputs[0];
    speed = node->params[0];
    if (speed < 0.1f) speed = 0.1f;

    current = *state;
    blend = 1.0f - expf(-speed * ctx->dt);
    current = current + (target - current) * blend;

    *state = current;
    outputs[0] = current;
    outputs[1] = 0.0f;
    outputs[2] = 0.0f;
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_COLORIZE: Map value to RGB using base colors
 * ============================================================ */
void node_eval_colorize(const Node *node, const float inputs[MAX_IN_PORTS],
                        float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float value;
    (void)ctx;

    value = inputs[0];

    /* Clamp value to [0, 1] */
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;

    outputs[0] = node->params[0] * value;  /* R */
    outputs[1] = node->params[1] * value;  /* G */
    outputs[2] = node->params[2] * value;  /* B */
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_TRANSFORM2D: Apply 2D transformation
 * ============================================================ */
void node_eval_transform2d(const Node *node, const float inputs[MAX_IN_PORTS],
                           float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float x, y, scale_in;
    float ox, oy, rot, scale_mul;
    float cos_r, sin_r;
    float rx, ry;
    (void)ctx;

    x = inputs[0];
    y = inputs[1];
    scale_in = inputs[2];
    if (scale_in == 0.0f) scale_in = 1.0f;

    ox = node->params[0];         /* offset X */
    oy = node->params[1];         /* offset Y */
    rot = node->params[2];        /* rotation in radians */
    scale_mul = node->params[3];  /* scale multiplier */
    if (scale_mul == 0.0f) scale_mul = 1.0f;

    /* Apply rotation */
    cos_r = cosf(rot);
    sin_r = sinf(rot);
    rx = x * cos_r - y * sin_r;
    ry = x * sin_r + y * cos_r;

    /* Apply offset and output */
    outputs[0] = rx + ox;
    outputs[1] = ry + oy;
    outputs[2] = scale_in * scale_mul;
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_RENDER2D: Sink node that outputs render parameters
 * ============================================================
 * Inputs: R, G, B, A (color from graph)
 * Params: X, Y, W, H (geometry)
 * Outputs: x, y, w, h (passed through for render pass)
 *
 * Color is stored in node state for render pass to read:
 *   state_u32[0] = packed RGBA color
 * ============================================================ */
void node_eval_render2d(const Node *node, const float inputs[MAX_IN_PORTS],
                        float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)ctx;

    /* Output geometry from params */
    outputs[0] = node->params[0];  /* X */
    outputs[1] = node->params[1];  /* Y */
    outputs[2] = node->params[2];  /* W */
    outputs[3] = node->params[3];  /* H */

    /* Color comes from inputs (0-1 range), clamped and stored.
     * We store as floats in outputs wouldn't fit, so caller
     * reads inputs directly via graph. For simplicity, main.c
     * will access the OutputBank of connected colorize node. */
    /* Actually, we'll read inputs in render pass via gathering.
     * Store normalized color in reserved outputs or just re-gather.
     * For MVP: render pass re-gathers inputs to this sink. */
    (void)inputs;  /* Color read separately in render pass */
}

/* ============================================================
 * NODE_TYPE_DEBUG: Pass-through for debugging
 * ============================================================ */
void node_eval_debug(const Node *node, const float inputs[MAX_IN_PORTS],
                     float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)node;
    (void)ctx;

    /* Pass all inputs to outputs */
    outputs[0] = inputs[0];
    outputs[1] = inputs[1];
    outputs[2] = inputs[2];
    outputs[3] = inputs[3];
}
