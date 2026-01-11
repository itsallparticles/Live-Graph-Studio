#ifndef NODE_REGISTRY_H
#define NODE_REGISTRY_H

#include <stdint.h>
#include "../graph/graph_types.h"
#include "../runtime/runtime.h"

/* ============================================================
 * Node Evaluation Function Signature
 * ============================================================
 * Each node type has an eval function with this signature.
 * - node: pointer to the node being evaluated
 * - inputs: input values from connected nodes (in[port])
 * - outputs: output values to write (out[port])
 * - ctx: runtime context (time, dt, pad state, etc.)
 * ============================================================ */
typedef void (*NodeEvalFunc)(const Node *node,
                             const float inputs[MAX_IN_PORTS],
                             float outputs[MAX_OUT_PORTS],
                             const RuntimeContext *ctx);

/* ============================================================
 * Node Metadata
 * ============================================================ */
typedef struct {
    const char   *name;              /* Display name */
    uint8_t       num_inputs;        /* Number of input ports used */
    uint8_t       num_outputs;       /* Number of output ports used */
    uint8_t       num_params;        /* Number of params used */
    const char   *input_names[MAX_IN_PORTS];
    const char   *output_names[MAX_OUT_PORTS];
    const char   *param_names[MAX_PARAMS];
    float         param_defaults[MAX_PARAMS];
    float         param_min[MAX_PARAMS];
    float         param_max[MAX_PARAMS];
} NodeMeta;

/* ============================================================
 * Registry API
 * ============================================================ */

/* Initialize the node registry (call once at startup) */
void node_registry_init(void);

/* Get eval function for a node type. Returns fallback if not initialized. */
NodeEvalFunc node_registry_get_eval(NodeType type);

/* Get metadata for a node type. Returns NULL if registry not initialized. */
const NodeMeta *node_registry_get_meta(NodeType type);

/* Get name string for a node type */
const char *node_registry_get_name(NodeType type);

/* Check if node type is a source (no inputs) */
int node_registry_is_source(NodeType type);

/* Check if node type is a sink (no outputs, renders) */
int node_registry_is_sink(NodeType type);

#endif /* NODE_REGISTRY_H */
