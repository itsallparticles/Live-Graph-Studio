/*
 * PS2 Live Graph Studio - Extended Node Implementations
 * node_extended.c - Additional node types for richer graphs
 */

#include "node_registry.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/* ============================================================
 * NODE_TYPE_NOISE: Pseudo-random noise generator
 * ============================================================
 * Uses a simple LCG seeded by time for cheap noise.
 * Output 0: Random value 0-1 (changes each frame)
 * Output 1: Smoothed noise
 * ============================================================ */
static uint32_t noise_rand(uint32_t *seed)
{
    *seed = (*seed * 1103515245u + 12345u) & 0x7fffffffu;
    return *seed;
}

void node_eval_noise(const Node *node, const float inputs[MAX_IN_PORTS],
                     float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    uint32_t *state = (uint32_t *)&((Node *)node)->state_u32[0];
    float *smooth = (float *)&((Node *)node)->state_u32[1];
    float speed = node->params[0];
    float raw, blend;
    (void)inputs;

    if (speed < 0.1f) speed = 1.0f;

    /* Seed with time on first call */
    if (*state == 0) {
        *state = (uint32_t)(ctx->time * 1000.0f) + 1;
    }

    /* Generate random value 0-1 */
    raw = (float)(noise_rand(state) & 0xFFFF) / 65535.0f;

    /* Smooth the noise */
    blend = 1.0f - expf(-speed * ctx->dt);
    *smooth = *smooth + (raw - *smooth) * blend;

    outputs[0] = raw;
    outputs[1] = *smooth;
    outputs[2] = raw * 2.0f - 1.0f;  /* Bipolar -1 to 1 */
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_LFO: Low-frequency oscillator
 * ============================================================
 * Params: freq, phase, shape (0=sin, 1=tri, 2=saw, 3=square)
 * ============================================================ */
void node_eval_lfo(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float freq = node->params[0];
    float phase = node->params[1];
    int shape = (int)node->params[2];
    float t, value;
    (void)inputs;

    if (freq < 0.001f) freq = 1.0f;

    t = fmodf(ctx->time * freq + phase, 1.0f);
    if (t < 0.0f) t += 1.0f;

    switch (shape) {
        case 1:  /* Triangle */
            value = t < 0.5f ? (t * 4.0f - 1.0f) : (3.0f - t * 4.0f);
            break;
        case 2:  /* Sawtooth */
            value = t * 2.0f - 1.0f;
            break;
        case 3:  /* Square */
            value = t < 0.5f ? 1.0f : -1.0f;
            break;
        default: /* Sine */
            value = sinf(t * 2.0f * M_PI);
            break;
    }

    outputs[0] = value;
    outputs[1] = (value + 1.0f) * 0.5f;  /* Unipolar 0-1 */
    outputs[2] = t;                       /* Phase 0-1 */
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_SUB: Subtract two inputs (a - b)
 * ============================================================ */
void node_eval_sub(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)node;
    (void)ctx;
    outputs[0] = inputs[0] - inputs[1];
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_DIV: Divide a / b (with safe divide-by-zero)
 * ============================================================ */
void node_eval_div(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float b = inputs[1];
    (void)node;
    (void)ctx;

    if (fabsf(b) < 0.0001f) {
        outputs[0] = 0.0f;
    } else {
        outputs[0] = inputs[0] / b;
    }
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_MOD: Modulo a % b
 * ============================================================ */
void node_eval_mod(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float b = inputs[1];
    (void)node;
    (void)ctx;

    if (fabsf(b) < 0.0001f) {
        outputs[0] = 0.0f;
    } else {
        outputs[0] = fmodf(inputs[0], b);
    }
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_ABS: Absolute value
 * ============================================================ */
void node_eval_abs(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)node;
    (void)ctx;
    outputs[0] = fabsf(inputs[0]);
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_NEG: Negate (multiply by -1)
 * ============================================================ */
void node_eval_neg(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)node;
    (void)ctx;
    outputs[0] = -inputs[0];
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_MIN: Minimum of two values
 * ============================================================ */
void node_eval_min(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)node;
    (void)ctx;
    outputs[0] = inputs[0] < inputs[1] ? inputs[0] : inputs[1];
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_MAX: Maximum of two values
 * ============================================================ */
void node_eval_max(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)node;
    (void)ctx;
    outputs[0] = inputs[0] > inputs[1] ? inputs[0] : inputs[1];
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_CLAMP: Clamp value to [min, max]
 * ============================================================ */
void node_eval_clamp(const Node *node, const float inputs[MAX_IN_PORTS],
                     float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float val = inputs[0];
    float lo = node->params[0];
    float hi = node->params[1];
    (void)ctx;

    if (val < lo) val = lo;
    if (val > hi) val = hi;
    outputs[0] = val;
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_MAP: Remap value from [in_min, in_max] to [out_min, out_max]
 * ============================================================ */
void node_eval_map(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float val = inputs[0];
    float in_min = node->params[0];
    float in_max = node->params[1];
    float out_min = node->params[2];
    float out_max = node->params[3];
    float t, result;
    (void)ctx;

    /* Normalize to 0-1 */
    if (fabsf(in_max - in_min) < 0.0001f) {
        t = 0.0f;
    } else {
        t = (val - in_min) / (in_max - in_min);
    }

    /* Map to output range */
    result = out_min + t * (out_max - out_min);
    outputs[0] = result;
    outputs[1] = t;  /* Also output normalized value */
    outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_COS: Cosine with freq/amp params
 * ============================================================ */
void node_eval_cos(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float freq = node->params[0];
    float amp = node->params[1];
    float angle;
    (void)ctx;

    if (freq == 0.0f) freq = 1.0f;
    if (amp == 0.0f) amp = 1.0f;

    angle = inputs[0] * freq;
    outputs[0] = cosf(angle) * amp;
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_TAN: Tangent
 * ============================================================ */
void node_eval_tan(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float val = tanf(inputs[0]);
    (void)node;
    (void)ctx;

    /* Clamp to prevent infinity */
    if (val > 1000.0f) val = 1000.0f;
    if (val < -1000.0f) val = -1000.0f;

    outputs[0] = val;
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_ATAN2: Arctangent of y/x, returns angle in radians
 * ============================================================ */
void node_eval_atan2(const Node *node, const float inputs[MAX_IN_PORTS],
                     float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float angle = atan2f(inputs[0], inputs[1]);
    (void)node;
    (void)ctx;

    outputs[0] = angle;
    outputs[1] = angle / M_PI;  /* Normalized -1 to 1 */
    outputs[2] = (angle + M_PI) / (2.0f * M_PI);  /* Normalized 0 to 1 */
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_STEP: Step function (threshold)
 * ============================================================ */
void node_eval_step(const Node *node, const float inputs[MAX_IN_PORTS],
                    float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float val = inputs[0];
    float threshold = node->params[0];
    float edge = node->params[1];  /* Softness */
    (void)ctx;

    if (edge < 0.001f) {
        /* Hard step */
        outputs[0] = val >= threshold ? 1.0f : 0.0f;
    } else {
        /* Smooth step */
        float t = (val - threshold + edge) / (2.0f * edge);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        t = t * t * (3.0f - 2.0f * t);  /* Smoothstep */
        outputs[0] = t;
    }
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_PULSE: Generate pulse when input crosses threshold
 * ============================================================ */
void node_eval_pulse(const Node *node, const float inputs[MAX_IN_PORTS],
                     float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float *prev = (float *)&((Node *)node)->state_u32[0];
    float *timer = (float *)&((Node *)node)->state_u32[1];
    float val = inputs[0];
    float threshold = node->params[0];
    float duration = node->params[1];
    int triggered = 0;

    if (duration < 0.01f) duration = 0.1f;

    /* Detect rising edge */
    if (val >= threshold && *prev < threshold) {
        triggered = 1;
        *timer = duration;
    }
    *prev = val;

    /* Count down timer */
    if (*timer > 0.0f) {
        outputs[0] = 1.0f;
        *timer -= ctx->dt;
    } else {
        outputs[0] = 0.0f;
    }

    outputs[1] = triggered ? 1.0f : 0.0f;  /* Instant trigger */
    outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_HOLD: Sample and hold
 * ============================================================ */
void node_eval_hold(const Node *node, const float inputs[MAX_IN_PORTS],
                    float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float *held = (float *)&((Node *)node)->state_u32[0];
    float *prev_trigger = (float *)&((Node *)node)->state_u32[1];
    float val = inputs[0];
    float trigger = inputs[1];
    float threshold = node->params[0];
    (void)ctx;

    /* Sample on rising edge of trigger */
    if (trigger >= threshold && *prev_trigger < threshold) {
        *held = val;
    }
    *prev_trigger = trigger;

    outputs[0] = *held;
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_DELAY: Delay signal by N frames (ring buffer)
 * ============================================================ */
#define DELAY_MAX_FRAMES 16
void node_eval_delay(const Node *node, const float inputs[MAX_IN_PORTS],
                     float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    /* Use state_u32 as buffer: [0]=write index, [1-16]=samples */
    uint32_t *state = ((Node *)node)->state_u32;
    int delay_frames = (int)node->params[0];
    int write_idx, read_idx;
    float *buffer = (float *)&state[1];
    (void)ctx;

    if (delay_frames < 1) delay_frames = 1;
    if (delay_frames > DELAY_MAX_FRAMES) delay_frames = DELAY_MAX_FRAMES;

    write_idx = (int)state[0];
    read_idx = (write_idx - delay_frames + DELAY_MAX_FRAMES) % DELAY_MAX_FRAMES;

    outputs[0] = buffer[read_idx];
    buffer[write_idx] = inputs[0];
    state[0] = (write_idx + 1) % DELAY_MAX_FRAMES;

    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_COMPARE: Compare two values
 * Output 0: 1 if condition true, 0 otherwise
 * Param 0: comparison mode (0=<, 1=<=, 2==, 3=>=, 4=>)
 * ============================================================ */
void node_eval_compare(const Node *node, const float inputs[MAX_IN_PORTS],
                       float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float a = inputs[0];
    float b = inputs[1];
    int mode = (int)node->params[0];
    int result = 0;
    (void)ctx;

    switch (mode) {
        case 0: result = (a < b); break;
        case 1: result = (a <= b); break;
        case 2: result = (fabsf(a - b) < 0.0001f); break;
        case 3: result = (a >= b); break;
        case 4: result = (a > b); break;
        default: result = 0; break;
    }

    outputs[0] = result ? 1.0f : 0.0f;
    outputs[1] = a - b;  /* Difference */
    outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_SELECT: Select between two inputs based on condition
 * ============================================================ */
void node_eval_select(const Node *node, const float inputs[MAX_IN_PORTS],
                      float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float a = inputs[0];
    float b = inputs[1];
    float cond = inputs[2];
    float threshold = node->params[0];
    (void)ctx;

    outputs[0] = (cond >= threshold) ? b : a;
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_GATE: Gate signal by threshold
 * Output = input if gate >= threshold, else 0
 * ============================================================ */
void node_eval_gate(const Node *node, const float inputs[MAX_IN_PORTS],
                    float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float val = inputs[0];
    float gate = inputs[1];
    float threshold = node->params[0];
    (void)ctx;

    outputs[0] = (gate >= threshold) ? val : 0.0f;
    outputs[1] = outputs[2] = outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_SPLIT: Split input to 4 outputs (pass-through)
 * ============================================================ */
void node_eval_split(const Node *node, const float inputs[MAX_IN_PORTS],
                     float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)node;
    (void)ctx;
    outputs[0] = inputs[0];
    outputs[1] = inputs[0];
    outputs[2] = inputs[0];
    outputs[3] = inputs[0];
}

/* ============================================================
 * NODE_TYPE_COMBINE: Combine 4 inputs (pack for debugging)
 * ============================================================ */
void node_eval_combine(const Node *node, const float inputs[MAX_IN_PORTS],
                       float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)node;
    (void)ctx;
    outputs[0] = inputs[0];
    outputs[1] = inputs[1];
    outputs[2] = inputs[2];
    outputs[3] = inputs[3];
}

/* ============================================================
 * NODE_TYPE_HSV: HSV to RGB conversion
 * Inputs: H (0-1), S (0-1), V (0-1)
 * Outputs: R, G, B (0-1)
 * ============================================================ */
void node_eval_hsv(const Node *node, const float inputs[MAX_IN_PORTS],
                   float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float h = inputs[0];
    float s = inputs[1];
    float v = inputs[2];
    float c, x, m;
    float r = 0, g = 0, b = 0;
    int hi;
    (void)node;
    (void)ctx;

    /* Wrap hue to 0-1 */
    h = fmodf(h, 1.0f);
    if (h < 0.0f) h += 1.0f;

    /* Clamp s and v */
    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    c = v * s;
    hi = (int)(h * 6.0f);
    x = c * (1.0f - fabsf(fmodf(h * 6.0f, 2.0f) - 1.0f));
    m = v - c;

    switch (hi % 6) {
        case 0: r = c; g = x; b = 0; break;
        case 1: r = x; g = c; b = 0; break;
        case 2: r = 0; g = c; b = x; break;
        case 3: r = 0; g = x; b = c; break;
        case 4: r = x; g = 0; b = c; break;
        case 5: r = c; g = 0; b = x; break;
    }

    outputs[0] = r + m;
    outputs[1] = g + m;
    outputs[2] = b + m;
    outputs[3] = 1.0f;  /* Alpha */
}

/* ============================================================
 * NODE_TYPE_GRADIENT: Multi-stop gradient (3 colors)
 * Params: r1,g1,b1, r2,g2,b2 (start and end colors)
 * Input: t (0-1 position)
 * ============================================================ */
void node_eval_gradient(const Node *node, const float inputs[MAX_IN_PORTS],
                        float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    float t = inputs[0];
    float r1 = node->params[0], g1 = node->params[1], b1 = node->params[2];
    float r2 = node->params[3], g2 = node->params[4], b2 = node->params[5];
    (void)ctx;

    /* Clamp t */
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    outputs[0] = r1 + (r2 - r1) * t;
    outputs[1] = g1 + (g2 - g1) * t;
    outputs[2] = b1 + (b2 - b1) * t;
    outputs[3] = 1.0f;
}

/* ============================================================
 * NODE_TYPE_RENDER_CIRCLE: Render a filled circle
 * Inputs: R, G, B, A
 * Params: X, Y, radius
 * ============================================================ */
void node_eval_render_circle(const Node *node, const float inputs[MAX_IN_PORTS],
                             float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)ctx;
    (void)inputs;

    /* Output geometry from params */
    outputs[0] = node->params[0];  /* X center */
    outputs[1] = node->params[1];  /* Y center */
    outputs[2] = node->params[2];  /* Radius */
    outputs[3] = 0.0f;
}

/* ============================================================
 * NODE_TYPE_RENDER_LINE: Render a line
 * Inputs: R, G, B, A
 * Params: x1, y1, x2, y2
 * ============================================================ */
void node_eval_render_line(const Node *node, const float inputs[MAX_IN_PORTS],
                           float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    (void)ctx;
    (void)inputs;

    outputs[0] = node->params[0];  /* X1 */
    outputs[1] = node->params[1];  /* Y1 */
    outputs[2] = node->params[2];  /* X2 */
    outputs[3] = node->params[3];  /* Y2 */
}
