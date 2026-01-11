#ifndef GRAPH_TYPES_H
#define GRAPH_TYPES_H

#include "../common.h"

/* ============================================================
 * Node Types (MVP)
 * ============================================================ */
typedef enum {
    NODE_TYPE_NONE = 0,
    /* Sources (no inputs) */
    NODE_TYPE_CONST,
    NODE_TYPE_TIME,
    NODE_TYPE_PAD,
    NODE_TYPE_NOISE,        /* Random noise generator */
    NODE_TYPE_LFO,          /* Low-frequency oscillator */
    /* Math */
    NODE_TYPE_ADD,
    NODE_TYPE_MUL,
    NODE_TYPE_SUB,          /* Subtract */
    NODE_TYPE_DIV,          /* Divide */
    NODE_TYPE_MOD,          /* Modulo */
    NODE_TYPE_ABS,          /* Absolute value */
    NODE_TYPE_NEG,          /* Negate */
    NODE_TYPE_MIN,          /* Minimum of two values */
    NODE_TYPE_MAX,          /* Maximum of two values */
    NODE_TYPE_CLAMP,        /* Clamp to range */
    NODE_TYPE_MAP,          /* Remap value range */
    /* Trigonometry */
    NODE_TYPE_SIN,
    NODE_TYPE_COS,          /* Cosine */
    NODE_TYPE_TAN,          /* Tangent */
    NODE_TYPE_ATAN2,        /* Arctangent of y/x */
    /* Filters */
    NODE_TYPE_LERP,
    NODE_TYPE_SMOOTH,
    NODE_TYPE_STEP,         /* Step function (threshold) */
    NODE_TYPE_PULSE,        /* Pulse/trigger generator */
    NODE_TYPE_HOLD,         /* Sample and hold */
    NODE_TYPE_DELAY,        /* Delay by N frames */
    /* Logic/Comparison */
    NODE_TYPE_COMPARE,      /* Compare two values */
    NODE_TYPE_SELECT,       /* Select between inputs */
    NODE_TYPE_GATE,         /* Gate signal by threshold */
    /* Vector/Color */
    NODE_TYPE_SPLIT,        /* Split vector to components */
    NODE_TYPE_COMBINE,      /* Combine components to vector */
    NODE_TYPE_COLORIZE,
    NODE_TYPE_HSV,          /* HSV to RGB conversion */
    NODE_TYPE_GRADIENT,     /* Multi-stop gradient */
    /* Transform */
    NODE_TYPE_TRANSFORM2D,
    /* Sinks */
    NODE_TYPE_RENDER2D,
    NODE_TYPE_RENDER_CIRCLE,/* Render filled circle */
    NODE_TYPE_RENDER_LINE,  /* Render line */
    /* Utility */
    NODE_TYPE_DEBUG,
    NODE_TYPE_COUNT
} NodeType;

/* ============================================================
 * Connection (input reference)
 * ============================================================ */
typedef struct {
    NodeId   src_node;    /* Source node ID, INVALID_NODE_ID if disconnected */
    uint8_t  src_port;    /* Source output port index */
    uint8_t  _pad[1];     /* Padding for alignment */
} Connection;

/* ============================================================
 * Node
 * ============================================================ */
typedef struct {
    NodeType    type;
    Connection  inputs[MAX_IN_PORTS];
    float       params[MAX_PARAMS];
    uint32_t    state_u32[MAX_NODE_STATE];
} Node;

/* ============================================================
 * Graph
 * ============================================================ */
typedef struct {
    Node     nodes[MAX_NODES];
    uint16_t node_count;              /* Number of allocated nodes */
    uint16_t version;                 /* Incremented on each commit */
} Graph;

/* ============================================================
 * OutputBank (evaluation outputs)
 * ============================================================ */
typedef struct {
    float out[MAX_NODES][MAX_OUT_PORTS];
} OutputBank;

/* ============================================================
 * EvalPlan (topological order for evaluation)
 * ============================================================ */
typedef struct {
    uint16_t count;
    NodeId   sink_id;
    NodeId   order[MAX_NODES];
} EvalPlan;

/* ============================================================
 * UiMeta (per-node UI metadata for editor)
 * ============================================================ */
typedef struct {
    float x;                          /* UI X position */
    float y;                          /* UI Y position */
    uint8_t selected;                 /* Selection flag */
    uint8_t collapsed;                /* Collapsed in UI */
    uint8_t _pad[2];                  /* Padding for alignment */
} UiMeta;

/* ============================================================
 * UiMetaBank (UI metadata for all nodes)
 * ============================================================ */
typedef struct {
    UiMeta meta[MAX_NODES];
} UiMetaBank;

#endif /* GRAPH_TYPES_H */
