#include "node_registry.h"
#include <string.h>

/* ============================================================
 * Forward declarations for node eval functions
 * Implemented in node_basic.c or other node implementation files
 * ============================================================ */
/* Basic nodes (node_basic.c) */
extern void node_eval_const(const Node *node, const float inputs[MAX_IN_PORTS],
                            float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_time(const Node *node, const float inputs[MAX_IN_PORTS],
                           float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_pad(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_add(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_mul(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_sin(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_lerp(const Node *node, const float inputs[MAX_IN_PORTS],
                           float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_smooth(const Node *node, const float inputs[MAX_IN_PORTS],
                             float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_colorize(const Node *node, const float inputs[MAX_IN_PORTS],
                               float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_transform2d(const Node *node, const float inputs[MAX_IN_PORTS],
                                  float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_render2d(const Node *node, const float inputs[MAX_IN_PORTS],
                               float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_debug(const Node *node, const float inputs[MAX_IN_PORTS],
                            float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);

/* Extended nodes (node_extended.c) */
extern void node_eval_noise(const Node *node, const float inputs[MAX_IN_PORTS],
                            float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_lfo(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_sub(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_div(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_mod(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_abs(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_neg(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_min(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_max(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_clamp(const Node *node, const float inputs[MAX_IN_PORTS],
                            float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_map(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_cos(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_tan(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_atan2(const Node *node, const float inputs[MAX_IN_PORTS],
                            float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_step(const Node *node, const float inputs[MAX_IN_PORTS],
                           float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_pulse(const Node *node, const float inputs[MAX_IN_PORTS],
                            float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_hold(const Node *node, const float inputs[MAX_IN_PORTS],
                           float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_delay(const Node *node, const float inputs[MAX_IN_PORTS],
                            float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_compare(const Node *node, const float inputs[MAX_IN_PORTS],
                              float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_select(const Node *node, const float inputs[MAX_IN_PORTS],
                             float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_gate(const Node *node, const float inputs[MAX_IN_PORTS],
                           float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_split(const Node *node, const float inputs[MAX_IN_PORTS],
                            float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_combine(const Node *node, const float inputs[MAX_IN_PORTS],
                              float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_hsv(const Node *node, const float inputs[MAX_IN_PORTS],
                          float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_gradient(const Node *node, const float inputs[MAX_IN_PORTS],
                               float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_render_circle(const Node *node, const float inputs[MAX_IN_PORTS],
                                    float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);
extern void node_eval_render_line(const Node *node, const float inputs[MAX_IN_PORTS],
                                  float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx);

/* ============================================================
 * Fallback: Unimplemented node outputs zeros
 * ============================================================ */
static void node_eval_none(const Node *node, const float inputs[MAX_IN_PORTS],
                           float outputs[MAX_OUT_PORTS], const RuntimeContext *ctx)
{
    int i;
    (void)node;
    (void)inputs;
    (void)ctx;

    for (i = 0; i < MAX_OUT_PORTS; i++) {
        outputs[i] = 0.0f;
    }
}

/* ============================================================
 * Registry Tables
 * ============================================================
 * Memory usage:
 *   s_eval_funcs: ~52 bytes (NODE_TYPE_COUNT * sizeof(ptr))
 *   s_meta: ~5.4KB (NODE_TYPE_COUNT * sizeof(NodeMeta))
 * ============================================================ */
static NodeEvalFunc s_eval_funcs[NODE_TYPE_COUNT];
static NodeMeta     s_meta[NODE_TYPE_COUNT];
static int          s_initialized = 0;

/* ============================================================
 * Initialize Metadata for Each Node Type
 * ============================================================ */
static void init_meta(void)
{
    int i, j;

    /* Zero all metadata first (sets pointers to NULL).
     * The following loop then sets valid default values.
     * Order matters: memset must come before the loop. */
    memset(s_meta, 0, sizeof(s_meta));

    /* Initialize default names and param ranges */
    for (i = 0; i < NODE_TYPE_COUNT; i++) {
        s_meta[i].name = "Unknown";
        for (j = 0; j < MAX_PARAMS; j++) {
            s_meta[i].param_min[j] = -1000.0f;
            s_meta[i].param_max[j] = 1000.0f;
        }
    }

    /* NODE_TYPE_NONE */
    s_meta[NODE_TYPE_NONE].name = "None";

    /* NODE_TYPE_CONST */
    s_meta[NODE_TYPE_CONST].name = "Const";
    s_meta[NODE_TYPE_CONST].num_inputs = 0;
    s_meta[NODE_TYPE_CONST].num_outputs = 1;
    s_meta[NODE_TYPE_CONST].num_params = 1;
    s_meta[NODE_TYPE_CONST].output_names[0] = "value";
    s_meta[NODE_TYPE_CONST].param_names[0] = "value";
    s_meta[NODE_TYPE_CONST].param_defaults[0] = 0.0f;

    /* NODE_TYPE_TIME */
    s_meta[NODE_TYPE_TIME].name = "Time";
    s_meta[NODE_TYPE_TIME].num_inputs = 0;
    s_meta[NODE_TYPE_TIME].num_outputs = 2;
    s_meta[NODE_TYPE_TIME].num_params = 1;
    s_meta[NODE_TYPE_TIME].output_names[0] = "time";
    s_meta[NODE_TYPE_TIME].output_names[1] = "dt";
    s_meta[NODE_TYPE_TIME].param_names[0] = "scale";
    s_meta[NODE_TYPE_TIME].param_defaults[0] = 1.0f;

    /* NODE_TYPE_PAD */
    s_meta[NODE_TYPE_PAD].name = "Pad";
    s_meta[NODE_TYPE_PAD].num_inputs = 0;
    s_meta[NODE_TYPE_PAD].num_outputs = 4;
    s_meta[NODE_TYPE_PAD].num_params = 1;
    s_meta[NODE_TYPE_PAD].output_names[0] = "lx";
    s_meta[NODE_TYPE_PAD].output_names[1] = "ly";
    s_meta[NODE_TYPE_PAD].output_names[2] = "rx";
    s_meta[NODE_TYPE_PAD].output_names[3] = "ry";
    s_meta[NODE_TYPE_PAD].param_names[0] = "channel";
    s_meta[NODE_TYPE_PAD].param_defaults[0] = 0.0f;
    s_meta[NODE_TYPE_PAD].param_min[0] = 0.0f;
    s_meta[NODE_TYPE_PAD].param_max[0] = 3.0f;

    /* NODE_TYPE_NOISE */
    s_meta[NODE_TYPE_NOISE].name = "Noise";
    s_meta[NODE_TYPE_NOISE].num_inputs = 0;
    s_meta[NODE_TYPE_NOISE].num_outputs = 3;
    s_meta[NODE_TYPE_NOISE].num_params = 1;
    s_meta[NODE_TYPE_NOISE].output_names[0] = "raw";
    s_meta[NODE_TYPE_NOISE].output_names[1] = "smooth";
    s_meta[NODE_TYPE_NOISE].output_names[2] = "bipolar";
    s_meta[NODE_TYPE_NOISE].param_names[0] = "speed";
    s_meta[NODE_TYPE_NOISE].param_defaults[0] = 5.0f;
    s_meta[NODE_TYPE_NOISE].param_min[0] = 0.1f;
    s_meta[NODE_TYPE_NOISE].param_max[0] = 50.0f;

    /* NODE_TYPE_LFO */
    s_meta[NODE_TYPE_LFO].name = "LFO";
    s_meta[NODE_TYPE_LFO].num_inputs = 0;
    s_meta[NODE_TYPE_LFO].num_outputs = 3;
    s_meta[NODE_TYPE_LFO].num_params = 3;
    s_meta[NODE_TYPE_LFO].output_names[0] = "value";
    s_meta[NODE_TYPE_LFO].output_names[1] = "uni";
    s_meta[NODE_TYPE_LFO].output_names[2] = "phase";
    s_meta[NODE_TYPE_LFO].param_names[0] = "freq";
    s_meta[NODE_TYPE_LFO].param_names[1] = "phase";
    s_meta[NODE_TYPE_LFO].param_names[2] = "shape";
    s_meta[NODE_TYPE_LFO].param_defaults[0] = 1.0f;
    s_meta[NODE_TYPE_LFO].param_defaults[1] = 0.0f;
    s_meta[NODE_TYPE_LFO].param_defaults[2] = 0.0f;
    s_meta[NODE_TYPE_LFO].param_min[0] = 0.01f;
    s_meta[NODE_TYPE_LFO].param_max[0] = 20.0f;
    s_meta[NODE_TYPE_LFO].param_min[2] = 0.0f;
    s_meta[NODE_TYPE_LFO].param_max[2] = 3.0f;

    /* NODE_TYPE_ADD */
    s_meta[NODE_TYPE_ADD].name = "Add";
    s_meta[NODE_TYPE_ADD].num_inputs = 2;
    s_meta[NODE_TYPE_ADD].num_outputs = 1;
    s_meta[NODE_TYPE_ADD].num_params = 0;
    s_meta[NODE_TYPE_ADD].input_names[0] = "a";
    s_meta[NODE_TYPE_ADD].input_names[1] = "b";
    s_meta[NODE_TYPE_ADD].output_names[0] = "sum";

    /* NODE_TYPE_MUL */
    s_meta[NODE_TYPE_MUL].name = "Mul";
    s_meta[NODE_TYPE_MUL].num_inputs = 2;
    s_meta[NODE_TYPE_MUL].num_outputs = 1;
    s_meta[NODE_TYPE_MUL].num_params = 0;
    s_meta[NODE_TYPE_MUL].input_names[0] = "a";
    s_meta[NODE_TYPE_MUL].input_names[1] = "b";
    s_meta[NODE_TYPE_MUL].output_names[0] = "product";

    /* NODE_TYPE_SUB */
    s_meta[NODE_TYPE_SUB].name = "Sub";
    s_meta[NODE_TYPE_SUB].num_inputs = 2;
    s_meta[NODE_TYPE_SUB].num_outputs = 1;
    s_meta[NODE_TYPE_SUB].num_params = 0;
    s_meta[NODE_TYPE_SUB].input_names[0] = "a";
    s_meta[NODE_TYPE_SUB].input_names[1] = "b";
    s_meta[NODE_TYPE_SUB].output_names[0] = "diff";

    /* NODE_TYPE_DIV */
    s_meta[NODE_TYPE_DIV].name = "Div";
    s_meta[NODE_TYPE_DIV].num_inputs = 2;
    s_meta[NODE_TYPE_DIV].num_outputs = 1;
    s_meta[NODE_TYPE_DIV].num_params = 0;
    s_meta[NODE_TYPE_DIV].input_names[0] = "a";
    s_meta[NODE_TYPE_DIV].input_names[1] = "b";
    s_meta[NODE_TYPE_DIV].output_names[0] = "quot";

    /* NODE_TYPE_MOD */
    s_meta[NODE_TYPE_MOD].name = "Mod";
    s_meta[NODE_TYPE_MOD].num_inputs = 2;
    s_meta[NODE_TYPE_MOD].num_outputs = 1;
    s_meta[NODE_TYPE_MOD].num_params = 0;
    s_meta[NODE_TYPE_MOD].input_names[0] = "a";
    s_meta[NODE_TYPE_MOD].input_names[1] = "b";
    s_meta[NODE_TYPE_MOD].output_names[0] = "rem";

    /* NODE_TYPE_ABS */
    s_meta[NODE_TYPE_ABS].name = "Abs";
    s_meta[NODE_TYPE_ABS].num_inputs = 1;
    s_meta[NODE_TYPE_ABS].num_outputs = 1;
    s_meta[NODE_TYPE_ABS].num_params = 0;
    s_meta[NODE_TYPE_ABS].input_names[0] = "in";
    s_meta[NODE_TYPE_ABS].output_names[0] = "out";

    /* NODE_TYPE_NEG */
    s_meta[NODE_TYPE_NEG].name = "Neg";
    s_meta[NODE_TYPE_NEG].num_inputs = 1;
    s_meta[NODE_TYPE_NEG].num_outputs = 1;
    s_meta[NODE_TYPE_NEG].num_params = 0;
    s_meta[NODE_TYPE_NEG].input_names[0] = "in";
    s_meta[NODE_TYPE_NEG].output_names[0] = "out";

    /* NODE_TYPE_MIN */
    s_meta[NODE_TYPE_MIN].name = "Min";
    s_meta[NODE_TYPE_MIN].num_inputs = 2;
    s_meta[NODE_TYPE_MIN].num_outputs = 1;
    s_meta[NODE_TYPE_MIN].num_params = 0;
    s_meta[NODE_TYPE_MIN].input_names[0] = "a";
    s_meta[NODE_TYPE_MIN].input_names[1] = "b";
    s_meta[NODE_TYPE_MIN].output_names[0] = "min";

    /* NODE_TYPE_MAX */
    s_meta[NODE_TYPE_MAX].name = "Max";
    s_meta[NODE_TYPE_MAX].num_inputs = 2;
    s_meta[NODE_TYPE_MAX].num_outputs = 1;
    s_meta[NODE_TYPE_MAX].num_params = 0;
    s_meta[NODE_TYPE_MAX].input_names[0] = "a";
    s_meta[NODE_TYPE_MAX].input_names[1] = "b";
    s_meta[NODE_TYPE_MAX].output_names[0] = "max";

    /* NODE_TYPE_CLAMP */
    s_meta[NODE_TYPE_CLAMP].name = "Clamp";
    s_meta[NODE_TYPE_CLAMP].num_inputs = 1;
    s_meta[NODE_TYPE_CLAMP].num_outputs = 1;
    s_meta[NODE_TYPE_CLAMP].num_params = 2;
    s_meta[NODE_TYPE_CLAMP].input_names[0] = "in";
    s_meta[NODE_TYPE_CLAMP].output_names[0] = "out";
    s_meta[NODE_TYPE_CLAMP].param_names[0] = "min";
    s_meta[NODE_TYPE_CLAMP].param_names[1] = "max";
    s_meta[NODE_TYPE_CLAMP].param_defaults[0] = 0.0f;
    s_meta[NODE_TYPE_CLAMP].param_defaults[1] = 1.0f;

    /* NODE_TYPE_MAP */
    s_meta[NODE_TYPE_MAP].name = "Map";
    s_meta[NODE_TYPE_MAP].num_inputs = 1;
    s_meta[NODE_TYPE_MAP].num_outputs = 2;
    s_meta[NODE_TYPE_MAP].num_params = 4;
    s_meta[NODE_TYPE_MAP].input_names[0] = "in";
    s_meta[NODE_TYPE_MAP].output_names[0] = "out";
    s_meta[NODE_TYPE_MAP].output_names[1] = "norm";
    s_meta[NODE_TYPE_MAP].param_names[0] = "in_min";
    s_meta[NODE_TYPE_MAP].param_names[1] = "in_max";
    s_meta[NODE_TYPE_MAP].param_names[2] = "out_min";
    s_meta[NODE_TYPE_MAP].param_names[3] = "out_max";
    s_meta[NODE_TYPE_MAP].param_defaults[0] = 0.0f;
    s_meta[NODE_TYPE_MAP].param_defaults[1] = 1.0f;
    s_meta[NODE_TYPE_MAP].param_defaults[2] = 0.0f;
    s_meta[NODE_TYPE_MAP].param_defaults[3] = 1.0f;

    /* NODE_TYPE_SIN */
    s_meta[NODE_TYPE_SIN].name = "Sin";
    s_meta[NODE_TYPE_SIN].num_inputs = 1;
    s_meta[NODE_TYPE_SIN].num_outputs = 1;
    s_meta[NODE_TYPE_SIN].num_params = 2;
    s_meta[NODE_TYPE_SIN].input_names[0] = "angle";
    s_meta[NODE_TYPE_SIN].output_names[0] = "value";
    s_meta[NODE_TYPE_SIN].param_names[0] = "freq";
    s_meta[NODE_TYPE_SIN].param_names[1] = "amp";
    s_meta[NODE_TYPE_SIN].param_defaults[0] = 1.0f;
    s_meta[NODE_TYPE_SIN].param_defaults[1] = 1.0f;

    /* NODE_TYPE_COS */
    s_meta[NODE_TYPE_COS].name = "Cos";
    s_meta[NODE_TYPE_COS].num_inputs = 1;
    s_meta[NODE_TYPE_COS].num_outputs = 1;
    s_meta[NODE_TYPE_COS].num_params = 2;
    s_meta[NODE_TYPE_COS].input_names[0] = "angle";
    s_meta[NODE_TYPE_COS].output_names[0] = "value";
    s_meta[NODE_TYPE_COS].param_names[0] = "freq";
    s_meta[NODE_TYPE_COS].param_names[1] = "amp";
    s_meta[NODE_TYPE_COS].param_defaults[0] = 1.0f;
    s_meta[NODE_TYPE_COS].param_defaults[1] = 1.0f;

    /* NODE_TYPE_TAN */
    s_meta[NODE_TYPE_TAN].name = "Tan";
    s_meta[NODE_TYPE_TAN].num_inputs = 1;
    s_meta[NODE_TYPE_TAN].num_outputs = 1;
    s_meta[NODE_TYPE_TAN].num_params = 0;
    s_meta[NODE_TYPE_TAN].input_names[0] = "angle";
    s_meta[NODE_TYPE_TAN].output_names[0] = "value";

    /* NODE_TYPE_ATAN2 */
    s_meta[NODE_TYPE_ATAN2].name = "Atan2";
    s_meta[NODE_TYPE_ATAN2].num_inputs = 2;
    s_meta[NODE_TYPE_ATAN2].num_outputs = 3;
    s_meta[NODE_TYPE_ATAN2].num_params = 0;
    s_meta[NODE_TYPE_ATAN2].input_names[0] = "y";
    s_meta[NODE_TYPE_ATAN2].input_names[1] = "x";
    s_meta[NODE_TYPE_ATAN2].output_names[0] = "rad";
    s_meta[NODE_TYPE_ATAN2].output_names[1] = "norm";
    s_meta[NODE_TYPE_ATAN2].output_names[2] = "uni";

    /* NODE_TYPE_LERP */
    s_meta[NODE_TYPE_LERP].name = "Lerp";
    s_meta[NODE_TYPE_LERP].num_inputs = 3;
    s_meta[NODE_TYPE_LERP].num_outputs = 1;
    s_meta[NODE_TYPE_LERP].num_params = 0;
    s_meta[NODE_TYPE_LERP].input_names[0] = "a";
    s_meta[NODE_TYPE_LERP].input_names[1] = "b";
    s_meta[NODE_TYPE_LERP].input_names[2] = "t";
    s_meta[NODE_TYPE_LERP].output_names[0] = "value";

    /* NODE_TYPE_SMOOTH */
    s_meta[NODE_TYPE_SMOOTH].name = "Smooth";
    s_meta[NODE_TYPE_SMOOTH].num_inputs = 1;
    s_meta[NODE_TYPE_SMOOTH].num_outputs = 1;
    s_meta[NODE_TYPE_SMOOTH].num_params = 1;
    s_meta[NODE_TYPE_SMOOTH].input_names[0] = "input";
    s_meta[NODE_TYPE_SMOOTH].output_names[0] = "output";
    s_meta[NODE_TYPE_SMOOTH].param_names[0] = "speed";
    s_meta[NODE_TYPE_SMOOTH].param_defaults[0] = 5.0f;
    s_meta[NODE_TYPE_SMOOTH].param_min[0] = 0.1f;
    s_meta[NODE_TYPE_SMOOTH].param_max[0] = 100.0f;

    /* NODE_TYPE_STEP */
    s_meta[NODE_TYPE_STEP].name = "Step";
    s_meta[NODE_TYPE_STEP].num_inputs = 1;
    s_meta[NODE_TYPE_STEP].num_outputs = 1;
    s_meta[NODE_TYPE_STEP].num_params = 2;
    s_meta[NODE_TYPE_STEP].input_names[0] = "in";
    s_meta[NODE_TYPE_STEP].output_names[0] = "out";
    s_meta[NODE_TYPE_STEP].param_names[0] = "threshold";
    s_meta[NODE_TYPE_STEP].param_names[1] = "edge";
    s_meta[NODE_TYPE_STEP].param_defaults[0] = 0.5f;
    s_meta[NODE_TYPE_STEP].param_defaults[1] = 0.0f;

    /* NODE_TYPE_PULSE */
    s_meta[NODE_TYPE_PULSE].name = "Pulse";
    s_meta[NODE_TYPE_PULSE].num_inputs = 1;
    s_meta[NODE_TYPE_PULSE].num_outputs = 2;
    s_meta[NODE_TYPE_PULSE].num_params = 2;
    s_meta[NODE_TYPE_PULSE].input_names[0] = "trigger";
    s_meta[NODE_TYPE_PULSE].output_names[0] = "pulse";
    s_meta[NODE_TYPE_PULSE].output_names[1] = "edge";
    s_meta[NODE_TYPE_PULSE].param_names[0] = "threshold";
    s_meta[NODE_TYPE_PULSE].param_names[1] = "duration";
    s_meta[NODE_TYPE_PULSE].param_defaults[0] = 0.5f;
    s_meta[NODE_TYPE_PULSE].param_defaults[1] = 0.1f;
    s_meta[NODE_TYPE_PULSE].param_min[1] = 0.01f;
    s_meta[NODE_TYPE_PULSE].param_max[1] = 5.0f;

    /* NODE_TYPE_HOLD */
    s_meta[NODE_TYPE_HOLD].name = "Hold";
    s_meta[NODE_TYPE_HOLD].num_inputs = 2;
    s_meta[NODE_TYPE_HOLD].num_outputs = 1;
    s_meta[NODE_TYPE_HOLD].num_params = 1;
    s_meta[NODE_TYPE_HOLD].input_names[0] = "value";
    s_meta[NODE_TYPE_HOLD].input_names[1] = "trigger";
    s_meta[NODE_TYPE_HOLD].output_names[0] = "held";
    s_meta[NODE_TYPE_HOLD].param_names[0] = "threshold";
    s_meta[NODE_TYPE_HOLD].param_defaults[0] = 0.5f;

    /* NODE_TYPE_DELAY */
    s_meta[NODE_TYPE_DELAY].name = "Delay";
    s_meta[NODE_TYPE_DELAY].num_inputs = 1;
    s_meta[NODE_TYPE_DELAY].num_outputs = 1;
    s_meta[NODE_TYPE_DELAY].num_params = 1;
    s_meta[NODE_TYPE_DELAY].input_names[0] = "in";
    s_meta[NODE_TYPE_DELAY].output_names[0] = "delayed";
    s_meta[NODE_TYPE_DELAY].param_names[0] = "frames";
    s_meta[NODE_TYPE_DELAY].param_defaults[0] = 5.0f;
    s_meta[NODE_TYPE_DELAY].param_min[0] = 1.0f;
    s_meta[NODE_TYPE_DELAY].param_max[0] = 16.0f;

    /* NODE_TYPE_COMPARE */
    s_meta[NODE_TYPE_COMPARE].name = "Compare";
    s_meta[NODE_TYPE_COMPARE].num_inputs = 2;
    s_meta[NODE_TYPE_COMPARE].num_outputs = 2;
    s_meta[NODE_TYPE_COMPARE].num_params = 1;
    s_meta[NODE_TYPE_COMPARE].input_names[0] = "a";
    s_meta[NODE_TYPE_COMPARE].input_names[1] = "b";
    s_meta[NODE_TYPE_COMPARE].output_names[0] = "result";
    s_meta[NODE_TYPE_COMPARE].output_names[1] = "diff";
    s_meta[NODE_TYPE_COMPARE].param_names[0] = "mode";
    s_meta[NODE_TYPE_COMPARE].param_defaults[0] = 0.0f;
    s_meta[NODE_TYPE_COMPARE].param_min[0] = 0.0f;
    s_meta[NODE_TYPE_COMPARE].param_max[0] = 4.0f;

    /* NODE_TYPE_SELECT */
    s_meta[NODE_TYPE_SELECT].name = "Select";
    s_meta[NODE_TYPE_SELECT].num_inputs = 3;
    s_meta[NODE_TYPE_SELECT].num_outputs = 1;
    s_meta[NODE_TYPE_SELECT].num_params = 1;
    s_meta[NODE_TYPE_SELECT].input_names[0] = "a";
    s_meta[NODE_TYPE_SELECT].input_names[1] = "b";
    s_meta[NODE_TYPE_SELECT].input_names[2] = "cond";
    s_meta[NODE_TYPE_SELECT].output_names[0] = "out";
    s_meta[NODE_TYPE_SELECT].param_names[0] = "threshold";
    s_meta[NODE_TYPE_SELECT].param_defaults[0] = 0.5f;

    /* NODE_TYPE_GATE */
    s_meta[NODE_TYPE_GATE].name = "Gate";
    s_meta[NODE_TYPE_GATE].num_inputs = 2;
    s_meta[NODE_TYPE_GATE].num_outputs = 1;
    s_meta[NODE_TYPE_GATE].num_params = 1;
    s_meta[NODE_TYPE_GATE].input_names[0] = "signal";
    s_meta[NODE_TYPE_GATE].input_names[1] = "gate";
    s_meta[NODE_TYPE_GATE].output_names[0] = "out";
    s_meta[NODE_TYPE_GATE].param_names[0] = "threshold";
    s_meta[NODE_TYPE_GATE].param_defaults[0] = 0.5f;

    /* NODE_TYPE_SPLIT */
    s_meta[NODE_TYPE_SPLIT].name = "Split";
    s_meta[NODE_TYPE_SPLIT].num_inputs = 1;
    s_meta[NODE_TYPE_SPLIT].num_outputs = 4;
    s_meta[NODE_TYPE_SPLIT].num_params = 0;
    s_meta[NODE_TYPE_SPLIT].input_names[0] = "in";
    s_meta[NODE_TYPE_SPLIT].output_names[0] = "out0";
    s_meta[NODE_TYPE_SPLIT].output_names[1] = "out1";
    s_meta[NODE_TYPE_SPLIT].output_names[2] = "out2";
    s_meta[NODE_TYPE_SPLIT].output_names[3] = "out3";

    /* NODE_TYPE_COMBINE */
    s_meta[NODE_TYPE_COMBINE].name = "Combine";
    s_meta[NODE_TYPE_COMBINE].num_inputs = 4;
    s_meta[NODE_TYPE_COMBINE].num_outputs = 4;
    s_meta[NODE_TYPE_COMBINE].num_params = 0;
    s_meta[NODE_TYPE_COMBINE].input_names[0] = "in0";
    s_meta[NODE_TYPE_COMBINE].input_names[1] = "in1";
    s_meta[NODE_TYPE_COMBINE].input_names[2] = "in2";
    s_meta[NODE_TYPE_COMBINE].input_names[3] = "in3";
    s_meta[NODE_TYPE_COMBINE].output_names[0] = "out0";
    s_meta[NODE_TYPE_COMBINE].output_names[1] = "out1";
    s_meta[NODE_TYPE_COMBINE].output_names[2] = "out2";
    s_meta[NODE_TYPE_COMBINE].output_names[3] = "out3";

    /* NODE_TYPE_COLORIZE */
    s_meta[NODE_TYPE_COLORIZE].name = "Colorize";
    s_meta[NODE_TYPE_COLORIZE].num_inputs = 1;
    s_meta[NODE_TYPE_COLORIZE].num_outputs = 3;
    s_meta[NODE_TYPE_COLORIZE].num_params = 3;
    s_meta[NODE_TYPE_COLORIZE].input_names[0] = "value";
    s_meta[NODE_TYPE_COLORIZE].output_names[0] = "r";
    s_meta[NODE_TYPE_COLORIZE].output_names[1] = "g";
    s_meta[NODE_TYPE_COLORIZE].output_names[2] = "b";
    s_meta[NODE_TYPE_COLORIZE].param_names[0] = "base_r";
    s_meta[NODE_TYPE_COLORIZE].param_names[1] = "base_g";
    s_meta[NODE_TYPE_COLORIZE].param_names[2] = "base_b";
    s_meta[NODE_TYPE_COLORIZE].param_defaults[0] = 1.0f;
    s_meta[NODE_TYPE_COLORIZE].param_defaults[1] = 1.0f;
    s_meta[NODE_TYPE_COLORIZE].param_defaults[2] = 1.0f;
    s_meta[NODE_TYPE_COLORIZE].param_min[0] = 0.0f;
    s_meta[NODE_TYPE_COLORIZE].param_min[1] = 0.0f;
    s_meta[NODE_TYPE_COLORIZE].param_min[2] = 0.0f;
    s_meta[NODE_TYPE_COLORIZE].param_max[0] = 1.0f;
    s_meta[NODE_TYPE_COLORIZE].param_max[1] = 1.0f;
    s_meta[NODE_TYPE_COLORIZE].param_max[2] = 1.0f;

    /* NODE_TYPE_HSV */
    s_meta[NODE_TYPE_HSV].name = "HSV";
    s_meta[NODE_TYPE_HSV].num_inputs = 3;
    s_meta[NODE_TYPE_HSV].num_outputs = 4;
    s_meta[NODE_TYPE_HSV].num_params = 0;
    s_meta[NODE_TYPE_HSV].input_names[0] = "H";
    s_meta[NODE_TYPE_HSV].input_names[1] = "S";
    s_meta[NODE_TYPE_HSV].input_names[2] = "V";
    s_meta[NODE_TYPE_HSV].output_names[0] = "R";
    s_meta[NODE_TYPE_HSV].output_names[1] = "G";
    s_meta[NODE_TYPE_HSV].output_names[2] = "B";
    s_meta[NODE_TYPE_HSV].output_names[3] = "A";

    /* NODE_TYPE_GRADIENT */
    s_meta[NODE_TYPE_GRADIENT].name = "Gradient";
    s_meta[NODE_TYPE_GRADIENT].num_inputs = 1;
    s_meta[NODE_TYPE_GRADIENT].num_outputs = 4;
    s_meta[NODE_TYPE_GRADIENT].num_params = 6;
    s_meta[NODE_TYPE_GRADIENT].input_names[0] = "t";
    s_meta[NODE_TYPE_GRADIENT].output_names[0] = "R";
    s_meta[NODE_TYPE_GRADIENT].output_names[1] = "G";
    s_meta[NODE_TYPE_GRADIENT].output_names[2] = "B";
    s_meta[NODE_TYPE_GRADIENT].output_names[3] = "A";
    s_meta[NODE_TYPE_GRADIENT].param_names[0] = "r1";
    s_meta[NODE_TYPE_GRADIENT].param_names[1] = "g1";
    s_meta[NODE_TYPE_GRADIENT].param_names[2] = "b1";
    s_meta[NODE_TYPE_GRADIENT].param_names[3] = "r2";
    s_meta[NODE_TYPE_GRADIENT].param_names[4] = "g2";
    s_meta[NODE_TYPE_GRADIENT].param_names[5] = "b2";
    s_meta[NODE_TYPE_GRADIENT].param_defaults[0] = 0.0f;
    s_meta[NODE_TYPE_GRADIENT].param_defaults[1] = 0.0f;
    s_meta[NODE_TYPE_GRADIENT].param_defaults[2] = 0.0f;
    s_meta[NODE_TYPE_GRADIENT].param_defaults[3] = 1.0f;
    s_meta[NODE_TYPE_GRADIENT].param_defaults[4] = 1.0f;
    s_meta[NODE_TYPE_GRADIENT].param_defaults[5] = 1.0f;
    s_meta[NODE_TYPE_GRADIENT].param_min[0] = 0.0f;
    s_meta[NODE_TYPE_GRADIENT].param_min[1] = 0.0f;
    s_meta[NODE_TYPE_GRADIENT].param_min[2] = 0.0f;
    s_meta[NODE_TYPE_GRADIENT].param_min[3] = 0.0f;
    s_meta[NODE_TYPE_GRADIENT].param_min[4] = 0.0f;
    s_meta[NODE_TYPE_GRADIENT].param_min[5] = 0.0f;
    s_meta[NODE_TYPE_GRADIENT].param_max[0] = 1.0f;
    s_meta[NODE_TYPE_GRADIENT].param_max[1] = 1.0f;
    s_meta[NODE_TYPE_GRADIENT].param_max[2] = 1.0f;
    s_meta[NODE_TYPE_GRADIENT].param_max[3] = 1.0f;
    s_meta[NODE_TYPE_GRADIENT].param_max[4] = 1.0f;
    s_meta[NODE_TYPE_GRADIENT].param_max[5] = 1.0f;

    /* NODE_TYPE_TRANSFORM2D */
    s_meta[NODE_TYPE_TRANSFORM2D].name = "Transform2D";
    s_meta[NODE_TYPE_TRANSFORM2D].num_inputs = 3;
    s_meta[NODE_TYPE_TRANSFORM2D].num_outputs = 3;
    s_meta[NODE_TYPE_TRANSFORM2D].num_params = 4;
    s_meta[NODE_TYPE_TRANSFORM2D].input_names[0] = "x";
    s_meta[NODE_TYPE_TRANSFORM2D].input_names[1] = "y";
    s_meta[NODE_TYPE_TRANSFORM2D].input_names[2] = "scale";
    s_meta[NODE_TYPE_TRANSFORM2D].output_names[0] = "x";
    s_meta[NODE_TYPE_TRANSFORM2D].output_names[1] = "y";
    s_meta[NODE_TYPE_TRANSFORM2D].output_names[2] = "scale";
    s_meta[NODE_TYPE_TRANSFORM2D].param_names[0] = "offset_x";
    s_meta[NODE_TYPE_TRANSFORM2D].param_names[1] = "offset_y";
    s_meta[NODE_TYPE_TRANSFORM2D].param_names[2] = "rotation";
    s_meta[NODE_TYPE_TRANSFORM2D].param_names[3] = "scale_mul";
    s_meta[NODE_TYPE_TRANSFORM2D].param_defaults[3] = 1.0f;

    /* NODE_TYPE_RENDER2D */
    s_meta[NODE_TYPE_RENDER2D].name = "Render2D";
    s_meta[NODE_TYPE_RENDER2D].num_inputs = 4;
    s_meta[NODE_TYPE_RENDER2D].num_outputs = 4;  /* x, y, w, h for render pass */
    s_meta[NODE_TYPE_RENDER2D].num_params = 4;
    s_meta[NODE_TYPE_RENDER2D].input_names[0] = "R";
    s_meta[NODE_TYPE_RENDER2D].input_names[1] = "G";
    s_meta[NODE_TYPE_RENDER2D].input_names[2] = "B";
    s_meta[NODE_TYPE_RENDER2D].input_names[3] = "A";
    s_meta[NODE_TYPE_RENDER2D].output_names[0] = "x";
    s_meta[NODE_TYPE_RENDER2D].output_names[1] = "y";
    s_meta[NODE_TYPE_RENDER2D].output_names[2] = "w";
    s_meta[NODE_TYPE_RENDER2D].output_names[3] = "h";
    s_meta[NODE_TYPE_RENDER2D].param_names[0] = "X";
    s_meta[NODE_TYPE_RENDER2D].param_names[1] = "Y";
    s_meta[NODE_TYPE_RENDER2D].param_names[2] = "W";
    s_meta[NODE_TYPE_RENDER2D].param_names[3] = "H";
    s_meta[NODE_TYPE_RENDER2D].param_defaults[0] = 0.3f;   /* Start at 30% from left */
    s_meta[NODE_TYPE_RENDER2D].param_defaults[1] = 0.3f;   /* Start at 30% from top */
    s_meta[NODE_TYPE_RENDER2D].param_defaults[2] = 0.4f;   /* 40% width */
    s_meta[NODE_TYPE_RENDER2D].param_defaults[3] = 0.4f;   /* 40% height */
    s_meta[NODE_TYPE_RENDER2D].param_min[0] = 0.0f;
    s_meta[NODE_TYPE_RENDER2D].param_min[1] = 0.0f;
    s_meta[NODE_TYPE_RENDER2D].param_min[2] = 0.0f;
    s_meta[NODE_TYPE_RENDER2D].param_min[3] = 0.0f;
    s_meta[NODE_TYPE_RENDER2D].param_max[0] = 1.0f;
    s_meta[NODE_TYPE_RENDER2D].param_max[1] = 1.0f;
    s_meta[NODE_TYPE_RENDER2D].param_max[2] = 1.0f;
    s_meta[NODE_TYPE_RENDER2D].param_max[3] = 2.0f;

    /* NODE_TYPE_RENDER_CIRCLE */
    s_meta[NODE_TYPE_RENDER_CIRCLE].name = "Circle";
    s_meta[NODE_TYPE_RENDER_CIRCLE].num_inputs = 4;
    s_meta[NODE_TYPE_RENDER_CIRCLE].num_outputs = 3;
    s_meta[NODE_TYPE_RENDER_CIRCLE].num_params = 3;
    s_meta[NODE_TYPE_RENDER_CIRCLE].input_names[0] = "R";
    s_meta[NODE_TYPE_RENDER_CIRCLE].input_names[1] = "G";
    s_meta[NODE_TYPE_RENDER_CIRCLE].input_names[2] = "B";
    s_meta[NODE_TYPE_RENDER_CIRCLE].input_names[3] = "A";
    s_meta[NODE_TYPE_RENDER_CIRCLE].output_names[0] = "x";
    s_meta[NODE_TYPE_RENDER_CIRCLE].output_names[1] = "y";
    s_meta[NODE_TYPE_RENDER_CIRCLE].output_names[2] = "r";
    s_meta[NODE_TYPE_RENDER_CIRCLE].param_names[0] = "X";
    s_meta[NODE_TYPE_RENDER_CIRCLE].param_names[1] = "Y";
    s_meta[NODE_TYPE_RENDER_CIRCLE].param_names[2] = "radius";
    s_meta[NODE_TYPE_RENDER_CIRCLE].param_defaults[0] = 0.5f;
    s_meta[NODE_TYPE_RENDER_CIRCLE].param_defaults[1] = 0.5f;
    s_meta[NODE_TYPE_RENDER_CIRCLE].param_defaults[2] = 0.1f;
    s_meta[NODE_TYPE_RENDER_CIRCLE].param_min[0] = 0.0f;
    s_meta[NODE_TYPE_RENDER_CIRCLE].param_min[1] = 0.0f;
    s_meta[NODE_TYPE_RENDER_CIRCLE].param_min[2] = 0.01f;
    s_meta[NODE_TYPE_RENDER_CIRCLE].param_max[0] = 1.0f;
    s_meta[NODE_TYPE_RENDER_CIRCLE].param_max[1] = 1.0f;
    s_meta[NODE_TYPE_RENDER_CIRCLE].param_max[2] = 0.5f;

    /* NODE_TYPE_RENDER_LINE */
    s_meta[NODE_TYPE_RENDER_LINE].name = "Line";
    s_meta[NODE_TYPE_RENDER_LINE].num_inputs = 4;
    s_meta[NODE_TYPE_RENDER_LINE].num_outputs = 4;
    s_meta[NODE_TYPE_RENDER_LINE].num_params = 4;
    s_meta[NODE_TYPE_RENDER_LINE].input_names[0] = "R";
    s_meta[NODE_TYPE_RENDER_LINE].input_names[1] = "G";
    s_meta[NODE_TYPE_RENDER_LINE].input_names[2] = "B";
    s_meta[NODE_TYPE_RENDER_LINE].input_names[3] = "A";
    s_meta[NODE_TYPE_RENDER_LINE].output_names[0] = "x1";
    s_meta[NODE_TYPE_RENDER_LINE].output_names[1] = "y1";
    s_meta[NODE_TYPE_RENDER_LINE].output_names[2] = "x2";
    s_meta[NODE_TYPE_RENDER_LINE].output_names[3] = "y2";
    s_meta[NODE_TYPE_RENDER_LINE].param_names[0] = "X1";
    s_meta[NODE_TYPE_RENDER_LINE].param_names[1] = "Y1";
    s_meta[NODE_TYPE_RENDER_LINE].param_names[2] = "X2";
    s_meta[NODE_TYPE_RENDER_LINE].param_names[3] = "Y2";
    s_meta[NODE_TYPE_RENDER_LINE].param_defaults[0] = 0.2f;
    s_meta[NODE_TYPE_RENDER_LINE].param_defaults[1] = 0.2f;
    s_meta[NODE_TYPE_RENDER_LINE].param_defaults[2] = 0.8f;
    s_meta[NODE_TYPE_RENDER_LINE].param_defaults[3] = 0.8f;
    s_meta[NODE_TYPE_RENDER_LINE].param_min[0] = 0.0f;
    s_meta[NODE_TYPE_RENDER_LINE].param_min[1] = 0.0f;
    s_meta[NODE_TYPE_RENDER_LINE].param_min[2] = 0.0f;
    s_meta[NODE_TYPE_RENDER_LINE].param_min[3] = 0.0f;
    s_meta[NODE_TYPE_RENDER_LINE].param_max[0] = 1.0f;
    s_meta[NODE_TYPE_RENDER_LINE].param_max[1] = 1.0f;
    s_meta[NODE_TYPE_RENDER_LINE].param_max[2] = 1.0f;
    s_meta[NODE_TYPE_RENDER_LINE].param_max[3] = 1.0f;

    /* NODE_TYPE_DEBUG */
    s_meta[NODE_TYPE_DEBUG].name = "Debug";
    s_meta[NODE_TYPE_DEBUG].num_inputs = 4;
    s_meta[NODE_TYPE_DEBUG].num_outputs = 4;
    s_meta[NODE_TYPE_DEBUG].num_params = 0;
    s_meta[NODE_TYPE_DEBUG].input_names[0] = "in0";
    s_meta[NODE_TYPE_DEBUG].input_names[1] = "in1";
    s_meta[NODE_TYPE_DEBUG].input_names[2] = "in2";
    s_meta[NODE_TYPE_DEBUG].input_names[3] = "in3";
    s_meta[NODE_TYPE_DEBUG].output_names[0] = "out0";
    s_meta[NODE_TYPE_DEBUG].output_names[1] = "out1";
    s_meta[NODE_TYPE_DEBUG].output_names[2] = "out2";
    s_meta[NODE_TYPE_DEBUG].output_names[3] = "out3";
}

/* ============================================================
 * Initialize Registry
 * ============================================================ */
void node_registry_init(void)
{
    int i;

    if (s_initialized) {
        return;
    }

    /* Default all to fallback */
    for (i = 0; i < NODE_TYPE_COUNT; i++) {
        s_eval_funcs[i] = node_eval_none;
    }

    /* Register actual implementations */
    /* Basic nodes */
    s_eval_funcs[NODE_TYPE_CONST] = node_eval_const;
    s_eval_funcs[NODE_TYPE_TIME] = node_eval_time;
    s_eval_funcs[NODE_TYPE_PAD] = node_eval_pad;
    s_eval_funcs[NODE_TYPE_NOISE] = node_eval_noise;
    s_eval_funcs[NODE_TYPE_LFO] = node_eval_lfo;
    /* Math */
    s_eval_funcs[NODE_TYPE_ADD] = node_eval_add;
    s_eval_funcs[NODE_TYPE_MUL] = node_eval_mul;
    s_eval_funcs[NODE_TYPE_SUB] = node_eval_sub;
    s_eval_funcs[NODE_TYPE_DIV] = node_eval_div;
    s_eval_funcs[NODE_TYPE_MOD] = node_eval_mod;
    s_eval_funcs[NODE_TYPE_ABS] = node_eval_abs;
    s_eval_funcs[NODE_TYPE_NEG] = node_eval_neg;
    s_eval_funcs[NODE_TYPE_MIN] = node_eval_min;
    s_eval_funcs[NODE_TYPE_MAX] = node_eval_max;
    s_eval_funcs[NODE_TYPE_CLAMP] = node_eval_clamp;
    s_eval_funcs[NODE_TYPE_MAP] = node_eval_map;
    /* Trig */
    s_eval_funcs[NODE_TYPE_SIN] = node_eval_sin;
    s_eval_funcs[NODE_TYPE_COS] = node_eval_cos;
    s_eval_funcs[NODE_TYPE_TAN] = node_eval_tan;
    s_eval_funcs[NODE_TYPE_ATAN2] = node_eval_atan2;
    /* Filters */
    s_eval_funcs[NODE_TYPE_LERP] = node_eval_lerp;
    s_eval_funcs[NODE_TYPE_SMOOTH] = node_eval_smooth;
    s_eval_funcs[NODE_TYPE_STEP] = node_eval_step;
    s_eval_funcs[NODE_TYPE_PULSE] = node_eval_pulse;
    s_eval_funcs[NODE_TYPE_HOLD] = node_eval_hold;
    s_eval_funcs[NODE_TYPE_DELAY] = node_eval_delay;
    /* Logic */
    s_eval_funcs[NODE_TYPE_COMPARE] = node_eval_compare;
    s_eval_funcs[NODE_TYPE_SELECT] = node_eval_select;
    s_eval_funcs[NODE_TYPE_GATE] = node_eval_gate;
    /* Vector/Color */
    s_eval_funcs[NODE_TYPE_SPLIT] = node_eval_split;
    s_eval_funcs[NODE_TYPE_COMBINE] = node_eval_combine;
    s_eval_funcs[NODE_TYPE_COLORIZE] = node_eval_colorize;
    s_eval_funcs[NODE_TYPE_HSV] = node_eval_hsv;
    s_eval_funcs[NODE_TYPE_GRADIENT] = node_eval_gradient;
    /* Transform */
    s_eval_funcs[NODE_TYPE_TRANSFORM2D] = node_eval_transform2d;
    /* Sinks */
    s_eval_funcs[NODE_TYPE_RENDER2D] = node_eval_render2d;
    s_eval_funcs[NODE_TYPE_RENDER_CIRCLE] = node_eval_render_circle;
    s_eval_funcs[NODE_TYPE_RENDER_LINE] = node_eval_render_line;
    /* Utility */
    s_eval_funcs[NODE_TYPE_DEBUG] = node_eval_debug;

    /* Initialize metadata */
    init_meta();

    s_initialized = 1;
}

/* ============================================================
 * Get Eval Function
 * ============================================================ */
NodeEvalFunc node_registry_get_eval(NodeType type)
{
    if (!s_initialized) {
        return node_eval_none;
    }
    if (type >= NODE_TYPE_COUNT) {
        return node_eval_none;
    }
    return s_eval_funcs[type];
}

/* ============================================================
 * Get Metadata
 * ============================================================ */
const NodeMeta *node_registry_get_meta(NodeType type)
{
    if (!s_initialized) {
        return NULL;
    }
    if (type >= NODE_TYPE_COUNT) {
        return &s_meta[NODE_TYPE_NONE];
    }
    return &s_meta[type];
}

/* ============================================================
 * Get Name
 * ============================================================ */
const char *node_registry_get_name(NodeType type)
{
    if (!s_initialized) {
        return "Unknown";
    }
    if (type >= NODE_TYPE_COUNT) {
        return "Unknown";
    }
    return s_meta[type].name;
}

/* ============================================================
 * Check if Source (no inputs)
 * ============================================================ */
int node_registry_is_source(NodeType type)
{
    if (!s_initialized || type >= NODE_TYPE_COUNT) {
        return 0;
    }
    return s_meta[type].num_inputs == 0;
}

/* ============================================================
 * Check if Sink (no outputs)
 * ============================================================ */
int node_registry_is_sink(NodeType type)
{
    if (!s_initialized || type >= NODE_TYPE_COUNT) {
        return 0;
    }
    return s_meta[type].num_outputs == 0;
}
